/* kernel/arch/arm/mach-ux500/gpio-kumquat.c
 *
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * Authors: Sun Yi <yi3.sun@sonyericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/string.h>

#include <plat/pincfg.h>
#include <plat/gpio.h>
#include <mach/hardware.h>
#include <mach/suspend.h>

#include "pins-db8500.h"
#include "pins.h"

enum custom_pin_cfg_t {
	PINS_FOR_DEFAULT,
	PINS_FOR_U9500_21,
};

static enum custom_pin_cfg_t pinsfor = PINS_FOR_DEFAULT;
static pin_cfg_t mop500_pins_default[] = {

	GPIO4_GPIO		|	PIN_INPUT_PULLUP,
	GPIO5_GPIO		|	PIN_OUTPUT_LOW,
	GPIO6_IP_GPIO0		|	PIN_OUTPUT_LOW,
	GPIO7_GPIO		|	PIN_INPUT_PULLUP,
	GPIO8_IPI2C_SDA,
	GPIO9_IPI2C_SCL,

	GPIO12_MSP0_TXD,
	GPIO13_MSP0_TFS,
	GPIO14_MSP0_TCK,
	GPIO15_MSP0_RXD,
	GPIO16_GPIO		|	PIN_OUTPUT_LOW,
	GPIO17_GPIO		|	PIN_INPUT_PULLUP,
	GPIO18_U2_RXD		|	PIN_INPUT_PULLUP,
	GPIO19_U2_TXD		|	PIN_OUTPUT_HIGH,
	GPIO20_GPIO		|	PIN_OUTPUT_LOW,
	GPIO21_GPIO		|	PIN_INPUT_PULLUP,
	GPIO22_GPIO		|	PIN_OUTPUT_LOW,
	GPIO24_GPIO		|	PIN_INPUT_PULLUP,

	GPIO29_SPI3_CLK,
	GPIO30_SPI3_RXD		|	PIN_PULL_DOWN,
	GPIO31_GPIO		|	PIN_OUTPUT_HIGH,
	GPIO32_SPI3_TXD,
	GPIO33_MSP1_TXD,
	GPIO34_MSP1_TFS,
	GPIO35_MSP1_TCK,
	GPIO36_MSP1_RXD,

	GPIO64_GPIO		|	PIN_OUTPUT_LOW,
	GPIO65_GPIO		|	PIN_OUTPUT_LOW,
	GPIO66_GPIO		|	PIN_INPUT_PULLUP,
	GPIO67_GPIO		|	PIN_OUTPUT_LOW,
	GPIO68_LCD_VSI0		|	PIN_INPUT_PULLUP,
	GPIO69_LCD_VSI1,

	GPIO75_GPIO		|	PIN_INPUT_PULLDOWN,
	GPIO76_U2_TXD,
	GPIO77_GPIO		|	PIN_INPUT_PULLUP,
	GPIO78_GPIO		|	PIN_INPUT_PULLUP,
	GPIO79_GPIO		|	PIN_INPUT_PULLUP,
	GPIO80_GPIO		|	PIN_INPUT_PULLUP,
	GPIO81_GPIO		|	PIN_INPUT_PULLUP,
	GPIO82_GPIO		|	PIN_INPUT_PULLUP,
	GPIO83_GPIO		|	PIN_INPUT_PULLUP,
	GPIO84_GPIO		|	PIN_INPUT_PULLUP,
	GPIO85_GPIO		|	PIN_INPUT_PULLUP,

	GPIO86_GPIO		|	PIN_OUTPUT_LOW,
	GPIO87_GPIO		|	PIN_INPUT_PULLUP,
	GPIO88_GPIO		|	PIN_INPUT_PULLUP,
	GPIO89_GPIO		|	PIN_INPUT_PULLUP,
	GPIO90_SM_ADQ4,
	GPIO91_GPIO		|	PIN_INPUT_PULLUP,
	GPIO92_GPIO		|	PIN_INPUT_PULLUP,
	GPIO93_GPIO		|	PIN_INPUT_PULLUP,
	GPIO94_GPIO		|	PIN_OUTPUT_LOW,
	GPIO95_GPIO		|	PIN_INPUT_PULLUP,
	GPIO96_GPIO		|	PIN_INPUT_NOPULL,
	GPIO97_GPIO		|	PIN_INPUT_PULLUP,
	GPIO130_GPIO		|	PIN_INPUT_PULLUP,
	GPIO139_GPIO		|	PIN_INPUT_PULLUP,
	GPIO140_GPIO		|	PIN_INPUT_PULLUP,
	GPIO141_GPIO 		| 	PIN_OUTPUT_LOW,
	GPIO142_GPIO 		| 	PIN_OUTPUT_HIGH,
	GPIO143_SSP0_CLK,
	GPIO144_SSP0_FRM,
	GPIO145_SSP0_RXD	|	PIN_INPUT_PULLDOWN,
	GPIO146_SSP0_TXD,

	GPIO147_GPIO		|	PIN_INPUT_PULLUP,
	GPIO148_GPIO		|	PIN_INPUT_PULLUP,
	GPIO149_GPIO		|	PIN_INPUT_PULLUP,
	GPIO150_GPIO		|	PIN_INPUT_PULLUP,

	GPIO169_GPIO		|	PIN_INPUT_NOPULL,
	GPIO170_GPIO		|	PIN_OUTPUT_LOW,
	GPIO171_GPIO		|	PIN_INPUT_PULLUP,

	GPIO192_GPIO		|	PIN_INPUT_PULLUP,
	GPIO193_GPIO		|	PIN_INPUT_PULLUP,
	GPIO194_GPIO		|	PIN_INPUT_PULLUP,
	GPIO195_GPIO		|	PIN_INPUT_PULLUP,
	GPIO196_GPIO		|	PIN_INPUT_PULLUP,

	GPIO226_GPIO		|	PIN_INPUT_PULLUP,
	GPIO227_GPIO 		| 	PIN_OUTPUT_LOW,
	GPIO228_GPIO 		| 	PIN_OUTPUT_LOW
};

/* USB */
static UX500_PINS(mop500_pins_usb,
	GPIO256_USB_NXT		| PIN_SLPM_INPUT_PULLDOWN,
	GPIO257_USB_STP		| PIN_SLPM_OUTPUT_HIGH | PIN_SLPM_PDIS_ENABLED,
	GPIO258_USB_XCLK	| PIN_SLPM_INPUT_PULLDOWN,
	GPIO259_USB_DIR		| PIN_SLPM_INPUT_PULLDOWN,
	GPIO260_USB_DAT7	| PIN_SLPM_INPUT_PULLDOWN,
	GPIO261_USB_DAT6	| PIN_SLPM_INPUT_PULLDOWN,
	GPIO262_USB_DAT5	| PIN_SLPM_INPUT_PULLDOWN,
	GPIO263_USB_DAT4	| PIN_SLPM_INPUT_PULLDOWN,
	GPIO264_USB_DAT3	| PIN_SLPM_INPUT_PULLDOWN,
	GPIO265_USB_DAT2	| PIN_SLPM_INPUT_PULLDOWN,
	GPIO266_USB_DAT1	| PIN_SLPM_INPUT_PULLDOWN,
	GPIO267_USB_DAT0	| PIN_SLPM_INPUT_PULLDOWN,
);

