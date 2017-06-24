/* Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#ifndef _MDNIE_LITE_TUNING_DATA_CHAGALL_
#define _MDNIE_LITE_TUNING_DATA_CHAGALL_

#define MDNIE_GLOBAL_COMMAND	0xB0

#define MDNIE_GLOBAL_FIRST	0x82
#define MDNIE_TUNE_FIRST_SIZE	83
#define MDNIE_GLOBAL_SECOND	0xD4
#define MDNIE_TUNE_SECOND_SIZE	55

#define MDNIE_COLOR_BLINDE_OFFSET 29
#define MDNIE_COLOR_BLINDE_CMD	19

#define BITSHIFT_SCR		2
#define ADDRESS_SCR_WHITE_RED   47
#define ADDRESS_SCR_WHITE_GREEN 49
#define ADDRESS_SCR_WHITE_BLUE  51

#define coordinate_data_size 6
#define scr_wr_addr 47

#define F1(x,y) ((y)-((164*(x))/151)+8)
#define F2(x,y) ((y)-((70*(x))/67)-7)
#define F3(x,y) ((y)+((181*(x))/35)-18852)
#define F4(x,y) ((y)+((157*(x))/52)-12055)

static char coordinate_data[][coordinate_data_size] = {
	{0xff, 0xff, 0xff}, /* dummy */
	{0xff, 0xfa, 0xfa}, /* Tune_1 */
	{0xff, 0xfb, 0xfe}, /* Tune_2 */
	{0xfc, 0xfb, 0xff}, /* Tune_3 */
	{0xff, 0xfe, 0xfb}, /* Tune_4 */
	{0xff, 0xff, 0xff}, /* Tune_5 */
	{0xfb, 0xfc, 0xff}, /* Tune_6 */
	{0xfc, 0xff, 0xfa}, /* Tune_7 */
	{0xfb, 0xff, 0xfb}, /* Tune_8 */
	{0xfb, 0xff, 0xff}, /* Tune_9 */
};

////////////////// UI /// /////////////////////
static unsigned char SCREEN_CURTAIN_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x00, // sharpen_weight 00000000
	0xff, // max_plus 11111111
	0xff, // max_plus 111  max_minus 11111
	0xfd, // max_minus 111111  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char SCREEN_CURTAIN_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x30, // up_distance 001100  down_distance 00
	0x30, // down_distance 001100  right_distance 00
	0x30, // right_distance 001100  left_distance 00
	0x30, // left_distance 001100  up_divided_distance 00
	0x2a, // up_divided_distance 00101010
	0xaa, // up_divided_distance 10101010
	0xc2, // up_divided_distance 11  down_divided_distance 000010
	0xaa, // down_divided_distance 10101010
	0xac, // down_divided_distance 101011 right_divided_distance 00
	0x2a, // right_divided_distance 00101010
	0xaa, // right_divided_distance 10101010
	0xc2, // right_divided_distance 11  left_divided_distance 000010
	0xaa, // left_divided_distance 10101010
	0xaf, // left_divided_distance 101011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x00, // ascr_Cr 000000  ascr_Rr 00
	0x00, // ascr_Rr 000000  ascr_Cg 00
	0x00, // ascr_Cg 000000  ascr_Rg 00
	0x00, // ascr_Rg 000000  ascr_Cb 00
	0x00, // ascr_Cb 000000  ascr_Rb 00
	0x00, // ascr_Rb 000000  ascr_Mr 00
	0x00, // ascr_Mr 000000  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x00, // ascr_Mg 000000  ascr_Gg 00
	0x00, // ascr_Gg 000000  ascr_Mb 00
	0x00, // ascr_Mb 000000  ascr_Gb 00
	0x00, // ascr_Gb 000000  ascr_Yr 00
	0x00, // ascr_Yr 000000  ascr_Br 00
	0x00, // ascr_Br 000000  ascr_Yg 00
	0x00, // ascr_Yg 000000  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x00, // ascr_Yb 000000  ascr_Bb 00
	0x00, // ascr_Bb 000000  ascr_Wr 00
	0x00, // ascr_Wr 000000  ascr_Kr 00
	0x00, // ascr_Kr 000000  ascr_Wg 00
	0x00, // ascr_Wg 000000  ascr_Kg 00
	0x00, // ascr_Kg 000000  ascr_Wb 00
	0x00, // ascr_Wb 000000  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char STANDARD_UI_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x93, // min_ref_offset 10  gamma 0  cs 1 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x10, // sharpen_weight 00010000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char STANDARD_UI_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xf3, // ascr_Rr 111100  ascr_Cg 11
	0xe8, // ascr_Cg 111010  ascr_Rg 00
	0x4b, // ascr_Rg 010010  ascr_Cb 11
	0xd4, // ascr_Cb 110101  ascr_Rb 00
	0x7b, // ascr_Rb 011110  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0xa3, // ascr_Mg 101000  ascr_Gg 11
	0xe3, // ascr_Gg 111000  ascr_Mb 11
	0x98, // ascr_Mb 100110  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xf0, // ascr_Yr 111100  ascr_Br 00
	0xb7, // ascr_Br 101101  ascr_Yg 11
	0xdc, // ascr_Yg 110111  ascr_Bg 00
	0x70, // ascr_Bg 011100  ascr_Yb 01
	0xf3, // ascr_Yb 111100  ascr_Bb 11
	0xe3, // ascr_Bb 111000  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xe0, // ascr_Wg 111000  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xb8, // ascr_Wb 101110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char NATURAL_UI_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x93, // min_ref_offset 10  gamma 0  cs 1 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x10, // sharpen_weight 00010000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char NATURAL_UI_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfe, // ascr_skin_Wb 111111  ascr_Cr 10
	0x2b, // ascr_Cr 001010  ascr_Rr 11
	0xa3, // ascr_Rr 101000  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x7b, // ascr_Rg 011110  ascr_Cb 11
	0xc8, // ascr_Cb 110010  ascr_Rb 00
	0x9b, // ascr_Rb 100110  ascr_Mr 11
	0xcd, // ascr_Mr 110011  ascr_Gr 01
	0xec, // ascr_Gr 111011  ascr_Mg 00
	0xbb, // ascr_Mg 101110  ascr_Gg 11
	0xf3, // ascr_Gg 111100  ascr_Mb 11
	0xd0, // ascr_Mb 110100  ascr_Gb 00
	0xeb, // ascr_Gb 111101  ascr_Yr 11
	0xe8, // ascr_Yr 111010  ascr_Br 00
	0xb7, // ascr_Br 101101  ascr_Yg 11
	0xd4, // ascr_Yg 110101  ascr_Bg 00
	0x65, // ascr_Bg 011001  ascr_Yb 01
	0x1b, // ascr_Yb 000110  ascr_Bb 11
	0xc7, // ascr_Bb 110001  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xe0, // ascr_Wg 111000  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xb8, // ascr_Wb 101110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char DYNAMIC_UI_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0xb3, // min_ref_offset 10  gamma 1  cs 1 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x14, // curve_1_a
	0x00, // curve_2_b
	0x14, // curve_2_a
	0x00, // curve_3_b
	0x14, // curve_3_a
	0x00, // curve_4_b
	0x14, // curve_4_a
	0x03, // curve_5_b
	0x9a, // curve_5_a
	0x03, // curve_6_b
	0x9a, // curve_6_a
	0x03, // curve_7_b
	0x9a, // curve_7_a
	0x03, // curve_8_b
	0x9a, // curve_8_a
	0x07, // curve_9_b
	0x9e, // curve_9_a
	0x07, // curve10_b
	0x9e, // curve10_a
	0x07, // curve11_b
	0x9e, // curve11_a
	0x07, // curve12_b
	0x9e, // curve12_a
	0x0a, // curve13_b
	0xa0, // curve13_a
	0x0a, // curve14_b
	0xa0, // curve14_a
	0x0a, // curve15_b
	0xa0, // curve15_a
	0x0a, // curve16_b
	0xa0, // curve16_a
	0x16, // curve17_b
	0xa6, // curve17_a
	0x16, // curve18_b
	0xa6, // curve18_a
	0x16, // curve19_b
	0xa6, // curve19_a
	0x16, // curve20_b
	0xa6, // curve20_a
	0x05, // curve21_b
	0x21, // curve21_a
	0x0b, // curve22_b
	0x20, // curve22_a
	0x87, // curve23_b
	0x0f, // curve23_a
	0x00, // curve24_b
	0xFF, // curve24_a
};

