#ifdef __KERNEL__
#define __DLOG_IMPLEMENTAION_MODULE__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/fb.h>
#include <linux/msm_mdp.h>
#include <linux/ktime.h>
#include <linux/wakelock.h>
#include <linux/time.h>
#include <asm/system.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <linux/lcd.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include "mdss_dsi.h"
#include "mdss_mdp.h"
#include "mdss_fb.h"
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

#include "mdss_panel.h"
#include "mdss_mdp.h"
#include "mdss_edp.h"
#include "mdss_debug.h"
#include <linux/input.h>
#include "linux/debugfs.h"

#define CFAKE_DEVICE_NAME "ddebugger"
#else
#include <ctype.h>
#include <debug.h>
#include <stdlib.h>
#include <printf.h>
#include <list.h>
#include <string.h>
#include <arch/ops.h>
#include <platform.h>
#include <platform/debug.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <reg.h>

#define pr_debug(fmt,...) dprintf(CRITICAL,fmt,##__VA_ARGS__);
#define pr_info(fmt,...) dprintf(CRITICAL,fmt,##__VA_ARGS__);
#define MIPI_INP(X) readl(X);
#endif


#include "mdss.h"


#ifndef __KERNEL__
u32 __debug_mdp_phys = 0x7FD00000;
#else
u32 __debug_mdp_phys = 0x00000;
#endif


static u32 dump_size;
struct debug_mdp *debug_mdp;
//MDP Instace Variable
#ifdef __KERNEL__
#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
extern struct sec_debug_log *secdbg_log;
extern struct sec_debug_subsys_data_krait *secdbg_krait;
extern struct _dlogdebug __start___dlog[];
extern struct _dlogdebug __stop___dlog[];
extern struct mdss_data_type *mdss_res;
#endif
static spinlock_t xlock;
extern struct msm_mdp_interface mdp5;
static int sec_debug_level = 1;
#endif
#define DLOG_BUFFER_SIZE 2000
#define DLOG_BUFFER_SIZE_SHIP 200
#define REGISTER_LOG_LEN 110
#define EVENT_DESC_LEN 10
#define CLOCK_DUMP_LEN 2

#if defined(CONFIG_ARCH_MSM8974)
#define CHIP_GPIO_COUNT_8974  146
static struct reg_desc mdss_reg_desc[]=
	{
	 	{0xFD900000,0x22100}, //mdp
		{0xFD922804,0x0600},  //dsi0
		{0xFD922E04,0x0600},  //dsi1
		{0xFD923404,0x0700},  //edp
		{0xFD511000,(CHIP_GPIO_COUNT_8974*4*4)} //gpio
		
	};
#elif defined(CONFIG_ARCH_MSM8226)
#define CHIP_GPIO_COUNT_8226 117
static struct reg_desc mdss_reg_desc[]=
	{
	 	{0xFD900000,0x22100}, //mdp
		{0xFD922804,0x0600},  //dsi0
		{0xFD511000,(CHIP_GPIO_COUNT_8226*4*4)} //gpio
		
	};
#endif
/* mdp_reg_info.txt contains the values to be initialized */
int read_ongoing;

volatile u32 mdp_reg_info[257] = {16, 260, 5243152, 7340336, 2097504, 10486144, 10486320, 3146464, 2097920, 2097936, 2097952, 8389536, 992, 1049588, 2098176, 1072, 10486864, 5244072, 3146992, 8390144, 1584, 8390400, 1840, 8390656, 2096, 8390912, 2352, 8391168, 2608, 1053696, 11539472, 5200, 5216, 5232, 3151044, 5344, 5360, 5376, 20976912, 10490368, 3150384, 4198984, 4720, 7344804, 2101968, 2101984, 1054720, 11540496, 6224, 6240, 6256, 3152068, 6368, 6384, 6400, 20977936, 10491392, 3151408, 4200008, 5744, 7345828, 2102992, 2103008, 1055744, 11541520, 7248, 7264, 7280, 3153092, 7392, 7408, 7424, 20978960, 10492416, 3152432, 4201032, 6768, 7346852, 2104016, 2104032, 8196, 1056784, 1056800, 10493440, 3153456, 4202056, 7792, 7347876, 2105040, 2105056, 9220, 1057808, 1057824, 10494464, 3154480, 4203080, 8816, 7348900, 2106064, 2106080, 10244, 1058832, 1058848, 10495488, 3155504, 4204104, 9840, 7349924, 2107088, 2107104, 10496512, 3156528, 4205128, 10864, 7350948, 2108112, 2108128, 10497536, 3157552, 4206152, 11888, 7351972, 2109136, 2109152, 2109952, 12816, 10498592, 10498640, 10498688, 27275952, 2110240, 2110256, 1061888, 1061904, 2110496, 2110976, 13840, 10499616, 10499664, 10499712, 27276976, 2111264, 2111280, 1062912, 1062928, 2111520, 2112000, 14864, 10500640, 10500688, 10500736, 27278000, 2112288, 2112304, 1063936, 1063952, 2112544, 2113024, 15888, 10501664, 10501712, 10501760, 27279024, 2113312, 2113328, 1064960, 1064976, 2113568, 2114048, 16912, 10502688, 10502736, 10502784, 27280048, 2114336, 2114352, 1065984, 1066000, 2114592, 17920, 18256, 3164004, 3164176, 6309932, 2115760, 2115776, 27281616, 18944, 19280, 3165028, 3165200, 6310956, 2116784, 2116800, 27282640, 19968, 20304, 3166052, 3166224, 6311980, 2117808, 2117824, 27283664, 3215616, 1118496, 3215664, 69960, 1118580, 16847712, 70576, 9515264, 3223856, 78152, 1126772, 16855904, 78768, 9523456, 3232048, 86344, 1134964, 16864096, 86960, 9531648, 3240240, 94536, 1143156, 16872288, 95152, 9539840, 3248432, 102728, 1151348, 16880480, 103344, 24252672, 135556, 135568, 1184172, 7475712, 24253184, 136068, 136080, 1184684, 7476224, 24253696, 136580, 136592, 1185196, 7476736, 24254208, 137092, 137104, 1185708, 7477248, 12720896, 12721152, 12721408};
u32 mdp_reg_vaddr[257];
/*TODO: Optimize/correct this function*/
int check_duplicate_event(u32 *event_buff, u32 address) {
	int i;

	for (i=0; i<debug_mdp->event_desc.len/sizeof(u32); i++) {
		if(event_buff[i] == address)
			return -1;
		i+=15; //as the event dump struct is 64 bytes
		}
	return 0;
}