static pin_cfg_t u9500_21_pins[] = {
	GPIO4_U1_RXD    | PIN_INPUT_PULLUP,
	GPIO5_U1_TXD    | PIN_OUTPUT_HIGH,
};

static UX500_PINS(mop500_pins_uart0,
	GPIO0_U0_CTSn   | PIN_INPUT_PULLUP,
	GPIO1_U0_RTSn   | PIN_OUTPUT_HIGH,
	GPIO2_U0_RXD    | PIN_INPUT_PULLUP,
	GPIO3_U0_TXD    | PIN_OUTPUT_HIGH,
);

/* sdi1 - WLAN */
static UX500_PINS(mop500_pins_sdi1,
	GPIO208_MC1_CLK,
	GPIO209_GPIO		|	PIN_INPUT_PULLUP,
	GPIO210_MC1_CMD		|	PIN_INPUT_PULLUP,
	GPIO211_MC1_DAT0	|	PIN_INPUT_PULLUP,
	GPIO212_MC1_DAT1	|	PIN_INPUT_PULLUP,
	GPIO213_MC1_DAT2	|	PIN_INPUT_PULLUP,
	GPIO214_MC1_DAT3	|	PIN_INPUT_PULLUP,

);

/* sdi2 - POP eMMC */
static UX500_PINS(mop500_pins_sdi2,
	GPIO128_MC2_CLK		|	PIN_OUTPUT_LOW,
	GPIO129_MC2_CMD		|	PIN_INPUT_PULLUP,
/*	GPIO130_MC2_FBCLK	|	PIN_INPUT_NOPULL, */
	GPIO131_MC2_DAT0	|	PIN_INPUT_PULLUP,
	GPIO132_MC2_DAT1	|	PIN_INPUT_PULLUP,
	GPIO133_MC2_DAT2	|	PIN_INPUT_PULLUP,
	GPIO134_MC2_DAT3	|	PIN_INPUT_PULLUP,
	GPIO135_MC2_DAT4	|	PIN_INPUT_PULLUP,
	GPIO136_MC2_DAT5	|	PIN_INPUT_PULLUP,
	GPIO137_MC2_DAT6	|	PIN_INPUT_PULLUP,
	GPIO138_MC2_DAT7	|	PIN_INPUT_PULLUP,
);


