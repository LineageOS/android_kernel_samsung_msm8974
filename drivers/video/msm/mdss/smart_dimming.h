#ifndef _SMART_DIMMING_H_
#define _SMART_DIMMING_H_

/* octa ldi */
#define EVT0_K_fhd_REVB 0x00
#define EVT0_K_fhd_REVF 0x01

#define EVT0_K_fhd_REVG 0x02
#define EVT1_K_fhd_REVH 0x12
#define EVT1_K_fhd_REVI 0x13

#define EVT0_K_wqhd_REVB 0x00
#define EVT0_K_wqhd_REVC 0x01
#define EVT0_K_wqhd_REVD 0x02
#define EVT0_K_wqhd_REVE 0x03
#define EVT0_K_wqhd_REVF 0x04

#define EVT2_K_fhd_magna_REVA  0x50
#define EVT2_K_fhd_magna_REVB  0x61
#define EVT2_K_fhd_magna_REVC  0x72

#define EVT0_S_hd_REVA 0x00
#define EVT0_S_hd_REVC 0x01
#define EVT1_S_hd_REVD 0x02
#define EVT1_S_hd_REVF 0x03
#define EVT1_S_hd_REVG 0x04

struct smartdim_conf{
	void (*generate_gamma)(int cd, char *str);
	void (*get_min_lux_table)(char *str, int size);
	void (*init)(void);
	void (*print_aid_log)(void);
	char *mtp_buffer;
	int *lux_tab;
	int lux_tabsize;
	unsigned int man_id;
};

/* Define the smart dimming LDIs*/
struct smartdim_conf *smart_S6E3_get_conf(void);
struct smartdim_conf *smart_S6E8FA0_get_conf(void);
struct smartdim_conf *smart_S6E3FA0_get_conf(void);
struct smartdim_conf *smart_S6TNMR7_get_conf(void);
struct smartdim_conf *smart_S6E3HA1_get_conf(void);
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
struct smartdim_conf *smart_S6E88A0_get_conf(void);
#endif
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
	|| defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
struct smartdim_conf *smart_S6E8AA4_get_conf(void);
#endif

#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_WXGA_PT_DUAL_PANEL)
struct smartdim_conf *smart_S6E8FA0_get_conf_main(void);
struct smartdim_conf *smart_S6E8FA0_get_conf_sub(void);
#endif

#if defined(CONFIG_LCD_HMT)
struct smartdim_conf_hmt {
	void (*generate_gamma)(int cd, char *str, int dual);
	void (*get_min_lux_table)(char *str, int size);
	void (*init)(int dual);
	void (*print_aid_log)(void);
	char *mtp_buffer;
	int *lux_tab;
	int lux_tabsize;
	unsigned int man_id;
};

struct smartdim_conf_hmt *smart_S6E3_get_conf_hmt(int dual);
#endif


#endif /* _SMART_DIMMING_H_ */
