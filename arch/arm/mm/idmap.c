#include <linux/kernel.h>

#include <asm/cputype.h>
#include <asm/idmap.h>
#include <asm/pgalloc.h>
#include <asm/pgtable.h>
#include <asm/sections.h>
#include <asm/system_info.h>

pgd_t *idmap_pgd;

#ifdef CONFIG_ARM_LPAE
static void idmap_add_pmd(pud_t *pud, unsigned long addr, unsigned long end,
	unsigned long prot)
{
	pmd_t *pmd;
	unsigned long next;

	if (pud_none_or_clear_bad(pud) || (pud_val(*pud) & L_PGD_SWAPPER)) {
		pmd = pmd_alloc_one(&init_mm, addr);
		if (!pmd) {
			pr_warning("Failed to allocate identity pmd.\n");
			return;
		}
		/*
		 * Copy the original PMD to ensure that the PMD entries for
		 * the kernel image are preserved.
		 */
		if (!pud_none(*pud))
			memcpy(pmd, pmd_offset(pud, 0),
			       PTRS_PER_PMD * sizeof(pmd_t));
		pud_populate(&init_mm, pud, pmd);
		pmd += pmd_index(addr);
	} else
		pmd = pmd_offset(pud, addr);

	do {
		next = pmd_addr_end(addr, end);
		*pmd = __pmd((addr & PMD_MASK) | prot);
		flush_pmd_entry(pmd);
	} while (pmd++, addr = next, addr != end);
}
#else	/* !CONFIG_ARM_LPAE */
static void idmap_add_pmd(pud_t *pud, unsigned long addr, unsigned long end,
	unsigned long prot)
{
#ifdef	CONFIG_TIMA_RKP_L1_TABLES
	unsigned long cmd_id = 0x3f809221;
	unsigned long tima_wr_out;
#endif
	pmd_t *pmd = pmd_offset(pud, addr);

	addr = (addr & PMD_MASK) | prot;

#ifdef	CONFIG_TIMA_RKP_L1_TABLES
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
        __asm__ __volatile__(".arch_extension sec");
#endif
	clean_dcache_area(pmd, 8);
	__asm__ __volatile__ (
		"stmfd  sp!,{r0, r8-r11}\n"
		"mov   	r11, r0\n"
		"mov    r0, %1\n"
		"mov	r8, %2\n"
		"mov    r9, %3\n"
		"mov    r10, %4\n"
		"mcr    p15, 0, r8, c7, c14, 1\n"
		"add    r8, r8, #4\n"
		"mcr    p15, 0, r8, c7, c14, 1\n"
		"dsb\n"
		"smc    #9\n"
		"sub    r8, r8, #4\n"
		"mcr    p15, 0, r8, c7, c6,  1\n"
		"dsb\n"

		"mov    %0, r10\n"
		"add    r8, r8, #4\n"
		"mcr    p15, 0, r8, c7, c6,  1\n"
		"dsb\n"

		"mov    r0, #0\n"
		"mcr    p15, 0, r0, c8, c3, 0\n"
		"dsb\n"
		"isb\n"
		"pop    {r0, r8-r11}\n"
		:"=r"(tima_wr_out):"r"(cmd_id),"r"((unsigned long)pmd),"r"(addr),"r"(addr + SECTION_SIZE):"r0","r8","r9","r10","r11","cc");

	if ((pmd[0]|0x4) != (addr|0x4)) {
		printk(KERN_ERR"pmd[0] %lx != addr %lx - %lx %lx in func: %s tima_wr_out = %lx\n",
				(unsigned long) pmd[0], addr, (unsigned long) pmd[1], addr + SECTION_SIZE, __func__, tima_wr_out);
	}
	if ((pmd[1]|0x4)!=((addr + SECTION_SIZE)|0x4)) {
		printk(KERN_ERR"pmd[1] %lx != (addr + SECTION_SIZE) %lx in func: %s\n",
				(unsigned long) pmd[1], (addr + SECTION_SIZE), __func__);
	}
#else
	pmd[0] = __pmd(addr);
	addr += SECTION_SIZE;
	pmd[1] = __pmd(addr);
#endif
	flush_pmd_entry(pmd);
}
#endif	/* CONFIG_ARM_LPAE */

static void idmap_add_pud(pgd_t *pgd, unsigned long addr, unsigned long end,
	unsigned long prot)
{
	pud_t *pud = pud_offset(pgd, addr);
	unsigned long next;

	do {
		next = pud_addr_end(addr, end);
		idmap_add_pmd(pud, addr, next, prot);
	} while (pud++, addr = next, addr != end);
}

static void identity_mapping_add(pgd_t *pgd, unsigned long addr, unsigned long end)
{
	unsigned long prot, next;

	prot = PMD_TYPE_SECT | PMD_SECT_AP_WRITE | PMD_SECT_AF;
	if (cpu_architecture() <= CPU_ARCH_ARMv5TEJ && !cpu_is_xscale())
		prot |= PMD_BIT4;

	pgd += pgd_index(addr);
	do {
		next = pgd_addr_end(addr, end);
		idmap_add_pud(pgd, addr, next, prot);
	} while (pgd++, addr = next, addr != end);
}

extern char  __idmap_text_start[], __idmap_text_end[];

static int __init init_static_idmap(void)
{
	phys_addr_t idmap_start, idmap_end;

	idmap_pgd = pgd_alloc(&init_mm);
	if (!idmap_pgd)
		return -ENOMEM;

	/* Add an identity mapping for the physical address of the section. */
	idmap_start = virt_to_phys((void *)__idmap_text_start);
	idmap_end = virt_to_phys((void *)__idmap_text_end);

	pr_info("Setting up static identity map for 0x%llx - 0x%llx\n",
		(long long)idmap_start, (long long)idmap_end);
	identity_mapping_add(idmap_pgd, idmap_start, idmap_end);

	return 0;
}
early_initcall(init_static_idmap);

/*
 * In order to soft-boot, we need to switch to a 1:1 mapping for the
 * cpu_reset functions. This will then ensure that we have predictable
 * results when turning off the mmu.
 */
void setup_mm_for_reboot(void)
{
	/* Clean and invalidate L1. */
	flush_cache_all();

	/* Switch to the identity mapping. */
	cpu_switch_mm(idmap_pgd, &init_mm);

	/* Flush the TLB. */
	local_flush_tlb_all();
}