void inc_put(u32 *buff,u32 l){

	buff[debug_mdp->log_buff.last] = l;
	debug_mdp->log_buff.last++;
	debug_mdp->log_buff.last %=  (debug_mdp->log_buff.len/sizeof(u32));
	if(debug_mdp->log_buff.last == debug_mdp->log_buff.first) {
						int count  = buff[debug_mdp->log_buff.first] &0x1F;
						debug_mdp->log_buff.first += count;  
						debug_mdp->log_buff.first %= (debug_mdp->log_buff.len/sizeof(u32));
	}
	pr_debug("[DDEBUGGER] buff:%pK, first:%d, last:%d, Val: %x\n",buff,debug_mdp->log_buff.first,debug_mdp->log_buff.last,(u32)l);
	return ;
}
#if defined(CONFIG_ARCH_MSM8974)
static struct dclock clock_list[] = {
	{"mdss_ahb_clk",HWIO_MMSS_MDSS_AHB_CBCR_ADDR,CLK_TEST_MDSS_AHB_CLK,0},
	{"mdss_axi_clk",HWIO_MMSS_MDSS_AXI_CBCR_ADDR,CLK_TEST_MDSS_AXI_CLK,0},
	{"mdss_byte0_clk",HWIO_MMSS_MDSS_BYTE0_CBCR_ADDR,CLK_TEST_MDSS_BYTE0_CLK,0},
	{"mdss_byte1_clk",HWIO_MMSS_MDSS_BYTE1_CBCR_ADDR,CLK_TEST_MDSS_BYTE1_CLK,0},
	{"mdss_edpaux_clk",HWIO_MMSS_MDSS_EDPAUX_CBCR_ADDR,CLK_TEST_MDSS_EDPAUX_CLK,0},
	{"mdss_edplink_clk",HWIO_MMSS_MDSS_EDPLINK_CBCR_ADDR,CLK_TEST_MDSS_EDPLINK_CLK,0},
	{"mdss_edppixel_clk",HWIO_MMSS_MDSS_EDPPIXEL_CBCR_ADDR,CLK_TEST_MDSS_EDPPIXEL_CLK,0},
	{"mdss_esc0_clk",HWIO_MMSS_MDSS_ESC0_CBCR_ADDR,CLK_TEST_MDSS_ESC0_CLK,0},
	{"mdss_esc1_clk",HWIO_MMSS_MDSS_ESC1_CBCR_ADDR,CLK_TEST_MDSS_ESC1_CLK,0},
	{"mdss_extpclk_clk",HWIO_MMSS_MDSS_EXTPCLK_CBCR_ADDR,CLK_TEST_MDSS_EXTPCLK_CLK,0},
	{"mdss_hdmi_ahb_clk",HWIO_MMSS_MDSS_HDMI_AHB_CBCR_ADDR,CLK_TEST_MDSS_HDMI_AHB_CLK,0},
	{"mdss_hdmi_clk",HWIO_MMSS_MDSS_HDMI_CBCR_ADDR,CLK_TEST_MDSS_HDMI_CLK,0},
	{"mdss_mdp_clk",HWIO_MMSS_MDSS_MDP_CBCR_ADDR,CLK_TEST_MDSS_MDP_CLK,0},
	{"mdss_mdp_lut_clk",HWIO_MMSS_MDSS_MDP_LUT_CBCR_ADDR,CLK_TEST_MDSS_MDP_LUT_CLK,0},
	{"mdss_pclk0_clk",HWIO_MMSS_MDSS_PCLK0_CBCR_ADDR,CLK_TEST_MDSS_PCLK0_CLK,0},
	{"mdss_pclk1_clk",HWIO_MMSS_MDSS_PCLK1_CBCR_ADDR,CLK_TEST_MDSS_PCLK1_CLK,0},
	{"mdss_vsync_clk",HWIO_MMSS_MDSS_VSYNC_CBCR_ADDR,CLK_TEST_MDSS_VSYNC_CLK,0},

};
#elif defined(CONFIG_ARCH_MSM8226)
static struct dclock clock_list[] = {
	{"mdss_ahb_clk",HWIO_MMSS_MDSS_AHB_CBCR_ADDR,CLK_TEST_MDSS_AHB_CLK,0},
	{"mdss_axi_clk",HWIO_MMSS_MDSS_AXI_CBCR_ADDR,CLK_TEST_MDSS_AXI_CLK,0},
	{"mdss_byte0_clk",HWIO_MMSS_MDSS_BYTE0_CBCR_ADDR,CLK_TEST_MDSS_BYTE0_CLK,0},	
	{"mdss_esc0_clk",HWIO_MMSS_MDSS_ESC0_CBCR_ADDR,CLK_TEST_MDSS_ESC0_CLK,0},	
	{"mdss_mdp_clk",HWIO_MMSS_MDSS_MDP_CBCR_ADDR,CLK_TEST_MDSS_MDP_CLK,0},
	{"mdss_mdp_lut_clk",HWIO_MMSS_MDSS_MDP_LUT_CBCR_ADDR,CLK_TEST_MDSS_MDP_LUT_CLK,0},
	{"mdss_pclk0_clk",HWIO_MMSS_MDSS_PCLK0_CBCR_ADDR,CLK_TEST_MDSS_PCLK0_CLK,0},
	{"mdss_vsync_clk",HWIO_MMSS_MDSS_VSYNC_CBCR_ADDR,CLK_TEST_MDSS_VSYNC_CLK,0},
};

