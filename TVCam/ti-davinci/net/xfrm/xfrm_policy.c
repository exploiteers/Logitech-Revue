/*
 * xfrm_policy.c
 *
 * Changes:
 *	Mitsuru KANDA @USAGI
 * 	Kazunori MIYAZAWA @USAGI
 * 	Kunihiro Ishiguro <kunihiro@ipinfusion.com>
 * 		IPv6 support
 * 	Kazunori MIYAZAWA @USAGI
 * 	YOSHIFUJI Hideaki
 * 		Split up af-specific portion
 *	Derek Atkins <derek@ihtfp.com>		Add the post_input processor
 *
 */

#include <linux/slab.h>
#include <linux/kmod.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/module.h>
#include <net/xfrm.h>
#include <net/ip.h>

DEFINE_MUTEX(xfrm_cfg_mutex);
EXPORT_SYMBOL(xfrm_cfg_mutex);

static DEFINE_RWLOCK(xfrm_policy_lock);

struct xfrm_policy *xfrm_policy_list[XFRM_POLICY_MAX*2];
EXPORT_SYMBOL(xfrm_policy_list);

static DEFINE_RWLOCK(xfrm_policy_afinfo_lock);
static struct xfrm_policy_afinfo *xfrm_policy_afinfo[NPROTO];

static kmem_cache_t *xfrm_dst_cache __read_mostly;

static struct work_struct xfrm_policy_gc_work;
static struct list_head xfrm_policy_gc_list =
	LIST_HEAD_INIT(xfrm_policy_gc_list);
static DEFINE_SPINLOCK(xfrm_policy_gc_lock);

static struct xfrm_policy_afinfo *xfrm_policy_get_afinfo(unsigned short family);
static void xfrm_policy_put_afinfo(struct xfrm_policy_afinfo *afinfo);
static struct xfrm_policy_afinfo *xfrm_policy_lock_afinfo(unsigned int family);
static void xfrm_policy_unlock_afinfo(struct xfrm_policy_afinfo *afinfo);

int xfrm_register_type(struct xfrm_type *type, unsigned short family)
{
	struct xfrm_policy_afinfo *afinfo = xfrm_policy_lock_afinfo(family);
	struct xfrm_type **typemap;
	int err = 0;

	if (unlikely(afinfo == NULL))
		return -EAFNOSUPPORT;
	typemap = afinfo->type_map;

	if (likely(typemap[type->proto] == NULL))
		typemap[type->proto] = type;
	else
		err = -EEXIST;
	xfrm_policy_unlock_afinfo(afinfo);
	return err;
}
EXPORT_SYMBOL(xfrm_register_type);

int xfrm_unregister_type(struct xfrm_type *type, unsigned short family)
{
	struct xfrm_policy_afinfo *afinfo = xfrm_policy_lock_afinfo(family);
	struct xfrm_type **typemap;
	int err = 0;

	if (unlikely(afinfo == NULL))
		return -EAFNOSUPPORT;
	typemap = afinfo->type_map;

	if (unlikely(typemap[type->proto] != type))
		err = -ENOENT;
	else
		typemap[type->proto] = NULL;
	xfrm_policy_unlock_afinfo(afinfo);
	return err;
}
EXPORT_SYMBOL(xfrm_unregister_type);

struct xfrm_type *xfrm_get_type(u8 proto, unsigned short family)
{
	struct xfrm_policy_afinfo *afinfo;
	struct xfrm_type **typemap;
	struct xfrm_type *type;
	int modload_attempted = 0;

retry:
	afinfo = xfrm_policy_get_afinfo(family);
	if (unlikely(afinfo == NULL))
		return NULL;
	typemap = afinfo->type_map;

	type = typemap[proto];
	if (unlikely(type && !try_module_get(type->owner)))
		type = NULL;
	if (!type && !modload_attempted) {
		xfrm_policy_put_afinfo(afinfo);
		request_module("xfrm-type-%d-%d",
			       (int) family, (int) proto);
		modload_attempted = 1;
		goto retry;
	}

	xfrm_policy_put_afinfo(afinfo);
	return type;
}

int xfrm_dst_lookup(struct xfrm_dst **dst, struct flowi *fl,
		    unsigned short family)
{
	struct xfrm_policy_afinfo *afinfo = xfrm_policy_get_afinfo(family);
	int err = 0;

	if (unlikely(afinfo == NULL))
		return -EAFNOSUPPORT;

	if (likely(afinfo->dst_lookup != NULL))
		err = afinfo->dst_lookup(dst, fl);
	else
		err = -EINVAL;
	xfrm_policy_put_afinfo(afinfo);
	return err;
}
EXPORT_SYMBOL(xfrm_dst_lookup);

void xfrm_put_type(struct xfrm_type *type)
{
	module_put(type->owner);
}

int xfrm_register_mode(struct xfrm_mode *mode, int family)
{
	struct xfrm_policy_afinfo *afinfo;
	struct xfrm_mode **modemap;
	int err;

	if (unlikely(mode->encap >= XFRM_MODE_MAX))
		return -EINVAL;

	afinfo = xfrm_policy_lock_afinfo(family);
	if (unlikely(afinfo == NULL))
		return -EAFNOSUPPORT;

	err = -EEXIST;
	modemap = afinfo->mode_map;
	if (likely(modemap[mode->encap] == NULL)) {
		modemap[mode->encap] = mode;
		err = 0;
	}

	xfrm_policy_unlock_afinfo(afinfo);
	return err;
}
EXPORT_SYMBOL(xfrm_register_mode);

int xfrm_unregister_mode(struct xfrm_mode *mode, int family)
{
	struct xfrm_policy_afinfo *afinfo;
	struct xfrm_mode **modemap;
	int err;

	if (unlikely(mode->encap >= XFRM_MODE_MAX))
		return -EINVAL;

	afinfo = xfrm_policy_lock_afinfo(family);
	if (unlikely(afinfo == NULL))
		return -EAFNOSUPPORT;

	err = -ENOENT;
	modemap = afinfo->mode_map;
	if (likely(modemap[mode->encap] == mode)) {
		modemap[mode->encap] = NULL;
		err = 0;
	}

	xfrm_policy_unlock_afinfo(afinfo);
	return err;
}
EXPORT_SYMBOL(xfrm_unregister_mode);

struct xfrm_mode *xfrm_get_mode(unsigned int encap, int family)
{
	struct xfrm_policy_afinfo *afinfo;
	struct xfrm_mode *mode;
	int modload_attempted = 0;

	if (unlikely(encap >= XFRM_MODE_MAX))
		return NULL;

retry:
	afinfo = xfrm_policy_get_afinfo(family);
	if (unlikely(afinfo == NULL))
		return NULL;

	mode = afinfo->mode_map[encap];
	if (unlikely(mode && !try_module_get(mode->owner)))
		mode = NULL;
	if (!mode && !modload_attempted) {
		xfrm_policy_put_afinfo(afinfo);
		request_module("xfrm-mode-%d-%d", family, encap);
		modload_attempted = 1;
		goto retry;
	}

	xfrm_policy_put_afinfo(afinfo);
	return mode;
}

void xfrm_put_mode(struct xfrm_mode *mode)
{
	module_put(mode->owner);
}

static inline unsigned long make_jiffies(long secs)
{
	if (secs >= (MAX_SCHEDULE_TIMEOUT-1)/HZ)
		return MAX_SCHEDULE_TIMEOUT-1;
	else
	        return secs*HZ;
}

static void xfrm_policy_timer(unsigned long data)
{
	struct xfrm_policy *xp = (struct xfrm_policy*)data;
	unsigned long now = (unsigned long)xtime.tv_sec;
	long next = LONG_MAX;
	int warn = 0;
	int dir;

	read_lock(&xp->lock);

	if (xp->dead)
		goto out;

	dir = xfrm_policy_id2dir(xp->index);

	if (xp->lft.hard_add_expires_seconds) {
		long tmo = xp->lft.hard_add_expires_seconds +
			xp->curlft.add_time - now;
		if (tmo <= 0)
			goto expired;
		if (tmo < next)
			next = tmo;
	}
	if (xp->lft.hard_use_expires_seconds) {
		long tmo = xp->lft.hard_use_expires_seconds +
			(xp->curlft.use_time ? : xp->curlft.add_time) - now;
		if (tmo <= 0)
			goto expired;
		if (tmo < next)
			next = tmo;
	}
	if (xp->lft.soft_add_expires_seconds) {
		long tmo = xp->lft.soft_add_expires_seconds +
			xp->curlft.add_time - now;
		if (tmo <= 0) {
			warn = 1;
			tmo = XFRM_KM_TIMEOUT;
		}
		if (tmo < next)
			next = tmo;
	}
	if (xp->lft.soft_use_expires_seconds) {
		long tmo = xp->lft.soft_use_expires_seconds +
			(xp->curlft.use_time ? : xp->curlft.add_time) - now;
		if (tmo <= 0) {
			warn = 1;
			tmo = XFRM_KM_TIMEOUT;
		}
		if (tmo < next)
			next = tmo;
	}

	if (warn)
		km_policy_expired(xp, dir, 0, 0);
	if (next != LONG_MAX &&
	    !mod_timer(&xp->timer, jiffies + make_jiffies(next)))
		xfrm_pol_hold(xp);

out:
	read_unlock(&xp->lock);
	xfrm_pol_put(xp);
	return;

expired:
	read_unlock(&xp->lock);
	if (!xfrm_policy_delete(xp, dir))
		km_policy_expired(xp, dir, 1, 0);
	xfrm_pol_put(xp);
}


