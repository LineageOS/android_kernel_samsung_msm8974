#ifndef SAMSUNG_DUAL_DSI_PANEL
#define SAMSUNG_DUAL_DSI_PANEL
#define MAX_PANEL_NAME_SIZE 100

#define RECOVERY_BRIGHTNESS 180

struct display_status {
	unsigned char acl_on;
	unsigned char curr_acl_cond;
	unsigned char is_mdnie_loaded;
	unsigned char auto_brightness;
	unsigned char recovery_boot_mode;
	unsigned char on;
	unsigned char wait_disp_on;
	int curr_acl_idx;
	int curr_gamma_idx;
	int bright_level;
	int recent_bright_level;

	int temperature;
	char temperature_value;
	int temper_need_update;
	int siop_status;
};

struct mipi_samsung_driver_data {
	struct display_status dstat;
	struct msm_db_data_type *mfd;
	struct mdss_panel_data *pdata;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata;
	struct mutex lock;

	char panel_name[MAX_PANEL_NAME_SIZE];
	int panel;
};
#endif
