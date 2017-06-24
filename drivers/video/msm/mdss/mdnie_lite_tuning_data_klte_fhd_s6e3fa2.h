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

#ifndef _MDNIE_LITE_TUNING_DATA_K_FHD_
#define _MDNIE_LITE_TUNING_DATA_K_FHD_

#define ADDRESS_SCR_WHITE_RED   0x7A
#define ADDRESS_SCR_WHITE_GREEN 0x7C
#define ADDRESS_SCR_WHITE_BLUE  0x7E

char GRAYSCALE_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x02, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

char GRAYSCALE_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x00, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0xb3, //ascr_Cr
	0x4c, //ascr_Rr
	0xb3, //ascr_Cg
	0x4c, //ascr_Rg
	0xb3, //ascr_Cb
	0x4c, //ascr_Rb
	0x69, //ascr_Mr
	0x96, //ascr_Gr
	0x69, //ascr_Mg
	0x96, //ascr_Gg
	0x69, //ascr_Mb
	0x96, //ascr_Gb
	0xe2, //ascr_Yr
	0x1d, //ascr_Br
	0xe2, //ascr_Yg
	0x1d, //ascr_Bg
	0xe2, //ascr_Yb
	0x1d, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

char GRAYSCALE_NEGATIVE_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x02, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

char GRAYSCALE_NEGATIVE_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x00, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x4c, //ascr_Cr
	0xb3, //ascr_Rr
	0x4c, //ascr_Cg
	0xb3, //ascr_Rg
	0x4c, //ascr_Cb
	0xb3, //ascr_Rb
	0x96, //ascr_Mr
	0x69, //ascr_Gr
	0x96, //ascr_Mg
	0x69, //ascr_Gg
	0x96, //ascr_Mb
	0x69, //ascr_Gb
	0x1d, //ascr_Yr
	0xe2, //ascr_Br
	0x1d, //ascr_Yg
	0xe2, //ascr_Bg
	0x1d, //ascr_Yb
	0xe2, //ascr_Bb
	0x00, //ascr_Wr
	0xff, //ascr_Kr
	0x00, //ascr_Wg
	0xff, //ascr_Kg
	0x00, //ascr_Wb
	0xff, //ascr_Kb
	//end

};

////////////////// UI /// /////////////////////
static char SCREEN_CURTAIN_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x02, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char SCREEN_CURTAIN_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x00, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x00,
	0x07, //sharpen_maxplus 11
	0xff,
	0x07, //sharpen_maxminus 11
	0xff,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x0c, //ascr_dist_up
	0x0c, //ascr_dist_down
	0x0c, //ascr_dist_right
	0x0c, //ascr_dist_left
	0x00, //ascr_div_up 20
	0xaa,
	0xab,
	0x00, //ascr_div_down
	0xaa,
	0xab,
	0x00, //ascr_div_right
	0xaa,
	0xab,
	0x00, //ascr_div_left
	0xaa,
	0xab,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0x00, //ascr_Rr
	0x00, //ascr_Cg
	0x00, //ascr_Rg
	0x00, //ascr_Cb
	0x00, //ascr_Rb
	0x00, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0x00, //ascr_Gg
	0x00, //ascr_Mb
	0x00, //ascr_Gb
	0x00, //ascr_Yr
	0x00, //ascr_Br
	0x00, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0x00, //ascr_Bb
	0x00, //ascr_Wr
	0x00, //ascr_Kr
	0x00, //ascr_Wg
	0x00, //ascr_Kg
	0x00, //ascr_Wb
	0x00, //ascr_Kb
	//end
};

static char STANDARD_UI_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char STANDARD_UI_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x01, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char NATURAL_UI_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char NATURAL_UI_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x01, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x16, //ascr_Rg
	0xe5, //ascr_Cb
	0x0e, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x14, //ascr_Mg
	0xff, //ascr_Gg
	0xd9, //ascr_Mb
	0x04, //ascr_Gb
	0xfc, //ascr_Yr
	0x26, //ascr_Br
	0xff, //ascr_Yg
	0x1b, //ascr_Bg
	0x23, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xf8, //ascr_Wg
	0x00, //ascr_Kg
	0xef, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char DYNAMIC_UI_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char DYNAMIC_UI_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x03, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x50,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x20,
	0x00, //curve_1_b
	0x0f, //curve_1_a
	0x00, //curve_2_b
	0x0f, //curve_2_a
	0x00, //curve_3_b
	0x0f, //curve_3_a
	0x00, //curve_4_b
	0x0f, //curve_4_a
	0x09, //curve_5_b
	0xa2, //curve_5_a
	0x09, //curve_6_b
	0xa2, //curve_6_a
	0x09, //curve_7_b
	0xa2, //curve_7_a
	0x09, //curve_8_b
	0xa2, //curve_8_a
	0x09, //curve_9_b
	0xa2, //curve_9_a
	0x09, //curve10_b
	0xa2, //curve10_a
	0x0a, //curve11_b
	0xa2, //curve11_a
	0x0a, //curve12_b
	0xa2, //curve12_a
	0x0a, //curve13_b
	0xa2, //curve13_a
	0x0a, //curve14_b
	0xa2, //curve14_a
	0x0a, //curve15_b
	0xa2, //curve15_a
	0x0a, //curve16_b
	0xa2, //curve16_a
	0x0a, //curve17_b
	0xa2, //curve17_a
	0x0a, //curve18_b
	0xa2, //curve18_a
	0x0f, //curve19_b
	0xa4, //curve19_a
	0x0f, //curve20_b
	0xa4, //curve20_a
	0x0f, //curve21_b
	0xa4, //curve21_a
	0x23, //curve22_b
	0x1c, //curve22_a
	0x48, //curve23_b
	0x17, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char MOVIE_UI_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char MOVIE_UI_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x03, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x20,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x7a, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x2b, //ascr_Rg
	0xf1, //ascr_Cb
	0x2d, //ascr_Rb
	0xff, //ascr_Mr
	0x68, //ascr_Gr
	0x15, //ascr_Mg
	0xff, //ascr_Gg
	0xfa, //ascr_Mb
	0x31, //ascr_Gb
	0xf8, //ascr_Yr
	0x34, //ascr_Br
	0xff, //ascr_Yg
	0x20, //ascr_Bg
	0x4b, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xf8, //ascr_Wg
	0x00, //ascr_Kg
	0xef, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