/* Allocate xfrm_policy. Not used here, it is supposed to be used by pfkeyv2
 * SPD calls.
 */

struct xfrm_policy *xfrm_policy_alloc(gfp_t gfp)
{
	struct xfrm_policy *policy;

	policy = kzalloc(sizeof(struct xfrm_policy), gfp);

	if (policy) {
		atomic_set(&policy->refcnt, 1);
		rwlock_init(&policy->lock);
		init_timer(&policy->timer);
		policy->timer.data = (unsigned long)policy;
		policy->timer.function = xfrm_policy_timer;
	}
	return policy;
}
EXPORT_SYMBOL(xfrm_policy_alloc);

/* Destroy xfrm_policy: descendant resources must be released to this moment. */

void __xfrm_policy_destroy(struct xfrm_policy *policy)
{
	BUG_ON(!policy->dead);

	BUG_ON(policy->bundles);

	if (del_timer(&policy->timer))
		BUG();

	security_xfrm_policy_free(policy);
	kfree(policy);
}
EXPORT_SYMBOL(__xfrm_policy_destroy);

static void xfrm_policy_gc_kill(struct xfrm_policy *policy)
{
	struct dst_entry *dst;

	while ((dst = policy->bundles) != NULL) {
		policy->bundles = dst->next;
		dst_free(dst);
	}

	if (del_timer(&policy->timer))
		atomic_dec(&policy->refcnt);

	if (atomic_read(&policy->refcnt) > 1)
		flow_cache_flush();

	xfrm_pol_put(policy);
}

static void xfrm_policy_gc_task(void *data)
{
	struct xfrm_policy *policy;
	struct list_head *entry, *tmp;
	struct list_head gc_list = LIST_HEAD_INIT(gc_list);

	spin_lock_bh(&xfrm_policy_gc_lock);
	list_splice_init(&xfrm_policy_gc_list, &gc_list);
	spin_unlock_bh(&xfrm_policy_gc_lock);

	list_for_each_safe(entry, tmp, &gc_list) {
		policy = list_entry(entry, struct xfrm_policy, list);
		xfrm_policy_gc_kill(policy);
	}
}

/* Rule must be locked. Release descentant resources, announce
 * entry dead. The rule must be unlinked from lists to the moment.
 */

static void xfrm_policy_kill(struct xfrm_policy *policy)
{
	int dead;

	write_lock_bh(&policy->lock);
	dead = policy->dead;
	policy->dead = 1;
	write_unlock_bh(&policy->lock);

	if (unlikely(dead)) {
		WARN_ON(1);
		return;
	}

	spin_lock(&xfrm_policy_gc_lock);
	list_add(&policy->list, &xfrm_policy_gc_list);
	spin_unlock(&xfrm_policy_gc_lock);

	schedule_work(&xfrm_policy_gc_work);
}

/* Generate new index... KAME seems to generate them ordered by cost
 * of an absolute inpredictability of ordering of rules. This will not pass. */
static u32 xfrm_gen_index(int dir)
{
	u32 idx;
	struct xfrm_policy *p;
	static u32 idx_generator;

	for (;;) {
		idx = (idx_generator | dir);
		idx_generator += 8;
		if (idx == 0)
			idx = 8;
		for (p = xfrm_policy_list[dir]; p; p = p->next) {
			if (p->index == idx)
				break;
		}
		if (!p)
			return idx;
	}
}

int xfrm_policy_insert(int dir, struct xfrm_policy *policy, int excl)
{
	struct xfrm_policy *pol, **p;
	struct xfrm_policy *delpol = NULL;
	struct xfrm_policy **newpos = NULL;
	struct dst_entry *gc_list;

	write_lock_bh(&xfrm_policy_lock);
	for (p = &xfrm_policy_list[dir]; (pol=*p)!=NULL;) {
		if (!delpol && memcmp(&policy->selector, &pol->selector, sizeof(pol->selector)) == 0 &&
		    xfrm_sec_ctx_match(pol->security, policy->security)) {
			if (excl) {
				write_unlock_bh(&xfrm_policy_lock);
				return -EEXIST;
			}
			*p = pol->next;
			delpol = pol;
			if (policy->priority > pol->priority &&
			    pol->next != NULL)
				continue;
		} else if (policy->priority >= pol->priority) {
			p = &pol->next;
			continue;
		}
		if (!newpos)
			newpos = p;
		if (delpol)
			break;
		p = &pol->next;
	}
	if (newpos)
		p = newpos;
	xfrm_pol_hold(policy);
	policy->next = *p;
	*p = policy;
	atomic_inc(&flow_cache_genid);
	policy->index = delpol ? delpol->index : xfrm_gen_index(dir);
	policy->curlft.add_time = (unsigned long)xtime.tv_sec;
	policy->curlft.use_time = 0;
	if (!mod_timer(&policy->timer, jiffies + HZ))
		xfrm_pol_hold(policy);
	write_unlock_bh(&xfrm_policy_lock);

	if (delpol)
		xfrm_policy_kill(delpol);

	read_lock_bh(&xfrm_policy_lock);
	gc_list = NULL;
	for (policy = policy->next; policy; policy = policy->next) {
		struct dst_entry *dst;

		write_lock(&policy->lock);
		dst = policy->bundles;
		if (dst) {
			struct dst_entry *tail = dst;
			while (tail->next)
				tail = tail->next;
			tail->next = gc_list;
			gc_list = dst;

			policy->bundles = NULL;
		}
		write_unlock(&policy->lock);
	}
	read_unlock_bh(&xfrm_policy_lock);

	while (gc_list) {
		struct dst_entry *dst = gc_list;

		gc_list = dst->next;
		dst_free(dst);
	}

	return 0;
}
EXPORT_SYMBOL(xfrm_policy_insert);

#ifdef CONFIG_XFRM_ENHANCEMENT
/* Allows comparison of selectors with wildcard user value */
static inline int selector_cmp(struct  xfrm_selector *user_sel,
			       struct  xfrm_selector *sel)
{
	return (memcmp(user_sel, sel, sizeof(*sel) - sizeof(sel->user)) ||
		((user_sel->user != sel->user) && user_sel->user));
}
#endif

#if 0
static void dump_sel(struct xfrm_selector *sel)
{
	int i;

	printk("daddr = ");
	for (i=0; i<4; i++)
		printk("%x.", sel->daddr.a6[i]);
	printk("\n");

	printk("saddr = ");
	for (i=0; i<4; i++)
		printk("%x.", sel->saddr.a6[i]);
	printk("\n");

	printk("\tsel.dport = %d\n", ntohs(sel->dport));
	printk("\tsel.dport_mask = %d\n", ntohs(sel->dport_mask));
	printk("\tsel.sport = %d\n", ntohs(sel->sport));
	printk("\tsel.sport_mask = %d\n", ntohs(sel->sport_mask));
	printk("\tsel.family = %d\n", ntohs(sel->family));
	printk("\tsel.prefixlen_d = 0x%x\n", sel->prefixlen_d);
	printk("\tsel.prefixlen_s = 0x%x\n", sel->prefixlen_s);
	printk("\tsel.proto = 0x%x\n", sel->proto);
	printk("\tsel.ifindex = %d\n", ntohl(sel->ifindex));
	printk("\tsel.user = %d\n", ntohs(sel->user));
	printk("-------------------------------------------------\n");

	return;
}
#endif

struct xfrm_policy *xfrm_policy_bysel_ctx(int dir, struct xfrm_selector *sel,
					  struct xfrm_sec_ctx *ctx, int delete)
{
	struct xfrm_policy *pol, **p;

	write_lock_bh(&xfrm_policy_lock);
	for (p = &xfrm_policy_list[dir]; (pol=*p)!=NULL; p = &pol->next) {
#ifdef CONFIG_XFRM_ENHANCEMENT
		if ((selector_cmp(sel, &pol->selector) == 0) &&
#else
		if ((memcmp(sel, &pol->selector, sizeof(*sel)) == 0) &&
#endif
		    (xfrm_sec_ctx_match(ctx, pol->security))) {
			xfrm_pol_hold(pol);
			if (delete)
				*p = pol->next;
			break;
		}
	}
	write_unlock_bh(&xfrm_policy_lock);

	if (pol && delete) {
		atomic_inc(&flow_cache_genid);
		xfrm_policy_kill(pol);
	}
	return pol;
}
EXPORT_SYMBOL(xfrm_policy_bysel_ctx);

struct xfrm_policy *xfrm_policy_byid(int dir, u32 id, int delete)
{
	struct xfrm_policy *pol, **p;

	write_lock_bh(&xfrm_policy_lock);
	for (p = &xfrm_policy_list[dir]; (pol=*p)!=NULL; p = &pol->next) {
		if (pol->index == id) {
			xfrm_pol_hold(pol);
			if (delete)
				*p = pol->next;
			break;
		}
	}
	write_unlock_bh(&xfrm_policy_lock);