static unsigned char DYNAMIC_UI_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x85, // ascr_strength 10000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0xdc, // up_distance 110111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x65, // right_distance 011001  left_distance 01
	0x1c, // left_distance 000111  up_divided_distance 00
	0x09, // up_divided_distance 00001001
	0x4f, // up_divided_distance 01001111
	0x40, // up_divided_distance 01  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0x73, // left_divided_distance 01110011
	0x63, // left_divided_distance 011000  ascr_skin_Rr 11
	0xfd, // ascr_skin_Rr 111111  ascr_skin_Rg 01
	0x41, // ascr_skin_Rg 010000  ascr_skin_Rb 01
	0x83, // ascr_skin_Rb 100000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xe4, // ascr_Cg 111001  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xfc, // ascr_Wg 111111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char MOVIE_UI_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char MOVIE_UI_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xdc, // ascr_Wg 110111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0x98, // ascr_Wb 100110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char AUTO_UI_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x15, // sharpen_weight 00010101
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char AUTO_UI_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xe4, // ascr_Cg 111001  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xfc, // ascr_Wg 111111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

////////////////// GALLERY /////////////////////
static unsigned char STANDARD_GALLERY_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x9b, // min_ref_offset 10  gamma 0  cs 1 sharpen 1 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x10, // sharpen_weight 00010000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char STANDARD_GALLERY_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xf3, // ascr_Rr 111100  ascr_Cg 11
	0xe8, // ascr_Cg 111010  ascr_Rg 00
	0x4b, // ascr_Rg 010010  ascr_Cb 11
	0xd4, // ascr_Cb 110101  ascr_Rb 00
	0x7b, // ascr_Rb 011110  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0xa3, // ascr_Mg 101000  ascr_Gg 11
	0xe3, // ascr_Gg 111000  ascr_Mb 11
	0x98, // ascr_Mb 100110  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xf0, // ascr_Yr 111100  ascr_Br 00
	0xb7, // ascr_Br 101101  ascr_Yg 11
	0xdc, // ascr_Yg 110111  ascr_Bg 00
	0x70, // ascr_Bg 011100  ascr_Yb 01
	0xf3, // ascr_Yb 111100  ascr_Bb 11
	0xe3, // ascr_Bb 111000  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xe0, // ascr_Wg 111000  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xb8, // ascr_Wb 101110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char NATURAL_GALLERY_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x9b, // min_ref_offset 10  gamma 0  cs 1 sharpen 1 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x10, // sharpen_weight 00010000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char NATURAL_GALLERY_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfe, // ascr_skin_Wb 111111  ascr_Cr 10
	0x2b, // ascr_Cr 001010  ascr_Rr 11
	0xa3, // ascr_Rr 101000  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x7b, // ascr_Rg 011110  ascr_Cb 11
	0xc8, // ascr_Cb 110010  ascr_Rb 00
	0x9b, // ascr_Rb 100110  ascr_Mr 11
	0xcd, // ascr_Mr 110011  ascr_Gr 01
	0xec, // ascr_Gr 111011  ascr_Mg 00
	0xbb, // ascr_Mg 101110  ascr_Gg 11
	0xf3, // ascr_Gg 111100  ascr_Mb 11
	0xd0, // ascr_Mb 110100  ascr_Gb 00
	0xeb, // ascr_Gb 111101  ascr_Yr 11
	0xe8, // ascr_Yr 111010  ascr_Br 00
	0xb7, // ascr_Br 101101  ascr_Yg 11
	0xd4, // ascr_Yg 110101  ascr_Bg 00
	0x65, // ascr_Bg 011001  ascr_Yb 01
	0x1b, // ascr_Yb 000110  ascr_Bb 11
	0xc7, // ascr_Bb 110001  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xe0, // ascr_Wg 111000  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xb8, // ascr_Wb 101110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char DYNAMIC_GALLERY_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0xbb, // min_ref_offset 10  gamma 1  cs 1 sharpen 1 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x20, // sharpen_weight 00100000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x14, // curve_1_a
	0x00, // curve_2_b
	0x14, // curve_2_a
	0x00, // curve_3_b
	0x14, // curve_3_a
	0x00, // curve_4_b
	0x14, // curve_4_a
	0x03, // curve_5_b
	0x9a, // curve_5_a
	0x03, // curve_6_b
	0x9a, // curve_6_a
	0x03, // curve_7_b
	0x9a, // curve_7_a
	0x03, // curve_8_b
	0x9a, // curve_8_a
	0x07, // curve_9_b
	0x9e, // curve_9_a
	0x07, // curve10_b
	0x9e, // curve10_a
	0x07, // curve11_b
	0x9e, // curve11_a
	0x07, // curve12_b
	0x9e, // curve12_a
	0x0a, // curve13_b
	0xa0, // curve13_a
	0x0a, // curve14_b
	0xa0, // curve14_a
	0x0a, // curve15_b
	0xa0, // curve15_a
	0x0a, // curve16_b
	0xa0, // curve16_a
	0x16, // curve17_b
	0xa6, // curve17_a
	0x16, // curve18_b
	0xa6, // curve18_a
	0x16, // curve19_b
	0xa6, // curve19_a
	0x16, // curve20_b
	0xa6, // curve20_a
	0x05, // curve21_b
	0x21, // curve21_a
	0x0b, // curve22_b
	0x20, // curve22_a
	0x87, // curve23_b
	0x0f, // curve23_a
	0x00, // curve24_b
	0xFF, // curve24_a
};

static unsigned char DYNAMIC_GALLERY_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x85, // ascr_strength 10000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0xdc, // up_distance 110111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x65, // right_distance 011001  left_distance 01
	0x1c, // left_distance 000111  up_divided_distance 00
	0x09, // up_divided_distance 00001001
	0x4f, // up_divided_distance 01001111
	0x40, // up_divided_distance 01  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0x73, // left_divided_distance 01110011
	0x63, // left_divided_distance 011000  ascr_skin_Rr 11
	0xfd, // ascr_skin_Rr 111111  ascr_skin_Rg 01
	0x41, // ascr_skin_Rg 010000  ascr_skin_Rb 01
	0x83, // ascr_skin_Rb 100000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xe4, // ascr_Cg 111001  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xfc, // ascr_Wg 111111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char MOVIE_GALLERY_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char MOVIE_GALLERY_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xdc, // ascr_Wg 110111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0x98, // ascr_Wb 100110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char AUTO_GALLERY_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x8b, // min_ref_offset 10  gamma 0  cs 0 sharpen 1 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x15, // sharpen_weight 00010101
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char AUTO_GALLERY_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x85, // ascr_strength 10000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfd, // ascr_skin_Rr 111111  ascr_skin_Rg 01
	0xa1, // ascr_skin_Rg 101000  ascr_skin_Rb 01
	0xc3, // ascr_skin_Rb 110000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xe3, // ascr_skin_Wg 111000  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xe4, // ascr_Cg 111001  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xfc, // ascr_Wg 111111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

