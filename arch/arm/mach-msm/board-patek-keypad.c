
#include <linux/input/matrix_keypad.h>

static unsigned int mpq_row_gpios[] = {18, 24, 27, 77, 144};
static unsigned int mpq_col_gpios[] = {11, 12, 31, 32, 33};

static const unsigned int folder_keymap[] = {
/* KEY(row, col, keycode) */
	/* row = scan, col - sense */
	KEY(0, 0, KEY_SEND),
	KEY(0, 1, KEY_BACKSPACE),
	KEY(0, 2, KEY_HOMEPAGE),
	KEY(0, 3, KEY_NET_SEL),
	KEY(0, 4, KEY_ENTER),

	KEY(1, 0, KEY_1),
	KEY(1, 1, KEY_2),
	KEY(1, 2, KEY_3),
	KEY(1, 3, KEY_LEFT), //KEY_RIGHT),
	KEY(1, 4, KEY_DOWN),

	KEY(2, 0, KEY_4),
	KEY(2, 1, KEY_5),
	KEY(2, 2, KEY_6),
	KEY(2, 3, KEY_BACK),
	KEY(2, 4, KEY_RIGHT), //KEY_LEFT),

	KEY(3, 0, KEY_7),
	KEY(3, 1, KEY_8),
	KEY(3, 2, KEY_9),
	KEY(3, 3, KEY_UP),

	KEY(4, 0, KEY_NUMERIC_STAR),
	KEY(4, 1, KEY_0),
	KEY(4, 2, KEY_NUMERIC_POUND),
	KEY(4, 3, KEY_RECENT),
};

static struct matrix_keymap_data folder_keymap_data = {
	.keymap		= folder_keymap,
	.keymap_size	= ARRAY_SIZE(folder_keymap),
};

static struct matrix_keypad_platform_data folder_keypad_data = {
	.keymap_data		= &folder_keymap_data,
	.row_gpios		= mpq_row_gpios,
	.col_gpios		= mpq_col_gpios,
	.num_row_gpios		= ARRAY_SIZE(mpq_row_gpios),
	.num_col_gpios		= ARRAY_SIZE(mpq_col_gpios),
	.col_scan_delay_us		= 5000, //32000,
	.debounce_ms		= 10, //20,
	.wakeup			= 1,
	.active_low		= 1,
	.no_autorepeat		= 1,
};

static struct platform_device folder_keypad_device = {
	.name           = "patek_3x4_keypad", //"montblanc_3x4_keypad",
	.id             = -1,
	.dev            = {
		.platform_data  = &folder_keypad_data,
	},
};