	if (pol && delete) {
		atomic_inc(&flow_cache_genid);
		xfrm_policy_kill(pol);
	}
	return pol;
}
EXPORT_SYMBOL(xfrm_policy_byid);

void xfrm_policy_flush(void)
{
	struct xfrm_policy *xp;
	int dir;

	write_lock_bh(&xfrm_policy_lock);
	for (dir = 0; dir < XFRM_POLICY_MAX; dir++) {
		while ((xp = xfrm_policy_list[dir]) != NULL) {
			xfrm_policy_list[dir] = xp->next;
			write_unlock_bh(&xfrm_policy_lock);

			xfrm_policy_kill(xp);

			write_lock_bh(&xfrm_policy_lock);
		}
	}
	atomic_inc(&flow_cache_genid);
	write_unlock_bh(&xfrm_policy_lock);
}
EXPORT_SYMBOL(xfrm_policy_flush);

int xfrm_policy_walk(int (*func)(struct xfrm_policy *, int, int, void*),
		     void *data)
{
	struct xfrm_policy *xp;
	int dir;
	int count = 0;
	int error = 0;

	read_lock_bh(&xfrm_policy_lock);
	for (dir = 0; dir < 2*XFRM_POLICY_MAX; dir++) {
		for (xp = xfrm_policy_list[dir]; xp; xp = xp->next)
			count++;
	}

	if (count == 0) {
		error = -ENOENT;
		goto out;
	}

	for (dir = 0; dir < 2*XFRM_POLICY_MAX; dir++) {
		for (xp = xfrm_policy_list[dir]; xp; xp = xp->next) {
			error = func(xp, dir%XFRM_POLICY_MAX, --count, data);
			if (error)
				goto out;
		}
	}

out:
	read_unlock_bh(&xfrm_policy_lock);
	return error;
}
EXPORT_SYMBOL(xfrm_policy_walk);

#ifdef CONFIG_XFRM_MIGRATE
struct xfrm_policy_bulk_info {
	struct list_head lh;
	int count;
	int reflected;
} ;

struct xfrm_policy_bulk_entry {
	struct list_head list;
	struct xfrm_policy *policy;
	u8 dir;
	int reflected;
	int unique; /* set when it is new entry for DB or different selector's
		     * item will replace it
		     */
};

static void xfrm_policy_bulk_put(struct xfrm_policy_bulk_info *bi);
static void xfrm_policy_bulk_destruct(struct xfrm_policy_bulk_info *bi,
				      void *arg,
				      void (*destruct)(struct xfrm_policy *, void *));
static void xfrm_policy_bulk_delete(struct xfrm_policy_bulk_info *bi);

static struct xfrm_policy_bulk_info *xfrm_policy_bulk_alloc(void)
{
	struct xfrm_policy_bulk_info *bi;

	bi = kmalloc(sizeof(struct xfrm_policy_bulk_info), GFP_KERNEL);
	if (!bi)
		return NULL;
	memset(bi, 0, sizeof(*bi));

	INIT_LIST_HEAD(&bi->lh);
	bi->count = 0;
	bi->reflected = 0;

