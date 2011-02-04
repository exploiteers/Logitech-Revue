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
	unsigned int sg_max, sg_idx, sig_size, sha1_size;
	u8 sha1_result[SHA1_DIGEST_SIZE];
	unsigned char trailer[6];
	unsigned char *ptr;
	int i, ptr_len;

	struct sechdr {
		uint32_t type;
		uint32_t flags;
		uint32_t size;
	} __attribute__((__packed__)) *sechdr;


	/* module.c does not fully verify sechdrs and sectrings */
	if (hdr->e_shoff > len) {
		return -ENOEXEC; /* truncated */
	}
	sechdrs = (void *)hdr + hdr->e_shoff;

	if (hdr->e_shnum < hdr->e_shstrndx) {
		return -EINVAL; /* string section out of range */
	}
	if (len < sechdrs[hdr->e_shstrndx].sh_offset + sechdrs[hdr->e_shstrndx].sh_size) {
		return -ENOEXEC; /* truncated */
	}
	secstrings = (void *)hdr + sechdrs[hdr->e_shstrndx].sh_offset;


	/* verify section headers and look for signature */
	sg_max = 0;
	for (i = 1; i < hdr->e_shnum; i++) {
		const char *name;

		if (sechdrs[i].sh_type != SHT_NOBITS
		    && len < sechdrs[i].sh_offset + sechdrs[i].sh_size) {
			return -ENOEXEC; /* truncated */
		}

		name = find_name(hdr, sechdrs, secstrings, i);
		if (name && strcmp(name, ".signature") == 0) {
			sig_index = i;
		}

		sg_max += (sechdrs[i].sh_size / PAGE_SIZE) + 4;
	}
	sg_max += 2;

	if (sig_index <= 0) {
		return -EINVAL;
	}

	sig = (unsigned char *)hdr + sechdrs[sig_index].sh_offset;
	sig_size = sechdrs[sig_index].sh_size;

	// TODO(richard) this only parses 2048 bit RSA keys

	/* verify gpg packet */
	if (sig[0] != 0x89) {
		return -EINVAL;
	}

	/* verify gpg packet length */
	ptr_len = (sig[1] << 8) | sig[2];
	if (ptr_len + 3 > sig_size) {
		return -EINVAL;
	}

	/* verify gpg signature packet */
	ptr = sig + 3;
	if (ptr[0] != 0x04 || /* version number */
	    ptr[1] != 0x00 || /* signature type */
	    ptr[2] != 0x01 || /* public-key algorithm RSA*/
	    ptr[3] != 0x02    /* hash algorithm SHA1 */) {
		return -EINVAL;
	}

	/* alloc hash */
	sha1_tfm = crypto_alloc_hash("sha1", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(sha1_tfm)) {
		return -ENOMEM;
	}
	desc.tfm = sha1_tfm;
	desc.flags = 0;

	sg = kmalloc(sizeof(struct scatterlist) * sg_max, GFP_KERNEL);
	if (!sg) {
		crypto_free_hash(sha1_tfm);
		return -ENOMEM;
	}

	sechdr = kmalloc(sizeof(struct sechdr) * hdr->e_shnum, GFP_KERNEL);
	if (!sechdr) {
		kfree(sg);
		crypto_free_hash(sha1_tfm);
		return -ENOMEM;
	}

	sg_idx = 0;
	sha1_size = 0;
	for (i = 1; i < hdr->e_shnum; i++) {
		void *addr = (void *)hdr + sechdrs[i].sh_offset;
		int size = sechdrs[i].sh_size;
		const char *name = find_name(hdr, sechdrs, secstrings, i);

		/* Don't hash the signature section */
		if (i == sig_index) {
			continue;
		}

		/* name */
		sg_idx = sg_add_buf(sg, sg_idx, name, strlen(name));
		sha1_size += strlen(name);

		/* headers */
		sechdr[i].type = htonl(sechdrs[i].sh_type);
		sechdr[i].flags = htonl(sechdrs[i].sh_flags);
		sechdr[i].size = htonl(size);

		sg_idx = sg_add_buf(sg, sg_idx, &sechdr[i], sizeof(struct sechdr));
		sha1_size += sizeof(struct sechdr);

		/* data */
		if (!size || sechdrs[i].sh_type == SHT_NOBITS) {
			continue;
		}

		sg_idx = sg_add_buf(sg, sg_idx, addr, size);
		sha1_size += size;
	}

	/* hash signature packet body */
	ptr_len = 6 + (ptr[4] << 8) + ptr[5];

	sg_idx = sg_add_buf(sg, sg_idx, ptr, ptr_len);
	sha1_size += ptr_len;

	ptr += ptr_len;

	/* hash signature trailer */
	trailer[0] = 0x04;
	trailer[1] = 0xff;
	trailer[2] = ptr_len >> 24;
	trailer[3] = ptr_len >> 16;
	trailer[4] = ptr_len >> 8;
	trailer[5] = ptr_len;

	sg_idx = sg_add_buf(sg, sg_idx, trailer, 6);
	sha1_size += 6;

	/* generate hash */
	crypto_hash_digest(&desc, sg, sha1_size, sha1_result);
	crypto_free_hash(sha1_tfm);
	kfree(sg);
	kfree(sechdr);

	/* skip unhashed subpacket data */
	ptr_len = 2 + (ptr[0] << 8) + ptr[1];
	ptr += ptr_len;

	if (0) {
		printk(KERN_INFO "sig begin digest %02x %02x\n", ptr[0], ptr[1]);
		printk(KERN_INFO "module digest: ");
		for (i = 0; i < sizeof(sha1_result); ++i)
			printk("%02x ", sha1_result[i]);
		printk("\n");
	}

	/* check left 16 bits of signed hash value */
	if (ptr[0] != sha1_result[0] && ptr[1] != sha1_result[1]) {
		return -EPERM;
	}
	ptr += 2;

	/* see if this sha1 is really encrypted in the section */
	return (rsa_verify_sig(ptr, sig + sig_size - ptr, sha1_result) == 0) ? 0 : -EPERM;
}