/* sdi3 - SD/MMC card */
/* remove in kumquat for we don't have sdcard.*/
#if 0
static UX500_PINS(mop500_pins_sdi3,
	GPIO84_GPIO		|	PIN_INPUT_NOPULL,
	GPIO85_GPIO		|	PIN_OUTPUT_LOW,
	GPIO139_GPIO		|	PIN_OUTPUT_LOW,
	GPIO215_MC3_DAT2DIR	|	PIN_OUTPUT_HIGH,
	GPIO216_MC3_CMDDIR	|	PIN_OUTPUT_LOW,
	GPIO217_GPIO		|	PIN_OUTPUT_LOW,
	GPIO218_MC3_DAT0DIR	|	PIN_OUTPUT_HIGH,
	GPIO219_MC3_CLK		|	PIN_OUTPUT_LOW,
	GPIO220_MC3_FBCLK	|	PIN_INPUT_NOPULL,
	GPIO221_MC3_CMD		|	PIN_INPUT_PULLUP,
	GPIO222_MC3_DAT0	|	PIN_INPUT_PULLUP,
	GPIO223_MC3_DAT1	|	PIN_INPUT_PULLUP,
	GPIO224_MC3_DAT2	|	PIN_INPUT_PULLUP,
	GPIO225_MC3_DAT3	|	PIN_INPUT_PULLUP,
);
#endif
/*
 * I2C
 */

/* remove according to spec.*/
/*
static UX500_PINS(mop500_pins_i2c0,
	GPIO147_I2C0_SCL,
	GPIO148_I2C0_SDA,
);
*/

/*
static UX500_PINS(mop500_pins_i2c1,
	GPIO16_I2C1_SCL,
	GPIO17_I2C1_SDA,
);
*/

static UX500_PINS(mop500_pins_i2c2,
	GPIO10_I2C2_SDA,
	GPIO11_I2C2_SCL,
);

/*remove it, no NFC*/
/*
static UX500_PINS(mop500_pins_i2c3,
	GPIO229_I2C3_SDA,
	GPIO230_I2C3_SCL,
);
*/

static UX500_PINS(mop500_pins_ske,
GPIO153_KP_I7 | PIN_INPUT_PULLDOWN | PIN_SLPM_INPUT_PULLUP,
GPIO154_KP_I6 | PIN_INPUT_PULLDOWN | PIN_SLPM_INPUT_PULLUP,
GPIO155_KP_I5 | PIN_INPUT_PULLDOWN | PIN_SLPM_INPUT_PULLUP,
GPIO156_KP_I4 | PIN_INPUT_PULLDOWN | PIN_SLPM_INPUT_PULLUP,
GPIO161_KP_I3 | PIN_INPUT_PULLDOWN | PIN_SLPM_INPUT_PULLUP,
GPIO162_KP_I2 | PIN_INPUT_PULLDOWN | PIN_SLPM_INPUT_PULLUP,
GPIO163_KP_I1 | PIN_INPUT_PULLDOWN | PIN_SLPM_INPUT_PULLUP,
GPIO164_KP_I0 | PIN_INPUT_PULLDOWN | PIN_SLPM_INPUT_PULLUP,
GPIO157_KP_O7 | PIN_INPUT_PULLUP | PIN_SLPM_OUTPUT_LOW,
GPIO158_KP_O6 | PIN_INPUT_PULLUP | PIN_SLPM_OUTPUT_LOW,
GPIO159_KP_O5 | PIN_INPUT_PULLUP | PIN_SLPM_OUTPUT_LOW,
GPIO160_KP_O4 | PIN_INPUT_PULLUP | PIN_SLPM_OUTPUT_LOW,
GPIO165_KP_O3 | PIN_INPUT_PULLUP | PIN_SLPM_OUTPUT_LOW,
GPIO166_KP_O2 | PIN_INPUT_PULLUP | PIN_SLPM_OUTPUT_LOW,
GPIO167_KP_O1 | PIN_INPUT_PULLUP | PIN_SLPM_OUTPUT_LOW,
GPIO168_KP_O0 | PIN_INPUT_PULLUP | PIN_SLPM_OUTPUT_LOW,
);

