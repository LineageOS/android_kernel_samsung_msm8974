mdss-mdp3-objs = mdp3.o mdp3_dma.o mdp3_ctrl.o dsi_status_v2.o
ifeq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DDLOG_USER_VARIANT
endif
obj-$(CONFIG_SAMSUNG_DEBUG_DISPLAY) += dlog.o
mdss-mdp3-objs = mdp3.o mdp3_dma.o mdp3_ctrl.o
mdss-mdp3-objs += mdp3_ppp.o mdp3_ppp_hwio.o mdp3_ppp_data.o
obj-$(CONFIG_FB_MSM_MDSS_MDP3) += mdss-mdp3.o

mdss-mdp-objs := mdss_mdp.o mdss_mdp_ctl.o mdss_mdp_pipe.o mdss_mdp_util.o dsi_status_6g.o
mdss-mdp-objs += mdss_mdp_pp.o
mdss-mdp-objs += mdss_mdp_intf_video.o
mdss-mdp-objs += mdss_mdp_intf_cmd.o
mdss-mdp-objs += mdss_mdp_intf_writeback.o
mdss-mdp-objs += mdss_mdp_rotator.o
mdss-mdp-objs += mdss_mdp_overlay.o
mdss-mdp-objs += mdss_mdp_splash_logo.o
mdss-mdp-objs += mdss_mdp_wb.o
obj-$(CONFIG_FB_MSM_MDSS) += mdss-mdp.o

ifeq ($(CONFIG_FB_MSM_MDSS),y)
obj-$(CONFIG_DEBUG_FS) += mdss_debug.o mdss_debug_xlog.o
endif

ifeq ($(CONFIG_FB_MSM_MDSS_DSI_DBG),y)
ifeq ($(CONFIG_DEBUG_FS),y)
obj-$(CONFIG_DEBUG_FS) += xlog.o
endif
endif

ifeq ($(CONFIG_FB_MSM_MDSS_MDP3),y)
dsi-v2-objs = dsi_v2.o dsi_host_v2.o dsi_io_v2.o
obj-$(CONFIG_FB_MSM_MDSS_MDP3) += dsi-v2.o
endif

mdss-dsi-objs := mdss_dsi.o mdss_dsi_host.o mdss_dsi_cmd.o mdss_dsi_status.o