	return bi;
}

static void xfrm_policy_bulk_free(struct xfrm_policy_bulk_info *bi)
{
	INIT_LIST_HEAD(&bi->lh);

	memset(bi, 0, sizeof(*bi));

	kfree(bi);
}

/*
 * Create new bulk message by constructing new policy for each entry on
 * existing bulk message which was prepared by hold function.
 * If error occures destruct function will be called for each constructed
 * entry.
 */
static int xfrm_policy_bulk_rearrange(
	struct xfrm_policy_bulk_info *new_bi, struct xfrm_policy_bulk_info *bi,
	void *arg,
	struct xfrm_policy *(*construct)(struct xfrm_policy *, void *, int *),
	void (*destruct)(struct xfrm_policy *, void *))
{
	struct xfrm_policy *policy;
	struct xfrm_policy_bulk_entry *e;
	struct xfrm_policy_bulk_entry *new_e;
	int err = 0;

	if (new_bi->reflected != 0)
		return -EINVAL;
	if (new_bi->count > 0)
		return -EINVAL;
	if (bi->reflected != 1)
		return -EINVAL;
	if (bi->count == 0)
		return -EINVAL;

	list_for_each_entry(e, &bi->lh, list) {
		new_e = kmalloc(sizeof(struct xfrm_policy_bulk_entry),
				GFP_KERNEL);
		if (!new_e) {
			err = -ENOMEM;
			goto out;
		}
		memset(new_e, 0, sizeof(*new_e));

		policy = construct(e->policy, arg, &err);
		if (!policy) {
			kfree(new_e);
			new_e = NULL;
			if (!err)
				err = -ENOMEM;

			goto out;
		}
		err = 0;

		e->unique = !!(memcmp(&e->policy->selector, &policy->selector,
				      sizeof(e->policy->selector)));

		new_e->policy = policy;
		new_e->dir = e->dir;
		new_e->reflected = 0;
		new_e->unique = 1;

		list_add(&new_e->list, &new_bi->lh);
		new_bi->count ++;

		policy = NULL;
		new_e = NULL;
	}

 out:
	if (err)
		xfrm_policy_bulk_destruct(new_bi, arg, destruct);

	return err;
}

/*
 * Destruct bulk message for each entry. This function must used only after
 * rearrange function and before insert one.
 */
static void xfrm_policy_bulk_destruct(struct xfrm_policy_bulk_info *bi,
				      void *arg,
				      void (*destruct)(struct xfrm_policy *, void *))
{
	struct list_head *le;
	struct list_head *tmp;
	struct xfrm_policy_bulk_entry *e;

	if (bi->reflected != 0) {
		BUG();
		return;
	}

	list_for_each_safe(le, tmp, &bi->lh) {
		e = list_entry(le, struct xfrm_policy_bulk_entry, list);

		if (e->reflected != 0) {
			BUG();
			continue;
		}

		list_del(&e->list);
		bi->count --;

		destruct(e->policy, arg);
		e->policy = NULL;

		kfree(e);
		e = NULL;
	}

	BUG_ON(bi->count > 0);
}

/*
 * Search policy all over and hold them to bulk message.
 */
static int xfrm_policy_bulk_hold(struct xfrm_policy_bulk_info *bi, u8 dir,
				 u32 index, struct xfrm_selector *sel,
				 void *arg,
				 int (*filter)(struct xfrm_policy *, u8, u32,
					       struct xfrm_selector *,
					       void *))
{
	struct xfrm_policy *policy;
	struct xfrm_policy *xp;
	struct xfrm_policy_bulk_entry *e;
	int err = 0;
	int ret;

	if (bi->reflected != 0)
		return -EINVAL;

	read_lock_bh(&xfrm_policy_lock);

	for (xp = xfrm_policy_list[dir]; xp; xp = xp->next) {
		if (unlikely(xp->dead))
			continue;
		ret = filter(xp, dir, index, sel, arg);
		if (ret < 0) {
			err = ret;
			goto out;
		} else if (ret == 0)
			continue;

		e = kmalloc(sizeof(struct xfrm_policy_bulk_entry), GFP_ATOMIC);
		if (!e) {
			err = -ENOMEM;
			goto out;
		}
		memset(e, 0, sizeof(*e));

		policy = xp;
		xfrm_pol_hold(policy);
		e->policy = policy;
		e->dir = dir;
		e->reflected = 1;

		list_add(&e->list, &bi->lh);
		bi->count ++;
	}

 out:
	if (bi->count > 0)
		bi->reflected = 1;
	else
		err = -ENOENT;

	read_unlock_bh(&xfrm_policy_lock);

	if (err && bi->count > 0)
		xfrm_policy_bulk_put(bi);

	return err;
}

/*
 * Release all policy in bulk message which was inserted or held.
 */
static void xfrm_policy_bulk_put(struct xfrm_policy_bulk_info *bi)
{
	struct xfrm_policy *policy;
	struct xfrm_policy_bulk_entry *e;
	struct list_head *le;
	struct list_head *tmp;

	if (bi->count == 0)
		return;

	list_for_each_safe(le, tmp, &bi->lh) {
		e = list_entry(le, struct xfrm_policy_bulk_entry, list);

		list_del(&e->list);
		bi->count --;

		policy = e->policy;
		e->policy = NULL;

		if (e->unique)
			xfrm_pol_put(policy); /* XXX: for the bulk list */

		kfree(e);
	}

	bi->reflected = 0;
}

/*
 * Reflect bulk message to real policy database by inserting it.
 */
static int xfrm_policy_bulk_insert(struct xfrm_policy_bulk_info *bi)
{
	struct xfrm_policy *policy;
	struct xfrm_policy_bulk_entry *e;
	int err = 0;

	if (bi->reflected != 0)
		return -EINVAL;
	if (bi->count == 0)
		return -EINVAL;

	list_for_each_entry(e, &bi->lh, list) {
		if (e->reflected != 0) {
			BUG();
			continue;
		}

		policy = e->policy;

		err = xfrm_policy_insert(e->dir, policy, 0);
		if (err)
			goto out;

		/* At net/{key/af_key.c,xfrm/xfrm_user.c},
		 * it is called like xfrm_pol_put(policy) after insert it.
		 * Here we don't do that because a bulk entry still holds it.
		 */

		e->reflected = 1;
	}

 out:
	bi->reflected = 1;

	if (err)
		xfrm_policy_bulk_delete(bi);

	return err;
}

/*
 * Reflect bulk message to real policy database by deleting it.
 */
static void xfrm_policy_bulk_delete(struct xfrm_policy_bulk_info *bi)
{
	struct xfrm_policy *policy;
	struct xfrm_policy_bulk_entry *e;

	if (bi->reflected != 1)
		return;

	list_for_each_entry(e, &bi->lh, list) {
		if (e->reflected != 1)
			continue;

		policy = e->policy;
		if (e->unique)
			xfrm_policy_delete(policy, e->dir);

		xfrm_pol_put(policy);

		e->reflected = 0;
	}

	bi->reflected = 0;
}

static int __xfrm_addr_any(const xfrm_address_t *a, unsigned short family)
{
	switch (family) {
	case AF_INET:
		return (a->a4 == 0);
	case AF_INET6:
		return ipv6_addr_any((const struct in6_addr *)a->a6);
	}
	return -1; /* XXX: */
}

static struct xfrm_policy *xfrm_policy_solidclone(struct xfrm_policy *old)
{
	int i;
	struct xfrm_policy *xp = xfrm_policy_alloc(GFP_ATOMIC);
	if (xp) {
		memcpy(&xp->selector, &old->selector, sizeof(xp->selector));
		memcpy(&xp->lft, &old->lft, sizeof(xp->lft));
		//memcpy(&xp->curlft, &old->curlft, sizeof(xp->curlft));
		xp->priority = old->priority;
		xp->index = old->index;
		xp->family = old->family;
		xp->action = old->action;
		xp->flags = old->flags;
		xp->xfrm_nr = old->xfrm_nr;
		for (i = 0; i < xp->xfrm_nr; i++)
			memcpy(&xp->xfrm_vec[i], &old->xfrm_vec[i],
			       sizeof(xp->xfrm_vec[i]));
	}
	return xp;
}

static int xfrm_migrate_policy_filter(struct xfrm_policy *xp, u8 dir,
				      u32 index, struct xfrm_selector *sel,
				      void *arg)
{
	if (xp->action != XFRM_POLICY_ALLOW || xp->xfrm_nr == 0)
		return 0;

	if (index) {
		if (index != xp->index)
			return 0;
	}

	if (sel) {
#ifdef CONFIG_XFRM_ENHANCEMENT
		if (sel->proto != IPSEC_ULPROTO_ANY) {
			if (memcmp(sel, &xp->selector, sizeof(*sel)))
				return 0;
		} else {
			struct xfrm_selector *s = &xp->selector;

			if (sel->family != s->family ||
			    xfrm_addr_cmp(&sel->daddr, &s->daddr,
					  sel->family) ||
			    xfrm_addr_cmp(&sel->saddr, &s->saddr,
					  sel->family) ||
			    sel->prefixlen_d != s->prefixlen_d ||
			    sel->prefixlen_s != s->prefixlen_s)
				return 0;
		}
#else
		if (memcmp(sel, &xp->selector, sizeof(*sel)))
			return 0;
#endif
	}

	return 1;
}

static int xfrm_migrate_id_filter(struct xfrm_id *id,
				  struct xfrm_user_migrate *m)
{
#ifdef CONFIG_XFRM_ENHANCEMENT
       if (xfrm_id_proto_match(id->proto, m->proto) &&
#else
       if ((id->proto == m->proto) &&
#endif
	   !xfrm_addr_cmp(&id->daddr, &m->old_daddr, m->family))
		return 1;

	return 0;
}

static int xfrm_migrate_state_filter(struct xfrm_state *x, void *arg)
{
	struct xfrm_user_migrate *m = (struct xfrm_user_migrate *)arg;

	if (x->props.family == m->family &&
	    xfrm_migrate_id_filter(&x->id, m) &&
	    !xfrm_addr_cmp(&x->props.saddr, &m->old_saddr, m->family) &&
	    x->props.mode == m->mode &&
	    (!m->reqid || x->props.reqid == m->reqid))
		return 1;

	return 0;
}

static int xfrm_migrate_tmpl_filter(struct xfrm_tmpl *t,
				    struct xfrm_user_migrate *m)
{
	if (xfrm_migrate_id_filter(&t->id, m) &&
	    !xfrm_addr_cmp(&t->saddr, &m->old_saddr, m->family) &&
	    t->mode == m->mode &&
	    (!m->reqid || t->reqid == m->reqid))
		return 1;
	return 0;
}

static struct xfrm_policy *__policy_construct(struct xfrm_policy *xp,
					      void *arg, int *errp)
{
	struct xfrm_user_migrate *m = (struct xfrm_user_migrate *)arg;
	struct xfrm_policy *new_xp = NULL;
	int matched = 0;
	int err = 0;
	int i;

	new_xp = xfrm_policy_solidclone(xp);
	if (!new_xp) {
		XFRM_DBG("%s: failed to clone policy\n", __FUNCTION__);
		err = -ENOMEM;
		goto error;
	}

	/*
	 * check templates and update its address
	 */
	for (i = 0; i < xp->xfrm_nr; i++) {
		struct xfrm_tmpl *t = &new_xp->xfrm_vec[i];

		int ret = xfrm_migrate_tmpl_filter(t, m);
		if (unlikely(ret < 0)) {
			err = ret;
			break;
		} else if (ret == 0)
			continue;

		memcpy(&t->id.daddr, &m->new_daddr, sizeof(t->id.daddr));
		memcpy(&t->saddr, &m->new_saddr, sizeof(t->saddr));

		matched ++;
	}

	if (!err && matched == 0) {
		XFRM_DBG("%s: no matching template in such a policy as index = %u\n", __FUNCTION__, xp->index);
		err = -ESRCH;
		goto error;
	}

	return new_xp;

 error:
	if (errp)
		*errp = err;
	if (new_xp)
		kfree(new_xp);
	return NULL;
}

static void __policy_destruct(struct xfrm_policy *xp, void *arg)
{
	xfrm_policy_kill(xp);
}

static struct xfrm_state *__state_construct(struct xfrm_state *x, void *arg,
					    int *errp)
{
	struct xfrm_user_migrate *m;
	struct xfrm_state *new_x;

	m = (struct xfrm_user_migrate *)arg;

	new_x = xfrm_state_solidclone(x, errp);
	if (!new_x) {
		if (errp)
			*errp = -ENOMEM;
	} else {
		memcpy(&new_x->id.daddr, &m->new_daddr,
		       sizeof(new_x->id.daddr));
		memcpy(&new_x->props.saddr, &m->new_saddr,
		       sizeof(new_x->props.saddr));
	}

	return new_x;
}

static void __state_destruct(struct xfrm_state *x, void *arg)
{
	kfree(x);
}

/*
 * Change the endpoint addresses.
 *
 *   NOTE:
 *   - This routine is called when the kernel receives PF_KEY/XFRM MIGRATE
 *     message from userland application (i.e. MIPv6)
 *
 * XXX: Todo: it should require a lock for policy DB to update tempaltes
 * operation during all policy is complete.
 * Alternatively it should require a lock for both state and policy DB during
 * all operation in this function.
 */
int xfrm_migrate(struct xfrm_user_migrate *m, struct xfrm_userpolicy_id *pol)
{
	struct xfrm_policy_bulk_info *old_pbi = NULL;
	struct xfrm_policy_bulk_info *new_pbi = NULL;
	struct xfrm_state_bulk_info *old_sbi = NULL;
	struct xfrm_state_bulk_info *new_sbi = NULL;
	int insert_force = 0;
	int err = -EINVAL;

	switch (m->family) {
	case AF_INET:
		break;
	case AF_INET6:
#if defined(CONFIG_IPV6) || defined(CONFIG_IPV6_MODULE)
		break;
#else
		XFRM_DBG("%s: not supported family = %u\n", __FUNCTION__, m->family);
		err = -EAFNOSUPPORT;
		goto error;
#endif
	default:
		XFRM_DBG("%s: invalid family = %u\n", __FUNCTION__, m->family);
		err = -EINVAL;
		goto error;
	}

	if (xfrm_addr_cmp(&m->old_daddr, &m->new_daddr, m->family) == 0 &&
	    xfrm_addr_cmp(&m->old_saddr, &m->new_saddr, m->family) == 0) {
		XFRM_DBG("%s: old/new addresses do not differ\n", __FUNCTION__);
		err = -EINVAL;
		goto error;
	}
	if (__xfrm_addr_any(&m->new_daddr, m->family) ||
	    __xfrm_addr_any(&m->new_saddr, m->family)) {
		XFRM_DBG("%s: new address is not specified\n", __FUNCTION__);
		err = -EINVAL;
		goto error;

	}
	if (xfrm_addr_cmp(&m->old_daddr, &m->new_daddr, m->family) == 0)
		insert_force = 1;

#if XFRM_DEBUG >= 4
	printk(KERN_DEBUG "%s:\n", __FUNCTION__);
	printk(KERN_DEBUG "  old-saddr = %s\n", XFRMSTRADDR(m->old_saddr, m->family));
	printk(KERN_DEBUG "  old-daddr = %s,\n", XFRMSTRADDR(m->old_daddr, m->family));
	printk(KERN_DEBUG "  new-saddr = %s,\n", XFRMSTRADDR(m->new_saddr, m->family));
	printk(KERN_DEBUG "  new-daddr = %s,\n", XFRMSTRADDR(m->new_daddr, m->family));
	printk(KERN_DEBUG "  proto = %u, mode = %u, reqid = %u\n", m->proto, m->mode, m->reqid);
#endif

	/*
	 * Stage 0: find policy
	 */
	old_pbi = xfrm_policy_bulk_alloc();
	if (!old_pbi) {
		err = -ENOMEM;
		XFRM_DBG("%s: failed bulk alloc (for old) policy\n", __FUNCTION__);
		goto error_s0;
	}

#if XFRM_DEBUG >= 4
	printk(KERN_DEBUG "%s: policy index = %u\n", __FUNCTION__, pol->index);
	printk(KERN_DEBUG "  sel saddr = %s/%u[%u]\n", XFRMSTRADDR(pol->sel.saddr, m->family), pol->sel.prefixlen_s, pol->sel.sport);
	printk(KERN_DEBUG "  sel daddr = %s/%u[%u],\n", XFRMSTRADDR(pol->sel.daddr, m->family), pol->sel.prefixlen_d, pol->sel.dport);
	printk(KERN_DEBUG "  proto = %u, ifindex = %d, user = %u\n", pol->sel.proto, pol->sel.ifindex, pol->sel.user);
#endif
	err = xfrm_policy_bulk_hold(old_pbi, pol->dir, pol->index, &pol->sel,
				    m, xfrm_migrate_policy_filter);
	if (err < 0) {
		if (err == -ENOENT)
			XFRM_DBG("%s: no policy found\n", __FUNCTION__);
		else
			XFRM_DBG("%s: failed bulk hold policy: %d\n", __FUNCTION__, err);
		goto error_s0;
	}

	/*
	 * Stage 1: find state
	 */
	old_sbi = xfrm_state_bulk_alloc();
	if (!old_sbi) {
		err = -ENOMEM;
		XFRM_DBG("%s: failed bulk alloc (for old) state: %d\n", __FUNCTION__, err);
		goto error_s1;
	}

	err = xfrm_state_bulk_hold(old_sbi, m->family, &m->old_daddr, m,
				   xfrm_migrate_state_filter);
	if (err) {
		if (err != -ESRCH) {
			XFRM_DBG("%s: failed bulk hold state: %d\n", __FUNCTION__, err);
			goto error_s1;
		}

		xfrm_state_bulk_free(old_sbi);
		old_sbi = NULL;
		err = 0;
	}

	/*
	 * Stage 2: add new state (if state was found)
	 */
	if (old_sbi) {
		new_sbi = xfrm_state_bulk_alloc();
		if (!new_sbi) {
			err = -ENOMEM;
			XFRM_DBG("%s: failed bulk alloc (for new) state: %d\n", __FUNCTION__, err);
			goto error_s2;
		}

		err = xfrm_state_bulk_rearrange(new_sbi, old_sbi, m,
						__state_construct,
						__state_destruct);
		if (err) {
			XFRM_DBG("%s: failed bulk rearrange state: %d\n", __FUNCTION__, err);
			goto error_s2;
		}

		err = xfrm_state_bulk_insert(new_sbi, insert_force);
		if (err) {
			XFRM_DBG("%s: failed bulk insert state: %d\n", __FUNCTION__, err);
			xfrm_state_bulk_destruct(new_sbi, m, __state_destruct);
			goto error_s2;
		}
	}

	/*
	 * Stage 3: add new policy
	 */
	new_pbi = xfrm_policy_bulk_alloc();
	if (!new_pbi) {
		err = -ENOMEM;
		XFRM_DBG("%s: failed bulk alloc (for new) policy\n", __FUNCTION__);
		goto error_s3;
	}

	err = xfrm_policy_bulk_rearrange(new_pbi, old_pbi, m,
					 __policy_construct,
					 __policy_destruct);
	if (err) {
		XFRM_DBG("%s: failed bulk rearrange policy: %d\n", __FUNCTION__, err);
		goto error_s3;
	}

	err = xfrm_policy_bulk_insert(new_pbi);
	if (err) {
		XFRM_DBG("%s: failed bulk insert policy: %d\n", __FUNCTION__, err);
		xfrm_policy_bulk_destruct(new_pbi, m, __policy_destruct);
		goto error_s3;
	}
#if XFRM_DEBUG >= 4
	printk(KERN_DEBUG "%s: %d policy inserted\n", __FUNCTION__, new_pbi->count);
#endif

	/*
	 * Stage 4: remove old policy (if selector was changed)
	 * and release policies
	 */
	xfrm_policy_bulk_delete(old_pbi);

	xfrm_policy_bulk_put(old_pbi);
	xfrm_policy_bulk_free(old_pbi);

	xfrm_policy_bulk_put(new_pbi);
	xfrm_policy_bulk_free(new_pbi);

	/*
	 * Stage 5: remove old state (if state was found)
	 * and release states
	 */
	if (old_sbi) {
		/*
		 * delete old SAs
		 */
		xfrm_state_bulk_delete(old_sbi);
#if XFRM_DEBUG >= 4
		printk(KERN_DEBUG "%s: %d state(s) deleted\n", __FUNCTION__, old_sbi->count);
#endif
		xfrm_state_bulk_put(old_sbi);
		xfrm_state_bulk_free(old_sbi);

		if (new_sbi) {
#if XFRM_DEBUG >= 4
			printk(KERN_DEBUG "%s: %d state(s) inserted\n", __FUNCTION__, new_sbi->count);
#endif
			xfrm_state_bulk_put(new_sbi);
			xfrm_state_bulk_free(new_sbi);
		}
	}

	/* XXX: it should be called per policy */
	km_migrate(m, pol);

	return 0;

 error_s3:
	if (new_pbi) {
		xfrm_policy_bulk_put(new_pbi);
		xfrm_policy_bulk_free(new_pbi);
	}
	if (new_sbi)
		xfrm_state_bulk_delete(new_sbi);
 error_s2:
	if (new_sbi) {
		xfrm_state_bulk_put(new_sbi);
		xfrm_state_bulk_free(new_sbi);
	}
 error_s1:
	if (old_sbi) {
		xfrm_state_bulk_put(old_sbi);
		xfrm_state_bulk_free(old_sbi);
	}
 error_s0:
	if (old_pbi) {
		xfrm_policy_bulk_put(old_pbi);
		xfrm_policy_bulk_free(old_pbi);
	}

 error:
	return err;
}
EXPORT_SYMBOL(xfrm_migrate);
#endif

/* Find policy to apply to this flow. */

static void xfrm_policy_lookup(struct flowi *fl, u32 sk_sid, u16 family, u8 dir,
			       void **objp, atomic_t **obj_refp)
{
	struct xfrm_policy *pol;

	read_lock_bh(&xfrm_policy_lock);
	for (pol = xfrm_policy_list[dir]; pol; pol = pol->next) {
		struct xfrm_selector *sel = &pol->selector;
		int match;

		if (pol->family != family)
			continue;

		match = xfrm_selector_match(sel, fl, family);

		if (match) {
 			if (!security_xfrm_policy_lookup(pol, sk_sid, dir)) {
				xfrm_pol_hold(pol);
				break;
			}
		}
	}
	read_unlock_bh(&xfrm_policy_lock);
	if ((*objp = (void *) pol) != NULL)
		*obj_refp = &pol->refcnt;
}

static inline int policy_to_flow_dir(int dir)
{
	if (XFRM_POLICY_IN == FLOW_DIR_IN &&
 	    XFRM_POLICY_OUT == FLOW_DIR_OUT &&
#ifdef CONFIG_USE_POLICY_FWD
 	    XFRM_POLICY_FWD == FLOW_DIR_FWD
#else
	    1
#endif
	    )
 		return dir;
 	switch (dir) {
 	default:
 	case XFRM_POLICY_IN:
 		return FLOW_DIR_IN;
 	case XFRM_POLICY_OUT:
 		return FLOW_DIR_OUT;
#ifdef CONFIG_USE_POLICY_FWD
 	case XFRM_POLICY_FWD:
 		return FLOW_DIR_FWD;
#endif
	};
}

static struct xfrm_policy *xfrm_sk_policy_lookup(struct sock *sk, int dir, struct flowi *fl, u32 sk_sid)
{
	struct xfrm_policy *pol;

