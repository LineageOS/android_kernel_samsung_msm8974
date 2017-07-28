/*
 * arch/arm/mm/proc-v7-2level.S
 *
 * Copyright (C) 2001 Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define TTB_S		(1 << 1)
#define TTB_RGN_NC	(0 << 3)
#define TTB_RGN_OC_WBWA	(1 << 3)
#define TTB_RGN_OC_WT	(2 << 3)
#define TTB_RGN_OC_WB	(3 << 3)
#define TTB_NOS		(1 << 5)
#define TTB_IRGN_NC	((0 << 0) | (0 << 6))
#define TTB_IRGN_WBWA	((0 << 0) | (1 << 6))
#define TTB_IRGN_WT	((1 << 0) | (0 << 6))
#define TTB_IRGN_WB	((1 << 0) | (1 << 6))

/* PTWs cacheable, inner WB not shareable, outer WB not shareable */
#define TTB_FLAGS_UP	TTB_IRGN_WB|TTB_RGN_OC_WB
#define PMD_FLAGS_UP	PMD_SECT_WB

/* PTWs cacheable, inner WBWA shareable, outer WBWA not shareable */
#define TTB_FLAGS_SMP	TTB_IRGN_WBWA|TTB_S|TTB_NOS|TTB_RGN_OC_WBWA
#define PMD_FLAGS_SMP	PMD_SECT_WBWA|PMD_SECT_S

#ifdef        CONFIG_TIMA_RKP
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
        .arch_extension sec
#endif
#endif

/*
 *	cpu_v7_switch_mm(pgd_phys, tsk)
 *
 *	Set the translation table base pointer to be pgd_phys
 *
 *	- pgd_phys - physical address of new TTB
 *
 *	It is assumed that:
 *	- we are not using split page tables
 */