ifeq ($(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL),y)
mdss-dsi-objs += mdss_samsung_oled_cmd_hd_wqhd_panel.o smart_mtp_s6e3.o
else ifeq ($(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_CMD_HD_PT_PANEL),y)
mdss-dsi-objs += mdss_samsung_oled_cmd_hd_wqhd_panel.o smart_mtp_ea8064.o
else ifeq ($(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_FULL_HD_PT_PANEL),y)
mdss-dsi-objs += mdss_samsung_dsi_panel.o smart_mtp_se6e8fa.o smart_mtp_se6e3fa.o
else ifeq ($(CONFIG_FB_MSM_MIPI_SAMSUNG_YOUM_CMD_FULL_HD_PT_PANEL),y)
mdss-dsi-objs += mdss_samsung_dsi_panel.o smart_mtp_se6e8fa.o smart_mtp_se6e3fa.o
else ifeq ($(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL),y)
mdss-dsi-objs += logo.o mdss_samsung_dsi_panel.o smart_mtp_se6e8fa.o smart_mtp_se6e3fa.o
else ifeq ($(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL),y)
mdss-dsi-objs += logo.o mdss_samsung_dsi_panel.o smart_mtp_ea8061.o smart_mtp_s6e8aa4.o
else ifeq ($(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL),y)
mdss-dsi-objs += logo.o mdss_samsung_dsi_panel.o smart_mtp_s6e8aa4.o
else ifeq ($(CONFIG_FB_MSM_MDSS_SHARP_HD_PANEL),y)
mdss-dsi-objs += logo.o mdss_sharp_hd_dsi_panel.o
else ifeq ($(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WQXGA_PT_PANEL),y)
mdss-dsi-objs +=logo.o  mdss_samsung_tft_video_dual_dsi_panel.o LP8556-backlight.o
else ifeq ($(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_WXGA_PT_DUAL_PANEL),y)
mdss-dsi-objs += logo.o mdss_magna_octa_video_wxga_dual_panel.o smart_mtp_patek.o
else ifeq ($(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQXGA_S6TNMR7_PT_PANEL),y)
mdss-dsi-objs +=mdss_samsung_dual_oled_cmd_wqxga_panel.o smart_mtp_s6tnmr7.o
else ifeq ($(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQXGA_S6E3HA1_PT_PANEL),y)
mdss-dsi-objs +=mdss_samsung_dual_oled_cmd_wqxga_s6e3ha1_panel.o smart_mtp_s6e3ha1.o
else ifeq ($(CONFIG_FB_MSM_MDSS_SDC_WXGA_PANEL),y)
ifeq ($(CONFIG_MACH_DEGASLTE_SPR),y)
mdss-dsi-objs +=logo.o mdss_sdc_wxga_dsi_panel.o
else
mdss-dsi-objs +=logo.o mdss_sdc_wxga_dsi_panel.o  LP8556-backlight.o
endif
else ifeq ($(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL),y)
mdss-dsi-objs += logo.o mdss_samsung_dsi_panel.o smart_mtp_ea8061v.o
else ifeq ($(CONFIG_FB_MSM_MIPI_S6E8AA0A_720P_PT_PANEL),y)
mdss-dsi-objs += logo.o mdss_samsung_dsi_panel.o smart_mtp_se6e8fa.o smart_mtp_se6e3fa.o
else ifeq ($(CONFIG_FB_MSM_MIPI_JDI_TFT_VIDEO_FULL_HD_PT_PANEL),y)
mdss-dsi-objs += logo.o mdss_jdi_dsi_panel.o
else ifeq ($(CONFIG_FB_MSM_MIPI_S6E88A0_QHD_VIDEO_PT_PANEL),y)
mdss-dsi-objs += logo.o mdss_samsung_dsi_panel.o smart_mtp_se6e8fa.o smart_mtp_se6e3fa.o
else ifeq ($(CONFIG_FB_MSM_MDSS_S6E8AA0A_HD_PANEL),y)
mdss-dsi-objs += logo.o mdss_s6e8aa0a_hd_dsi_panel.o smart_mtp_s6e8aa0x01.o
else ifeq ($(CONFIG_FB_MSM_MIPI_VIDEO_WVGA_NT35502_PT_PANEL),y)
mdss-dsi-objs += mdss_dsi_panel.o
else ifeq ($(CONFIG_FB_MSM_MDSS_HX8369B_TFT_VIDEO_WVGA_PT_PANEL),y)
ifeq ($(CONFIG_BACKLIGHT_IC_KTD2801),y)
mdss-dsi-objs += logo.o mdss_hx8369b_wvga_dsi_panel.o mdss_msm_gpio_pwm.o
else
mdss-dsi-objs += logo.o mdss_hx8369b_wvga_dsi_panel.o
endif
else ifeq ($(CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL),y)
mdss-dsi-objs += logo.o mdss_hx8394c_720p_dsi_panel.o
else ifeq ($(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL),y)
mdss-dsi-objs += logo.o mdss_samsung_dsi_panel.o smart_mtp_ea8061v.o
else ifeq ($(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL),y)
mdss-dsi-objs += logo.o mdss_samsung_dsi_panel.o smart_mtp_ea8061.o
else
ifeq ($(CONFIG_FB_MSM_MDSS_TC_DSI2LVDS_WXGA_PANEL),y)
mdss-dsi-objs +=logo.o mdss_tc358764_dsi2lvds.o LP8556-backlight.o mdss_msm_gpio_pwm.o
else
ifeq ($(CONFIG_FB_MSM_MIPI_CMD_WVGA_ILI9806B_PT_PANEL),y)
mdss-dsi-objs +=mdss_dsi_panel.o
else
ifeq ($(CONFIG_FB_MSM_MDSS_CPT_QHD_PANEL),y)
mdss-dsi-objs +=logo.o mdss_cpt_qHD_dsi_panel.o
else
mdss-dsi-objs +=logo.o  mdss_samsung_tft_video_dual_dsi_panel.o LP8556-backlight.o
endif
endif
endif
endif