////////////////// VIDEO /////////////////////
static unsigned char STANDARD_VIDEO_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x9b, // min_ref_offset 10  gamma 0  cs 1 sharpen 1 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x10, // sharpen_weight 00010000
	0x08, // max_plus 00001000
	0x01, // max_plus 000  max_minus 00001
	0x01, // max_minus 000000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char STANDARD_VIDEO_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xf3, // ascr_Rr 111100  ascr_Cg 11
	0xe8, // ascr_Cg 111010  ascr_Rg 00
	0x4b, // ascr_Rg 010010  ascr_Cb 11
	0xd4, // ascr_Cb 110101  ascr_Rb 00
	0x7b, // ascr_Rb 011110  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0xa3, // ascr_Mg 101000  ascr_Gg 11
	0xe3, // ascr_Gg 111000  ascr_Mb 11
	0x98, // ascr_Mb 100110  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xf0, // ascr_Yr 111100  ascr_Br 00
	0xb7, // ascr_Br 101101  ascr_Yg 11
	0xdc, // ascr_Yg 110111  ascr_Bg 00
	0x70, // ascr_Bg 011100  ascr_Yb 01
	0xf3, // ascr_Yb 111100  ascr_Bb 11
	0xe3, // ascr_Bb 111000  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xe0, // ascr_Wg 111000  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xb8, // ascr_Wb 101110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char NATURAL_VIDEO_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x9b, // min_ref_offset 10  gamma 0  cs 1 sharpen 1 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x10, // sharpen_weight 00010000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char NATURAL_VIDEO_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfe, // ascr_skin_Wb 111111  ascr_Cr 10
	0x2b, // ascr_Cr 001010  ascr_Rr 11
	0xa3, // ascr_Rr 101000  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x7b, // ascr_Rg 011110  ascr_Cb 11
	0xc8, // ascr_Cb 110010  ascr_Rb 00
	0x9b, // ascr_Rb 100110  ascr_Mr 11
	0xcd, // ascr_Mr 110011  ascr_Gr 01
	0xec, // ascr_Gr 111011  ascr_Mg 00
	0xbb, // ascr_Mg 101110  ascr_Gg 11
	0xf3, // ascr_Gg 111100  ascr_Mb 11
	0xd0, // ascr_Mb 110100  ascr_Gb 00
	0xeb, // ascr_Gb 111101  ascr_Yr 11
	0xe8, // ascr_Yr 111010  ascr_Br 00
	0xb7, // ascr_Br 101101  ascr_Yg 11
	0xd4, // ascr_Yg 110101  ascr_Bg 00
	0x65, // ascr_Bg 011001  ascr_Yb 01
	0x1b, // ascr_Yb 000110  ascr_Bb 11
	0xc7, // ascr_Bb 110001  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xe0, // ascr_Wg 111000  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xb8, // ascr_Wb 101110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char DYNAMIC_VIDEO_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0xbb, // min_ref_offset 10  gamma 1  cs 1 sharpen 1 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x20, // sharpen_weight 00100000
	0x08, // max_plus 00001000
	0x01, // max_plus 000  max_minus 00001
	0x01, // max_minus 000000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x14, // curve_1_a
	0x00, // curve_2_b
	0x14, // curve_2_a
	0x00, // curve_3_b
	0x14, // curve_3_a
	0x00, // curve_4_b
	0x14, // curve_4_a
	0x03, // curve_5_b
	0x9a, // curve_5_a
	0x03, // curve_6_b
	0x9a, // curve_6_a
	0x03, // curve_7_b
	0x9a, // curve_7_a
	0x03, // curve_8_b
	0x9a, // curve_8_a
	0x07, // curve_9_b
	0x9e, // curve_9_a
	0x07, // curve10_b
	0x9e, // curve10_a
	0x07, // curve11_b
	0x9e, // curve11_a
	0x07, // curve12_b
	0x9e, // curve12_a
	0x0a, // curve13_b
	0xa0, // curve13_a
	0x0a, // curve14_b
	0xa0, // curve14_a
	0x0a, // curve15_b
	0xa0, // curve15_a
	0x0a, // curve16_b
	0xa0, // curve16_a
	0x16, // curve17_b
	0xa6, // curve17_a
	0x16, // curve18_b
	0xa6, // curve18_a
	0x16, // curve19_b
	0xa6, // curve19_a
	0x16, // curve20_b
	0xa6, // curve20_a
	0x05, // curve21_b
	0x21, // curve21_a
	0x0b, // curve22_b
	0x20, // curve22_a
	0x87, // curve23_b
	0x0f, // curve23_a
	0x00, // curve24_b
	0xFF, // curve24_a
};

static unsigned char DYNAMIC_VIDEO_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x85, // ascr_strength 10000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0xdc, // up_distance 110111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x65, // right_distance 011001  left_distance 01
	0x1c, // left_distance 000111  up_divided_distance 00
	0x09, // up_divided_distance 00001001
	0x4f, // up_divided_distance 01001111
	0x40, // up_divided_distance 01  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0x73, // left_divided_distance 01110011
	0x63, // left_divided_distance 011000  ascr_skin_Rr 11
	0xfd, // ascr_skin_Rr 111111  ascr_skin_Rg 01
	0xd1, // ascr_skin_Rg 110100  ascr_skin_Rb 01
	0xe3, // ascr_skin_Rb 111000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xd3, // ascr_skin_Wg 110100  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xe4, // ascr_Cg 111001  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xfc, // ascr_Wg 111111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char MOVIE_VIDEO_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char MOVIE_VIDEO_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xdc, // ascr_Wg 110111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0x98, // ascr_Wb 100110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char AUTO_VIDEO_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0xbb, // min_ref_offset 10  gamma 1  cs 1 sharpen 1 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x20, // sharpen_weight 00100000
	0x08, // max_plus 00001000
	0x01, // max_plus 000  max_minus 00001
	0x01, // max_minus 000000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x14, // curve_1_a
	0x00, // curve_2_b
	0x14, // curve_2_a
	0x00, // curve_3_b
	0x14, // curve_3_a
	0x00, // curve_4_b
	0x14, // curve_4_a
	0x03, // curve_5_b
	0x9a, // curve_5_a
	0x03, // curve_6_b
	0x9a, // curve_6_a
	0x03, // curve_7_b
	0x9a, // curve_7_a
	0x03, // curve_8_b
	0x9a, // curve_8_a
	0x07, // curve_9_b
	0x9e, // curve_9_a
	0x07, // curve10_b
	0x9e, // curve10_a
	0x07, // curve11_b
	0x9e, // curve11_a
	0x07, // curve12_b
	0x9e, // curve12_a
	0x0a, // curve13_b
	0xa0, // curve13_a
	0x0a, // curve14_b
	0xa0, // curve14_a
	0x0a, // curve15_b
	0xa0, // curve15_a
	0x0a, // curve16_b
	0xa0, // curve16_a
	0x16, // curve17_b
	0xa6, // curve17_a
	0x16, // curve18_b
	0xa6, // curve18_a
	0x16, // curve19_b
	0xa6, // curve19_a
	0x16, // curve20_b
	0xa6, // curve20_a
	0x05, // curve21_b
	0x21, // curve21_a
	0x0b, // curve22_b
	0x20, // curve22_a
	0x87, // curve23_b
	0x0f, // curve23_a
	0x00, // curve24_b
	0xFF, // curve24_a
};

