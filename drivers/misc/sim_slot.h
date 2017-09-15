#ifdef CONFIG_CHECK_SIMSLOT_COUNT

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/i2c-gpio.h>
#include <asm/gpio.h>

/* below values would be change by H/W schematic of each model */
#ifdef CONFIG_CHECK_SIMSLOT_COUNT_GPIO
#define SIM_SLOT_PIN CONFIG_CHECK_SIMSLOT_COUNT_GPIO
#else
	# CONFIG_CHECK_SIMSLOT_COUNT_GPIO is not defined
#endif

#define SINGLE_SIM_VALUE 1
#define DUAL_SIM_VALUE 0



enum
{
        NO_SIM = 0,
        SINGLE_SIM,
        DUAL_SIM,
        TRIPLE_SIM
};

#endif