CFLAGS_smart_mtp_s6e3.o += -Wno-unused-variable
CFLAGS_smart_mtp_se6e3fa.o += -Wno-unused-variable
CFLAGS_smart_mtp_se6e8fa.o += -Wno-unused-variable
CFLAGS_smart_mtp_s6tnmr7.o += -Wno-unused-variable
CFLAGS_smart_mtp_s6e3ha1.o += -Wno-unused-variable

mdss-dsi-objs += msm_mdss_io_8974.o
ifeq ($(CONFIG_FB_MSM_MDSS_S6E8AA0A_HD_PANEL),y)
obj-$(CONFIG_MDNIE_LITE_TUNING) += mdss_video_enhance.o
else ifeq ($(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQXGA_S6TNMR7_PT_PANEL),y)
obj-$(CONFIG_MDNIE_LITE_TUNING) += mdnie_lite_tuning_chagall.o
else
obj-$(CONFIG_MDNIE_LITE_TUNING) += mdnie_lite_tuning.o
endif
obj-$(CONFIG_MDNIE_TFT_MSM8X26) += mdnie_tft_msm8x26.o
obj-$(CONFIG_CABC_TUNING) += cabc_tuning.o
obj-$(CONFIG_MDNIE_VIDEO_ENHANCED) += mdss_video_enhance.o

obj-$(CONFIG_FB_MSM_MDSS) += mdss-dsi.o
ifeq ($(CONFIG_FB_MSM_EDP_SAMSUNG),y)
obj-$(CONFIG_FB_MSM_EDP_SAMSUNG) += mdss_edp.o
obj-$(CONFIG_FB_MSM_EDP_SAMSUNG) += mdss_edp_aux.o
endif

obj-$(CONFIG_FB_MSM_MDSS) += mdss_io_util.o
obj-$(CONFIG_FB_MSM_MDSS_HDMI_PANEL) += mdss_hdmi_tx.o
obj-$(CONFIG_FB_MSM_MDSS_HDMI_PANEL) += mdss_hdmi_util.o
obj-$(CONFIG_FB_MSM_MDSS_HDMI_PANEL) += mdss_hdmi_edid.o
obj-$(CONFIG_FB_MSM_MDSS_HDMI_PANEL) += mdss_hdmi_hdcp.o
obj-$(CONFIG_FB_MSM_MDSS_HDMI_PANEL) += mdss_hdmi_cec.o
obj-$(CONFIG_FB_MSM_MDSS_HDMI_MHL_SII8334) += mhl_sii8334.o mhl_msc.o

obj-$(CONFIG_FB_MSM_MDSS_WRITEBACK) += mdss_wb.o

mdss-qpic-objs := mdss_qpic.o mdss_fb.o mdss_qpic_panel.o
obj-$(CONFIG_FB_MSM_QPIC) += mdss-qpic.o
obj-$(CONFIG_FB_MSM_QPIC_ILI_QVGA_PANEL) += qpic_panel_ili_qvga.o

obj-$(CONFIG_FB_MSM_MDSS) += mdss_fb.o

ifeq ($(CONFIG_FB_MSM_MDSS_MDP3),y)
obj-$(CONFIG_FB_MSM_MDSS_DSI_CTRL_STATUS) += dsi_status_v2.o
else
obj-$(CONFIG_FB_MSM_MDSS_DSI_CTRL_STATUS) += mdss_dsi_status.o
endif

obj-$(CONFIG_FB_MSM_EDP_SAMSUNG) += edp-eeprom.o edp-backlight.o
ifeq ($(CONFIG_FB_MSM_EDP_SAMSUNG),y)
obj-$(CONFIG_EDP_TCON_MDNIE) += edp_tcon_mdnie.o
endif

obj-$(CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL) += isl98611-backlight.o

obj-$(CONFIG_FB_MSM_MDSS_KCAL_CTRL) += mdss_mdp_kcal_ctrl.o