static unsigned char AUTO_VIDEO_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x85, // ascr_strength 10000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0xdc, // up_distance 110111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x65, // right_distance 011001  left_distance 01
	0x1c, // left_distance 000111  up_divided_distance 00
	0x09, // up_divided_distance 00001001
	0x4f, // up_divided_distance 01001111
	0x40, // up_divided_distance 01  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0x73, // left_divided_distance 01110011
	0x63, // left_divided_distance 011000  ascr_skin_Rr 11
	0xfd, // ascr_skin_Rr 111111  ascr_skin_Rg 01
	0xd1, // ascr_skin_Rg 110100  ascr_skin_Rb 01
	0xe3, // ascr_skin_Rb 111000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xd3, // ascr_skin_Wg 110100  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xe4, // ascr_Cg 111001  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xfc, // ascr_Wg 111111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

////////////////// VT /////////////////////
static unsigned char STANDARD_VT_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x9b, // min_ref_offset 10  gamma 0  cs 1 sharpen 1 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x10, // sharpen_weight 00010000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char STANDARD_VT_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xf3, // ascr_Rr 111100  ascr_Cg 11
	0xe8, // ascr_Cg 111010  ascr_Rg 00
	0x4b, // ascr_Rg 010010  ascr_Cb 11
	0xd4, // ascr_Cb 110101  ascr_Rb 00
	0x7b, // ascr_Rb 011110  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0xa3, // ascr_Mg 101000  ascr_Gg 11
	0xe3, // ascr_Gg 111000  ascr_Mb 11
	0x98, // ascr_Mb 100110  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xf0, // ascr_Yr 111100  ascr_Br 00
	0xb7, // ascr_Br 101101  ascr_Yg 11
	0xdc, // ascr_Yg 110111  ascr_Bg 00
	0x70, // ascr_Bg 011100  ascr_Yb 01
	0xf3, // ascr_Yb 111100  ascr_Bb 11
	0xe3, // ascr_Bb 111000  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xe0, // ascr_Wg 111000  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xb8, // ascr_Wb 101110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char NATURAL_VT_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x9b, // min_ref_offset 10  gamma 0  cs 1 sharpen 1 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x10, // sharpen_weight 00010000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char NATURAL_VT_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfe, // ascr_skin_Wb 111111  ascr_Cr 10
	0x2b, // ascr_Cr 001010  ascr_Rr 11
	0xa3, // ascr_Rr 101000  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x7b, // ascr_Rg 011110  ascr_Cb 11
	0xc8, // ascr_Cb 110010  ascr_Rb 00
	0x9b, // ascr_Rb 100110  ascr_Mr 11
	0xcd, // ascr_Mr 110011  ascr_Gr 01
	0xec, // ascr_Gr 111011  ascr_Mg 00
	0xbb, // ascr_Mg 101110  ascr_Gg 11
	0xf3, // ascr_Gg 111100  ascr_Mb 11
	0xd0, // ascr_Mb 110100  ascr_Gb 00
	0xeb, // ascr_Gb 111101  ascr_Yr 11
	0xe8, // ascr_Yr 111010  ascr_Br 00
	0xb7, // ascr_Br 101101  ascr_Yg 11
	0xd4, // ascr_Yg 110101  ascr_Bg 00
	0x65, // ascr_Bg 011001  ascr_Yb 01
	0x1b, // ascr_Yb 000110  ascr_Bb 11
	0xc7, // ascr_Bb 110001  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xe0, // ascr_Wg 111000  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xb8, // ascr_Wb 101110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char DYNAMIC_VT_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0xbb, // min_ref_offset 10  gamma 1  cs 1 sharpen 1 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x20, // sharpen_weight 00100000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x14, // curve_1_a
	0x00, // curve_2_b
	0x14, // curve_2_a
	0x00, // curve_3_b
	0x14, // curve_3_a
	0x00, // curve_4_b
	0x14, // curve_4_a
	0x03, // curve_5_b
	0x9a, // curve_5_a
	0x03, // curve_6_b
	0x9a, // curve_6_a
	0x03, // curve_7_b
	0x9a, // curve_7_a
	0x03, // curve_8_b
	0x9a, // curve_8_a
	0x07, // curve_9_b
	0x9e, // curve_9_a
	0x07, // curve10_b
	0x9e, // curve10_a
	0x07, // curve11_b
	0x9e, // curve11_a
	0x07, // curve12_b
	0x9e, // curve12_a
	0x0a, // curve13_b
	0xa0, // curve13_a
	0x0a, // curve14_b
	0xa0, // curve14_a
	0x0a, // curve15_b
	0xa0, // curve15_a
	0x0a, // curve16_b
	0xa0, // curve16_a
	0x16, // curve17_b
	0xa6, // curve17_a
	0x16, // curve18_b
	0xa6, // curve18_a
	0x16, // curve19_b
	0xa6, // curve19_a
	0x16, // curve20_b
	0xa6, // curve20_a
	0x05, // curve21_b
	0x21, // curve21_a
	0x0b, // curve22_b
	0x20, // curve22_a
	0x87, // curve23_b
	0x0f, // curve23_a
	0x00, // curve24_b
	0xFF, // curve24_a
};

static unsigned char DYNAMIC_VT_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x85, // ascr_strength 10000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0xdc, // up_distance 110111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x65, // right_distance 011001  left_distance 01
	0x1c, // left_distance 000111  up_divided_distance 00
	0x09, // up_divided_distance 00001001
	0x4f, // up_divided_distance 01001111
	0x40, // up_divided_distance 01  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0x73, // left_divided_distance 01110011
	0x63, // left_divided_distance 011000  ascr_skin_Rr 11
	0xfd, // ascr_skin_Rr 111111  ascr_skin_Rg 01
	0x41, // ascr_skin_Rg 010000  ascr_skin_Rb 01
	0x83, // ascr_skin_Rb 100000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xe4, // ascr_Cg 111001  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xfc, // ascr_Wg 111111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char MOVIE_VT_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char MOVIE_VT_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xdc, // ascr_Wg 110111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0x98, // ascr_Wb 100110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char AUTO_VT_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x8b, // min_ref_offset 10  gamma 0  cs 0 sharpen 1 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x15, // sharpen_weight 00010101
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char AUTO_VT_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xe4, // ascr_Cg 111001  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xfc, // ascr_Wg 111111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