char AUTO_UI_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

char AUTO_UI_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x01, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

////////////////// GALLERY /////////////////////
static char STANDARD_GALLERY_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char STANDARD_GALLERY_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x05, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x28,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char NATURAL_GALLERY_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char NATURAL_GALLERY_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x05, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x10,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x16, //ascr_Rg
	0xe5, //ascr_Cb
	0x0e, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x14, //ascr_Mg
	0xff, //ascr_Gg
	0xd9, //ascr_Mb
	0x04, //ascr_Gb
	0xfc, //ascr_Yr
	0x26, //ascr_Br
	0xff, //ascr_Yg
	0x1b, //ascr_Bg
	0x23, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xf8, //ascr_Wg
	0x00, //ascr_Kg
	0xef, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char DYNAMIC_GALLERY_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char DYNAMIC_GALLERY_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x07, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x20,
	0x00, //curve_1_b
	0x0f, //curve_1_a
	0x00, //curve_2_b
	0x0f, //curve_2_a
	0x00, //curve_3_b
	0x0f, //curve_3_a
	0x00, //curve_4_b
	0x0f, //curve_4_a
	0x09, //curve_5_b
	0xa2, //curve_5_a
	0x09, //curve_6_b
	0xa2, //curve_6_a
	0x09, //curve_7_b
	0xa2, //curve_7_a
	0x09, //curve_8_b
	0xa2, //curve_8_a
	0x09, //curve_9_b
	0xa2, //curve_9_a
	0x09, //curve10_b
	0xa2, //curve10_a
	0x0a, //curve11_b
	0xa2, //curve11_a
	0x0a, //curve12_b
	0xa2, //curve12_a
	0x0a, //curve13_b
	0xa2, //curve13_a
	0x0a, //curve14_b
	0xa2, //curve14_a
	0x0a, //curve15_b
	0xa2, //curve15_a
	0x0a, //curve16_b
	0xa2, //curve16_a
	0x0a, //curve17_b
	0xa2, //curve17_a
	0x0a, //curve18_b
	0xa2, //curve18_a
	0x0f, //curve19_b
	0xa4, //curve19_a
	0x0f, //curve20_b
	0xa4, //curve20_a
	0x0f, //curve21_b
	0xa4, //curve21_a
	0x23, //curve22_b
	0x1c, //curve22_a
	0x48, //curve23_b
	0x17, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char MOVIE_GALLERY_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char MOVIE_GALLERY_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x07, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x10,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x20,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x7a, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x2b, //ascr_Rg
	0xf1, //ascr_Cb
	0x2d, //ascr_Rb
	0xff, //ascr_Mr
	0x68, //ascr_Gr
	0x15, //ascr_Mg
	0xff, //ascr_Gg
	0xfa, //ascr_Mb
	0x31, //ascr_Gb
	0xf8, //ascr_Yr
	0x34, //ascr_Br
	0xff, //ascr_Yg
	0x20, //ascr_Bg
	0x4b, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xf8, //ascr_Wg
	0x00, //ascr_Kg
	0xef, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

char AUTO_GALLERY_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

char AUTO_GALLERY_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x05, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x28,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x30, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x50, //ascr_skin_Rg
	0x60, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

////////////////// VIDEO /////////////////////

