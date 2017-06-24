/////////////////////////////////////////////////////////////////////////////////////////
//		Concord Typical Functional Operation Mode
//		Mode 1: Full Normal PAF correct only
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
// 73C1XX
// Full resolution V2
// x_output_size: 5328
// y_output_size: 3000
// frame_rate: 30.002
// input_format: RAW10			-->		output_format: RAW10
// input_interface: MIPI		-->		output_interface: MIPI
// input_lanes: 4				-->		output_lanes: 4
// input_clock_mhz: 1392.000	-->		output_clock_mhz: 1392.000
// system_clock_mhz: 560.000
// input_clock_mhz: 24.00 

/////////////////////////////////////////////////////////////////////////////////////////
// $Rev: 4424 $
// $Date: 2013-10-03 17:30:26 +0200 (Thu, 03 Oct 2013) $
/////////////////////////////////////////////////////////////////////////////////////////
static struct msm_camera_i2c_reg_array mode1_sset_step1_array[] = {
{0x0118, 0x0000},	//set config to config_0 5328x3000
{0x0588, 0x0000},	//WDR disable
{0x1206, 0x0000},	//No SIRC BPC indication out
{0x1208, 0x00F5},	//SIRC BPC Correct PAF ONLY
{0x0E80, 0x0055},	//Tango BPC disable
{0x0E82, 0x0000},	//Tango BPC correctionMode: Do Nothing
//WRITE	#api_init_done									0001	//Use init_done for initial set-up only; otherwise use Host Interrupt
};
//p10
static struct msm_camera_i2c_reg_array mode1_sset_step2_array[] = {
{0x7000, 0x0001}	// Write Host Interrupt
};

static struct msm_camera_i2c_reg_setting mode1_sset_1[] = {
  {
    .reg_setting = mode1_sset_step1_array,
    .size = ARRAY_SIZE(mode1_sset_step1_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_WORD_DATA,
    .delay = 10,
  },
};
static struct msm_camera_i2c_reg_setting mode1_sset_2[] = {
  {
    .reg_setting = mode1_sset_step2_array,
    .size = ARRAY_SIZE(mode1_sset_step2_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_WORD_DATA,
    .delay = 1,
  },
};