////////////////// CAMERA /////////////////////
static unsigned char CAMERA_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char CAMERA_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xfc, // ascr_Wg 111111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char AUTO_CAMERA_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char AUTO_CAMERA_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x85, // ascr_strength 10000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfd, // ascr_skin_Rr 111111  ascr_skin_Rg 01
	0xa1, // ascr_skin_Rg 101000  ascr_skin_Rb 01
	0xc3, // ascr_skin_Rb 110000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xe3, // ascr_skin_Wg 111000  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xfc, // ascr_Wg 111111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char NEGATIVE_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x00, // sharpen_weight 00000000
	0xff, // max_plus 11111111
	0xff, // max_plus 111  max_minus 11111
	0xfd, // max_minus 111111  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char NEGATIVE_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x30, // up_distance 001100  down_distance 00
	0x30, // down_distance 001100  right_distance 00
	0x30, // right_distance 001100  left_distance 00
	0x30, // left_distance 001100  up_divided_distance 00
	0x2a, // up_divided_distance 00101010
	0xaa, // up_divided_distance 10101010
	0xc2, // up_divided_distance 11  down_divided_distance 000010
	0xaa, // down_divided_distance 10101010
	0xac, // down_divided_distance 101011 right_divided_distance 00
	0x2a, // right_divided_distance 00101010
	0xaa, // right_divided_distance 10101010
	0xc2, // right_divided_distance 11  left_divided_distance 000010
	0xaa, // left_divided_distance 10101010
	0xac, // left_divided_distance 101011  ascr_skin_Rr 00
	0x03, // ascr_skin_Rr 000000  ascr_skin_Rg 11
	0xff, // ascr_skin_Rg 111111  ascr_skin_Rb 11
	0xfc, // ascr_skin_Rb 111111  ascr_skin_Yr 00
	0x00, // ascr_skin_Yr 000000  ascr_skin_Yg 00
	0x03, // ascr_skin_Yg 000000  ascr_skin_Yb 11
	0xfc, // ascr_skin_Yb 111111  ascr_skin_Mr 00
	0x03, // ascr_skin_Mr 000000  ascr_skin_Mg 11
	0xfc, // ascr_skin_Mg 111111  ascr_skin_Mb 00
	0x00, // ascr_skin_Mb 000000  ascr_skin_Wr 00
	0x00, // ascr_skin_Wr 000000  ascr_skin_Wg 00
	0x00, // ascr_skin_Wg 000000  ascr_skin_Wb 00
	0x03, // ascr_skin_Wb 000000  ascr_Cr 11
	0xfc, // ascr_Cr 111111  ascr_Rr 00
	0x00, // ascr_Rr 000000  ascr_Cg 00
	0x03, // ascr_Cg 000000  ascr_Rg 11
	0xfc, // ascr_Rg 111111  ascr_Cb 00
	0x03, // ascr_Cb 000000  ascr_Rb 11
	0xfc, // ascr_Rb 111111  ascr_Mr 00
	0x03, // ascr_Mr 000000  ascr_Gr 11
	0xff, // ascr_Gr 111111  ascr_Mg 11
	0xfc, // ascr_Mg 111111  ascr_Gg 00
	0x00, // ascr_Gg 000000  ascr_Mb 00
	0x03, // ascr_Mb 000000  ascr_Gb 11
	0xfc, // ascr_Gb 111111  ascr_Yr 00
	0x03, // ascr_Yr 000000  ascr_Br 11
	0xfc, // ascr_Br 111111  ascr_Yg 00
	0x03, // ascr_Yg 000000  ascr_Bg 11
	0xff, // ascr_Bg 111111  ascr_Yb 11
	0xfc, // ascr_Yb 111111  ascr_Bb 00
	0x00, // ascr_Bb 000000  ascr_Wr 00
	0x03, // ascr_Wr 000000  ascr_Kr 11
	0xfc, // ascr_Kr 111111  ascr_Wg 00
	0x03, // ascr_Wg 000000  ascr_Kg 11
	0xfc, // ascr_Kg 111111  ascr_Wb 00
	0x03, // ascr_Wb 000000  ascr_Kb 11
	0xfc, // ascr_Kb 111111  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char COLOR_BLIND_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x00, // sharpen_weight 00000000
	0xff, // max_plus 11111111
	0xff, // max_plus 111  max_minus 11111
	0xfd, // max_minus 111111  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char COLOR_BLIND_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x30, // up_distance 001100  down_distance 00
	0x30, // down_distance 001100  right_distance 00
	0x30, // right_distance 001100  left_distance 00
	0x30, // left_distance 001100  up_divided_distance 00
	0x2a, // up_divided_distance 00101010
	0xaa, // up_divided_distance 10101010
	0xc2, // up_divided_distance 11  down_divided_distance 000010
	0xaa, // down_divided_distance 10101010
	0xac, // down_divided_distance 101011 right_divided_distance 00
	0x2a, // right_divided_distance 00101010
	0xaa, // right_divided_distance 10101010
	0xc2, // right_divided_distance 11  left_divided_distance 000010
	0xaa, // left_divided_distance 10101010
	0xaf, // left_divided_distance 101011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xfc, // ascr_Wg 111111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

////////////////// BROWSER /////////////////////
static unsigned char STANDARD_BROWSER_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x93, // min_ref_offset 10  gamma 0  cs 1 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x10, // sharpen_weight 00010000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char STANDARD_BROWSER_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xf3, // ascr_Rr 111100  ascr_Cg 11
	0xe8, // ascr_Cg 111010  ascr_Rg 00
	0x4b, // ascr_Rg 010010  ascr_Cb 11
	0xd4, // ascr_Cb 110101  ascr_Rb 00
	0x7b, // ascr_Rb 011110  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0xa3, // ascr_Mg 101000  ascr_Gg 11
	0xe3, // ascr_Gg 111000  ascr_Mb 11
	0x98, // ascr_Mb 100110  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xf0, // ascr_Yr 111100  ascr_Br 00
	0xb7, // ascr_Br 101101  ascr_Yg 11
	0xdc, // ascr_Yg 110111  ascr_Bg 00
	0x70, // ascr_Bg 011100  ascr_Yb 01
	0xf3, // ascr_Yb 111100  ascr_Bb 11
	0xe3, // ascr_Bb 111000  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xe0, // ascr_Wg 111000  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xb8, // ascr_Wb 101110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char NATURAL_BROWSER_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x93, // min_ref_offset 10  gamma 0  cs 1 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x10, // sharpen_weight 00010000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char NATURAL_BROWSER_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfe, // ascr_skin_Wb 111111  ascr_Cr 10
	0x2b, // ascr_Cr 001010  ascr_Rr 11
	0xa3, // ascr_Rr 101000  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x7b, // ascr_Rg 011110  ascr_Cb 11
	0xc8, // ascr_Cb 110010  ascr_Rb 00
	0x9b, // ascr_Rb 100110  ascr_Mr 11
	0xcd, // ascr_Mr 110011  ascr_Gr 01
	0xec, // ascr_Gr 111011  ascr_Mg 00
	0xbb, // ascr_Mg 101110  ascr_Gg 11
	0xf3, // ascr_Gg 111100  ascr_Mb 11
	0xd0, // ascr_Mb 110100  ascr_Gb 00
	0xeb, // ascr_Gb 111101  ascr_Yr 11
	0xe8, // ascr_Yr 111010  ascr_Br 00
	0xb7, // ascr_Br 101101  ascr_Yg 11
	0xd4, // ascr_Yg 110101  ascr_Bg 00
	0x65, // ascr_Bg 011001  ascr_Yb 01
	0x1b, // ascr_Yb 000110  ascr_Bb 11
	0xc7, // ascr_Bb 110001  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xe0, // ascr_Wg 111000  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xb8, // ascr_Wb 101110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char DYNAMIC_BROWSER_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x03, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0xb3, // min_ref_offset 10  gamma 1  cs 1 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x50, // sharpen_weight 01010000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x20, // cs_gain 00100000
	0x00, // curve_1_b
	0x14, // curve_1_a
	0x00, // curve_2_b
	0x14, // curve_2_a
	0x00, // curve_3_b
	0x14, // curve_3_a
	0x00, // curve_4_b
	0x14, // curve_4_a
	0x03, // curve_5_b
	0x9a, // curve_5_a
	0x03, // curve_6_b
	0x9a, // curve_6_a
	0x03, // curve_7_b
	0x9a, // curve_7_a
	0x03, // curve_8_b
	0x9a, // curve_8_a
	0x07, // curve_9_b
	0x9e, // curve_9_a
	0x07, // curve10_b
	0x9e, // curve10_a
	0x07, // curve11_b
	0x9e, // curve11_a
	0x07, // curve12_b
	0x9e, // curve12_a
	0x0a, // curve13_b
	0xa0, // curve13_a
	0x0a, // curve14_b
	0xa0, // curve14_a
	0x0a, // curve15_b
	0xa0, // curve15_a
	0x0a, // curve16_b
	0xa0, // curve16_a
	0x16, // curve17_b
	0xa6, // curve17_a
	0x16, // curve18_b
	0xa6, // curve18_a
	0x16, // curve19_b
	0xa6, // curve19_a
	0x16, // curve20_b
	0xa6, // curve20_a
	0x05, // curve21_b
	0x21, // curve21_a
	0x0b, // curve22_b
	0x20, // curve22_a
	0x87, // curve23_b
	0x0f, // curve23_a
	0x00, // curve24_b
	0xFF, // curve24_a
};