#endif
#define writelx(r,v) writel((v),(r))

	void *vHWIO_GCC_DEBUG_CLK_CTL_ADDR = (void*)0xfc401880;
    void *vHWIO_MMSS_DEBUG_CLK_CTL_ADDR = (void*)0xfd8c0900;
    void *vHWIO_GCC_CLOCK_FRQ_MEASURE_STATUS_ADDR = (void*)0xfc401888;
    void *vHWIO_GCC_CLOCK_FRQ_MEASURE_CTL_ADDR =(void*) 0xfc401884;
    void *vHWIO_GCC_XO_DIV4_CBCR_ADDR = (void*)0xfc4010c8;

static long read_clock(u32 clk_test,u32 clk_reg){
	long clock_val;

	pr_debug("%s: test:0x%x reg:%x\n",__func__,clk_test,clk_reg);


	//Print_Clk_Info_Line
	{
		u32 is_on=0;
		u32 clk_freq=0;
		//u32 clk_freq_val=0;
		
		if(clk_reg != 0){
			if((readl_relaxed(clk_reg) & 0x80000000) == 0x0) 
				is_on = 1;
		}
		pr_debug("%s:is_on:%d\n",__func__,is_on);

		if(!is_on) return 0;
		//Program Clock Test
		{
	
			u32 testval = clk_test & CLK_TEST_TYPE_MASK;
			u32 setval = clk_test & CLK_TEST_SEL_MASK;
			//u32 submuxval = clk_test & CLK_TEST_SUB_MUX_MASK;


			if(setval == CLK_MMSS_TEST) {
				writelx(vHWIO_GCC_DEBUG_CLK_CTL_ADDR,0x00013000|(0x2c&0x000001FF));
				writelx(vHWIO_MMSS_DEBUG_CLK_CTL_ADDR,0x00010000|(testval&0x00000FFF));
			}
		}
		pr_debug("%s:2\n",__func__);

		//Calc_Clk_Freq
		{
			//Configure l2cpuclkselr...for accuaracy

			u32 xo_div4_cbcr = readl_relaxed(vHWIO_GCC_XO_DIV4_CBCR_ADDR);
			u32 multiplier = 4;
			u32 tcxo_count = 0x800;
			u32 measure_ctl = 0;
			u32 short_clock_count;
			u32 clock_count = 0;
			u32 dbg_clk_ctl = 0;
			u32 temp = 0;
			//Measure a short run

			//Config XO DIV4 comparator clock

			writelx(vHWIO_GCC_XO_DIV4_CBCR_ADDR,readl_relaxed(vHWIO_GCC_XO_DIV4_CBCR_ADDR)|0x1);
			// Start with the counter disabled   
			measure_ctl=readl_relaxed(vHWIO_GCC_CLOCK_FRQ_MEASURE_CTL_ADDR) ; 
			measure_ctl=measure_ctl&~0x1FFFFF ; 
			writelx(vHWIO_GCC_CLOCK_FRQ_MEASURE_CTL_ADDR,measure_ctl);
			// Program the starting counter value, high enough to get good accuracy
			pr_debug("%s:measure_ctl:0x%x xo_div4_cbcr:0x%x \n",__func__,measure_ctl
					,xo_div4_cbcr);


			measure_ctl= measure_ctl| tcxo_count;
			//Start the counting  
			measure_ctl=measure_ctl|0x100000;
			writelx(vHWIO_GCC_CLOCK_FRQ_MEASURE_CTL_ADDR,measure_ctl);
			pr_debug("HWIO_GCC_CLOCK_FRQ_MEASURE_CTL_ADDR:0x%X",
				readl_relaxed(vHWIO_GCC_CLOCK_FRQ_MEASURE_CTL_ADDR) ); 
			
			//Wait for the counters to finish
			mdelay(1);
			while ((temp = readl_relaxed(vHWIO_GCC_CLOCK_FRQ_MEASURE_STATUS_ADDR)&0x2000000)==0) 
				 pr_debug("HWIO_GCC_CLOCK_FRQ_MEASURE_STATUS_ADDR:0x%X\n",temp);
			
			pr_debug("%s:4\n",__func__);		
			// Turn off the test clock and read the clock count
			measure_ctl = readl_relaxed(vHWIO_GCC_CLOCK_FRQ_MEASURE_CTL_ADDR);
			writelx(vHWIO_GCC_CLOCK_FRQ_MEASURE_CTL_ADDR, measure_ctl&~0x100000);
			pr_debug("%s:5\n",__func__); 	

			short_clock_count=readl_relaxed(vHWIO_GCC_CLOCK_FRQ_MEASURE_STATUS_ADDR)&0x1FFFFFF;

			//Restore the register
			writelx(vHWIO_GCC_XO_DIV4_CBCR_ADDR,xo_div4_cbcr);

			pr_debug("%s:6:short_clock_count: %d\n",__func__,short_clock_count); 	

			//Longer count and compare
			xo_div4_cbcr = readl_relaxed(vHWIO_GCC_XO_DIV4_CBCR_ADDR);
			multiplier = 4;
			tcxo_count = 0x8000;

			
			//Config XO DIV4 comparator clock
			
			writelx(vHWIO_GCC_XO_DIV4_CBCR_ADDR,readl_relaxed(vHWIO_GCC_XO_DIV4_CBCR_ADDR)|0x1);
				pr_debug("%s:7\n",__func__);		
			// Start with the counter disabled	 
			measure_ctl=readl_relaxed(vHWIO_GCC_CLOCK_FRQ_MEASURE_CTL_ADDR) ; 
			measure_ctl=measure_ctl&~0x1FFFFF ; 
			writelx(vHWIO_GCC_CLOCK_FRQ_MEASURE_CTL_ADDR,measure_ctl);
			// Program the starting counter value, high enough to get good accuracy
			pr_debug("%s:8\n",__func__); 	

			measure_ctl= measure_ctl| tcxo_count;
			//Start the counting  
			measure_ctl=measure_ctl|0x100000;
			writelx(vHWIO_GCC_CLOCK_FRQ_MEASURE_CTL_ADDR,measure_ctl);
				pr_debug("%s:9\n",__func__);		
			//Wait for the counters to finish
			mdelay(1);
			while ((readl_relaxed(vHWIO_GCC_CLOCK_FRQ_MEASURE_STATUS_ADDR)&0x2000000)==0) ;
			
			
			
			// Turn off the test clock and read the clock count
			measure_ctl = readl_relaxed(vHWIO_GCC_CLOCK_FRQ_MEASURE_CTL_ADDR);
			writelx(vHWIO_GCC_CLOCK_FRQ_MEASURE_CTL_ADDR, measure_ctl&~0x100000);
			pr_debug("%s:10\n",__func__); 	

			clock_count=readl_relaxed(vHWIO_GCC_CLOCK_FRQ_MEASURE_STATUS_ADDR)&0x1FFFFFF;

			if( clock_count == short_clock_count)
				clk_freq = 0;
			else
				clk_freq = (48*(2*multiplier)/10)*(2*clock_count+3)*2/(2*tcxo_count+7); /* need this for furthur implementation*/
			
			//Restore the register
			writelx(vHWIO_GCC_XO_DIV4_CBCR_ADDR,xo_div4_cbcr);
			pr_debug("%s:11\n",__func__);		
			
			//Clear the divide by 4 in DEBUG_CLK_CTL to make the scope view of the clock
			//the correct frequency
			 dbg_clk_ctl=readl_relaxed(vHWIO_GCC_DEBUG_CLK_CTL_ADDR);
			 dbg_clk_ctl=dbg_clk_ctl&~0x00003000;
			 writelx(vHWIO_GCC_DEBUG_CLK_CTL_ADDR, dbg_clk_ctl);
			 pr_debug("%s:12\n",__func__);	 

			//store freq
			clock_val = clock_count;
		}
		
	}

	return clock_val;
}

