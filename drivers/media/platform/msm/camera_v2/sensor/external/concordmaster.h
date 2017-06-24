/////////////////////////////////////////////////////////////////////////////////////////
//		Concord master set file for 2P2 EVT0 BSI - TN Integration
//		Note some of the Reg's values are the FW default values they are mentioned here to understand more how the system work
//		Muster set file is supporting 3 main Typical Functional Operation Modes using 2P2 as Ext sensor:
//		1.Full 5328x3000 MIPI4@1392MHz 30fps InputCLK: 24MHz
//		2.Binning_2 for FHD 2656x1500 MIPI3@1092MHz 60fps InputCLK: 24MHz
//		3.WVGA 832x500 MIPI2@996MHz 300fps InputCLK: 24MHz
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
// $Rev: 4424 $
// $Date: 2013-10-03 17:30:26 +0200 (Thu, 03 Oct 2013) $
/////////////////////////////////////////////////////////////////////////////////////////

static struct msm_camera_i2c_reg_array concord_master_setfile_sset_array[] = {
/////////////////////////////////////////////////////////////////////////////////////////
//		1.Sensor Attribute Tuning Params + AF params + xtalkTuningParams :
/////////////////////////////////////////////////////////////////////////////////////////

//No need to configure  PDFA pattern description,the default FW PDAF parameters match the PDAF pattern (pattern shape offset type)
{0x106A, 0x0008},	//xtalkTuningParams_calibrateArea_topLeftX
{0x106C, 0x0008},	//xtalkTuningParams_calibrateArea_topLeftY
{0x106E, 0x14E8},	//xtalkTuningParams_calibrateArea_Width			//5352
{0x1070, 0x0BC8},	//xtalkTuningParams_calibrateArea_Hight		 	//3016
{0x0794, 0x0000},	//sensorAttrTuningParams_activeStartY
{0x0792, 0x0000},	//sensorAttrTuningParams_activeStartX
{0x0796, 0x14E7},	//sensorAttrTuningParams_activeLastX				//5336+15
{0x0798, 0x0BC7},	//sensorAttrTuningParams_activeLastY				//3000+15
{0x079A, 0x0000},	//sensorAttrTuningParams_headColor				//Gr First
{0x079C, 0x0001},	//sensorAttrTuningParams_exposure_order_green	//WDR Pattern Long-Gr,Short-r,Short-b
{0x079E, 0x0000},	//sensorAttrTuningParams_exposure_order_red
{0x07A0, 0x0000},	//sensorAttrTuningParams_exposure_order_blue
{0x0CC0, 0x0014},	//afTuningParams_afBlockStartX			//2P2 PDAF 64x64 Block start at X=20,Y=36 (Relate to Full Active 2P2 ,including dummy active 5352*3016)
{0x0CC2, 0x0024},	//afTuningParams_afBlockStartY
{0x0CC4, 0x14D3},	//afTuningParams_afBlockLastX			//20+5312-1=5331	Effective PDAF resolution	5312(83*64)X 2944(46*64)
{0x0CC6, 0x0BA3},	//afTuningParams_afBlockLastY			//36+2944-1=2979
{0x0CD0, 0x014C},	//afTuningParams_fullAFStatImageWidth	//(4 Per 64x64 Blk)4*83=332
{0x0CD2, 0x00B8},	//afTuningParams_fullAFStatImageHeight	//(4 64x64 Per AF type x2)	4*46=184

/////////////////////////////////////////////////////////////////////////////////////////
//		2.PLL Settings - Start
/////////////////////////////////////////////////////////////////////////////////////////

{0x0112, 0x1800},	//api_input_clock	//24MHz
//ISP Concord System CLK=560Mhz (Main VCO 24/6=4*560/4=560) (Arm Clk 560/2=280Mhz)

{0x0120, 0x0230},	//api_clock_0__SystemMainPll_uMult
{0x0122, 0x0602},	//api_clock_0__SystemMainPll_uPreDiv, api_clock_0__SystemMainPll_uShifter
//MIPI Clk 24/4=6*116=696DDR-->1392Mhz per Lane

{0x0124, 0x0074},	//api_clock_0__SystemOifPll_uMult
{0x0126, 0x0400},	//api_clock_0__SystemOifPll_uPreDiv, api_clock_0__SystemOifPll_uShifter
//========================================================================================

//ISP Concord System CLK=560Mhz (Main VCO 24/6=4*560/4=560) (Arm Clk 560/2=280Mhz)

{0x0128, 0x0230},	//api_clock_1__SystemMainPll_uMult
{0x012A, 0x0602},	//api_clock_1__SystemMainPll_uPreDiv, api_clock_1__SystemMainPll_uShifter
//MIPI Clk 24/4=6*91=546DDR-->1092Mhz per Lane

{0x012C, 0x005B},	//api_clock_1__SystemOifPll_uMult
{0x012E, 0x0400},	//api_clock_1__SystemOifPll_uPreDiv, api_clock_1__SystemOifPll_uShifter
//========================================================================================

//ISP Concord System CLK=560Mhz (Main VCO 24/6=4*560/4=560) (Arm Clk 560/2=280Mhz)

{0x0130, 0x0230},	//api_clock_2__SystemMainPll_uMult
{0x0132, 0x0602},	//api_clock_2__SystemMainPll_uPreDiv, api_clock_2__SystemMainPll_uShifter
//MIPI Clk 24/4=6*83=498DDR-->996Mhz per Lane

{0x0134, 0x0053},	//api_clock_2__SystemOifPll_uMult
{0x0136, 0x0400},	//api_clock_2__SystemOifPll_uPreDiv, api_clock_2__SystemOifPll_uShifter
/////////////////////////////////////////////////////////////////////////////////////////
//		2.PLL Settings - End
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
//		3.Configuration input/output (Size,Interface) - Start
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
//		Configuration 0: Modes 1-3
/////////////////////////////////////////////////////////////////////////////////////////

{0x0138, 0x0000},	//api_config_0__ClockConfigIndex			//Clock 0
{0x015A, 0x0000},	//api_config_0__input_uPixelOrder		//Gr first
{0x014C, 0x000C},	//api_config_0__input_uStartX
{0x014E, 0x0008},	//api_config_0__input_uStartY
{0x0150, 0x14D0},	//api_config_0__input_uWidth				//5328
{0x0152, 0x0BB8},	//api_config_0__input_uHeight			//3000
{0x0156, 0x0000},	//api_config_0__input_uMirrorXY			//No Mirror
{0x0158, 0x0101},	//api_config_0__input_uAnalogBinning		//No Analog binning was done on Input image
{0x015E, 0x1010},	//api_config_0__input_scaler_uDownScale	//No Digital binning was done on Input image
{0x0164, 0x1668},	//api_config_0__input_uLineLengthPck		//5736
{0x0168, 0x0008},	//api_config_0__input_pixel_clock_khz	//CIS ISP clk 560000Khz
{0x016A, 0x8B80},
{0x016E, 0x0570},	//api_config_0__input_uInputMipiClockMHz	//1392Mhz
{0x016C, 0x0004},	//api_config_0__input_uNumberOfLanes		//4 Lane
{0x0154, 0x000A},	//api_config_0__input_uEffectiveBittage
{0x0162, 0x0000},	//api_config_0__input_pedestal			//No Input Pedestal
//Image input is Gr so no need to use First Cropper block

{0x0170, 0x0000},	//api_config_0__output_inputCropper_margins_topLeft_x
{0x0172, 0x0000},	//api_config_0__output_inputCropper_margins_topLeft_y
{0x0174, 0x0000},	//api_config_0__output_inputCropper_margins_bottomRight_x
{0x0176, 0x0000},	//api_config_0__output_inputCropper_margins_bottomRight_y
{0x0186, 0x0004},	//api_config_0__output_uNubmerOfLanes	//Output MIPI mode 4 lane
{0x017E, 0x1010},	//api_config_0__output_scaler_uDownScale	//Digital binning. 4 fraction bits:[7:0] Vertical binning.[15:8], Horizontal binning
{0x0180, 0x14D0},	//api_config_0__output_uWidth
{0x0182, 0x0BB8},	//api_config_0__output_uHeight
{0x0184, 0x000A},	//api_config_0__output_uBits
{0x0140, 0x0000},	//api_config_0__uEmbInOut
{0x0146, 0x0006},	//api_config_0__StatMethod
{0x0148, 0x0000},	//api_config_0__StatEnable				//
{0x013A, 0x0001},	//api_config_0__PDAFMode					//PDAF pixels mode: 0 - None, 1 - enabled
/////////////////////////////////////////////////////////////////////////////////////////
//		Configuration 1: Modes 4-6
/////////////////////////////////////////////////////////////////////////////////////////

{0x0214, 0x0001},	//api_config_1__ClockConfigIndex			//Clock 1
{0x0236, 0x0000},	//api_config_1__input_uPixelOrder		//Gr first
{0x0228, 0x0018},	//api_config_1__input_uStartX
{0x022A, 0x0008},	//api_config_1__input_uStartY
{0x022C, 0x0A60},	//api_config_1__input_uWidth				//2656
{0x022E, 0x05DC},	//api_config_1__input_uHeight			//1500
{0x0232, 0x0000},	//api_config_1__input_uMirrorXY			//No Mirror
{0x0234, 0x0202},	//api_config_1__input_uAnalogBinning		//Analog binning was done on Input image
{0x023A, 0x1010},	//api_config_1__input_scaler_uDownScale	//No Digital binning was done on Input image
{0x0240, 0x14C0},	//api_config_1__input_uLineLengthPck		//5312
{0x0244, 0x0008},	//api_config_1__input_pixel_clock_khz	//CIS ISP clk 560000Khz
{0x0246, 0x8B80},
{0x024A, 0x0444},	//api_config_1__input_uInputMipiClockMHz	//1092Mhz
{0x0248, 0x0003},	//api_config_1__input_uNumberOfLanes		//3 Lane
{0x0230, 0x000A},	//api_config_1__input_uEffectiveBittage
{0x023E, 0x0000},	//api_config_1__input_pedestal			//No Input Pedestal
//Image input is Gr so no need to use First Cropper block

{0x024C, 0x0000},	//api_config_1__output_inputCropper_margins_topLeft_x
{0x024E, 0x0000},	//api_config_1__output_inputCropper_margins_topLeft_y
{0x0250, 0x0000},	//api_config_1__output_inputCropper_margins_bottomRight_x
{0x0252, 0x0000},	//api_config_1__output_inputCropper_margins_bottomRight_y
{0x0262, 0x0003},	//api_config_1__output_uNubmerOfLanes	//Output MIPI mode 3 lane
{0x025A, 0x1010},	//api_config_1__output_scaler_uDownScale	//Digital binning. 4 fraction bits:[7:0] Vertical binning.[15:8], Horizontal binning
{0x025C, 0x0A60},	//api_config_1__output_uWidth
{0x025E, 0x05DC},	//api_config_1__output_uHeight
{0x0260, 0x000A},	//api_config_1__output_uBits
{0x021C, 0x0000},	//api_config_1__uEmbInOut
{0x0222, 0x0006},	//api_config_1__StatMethod
{0x0224, 0x0000},	//api_config_1__StatEnable				//
{0x0216, 0x0001},	//api_config_1__PDAFMode					//PDAF pixels mode: 0 - None, 1 - enabled
/////////////////////////////////////////////////////////////////////////////////////////
//		Configuration 2: Mode 7
/////////////////////////////////////////////////////////////////////////////////////////

{0x02F0, 0x0002},	//api_config_2__ClockConfigIndex			//Clock 2
{0x0312, 0x0000},	//api_config_2__input_uPixelOrder		//Gr first
{0x0304, 0x00C0},	//api_config_2__input_uStartX
{0x0306, 0x0008},	//api_config_2__input_uStartY
{0x0308, 0x0340},	//api_config_2__input_uWidth				//832
{0x030A, 0x01F4},	//api_config_2__input_uHeight			//500
{0x030E, 0x0000},	//api_config_2__input_uMirrorXY			//No Mirror
{0x0310, 0x0606},	//api_config_2__input_uAnalogBinning		// Analog binning was done on Input image
{0x0316, 0x1010},	//api_config_2__input_scaler_uDownScale	// Digital binning was done on Input image
{0x031C, 0x0C70},	//api_config_2__input_uLineLengthPck		//3184
{0x0320, 0x0008},	//api_config_2__input_pixel_clock_khz	//CIS ISP clk 560000Khz
{0x0322, 0x8B80},
{0x0326, 0x0364},	//api_config_2__input_uInputMipiClockMHz	//996Mhz
{0x0324, 0x0002},	//api_config_2__input_uNumberOfLanes		//2 Lane
{0x030C, 0x000A},	//api_config_2__input_uEffectiveBittage
{0x031A, 0x0000},	//api_config_2__input_pedestal			//No Input Pedestal
//Image input is Gr so no need to use First Cropper block

{0x0328, 0x0000},	//api_config_2__output_inputCropper_margins_topLeft_x
{0x032A, 0x0000},	//api_config_2__output_inputCropper_margins_topLeft_y
{0x032C, 0x0000},	//api_config_2__output_inputCropper_margins_bottomRight_x
{0x032E, 0x0000},	//api_config_2__output_inputCropper_margins_bottomRight_y
{0x033E, 0x0002},	//api_config_2__output_uNubmerOfLanes	//Output MIPI mode 2 lane
{0x0336, 0x1010},	//api_config_2__output_scaler_uDownScale	//Digital binning. 4 fraction bits:[7:0] Vertical binning.[15:8], Horizontal binning
{0x0338, 0x0340},	//api_config_2__output_uWidth			//832
{0x033A, 0x01F4},	//api_config_2__output_uHeight			//500
{0x033C, 0x000A},	//api_config_2__output_uBits
{0x02F8, 0x0000},	//api_config_2__uEmbInOut
{0x02FE, 0x0006},	//api_config_2__StatMethod
{0x0300, 0x0000},	//api_config_2__StatEnable				//
{0x02F2, 0x0000},	//api_config_2__PDAFMode					//PDAF pixels mode: 0 - None, 1 - enabled
/////////////////////////////////////////////////////////////////////////////////////////
//		3.Configuration input/output (Size,Interface) - End
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
//		4.Functional operation Mode
/////////////////////////////////////////////////////////////////////////////////////////

{0x073E, 0x1000},	//SflashTuningParams_uChipSize_KB			//SFLASH memory size 4M (tuning update)
{0x0588, 0x0000},	//api_otf_WdrMode			//WDR disable
{0x1206, 0x0000},	//dspclTuning_outStreamMode
{0x1208, 0xFFFF},	//dspclTuning_correctionMode	//SIRC BPC Correct All
{0x0E80, 0x0055},	//tangoBpcTuningParams_mode	//Tango BPC disable
{0x0116, 0x0010},	//api_ext_sen_id	//Set sensor iic ID Device for Image out and not STRGEN (for STRGEN set 0000)
{0x0118, 0x0000},	//api_config_idx	//Set config to config_0 5328x3000
{0x0114, 0x0000},	//api_debug_out	//set full Concord Chin Out
{0x011A, 0x0001},	//api_init_done
{0x0829, 0x0003},
};
//p10

//WRITE 40007000 0001 // Write Host Interrupt

static struct msm_camera_i2c_reg_setting concord_master_setfile_sset[] = {
  {
    .reg_setting = concord_master_setfile_sset_array,
    .size = ARRAY_SIZE(concord_master_setfile_sset_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_WORD_DATA,
    .delay = 10,
  },
};