	read_lock_bh(&xfrm_policy_lock);
	if ((pol = sk->sk_policy[dir]) != NULL) {
 		int match = xfrm_selector_match(&pol->selector, fl,
						sk->sk_family);
 		int err = 0;

		if (match)
		  err = security_xfrm_policy_lookup(pol, sk_sid, policy_to_flow_dir(dir));

 		if (match && !err)
			xfrm_pol_hold(pol);
		else
			pol = NULL;
	}
	read_unlock_bh(&xfrm_policy_lock);
	return pol;
}

static void __xfrm_policy_link(struct xfrm_policy *pol, int dir)
{
	pol->next = xfrm_policy_list[dir];
	xfrm_policy_list[dir] = pol;
	xfrm_pol_hold(pol);
}

static struct xfrm_policy *__xfrm_policy_unlink(struct xfrm_policy *pol,
						int dir)
{
	struct xfrm_policy **polp;

	for (polp = &xfrm_policy_list[dir];
	     *polp != NULL; polp = &(*polp)->next) {
		if (*polp == pol) {
			*polp = pol->next;
			return pol;
		}
	}
	return NULL;
}

int xfrm_policy_delete(struct xfrm_policy *pol, int dir)
{
	write_lock_bh(&xfrm_policy_lock);
	pol = __xfrm_policy_unlink(pol, dir);
	write_unlock_bh(&xfrm_policy_lock);
	if (pol) {
		if (dir < XFRM_POLICY_MAX)
			atomic_inc(&flow_cache_genid);
		xfrm_policy_kill(pol);
		return 0;
	}
	return -ENOENT;
}
EXPORT_SYMBOL(xfrm_policy_delete);

int xfrm_sk_policy_insert(struct sock *sk, int dir, struct xfrm_policy *pol)
{
	struct xfrm_policy *old_pol;

	write_lock_bh(&xfrm_policy_lock);
	old_pol = sk->sk_policy[dir];
	sk->sk_policy[dir] = pol;
	if (pol) {
		pol->curlft.add_time = (unsigned long)xtime.tv_sec;
		pol->index = xfrm_gen_index(XFRM_POLICY_MAX+dir);
		__xfrm_policy_link(pol, XFRM_POLICY_MAX+dir);
	}
	if (old_pol)
		__xfrm_policy_unlink(old_pol, XFRM_POLICY_MAX+dir);
	write_unlock_bh(&xfrm_policy_lock);

	if (old_pol) {
		xfrm_policy_kill(old_pol);
	}
	return 0;
}

static struct xfrm_policy *clone_policy(struct xfrm_policy *old, int dir)
{
	struct xfrm_policy *newp = xfrm_policy_alloc(GFP_ATOMIC);