static int init_clock_va(void){
	int i = 0;
	u32 clock_base_phy = 0xfd8c2300;
	u32 clock_base_virt  = (u32)ioremap(0xfd8c2300,0xFF);
	if(!clock_base_virt)
			pr_err("Error Mapping Clock adress for %s",clock_list[i].name);

	for(;i < sizeof(clock_list)/sizeof(struct dclock);i++){
		pr_debug("Mapping: clk: %s addr: %x\n",clock_list[i].name,clock_list[i].reg_addr);
#ifdef __KERNEL__
		if((clock_list[i].reg_addr - clock_base_phy) < 0) 
			pr_err("Check clock base @@@@@@@@@@@@@!!!!!!!!!!!!!!!!\n");
		else
			clock_list[i].vreg_addr = clock_base_virt + (clock_list[i].reg_addr - clock_base_phy);

#else
		clock_list[i].vreg_addr = clock_list[i].reg_addr;
#endif
	}
	for(i = 1; i < sizeof(mdss_reg_desc)/sizeof(struct reg_desc) ; i++){
		mdss_reg_desc[i].vaddr = (u32)ioremap(mdss_reg_desc[i].base,mdss_reg_desc[i].len*4);
	}

	return 0;
}
void dump_clock_state(void)
{
	static u32 *buff = NULL;
	int i = 0;

	if(debug_mdp && debug_mdp->clock_state.len == 0)
		return;
	
	 if(debug_mdp && buff == NULL) {
                buff = (u32 *)((char *)debug_mdp + (sizeof(struct debug_mdp) + debug_mdp->clock_state.offset));
     } else if(!debug_mdp){
                        pr_debug("Debug module not Initialized\n");
                        return ;
     }

	pr_debug("debug_mdp : %pK buff: %pK end: %pK",debug_mdp,buff,buff+ debug_mdp->clock_state.len);
	for(;i < sizeof(clock_list)/sizeof(struct dclock);i++){
			u32 clock_val ;
			char  *clk_ptr ;
			pr_debug("reading: %s i = %d, last: %d\n",clock_list[i].name,i,debug_mdp->clock_state.last);
			clk_ptr = (char *) &buff[debug_mdp->clock_state.last];
			memcpy(clk_ptr,clock_list[i].name,sizeof(clock_list[i].name));
			clock_val = read_clock(clock_list[i].test_reg, clock_list[i].vreg_addr);
			
			
			
			debug_mdp->clock_state.last += sizeof(clock_list[i].name)/sizeof(u32);
			pr_debug(" %s : %u :last : %d\n",clk_ptr,clock_val,debug_mdp->clock_state.last);
			buff[debug_mdp->clock_state.last++] = clock_val;
			pr_debug("buff[debug_mdp->clock_state.last++]: %pK",&buff[debug_mdp->clock_state.last-1]);
	}	
	
	
}

