
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
// $Rev: 4513 $
// $Date: 2013-11-07 19:37:41 +0200 (Thu, 07 Nov 2013) $
/////////////////////////////////////////////////////////////////////////////////////////
//// Mode setting ////
static struct msm_camera_i2c_reg_array mode1_sset_step1_array[] = {
{0x00A0, 0x0000},	//Set config to config_0 5328x3000
{0x04D8, 0x0000},	//WDR disable
{0x15C8, 0x0000},	//SIRC BPC disable
{0x15D6, 0x0000},	//No SIRC BPC indication out
{0x15D8, 0x0000},	//SIRC BPC Correct PAF ONLY
{0x1250, 0x0000},	//Tango BPC disable
{0x1252, 0x00F5},	//Tango BPC correctionMode: Do Nothing
{0x00F4, 0xA7C1},
//WRITE	#api_init_done						0001	//Use init_done for initial set-up only; otherwise use Host Interrupt
};

//p10

static struct msm_camera_i2c_reg_array mode1_sset_step2_array[] = {
{0x7000, 0x0001},	// Write Host Interrupt
};
