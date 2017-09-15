/* platform data for the PCAL6416A 16-bit I/O expander driver */

#ifndef _PCAL6416A_H_
#define _PCAL6416A_H_

#define PCAL6416A_INPUT			0x00  /* Input port [RO]                            */
#define PCAL6416A_DAT_OUT		0x02  /* GPIO DATA OUT [R/W]                        */
#define PCAL6416A_POLARITY		0x04  /* Polarity Inversion port [R/W]              */
#define PCAL6416A_CONFIG		0x06  /* Configuration port [R/W]                   */
#define PCAL6416A_DRIVE0		0x40  /* Output drive strength register Port0 [R/W] */
#define PCAL6416A_DRIVE1		0x42  /* Output drive strength register Port1 [R/W] */
#define PCAL6416A_INPUT_LATCH		0x44  /* Port0 Input latch register  [R/W]          */
#define PCAL6416A_EN_PULLUPDOWN		0x46  /* Port0 Pull-up/Pull-down Enable [R/W]       */
#define PCAL6416A_SEL_PULLUPDOWN	0x48  /* Port0 Pull-up/Pull-down selection [R/W]    */
#define PCAL6416A_INT_MASK		0x4A  /* Interrupt mask register [R/W]              */
#define PCAL6416A_INT_STATUS		0x4C  /* Interrupt status register [RO]             */
#define PCAL6416A_OUTPUT_CONFIG		0x4F  /* Output port configuration register [R/W]   */ 

#define NO_PULL				0x00
#define PULL_DOWN			0x01
#define PULL_UP				0x02

/* EXPANDER GPIO Drive Strength */
enum {
	GPIO_CFG_6_25MA,
	GPIO_CFG_12_5MA,
	GPIO_CFG_18_75MA,
	GPIO_CFG_25MA,
};

struct pcal6416a_platform_data {
	/* number of the first GPIO */
	unsigned gpio_base;
	int gpio_start;
	int ngpio;
	int irq_base;
	int reset_gpio;
	int irq_gpio;
	uint16_t support_init;
	uint16_t init_config;
	uint16_t init_data_out;
	uint16_t init_en_pull;
	uint16_t init_sel_pull;
	const char *supply_name;
};

#ifdef CONFIG_SEC_PM_DEBUG
int expander_print_all(void);
#endif

#endif