int fill_reg_log(u32 *buff, u32 base, int len)
{	
	int i;
	unsigned char *buf;
#ifdef __KERNEL__
	buf = (void*)base; 
#else
	buf = base; 
#endif
	for(i = 0; i < len/4; i++){
		pr_debug("buff:%pK ",&buff[debug_mdp->reg_log.last]);
		buff[debug_mdp->reg_log.last] = MIPI_INP(buf+i*4);
		debug_mdp->reg_log.last++;
	}
	
	return 0;
}
	

	
/*	void klog(void) is used to dump register values of dsi0, dsi1, edp and
	mdp registers respectively. Each section is identified by START_MAGIC 
	followed by start address of registers. For mdp case the detailed
	information of register adresses is found in mdp_reg_addrs_and_len.txt
*/

void klog(void)
{
	int i;
	static u32 *buff = NULL;
	int mdp_reg_count = 0;
	struct mdss_panel_data *pdata = NULL;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_data_type *mdata = mdss_mdp_get_mdata();
	int mdp_reg_dump_en;
	unsigned long flags;
	
	/* NULL Checks */
	if(mdata == NULL) return;
	if(mdata->ctl_off == NULL) return;
	pdata = (mdata->ctl_off+0)->panel_data;
	if(pdata ==NULL) return;
	
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	mdp_reg_dump_en = (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT);
	if(debug_mdp->reg_log.last*4 >= debug_mdp->reg_log.len  ) return;
	if(debug_mdp->reg_log.len == 0) return;

	
	pr_debug("KK: -----------> Inside %s",__func__);
	
#ifdef __KERNEL__
	mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_ON, false);
	spin_lock_irqsave(&xlock, flags);
	
#else
	if((readl_relaxed(HWIO_MMSS_MDSS_AHB_CBCR_ADDR) & 0x80000000) != 0x0) {
		pr_info("AHB Clock not ON, Cannot Read MDP Regs\n");
	}
	//Switch on clcok
#endif
	if(debug_mdp && buff == NULL){
		buff = (u32 *)((char *)debug_mdp + (sizeof(struct debug_mdp) + debug_mdp->reg_log.offset));
	}
	else if(!debug_mdp){
		pr_info("Debug module not Initialized\n");
		return ;
	}
	
	pr_debug("KK:------------------->(%s)::>> first: %x \t last: %x buff:%pK-%pK\n", __func__, debug_mdp->reg_log.first, debug_mdp->reg_log.last,buff,buff+debug_mdp->reg_log.len);

if(!mdp_reg_dump_en)
	i = sizeof(mdss_reg_desc)/sizeof(struct reg_desc) -1;
else
	i = 1;
	for(; i < sizeof(mdss_reg_desc)/sizeof(struct reg_desc) ; i++){
		buff[debug_mdp->reg_log.last++] = START_MAGIC;
        	buff[debug_mdp->reg_log.last++] = mdss_reg_desc[i].base;
#if defined(__KERNEL__)
		if(fill_reg_log(buff, mdss_reg_desc[i].vaddr, mdss_reg_desc[i].len))
			pr_info("failed to dump lcd regs at %x ----------> KK\n",mdss_reg_desc[i].base);
#else
		if(fill_reg_log(buff, base, len*4))
			pr_info("failed to dump lcd regs at %x ----------> KK\n",base);
#endif	
	}
