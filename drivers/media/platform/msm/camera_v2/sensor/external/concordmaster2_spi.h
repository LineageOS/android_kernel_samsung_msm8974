
/////////////////////////////////////////////////////////////////////////////////////////
//		Concord master set file for 2P2 EVT0 BSI - TN Integration
//		Note some of the Reg's values are the FW default values they are mentioned here to understand more how the system work
//		Muster set file is supporting 3 main Typical Functional Operation Modes using 2P2 as Ext sensor:
//		1.Full 5344x3000 MIPI4@1392MHz 30fps InputCLK: 24MHz 
//		2.Binning_2 for FHD 2672x1500 MIPI3@1092MHz 60fps InputCLK: 24MHz
//		3.WVGA 890x500 MIPI2@996MHz 300fps InputCLK: 24MHz
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
// $Rev: 4462 $
// $Date: 2013-10-15 18:06:25 +0300 (Tue, 15 Oct 2013) $
/////////////////////////////////////////////////////////////////////////////////////////

static struct msm_camera_i2c_reg_array concord_master_setfile_sset_array[] = {
/////////////////////////////////////////////////////////////////////////////////////////
//		1.Sensor Attribute Tuning Params + AF params + xtalkTuningParams : 
/////////////////////////////////////////////////////////////////////////////////////////

//No need to configure  PDFA pattern description,the default FW PDAF parameters match the PDAF pattern (pattern shape offset type)
{0x11EA, 0x0008},	//xtalkTuningParams_calibrateArea_topLeftX
{0x11EC, 0x0008},	//xtalkTuningParams_calibrateArea_topLeftY
{0x11EE, 0x14F8},	//xtalkTuningParams_calibrateArea_Width			//5352
{0x11F0, 0x0BC8},	//xtalkTuningParams_calibrateArea_Hight		 	//3016
{0x07D4, 0x0000},	//sensorAttrTuningParams_activeStartY
{0x07D2, 0x0000},	//sensorAttrTuningParams_activeStartX
{0x07D6, 0x14F7},	//sensorAttrTuningParams_activeLastX				//5336+15
{0x07D8, 0x0BC7},	//sensorAttrTuningParams_activeLastY				//3000+15
{0x07DA, 0x0000},	//sensorAttrTuningParams_headColor				//Gr First
{0x07DC, 0x0001},	//sensorAttrTuningParams_exposure_order_green	//WDR Pattern Long-Gr,Short-r,Short-b
{0x07DE, 0x0000},	//sensorAttrTuningParams_exposure_order_red
{0x07E0, 0x0000},	//sensorAttrTuningParams_exposure_order_blue
{0x0E40, 0x0014},	//afTuningParams_afBlockStartX			//2P2 PDAF 64x64 Block start at X=20,Y=36 (Relate to Full Active 2P2 ,including dummy active 5352*3016)
{0x0E42, 0x0024},	//afTuningParams_afBlockStartY
{0x0E44, 0x14E3},	//afTuningParams_afBlockLastX			//20+5312-1=5331	Effective PDAF resolution	5312(83*64)X 2944(46*64)
{0x0E46, 0x0BA3},	//afTuningParams_afBlockLastY			//36+2944-1=2979
{0x0E50, 0x014C},	//afTuningParams_fullAFStatImageWidth	//(4 Per 64x64 Blk)4*83=332
{0x0E52, 0x00B8},	//afTuningParams_fullAFStatImageHeight	//(4 64x64 Per AF type x2)	4*46=184

/////////////////////////////////////////////////////////////////////////////////////////
//		2.PLL Settings - Start
/////////////////////////////////////////////////////////////////////////////////////////

{0x0122, 0x1800},	//api_input_clock	//24MHz

//ISP Concord System CLK=560Mhz (Main VCO 24/6=4*560/4=560) (Arm Clk 560/2=280Mhz)
{0x0132, 0x0006},	//api_clock_0__SystemMainPll_uPreDiv
{0x0130, 0x0230},	//api_clock_0__SystemMainPll_uMult
{0x0134, 0x0002},	//api_clock_0__SystemMainPll_uShifter

//MIPI Clk 24/4=6*116=696DDR-->1392Mhz per Lane 
{0x0138, 0x0004},	//api_clock_0__SystemOifPll_uPreDiv
{0x0136, 0x0074},	//api_clock_0__SystemOifPll_uMult
{0x013A, 0x0000},	//api_clock_0__SystemOifPll_uShifter

//========================================================================================
//ISP Concord System CLK=560Mhz (Main VCO 24/6=4*560/4=560) (Arm Clk 560/2=280Mhz)
{0x013E, 0x0006},	//api_clock_1__SystemMainPll_uPreDiv
{0x013C, 0x0230},	//api_clock_1__SystemMainPll_uMult
{0x0140, 0x0002},	//api_clock_1__SystemMainPll_uShifter

//MIPI Clk 24/4=6*91=546DDR-->1092Mhz per Lane 
{0x0144, 0x0004},	//api_clock_1__SystemOifPll_uPreDiv
{0x0142, 0x005B},	//api_clock_1__SystemOifPll_uMult
{0x0146, 0x0000},	//api_clock_1__SystemOifPll_uShifter

//========================================================================================
//ISP Concord System CLK=560Mhz (Main VCO 24/6=4*560/4=560) (Arm Clk 560/2=280Mhz)
{0x014A, 0x0006},	//api_clock_2__SystemMainPll_uPreDiv
{0x0148, 0x0230},	//api_clock_2__SystemMainPll_uMult
{0x014C, 0x0002},	//api_clock_2__SystemMainPll_uShifter

//MIPI Clk 24/4=6*83=498DDR-->996Mhz per Lane 
{0x0150, 0x0004},	//api_clock_2__SystemOifPll_uPreDiv
{0x014E, 0x0053},	//api_clock_2__SystemOifPll_uMult
{0x0152, 0x0000},	//api_clock_2__SystemOifPll_uShifter

/////////////////////////////////////////////////////////////////////////////////////////
//		2.PLL Settings - End
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
//		3.Configuration input/output (Size,Interface) - Start
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
//		Configuration 0: Modes 1-3
/////////////////////////////////////////////////////////////////////////////////////////
{0x0154, 0x0000},	//api_config_0__ClockConfigIndex			//Clock 0
{0x0176, 0x0000},	//api_config_0__input_uPixelOrder		//Gr first
{0x0168, 0x000C},	//api_config_0__input_uStartX
{0x016A, 0x0008},	//api_config_0__input_uStartY
{0x016C, 0x14E0},	//api_config_0__input_uWidth				//5344
{0x016E, 0x0BB8},	//api_config_0__input_uHeight			//3000
{0x0172, 0x0000},	//api_config_0__input_uMirrorXY			//No Mirror
{0x0174, 0x0101},	//api_config_0__input_uAnalogBinning		//No Analog binning was done on Input image
{0x017A, 0x1010},	//api_config_0__input_scaler_uDownScale	//No Digital binning was done on Input image
{0x0180, 0x16C0},	//api_config_0__input_uLineLengthPck		//5736
{0x0184, 0x0008},   //api_config_0__input_pixel_clock_khz	//CIS ISP clk 560000Khz
{0x0186, 0x8B80},	//api_config_0__input_pixel_clock_khz	//CIS ISP clk 560000Khz
{0x018A, 0x0570},	//api_config_0__input_uInputMipiClockMHz	//1392Mhz
{0x0188, 0x0004},	//api_config_0__input_uNumberOfLanes		//4 Lane
{0x0170, 0x000A},	//api_config_0__input_uEffectiveBittage
{0x017E, 0x0040},	//api_config_0__input_pedestal			//No Input Pedestal

//Image input is Gr so no need to use First Cropper block
{0x018C, 0x0000},	//api_config_0__output_inputCropper_margins_topLeft_x
{0x018E, 0x0000},	//api_config_0__output_inputCropper_margins_topLeft_y
{0x0190, 0x0000},	//api_config_0__output_inputCropper_margins_bottomRight_x
{0x0192, 0x0000},	//api_config_0__output_inputCropper_margins_bottomRight_y
{0x01A2, 0x0004},	//api_config_0__output_uNubmerOfLanes	//Output MIPI mode 4 lane
{0x019A, 0x1010},	//api_config_0__output_scaler_uDownScale	//Digital binning. 4 fraction bits:[7:0] Vertical binning.[15:8], Horizontal binning
{0x019C, 0x14E0},	//api_config_0__output_uWidth
{0x019E, 0x0BB8},	//api_config_0__output_uHeight
{0x01A0, 0x000A},	//api_config_0__output_uBits
{0x015C, 0x0000},	//api_config_0__uEmbInOut
{0x0162, 0x0006},	//api_config_0__StatMethod
{0x0164, 0xFFC0},	//api_config_0__StatEnable				//
{0x0156, 0x0001},	//api_config_0__PDAFMode					//PDAF pixels mode: 0 - None, 1 - enabled

//Set PDAF window in the middle half of the image 
{0x01A4, 0x0000},	//api_config_0__afstat_bBypass
{0x01A6, 0x0001},	//api_config_0__afstat_numWindows
{0x01A8, 0x1000},	//api_config_0__afstat_windows_0__topLeft_x
{0x01AA, 0x1000},	//api_config_0__afstat_windows_0__topLeft_y
{0x01AC, 0x3000},	//api_config_0__afstat_windows_0__bottomRight_x
{0x01AE, 0x3000},	//api_config_0__afstat_windows_0__bottomRight_y

/////////////////////////////////////////////////////////////////////////////////////////
//		Configuration 1: Modes 4-6
/////////////////////////////////////////////////////////////////////////////////////////
{0x0238, 0x0001},	//api_config_1__ClockConfigIndex			//Clock 1
{0x025A, 0x0000},	//api_config_1__input_uPixelOrder		//Gr first
{0x024C, 0x0018},	//api_config_1__input_uStartX
{0x024E, 0x0008},	//api_config_1__input_uStartY
{0x0250, 0x0A70},	//api_config_1__input_uWidth				//2672
{0x0252, 0x05DC},	//api_config_1__input_uHeight			//1500
{0x0256, 0x0000},	//api_config_1__input_uMirrorXY			//No Mirror
{0x0258, 0x0202},	//api_config_1__input_uAnalogBinning		//Analog binning was done on Input image
{0x025E, 0x1010},	//api_config_1__input_scaler_uDownScale	//No Digital binning was done on Input image
{0x0264, 0x14C0},	//api_config_1__input_uLineLengthPck		//5312
{0x0268, 0x0008},   //api_config_1__input_pixel_clock_khz	//CIS ISP clk 560000Khz
{0x026A, 0x8B80},	//api_config_1__input_pixel_clock_khz	//CIS ISP clk 560000Khz
{0x026E, 0x0444},	//api_config_1__input_uInputMipiClockMHz	//1092Mhz
{0x026C, 0x0003},	//api_config_1__input_uNumberOfLanes		//3 Lane
{0x0254, 0x000A},	//api_config_1__input_uEffectiveBittage
{0x0262, 0x0040},	//api_config_1__input_pedestal			//No Input Pedestal

//Image input is Gr so no need to use First Cropper block
{0x0270, 0x0000},	//api_config_1__output_inputCropper_margins_topLeft_x
{0x0272, 0x0000},	//api_config_1__output_inputCropper_margins_topLeft_y
{0x0274, 0x0000},	//api_config_1__output_inputCropper_margins_bottomRight_x
{0x0276, 0x0000},	//api_config_1__output_inputCropper_margins_bottomRight_y
{0x0286, 0x0003},	//api_config_1__output_uNubmerOfLanes	//Output MIPI mode 3 lane
{0x027E, 0x1010},	//api_config_1__output_scaler_uDownScale	//Digital binning. 4 fraction bits:[7:0] Vertical binning.[15:8], Horizontal binning
{0x0280, 0x0A70},	//api_config_1__output_uWidth
{0x0282, 0x05DC},	//api_config_1__output_uHeight
{0x0284, 0x000A},	//api_config_1__output_uBits
{0x0240, 0x0000},	//api_config_1__uEmbInOut
{0x0246, 0x0006},	//api_config_1__StatMethod
{0x0248, 0xFFC0},	//api_config_1__StatEnable				//
{0x023A, 0x0001},	//api_config_1__PDAFMode					//PDAF pixels mode: 0 - None, 1 - enabled

//Set PDAF window in the middle half of the image 
{0x0288, 0x0000},	//api_config_1__afstat_bBypass
{0x028A, 0x0001},	//api_config_1__afstat_numWindows
{0x028C, 0x1000},	//api_config_1__afstat_windows_0__topLeft_x
{0x028E, 0x1000},	//api_config_1__afstat_windows_0__topLeft_y
{0x0290, 0x3000},	//api_config_1__afstat_windows_0__bottomRight_x
{0x0292, 0x3000},	//api_config_1__afstat_windows_0__bottomRight_y

/////////////////////////////////////////////////////////////////////////////////////////
//		Configuration 2: Mode 7
/////////////////////////////////////////////////////////////////////////////////////////
{0x031C, 0x0002},	//api_config_2__ClockConfigIndex			//Clock 2
{0x033E, 0x0000},	//api_config_2__input_uPixelOrder		//Gr first
{0x0330, 0x00C0},	//api_config_2__input_uStartX
{0x0332, 0x0008},	//api_config_2__input_uStartY
{0x0334, 0x0380},	//api_config_2__input_uWidth				//890
{0x0336, 0x01F4},	//api_config_2__input_uHeight			//500
{0x033A, 0x0000},	//api_config_2__input_uMirrorXY			//No Mirror
{0x033C, 0x0606},	//api_config_2__input_uAnalogBinning		// Analog binning was done on Input image
{0x0342, 0x1010},	//api_config_2__input_scaler_uDownScale	// Digital binning was done on Input image
{0x0348, 0x0C70},	//api_config_2__input_uLineLengthPck		//3184
{0x034C, 0x0008},   //api_config_2__input_pixel_clock_khz	//CIS ISP clk 560000Khz
{0x034C, 0x8B80},	//api_config_2__input_pixel_clock_khz	//CIS ISP clk 560000Khz
{0x0352, 0x0364},	//api_config_2__input_uInputMipiClockMHz	//996Mhz
{0x0350, 0x0002},	//api_config_2__input_uNumberOfLanes		//2 Lane
{0x0338, 0x000A},	//api_config_2__input_uEffectiveBittage
{0x0346, 0x0040},	//api_config_2__input_pedestal			//No Input Pedestal

//Image input is Gr so no need to use First Cropper block
{0x0354, 0x0000},	//api_config_2__output_inputCropper_margins_topLeft_x
{0x0356, 0x0000},	//api_config_2__output_inputCropper_margins_topLeft_y
{0x0358, 0x0000},	//api_config_2__output_inputCropper_margins_bottomRight_x
{0x035A, 0x0000},	//api_config_2__output_inputCropper_margins_bottomRight_y
{0x036A, 0x0002},	//api_config_2__output_uNubmerOfLanes	//Output MIPI mode 2 lane
{0x0362, 0x1010},	//api_config_2__output_scaler_uDownScale	//Digital binning. 4 fraction bits:[7:0] Vertical binning.[15:8], Horizontal binning
{0x0364, 0x0380},	//api_config_2__output_uWidth			//890
{0x0366, 0x01F4},	//api_config_2__output_uHeight			//500
{0x0368, 0x000A},	//api_config_2__output_uBits
{0x0324, 0x0000},	//api_config_2__uEmbInOut
{0x032A, 0x0006},	//api_config_2__StatMethod
{0x032C, 0x0FC0},	//api_config_2__StatEnable			//No PDAF Stat Data
{0x031E, 0x0000},	//api_config_2__PDAFMode			//PDAF pixels mode: 0 - None, 1 - enabled

/////////////////////////////////////////////////////////////////////////////////////////
//		3.Configuration input/output (Size,Interface) - End
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
//		4.Functional operation Mode
/////////////////////////////////////////////////////////////////////////////////////////
{0x083A, 0x0030},	//outifTuningParams_uFooterMipiDataType - User Define
{0x084C, 0x0000},	//outifTuningParams_bEmbEqualToPixel - one byte
{0x077E, 0x1000},	//SflashTuningParams_uChipSize_KB			//SFLASH memory size 4M (tuning update)
{0x05CC, 0x0000},	//api_otf_WdrMode			//WDR disable
{0x138A, 0x0000},	//dspclTuning_outStreamMode
{0x138C, 0xFFFF},	//dspclTuning_correctionMode	//SIRC BPC Correct All
{0x1000, 0x0055},	//tangoBpcTuningParams_mode	//Tango BPC disable
{0x0126, 0x0010},	//api_ext_sen_id	//Set sensor iic ID Device for Image out and not STRGEN (for STRGEN set 0000)
{0x0128, 0x0000},	//api_config_idx	//Set config to config_0 5328x3000
{0x0124, 0x0000},	//api_debug_out	//set full Concord Chin Out
{0x012A, 0x0001},	//api_init_done
};
//p10
