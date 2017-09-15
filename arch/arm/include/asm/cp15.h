#ifndef __ASM_ARM_CP15_H
#define __ASM_ARM_CP15_H

#include <asm/barrier.h>

/*
 * CR1 bits (CP#15 CR1)
 */
#define CR_M	(1 << 0)	/* MMU enable				*/
#define CR_A	(1 << 1)	/* Alignment abort enable		*/
#define CR_C	(1 << 2)	/* Dcache enable			*/
#define CR_W	(1 << 3)	/* Write buffer enable			*/
#define CR_P	(1 << 4)	/* 32-bit exception handler		*/
#define CR_D	(1 << 5)	/* 32-bit data address range		*/
#define CR_L	(1 << 6)	/* Implementation defined		*/
#define CR_B	(1 << 7)	/* Big endian				*/
#define CR_S	(1 << 8)	/* System MMU protection		*/
#define CR_R	(1 << 9)	/* ROM MMU protection			*/
#define CR_F	(1 << 10)	/* Implementation defined		*/
#define CR_Z	(1 << 11)	/* Implementation defined		*/
#define CR_I	(1 << 12)	/* Icache enable			*/
#define CR_V	(1 << 13)	/* Vectors relocated to 0xffff0000	*/
#define CR_RR	(1 << 14)	/* Round Robin cache replacement	*/
#define CR_L4	(1 << 15)	/* LDR pc can set T bit			*/
#define CR_DT	(1 << 16)
#define CR_IT	(1 << 18)
#define CR_ST	(1 << 19)
#define CR_FI	(1 << 21)	/* Fast interrupt (lower latency mode)	*/
#define CR_U	(1 << 22)	/* Unaligned access operation		*/
#define CR_XP	(1 << 23)	/* Extended page tables			*/
#define CR_VE	(1 << 24)	/* Vectored interrupts			*/
#define CR_EE	(1 << 25)	/* Exception (Big) Endian		*/
#define CR_TRE	(1 << 28)	/* TEX remap enable			*/
#define CR_AFE	(1 << 29)	/* Access flag enable			*/
#define CR_TE	(1 << 30)	/* Thumb exception enable		*/

#ifndef __ASSEMBLY__

#if __LINUX_ARM_ARCH__ >= 4
#define vectors_high()	(cr_alignment & CR_V)
#else
#define vectors_high()	(0)
#endif

extern unsigned long cr_no_alignment;	/* defined in entry-armv.S */
extern unsigned long cr_alignment;	/* defined in entry-armv.S */

static inline unsigned int get_cr(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c1, c0, 0	@ get CR" : "=r" (val) : : "cc");
	return val;
}

#ifdef CONFIG_TIMA_RKP
 
void tima_dump_log2(void);
void tima_verify_state(unsigned long pmdp, unsigned long val, unsigned long rd_only, unsigned long caller);
int tima_is_pg_protected(unsigned long va);

#define BUILD_CMD_ID(cmdid) ((0x3f8<<20)|(cmdid <<12)|0x221)