#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8226)	
if(mdp_reg_dump_en){
	
	buff[debug_mdp->reg_log.last++] = START_MAGIC;
        buff[debug_mdp->reg_log.last++] = mdss_reg_desc[0].base;

	for(i = 0; i < sizeof(mdp_reg_info)/sizeof(u32); i++){
		int len;
		u32 base;
		len = mdp_reg_info[i] & 0xfff00000;
		len = len >> 20;
		len += 1;
		mdp_reg_count += len;
		base = mdp_reg_info[i] & 0x000fffff;

#if defined(__KERNEL__)
		if(fill_reg_log(buff, (u32) mdss_res->mdp_base +base, len*4))
			pr_info("failed to dump lcd regs at %x ----------> KK\n",base);

#else
			base = base | mdss_reg_desc[0].base;
			if(fill_reg_log(buff, base, len*4))
			pr_info("failed to dump lcd regs at %x ----------> KK\n",base);
#endif
	}
}
#endif
#ifdef __KERNEL__
	
	spin_unlock_irqrestore(&xlock, flags);
	mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_OFF, false);

#else
        //Switch off clock 
#endif
	pr_debug("total mdp regs: %d\n",mdp_reg_count);
}

void dlog(struct _dlogdebug *desc, ...)
{	
	
	
	va_list argp;

	static u32 *buff = NULL;
	unsigned long flags;
	int ev_idx = desc - __start___dlog;
	int para_count = 0;
	int temp_main_idx = 0 ;
#ifdef __KERNEL__
	ktime_t time;
	if(!debug_mdp) return;

	spin_lock_irqsave(&xlock, flags);

	if(read_ongoing) {
		spin_unlock_irqrestore(&xlock, flags);
		return;
	}
#else
	time_t time;
#endif
	if(debug_mdp && buff == NULL ) {
		buff=  (u32 *)((char *)debug_mdp + (sizeof(struct debug_mdp) + debug_mdp->log_buff.offset));
	}
	else if(!debug_mdp){
#ifdef __KERNEL__
		spin_unlock_irqrestore(&xlock, flags);
#endif
		pr_info("Debug Module Not Initialized\n");	
		return;
	}
	//Store the reference of 32bit header
	temp_main_idx = debug_mdp->log_buff.last;
	inc_put(buff,(current->pid<<16)|(ev_idx<<5));
#ifdef __KERNEL__	
	time = ktime_get();
	inc_put(buff,(u32)ktime_to_us(time));
#else
	time = current_time();
	inc_put(buff,(u32)time);

#endif

	
	va_start(argp,(u32) desc);
	do{
		u32 l = va_arg(argp, u32);
	//	pr_info("[DLOG],%pS: %x\n",__builtin_return_address(0),l);
		if(l==0xBABEBABE) 
			break;
		inc_put(buff,l);
		para_count++;
	}while(1);
	va_end(argp);

	 para_count += 2;// 1 for main indx, 1 for time

	 //Put the length
	 buff[temp_main_idx] = buff[temp_main_idx] | (para_count & 0x1F); //5 bits for length
#ifdef __KERNEL__ 
	spin_unlock_irqrestore(&xlock, flags);
#endif
}
 
#ifdef __KERNEL__ 


 
 static unsigned long read_byte;
 /* 
  * Called when a process tries to open the device file, like
  * "cat /dev/mycharfile"
  */
 static int device_open(struct inode *inode, struct file *file)
 {
	read_byte = 0;
	printk(" Dlogger Opened>>>\n");
	 return 0;
 }

static enum  {	DLOG_BUFFER_READING,	KLOG_BUFFER_READING,	SECLOG_BUFFER_READING, }read_state;


 int dlog_read(struct file *filp, char __user *buf, size_t count, 

	loff_t *f_pos)