	if (newp) {
		newp->selector = old->selector;
		if (security_xfrm_policy_clone(old, newp)) {
			kfree(newp);
			return NULL;  /* ENOMEM */
		}
		newp->lft = old->lft;
		newp->curlft = old->curlft;
		newp->action = old->action;
		newp->flags = old->flags;
		newp->xfrm_nr = old->xfrm_nr;
		newp->index = old->index;
		memcpy(newp->xfrm_vec, old->xfrm_vec,
		       newp->xfrm_nr*sizeof(struct xfrm_tmpl));
		write_lock_bh(&xfrm_policy_lock);
		__xfrm_policy_link(newp, XFRM_POLICY_MAX+dir);
		write_unlock_bh(&xfrm_policy_lock);
		xfrm_pol_put(newp);
	}
	return newp;
}

int __xfrm_sk_clone_policy(struct sock *sk)
{
	struct xfrm_policy *p0 = sk->sk_policy[0],
			   *p1 = sk->sk_policy[1];

	sk->sk_policy[0] = sk->sk_policy[1] = NULL;
	if (p0 && (sk->sk_policy[0] = clone_policy(p0, 0)) == NULL)
		return -ENOMEM;
	if (p1 && (sk->sk_policy[1] = clone_policy(p1, 1)) == NULL)
		return -ENOMEM;
	return 0;
}

/* Resolve list of templates for the flow, given policy. */

static int
xfrm_tmpl_resolve(struct xfrm_policy *policy, struct flowi *fl,
		  struct xfrm_state **xfrm,
		  unsigned short family)
{
	int nx;
	int i, error;
	xfrm_address_t *daddr = xfrm_flowi_daddr(fl, family);
	xfrm_address_t *saddr = xfrm_flowi_saddr(fl, family);

	for (nx=0, i = 0; i < policy->xfrm_nr; i++) {
		struct xfrm_state *x;
		xfrm_address_t *remote = daddr;
		xfrm_address_t *local  = saddr;
		struct xfrm_tmpl *tmpl = &policy->xfrm_vec[i];

		if (tmpl->mode == XFRM_MODE_TUNNEL) {
			remote = &tmpl->id.daddr;
			local = &tmpl->saddr;
		}

		x = xfrm_state_find(remote, local, fl, tmpl, policy, &error, family);

		if (x && x->km.state == XFRM_STATE_VALID) {
			xfrm[nx++] = x;
			daddr = remote;
			saddr = local;
			continue;
		}
		if (x) {
			error = (x->km.state == XFRM_STATE_ERROR ?
				 -EINVAL : -EAGAIN);
			xfrm_state_put(x);
		}

		if (!tmpl->optional)
			goto fail;
	}
	return nx;

fail:
	for (nx--; nx>=0; nx--)
		xfrm_state_put(xfrm[nx]);
	return error;
}

/* Check that the bundle accepts the flow and its components are
 * still valid.
 */

static struct dst_entry *
xfrm_find_bundle(struct flowi *fl, struct xfrm_policy *policy, unsigned short family)
{
	struct dst_entry *x;
	struct xfrm_policy_afinfo *afinfo = xfrm_policy_get_afinfo(family);
	if (unlikely(afinfo == NULL))
		return ERR_PTR(-EINVAL);
	x = afinfo->find_bundle(fl, policy);
	xfrm_policy_put_afinfo(afinfo);
	return x;
}

/* Allocate chain of dst_entry's, attach known xfrm's, calculate
 * all the metrics... Shortly, bundle a bundle.
 */

static int
xfrm_bundle_create(struct xfrm_policy *policy, struct xfrm_state **xfrm, int nx,
		   struct flowi *fl, struct dst_entry **dst_p,
		   unsigned short family)
{
	int err;
	struct xfrm_policy_afinfo *afinfo = xfrm_policy_get_afinfo(family);
	if (unlikely(afinfo == NULL))
		return -EINVAL;
	err = afinfo->bundle_create(policy, xfrm, nx, fl, dst_p);
	xfrm_policy_put_afinfo(afinfo);
	return err;
}


static int stale_bundle(struct dst_entry *dst);

/* Main function: finds/creates a bundle for given flow.
 *
 * At the moment we eat a raw IP route. Mostly to speed up lookups
 * on interfaces with disabled IPsec.
 */
int xfrm_lookup(struct dst_entry **dst_p, struct flowi *fl,
		struct sock *sk, int flags)
{
	struct xfrm_policy *policy;
	struct xfrm_state *xfrm[XFRM_MAX_DEPTH];
	struct dst_entry *dst, *dst_orig = *dst_p;
	int nx = 0;
	int err;
	u32 genid;
	u16 family;
	u8 dir = policy_to_flow_dir(XFRM_POLICY_OUT);
	u32 sk_sid = security_sk_sid(sk, fl, dir);
#ifdef CONFIG_XFRM_ENHANCEMENT
	fl->oif = dst_orig->dev->ifindex;
#endif
restart:
	genid = atomic_read(&flow_cache_genid);
	policy = NULL;
	if (sk && sk->sk_policy[1])
		policy = xfrm_sk_policy_lookup(sk, XFRM_POLICY_OUT, fl, sk_sid);

	if (!policy) {
		/* To accelerate a bit...  */
		if ((dst_orig->flags & DST_NOXFRM) || !xfrm_policy_list[XFRM_POLICY_OUT])
			return 0;

		policy = flow_cache_lookup(fl, sk_sid, dst_orig->ops->family,
					   dir, xfrm_policy_lookup);
	}

	if (!policy)
		return 0;

	family = dst_orig->ops->family;
	policy->curlft.use_time = (unsigned long)xtime.tv_sec;