static unsigned char DYNAMIC_BROWSER_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x85, // ascr_strength 10000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0xdc, // up_distance 110111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x65, // right_distance 011001  left_distance 01
	0x1c, // left_distance 000111  up_divided_distance 00
	0x09, // up_divided_distance 00001001
	0x4f, // up_divided_distance 01001111
	0x40, // up_divided_distance 01  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0x73, // left_divided_distance 01110011
	0x63, // left_divided_distance 011000  ascr_skin_Rr 11
	0xfd, // ascr_skin_Rr 111111  ascr_skin_Rg 01
	0x41, // ascr_skin_Rg 010000  ascr_skin_Rb 01
	0x83, // ascr_skin_Rb 100000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xe4, // ascr_Cg 111001  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xfc, // ascr_Wg 111111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char MOVIE_BROWSER_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char MOVIE_BROWSER_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xdc, // ascr_Wg 110111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0x98, // ascr_Wb 100110  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char AUTO_BROWSER_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char AUTO_BROWSER_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x85, // ascr_strength 10000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfd, // ascr_skin_Rr 111111  ascr_skin_Rg 01
	0xa1, // ascr_skin_Rg 101000  ascr_skin_Rb 01
	0xc3, // ascr_skin_Rb 110000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xe3, // ascr_skin_Wg 111000  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xe4, // ascr_Cg 111001  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xfc, // ascr_Wg 111111  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

////////////////// eBOOK /////////////////////
static unsigned char AUTO_EBOOK_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char AUTO_EBOOK_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xd8, // ascr_Wg 110110  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xa0, // ascr_Wb 101000  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char DYNAMIC_EBOOK_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char DYNAMIC_EBOOK_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xd8, // ascr_Wg 110110  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xa0, // ascr_Wb 101000  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char STANDARD_EBOOK_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char STANDARD_EBOOK_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xd8, // ascr_Wg 110110  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xa0, // ascr_Wb 101000  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char NATURAL_EBOOK_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char NATURAL_EBOOK_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xd8, // ascr_Wg 110110  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xa0, // ascr_Wb 101000  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char MOVIE_EBOOK_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char MOVIE_EBOOK_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xd8, // ascr_Wg 110110  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xa0, // ascr_Wb 101000  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char AUTO_EMAIL_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x00, // data_width 00  lce_module 0  lcd_bypass 0  lcd_roi 00  algo_module 0  algo_bypass 0
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x62, // gain 110000 cgain 10
	0x48, // cgain 0100  scene_on 1  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0xb3, // illum_gain 10110011
	0x87, // ref_offset 10000111
	0x40, // ref_offset 0  ref_gain 1000000
	0x36, // ref_gain 00  hbsize 110  vbsize 110
	0xfa, // bth 11111010
	0x5a, // bin_size_ratio 0101101  dth 0
	0xe5, // dth 11  min_ref_offset 100101
	0x83, // min_ref_offset 10  gamma 0  cs 0 sharpen 0 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x00, // cs_gain 00000000
	0x00, // curve_1_b
	0x20, // curve_1_a
	0x00, // curve_2_b
	0x20, // curve_2_a
	0x00, // curve_3_b
	0x20, // curve_3_a
	0x00, // curve_4_b
	0x20, // curve_4_a
	0x00, // curve_5_b
	0x20, // curve_5_a
	0x00, // curve_6_b
	0x20, // curve_6_a
	0x00, // curve_7_b
	0x20, // curve_7_a
	0x00, // curve_8_b
	0x20, // curve_8_a
	0x00, // curve_9_b
	0x20, // curve_9_a
	0x00, // curve_10_b
	0x20, // curve_10_a
	0x00, // curve_11_b
	0x20, // curve_11_a
	0x00, // curve_12_b
	0x20, // curve_12_a
	0x00, // curve_13_b
	0x20, // curve_13_a
	0x00, // curve_14_b
	0x20, // curve_14_a
	0x00, // curve_15_b
	0x20, // curve_15_a
	0x00, // curve_16_b
	0x20, // curve_16_a
	0x00, // curve_17_b
	0x20, // curve_17_a
	0x00, // curve_18_b
	0x20, // curve_18_a
	0x00, // curve_19_b
	0x20, // curve_19_a
	0x00, // curve_20_b
	0x20, // curve_20_a
	0x00, // curve_21_b
	0x20, // curve_21_a
	0x00, // curve_22_b
	0x20, // curve_22_a
	0x00, // curve_23_b
	0x20, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char AUTO_EMAIL_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x05, // ascr_strength 00000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfc, // ascr_skin_Rr 111111  ascr_skin_Rg 00
	0x00, // ascr_skin_Rg 000000  ascr_skin_Rb 00
	0x03, // ascr_skin_Rb 000000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xe8, // ascr_Wg 111010  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xbc, // ascr_Wb 101111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char LOCAL_CE_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x33, // data_width 00  lce_module 1  lcd_bypass 1  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x17, // gain 000101 cgain 11
	0x00, // cgain 0000  scene_on 0  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0x90, // illum_gain 10010000
	0xdf, // ref_offset 11011111
	0xac, // ref_offset 1  ref_gain 0101100
	0x3f, // ref_gain 00  hbsize 111  vbsize 111
	0xfa, // bth 11111010
	0xfe, // bin_size_ratio 1111111  dth 0
	0x10, // dth 00  min_ref_offset 010000
	0x2b, // min_ref_offset 00  gamma 1  cs 0 sharpen 1 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x40, // cs_gain 01000000
	0x00, // curve_1_b
	0x6b, // curve_1_a
	0x03, // curve_2_b
	0x48, // curve_2_a
	0x08, // curve_3_b
	0x32, // curve_3_a
	0x08, // curve_4_b
	0x32, // curve_4_a
	0x08, // curve_5_b
	0x32, // curve_5_a
	0x08, // curve_6_b
	0x32, // curve_6_a
	0x08, // curve_7_b
	0x32, // curve_7_a
	0x10, // curve_8_b
	0x28, // curve_8_a
	0x10, // curve_9_b
	0x28, // curve_9_a
	0x10, // curve_10_b
	0x28, // curve_10_a
	0x10, // curve_11_b
	0x28, // curve_11_a
	0x10, // curve_12_b
	0x28, // curve_12_a
	0x19, // curve_13_b
	0x22, // curve_13_a
	0x49, // curve_14_b
	0xdf, // curve_14_a
	0x49, // curve_15_b
	0xdf, // curve_15_a
	0x49, // curve_16_b
	0xdf, // curve_16_a
	0x49, // curve_17_b
	0xdf, // curve_17_a
	0x50, // curve_18_b
	0x1c, // curve_18_a
	0x5b, // curve_19_b
	0x18, // curve_19_a
	0x6a, // curve_20_b
	0x14, // curve_20_a
	0x7a, // curve_21_b
	0x11, // curve_21_a
	0x87, // curve_22_b
	0x0f, // curve_22_a
	0x87, // curve_23_b
	0x0f, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char LOCAL_CE_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x85, // ascr_strength 10000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa4, // skin_cr 101001  up_distance 00
	0x5c, // up_distance 010111  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x64, // right_distance 011001  left_distance 00
	0x9c, // left_distance 100111  up_divided_distance 00
	0x16, // up_divided_distance 00010110
	0x42, // up_divided_distance 01000010
	0xc0, // up_divided_distance 11  down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0xd2, // left_divided_distance 11010010
	0x0f, // left_divided_distance 000011  ascr_skin_Rr 11
	0xfd, // ascr_skin_Rr 111111  ascr_skin_Rg 01
	0x41, // ascr_skin_Rg 010000  ascr_skin_Rb 01
	0x83, // ascr_skin_Rb 100000  ascr_skin_Yr 11
	0xff, // ascr_skin_Yr 111111  ascr_skin_Yg 11
	0xfc, // ascr_skin_Yg 111111  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xe0, // ascr_Wg 111000  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