static char STANDARD_VIDEO_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char STANDARD_VIDEO_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x05, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x28,
	0x00, //sharpen_maxplus 11
	0x40,
	0x00, //sharpen_maxminus 11
	0x40,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char NATURAL_VIDEO_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char NATURAL_VIDEO_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x05, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x10,
	0x00, //sharpen_maxplus 11
	0x40,
	0x00, //sharpen_maxminus 11
	0x40,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x16, //ascr_Rg
	0xe5, //ascr_Cb
	0x0e, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x14, //ascr_Mg
	0xff, //ascr_Gg
	0xd9, //ascr_Mb
	0x04, //ascr_Gb
	0xfc, //ascr_Yr
	0x26, //ascr_Br
	0xff, //ascr_Yg
	0x1b, //ascr_Bg
	0x23, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xf8, //ascr_Wg
	0x00, //ascr_Kg
	0xef, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char DYNAMIC_VIDEO_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char DYNAMIC_VIDEO_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x07, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0x40,
	0x00, //sharpen_maxminus 11
	0x40,
	0x01, //cs_gain 10
	0x20,
	0x00, //curve_1_b
	0x0f, //curve_1_a
	0x00, //curve_2_b
	0x0f, //curve_2_a
	0x00, //curve_3_b
	0x0f, //curve_3_a
	0x00, //curve_4_b
	0x0f, //curve_4_a
	0x09, //curve_5_b
	0xa2, //curve_5_a
	0x09, //curve_6_b
	0xa2, //curve_6_a
	0x09, //curve_7_b
	0xa2, //curve_7_a
	0x09, //curve_8_b
	0xa2, //curve_8_a
	0x09, //curve_9_b
	0xa2, //curve_9_a
	0x09, //curve10_b
	0xa2, //curve10_a
	0x0a, //curve11_b
	0xa2, //curve11_a
	0x0a, //curve12_b
	0xa2, //curve12_a
	0x0a, //curve13_b
	0xa2, //curve13_a
	0x0a, //curve14_b
	0xa2, //curve14_a
	0x0a, //curve15_b
	0xa2, //curve15_a
	0x0a, //curve16_b
	0xa2, //curve16_a
	0x0a, //curve17_b
	0xa2, //curve17_a
	0x0a, //curve18_b
	0xa2, //curve18_a
	0x0f, //curve19_b
	0xa4, //curve19_a
	0x0f, //curve20_b
	0xa4, //curve20_a
	0x0f, //curve21_b
	0xa4, //curve21_a
	0x23, //curve22_b
	0x1c, //curve22_a
	0x48, //curve23_b
	0x17, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char MOVIE_VIDEO_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char MOVIE_VIDEO_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x07, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x10,
	0x00, //sharpen_maxplus 11
	0x40,
	0x00, //sharpen_maxminus 11
	0x40,
	0x01, //cs_gain 10
	0x20,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x7a, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x2b, //ascr_Rg
	0xf1, //ascr_Cb
	0x2d, //ascr_Rb
	0xff, //ascr_Mr
	0x68, //ascr_Gr
	0x15, //ascr_Mg
	0xff, //ascr_Gg
	0xfa, //ascr_Mb
	0x31, //ascr_Gb
	0xf8, //ascr_Yr
	0x34, //ascr_Br
	0xff, //ascr_Yg
	0x20, //ascr_Bg
	0x4b, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xf8, //ascr_Wg
	0x00, //ascr_Kg
	0xef, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

char AUTO_VIDEO_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

char AUTO_VIDEO_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x05, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x28,
	0x00, //sharpen_maxplus 11
	0x40,
	0x00, //sharpen_maxminus 11
	0x40,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x30, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x50, //ascr_skin_Rg
	0x60, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

////////////////// VT /////////////////////

static char STANDARD_VT_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char STANDARD_VT_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x05, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x28,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char NATURAL_VT_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char NATURAL_VT_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x05, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x10,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x16, //ascr_Rg
	0xe5, //ascr_Cb
	0x0e, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x14, //ascr_Mg
	0xff, //ascr_Gg
	0xd9, //ascr_Mb
	0x04, //ascr_Gb
	0xfc, //ascr_Yr
	0x26, //ascr_Br
	0xff, //ascr_Yg
	0x1b, //ascr_Bg
	0x23, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xf8, //ascr_Wg
	0x00, //ascr_Kg
	0xef, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char DYNAMIC_VT_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char DYNAMIC_VT_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x07, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x20,
	0x00, //curve_1_b
	0x0f, //curve_1_a
	0x00, //curve_2_b
	0x0f, //curve_2_a
	0x00, //curve_3_b
	0x0f, //curve_3_a
	0x00, //curve_4_b
	0x0f, //curve_4_a
	0x09, //curve_5_b
	0xa2, //curve_5_a
	0x09, //curve_6_b
	0xa2, //curve_6_a
	0x09, //curve_7_b
	0xa2, //curve_7_a
	0x09, //curve_8_b
	0xa2, //curve_8_a
	0x09, //curve_9_b
	0xa2, //curve_9_a
	0x09, //curve10_b
	0xa2, //curve10_a
	0x0a, //curve11_b
	0xa2, //curve11_a
	0x0a, //curve12_b
	0xa2, //curve12_a
	0x0a, //curve13_b
	0xa2, //curve13_a
	0x0a, //curve14_b
	0xa2, //curve14_a
	0x0a, //curve15_b
	0xa2, //curve15_a
	0x0a, //curve16_b
	0xa2, //curve16_a
	0x0a, //curve17_b
	0xa2, //curve17_a
	0x0a, //curve18_b
	0xa2, //curve18_a
	0x0f, //curve19_b
	0xa4, //curve19_a
	0x0f, //curve20_b
	0xa4, //curve20_a
	0x0f, //curve21_b
	0xa4, //curve21_a
	0x23, //curve22_b
	0x1c, //curve22_a
	0x48, //curve23_b
	0x17, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char MOVIE_VT_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char MOVIE_VT_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x07, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x10,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x20,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x7a, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x2b, //ascr_Rg
	0xf1, //ascr_Cb
	0x2d, //ascr_Rb
	0xff, //ascr_Mr
	0x68, //ascr_Gr
	0x15, //ascr_Mg
	0xff, //ascr_Gg
	0xfa, //ascr_Mb
	0x31, //ascr_Gb
	0xf8, //ascr_Yr
	0x34, //ascr_Br
	0xff, //ascr_Yg
	0x20, //ascr_Bg
	0x4b, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xf8, //ascr_Wg
	0x00, //ascr_Kg
	0xef, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

char BYPASS_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x00, //ascr_roi 1 ascr 00 1 0
	0x02, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