{
	ssize_t retval = 0;
	unsigned long flags;
	int ret = 0;
	
	if(*f_pos == 0) {

		pr_info("===DLogger Header=====\n");
		pr_info(" DUMP SIZE: %u, read: %lu\n",debug_mdp->size,read_byte);
		pr_info("[0] first: %d last: %d  off: %d size: %d\n",
					debug_mdp->log_buff.first, debug_mdp->log_buff.last,
					debug_mdp->log_buff.offset, debug_mdp->log_buff.len);
		
		pr_info("[1] first: %d last: %d  off: %d size: %d\n",
					debug_mdp->event_desc.first, debug_mdp->event_desc.last,
					debug_mdp->event_desc.offset, debug_mdp->event_desc.len);
		pr_info("[2] first: %d last: %d  off: %d size: %d\n",
					debug_mdp->reg_log.first, debug_mdp->reg_log.last,
					debug_mdp->reg_log.offset, debug_mdp->reg_log.len);
		pr_info("[3] first: %d last: %d  off: %d size: %d\n",
					debug_mdp->clock_state.first, debug_mdp->clock_state.last,
					debug_mdp->clock_state.offset, debug_mdp->clock_state.len);
#ifdef __KERNEL__

		spin_lock_irqsave(&xlock, flags);
		read_ongoing = 1;	
		debug_mdp->reserv = CONFIG_NR_CPUS;
#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
		debug_mdp->klog_size =secdbg_krait->log.size;
		debug_mdp->seclog_size = 0;
		pr_debug("Klog Size: %d SecLog Size: %d\n", debug_mdp->klog_size, debug_mdp->seclog_size);
		
#endif		
		spin_unlock_irqrestore(&xlock, flags);
		
#endif

	}

		if(read_state == DLOG_BUFFER_READING && read_byte >= debug_mdp->size ) {
#ifdef __KERNEL__
					spin_lock_irqsave(&xlock, flags);
					read_ongoing = 0;
					debug_mdp->reg_log.first = 0;
					debug_mdp->reg_log.last = 0;
					debug_mdp->clock_state.first = 0;
					debug_mdp->clock_state.last = 0;
					read_state = KLOG_BUFFER_READING;
				
					spin_unlock_irqrestore(&xlock, flags);
#endif
		
#ifndef CONFIG_SEC_DEBUG_SCHED_LOG
					read_state = DLOG_BUFFER_READING;
					read_byte = 0;
					pr_info("Reading complete...\n");
					return 0;
#endif
	}

	if(read_state == DLOG_BUFFER_READING)
	{
		pr_debug("(count + *f_pos - 1):=%llu\n",(count + *f_pos - 1));
		retval = ((count + *f_pos - 1)< debug_mdp->size)? count-1:(debug_mdp->size - *f_pos);
		
		ret = copy_to_user(buf, (char *)debug_mdp + *f_pos, retval);
		if(ret < 0)
			return 0;
		read_byte += retval;
		*f_pos = read_byte;
		pr_debug("-read: %lu, fpos = %llu :count = %d:retval: %d: dump_size: %d\n",read_byte,*f_pos,count,retval,debug_mdp->size);
	}

#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
	
	if(read_state == KLOG_BUFFER_READING && read_byte >= debug_mdp->size + debug_mdp->klog_size ) {

		read_state = DLOG_BUFFER_READING;
					read_byte = 0;
					pr_info("Reading complete...\n");
	}

	

	if(read_state == KLOG_BUFFER_READING)
	{
		int start = *f_pos - debug_mdp->size;
		
		retval = ((count + *f_pos - 1)< debug_mdp->size +debug_mdp->klog_size)? count-1:(debug_mdp->size +debug_mdp->klog_size- *f_pos);
		ret = copy_to_user(buf, (char *)__va(secdbg_krait->log.log_paddr) + start, retval);	
		if(ret < 0)
					return 0;

		read_byte += retval;
		*f_pos = read_byte;
	}

#endif

	return retval;

}
  static int reg_open(struct inode *inode, struct file *file)
  {
	  pr_info("Register dump opened\n");
	  return 0;
  }
  int reg_read(struct file *filp, char __user *buf, size_t count, 
 
	 loff_t *f_pos)
 
 {
		dump_clock_state();
		klog();
		return 0;
 }

 static const struct file_operations reg_fops = {
	 .open = reg_open,
	 .release = NULL,
	 .read = reg_read,
	 .write = NULL,
 };

 static const struct file_operations dlog_fops = {
	 .open = device_open,
	 .release = NULL,
	 .read = dlog_read,
	 .write = NULL,
 };

 static int __init setup_debug_memory(char *mode)
 {
	 __debug_mdp_phys = 0;
	 if(!sscanf(mode,"%x", &(__debug_mdp_phys)))
	 	pr_err("Error parsing display logging mem base:%s\n",mode);
	 else
	 	pr_info("Display Logging base: %x\n",__debug_mdp_phys);
	 return 1;
 }
 
 /* Get the size of description section needed */
 static int get_desc_size(void){
			int len = __stop___dlog - __start___dlog;
			int i = 0;
			int str_len = 0;

			for(; i < len; i++) {
				struct _dlogdebug *ptr = __start___dlog + i;
				str_len += strlen(ptr->filename) + 1;
				str_len += strlen(ptr->format) + 1;
				str_len += strlen(ptr->function) + 1;
				str_len += 4; //flags + lineno

			}
		return str_len;
 }

 /* Initialize event descriptor section */
 static void init_event_desc(char *buff,int length){
		int len = __stop___dlog - __start___dlog;
		int i = 0;
		int str_len = 0;

		memset(buff,0x0,length);
		for(; (i < len && str_len < length); i++) {
			struct _dlogdebug *ptr = __start___dlog + i;
			int *line_ptr = 0;
			str_len += (snprintf(buff+str_len,length-str_len,"%s", ptr->filename) + 1);
			str_len += (snprintf(buff+str_len,length-str_len,"%s", ptr->format) + 1);
			str_len += (snprintf(buff+str_len,length-str_len,"%s", ptr->function) + 1);
			line_ptr  = (int *) (buff + str_len);
			*line_ptr = ptr->lineno | ptr->flags<<24;
			str_len += 4; //flags + lineno
			if(str_len >= length) break;
		}

 }

 int dlog_sec_get_debug_level(void) {
		return sec_debug_level;
 }
 __setup("lcd_dlog_base=", setup_debug_memory);