static unsigned char LOCAL_CE_TEXT_1[] = {
	/* start */
	0xBA, // Start offset 0x0982, base B1h
	0x33, // data_width 00  lce_module 1  lcd_bypass 1  lcd_roi 00  algo_module 1  algo_bypass 1
	0x30, // algo_roi 00  ascr_module 1  ascr_bypass 1  ascr_roi 00  roi_type 00
	0x00, // roi0_x_start 00000000
	0x00, // roi0_x_start 0000  roi0_x_end 0000
	0x00, // roi0_x_end 00000000
	0x00, // roi0_y_start 00000000
	0x00, // roi0_y_start 0000  roi0_y_end 0000
	0x00, // roi0_y_end 00000000
	0x00, // roi1_x_start 00000000
	0x00, // roi1_x_start 0000  roi1_x_end 0000
	0x00, // roi1_x_end 00000000
	0x00, // roi1_y_start 00000000
	0x00, // roi1_y_start 0000  roi1_y_end 0000
	0x00, // roi1_y_end 00000000
	0xa0, // hsize 10100000
	0x06, // hsize 0000  vsize 0110
	0x40, // vsize 01000000
	0x17, // gain 000101 cgain 11
	0x00, // cgain 0000  scene_on 0  scene_tran 000
	0x14, // scene_tran 0  min_diff 0010100
	0x90, // illum_gain 10010000
	0xdf, // ref_offset 11011111
	0xac, // ref_offset 1  ref_gain 0101100
	0x3f, // ref_gain 00  hbsize 111  vbsize 111
	0xfa, // bth 11111010
	0xfe, // bin_size_ratio 1111111  dth 0
	0x10, // dth 00  min_ref_offset 010000
	0x1b, // min_ref_offset 00  gamma 0  cs 1 sharpen 1 nr 0 mask_th 11
	0xfc, // mask_th 111111  sharpen_weight 00
	0x40, // sharpen_weight 01000000
	0x14, // max_plus 00010100
	0x02, // max_plus 000  max_minus 00010
	0x81, // max_minus 100000  cs_gain 01
	0x40, // cs_gain 00000000
	0x00, // curve_1_b
	0x6b, // curve_1_a
	0x03, // curve_2_b
	0x48, // curve_2_a
	0x08, // curve_3_b
	0x32, // curve_3_a
	0x08, // curve_4_b
	0x32, // curve_4_a
	0x08, // curve_5_b
	0x32, // curve_5_a
	0x08, // curve_6_b
	0x32, // curve_6_a
	0x08, // curve_7_b
	0x32, // curve_7_a
	0x10, // curve_8_b
	0x28, // curve_8_a
	0x10, // curve_9_b
	0x28, // curve_9_a
	0x10, // curve_10_b
	0x28, // curve_10_a
	0x10, // curve_11_b
	0x28, // curve_11_a
	0x10, // curve_12_b
	0x28, // curve_12_a
	0x19, // curve_13_b
	0x22, // curve_13_a
	0x49, // curve_14_b
	0xdf, // curve_14_a
	0x49, // curve_15_b
	0xdf, // curve_15_a
	0x49, // curve_16_b
	0xdf, // curve_16_a
	0x49, // curve_17_b
	0xdf, // curve_17_a
	0x50, // curve_18_b
	0x1c, // curve_18_a
	0x5b, // curve_19_b
	0x18, // curve_19_a
	0x6a, // curve_20_b
	0x14, // curve_20_a
	0x7a, // curve_21_b
	0x11, // curve_21_a
	0x87, // curve_22_b
	0x0f, // curve_22_a
	0x87, // curve_23_b
	0x0f, // curve_23_a
	0x00, // curve_24_b
	0xff, // curve_24_a
};

static unsigned char LOCAL_CE_TEXT_2[] = {
	0xBA, // Start offset 0x09D4, base B1h
	0x85, // ascr_strength 10000  ascr_on 1  skin_cb 01
	0x9e, // skin_cb 100111  skin_cr 10
	0xa5, // skin_cr 101001  up_distance 01
	0x58, // up_distance 010110  down_distance 00
	0xa4, // down_distance 101001  right_distance 00
	0x65, // right_distance 011001  left_distance 01
	0x9c, // left_distance 100111  up_divided_distance 00
	0x05, // up_divided_distance 00000101
	0xf4, // up_divided_distance 11110100
	0x00, // up_divided_distance 00 down_divided_distance 000000
	0xc7, // down_divided_distance 11000111
	0xd0, // down_divided_distance 110100 right_divided_distance 00
	0x14, // right_divided_distance 00010100
	0x7b, // right_divided_distance 01111011
	0x00, // right_divided_distance 00  left_divided_distance 000000
	0x4f, // left_divided_distance 01001111
	0x8b, // left_divided_distance 100010  ascr_skin_Rr 11
	0xfe, // ascr_skin_Rr 111111  ascr_skin_Rg 10
	0x82, // ascr_skin_Rg 100000  ascr_skin_Rb 10
	0x83, // ascr_skin_Rb 100000  ascr_skin_Yr 11
	0xfe, // ascr_skin_Yr 111111  ascr_skin_Yg 10
	0x40, // ascr_skin_Yg 010000  ascr_skin_Yb 00
	0x03, // ascr_skin_Yb 000000  ascr_skin_Mr 11
	0xfc, // ascr_skin_Mr 111111  ascr_skin_Mg 00
	0x03, // ascr_skin_Mg 000000  ascr_skin_Mb 11
	0xff, // ascr_skin_Mb 111111  ascr_skin_Wr 11
	0xff, // ascr_skin_Wr 111111  ascr_skin_Wg 11
	0xff, // ascr_skin_Wg 111111  ascr_skin_Wb 11
	0xfc, // ascr_skin_Wb 111111  ascr_Cr 00
	0x03, // ascr_Cr 000000  ascr_Rr 11
	0xff, // ascr_Rr 111111  ascr_Cg 11
	0xfc, // ascr_Cg 111111  ascr_Rg 00
	0x03, // ascr_Rg 000000  ascr_Cb 11
	0xfc, // ascr_Cb 111111  ascr_Rb 00
	0x03, // ascr_Rb 000000  ascr_Mr 11
	0xfc, // ascr_Mr 111111  ascr_Gr 00
	0x00, // ascr_Gr 000000  ascr_Mg 00
	0x03, // ascr_Mg 000000  ascr_Gg 11
	0xff, // ascr_Gg 111111  ascr_Mb 11
	0xfc, // ascr_Mb 111111  ascr_Gb 00
	0x03, // ascr_Gb 000000  ascr_Yr 11
	0xfc, // ascr_Yr 111111  ascr_Br 00
	0x03, // ascr_Br 000000  ascr_Yg 11
	0xfc, // ascr_Yg 111111  ascr_Bg 00
	0x00, // ascr_Bg 000000  ascr_Yb 00
	0x03, // ascr_Yb 000000  ascr_Bb 11
	0xff, // ascr_Bb 111111  ascr_Wr 11
	0xfc, // ascr_Wr 111111  ascr_Kr 00
	0x03, // ascr_Kr 000000  ascr_Wg 11
	0xe0, // ascr_Wg 111000  ascr_Kg 00
	0x03, // ascr_Kg 000000  ascr_Wb 11
	0xfc, // ascr_Wb 111111  ascr_Kb 00
	0x00, // ascr_Kb 000000  reserved 00
	0x0f, // reserved 0000  vsync_mask 1111
	/* end */
};