char BYPASS_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x00, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x00,
	0x07, //sharpen_maxplus 11
	0xff,
	0x07, //sharpen_maxminus 11
	0xff,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x10, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x0c, //ascr_dist_up
	0x0c, //ascr_dist_down
	0x0c, //ascr_dist_right
	0x0c, //ascr_dist_left
	0x00, //ascr_div_up 20
	0xaa,
	0xab,
	0x00, //ascr_div_down
	0xaa,
	0xab,
	0x00, //ascr_div_right
	0xaa,
	0xab,
	0x00, //ascr_div_left
	0xaa,
	0xab,
	0xd5, //ascr_skin_Rr
	0x2c, //ascr_skin_Rg
	0x2a, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xf5, //ascr_skin_Yg
	0x63, //ascr_skin_Yb
	0xfe, //ascr_skin_Mr
	0x4a, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xf9, //ascr_skin_Wg
	0xf8, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

char AUTO_VT_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

char AUTO_VT_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x05, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x28,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

////////////////// CAMERA /////////////////////

static char CAMERA_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char CAMERA_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x01, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

char AUTO_CAMERA_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

char AUTO_CAMERA_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x01, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x30, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x50, //ascr_skin_Rg
	0x60, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};


static char CAMERA_OUTDOOR_1[] = {
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

static char CAMERA_OUTDOOR_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x07, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x10,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x2c, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x4e, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x5f, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x1a,
	0x74,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x15,
	0x8f,
	0xff, //ascr_skin_Rr
	0x20, //ascr_skin_Rg
	0x20, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xfc, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xfd, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char COLD_1[] = {
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

static char COLD_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x07, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x10,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x2c, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x4e, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x5f, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x1a,
	0x74,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x15,
	0x8f,
	0xff, //ascr_skin_Rr
	0x20, //ascr_skin_Rg
	0x20, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xfc, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xfd, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char COLD_OUTDOOR_1[] = {
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

static char COLD_OUTDOOR_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x07, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x10,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x2c, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x4e, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x5f, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x1a,
	0x74,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x15,
	0x8f,
	0xff, //ascr_skin_Rr
	0x20, //ascr_skin_Rg
	0x20, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xfc, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xfd, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char WARM_1[] = {
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

static char WARM_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x07, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x10,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x2c, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x4e, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x5f, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x1a,
	0x74,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x15,
	0x8f,
	0xff, //ascr_skin_Rr
	0x20, //ascr_skin_Rg
	0x20, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xfc, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xfd, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char WARM_OUTDOOR_1[] = {
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

static char WARM_OUTDOOR_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x07, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x10,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x2c, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x4e, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x5f, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x1a,
	0x74,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x15,
	0x8f,
	0xff, //ascr_skin_Rr
	0x20, //ascr_skin_Rg
	0x20, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xfc, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xfd, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char NEGATIVE_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x02, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

static char NEGATIVE_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x00, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x00,
	0x07, //sharpen_maxplus 11
	0xff,
	0x07, //sharpen_maxminus 11
	0xff,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x0c, //ascr_dist_up
	0x0c, //ascr_dist_down
	0x0c, //ascr_dist_right
	0x0c, //ascr_dist_left
	0x00, //ascr_div_up 20
	0xaa,
	0xab,
	0x00, //ascr_div_down
	0xaa,
	0xab,
	0x00, //ascr_div_right
	0xaa,
	0xab,
	0x00, //ascr_div_left
	0xaa,
	0xab,
	0x00, //ascr_skin_Rr
	0xff, //ascr_skin_Rg
	0xff, //ascr_skin_Rb
	0x00, //ascr_skin_Yr
	0x00, //ascr_skin_Yg
	0xff, //ascr_skin_Yb
	0x00, //ascr_skin_Mr
	0xff, //ascr_skin_Mg
	0x00, //ascr_skin_Mb
	0x00, //ascr_skin_Wr
	0x00, //ascr_skin_Wg
	0x00, //ascr_skin_Wb
	0xff, //ascr_Cr
	0x00, //ascr_Rr
	0x00, //ascr_Cg
	0xff, //ascr_Rg
	0x00, //ascr_Cb
	0xff, //ascr_Rb
	0x00, //ascr_Mr
	0xff, //ascr_Gr
	0xff, //ascr_Mg
	0x00, //ascr_Gg
	0x00, //ascr_Mb
	0xff, //ascr_Gb
	0x00, //ascr_Yr
	0xff, //ascr_Br
	0x00, //ascr_Yg
	0xff, //ascr_Bg
	0xff, //ascr_Yb
	0x00, //ascr_Bb
	0x00, //ascr_Wr
	0xff, //ascr_Kr
	0x00, //ascr_Wg
	0xff, //ascr_Kg
	0x00, //ascr_Wb
	0xff, //ascr_Kb
	//end

};

static char OUTDOOR_VIDEO_1[] = {
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

static char OUTDOOR_VIDEO_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x07, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x10,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x2c, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x4e, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x5f, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x1a,
	0x74,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x15,
	0x8f,
	0xff, //ascr_skin_Rr
	0x20, //ascr_skin_Rg
	0x20, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xfc, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xfd, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

char COLOR_BLIND_1[] = {
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x02, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

char COLOR_BLIND_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x00, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x00,
	0x07, //sharpen_maxplus 11
	0xff,
	0x07, //sharpen_maxminus 11
	0xff,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x0c, //ascr_dist_up
	0x0c, //ascr_dist_down
	0x0c, //ascr_dist_right
	0x0c, //ascr_dist_left
	0x00, //ascr_div_up 20
	0xaa,
	0xab,
	0x00, //ascr_div_down
	0xaa,
	0xab,
	0x00, //ascr_div_right
	0xaa,
	0xab,
	0x00, //ascr_div_left
	0xaa,
	0xab,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

////////////////// BROWSER /////////////////////

char STANDARD_BROWSER_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

char STANDARD_BROWSER_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x01, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

char NATURAL_BROWSER_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

char NATURAL_BROWSER_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x01, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x16, //ascr_Rg
	0xe5, //ascr_Cb
	0x0e, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x14, //ascr_Mg
	0xff, //ascr_Gg
	0xd9, //ascr_Mb
	0x04, //ascr_Gb
	0xfc, //ascr_Yr
	0x26, //ascr_Br
	0xff, //ascr_Yg
	0x1b, //ascr_Bg
	0x23, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xf8, //ascr_Wg
	0x00, //ascr_Kg
	0xef, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

char DYNAMIC_BROWSER_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

char DYNAMIC_BROWSER_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x03, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x50,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x20,
	0x00, //curve_1_b
	0x0f, //curve_1_a
	0x00, //curve_2_b
	0x0f, //curve_2_a
	0x00, //curve_3_b
	0x0f, //curve_3_a
	0x00, //curve_4_b
	0x0f, //curve_4_a
	0x09, //curve_5_b
	0xa2, //curve_5_a
	0x09, //curve_6_b
	0xa2, //curve_6_a
	0x09, //curve_7_b
	0xa2, //curve_7_a
	0x09, //curve_8_b
	0xa2, //curve_8_a
	0x09, //curve_9_b
	0xa2, //curve_9_a
	0x09, //curve10_b
	0xa2, //curve10_a
	0x0a, //curve11_b
	0xa2, //curve11_a
	0x0a, //curve12_b
	0xa2, //curve12_a
	0x0a, //curve13_b
	0xa2, //curve13_a
	0x0a, //curve14_b
	0xa2, //curve14_a
	0x0a, //curve15_b
	0xa2, //curve15_a
	0x0a, //curve16_b
	0xa2, //curve16_a
	0x0a, //curve17_b
	0xa2, //curve17_a
	0x0a, //curve18_b
	0xa2, //curve18_a
	0x0f, //curve19_b
	0xa4, //curve19_a
	0x0f, //curve20_b
	0xa4, //curve20_a
	0x0f, //curve21_b
	0xa4, //curve21_a
	0x23, //curve22_b
	0x1c, //curve22_a
	0x48, //curve23_b
	0x17, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

char MOVIE_BROWSER_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

char MOVIE_BROWSER_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x03, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x20,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x7a, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x2b, //ascr_Rg
	0xf1, //ascr_Cb
	0x2d, //ascr_Rb
	0xff, //ascr_Mr
	0x68, //ascr_Gr
	0x15, //ascr_Mg
	0xff, //ascr_Gg
	0xfa, //ascr_Mb
	0x31, //ascr_Gb
	0xf8, //ascr_Yr
	0x34, //ascr_Br
	0xff, //ascr_Yg
	0x20, //ascr_Bg
	0x4b, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xf8, //ascr_Wg
	0x00, //ascr_Kg
	0xef, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

char AUTO_BROWSER_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

char AUTO_BROWSER_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x01, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x30, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x50, //ascr_skin_Rg
	0x60, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

////////////////// eBOOK /////////////////////
char DYNAMIC_EBOOK_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

char DYNAMIC_EBOOK_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x03, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x50,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x20,
	0x00, //curve_1_b
	0x0f, //curve_1_a
	0x00, //curve_2_b
	0x0f, //curve_2_a
	0x00, //curve_3_b
	0x0f, //curve_3_a
	0x00, //curve_4_b
	0x0f, //curve_4_a
	0x09, //curve_5_b
	0xa2, //curve_5_a
	0x09, //curve_6_b
	0xa2, //curve_6_a
	0x09, //curve_7_b
	0xa2, //curve_7_a
	0x09, //curve_8_b
	0xa2, //curve_8_a
	0x09, //curve_9_b
	0xa2, //curve_9_a
	0x09, //curve10_b
	0xa2, //curve10_a
	0x0a, //curve11_b
	0xa2, //curve11_a
	0x0a, //curve12_b
	0xa2, //curve12_a
	0x0a, //curve13_b
	0xa2, //curve13_a
	0x0a, //curve14_b
	0xa2, //curve14_a
	0x0a, //curve15_b
	0xa2, //curve15_a
	0x0a, //curve16_b
	0xa2, //curve16_a
	0x0a, //curve17_b
	0xa2, //curve17_a
	0x0a, //curve18_b
	0xa2, //curve18_a
	0x0f, //curve19_b
	0xa4, //curve19_a
	0x0f, //curve20_b
	0xa4, //curve20_a
	0x0f, //curve21_b
	0xa4, //curve21_a
	0x23, //curve22_b
	0x1c, //curve22_a
	0x48, //curve23_b
	0x17, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

char STANDARD_EBOOK_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

char STANDARD_EBOOK_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x01, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end

};
char NATURAL_EBOOK_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

char NATURAL_EBOOK_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x01, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x16, //ascr_Rg
	0xe5, //ascr_Cb
	0x0e, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x14, //ascr_Mg
	0xff, //ascr_Gg
	0xd9, //ascr_Mb
	0x04, //ascr_Gb
	0xfc, //ascr_Yr
	0x26, //ascr_Br
	0xff, //ascr_Yg
	0x1b, //ascr_Bg
	0x23, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xf8, //ascr_Wg
	0x00, //ascr_Kg
	0xef, //ascr_Wb
	0x00, //ascr_Kb
	//end

};
char MOVIE_EBOOK_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

char MOVIE_EBOOK_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x03, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x20,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x7a, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x2b, //ascr_Rg
	0xf1, //ascr_Cb
	0x2d, //ascr_Rb
	0xff, //ascr_Mr
	0x68, //ascr_Gr
	0x15, //ascr_Mg
	0xff, //ascr_Gg
	0xfa, //ascr_Mb
	0x31, //ascr_Gb
	0xf8, //ascr_Yr
	0x34, //ascr_Br
	0xff, //ascr_Yg
	0x20, //ascr_Bg
	0x4b, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xf8, //ascr_Wg
	0x00, //ascr_Kg
	0xef, //ascr_Wb
	0x00, //ascr_Kb
	//end

};
char AUTO_EBOOK_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x02, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,

};

char AUTO_EBOOK_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x00, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xf6, //ascr_Wg
	0x00, //ascr_Kg
	0xe6, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

char AUTO_EMAIL_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x02, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

char AUTO_EMAIL_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x00, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x00, //curve_1_b
	0x20, //curve_1_a
	0x00, //curve_2_b
	0x20, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x02, //curve_5_b
	0x1b, //curve_5_a
	0x02, //curve_6_b
	0x1b, //curve_6_a
	0x02, //curve_7_b
	0x1b, //curve_7_a
	0x02, //curve_8_b
	0x1b, //curve_8_a
	0x09, //curve_9_b
	0xa6, //curve_9_a
	0x09, //curve10_b
	0xa6, //curve10_a
	0x09, //curve11_b
	0xa6, //curve11_a
	0x09, //curve12_b
	0xa6, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xfa, //ascr_Wg
	0x00, //ascr_Kg
	0xef, //ascr_Wb
	0x00, //ascr_Kb
	//end
};

#if defined(CONFIG_LCD_HMT)
char HMT_GRAY_8_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x02, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

char HMT_GRAY_8_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x01, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x08, //curve_1_b
	0x00, //curve_1_a
	0x08, //curve_2_b
	0x00, //curve_2_a
	0x00, //curve_3_b
	0x20, //curve_3_a
	0x00, //curve_4_b
	0x20, //curve_4_a
	0x00, //curve_5_b
	0x20, //curve_5_a
	0x00, //curve_6_b
	0x20, //curve_6_a
	0x00, //curve_7_b
	0x20, //curve_7_a
	0x00, //curve_8_b
	0x20, //curve_8_a
	0x00, //curve_9_b
	0x20, //curve_9_a
	0x00, //curve10_b
	0x20, //curve10_a
	0x00, //curve11_b
	0x20, //curve11_a
	0x00, //curve12_b
	0x20, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end
};

