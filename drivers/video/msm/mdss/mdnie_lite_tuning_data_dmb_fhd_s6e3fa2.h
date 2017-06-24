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

#ifndef _MDNIE_LITE_TUNING_DATA_DMB_FHD_S6E3FA2_H_
#define _MDNIE_LITE_TUNING_DATA_DMB_FHD_S6E3FA2_H_

static char STANDARD_DMB_1[] = {
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

static char STANDARD_DMB_2[] = {
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
	0x01, //sharpen_weight 10
	0x00,
	0x01, //sharpen_maxplus 11
	0x00,
	0x01, //sharpen_maxminus 11
	0x00,
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

static char NATURAL_DMB_1[] = {
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

static char NATURAL_DMB_2[] = {
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
	0x80,
	0x01, //sharpen_maxplus 11
	0x00,
	0x01, //sharpen_maxminus 11
	0x00,
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

static char DYNAMIC_DMB_1[] = {
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

static char DYNAMIC_DMB_2[] = {
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
	0x01, //sharpen_weight 10
	0x20,
	0x01, //sharpen_maxplus 11
	0x00,
	0x01, //sharpen_maxminus 11
	0x00,
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

static char MOVIE_DMB_1[] = {
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

static char MOVIE_DMB_2[] = {
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
	0x80,
	0x01, //sharpen_maxplus 11
	0x00,
	0x01, //sharpen_maxminus 11
	0x00,
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
	0x82, //ascr_Cr
	0xff, //ascr_Rr
	0xff, //ascr_Cg
	0x2d, //ascr_Rg
	0xec, //ascr_Cb
	0x21, //ascr_Rb
	0xef, //ascr_Mr
	0x57, //ascr_Gr
	0x36, //ascr_Mg
	0xff, //ascr_Gg
	0xff, //ascr_Mb
	0x28, //ascr_Gb
	0xf7, //ascr_Yr
	0x34, //ascr_Br
	0xff, //ascr_Yg
	0x24, //ascr_Bg
	0x44, //ascr_Yb
	0xff, //ascr_Bb
	0xff, //ascr_Wr
	0x00, //ascr_Kr
	0xf8, //ascr_Wg
	0x00, //ascr_Kg
	0xef, //ascr_Wb
	0x00, //ascr_Kb
	//end

};

static char AUTO_DMB_1[] = {
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

static char AUTO_DMB_2[] = {
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
	0x01, //sharpen_weight 10
	0x00,
	0x01, //sharpen_maxplus 11
	0x00,
	0x01, //sharpen_maxminus 11
	0x00,
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
	0x40, //ascr_skin_Rg
	0x40, //ascr_skin_Rb
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



char *dmb_tune_value[MAX_DMB_MODE][MAX_BACKGROUND_MODE][MAX_OUTDOOR_MODE][2] = {
		/*
			DMB MODE (outdoor off/on)
			DMB_WARM MODE (outdoor off/on)
			DMB_COLD MODE (outdoor off/on)
		*/
		// DMB
		{
			{{DYNAMIC_DMB_1, DYNAMIC_DMB_2}, {DYNAMIC_DMB_1, DYNAMIC_DMB_2}},
			{{STANDARD_DMB_1, STANDARD_DMB_2}, {STANDARD_DMB_1, STANDARD_DMB_2}},
			{{NATURAL_DMB_1, NATURAL_DMB_2}, {NATURAL_DMB_1, NATURAL_DMB_2}},
			{{MOVIE_DMB_1, MOVIE_DMB_2}, {MOVIE_DMB_1, MOVIE_DMB_2}},
			{{AUTO_DMB_1, AUTO_DMB_2}, {AUTO_DMB_1, AUTO_DMB_2}},
		},
		// DMB_WARM
		{
			{{DYNAMIC_DMB_1, DYNAMIC_DMB_2}, {DYNAMIC_DMB_1, DYNAMIC_DMB_2}},
			{{STANDARD_DMB_1, STANDARD_DMB_2}, {STANDARD_DMB_1, STANDARD_DMB_2}},
			{{NATURAL_DMB_1, NATURAL_DMB_2}, {NATURAL_DMB_1, NATURAL_DMB_2}},
			{{MOVIE_DMB_1, MOVIE_DMB_2}, {MOVIE_DMB_1, MOVIE_DMB_2}},
			{{AUTO_DMB_1, AUTO_DMB_2}, {AUTO_DMB_1, AUTO_DMB_2}},
		},
		// DMB_COLD
		{
			{{DYNAMIC_DMB_1, DYNAMIC_DMB_2}, {DYNAMIC_DMB_1, DYNAMIC_DMB_2}},
			{{STANDARD_DMB_1, STANDARD_DMB_2}, {STANDARD_DMB_1, STANDARD_DMB_2}},
			{{NATURAL_DMB_1, NATURAL_DMB_2}, {NATURAL_DMB_1, NATURAL_DMB_2}},
			{{MOVIE_DMB_1, MOVIE_DMB_2}, {MOVIE_DMB_1, MOVIE_DMB_2}},
			{{AUTO_DMB_1, AUTO_DMB_2}, {AUTO_DMB_1, AUTO_DMB_2}},
		},
};


#endif

