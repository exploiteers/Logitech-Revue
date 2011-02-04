#ifndef MODULE_SIG
#define MODULE_SIG

#ifdef CONFIG_MODULE_SIG
extern int module_check_sig(Elf_Ehdr *hdr, unsigned int len);
#else
static int inline module_check_sig(Elf_Ehdr *hdr, unsigned int len)
{
	return 0;
}
#endif

#endif