char HMT_GRAY_16_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x02, //ascr_roi 1 ascr 00 1 0
	0x32, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

char HMT_GRAY_16_2[] = {
	0xEC,
	0x18, //lce_gain 00 0000
	0x24, //lce_color_gain 00 0000
	0x10, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0xb3, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0x0e,
	0x01, //lce_ref_gain 9
	0x00,
	0x66, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x2d, //lce_bin_size_ratio
	0x03, //lce_dark_th 000
	0x96, //lce_min_ref_offset
	0x01, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x00,
	0x10, //curve_1_b
	0x00, //curve_1_a
	0x10, //curve_2_b
	0x00, //curve_2_a
	0x10, //curve_3_b
	0x00, //curve_3_a
	0x10, //curve_4_b
	0x00, //curve_4_a
	0x00, //curve_5_b
	0x20, //curve_5_a
	0x00, //curve_6_b
	0x20, //curve_6_a
	0x00, //curve_7_b
	0x20, //curve_7_a
	0x00, //curve_8_b
	0x20, //curve_8_a
	0x00, //curve_9_b
	0x20, //curve_9_a
	0x00, //curve10_b
	0x20, //curve10_a
	0x00, //curve11_b
	0x20, //curve11_a
	0x00, //curve12_b
	0x20, //curve12_a
	0x00, //curve13_b
	0x20, //curve13_a
	0x00, //curve14_b
	0x20, //curve14_a
	0x00, //curve15_b
	0x20, //curve15_a
	0x00, //curve16_b
	0x20, //curve16_a
	0x00, //curve17_b
	0x20, //curve17_a
	0x00, //curve18_b
	0x20, //curve18_a
	0x00, //curve19_b
	0x20, //curve19_a
	0x00, //curve20_b
	0x20, //curve20_a
	0x00, //curve21_b
	0x20, //curve21_a
	0x00, //curve22_b
	0x20, //curve22_a
	0x00, //curve23_b
	0x20, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x20, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x00, //ascr_skin_Rg
	0x00, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end
};
#endif
char LOCAL_CE_1[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x00, //ascr_roi 1 ascr 00 1 0
	0x33, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

char LOCAL_CE_2[] = {
	0xEC,
	0x06, //lce_gain 00 0000
	0x30, //lce_color_gain 00 0000
	0x00, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0x79, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0xbf,
	0x00, //lce_ref_gain 9
	0xb0,
	0x77, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x7f, //lce_bin_size_ratio
	0x00, //lce_dark_th 000
	0x40, //lce_min_ref_offset
	0x05, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x90,
	0x00, //curve_1_b
	0x7b, //curve_1_a
	0x03, //curve_2_b
	0x48, //curve_2_a
	0x08, //curve_3_b
	0x32, //curve_3_a
	0x08, //curve_4_b
	0x32, //curve_4_a
	0x08, //curve_5_b
	0x32, //curve_5_a
	0x08, //curve_6_b
	0x32, //curve_6_a
	0x08, //curve_7_b
	0x32, //curve_7_a
	0x10, //curve_8_b
	0x28, //curve_8_a
	0x10, //curve_9_b
	0x28, //curve_9_a
	0x10, //curve10_b
	0x28, //curve10_a
	0x10, //curve11_b
	0x28, //curve11_a
	0x10, //curve12_b
	0x28, //curve12_a
	0x19, //curve13_b
	0x22, //curve13_a
	0x19, //curve14_b
	0x22, //curve14_a
	0x19, //curve15_b
	0x22, //curve15_a
	0x19, //curve16_b
	0x22, //curve16_a
	0x19, //curve17_b
	0x22, //curve17_a
	0x19, //curve18_b
	0x22, //curve18_a
	0x23, //curve19_b
	0x1e, //curve19_a
	0x2e, //curve20_b
	0x1b, //curve20_a
	0x33, //curve21_b
	0x1a, //curve21_a
	0x40, //curve22_b
	0x18, //curve22_a
	0x48, //curve23_b
	0x17, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x2f, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x4e, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x5f, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x1a,
	0x74,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x15,
	0x8f,
	0xff, //ascr_skin_Rr
	0x20, //ascr_skin_Rg
	0x20, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end
};

char LOCAL_CE_1_ENHENCED[] = {
	//start
	0xEB,
	0x01, //mdnie_en
	0x00, //data_width mask 00 0000
	0x03, //ascr_roi 1 ascr 00 1 0
	0x33, //algo_roi 1 algo lce_roi 1 lce 00 1 0 00 1 0
	0x00, //roi_ctrl 00
	0x00, //roi0_x_start 12
	0x00,
	0x00, //roi0_x_end
	0x00,
	0x00, //roi0_y_start
	0x00,
	0x00, //roi0_y_end
	0x00,
	0x00, //roi1_x_strat
	0x00,
	0x00, //roi1_x_end
	0x00,
	0x00, //roi1_y_start
	0x00,
	0x00, //roi1_y_end
	0x00,
};

char LOCAL_CE_2_ENHENCED[] = {
	0xEC,
	0x06, //lce_gain 00 0000
	0x30, //lce_color_gain 00 0000
	0x00, //lce_scene_change_on scene_trans 0 0000
	0x14, //lce_min_diff
	0x90, //lce_illum_gain
	0x01, //lce_ref_offset 9
	0xbf,
	0x00, //lce_ref_gain 9
	0xb0,
	0x77, //lce_block_size h v 0000 0000
	0xfa, //lce_bright_th
	0x7f, //lce_bin_size_ratio
	0x00, //lce_dark_th 000
	0x40, //lce_min_ref_offset
	0x07, //nr sharp cs gamma 0000
	0xff, //nr_mask_th
	0x00, //sharpen_weight 10
	0x40,
	0x00, //sharpen_maxplus 11
	0xa0,
	0x00, //sharpen_maxminus 11
	0xa0,
	0x01, //cs_gain 10
	0x40,
	0x00, //curve_1_b
	0x7b, //curve_1_a
	0x03, //curve_2_b
	0x48, //curve_2_a
	0x08, //curve_3_b
	0x32, //curve_3_a
	0x08, //curve_4_b
	0x32, //curve_4_a
	0x08, //curve_5_b
	0x32, //curve_5_a
	0x08, //curve_6_b
	0x32, //curve_6_a
	0x08, //curve_7_b
	0x32, //curve_7_a
	0x10, //curve_8_b
	0x28, //curve_8_a
	0x10, //curve_9_b
	0x28, //curve_9_a
	0x10, //curve10_b
	0x28, //curve10_a
	0x10, //curve11_b
	0x28, //curve11_a
	0x10, //curve12_b
	0x28, //curve12_a
	0x19, //curve13_b
	0x22, //curve13_a
	0x70, //curve14_b
	0xf7, //curve14_a
	0x70, //curve15_b
	0xf7, //curve15_a
	0x70, //curve16_b
	0xf7, //curve16_a
	0x70, //curve17_b
	0xf7, //curve17_a
	0x66, //curve18_b
	0x1a, //curve18_a
	0x76, //curve19_b
	0x14, //curve19_a
	0x82, //curve20_b
	0x11, //curve20_a
	0x92, //curve21_b
	0x0e, //curve21_a
	0x98, //curve22_b
	0x0d, //curve22_a
	0x9f, //curve23_b
	0x0c, //curve23_a
	0x00, //curve24_b
	0xFF, //curve24_a
	0x30, //ascr_skin_on strength 0 00000
	0x67, //ascr_skin_cb
	0xa9, //ascr_skin_cr
	0x17, //ascr_dist_up
	0x29, //ascr_dist_down
	0x19, //ascr_dist_right
	0x27, //ascr_dist_left
	0x00, //ascr_div_up 20
	0x59,
	0x0b,
	0x00, //ascr_div_down
	0x31,
	0xf4,
	0x00, //ascr_div_right
	0x51,
	0xec,
	0x00, //ascr_div_left
	0x34,
	0x83,
	0xff, //ascr_skin_Rr
	0x50, //ascr_skin_Rg
	0x60, //ascr_skin_Rb
	0xff, //ascr_skin_Yr
	0xff, //ascr_skin_Yg
	0x00, //ascr_skin_Yb
	0xff, //ascr_skin_Mr
	0x00, //ascr_skin_Mg
	0xff, //ascr_skin_Mb
	0xff, //ascr_skin_Wr
	0xff, //ascr_skin_Wg
	0xff, //ascr_skin_Wb
	0x00, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x00, //ascr_Rg
	0xff, //ascr_Cb
	0x00, //ascr_Rb
	0xff, //ascr_Mr
	0x00, //ascr_Gr
	0x00, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x00, //ascr_Gb
	0xff, //ascr_Yr
	0x00, //ascr_Br
	0xff, //ascr_Yg
	0x00, //ascr_Bg
	0x00, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xff, //ascr_Wg
	0x00, //ascr_Kg
	0xff, //ascr_Wb
	0x00, //ascr_Kb
	//end
};



char *blind_tune_value[ACCESSIBILITY_MAX][2] = {
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
		{GRAYSCALE_1, GRAYSCALE_2},
		{GRAYSCALE_NEGATIVE_1, GRAYSCALE_NEGATIVE_2}
};

char *mdnie_tune_value[MAX_mDNIe_MODE][MAX_BACKGROUND_MODE][MAX_OUTDOOR_MODE][2] = {
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
			{{DYNAMIC_VIDEO_1, DYNAMIC_VIDEO_2}, {OUTDOOR_VIDEO_1, OUTDOOR_VIDEO_2}},
			{{STANDARD_VIDEO_1, STANDARD_VIDEO_2}, {OUTDOOR_VIDEO_1, OUTDOOR_VIDEO_2}},
			{{NATURAL_VIDEO_1, NATURAL_VIDEO_2}, {OUTDOOR_VIDEO_1, OUTDOOR_VIDEO_2}},
			{{MOVIE_VIDEO_1, MOVIE_VIDEO_2}, {OUTDOOR_VIDEO_1, OUTDOOR_VIDEO_2}},
			{{AUTO_VIDEO_1, AUTO_VIDEO_2}, {OUTDOOR_VIDEO_1, OUTDOOR_VIDEO_2}},
		},
		// VIDEO_WARM_APP
		{
			{{WARM_1, WARM_2}, {WARM_OUTDOOR_1, WARM_OUTDOOR_2}},
			{{WARM_1, WARM_2}, {WARM_OUTDOOR_1, WARM_OUTDOOR_2}},
			{{WARM_1, WARM_2}, {WARM_OUTDOOR_1, WARM_OUTDOOR_2}},
			{{WARM_1, WARM_2}, {WARM_OUTDOOR_1, WARM_OUTDOOR_2}},
			{{WARM_1, WARM_2}, {WARM_OUTDOOR_1, WARM_OUTDOOR_2}},
		},
		// VIDEO_COLD_APP
		{
			{{COLD_1, COLD_2}, {COLD_OUTDOOR_1, COLD_OUTDOOR_2}},
			{{COLD_1, COLD_2}, {COLD_OUTDOOR_1, COLD_OUTDOOR_2}},
			{{COLD_1, COLD_2}, {COLD_OUTDOOR_1, COLD_OUTDOOR_2}},
			{{COLD_1, COLD_2}, {COLD_OUTDOOR_1, COLD_OUTDOOR_2}},
			{{COLD_1, COLD_2}, {COLD_OUTDOOR_1, COLD_OUTDOOR_2}},
		},
		// CAMERA_APP
		{
			{{CAMERA_1, CAMERA_2}, {CAMERA_OUTDOOR_1, CAMERA_OUTDOOR_2}},
			{{CAMERA_1, CAMERA_2}, {CAMERA_OUTDOOR_1, CAMERA_OUTDOOR_2}},
			{{CAMERA_1, CAMERA_2}, {CAMERA_OUTDOOR_1, CAMERA_OUTDOOR_2}},
			{{CAMERA_1, CAMERA_2}, {CAMERA_OUTDOOR_1, CAMERA_OUTDOOR_2}},
			{{AUTO_CAMERA_1, AUTO_CAMERA_2}, {CAMERA_OUTDOOR_1, CAMERA_OUTDOOR_2}},
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
#if defined(CONFIG_LCD_HMT)
		// HMT GRAY 8
		{
			{{HMT_GRAY_8_1, HMT_GRAY_8_2}, {NULL, NULL}},
			{{HMT_GRAY_8_1, HMT_GRAY_8_2}, {NULL, NULL}},
			{{HMT_GRAY_8_1, HMT_GRAY_8_2}, {NULL, NULL}},
			{{HMT_GRAY_8_1, HMT_GRAY_8_2}, {NULL, NULL}},
			{{HMT_GRAY_8_1, HMT_GRAY_8_2}, {NULL, NULL}},
		},
		// HMT GRAY 16
		{
			{{HMT_GRAY_16_1, HMT_GRAY_16_2}, {NULL, NULL}},
			{{HMT_GRAY_16_1, HMT_GRAY_16_2}, {NULL, NULL}},
			{{HMT_GRAY_16_1, HMT_GRAY_16_2}, {NULL, NULL}},
			{{HMT_GRAY_16_1, HMT_GRAY_16_2}, {NULL, NULL}},
			{{HMT_GRAY_16_1, HMT_GRAY_16_2}, {NULL, NULL}},
		},
#endif
};

#endif