#endif
#ifdef __KERNEL__
 static int __init mdss_debug_init(void) {
#else
 int  mdss_debug_init(void) {
#endif
		
		u32 log_buff_len = DLOG_BUFFER_SIZE_SHIP*1024 - sizeof(struct debug_mdp);
		u32 event_desc_len = 0;
		u32 reg_log_len = 0;
		u32 clock_state_len = 0;
		struct dentry *dent = debugfs_create_dir("dlog", NULL);

#if defined(CONFIG_SEC_DEBUG)
		sec_debug_level = sec_debug_is_enabled();
#endif
		
		if(sec_debug_level){
			log_buff_len = DLOG_BUFFER_SIZE*1024;
			event_desc_len = get_desc_size();
			reg_log_len = REGISTER_LOG_LEN*1024;
			clock_state_len = CLOCK_DUMP_LEN*1024;
		}
		
		dump_size = log_buff_len + event_desc_len+ reg_log_len \
			 +clock_state_len+ sizeof(struct debug_mdp);
#ifdef __KERNEL__
		if(__debug_mdp_phys){
			debug_mdp = ioremap_nocache(__debug_mdp_phys,CARVEOUT_MEM_SIZE);
			pr_info("Using MDSS debug memory from LK:Phys: %x,  VA: %pK\n",__debug_mdp_phys,debug_mdp);
			
		}
			
		if(!__debug_mdp_phys || !debug_mdp) {
			debug_mdp = kzalloc (dump_size, GFP_KERNEL);
			if(!debug_mdp) {
				pr_err("Memory allocation failed for MDP DEBUG MODULE\n");
				return -1;
			}
		} 
		//debug_mdp->log_buff.offset = 0;
		
		pr_info(KERN_INFO "MDP debug init:debug_mdp: %pK \n",debug_mdp);
#else
		debug_mdp = __debug_mdp_phys;

		memset(debug_mdp,0x0,CARVEOUT_MEM_SIZE);

#endif
		if(!__debug_mdp_phys || (__debug_mdp_phys == (u32)debug_mdp)){
			/* Initialize buffer header */
			debug_mdp->log_buff.len = log_buff_len; 

			debug_mdp->event_desc.offset = debug_mdp->log_buff.offset + debug_mdp->log_buff.len;
			debug_mdp->event_desc.len = event_desc_len;   
			
			debug_mdp->reg_log.offset = debug_mdp->event_desc.offset + debug_mdp->event_desc.len;
			debug_mdp->reg_log.len = reg_log_len;

			debug_mdp->clock_state.offset = debug_mdp->reg_log.offset + debug_mdp->reg_log.len;
			debug_mdp->clock_state.len = clock_state_len;
			debug_mdp->size = dump_size;
			pr_info("size:%d",sizeof(debug_mdp->size));
				
			strncpy(debug_mdp->marker,"*#$$_START_OF_MDP_DEBUG_DUMP##$", sizeof("*#$$_START_OF_MDP_DEBUG_DUMP##$"));
			if(debug_mdp !=NULL)
				init_event_desc((char *)debug_mdp + (sizeof(struct debug_mdp) + debug_mdp->event_desc.offset), debug_mdp->event_desc.len);		

		}else {
			pr_info("===DLogger Header=====\n");
			pr_info(" DUMP SIZE: %u\n",debug_mdp->size);
			pr_info("[0] first: %d last: %d  off: %d size: %d\n",
						debug_mdp->log_buff.first, debug_mdp->log_buff.last,
						debug_mdp->log_buff.offset, debug_mdp->log_buff.len);
			
			pr_info("[1] first: %d last: %d  off: %d size: %d\n",
						debug_mdp->event_desc.first, debug_mdp->event_desc.last,
						debug_mdp->event_desc.offset, debug_mdp->event_desc.len);
			pr_info("[2] first: %d last: %d  off: %d size: %d\n",
						debug_mdp->reg_log.first, debug_mdp->reg_log.last,
						debug_mdp->reg_log.offset, debug_mdp->reg_log.len);
			pr_info("[2] first: %d last: %d  off: %d size: %d\n",
						debug_mdp->clock_state.first, debug_mdp->clock_state.last,
						debug_mdp->clock_state.offset, debug_mdp->clock_state.len);
		}
		
#ifdef __KERNEL__	
		spin_lock_init(&xlock);

{
	if(sec_debug_level) {
		init_clock_va();	
		vHWIO_GCC_DEBUG_CLK_CTL_ADDR = ioremap((0xfc401880),4);
		vHWIO_MMSS_DEBUG_CLK_CTL_ADDR = ioremap(0xfd8c0900,4);
		vHWIO_GCC_CLOCK_FRQ_MEASURE_STATUS_ADDR = ioremap(0xfc401888,4);
		vHWIO_GCC_CLOCK_FRQ_MEASURE_CTL_ADDR = ioremap(0xfc401884,4);
		vHWIO_GCC_XO_DIV4_CBCR_ADDR = ioremap(0xfc4010c8,4);
		if (debugfs_create_file("reg_dump", 0644, dent, 0, &reg_fops)
								== NULL) {
							printk(KERN_ERR "%s(%d): debugfs_create_file: debug fail\n",
								__FILE__, __LINE__);
							return -1;
		}
		pr_info("Init Section: %pK",__start___dlog);
	}
	if (debugfs_create_file("dlogger", 0644, dent, 0, &dlog_fops)
		== NULL) {
	printk(KERN_ERR "%s(%d): debugfs_create_file: debug fail\n",
		__FILE__, __LINE__);
	return -1;
	}
}
#endif
return 0;
 }
 
#ifdef __KERNEL__
arch_initcall(mdss_debug_init);
#endif