static struct ux500_pin_lookup mop500_pins[] = {
	PIN_LOOKUP("uart0", &mop500_pins_uart0),
	/*PIN_LOOKUP("nmk-i2c.0", &mop500_pins_i2c0),*/
	/*PIN_LOOKUP("nmk-i2c.1", &mop500_pins_i2c1),*/
	PIN_LOOKUP("nmk-i2c.2", &mop500_pins_i2c2),
/*	PIN_LOOKUP("nmk-i2c.3", &mop500_pins_i2c3),*/
	PIN_LOOKUP("ske", &mop500_pins_ske),
	PIN_LOOKUP("sdi1", &mop500_pins_sdi1),
	PIN_LOOKUP("sdi2", &mop500_pins_sdi2),
/*	PIN_LOOKUP("sdi3", &mop500_pins_sdi3),*/
	PIN_LOOKUP("ab8500-usb.0", &mop500_pins_usb),
};


/*
 * This function is called to force gpio power save
 * settings during suspend.
 * This is a temporary solution until all drivers are
 * controlling their pin settings when in inactive mode.
 */
static void mop500_pins_suspend_force(void)
{
	u32 bankaddr;
	u32 w_imsc;
	u32 imsc;
	u32 mask;

	/*
	 * Apply HSI GPIO Config for DeepSleep
	 *
	 * Bank0
	 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK0_BASE);

	w_imsc = readl(bankaddr + NMK_GPIO_RWIMSC) |
		readl(bankaddr + NMK_GPIO_FWIMSC);

	imsc = readl(bankaddr + NMK_GPIO_RIMSC) |
		readl(bankaddr + NMK_GPIO_FIMSC);

	mask = 0;
	writel(0x409C702A & ~w_imsc & ~mask, bankaddr + NMK_GPIO_DIR);
	writel(0x001C0022 & ~w_imsc & ~mask, bankaddr + NMK_GPIO_DATS);
	writel(0x807000 & ~w_imsc & ~mask, bankaddr + NMK_GPIO_DATC);
	writel(0x5FFFFFFF & ~w_imsc & ~imsc & ~mask, bankaddr + NMK_GPIO_PDIS);
	writel(readl(bankaddr + NMK_GPIO_SLPC) & mask,
	       bankaddr + NMK_GPIO_SLPC);

	/* Bank1 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK1_BASE);

	w_imsc = readl(bankaddr + NMK_GPIO_RWIMSC) |
		readl(bankaddr + NMK_GPIO_FWIMSC);

	imsc = readl(bankaddr + NMK_GPIO_RIMSC) |
		readl(bankaddr + NMK_GPIO_FIMSC);

	writel(0x3        & ~w_imsc, bankaddr + NMK_GPIO_DIRS);
	writel(0x1C      , bankaddr + NMK_GPIO_DIRC);
	writel(0x2        & ~w_imsc, bankaddr + NMK_GPIO_DATC);
	writel(0xFFFFFFFF & ~w_imsc & ~imsc, bankaddr + NMK_GPIO_PDIS);
	writel(0         , bankaddr + NMK_GPIO_SLPC);

	/* Bank2 */

	bankaddr = IO_ADDRESS(U8500_GPIOBANK2_BASE);

	w_imsc = readl(bankaddr + NMK_GPIO_RWIMSC) |
		readl(bankaddr + NMK_GPIO_FWIMSC);

	imsc = readl(bankaddr + NMK_GPIO_RIMSC) |
		readl(bankaddr + NMK_GPIO_FIMSC);

	mask = 0;
	writel(0x3D7C0    & ~w_imsc & ~mask, bankaddr + NMK_GPIO_DIRS);
	writel(0x803C2830 & ~mask, bankaddr + NMK_GPIO_DIRC);
	writel(0x3D7C0    & ~w_imsc  & ~mask, bankaddr + NMK_GPIO_DATC);
	writel(0xFFFFFFFF & ~w_imsc & ~imsc & ~mask, bankaddr + NMK_GPIO_PDIS);
	/*
	 * No need to set SLPC (SLPM) register. This can break modem STM
	 * settings on pins (70-76) because modem is special and needs to
	 * have its mux connected even in suspend because modem could still
	 * be on and might send interesting STM debugging data.
	 */

	/* Bank3 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK3_BASE);

	w_imsc = readl(bankaddr + NMK_GPIO_RWIMSC) |
		readl(bankaddr + NMK_GPIO_FWIMSC);

	imsc = readl(bankaddr + NMK_GPIO_RIMSC) |
		readl(bankaddr + NMK_GPIO_FIMSC);

	writel(0x3        & ~w_imsc, bankaddr + NMK_GPIO_DIRS);
	writel(0x3        & ~w_imsc, bankaddr + NMK_GPIO_DATC);
	writel(0xFFFFFFFF & ~w_imsc & ~imsc, bankaddr + NMK_GPIO_PDIS);
	writel(0         , bankaddr + NMK_GPIO_SLPC);

	/* Bank4 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK4_BASE);

	w_imsc = readl(bankaddr + NMK_GPIO_RWIMSC) |
		readl(bankaddr + NMK_GPIO_FWIMSC);

	imsc = readl(bankaddr + NMK_GPIO_RIMSC) |
		readl(bankaddr + NMK_GPIO_FIMSC);

	writel(0x5E000    & ~w_imsc, bankaddr + NMK_GPIO_DIRS);
	writel(0xFFDA1800 , bankaddr + NMK_GPIO_DIRC);
	writel(0x4E000    & ~w_imsc, bankaddr + NMK_GPIO_DATC);
	writel(0x10000    & ~w_imsc, bankaddr + NMK_GPIO_DATS);
	writel(0xFFFFFFF9 & ~w_imsc & ~imsc, bankaddr + NMK_GPIO_PDIS);
	writel(0         , bankaddr + NMK_GPIO_SLPC);

	/* Bank5 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK5_BASE);

	w_imsc = readl(bankaddr + NMK_GPIO_RWIMSC) |
		readl(bankaddr + NMK_GPIO_FWIMSC);

	imsc = readl(bankaddr + NMK_GPIO_RIMSC) |
		readl(bankaddr + NMK_GPIO_FIMSC);

	writel(0x3FF     , bankaddr + NMK_GPIO_DIRC);
	writel(0xFFFFFFFF & ~w_imsc & ~imsc, bankaddr + NMK_GPIO_PDIS);
	writel(0         , bankaddr + NMK_GPIO_SLPC);

	/* Bank6 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK6_BASE);

	w_imsc = readl(bankaddr + NMK_GPIO_RWIMSC) |
		readl(bankaddr + NMK_GPIO_FWIMSC);

	imsc = readl(bankaddr + NMK_GPIO_RIMSC) |
		readl(bankaddr + NMK_GPIO_FIMSC);

	mask = 0;
	/* Mask away SDI 4 pins (197..207) */
	mask |= 0xffe0;
	writel(0x8810810  & ~w_imsc & ~mask, bankaddr + NMK_GPIO_DIRS);
	writel(0xF57EF7EF & ~mask, bankaddr + NMK_GPIO_DIRC);
	writel(0x8810810  & ~w_imsc & ~mask, bankaddr + NMK_GPIO_DATC);
	writel(0xFFFFFFFF & ~w_imsc & ~imsc & ~mask, bankaddr + NMK_GPIO_PDIS);
	writel(readl(bankaddr + NMK_GPIO_SLPC) & mask,
	       bankaddr + NMK_GPIO_SLPC);


	/* Bank7 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK7_BASE);

	w_imsc = readl(bankaddr + NMK_GPIO_RWIMSC) |
		readl(bankaddr + NMK_GPIO_FWIMSC);

	imsc = readl(bankaddr + NMK_GPIO_RIMSC) |
		readl(bankaddr + NMK_GPIO_FIMSC);

	writel(0x1C       & ~w_imsc, bankaddr + NMK_GPIO_DIRS);
	writel(0x63      , bankaddr + NMK_GPIO_DIRC);
	writel(0x18       & ~w_imsc, bankaddr + NMK_GPIO_DATC);
	writel(0xFFFFFFFF & ~w_imsc & ~imsc, bankaddr + NMK_GPIO_PDIS);
	writel(0         , bankaddr + NMK_GPIO_SLPC);

	/* Bank8 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK8_BASE);

	w_imsc = readl(bankaddr + NMK_GPIO_RWIMSC) |
		readl(bankaddr + NMK_GPIO_FWIMSC);

	imsc = readl(bankaddr + NMK_GPIO_RIMSC) |
		readl(bankaddr + NMK_GPIO_FIMSC);

	writel(0x2        & ~w_imsc, bankaddr + NMK_GPIO_DIRS);
	writel(0xFF0     , bankaddr + NMK_GPIO_DIRC);
	writel(0x2        & ~w_imsc, bankaddr + NMK_GPIO_DATS);
	writel(0x2        & ~w_imsc & ~imsc, bankaddr + NMK_GPIO_PDIS);
	writel(0         , bankaddr + NMK_GPIO_SLPC);
}


/*
 * This function is called to force gpio power save
 * mux settings during suspend.
 * This is a temporary solution until all drivers are
 * controlling their pin settings when in inactive mode.
 */
static void mop500_pins_suspend_force_mux(void)
{
	u32 bankaddr;


	/*
	 * Apply HSI GPIO Config for DeepSleep
	 *
	 * Bank0
	 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK0_BASE);

	writel(0xE0000000, bankaddr + NMK_GPIO_AFSLA);
	writel(0xE0000000, bankaddr + NMK_GPIO_AFSLB);

	/* Bank1 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK1_BASE);

	writel(0x1       , bankaddr + NMK_GPIO_AFSLA);
	writel(0x1       , bankaddr + NMK_GPIO_AFSLB);

	/* Bank2 (Nothing needs to be done) */

	/* Bank3 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK3_BASE);

	writel(0         , bankaddr + NMK_GPIO_AFSLA);
	writel(0         , bankaddr + NMK_GPIO_AFSLB);

	/* Bank4 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK4_BASE);

	writel(0x7FF     , bankaddr + NMK_GPIO_AFSLA);
	writel(0         , bankaddr + NMK_GPIO_AFSLB);

	/* Bank5 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK5_BASE);

	writel(0         , bankaddr + NMK_GPIO_AFSLA);
	writel(0         , bankaddr + NMK_GPIO_AFSLB);

	/* Bank6 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK6_BASE);

	writel(0         , bankaddr + NMK_GPIO_AFSLA);
	writel(0         , bankaddr + NMK_GPIO_AFSLB);

	/* Bank7 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK7_BASE);

	writel(0         , bankaddr + NMK_GPIO_AFSLA);
	writel(0         , bankaddr + NMK_GPIO_AFSLB);

	/* Bank8 */
	bankaddr = IO_ADDRESS(U8500_GPIOBANK8_BASE);

	writel(0         , bankaddr + NMK_GPIO_AFSLA);
	writel(0         , bankaddr + NMK_GPIO_AFSLB);

}

/*
 * passing "pinsfor=" in kernel cmdline allows for custom
 * configuration of GPIOs on u8500 derived boards.
 */
static int __init early_pinsfor(char *p)
{
	pinsfor = PINS_FOR_DEFAULT;

	if (strcmp(p, "u9500-21") == 0)
		pinsfor = PINS_FOR_U9500_21;

	return 0;
}
early_param("pinsfor", early_pinsfor);

void __init mop500_pins_init(void)
{
	nmk_config_pins(mop500_pins_default, ARRAY_SIZE(mop500_pins_default));
	ux500_pins_add(mop500_pins, ARRAY_SIZE(mop500_pins));

	switch (pinsfor) {
	case PINS_FOR_U9500_21:
		nmk_config_pins(u9500_21_pins, ARRAY_SIZE(u9500_21_pins));
		break;

	case PINS_FOR_DEFAULT:
	default:
		break;
	}
}