unsigned char *blind_tune_value[ACCESSIBILITY_MAX][2] = {
		/*
			ACCESSIBILITY_OFF,
			NEGATIVE,
			COLOR_BLIND,
			SCREEN_CURTAIN,
		*/
		{NULL, NULL},
		{NEGATIVE_1, NEGATIVE_2},
		{COLOR_BLIND_1, COLOR_BLIND_2},
		{SCREEN_CURTAIN_1, SCREEN_CURTAIN_2},
};

unsigned char *mdnie_tune_value[MAX_mDNIe_MODE][MAX_BACKGROUND_MODE][MAX_OUTDOOR_MODE][2] = {
		/*
			DYNAMIC_MODE (outdoor off/on)
			STANDARD_MODE (outdoor off/on)
			NATURAL_MODE (outdoor off/on)
			MOVIE_MODE (outdoor off/on)
			AUTO_MODE (outdoor off/on)
		*/
		// UI_APP
		{
			{{DYNAMIC_UI_1, DYNAMIC_UI_2}, {NULL, NULL}},
			{{STANDARD_UI_1, STANDARD_UI_2}, {NULL, NULL}},
			{{NATURAL_UI_1, NATURAL_UI_2}, {NULL, NULL}},
			{{MOVIE_UI_1, MOVIE_UI_2}, {NULL, NULL}},
			{{AUTO_UI_1, AUTO_UI_2}, {NULL, NULL}},
		},
		// VIDEO_APP
		{
			{{DYNAMIC_VIDEO_1, DYNAMIC_VIDEO_2}, {NULL, NULL}},
			{{STANDARD_VIDEO_1, STANDARD_VIDEO_2}, {NULL, NULL}},
			{{NATURAL_VIDEO_1, NATURAL_VIDEO_2}, {NULL, NULL}},
			{{MOVIE_VIDEO_1, MOVIE_VIDEO_2}, {NULL, NULL}},
			{{AUTO_VIDEO_1, AUTO_VIDEO_2}, {NULL, NULL}},
		},
		// VIDEO_WARM_APP
		{
			{{NULL, NULL}, {NULL, NULL}},
			{{NULL, NULL}, {NULL, NULL}},
			{{NULL, NULL}, {NULL, NULL}},
			{{NULL, NULL}, {NULL, NULL}},
			{{NULL, NULL}, {NULL, NULL}},
		},
		// VIDEO_COLD_APP
		{
			{{NULL, NULL}, {NULL, NULL}},
			{{NULL, NULL}, {NULL, NULL}},
			{{NULL, NULL}, {NULL, NULL}},
			{{NULL, NULL}, {NULL, NULL}},
			{{NULL, NULL}, {NULL, NULL}},
		},
		// CAMERA_APP
		{
			{{CAMERA_1, CAMERA_2}, {NULL, NULL}},
			{{CAMERA_1, CAMERA_2}, {NULL, NULL}},
			{{CAMERA_1, CAMERA_2}, {NULL, NULL}},
			{{CAMERA_1, CAMERA_2}, {NULL, NULL}},
			{{AUTO_CAMERA_1, AUTO_CAMERA_2}, {NULL, NULL}},
		},
		// NAVI_APP
		{
			{{NULL, NULL}, {NULL, NULL}},
			{{NULL, NULL}, {NULL, NULL}},
			{{NULL, NULL}, {NULL, NULL}},
			{{NULL, NULL}, {NULL, NULL}},
			{{NULL, NULL}, {NULL, NULL}},
		},
		// GALLERY_APP
		{
			{{DYNAMIC_GALLERY_1, DYNAMIC_GALLERY_2}, {NULL,NULL}},
			{{STANDARD_GALLERY_1, STANDARD_GALLERY_2}, {NULL,NULL}},
			{{NATURAL_GALLERY_1, NATURAL_GALLERY_2}, {NULL,NULL}},
			{{MOVIE_GALLERY_1, MOVIE_GALLERY_2}, {NULL,NULL}},
			{{AUTO_GALLERY_1, AUTO_GALLERY_2}, {NULL,NULL}},
		},
		// VT_APP
		{
			{{DYNAMIC_VT_1, DYNAMIC_VT_2}, {NULL, NULL}},
			{{STANDARD_VT_1, STANDARD_VT_2}, {NULL, NULL}},
			{{NATURAL_VT_1, NATURAL_VT_2}, {NULL, NULL}},
			{{MOVIE_VT_1, MOVIE_VT_2}, {NULL, NULL}},
			{{AUTO_VT_1, AUTO_VT_2}, {NULL, NULL}},
		},
		// BROWSER_APP
		{
			{{DYNAMIC_BROWSER_1, DYNAMIC_BROWSER_2}, {NULL, NULL}},
			{{STANDARD_BROWSER_1, STANDARD_BROWSER_2}, {NULL, NULL}},
			{{NATURAL_BROWSER_1, NATURAL_BROWSER_2}, {NULL, NULL}},
			{{MOVIE_BROWSER_1, MOVIE_BROWSER_2}, {NULL, NULL}},
			{{AUTO_BROWSER_1, AUTO_BROWSER_2}, {NULL, NULL}},
		},
		// eBOOK_APP
		{
			{{DYNAMIC_EBOOK_1, DYNAMIC_EBOOK_2}, {NULL, NULL}},
			{{STANDARD_EBOOK_1, STANDARD_EBOOK_2}, {NULL, NULL}},
			{{NATURAL_EBOOK_1, NATURAL_EBOOK_2}, {NULL, NULL}},
			{{MOVIE_EBOOK_1, MOVIE_EBOOK_2}, {NULL, NULL}},
			{{AUTO_EBOOK_1, AUTO_EBOOK_2}, {NULL, NULL}},
		},
		// EMAIL_APP
		{
			{{AUTO_EMAIL_1, AUTO_EMAIL_2}, {NULL, NULL}},
			{{AUTO_EMAIL_1, AUTO_EMAIL_2}, {NULL, NULL}},
			{{AUTO_EMAIL_1, AUTO_EMAIL_2}, {NULL, NULL}},
			{{AUTO_EMAIL_1, AUTO_EMAIL_2}, {NULL, NULL}},
			{{AUTO_EMAIL_1, AUTO_EMAIL_2}, {NULL, NULL}},
		},
};

#endif
