
/////////////////////////////////////////////////////////////////////////////////////////
//		Concord Typical Functional Operation Mode
//		Mode 1: Full Normal PAF correct only
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
// 73C1XX
// Full resolution V2
// x_output_size: 5344
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

//// Mode setting ////
static struct msm_camera_i2c_reg_array mode1_sset_step1_array[] = {
{0x0128, 0x0000},	//Set config to config_0 5344x3000
{0x05CC, 0x0000},	//WDR disable
{0x137C, 0x0000},	//SIRC BPC Off
{0x138A, 0x0000},	//No SIRC BPC indication out
{0x138C, 0x0000},	//SIRC BPC Correct no indicate
{0x1000, 0x0000},	//Tango BPC disable
{0x1002, 0x0055},	//Tango BPC correction Mode: PAF only
};
//WRITE	#api_init_done 0001	//Use init_done for initial set-up only; otherwise use Host Interrupt

//p10
static struct msm_camera_i2c_reg_array mode1_sset_step2_array[] = {
{0x7000, 0x0001},	// Write Host Interrupt
};
