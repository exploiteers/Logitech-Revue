#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/elf.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <asm/scatterlist.h>

#include <linux/rsa_verify.h>

#define SHA1_DIGEST_SIZE        20

/*
 * DEBUG enables extra prints and DISABLES the security.
 * Don't define in production releases!
 */
//#define DEBUG 1

#ifdef DEBUG
#warning **** MODULE SECURITY DISABLED ****
#define pr_debug1(...)	printk(KERN_DEBUG __VA_ARGS__)
#else
#define pr_debug1(...)
#endif


static inline unsigned int sg_add_buf(struct scatterlist *sg, unsigned int sg_idx, const void *buf, unsigned long buflen)
{
	int len;

	do {
		sg[sg_idx].page = virt_to_page(buf);
		sg[sg_idx].offset = offset_in_page(buf);
		len = min(buflen, (PAGE_SIZE - sg[sg_idx].offset));
		sg[sg_idx++].length = len;

		buf += len;
		buflen -= len;
	} while (buflen > 0);

	return sg_idx;
}

static inline const char *find_name(Elf_Ehdr *hdr, Elf_Shdr *sechdrs, const char *secstrings, int idx)
{
	if (hdr->e_shnum < idx
	    || sechdrs[hdr->e_shstrndx].sh_size < sechdrs[idx].sh_name) {
		return NULL;
	}

	/* we verified the string table is null terminated, so this string must be */
	return secstrings + sechdrs[idx].sh_name;
}


int module_check_sig(Elf_Ehdr *hdr, unsigned int len)
{
	Elf_Shdr *sechdrs;
	const char *secstrings;
	static struct crypto_hash *sha1_tfm;
	struct hash_desc desc;
	struct scatterlist *sg;
	unsigned int sig_index = 0;
	unsigned char *sig;
	unsigned int sg_size, sg_idx;
	unsigned int sig_size, sig_offset, sha1_size;
	u8 sha1_result[SHA1_DIGEST_SIZE];
	unsigned char trailer[4];
	int ret, i;

	if (len > 10 * 1024 * 1024) {
		return -ENOEXEC; /* sanity check length to 10M */
	}

	/* module.c does not fully verify sechdrs and sectrings */
	if (hdr->e_shoff > len) {
		return -ENOEXEC; /* truncated */
	}
	sechdrs = (void *)hdr + hdr->e_shoff;

	if (hdr->e_shnum < hdr->e_shstrndx) {
		return -EINVAL; /* string section out of range */
	}
	if (len < sechdrs[hdr->e_shstrndx].sh_offset ||
	    len < sechdrs[hdr->e_shstrndx].sh_size ||
	    len < sechdrs[hdr->e_shstrndx].sh_offset + sechdrs[hdr->e_shstrndx].sh_size) {
		return -ENOEXEC; /* truncated or overflow */
	}
	secstrings = (void *)hdr + sechdrs[hdr->e_shstrndx].sh_offset;

	/* first and last byte of of the string table must be null */
	if (*secstrings || *(secstrings + sechdrs[hdr->e_shstrndx].sh_size)) {
		pr_debug("String null check failed\n");
		return -ENOEXEC;
	}

	/* find and verify signature section */
	for (i = 1; i < hdr->e_shnum; i++) {
		const char *name;

		name = find_name(hdr, sechdrs, secstrings, i);
		if (name && strcmp(name, ".signature") == 0) {
			sig_index = i;
			break;
		}
	}
	if (sig_index == 0) {
		pr_debug("No module signature\n");
		return -EINVAL;
	}

	sig_offset = sechdrs[sig_index].sh_offset;
	sig_size = sechdrs[sig_index].sh_size;

	if (len < sig_offset ||
	    len < sig_size ||
	    len < sig_offset + sig_size ||
	    sig_size < 512) {
		pr_debug("Signature size/offset failed\n");
		return -EINVAL;
	}

	sig = (unsigned char *)hdr + sig_offset;

	/* verify signature magic */
	if (sig[0] != 0x53 || sig[1] != 0x69 || 
	    sig[2] != 0x67 || sig[3] != 0x6e) {
		pr_debug("Signature magic failed\n");
		return -EINVAL;
	}

	/* verify signature version */
	if (sig[4] != 0x00 || sig[5] != 0x00 || 
	    sig[6] != 0x00 || sig[7] != 0x01) {
		pr_debug("Signature version failed\n");
		return -EINVAL;
	}

	/* verify no other sections overlap signature */
	for (i = 1; i < hdr->e_shnum; i++) {
		if (i == sig_index || sechdrs[i].sh_type == SHT_NOBITS) {
			continue;
		}

		if (sechdrs[i].sh_offset <= sig_offset) {
			if (sechdrs[i].sh_size > sig_offset ||
			    sechdrs[i].sh_offset + sechdrs[i].sh_size > sig_offset) {
				pr_debug("Section overlap\n");
				return -EINVAL;
			}
		}
		else {
			if (sechdrs[i].sh_offset < sig_offset + sig_size) {
				pr_debug("Section overlap\n");
				return -EINVAL;
			}
		}
	}

	/* alloc hash and sg_list */
	sha1_tfm = crypto_alloc_hash("sha1", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(sha1_tfm)) {
		return -ENOMEM;
	}
	desc.tfm = sha1_tfm;
	desc.flags = 0;

	sg_size = (len / PAGE_SIZE) + 4;
	sg = kmalloc(sizeof(struct scatterlist) * sg_size, GFP_KERNEL);
	if (!sg) {
		crypto_free_hash(sha1_tfm);
		return -ENOMEM;
	}

	/* build sg list */
	sg_idx = 0;
	sha1_size = 0;

	/* sections before signature */
	sg_idx = sg_add_buf(sg, sg_idx, hdr, sig_offset);
	BUG_ON(sg_idx > sg_size);
	sha1_size += sig_offset;

	/* sections after signature */
	sg_idx = sg_add_buf(sg, sg_idx, sig + sig_size, len - sig_offset - sig_size);
	BUG_ON(sg_idx > sg_size);
	sha1_size += len - sig_offset - sig_size;

	trailer[0] = sha1_size >> 24;
	trailer[1] = sha1_size >> 16;
	trailer[2] = sha1_size >> 8;
	trailer[3] = sha1_size;

	/* signature header */
	sg_idx = sg_add_buf(sg, sg_idx, sig, 8);
	BUG_ON(sg_idx > sg_size);
	sha1_size += 8;

	/* section size */
	sg_idx = sg_add_buf(sg, sg_idx, trailer, 4);
	BUG_ON(sg_idx > sg_size);
	sha1_size += 4;

	/* generate hash */
	crypto_hash_digest(&desc, sg, sha1_size, sha1_result);
	crypto_free_hash(sha1_tfm);
	kfree(sg);

#ifdef DEBUG
	printk(KERN_INFO "sha1: ");
	for (i=0; i<SHA1_DIGEST_SIZE; i++) {
		printk("%02x ", sha1_result[i]);
	}
	printk("\n");
#endif

	/* verify signature */
	ret = (rsa_verify_sig(sig + 8, sig_size - 8, sha1_result) == 0) ? 0 : -EPERM;

#ifdef DEBUG
	printk(KERN_INFO "modsig debug: verify %s\n", (ret == -EPERM) ? "failure" : "ok");
	return 0;
#else
	return ret;
#endif
}