ENTRY(cpu_v7_switch_mm)
#ifdef CONFIG_MMU
	mov	r2, #0
	ldr	r1, [r1, #MM_CONTEXT_ID]	@ get mm->context.id
	ALT_SMP(orr	r0, r0, #TTB_FLAGS_SMP)
	ALT_UP(orr	r0, r0, #TTB_FLAGS_UP)
#ifdef CONFIG_ARM_ERRATA_430973
	mcr	p15, 0, r2, c7, c5, 6		@ flush BTAC/BTB
#endif
#ifdef CONFIG_PID_IN_CONTEXTIDR
	mrc     p15, 0, r2, c13, c0, 1          @ read current context ID
	bic     r2, r2, #0xff                   @ extract the PID
	and     r1, r1, #0xff
	orr     r1, r1, r2                      @ insert the PID into r1
#endif
#ifdef CONFIG_TIMA_RKP
        stmfd   sp!, {r2, r11}
       /* TZ side expects r0 to be in r11 */
        mov     r11,    r0
       /* r2 : mcr_val */
        mov     r2, r0
       /* r0: command */
        ldr     r0, =0x3f803221
        smc     #1
       /* Restore r0 and r2 */
        ldmfd   sp!, {r2, r11}
#ifdef CONFIG_TIMA_RKP_30
	/* Flush TLB */
	mcr    p15, 0, r0, c8, c3, 0
	dsb
#endif
#endif

	isb
#ifdef CONFIG_ARM_ERRATA_754322
	dsb
#endif
	mcr	p15, 0, r1, c13, c0, 1		@ set context ID
	isb
	mcr	p15, 0, r0, c2, c0, 0		@ set TTB 0
	isb
#endif
	mov	pc, lr
ENDPROC(cpu_v7_switch_mm)

#ifdef CONFIG_TIMA_RKP_L2_GROUP
/* 
 * Pass the pointer to the group buffer and its length to TZ side. 
 * cpu_v7_timal2group_set_pte_commit(tima_l2group_entry_ptr,
 * 				tima_l2group_entries_count)
 */
ENTRY(cpu_v7_timal2group_set_pte_commit)
	/* Second parameter is the length, move it to r2 */
	mov	r2, r1
	/* First parameter is the address, move it to r1 */
	mov	r1, r0
	stmfd   sp!,{r0-r3, r11}
	stmfd   sp!,{r0}
	mov	r11, r3       
	ldr     r0, =0x3f811221        @cmd_id=0x11
	smc     #1
	pop	{r0}
	pop     {r0-r3, r11}
	mov	pc, lr
ENDPROC(cpu_v7_timal2group_set_pte_commit)

/*  
 * Function writes the pte addr, arm pte value and linux
 * pte value at the address pointed to by the L2 group
 * pointer.
 * cpu_v7_timal2group_set_pte_ext(pte_t *ptep, pte_t pte, 
 *	unsigned int ext, unsigned long tima_l2group_entry_ptr);
 */
ENTRY(cpu_v7_timal2group_set_pte_ext)
#ifdef CONFIG_MMU
        /* r3 has the pointer to the new entry */
	stmfd	sp!,{r4}
	stmfd	sp!,{r5}
        stmfd	sp!,{r1}
	stmfd	sp!,{r3}

	bic	r3, r1, #0x000003f0
	bic	r3, r3, #PTE_TYPE_MASK
	orr	r3, r3, r2
	orr	r3, r3, #PTE_EXT_AP0 | 2

	tst	r1, #1 << 4
	orrne	r3, r3, #PTE_EXT_TEX(1)

	eor	r1, r1, #L_PTE_DIRTY
	tst	r1, #L_PTE_RDONLY | L_PTE_DIRTY
	orrne	r3, r3, #PTE_EXT_APX

	tst	r1, #L_PTE_USER
	orrne	r3, r3, #PTE_EXT_AP1
#ifdef CONFIG_CPU_USE_DOMAINS
	@ allow kernel read/write access to read-only user pages
	tstne	r3, #PTE_EXT_APX
	bicne	r3, r3, #PTE_EXT_APX | PTE_EXT_AP0
#endif

	tst	r1, #L_PTE_XN
	orrne	r3, r3, #PTE_EXT_XN

	tst	r1, #L_PTE_YOUNG
	tstne	r1, #L_PTE_PRESENT
	moveq	r3, #0

	/* grouped operations*/
	/*pop back the buffer pointer to r2*/
	pop	{r2}
	pop	{r1}

	mov	r5, #0
	ldr     r4, [r0]
	cmp     r1, r4	/* r1 has linux pte value */
	bne	1f
	ldr     r4, [r0, #2048]
	cmp     r3, r4	/* r3 has arm pte value */
	bne	1f
	mov	r5, #5
	b 	2f
1:
	//ldr	r0, =0xdeadbee1
	/* r0 has the pointer to the pte */
	str	r0, [r2]
	/* flush cache after the str */
	mcr     p15, 0, r2, c7, c14, 1          
	//ldr	r1, =0xdeadbee2
	add	r2, r2, #4
	/* r1 has the linux pte value */
	str	r1, [r2]
	/* flush cache after the str */
	mcr     p15, 0, r2, c7, c14, 1  
	//ldr	r3, =0xdeadbee3
	add	r2, r2, #4
	/* r3 has the arm pte value */
	str	r3, [r2]
	/* flush cache after the str */
	mcr     p15, 0, r2, c7, c14, 1
	dsb
	isb
#endif
2:	
	mov	r0, r5
	pop	{r5}
	pop	{r4}
	mov	pc, lr
ENDPROC(cpu_v7_timal2group_set_pte_ext)
#endif  /* CONFIG_TIMA_RKP_L2_GROUP */

#ifdef CONFIG_TIMA_RKP
ENTRY(cpu_v7_tima_iommu_opt)
	
	stmfd   sp!,{r0-r3, r11}
	mov	r3, r2
	/* Second parameter is the end addr, move it to r2 */
	mov	r2, r1
	/* First parameter is the start addr, move it to r1 */
	mov	r1, r0
	stmfd   sp!,{r0}
	mov	r11, r3       
	ldr     r0, =0x3f820221        @cmd_id=0x20
	smc     #1
	pop	{r0}
	pop     {r0-r3, r11}
	mov	pc, lr
ENDPROC(cpu_v7_tima_iommu_opt)
#endif

/*
 *	cpu_v7_set_pte_ext(ptep, pte)
 *
 *	Set a level 2 translation table entry.
 *
 *	- ptep  - pointer to level 2 translation table entry
 *		  (hardware version is stored at +2048 bytes)
 *	- pte   - PTE value to store
 *	- ext	- value for extended PTE bits
 */
#ifdef CONFIG_TIMA_RKP_L2_TABLES
ENTRY(cpu_v7_tima_set_pte_ext)
# ifdef CONFIG_MMU
        stmfd   sp!, {r1}	//added for tima
	bic	r3, r1, #0x000003f0
	bic	r3, r3, #PTE_TYPE_MASK
	orr	r3, r3, r2
	orr	r3, r3, #PTE_EXT_AP0 | 2

	tst	r1, #1 << 4
	orrne	r3, r3, #PTE_EXT_TEX(1)

	eor	r1, r1, #L_PTE_DIRTY
	tst	r1, #L_PTE_RDONLY | L_PTE_DIRTY
	orrne	r3, r3, #PTE_EXT_APX

	tst	r1, #L_PTE_USER
	orrne	r3, r3, #PTE_EXT_AP1
#ifdef CONFIG_CPU_USE_DOMAINS
	@ allow kernel read/write access to read-only user pages
	tstne	r3, #PTE_EXT_APX
	bicne	r3, r3, #PTE_EXT_APX | PTE_EXT_AP0
#endif

	tst	r1, #L_PTE_XN
	orrne	r3, r3, #PTE_EXT_XN

	tst	r1, #L_PTE_YOUNG
	tstne	r1, #L_PTE_PRESENT
	moveq	r3, #0

	/*next part is added for tima*/
	/** Check if the kernel is re-writing the same value!
	 *  In case of tima, such rewrites are unnecessary and expensive.
	 */
	ldr     r2, [r0]
	cmp     r1, r2
	bne	1f
	ldr     r2, [r0, #2048]
	cmp     r3, r2
	beq 	4f
1:
	/** Prepare the stack for L2 write emulation!
	 */	
	pop	{r1}	 	
	stmfd   sp!,{r0-r3, r11}
	stmfd   sp!,{r0}
	mov     r11, r0
	/** Cache invalidate the values to be written
	 */
#ifndef	CONFIG_TIMA_RKP_COHERENT_TT
	mcr     p15, 0, r0, c7, c14, 1          @ flush_linux_pte
#endif
	add     r0, r0, #2048
#ifndef	CONFIG_TIMA_RKP_COHERENT_TT
	mcr     p15, 0, r0, c7, c14, 1          @ flush_arm_pte
	dsb
	isb
#endif
	ldr     r0, =0x3f807221
	smc     #1
	pop	{r0}
	/** Cache invalidate again to ensure no 
	 *  values re-cached by speculative fetching
	 */
#ifndef	CONFIG_TIMA_RKP_COHERENT_TT
	mcr     p15, 0, r0, c7, c6,  1
	dsb
	isb
#endif
#ifdef CONFIG_TIMA_RKP_DEBUG
	ldr     r2, [r0]
	cmp     r1, r2
	beq	2f     
	stmfd	sp!, {r0}
	ldr	r0, =0x3f80f221		@cmd_id=0x0f
	smc	#1	
	pop	{r0}
2:
#endif

	add	r0, r0, #2048
#ifndef	CONFIG_TIMA_RKP_COHERENT_TT
	mcr     p15, 0, r0, c7, c6,  1
	dsb
	isb
#else
	mcr	p15, 0, r0, c7, c10, 1		@ flush_pte
	dsb
	isb
#endif
#ifdef CONFIG_TIMA_RKP_DEBUG
	ldr     r2, [r0]
	cmp     r3, r2
	beq	3f
	stmfd	sp!, {r0}
	ldr	r0, =0x3f80f221		@cmd_id=0x0f
	smc	#1
	pop	{r0}
3:
#endif

	pop     {r0-r3, r11}
4:

#endif
	mov	pc, lr
ENDPROC(cpu_v7_tima_set_pte_ext)
#endif

#ifdef	CONFIG_TIMA_RKP
	.global cpu_v7_set_pte_ext_proc_end
#endif
ENTRY(cpu_v7_set_pte_ext)
#ifdef CONFIG_MMU
	str	r1, [r0]			@ linux version
	bic	r3, r1, #0x000003f0
	bic	r3, r3, #PTE_TYPE_MASK
	orr	r3, r3, r2
	orr	r3, r3, #PTE_EXT_AP0 | 2

	tst	r1, #1 << 4
	orrne	r3, r3, #PTE_EXT_TEX(1)

	eor	r1, r1, #L_PTE_DIRTY
	tst	r1, #L_PTE_RDONLY | L_PTE_DIRTY
	orrne	r3, r3, #PTE_EXT_APX

	tst	r1, #L_PTE_USER
	orrne	r3, r3, #PTE_EXT_AP1
#ifdef CONFIG_CPU_USE_DOMAINS
	@ allow kernel read/write access to read-only user pages
	tstne	r3, #PTE_EXT_APX
	bicne	r3, r3, #PTE_EXT_APX | PTE_EXT_AP0
#endif

	tst	r1, #L_PTE_XN
	orrne	r3, r3, #PTE_EXT_XN

	tst	r1, #L_PTE_YOUNG
	tstne	r1, #L_PTE_PRESENT
	moveq	r3, #0

 ARM(	str	r3, [r0, #2048]! )
 THUMB(	add	r0, r0, #2048 )
 THUMB(	str	r3, [r0] )
	mcr	p15, 0, r0, c7, c10, 1		@ flush_pte
#endif
	mov	pc, lr
#ifdef	CONFIG_TIMA_RKP
cpu_v7_set_pte_ext_proc_end:
#endif
ENDPROC(cpu_v7_set_pte_ext)

	/*
	 * Memory region attributes with SCTLR.TRE=1
	 *
	 *   n = TEX[0],C,B
	 *   TR = PRRR[2n+1:2n]		- memory type
	 *   IR = NMRR[2n+1:2n]		- inner cacheable property
	 *   OR = NMRR[2n+17:2n+16]	- outer cacheable property
	 *
	 *			n	TR	IR	OR
	 *   UNCACHED		000	00
	 *   BUFFERABLE		001	10	00	00
	 *   WRITETHROUGH	010	10	10	10
	 *   WRITEBACK		011	10	11	11
	 *   reserved		110
	 *   WRITEALLOC		111	10	01	01
	 *   DEV_SHARED		100	01
	 *   DEV_NONSHARED	100	01
	 *   DEV_WC		001	10
	 *   DEV_CACHED		011	10
	 *
	 * Other attributes:
	 *
	 *   DS0 = PRRR[16] = 0		- device shareable property
	 *   DS1 = PRRR[17] = 1		- device shareable property
	 *   NS0 = PRRR[18] = 0		- normal shareable property
	 *   NS1 = PRRR[19] = 1		- normal shareable property
	 *   NOS = PRRR[24+n] = 1	- not outer shareable
	 */
.equ	PRRR,	0xff0a81a8
#ifdef CONFIG_ARCH_MSM_SCORPIONMP
.equ	NMRR,	0x40e080e0
#else
.equ	NMRR,	0x40e040e0
#endif

	/*
	 * Macro for setting up the TTBRx and TTBCR registers.
	 * - \ttb0 and \ttb1 updated with the corresponding flags.
	 */
	.macro	v7_ttb_setup, zero, ttbr0, ttbr1, tmp
#ifdef	CONFIG_TIMA_RKP_EMUL_CP15_INSTR 
	ldr	r0, =0x3f805221
	smc	#1
#else 
	mcr	p15, 0, \zero, c2, c0, 2	@ TTB control register
#endif
	ALT_SMP(orr	\ttbr0, \ttbr0, #TTB_FLAGS_SMP)
	ALT_UP(orr	\ttbr0, \ttbr0, #TTB_FLAGS_UP)
	ALT_SMP(orr	\ttbr1, \ttbr1, #TTB_FLAGS_SMP)
	ALT_UP(orr	\ttbr1, \ttbr1, #TTB_FLAGS_UP)
	mcr	p15, 0, \ttbr1, c2, c0, 1	@ load TTB1
	.endm

	__CPUINIT

	/*   AT
	 *  TFR   EV X F   I D LR    S
	 * .EEE ..EE PUI. .T.T 4RVI ZWRS BLDP WCAM
	 * rxxx rrxx xxx0 0101 xxxx xxxx x111 xxxx < forced
	 *    1    0 110       0011 1100 .111 1101 < we want
	 */
	.align	2
	.type	v7_crval, #object
v7_crval:
	crval	clear=0x0120c302, mmuset=0x10c03c7d, ucset=0x00c01c7c

	.previous