	switch (policy->action) {
	case XFRM_POLICY_BLOCK:
		/* Prohibit the flow */
		err = -EPERM;
		goto error;

	case XFRM_POLICY_ALLOW:
		if (policy->xfrm_nr == 0) {
			/* Flow passes not transformed. */
			xfrm_pol_put(policy);
			return 0;
		}

		/* Try to find matching bundle.
		 *
		 * LATER: help from flow cache. It is optional, this
		 * is required only for output policy.
		 */
		dst = xfrm_find_bundle(fl, policy, family);
		if (IS_ERR(dst)) {
			XFRM_DBG("%s: failed to find bundle at policy index = %u\n", __FUNCTION__, policy->index);
			err = PTR_ERR(dst);
			goto error;
		}

		if (dst)
			break;

		nx = xfrm_tmpl_resolve(policy, fl, xfrm, family);

		if (unlikely(nx<0)) {
			err = nx;
			if (err == -EAGAIN && flags) {
				DECLARE_WAITQUEUE(wait, current);

				add_wait_queue(&km_waitq, &wait);
				set_current_state(TASK_INTERRUPTIBLE);
				schedule();
				set_current_state(TASK_RUNNING);
				remove_wait_queue(&km_waitq, &wait);

				nx = xfrm_tmpl_resolve(policy, fl, xfrm, family);

				if (nx == -EAGAIN && signal_pending(current)) {
					err = -ERESTART;
					goto error;
				}
				if (nx == -EAGAIN ||
				    genid != atomic_read(&flow_cache_genid)) {
					xfrm_pol_put(policy);
					goto restart;
				}
				err = nx;
			}
			if (err < 0) {
				XFRM_DBG("%s: failed to resolve tmpl\n", __FUNCTION__);
				goto error;
			}
		}
		if (nx == 0) {
			/* Flow passes not transformed. */
			xfrm_pol_put(policy);
			return 0;
		}

		dst = dst_orig;
		err = xfrm_bundle_create(policy, xfrm, nx, fl, &dst, family);

		if (unlikely(err)) {
			int i;
			for (i = 0; i < nx; i++)
				xfrm_state_put(xfrm[i]);
			XFRM_DBG("%s: failed to create bundle\n", __FUNCTION__);
			goto error;
		}

		write_lock_bh(&policy->lock);
		if (unlikely(policy->dead || stale_bundle(dst))) {
			/* Wow! While we worked on resolving, this
			 * policy has gone. Retry. It is not paranoia,
			 * we just cannot enlist new bundle to dead object.
			 * We can't enlist stable bundles either.
			 */
			write_unlock_bh(&policy->lock);
			if (dst)
				dst_free(dst);

			err = -EHOSTUNREACH;
			goto error;
		}
		dst->next = policy->bundles;
		if (unlikely(fl->flags & FLOWI_FLAG_NOTROUTE))
			dst->flags |= DST_OTBUNDLE;
		policy->bundles = dst;
		dst_hold(dst);
		write_unlock_bh(&policy->lock);
	}
	*dst_p = dst;
	dst_release(dst_orig);
	xfrm_pol_put(policy);
	return 0;

error:
 	XFRM_DBG("%s: error = %d, policy index = %u\n", __FUNCTION__, err,
 		 policy->index);
	dst_release(dst_orig);
	xfrm_pol_put(policy);
	*dst_p = NULL;
	return err;
}
EXPORT_SYMBOL(xfrm_lookup);

#ifdef CONFIG_XFRM_ENHANCEMENT
static inline int
xfrm_post_reject(struct xfrm_state *x, struct sk_buff *skb, struct flowi *fl)
{
	if (!x || !x->type->post_reject)
		return 0;
	return x->type->post_reject(x, skb, fl);
}
#endif

/* When skb is transformed back to its "native" form, we have to
 * check policy restrictions. At the moment we make this in maximally
 * stupid way. Shame on me. :-) Of course, connected sockets must
 * have policy cached at them.
 */

static inline int
xfrm_state_ok(struct xfrm_tmpl *tmpl, struct xfrm_state *x,
	      unsigned short family)
{
	if (xfrm_state_kern(x))
		return tmpl->optional && !xfrm_state_addr_cmp(tmpl, x, family);
	return	x->id.proto == tmpl->id.proto &&
		(x->id.spi == tmpl->id.spi || !tmpl->id.spi) &&
		(x->props.reqid == tmpl->reqid || !tmpl->reqid) &&
		x->props.mode == tmpl->mode &&
#ifdef CONFIG_XFRM_ENHANCEMENT
		((tmpl->id.proto != IPPROTO_AH &&
		  tmpl->id.proto != IPPROTO_ESP &&
		  tmpl->id.proto != IPPROTO_COMP) ||
		 (tmpl->aalgos & (1<<x->props.aalgo))) &&
		!((x->props.mode != XFRM_MODE_TRANSPORT) &&
		  xfrm_state_addr_cmp(tmpl, x, family));
#else
		(tmpl->aalgos & (1<<x->props.aalgo)) &&
		!((x->props.mode != XFRM_MODE_TRANSPORT) &&
		  xfrm_state_addr_cmp(tmpl, x, family));
#endif
}

#ifdef CONFIG_XFRM_ENHANCEMENT
static inline int
xfrm_policy_ok(struct flowi *fl, struct xfrm_policy *xp,
	       struct xfrm_tmpl *tmpl, struct sec_path *sp, int start,
	       unsigned short family, struct xfrm_state **xerrp)
#else
static inline int
xfrm_policy_ok(struct xfrm_tmpl *tmpl, struct sec_path *sp, int start,
	       unsigned short family)
#endif
{
	int idx = start;

	if (tmpl->optional) {
#ifdef CONFIG_XFRM_ENHANCEMENT
		if (tmpl->mode == XFRM_MODE_IN_TRIGGER) {
			if (xfrm_ro_acq(fl, tmpl, xp, family) < 0)
				return -1;
		} else if (tmpl->mode == XFRM_MODE_TRANSPORT)
#else
		if (!tmpl->mode)
#endif
			return start;
	} else
		start = -1;
	for (; idx < sp->len; idx++) {
		if (xfrm_state_ok(tmpl, sp->xvec[idx], family))
			return ++idx;
#ifdef CONFIG_XFRM_ENHANCEMENT
		if (sp->xvec[idx]->props.mode != XFRM_MODE_TRANSPORT) {
			*xerrp = sp->xvec[idx];
			break;
		}
#else
		if (sp->xvec[idx]->props.mode != XFRM_MODE_TRANSPORT)
			break;
#endif
	}
	return start;
}

int
xfrm_decode_session(struct sk_buff *skb, struct flowi *fl, unsigned short family)
{
	struct xfrm_policy_afinfo *afinfo = xfrm_policy_get_afinfo(family);

	if (unlikely(afinfo == NULL))
		return -EAFNOSUPPORT;

	afinfo->decode_session(skb, fl);
	xfrm_policy_put_afinfo(afinfo);
	return 0;
}
EXPORT_SYMBOL(xfrm_decode_session);

#ifdef CONFIG_XFRM_ENHANCEMENT
static inline int secpath_has_nontransport(struct sec_path *sp, int k,
					   struct xfrm_state **xerrp)
#else
static inline int secpath_has_tunnel(struct sec_path *sp, int k)
#endif
{
	for (; k < sp->len; k++) {
#ifdef CONFIG_XFRM_ENHANCEMENT
		if (sp->xvec[k]->props.mode != XFRM_MODE_TRANSPORT) {
			*xerrp = sp->xvec[k];
			return 1;
		}
#else
		if (sp->xvec[k]->props.mode != XFRM_MODE_TRANSPORT)
			return 1;
#endif
	}

	return 0;
}

int __xfrm_policy_check(struct sock *sk, int dir, struct sk_buff *skb, 
			unsigned short family)
{
	struct xfrm_policy *pol;
	struct flowi fl;
	u8 fl_dir = policy_to_flow_dir(dir);
	u32 sk_sid;

	if (xfrm_decode_session(skb, &fl, family) < 0)
		return 0;
	nf_nat_decode_session(skb, &fl, family);

	sk_sid = security_sk_sid(sk, &fl, fl_dir);

	/* First, check used SA against their selectors. */
	if (skb->sp) {
		int i;

		for (i=skb->sp->len-1; i>=0; i--) {
			struct xfrm_state *x = skb->sp->xvec[i];
			if (!xfrm_selector_match(&x->sel, &fl, family))
				return 0;
		}
	}

	pol = NULL;
	if (sk && sk->sk_policy[dir])
		pol = xfrm_sk_policy_lookup(sk, dir, &fl, sk_sid);

	if (!pol)
		pol = flow_cache_lookup(&fl, sk_sid, family, fl_dir,
					xfrm_policy_lookup);

	if (!pol) {
#ifdef CONFIG_XFRM_ENHANCEMENT
		struct xfrm_state *xerr = NULL;
		if (!skb->sp)
			return 1;

		XFRM_DBG("%s: no policy but sec path exists\n", __FUNCTION__);

		if (secpath_has_nontransport(skb->sp, 0, &xerr)) {
			xfrm_post_reject(xerr, skb, &fl);
			return 0;
		}
		return 1;
#else
		return !skb->sp || !secpath_has_tunnel(skb->sp, 0);
#endif
	}
	pol->curlft.use_time = (unsigned long)xtime.tv_sec;

	if (pol->action == XFRM_POLICY_BLOCK) {
		xfrm_pol_put(pol);
		return 0;
	}

	if (pol->action == XFRM_POLICY_ALLOW) {
		struct sec_path *sp;
		static struct sec_path dummy;
#ifdef CONFIG_XFRM_ENHANCEMENT
		struct xfrm_state *xerr;
#endif
		int i, k;

		if ((sp = skb->sp) == NULL)
			sp = &dummy;

		/* For each tunnel xfrm, find the first matching tmpl.
		 * For each tmpl before that, find corresponding xfrm.
		 * Order is _important_. Later we will implement
		 * some barriers, but at the moment barriers
		 * are implied between each two transformations.
		 */
		for (i = pol->xfrm_nr-1, k = 0; i >= 0; i--) {
#ifdef CONFIG_XFRM_ENHANCEMENT
			xerr = NULL;
			k = xfrm_policy_ok(&fl, pol, pol->xfrm_vec+i, sp, k, family, &xerr);
			if (k < 0) {
				XFRM_DBG("%s: failed to check policy at tmpl proto = %u\n", __FUNCTION__, ((struct xfrm_tmpl *)pol->xfrm_vec+i)->id.proto);
				xfrm_post_reject(xerr, skb, &fl);
				goto reject;
			}
#else
			k = xfrm_policy_ok(pol->xfrm_vec+i, sp, k, family);
			if (k < 0)
				goto reject;
#endif
		}

#ifdef CONFIG_XFRM_ENHANCEMENT
		xerr = NULL;
		if (secpath_has_nontransport(sp, k, &xerr)) {
			XFRM_DBG("%s: unexpected non-transport mode\n", __FUNCTION__);
			xfrm_post_reject(xerr, skb, &fl);
			goto reject;
		}
#else
		if (secpath_has_tunnel(sp, k))
			goto reject;
#endif

		xfrm_pol_put(pol);
		return 1;
	}

reject:
	xfrm_pol_put(pol);
	return 0;
}
EXPORT_SYMBOL(__xfrm_policy_check);

int __xfrm_route_forward(struct sk_buff *skb, unsigned short family)
{
	struct flowi fl;

	if (xfrm_decode_session(skb, &fl, family) < 0)
		return 0;

	return xfrm_lookup(&skb->dst, &fl, NULL, 0) == 0;
}
EXPORT_SYMBOL(__xfrm_route_forward);

/* Optimize later using cookies and generation ids. */

static struct dst_entry *xfrm_dst_check(struct dst_entry *dst, u32 cookie)
{
	/* Code (such as __xfrm4_bundle_create()) sets dst->obsolete
	 * to "-1" to force all XFRM destinations to get validated by
	 * dst_ops->check on every use.  We do this because when a
	 * normal route referenced by an XFRM dst is obsoleted we do
	 * not go looking around for all parent referencing XFRM dsts
	 * so that we can invalidate them.  It is just too much work.
	 * Instead we make the checks here on every use.  For example:
	 *
	 *	XFRM dst A --> IPv4 dst X
	 *
	 * X is the "xdst->route" of A (X is also the "dst->path" of A
	 * in this example).  If X is marked obsolete, "A" will not
	 * notice.  That's what we are validating here via the
	 * stale_bundle() check.
	 *
	 * When a policy's bundle is pruned, we dst_free() the XFRM
	 * dst which causes it's ->obsolete field to be set to a
	 * positive non-zero integer.  If an XFRM dst has been pruned
	 * like this, we want to force a new route lookup.
	 */
	if (dst->obsolete < 0 && !stale_bundle(dst))
		return dst;

