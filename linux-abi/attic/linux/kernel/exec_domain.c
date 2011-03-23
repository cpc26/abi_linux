#include <linux/mm.h>
#include <linux/smp_lock.h>
#include <linux/module.h>
#include <linux/kmod.h>

static asmlinkage void no_lcall7(int segment, struct pt_regs * regs);


static unsigned long ident_map[32] = {
	0,	1,	2,	3,	4,	5,	6,	7,
	8,	9,	10,	11,	12,	13,	14,	15,
	16,	17,	18,	19,	20,	21,	22,	23,
	24,	25,	26,	27,	28,	29,	30,	31
};

struct exec_domain default_exec_domain = {
	"Linux",	/* name */
	no_lcall7,	/* lcall7 causes a seg fault. */
	0, 0,		/* PER_LINUX personality. */
	ident_map,	/* Identity map signals. */
	ident_map,	/*  - both ways. */
	NULL,		/* Identity map errors */
	NULL,		/* Identity map socket types. */
	NULL,		/* Identity map socket options. */
	NULL,		/* Identity map af_types. */
	NULL,		/* No usage counter. */
	NULL		/* Nothing after this in the list. */
};

static struct exec_domain *exec_domains = &default_exec_domain;
static spinlock_t exec_domains_lock = SPIN_LOCK_UNLOCKED;

static asmlinkage void no_lcall7(int segment, struct pt_regs * regs)
{
	int new_personality = 0;
  /*
   * This may have been a static linked SVr4 binary, so we would have the
   * personality set incorrectly.  Or it might have been a Solaris/x86
   * binary. We can tell which because the former uses lcall7, while
   * the latter used lcall 0x27.
   * Try to find or load the appropriate personality, and fall back to
   * just forcing a SEGV.
   */
	if (segment == 0x27)
		new_personality = PER_SOLARIS;
	else if (segment == 7)
		new_personality = PER_SVR4;
	
	if (new_personality) {
		put_exec_domain(current->exec_domain);
		current->personality = PER_SVR4;
		current->exec_domain = lookup_exec_domain(current->personality);

		if (current->exec_domain && current->exec_domain->handler
		&& current->exec_domain->handler != no_lcall7) {
			current->exec_domain->handler(segment, regs);
			return;
		}
	}

	send_sig(SIGSEGV, current, 1);
}

struct exec_domain *lookup_exec_domain(unsigned long personality)
{
	unsigned long pers = personality & PER_MASK;
	struct exec_domain *it;
#ifdef CONFIG_KMOD
	char buffer[30];
#endif
	
	spin_lock(&exec_domains_lock);
	for(it=exec_domains; it; it=it->next)
		if (pers >= it->pers_low && pers <= it->pers_high) {
			spin_unlock(&exec_domains_lock);
			return it;
		}
	spin_unlock(&exec_domains_lock);
	
#ifdef CONFIG_KMOD
	sprintf(buffer, "abi-personality-%ld", pers);
	request_module(buffer);

	spin_lock(&exec_domains_lock);
	for (it=exec_domains; it; it=it->next)
		if (pers >= it->pers_low && pers <= it->pers_high) {
			if (!try_inc_mod_count(it->module))
				continue;
			spin_unlock(&exec_domains_lock);
			return it;
		}
	spin_unlock(&exec_domains_lock);
#endif

	return &default_exec_domain;
}

int register_exec_domain(struct exec_domain *it)
{
	struct exec_domain *tmp;

	if (!it)
		return -EINVAL;
	if (it->next)
		return -EBUSY;
	spin_lock(&exec_domains_lock);
	for (tmp=exec_domains; tmp; tmp=tmp->next)
		if (tmp == it) {
			spin_unlock(&exec_domains_lock);
			return -EBUSY;
		}
	it->next = exec_domains;
	exec_domains = it;
	spin_unlock(&exec_domains_lock);
	return 0;
}

int unregister_exec_domain(struct exec_domain *it)
{
	struct exec_domain ** tmp;

	tmp = &exec_domains;
	spin_lock(&exec_domains_lock);
	while (*tmp) {
		if (it == *tmp) {
			*tmp = it->next;
			it->next = NULL;
			spin_unlock(&exec_domains_lock);
			return 0;
		}
		tmp = &(*tmp)->next;
	}
	spin_unlock(&exec_domains_lock);
	return -EINVAL;
}

asmlinkage long sys_personality(unsigned long personality)
{
	struct exec_domain *it;
	unsigned long old_personality;
	int ret;

	if (personality == 0xffffffff)
		return current->personality;

	ret = -EINVAL;
	lock_kernel();
	it = lookup_exec_domain(personality);
	if (!it)
		goto out;

	old_personality = current->personality;
	put_exec_domain(current->exec_domain);
	current->personality = personality;
	current->exec_domain = it;
	ret = old_personality;
out:
	unlock_kernel();
	return ret;
}

int get_exec_domain_list(char * page)
{
	int len = 0;
	struct exec_domain * e;

	spin_lock(&exec_domains_lock);
	for (e=exec_domains; e && len < PAGE_SIZE - 80; e=e->next)
		len += sprintf(page+len, "%d-%d\t%-16s\t[%s]\n",
			e->pers_low, e->pers_high, e->name,
			e->module ? e->module->name : "kernel");
	spin_unlock(&exec_domains_lock);
	return len;
}