static inline void tima_send_cmd (unsigned int r2val, unsigned int cmdid)
{
    volatile unsigned int tima_cmdid = BUILD_CMD_ID(cmdid);
	asm volatile (
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
        ".arch_extension sec\n"
#endif	
	    "stmfd   sp!, {r0-r3, r11}\n"
        "mov     r11, r0\n"
        "mov     r2, %0\n" 
		"mov     r0, %1\n"
		"smc     #1\n" 
        "ldmfd   sp!, {r0-r3, r11}" : : "r" (r2val), "r" (tima_cmdid) : "r0","r2","cc");
}
static inline void tima_send_cmd2 (unsigned int p1, unsigned int p2, unsigned int cmdid)
{
    volatile unsigned int tima_cmdid = BUILD_CMD_ID(cmdid);
	asm volatile (
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
        ".arch_extension sec\n"
#endif	
	"stmfd   sp!, {r0-r3, r11}\n"
        "mov     r11, r0\n"
        "mov     r2, %0\n"
	"mov     r3, %1\n"  
	"mov     r0, %2\n"
	"smc     #1\n" 
        "ldmfd   sp!, {r0-r3, r11}" : : "r" (p1), "r" (p2), "r" (tima_cmdid) : "r0","r2","r3","cc");
}
static inline void tima_send_cmd3 (unsigned int p1, unsigned int p2, unsigned int p3, unsigned int cmdid)
{
    volatile unsigned int tima_cmdid = BUILD_CMD_ID(cmdid);
	asm volatile (
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
        ".arch_extension sec\n"
#endif	
	"stmfd   sp!, {r0-r4, r11}\n"
        "mov     r11, r0\n"
        "mov     r2, %0\n"
	"mov     r3, %1\n"  
	"mov     r4, %2\n"  
	"mov     r0, %3\n"
	"smc     #1\n" 
        "ldmfd   sp!, {r0-r4, r11}" : : "r" (p1), "r" (p2), "r" (p3), "r" (tima_cmdid) : "r0","r2","r3","r4","cc");
}
static inline void tima_send_cmd4 (unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned int cmdid)
{
    volatile unsigned int tima_cmdid = BUILD_CMD_ID(cmdid);
	asm volatile (
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
        ".arch_extension sec\n"
#endif	
	"stmfd   sp!, {r0-r5, r11}\n"
        "mov     r11, r0\n"
        "mov     r2, %0\n"
	"mov     r3, %1\n"  
	"mov     r4, %2\n"  
	"mov     r5, %3\n"  
	"mov     r0, %4\n"
	"smc     #1\n" 
        "ldmfd   sp!, {r0-r5, r11}" : : "r" (p1), "r" (p2), "r" (p3), "r" (p4), "r" (tima_cmdid) : "r0","r2","r3","r4","r5","cc");
}

static inline void tima_send_cmd5 (unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned int p5,unsigned int cmdid)
{
    volatile unsigned int tima_cmdid = BUILD_CMD_ID(cmdid);
	asm volatile (
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
        ".arch_extension sec\n"
#endif	
	"stmfd   sp!, {r0-r6, r11}\n"
        "mov     r11, r0\n"
        "mov     r2, %0\n"
	"mov     r3, %1\n"  
	"mov     r4, %2\n"  
	"mov     r5, %3\n"  
	"mov     r6, %4\n"  
	"mov     r0, %5\n"
	"smc     #1\n" 
        "ldmfd   sp!, {r0-r6, r11}" : : "r" (p1), "r" (p2), "r" (p3), "r" (p4), "r" (p5),"r" (tima_cmdid) : "r0","r2","r3","r4","r5","r6","cc");
}

#define tima_cache_flush(x)						\
	__asm__ __volatile__(	"mcr     p15, 0, %0, c7, c14, 1\n"	\
				"dsb\n"					\
				"isb\n"					\
                		: : "r" (x))
#define tima_cache_inval(x)						\
	__asm__ __volatile__(	"mcr     p15, 0, %0, c7, c6, 1\n"	\
				"dsb\n"					\
				"isb\n"					\
                		: : "r" (x))

#define tima_tlb_inval_is(x)						\
	__asm__ __volatile__(	"mcr     p15, 0, %0, c8, c3, 0\n"	\
				"dsb\n"					\
				"isb\n"					\
                		: : "r" (x))
#endif	/* CONFIG_TIMA_RKP */
static inline void set_cr(unsigned int val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 0	@ set CR"
	  : : "r" (val) : "cc");
	isb();
}

#ifndef CONFIG_SMP
extern void adjust_cr(unsigned long mask, unsigned long set);
#endif

#define CPACC_FULL(n)		(3 << (n * 2))
#define CPACC_SVC(n)		(1 << (n * 2))
#define CPACC_DISABLE(n)	(0 << (n * 2))

static inline unsigned int get_copro_access(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c1, c0, 2 @ get copro access"
	  : "=r" (val) : : "cc");
	return val;
}

static inline void set_copro_access(unsigned int val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 2 @ set copro access"
	  : : "r" (val) : "cc");
	isb();
}

#endif

#endif