	return NULL;
}

static int stale_bundle(struct dst_entry *dst)
{
	return !xfrm_bundle_ok((struct xfrm_dst *)dst, NULL, AF_UNSPEC);
}

void xfrm_dst_ifdown(struct dst_entry *dst, struct net_device *dev)
{
	while ((dst = dst->child) && dst->xfrm && dst->dev == dev) {
		dst->dev = &loopback_dev;
		dev_hold(&loopback_dev);
		dev_put(dev);
	}
}
EXPORT_SYMBOL(xfrm_dst_ifdown);

static void xfrm_link_failure(struct sk_buff *skb)
{
	/* Impossible. Such dst must be popped before reaches point of failure. */
	return;
}

static struct dst_entry *xfrm_negative_advice(struct dst_entry *dst)
{
	if (dst) {
		if (dst->obsolete) {
			dst_release(dst);
			dst = NULL;
		}
	}
	return dst;
}

static void xfrm_prune_bundles(int (*func)(struct dst_entry *))
{
	int i;
	struct xfrm_policy *pol;
	struct dst_entry *dst, **dstp, *gc_list = NULL;

	read_lock_bh(&xfrm_policy_lock);
	for (i=0; i<2*XFRM_POLICY_MAX; i++) {
		for (pol = xfrm_policy_list[i]; pol; pol = pol->next) {
			write_lock(&pol->lock);
			dstp = &pol->bundles;
			while ((dst=*dstp) != NULL) {
				if (func(dst)) {
					*dstp = dst->next;
					dst->next = gc_list;
					gc_list = dst;
				} else {
					dstp = &dst->next;
				}
			}
			write_unlock(&pol->lock);
		}
	}
	read_unlock_bh(&xfrm_policy_lock);

	while (gc_list) {
		dst = gc_list;
		gc_list = dst->next;
		dst_free(dst);
	}
}

static int unused_bundle(struct dst_entry *dst)
{
	return !atomic_read(&dst->__refcnt);
}

static void __xfrm_garbage_collect(void)
{
	xfrm_prune_bundles(unused_bundle);
}

int xfrm_flush_bundles(void)
{
	xfrm_prune_bundles(stale_bundle);
	return 0;
}

static int always_true(struct dst_entry *dst)
{
	return 1;
}

void xfrm_flush_all_bundles(void)
{
	xfrm_prune_bundles(always_true);
}

void xfrm_init_pmtu(struct dst_entry *dst)
{
	do {
		struct xfrm_dst *xdst = (struct xfrm_dst *)dst;
		u32 pmtu, route_mtu_cached;

		pmtu = dst_mtu(dst->child);
		xdst->child_mtu_cached = pmtu;

		pmtu = xfrm_state_mtu(dst->xfrm, pmtu);

		route_mtu_cached = dst_mtu(xdst->route);
		xdst->route_mtu_cached = route_mtu_cached;

		if (pmtu > route_mtu_cached)
			pmtu = route_mtu_cached;

		dst->metrics[RTAX_MTU-1] = pmtu;
	} while ((dst = dst->next));
}

EXPORT_SYMBOL(xfrm_init_pmtu);

/* Check that the bundle accepts the flow and its components are
 * still valid.
 */

int xfrm_bundle_ok(struct xfrm_dst *first, struct flowi *fl, int family)
{
	struct dst_entry *dst = &first->u.dst;
	struct xfrm_dst *last;
	u32 mtu;

	if (!dst_check(dst->path, ((struct xfrm_dst *)dst)->path_cookie) ||
	    (dst->dev && !netif_running(dst->dev)))
		return 0;

	last = NULL;

	do {
		struct xfrm_dst *xdst = (struct xfrm_dst *)dst;

		if (fl && !xfrm_selector_match(&dst->xfrm->sel, fl, family))
			return 0;
		if (dst->xfrm->km.state != XFRM_STATE_VALID)
			return 0;
		if (xdst->genid != dst->xfrm->genid)
			return 0;

		mtu = dst_mtu(dst->child);
		if (xdst->child_mtu_cached != mtu) {
			last = xdst;
			xdst->child_mtu_cached = mtu;
		}

		if (!dst_check(xdst->route, xdst->route_cookie))
			return 0;
		mtu = dst_mtu(xdst->route);
		if (xdst->route_mtu_cached != mtu) {
			last = xdst;
			xdst->route_mtu_cached = mtu;
		}

		dst = dst->child;
	} while (dst->xfrm);

	if (likely(!last))
		return 1;

	mtu = last->child_mtu_cached;
	for (;;) {
		dst = &last->u.dst;

		mtu = xfrm_state_mtu(dst->xfrm, mtu);
		if (mtu > last->route_mtu_cached)
			mtu = last->route_mtu_cached;
		dst->metrics[RTAX_MTU-1] = mtu;

		if (last == first)
			break;

		last = last->u.next;
		last->child_mtu_cached = mtu;
	}

	return 1;
}

EXPORT_SYMBOL(xfrm_bundle_ok);

int xfrm_policy_register_afinfo(struct xfrm_policy_afinfo *afinfo)
{
	int err = 0;
	if (unlikely(afinfo == NULL))
		return -EINVAL;
	if (unlikely(afinfo->family >= NPROTO))
		return -EAFNOSUPPORT;
	write_lock_bh(&xfrm_policy_afinfo_lock);
	if (unlikely(xfrm_policy_afinfo[afinfo->family] != NULL))
		err = -ENOBUFS;
	else {
		struct dst_ops *dst_ops = afinfo->dst_ops;
		if (likely(dst_ops->kmem_cachep == NULL))
			dst_ops->kmem_cachep = xfrm_dst_cache;
		if (likely(dst_ops->check == NULL))
			dst_ops->check = xfrm_dst_check;
		if (likely(dst_ops->negative_advice == NULL))
			dst_ops->negative_advice = xfrm_negative_advice;
		if (likely(dst_ops->link_failure == NULL))
			dst_ops->link_failure = xfrm_link_failure;
		if (likely(afinfo->garbage_collect == NULL))
			afinfo->garbage_collect = __xfrm_garbage_collect;
		xfrm_policy_afinfo[afinfo->family] = afinfo;
	}
	write_unlock_bh(&xfrm_policy_afinfo_lock);
	return err;
}
EXPORT_SYMBOL(xfrm_policy_register_afinfo);

int xfrm_policy_unregister_afinfo(struct xfrm_policy_afinfo *afinfo)
{
	int err = 0;
	if (unlikely(afinfo == NULL))
		return -EINVAL;
	if (unlikely(afinfo->family >= NPROTO))
		return -EAFNOSUPPORT;
	write_lock_bh(&xfrm_policy_afinfo_lock);
	if (likely(xfrm_policy_afinfo[afinfo->family] != NULL)) {
		if (unlikely(xfrm_policy_afinfo[afinfo->family] != afinfo))
			err = -EINVAL;
		else {
			struct dst_ops *dst_ops = afinfo->dst_ops;
			xfrm_policy_afinfo[afinfo->family] = NULL;
			dst_ops->kmem_cachep = NULL;
			dst_ops->check = NULL;
			dst_ops->negative_advice = NULL;
			dst_ops->link_failure = NULL;
			afinfo->garbage_collect = NULL;
		}
	}
	write_unlock_bh(&xfrm_policy_afinfo_lock);
	return err;
}
EXPORT_SYMBOL(xfrm_policy_unregister_afinfo);

static struct xfrm_policy_afinfo *xfrm_policy_get_afinfo(unsigned short family)
{
	struct xfrm_policy_afinfo *afinfo;
	if (unlikely(family >= NPROTO))
		return NULL;
	read_lock(&xfrm_policy_afinfo_lock);
	afinfo = xfrm_policy_afinfo[family];
	if (unlikely(!afinfo))
		read_unlock(&xfrm_policy_afinfo_lock);
	return afinfo;
}

static void xfrm_policy_put_afinfo(struct xfrm_policy_afinfo *afinfo)
{
	read_unlock(&xfrm_policy_afinfo_lock);
}

static struct xfrm_policy_afinfo *xfrm_policy_lock_afinfo(unsigned int family)
{
	struct xfrm_policy_afinfo *afinfo;
	if (unlikely(family >= NPROTO))
		return NULL;
	write_lock_bh(&xfrm_policy_afinfo_lock);
	afinfo = xfrm_policy_afinfo[family];
	if (unlikely(!afinfo))
		write_unlock_bh(&xfrm_policy_afinfo_lock);
	return afinfo;
}

static void xfrm_policy_unlock_afinfo(struct xfrm_policy_afinfo *afinfo)
{
	write_unlock_bh(&xfrm_policy_afinfo_lock);
}

static int xfrm_dev_event(struct notifier_block *this, unsigned long event, void *ptr)
{
	switch (event) {
	case NETDEV_DOWN:
		xfrm_flush_bundles();
	}
	return NOTIFY_DONE;
}

static struct notifier_block xfrm_dev_notifier = {
	xfrm_dev_event,
	NULL,
	0
};

static void __init xfrm_policy_init(void)
{
	xfrm_dst_cache = kmem_cache_create("xfrm_dst_cache",
					   sizeof(struct xfrm_dst),
					   0, SLAB_HWCACHE_ALIGN,
					   NULL, NULL);
	if (!xfrm_dst_cache)
		panic("XFRM: failed to allocate xfrm_dst_cache\n");

	INIT_WORK(&xfrm_policy_gc_work, xfrm_policy_gc_task, NULL);
	register_netdevice_notifier(&xfrm_dev_notifier);
}

void __init xfrm_init(void)
{
	xfrm_state_init();
	xfrm_policy_init();
	xfrm_input_init();
}

