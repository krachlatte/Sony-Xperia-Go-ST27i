/*
 * Copyright (C) 2009 ST-Ericsson SA
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/amba/bus.h>
#include <linux/amba/pl022.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/hsi/hsi.h>
#include <linux/i2s/i2s.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/input/matrix_keypad.h>
#include <linux/mfd/stmpe.h>
#include <linux/mfd/tc35892.h>
#include <linux/i2c/lp5521.h>
#include <linux/power_supply.h>
#include <linux/mfd/core.h>
#include <linux/mfd/abx500.h>
#include <linux/mfd/ab8500.h>
#include <linux/bootmem.h>
#include <linux/console.h>
#ifdef CONFIG_INPUT_LSM303DLH_ACCELEROMETER
#include  <linux/lsm303dlh_acc.h>
#endif
#ifdef CONFIG_INPUT_LSM303DLHC_ACCELEROMETER
#include  <linux/lsm303dlhc_acc.h>
#endif
#ifdef CONFIG_INPUT_LSM303DLH_MAGNETOMETER
#include  <linux/lsm303dlh_mag.h>
#endif
#ifdef CONFIG_INPUT_LPS331AP
#include <linux/lps331ap.h>
#endif
#ifdef CONFIG_INPUT_L3G4200D
#include  <linux/l3g4200d_gyr.h>
#endif
#include <linux/mmio.h>
#include <linux/spi/stm_msp.h>
#include <linux/leds_pwm.h>
#include <linux/pwm_backlight.h>
#include <linux/mfd/ab8500/denc.h>
#include <linux/mfd/ab8500/ab8500-gpio.h>
#include <linux/mfd/ab8500/sysctrl.h>
#include <linux/mfd/cg2900.h>

#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android_composite.h>
#endif
#ifdef CONFIG_USB_ANDROID_ACCESSORY
#include <linux/usb/f_accessory.h>
#endif

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci.h>

#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>

#include <plat/pincfg.h>
#include <plat/ske.h>
#include <plat/i2c.h>
#include <plat/ste_dma40.h>

#include <mach/devices.h>
#include <mach/sensors1p.h>
#include <mach/ste_audio_io.h>
#include <mach/setup.h>
#ifdef CONFIG_AV8100
#include <video/av8100.h>
#endif
#include <mach/devices.h>
#include <mach/irqs.h>
#include <mach/ste-dma40-db8500.h>
#ifdef CONFIG_DB8500_MLOADER
#include <mach/mloader-dbx500.h>
#endif
#ifdef CONFIG_U8500_SIM_DETECT
#include <mach/sim_detect.h>
#endif
#include <mach/crypto-ux500.h>

#include <sound/ux500_ab8500_ext.h>

#include "devices-cg2900.h"
#include "devices-db8500.h"
#include "board-mop500-regulators.h"
#include "regulator-u8500.h"
#include "pins-db8500.h"
#include "board-mop500-bm.h"
#include "board-mop500-wlan.h"
#include "board-mop500.h"
#include "board-rio-grande-keypad.h"
#include "board-rio-grande-leds.h"
#include "pins.h"

#ifdef CONFIG_TOUCHSCREEN_CYTTSP_SPI
#include <linux/cyttsp.h>
#include <linux/delay.h>
#include <mach/touch_panel.h>
#endif

#ifdef CONFIG_RMI4_BUS
#include <linux/rmi.h>
#endif

/* TODO: This is incorrectly switched! */
#ifdef CONFIG_SEMC_RMI4_BUS
#include <linux/rmi4/rmi4.h>
#endif
#if defined(CONFIG_SEMC_GENERIC_RMI4_SPI_ADAPTOR) ||		\
	defined(CONFIG_SEMC_GENERIC_RMI4_SPI_ADAPTOR_MODULE)
#include <linux/rmi4/rmi4_spi.h>
#endif

#ifdef CONFIG_INPUT_APDS9702
#include <linux/apds9702.h>
#endif

#ifdef CONFIG_SIMPLE_REMOTE_PLATFORM
#include <mach/simple_remote_ux500_pf.h>
#endif

#ifdef CONFIG_LM3560
#include <linux/lm3560.h>
#endif

#ifdef CONFIG_LM3561
#include <linux/lm3561.h>
#endif

#ifdef CONFIG_INPUT_NOA3402
#include <linux/input/noa3402.h>
#endif

#ifdef CONFIG_LEDS_LM3533
#include <linux/leds-lm3533_ng.h>
#endif

#ifdef CONFIG_NFC_PN544
#include <linux/pn544.h>
#endif

#ifdef CONFIG_INPUT_SO340010_TOUCH_KEY
#include <linux/so340010.h>
#endif

#ifdef CONFIG_SENSORS_TSL2772
#include "../../../drivers/staging/taos/tsl277x.h"
#endif

#define IRQ_KP 1 /*To DO*/

#ifdef CONFIG_USB_ANDROID
#define USB_SERIAL_NUMBER_LEN 10
#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define RAM_CONSOLE_SIZE    (128 * SZ_1K)
#define RAM_CONSOLE_START   (0x20000000 - RAM_CONSOLE_SIZE)
#endif

#ifdef CONFIG_AV8100
static int av8100_plat_init(void)
{
	struct ux500_pins *pins;
	int res;

	pins = ux500_pins_get("av8100-hdmi");
	if (!pins) {
		res = -EINVAL;
		goto failed;
	}

	res = ux500_pins_enable(pins);
	if (res != 0)
		goto failed;

	return res;

failed:
	pr_err("%s failed\n");
	return res;
}

static int av8100_plat_exit(void)
{
	struct ux500_pins *pins;
	int res;

	pins = ux500_pins_get("av8100-hdmi");
	if (!pins) {
		res = -EINVAL;
		goto failed;
	}
	res = ux500_pins_disable(pins);
	if (res != 0)
		goto failed;
	return res;

failed:
	pr_err("%s failed\n");
	return res;
}

static struct av8100_platform_data av8100_plat_data = {
	.init = av8100_plat_init,
	.exit = av8100_plat_exit,
	.irq = GPIO_TO_IRQ(192),
	.reset = 196,
	.inputclk_id = "sysclk2",
	.regulator_pwr_id = "hdmi_1v8",
	.alt_powerupseq = false,
	.mclk_freq = 3, /* MCLK_RNG_31_38 */
};
#endif

#ifdef CONFIG_INPUT_LSM303DLH_ACCELEROMETER
static struct lsm303dlh_acc_platform_data lsm303dlh_acc_platform_data = {
	.range = LSM303_RANGE_2G,
	.poll_interval_ms = 100,
	.irq_pad = 75,
	.power_on = NULL,
	.power_off = NULL,
};
#endif

#if defined(CONFIG_INPUT_LSM303DLHC_ACCELEROMETER) || \
	defined(CONFIG_INPUT_LSM303DLH_MAGNETOMETER) || \
	defined(CONFIG_INPUT_L3G4200D)
static int platform_power_config(struct device *dev, bool enable,
			struct regulator **regulator, char *regulator_id)
{
	int rc = 0;
	dev_dbg(dev, "%s\n", __func__);

	if (enable) {
		if (*regulator == NULL) {
			dev_dbg(dev, "%s: get regulator %s\n",
							__func__, regulator_id);
			*regulator = regulator_get(NULL, regulator_id);
			if (IS_ERR(*regulator)) {
				rc = PTR_ERR(*regulator);
				dev_err(dev, "%s: Failed to get regulator %s\n",
							__func__, regulator_id);
				return rc;
			}
		}
		rc = regulator_set_voltage(*regulator, 2800000, 2800000);
		if (rc) {
			dev_err(dev, "%s: unable to set voltage "
						"rc = %d!\n", __func__, rc);
			goto exit;
		}
	} else {
		goto exit;
	}

	return rc;

exit:
	regulator_put(*regulator);
	*regulator = NULL;
	return rc;
}

#endif

#ifdef CONFIG_INPUT_LSM303DLHC_ACCELEROMETER
static struct regulator *acc_regulator;

static int power_config_acc(struct device *dev, bool enable)
{
	int rc;

	dev_dbg(dev, "%s enable = %d\n", __func__, enable);

	rc = platform_power_config(dev, enable, &acc_regulator, "v-lsm303dlhc");

	return rc;
}

static int power(struct device *dev, enum lsm303dlhc_acc_power_sate pwr_state)
{
	int rc = -ENOSYS;

	dev_dbg(dev, "%s pwr_state = %d\n", __func__, pwr_state);

	if (pwr_state == LSM303DLHC_STANDBY) {
		goto exit;
	} else if (pwr_state == LSM303DLHC_PWR_ON) {
		rc = regulator_enable(acc_regulator);
		if (rc)
			dev_err(dev, "%s: failed to enable regulator\n",
								__func__);
	} else if (pwr_state == LSM303DLHC_PWR_OFF) {
		rc = regulator_disable(acc_regulator);
		if (rc)
			dev_err(dev, "%s: failed to disable regulator\n",
								__func__);
	}

exit:
	return rc;
}

static struct lsm303dlhc_acc_platform_data lsm303dlhc_acc_platform_data = {
	.range = 2,
	.poll_interval_ms = 100,
	.mode = MODE_INTERRUPT,
	.irq_pad = 75,
	.power = power,
	.power_config = power_config_acc,
};
#endif

static struct ab3550_platform_data ab3550_plf_data = {
	.irq = {
		.base = 0,
		.count = 0,
	},
	.dev_data = {
	},
	.dev_data_sz = {
	},
	.init_settings = NULL,
	.init_settings_sz = 0,
};

#ifdef CONFIG_SIMPLE_REMOTE_PLATFORM
static struct resource simple_remote_resources[] = {
	{
		.name = "ACC_DETECT_1DB_F",
		.start = AB8500_INT_ACC_DETECT_1DB_F,
		.end = AB8500_INT_ACC_DETECT_1DB_F,
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "ACC_DETECT_1DB_R",
		.start = AB8500_INT_ACC_DETECT_1DB_R,
		.end = AB8500_INT_ACC_DETECT_1DB_R,
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "ACC_DETECT_22DB_F",
		.start = AB8500_INT_ACC_DETECT_22DB_F,
		.end = AB8500_INT_ACC_DETECT_22DB_F,
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "ACC_DETECT_22DB_R",
		.start = AB8500_INT_ACC_DETECT_22DB_R,
		.end = AB8500_INT_ACC_DETECT_22DB_R,
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "ACC_DETECT_21DB_F",
		.start = AB8500_INT_ACC_DETECT_21DB_F,
		.end = AB8500_INT_ACC_DETECT_21DB_F,
		.flags = IORESOURCE_IRQ,
	},
	{
		.name = "ACC_DETECT_21DB_R",
		.start = AB8500_INT_ACC_DETECT_21DB_R,
		.end = AB8500_INT_ACC_DETECT_21DB_R,
		.flags = IORESOURCE_IRQ,
	}
};
#endif /* CONFIG_SIMPLE_REMOTE_PLATFORM */

#ifdef CONFIG_LM3560
#define LM3560_HW_RESET_GPIO 6
int lm3560_request_gpio_pins(void)
{
	int result;

	result = gpio_request(LM3560_HW_RESET_GPIO, "LM3560 hw reset");
	if (result)
		return result;

	gpio_set_value(LM3560_HW_RESET_GPIO, 1);

	udelay(20);
	return result;
}
int lm3560_release_gpio_pins(void)
{

	gpio_set_value(LM3560_HW_RESET_GPIO, 0);
	gpio_free(LM3560_HW_RESET_GPIO);

	return 0;
}
static struct lm3560_platform_data lm3560_platform_data = {
	.hw_enable              = lm3560_request_gpio_pins,
	.hw_disable             = lm3560_release_gpio_pins,
	.led_nums		= 2,
	.strobe_trigger		= LM3560_STROBE_TRIGGER_EDGE,
	.privacy_terminate	= LM3560_PRIVACY_MODE_TURN_BACK,
	.privacy_led_nums	= 1,
	.privacy_blink_period	= 0, /* No bliking */
	.current_limit		= 2100000, /* uA */
	.flash_sync		= LM3560_SYNC_ON,
	.strobe_polarity	= LM3560_STROBE_POLARITY_HIGH,
	.ledintc_pin_setting	= LM3560_LEDINTC_NTC_THERMISTOR_INPUT,
	.tx1_polarity		= LM3560_TX1_POLARITY_HIGH,
	.tx2_polarity		= LM3560_TX2_POLARITY_HIGH,
	.hw_torch_mode		= LM3560_HW_TORCH_MODE_DISABLE,
};
#endif /* CONFIG_LM3560 */

#ifdef CONFIG_LM3561
#define LM3561_HW_RESET_GPIO 6
int lm3561_request_gpio_pins(void)
{
	int result;

	result = gpio_request(LM3561_HW_RESET_GPIO, "LM3561 hw reset");
	if (result)
		return result;

	gpio_set_value(LM3561_HW_RESET_GPIO, 1);

	udelay(20);
	return result;
}
int lm3561_release_gpio_pins(void)
{

	gpio_set_value(LM3561_HW_RESET_GPIO, 0);
	gpio_free(LM3561_HW_RESET_GPIO);

	return 0;
}
static struct lm3561_platform_data lm3561_platform_data = {
	.hw_enable              = lm3561_request_gpio_pins,
	.hw_disable             = lm3561_release_gpio_pins,
	.led_nums		= 1,
	.strobe_trigger		= LM3561_STROBE_TRIGGER_LEVEL,
	.current_limit		= 1000000, /* uA
				   selectable value are 1500mA or 1000mA.
				   if set other value,
				   it assume current limit is 1000mA.
				*/
	.flash_sync		= LM3561_SYNC_ON,
	.strobe_polarity	= LM3561_STROBE_POLARITY_HIGH,
	.ledintc_pin_setting	= LM3561_LEDINTC_NTC_THERMISTOR_INPUT,
	.tx1_polarity		= LM3561_TX1_POLARITY_HIGH,
	.tx2_polarity		= LM3561_TX2_POLARITY_HIGH,
	.hw_torch_mode		= LM3561_HW_TORCH_MODE_DISABLE,
};
#endif /* CONFIG_LM3561 */

#ifdef CONFIG_INPUT_LSM303DLH_MAGNETOMETER
static struct regulator *mag_regulator;

static int power_config_mag(struct device *dev, bool enable)
{
	int rc;

	dev_dbg(dev, "%s enable = %d\n", __func__, enable);

	rc = platform_power_config(dev, enable, &mag_regulator, "v-lsm303dlh");

	return rc;
}

static int power_on_mag(struct device *dev)
{
	int rc;

	dev_dbg(dev, "%s\n", __func__);

	rc = regulator_enable(mag_regulator);
	if (rc)
		dev_err(dev, "%s: failed to enable regulator\n", __func__);

	return rc;
}

static int power_off_mag(struct device *dev)
{
	int rc;

	dev_dbg(dev, "%s\n", __func__);

	rc = regulator_disable(mag_regulator);

	return rc;
}

static struct lsm303dlh_mag_platform_data lsm303dlh_mag_platform_data = {
	.range = LSM303_RANGE_8200mG,
	.poll_interval_ms = 100,
	.power_on = power_on_mag,
	.power_off = power_off_mag,
	.power_config = power_config_mag,
};
#endif

#ifdef CONFIG_INPUT_LPS331AP
static struct lps331ap_prs_platform_data lps331ap_platform_data = {
	.init                 = NULL,
	.exit                 = NULL,
	.power_on             = NULL,
	.power_off            = NULL,
	.poll_interval        = 200,
	.min_interval         = 40,
};
#endif

#ifdef CONFIG_INPUT_L3G4200D
static struct regulator *gyro_regulator;

static int power_config_gyro(struct device *dev, bool enable)
{
	int rc;

	dev_dbg(dev, "%s enable = %d\n", __func__, enable);

	rc = platform_power_config(dev, enable, &gyro_regulator, "v-l3g4200d");

	return rc;
}

static int power_on_gyro(struct device *dev)
{
	int rc;

	dev_dbg(dev, "%s\n", __func__);

	rc = regulator_enable(gyro_regulator);
	if (rc)
		dev_err(dev, "%s: failed to enable regulator\n", __func__);

	return rc;
}

static int power_off_gyro(struct device *dev)
{
	int rc;

	dev_dbg(dev, "%s\n", __func__);

	rc = regulator_disable(gyro_regulator);

	return rc;
}

static struct l3g4200d_gyr_platform_data l3g4200d_gyro_platform_data = {
	.poll_interval = 100,
	.min_interval = 40,

	.fs_range = L3G4200D_FS_2000DPS,

	.axis_map_x =  0,
	.axis_map_y = 1,
	.axis_map_z = 2,

	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,

	.init = NULL,
	.exit = NULL,
	.power_on = power_on_gyro,
	.power_off = power_off_gyro,
	.power_config = power_config_gyro,
};
#endif

#ifdef CONFIG_INPUT_APDS9702
#define APDS9702_DOUT_GPIO	89
static int apds9702_gpio_setup(int request)
{
	int rc = 0;

	if (request) {
		rc = gpio_request(APDS9702_DOUT_GPIO, "apds9702_dout");
		if (rc) {
			printk(KERN_ERR "%s: gpio_request failed!", __func__);
			return rc;
		}

		rc = gpio_direction_input(APDS9702_DOUT_GPIO);
		if (rc) {
			printk(KERN_ERR "%s: gpio_direction_input failed!",
				   __func__);
			goto exit;
		}

		/*
		 * request_threaded_irq will be called in the driver,
		 * so free gpio for now...
		 */
	}

exit:
	gpio_free(APDS9702_DOUT_GPIO);
	return rc;
}

static void apds9702_hw_config(int enable)
{
}

static struct apds9702_platform_data apds9702_pdata = {
	.gpio_dout	= APDS9702_DOUT_GPIO,
	.is_irq_wakeup	= 1,
	.hw_config	= apds9702_hw_config,
	.gpio_setup	= apds9702_gpio_setup,
	.ctl_reg = {
		.trg	= 1,
		.pwr	= 1,
		.burst	= 7,
		.frq	= 3,
		.dur	= 2,
		.th		= 15,
		.rfilt	= 0,
	},
	.phys_dev_path	= "/sys/bus/i2c/devices/2-0054",
};
#endif

#ifdef CONFIG_INPUT_NOA3402
#define NOA3402_GPIO	89

static struct noa3402_platform_data noa3402_pdata;

static int noa3402_hw_setup(struct noa3402_chip *chip, bool enable)
{
	int ret = 0;

	pr_debug("%s: enable = %d\n", __func__, enable);

	if (!chip) {
		pr_err("%s: no device\n", __func__);
		return -ENODEV;
	}

	if (enable) {
		ret = gpio_request(noa3402_pdata.gpio, "noa3402");
		if (ret) {
			pr_err("%s: gpio_request failed!", __func__);
			goto exit;
		}

		ret = gpio_direction_input(noa3402_pdata.gpio);
		if (ret) {
			pr_err("%s: gpio_direction_input failed!", __func__);
			goto exit_free_gpio;
		}

		chip->noa_regulator = regulator_get(NULL,
						noa3402_pdata.regulator_id);
		if (IS_ERR(chip->noa_regulator)) {
			ret = PTR_ERR(chip->noa_regulator);
			pr_err("%s: Failed to get reg '%s'\n", __func__,
						noa3402_pdata.regulator_id);
			goto exit_free_gpio;
		}
		goto exit;
	} else {
		regulator_put(chip->noa_regulator);
		chip->noa_regulator = NULL;
	}
exit_free_gpio:
	gpio_free(noa3402_pdata.gpio);
exit:
	return ret;
}

static int noa3402_pwr_enable(struct noa3402_chip *chip, bool enable)
{
	int ret = 0;

	pr_debug("%s: enable = %d\n", __func__, enable);

	if (!chip || !chip->noa_regulator) {
		pr_err("%s: no device\n", __func__);
		return -ENODEV;
	}

	if (enable) {
		pr_debug("%s: enable '%s'\n", __func__,
						noa3402_pdata.regulator_id);
		/* We assume that the regulator has been initialized
		   with the correct voltage since it is shared */
		ret = regulator_enable(chip->noa_regulator);
		if (ret < 0)
			pr_err("%s: Failed to enable regulator\n", __func__);
	} else {
		pr_debug("%s: disable '%s'\n", __func__,
						noa3402_pdata.regulator_id);
		ret = regulator_disable(chip->noa_regulator);
		if (ret < 0)
			pr_err("%s:Failed to disable regulator\n", __func__);
	}
	return ret;
}

static struct noa3402_platform_data noa3402_pdata = {
	.gpio				= NOA3402_GPIO,
	.regulator_id			= "v-noa3402",
	.pwm_sensitivity		= PWM_SENSITIVITY_STD,
	.pwm_res			= PWM_RES_8_BIT,
	.pwm_type			= PWM_TYPE_LOG,
	.ps_filter_nbr_correct		= 1,
	.ps_filter_nbr_measurements	= 1,
	.ps_led_current			= LED_CURRENT_MA_TO_REG(160),
	.ps_integration_time		= PS_INTEGRATION_300_US,
	.ps_interval			= PS_INTERVAL_MS_TO_REG(50),
	.als_integration_time		= ALS_INTEGRATION_100_MS,
	.als_interval			= ALS_INTERVAL_MS_TO_REG(500),
	.is_irq_wakeup			= 1,
	.phys_dev_path			= "/sys/bus/i2c/devices/2-0037",
	.pwr_enable			= noa3402_pwr_enable,
	.hw_setup			= noa3402_hw_setup,
};

#endif

#ifdef CONFIG_SENSORS_TSL2772
#define TSL2772_GPIO	89
static struct regulator *tsl2772_vreg;
static int board_tsl2772_init(struct device *dev)
{
	int ret;
	dev_dbg(dev, "%s\n", __func__);
	ret = gpio_request(TSL2772_GPIO, dev_name(dev));
	if (ret) {
		dev_err(dev, "%s: gpio_request failed!", __func__);
		goto exit;
	}
	ret = gpio_direction_input(TSL2772_GPIO);
	if (ret) {
		dev_err(dev, "%s: gpio_direction_input failed!", __func__);
		goto exit_free_gpio;
	}
	tsl2772_vreg = regulator_get(NULL, "v-tsl2772");
	if (IS_ERR(tsl2772_vreg)) {
		ret = PTR_ERR(tsl2772_vreg);
		tsl2772_vreg = NULL;
		dev_err(dev, "%s: Failed to get reg '%s'\n", __func__,
				"v-tsl2772");
		goto exit_free_gpio;
	}
	return 0;
exit_free_gpio:
	gpio_free(TSL2772_GPIO);
exit:
	return ret;
}

static void board_tsl2772_teardown(struct device *dev)
{
	dev_dbg(dev, "%s\n", __func__);
	gpio_free(TSL2772_GPIO);
	if (tsl2772_vreg) {
		regulator_put(tsl2772_vreg);
		tsl2772_vreg = NULL;
	}
}

static int board_tsl2772_power(struct device *dev, enum tsl2772_pwr_state state)
{
	int ret = 0;

	dev_dbg(dev, "%s: state %d\n", __func__, state);
	if (state == POWER_ON && tsl2772_vreg) {
		ret = regulator_enable(tsl2772_vreg);
		if (ret < 0)
			dev_err(dev, "%s: Failed to enable vreg\n", __func__);
	} else if (state == POWER_OFF && tsl2772_vreg) {
		ret = regulator_disable(tsl2772_vreg);
		if (ret < 0)
			dev_err(dev, "%s: Failed to disable vreg\n", __func__);
	} else {
		dev_info(dev, "%s: nothing to do\n", __func__);
	}
	return ret;
}


struct tsl2772_platform_data tsl2772_data = {
	.platform_power = board_tsl2772_power,
	.platform_init = board_tsl2772_init,
	.platform_teardown = board_tsl2772_teardown,
	.prox_name = "tsl2772_proximity",
	.als_name = "tsl2772_als",
	.raw_settings = NULL,
	.parameters = {
		.prox_th_min = 500,
		.prox_th_max = 520,
		.als_gate = 10,
	},
	.als_can_wake = false,
	.proximity_can_wake = true,
};
#endif

#ifdef CONFIG_LEDS_LM3533
#define LM3533_HWEN_GPIO 93
static int lm3533_setup(struct device *dev)
{
	int rc = gpio_request(LM3533_HWEN_GPIO, "lm3533_hwen");
	if (rc)
		dev_err(dev, "failed to request gpio %d\n", LM3533_HWEN_GPIO);
	return rc;
}
static void lm3533_teardown(struct device *dev)
{
	gpio_free(LM3533_HWEN_GPIO);
	return;
}
static int lm3533_power_on(struct device *dev)
{
	gpio_set_value(LM3533_HWEN_GPIO, 1);
	return 0;
}
static int lm3533_power_off(struct device *dev)
{
	gpio_set_value(LM3533_HWEN_GPIO, 0);
	return 0;
}

static struct lm3533_platform_data lm3533_pdata = {
	.b_cnf = {
		[LM3533_CBNKA] = {
			.pwm = 0x3f,
			.ctl = LM3533_HVA_MAP_LIN | LM3533_HVA_BR_CTL,
			.fsc =  I_UA_TO_FSC(20200),
			.iname = "lcd-backlight",
			.startup_brightness = 255,
		},
		[LM3533_CBNKB] = {
			.pwm = 0,
			.ctl = LM3533_HVA_MAP_LIN | LM3533_HVB_BR_CTL,
			.fsc =  I_UA_TO_FSC(0),
			.iname = "not-connected",
		},
		[LM3533_CBNKC] = {
			.pwm = 0,
			.ctl = LM3533_LV_MAP_LIN | LM3533_LV_BR_CTL,
			.fsc =  I_UA_TO_FSC(17000),
			.iname = "red",
		},
		[LM3533_CBNKD] = {
			.pwm = 0,
			.ctl = LM3533_LV_MAP_LIN | LM3533_LV_BR_CTL,
			.fsc =  I_UA_TO_FSC(19400),
			.iname = "green",
		},
		[LM3533_CBNKE] = {
			.pwm = 0,
			.ctl = LM3533_LV_MAP_LIN | LM3533_LV_BR_CTL,
			.fsc =  I_UA_TO_FSC(9800),
			.iname = "blue",
		},
		[LM3533_CBNKF] = {
			.pwm = 0,
			.ctl = LM3533_LV_MAP_LIN | LM3533_LV_BR_CTL,
			.fsc =  I_UA_TO_FSC(0),
			.iname = "not-connected",
		},
	},
	.l_cnf = {
		[LM3533_HVLED1] = {
			.connected = true, .cpout = true, .bank =  LM3533_CBNKA,
		},
		[LM3533_HVLED2] = {
			.connected = true, .cpout = true, .bank =  LM3533_CBNKB,
		},
		[LM3533_LVLED1] = {
			.connected = true, .cpout = true, .bank =  LM3533_CBNKC,
		},
		[LM3533_LVLED2] = {
			.connected = true, .cpout = true, .bank =  LM3533_CBNKD,
		},
		[LM3533_LVLED3] = {
			.connected = true, .cpout = true, .bank =  LM3533_CBNKE,
		},
		[LM3533_LVLED4] = {
			.connected = true, .cpout = true, .bank =  LM3533_CBNKF,
		},
		[LM3533_LVLED5] = {
			.connected = true, .cpout = true, .bank =  LM3533_CBNKF,
		},
	},
	.ovp_boost_pwm = LM3533_BOOST_500KHZ | LM3533_OVP_32V | LM3533_PWM_HIGH,
	.setup = lm3533_setup,
	.teardown = lm3533_teardown,
	.power_on = lm3533_power_on,
	.power_off = lm3533_power_off,
};
#endif

static struct i2c_board_info __initdata pdp_i2c0_devices[] = {
#ifdef CONFIG_AV8100
	{
		/* Config-spec is 8-bit = 0xE0, src-code need 7-bit => 0x70 */
		I2C_BOARD_INFO("av8100", 0xE0 >> 1),
		.platform_data = &av8100_plat_data,
	},
#endif
};

static struct i2c_board_info __initdata pdp_i2c1_devices[] = {
	{
		/* AB3550 */
		/* Config-spec is 8-bit = 0x94, src-code need 7-bit => 0x4A */
		I2C_BOARD_INFO("ab3550", 0x94 >> 1),
		.irq = -1,
		.platform_data = &ab3550_plf_data,
	},
#ifdef CONFIG_INPUT_LPS331AP
	{
		/* Config-spec is 8-bit = 0xB8, src-code need 7-bit => 0x5C */
		I2C_BOARD_INFO(LPS331AP_PRS_DEV_NAME, 0xB8 >> 1),
		.platform_data = &lps331ap_platform_data,
	},
#endif
#ifdef CONFIG_INPUT_L3G4200D
	{
		/* Config-spec is 8-bit = 0xD0, src-code need 7-bit => 0x68 */
		I2C_BOARD_INFO(L3G4200D_DEV_NAME, 0xD0 >> 1),
		.platform_data = &l3g4200d_gyro_platform_data,
	},
#endif
#ifdef CONFIG_INPUT_BMP180
	{
		/* Config-spec is 8-bit = 0xEE, src-code need 7-bit => 0x77 */
		I2C_BOARD_INFO("bmp180", 0xEE >> 1)
	},
#endif
};

#ifdef CONFIG_INPUT_SO340010_TOUCH_KEY

#define SO340010_TOUCH_KEY_IRQ_GPIO 97
#define SO340010_TOUCH_KEY_ENA_GPIO 209

#ifdef SO340010_TOUCH_KEY_IRQ_GPIO
#define SO340010_TOUCH_KEY_IRQ GPIO_TO_IRQ(SO340010_TOUCH_KEY_IRQ_GPIO)
#endif

static int so340010_setup(struct device *dev)
{
	int rc;

	dev_dbg(dev, "%s\n", __func__);
#ifdef SO340010_TOUCH_KEY_IRQ_GPIO
	rc = gpio_request(SO340010_TOUCH_KEY_IRQ_GPIO, "so340010_irq");
	if (rc)
		dev_err(dev, "Failed requesting GPIO-%d\n",
				SO340010_TOUCH_KEY_IRQ_GPIO);
#endif
	rc = gpio_request(SO340010_TOUCH_KEY_ENA_GPIO, "so340010_ena");
	if (rc)
		dev_err(dev, "Failed requesting GPIO-%d\n",
				SO340010_TOUCH_KEY_ENA_GPIO);
	return rc;
}

static void so340010_teardown(struct device *dev)
{
#ifdef SO340010_TOUCH_KEY_IRQ_GPIO
	gpio_free(SO340010_TOUCH_KEY_IRQ_GPIO);
#endif
	gpio_free(SO340010_TOUCH_KEY_ENA_GPIO);
	dev_dbg(dev, "%s\n", __func__);
}

static int so340010_power(struct device *dev,
		enum so34_platform_power request,
		enum so34_platform_power *result)
{
	dev_dbg(dev, "power state 0x%02x requested\n", request);
	/*
	 * power is always enabled, so set SO34_PWR_ENABLED
	 */
	*result = SO34_PWR_ENABLED;
	return 0;
}

static struct so340010_config so340010_config = {
	.pin = {
		[0] = {
			.mode = SO340010_LED,
			.u.led.max_level = 31, /* full brightness */
			.u.led.effect = SO340010_RAMP_UP_DN,
			.u.led.name = "so34-led0",
		},
		[1] = {
			.mode = SO340010_LED,
			.u.led.max_level = 31,
			.u.led.effect = SO340010_RAMP_UP_DN,
			.u.led.name = "so34-led1",
		},
		[2] = {
			.mode = SO340010_LED,
			.u.led.max_level = 31,
			.u.led.effect = SO340010_RAMP_UP_DN,
			.u.led.name = "so34-led2",
		},
	},
	.btn = {
		[0] = {
			.enable = 1,
			.sensitivity = 0xd8,
			.code = KEY_MENU,
			.map2gpio = false,
			.suspend = SO340010_SUSPEND_EARLY,
		},
		[1] = {
			.enable = 1,
			.sensitivity = 0xd8,
			.code = KEY_HOME,
			.map2gpio = false,
			.suspend = SO340010_SUSPEND_EARLY,
		},
		[2] = {
			.enable = 1,
			.sensitivity = 0xd8,
			.code = KEY_BACK,
			.map2gpio = false,
			.suspend = SO340010_SUSPEND_EARLY,
		},
	},
	.period_a_x12_5_ms = 5,
	.period_b_x12_5_ms = 5,
	.btn_out_mode = SO340010_ACTIVE_HI,
	.btn_mode = SO340010_INRESTRICTED,
	.btn_map2dir = SO340010_MAP2DATA,
	.gpi_suspend = SO340010_SUSPEND_EARLY,
	.gpi_enable = false,
#ifdef SO340010_TOUCH_KEY_IRQ
	.polling_ms = SO340010_NO_POLLING,
#else
	.polling_ms = 20,
#endif
	.input_name = "so34-buttons",
	.enable_wake = true,
	.platform_setup = so340010_setup,
	.platform_teardown = so340010_teardown,
	.platform_pwr = so340010_power,
};
#endif /*CONFIG_INPUT_SO340010_TOUCH_KEY*/

static struct i2c_board_info __initdata pdp_i2c2_devices[] = {
#ifdef CONFIG_INPUT_SO340010_TOUCH_KEY
	{
		/* Config-spec is 8-bit = 0x58, src-code need 7-bit => 0x2C */
		I2C_BOARD_INFO(SO340010_DEV_NAME, 0x58 >> 1),
#ifdef SO340010_TOUCH_KEY_IRQ
		.irq		= SO340010_TOUCH_KEY_IRQ,
#else
		.irq		= -1,
#endif
		.platform_data	= &so340010_config,
	},
#endif
#ifdef CONFIG_INPUT_LSM303DLH_ACCELEROMETER
	{
		/* Config-spec is 8-bit = 0x30, src-code need 7-bit => 0x18 */
		I2C_BOARD_INFO(LSM303DLH_ACC_DEV_NAME, 0x30 >> 1),
		.platform_data = &lsm303dlh_acc_platform_data,
	},
#endif
#ifdef CONFIG_INPUT_LSM303DLHC_ACCELEROMETER
	{
		/* Config-spec is 8-bit = 0x32, src-code need 7-bit => 0x19 */
		I2C_BOARD_INFO(LSM303DLHC_ACC_DEV_NAME, 0x32 >> 1),
		.platform_data = &lsm303dlhc_acc_platform_data,
	},
#endif
#ifdef CONFIG_INPUT_LSM303DLH_MAGNETOMETER
	{
		/* Config-spec is 8-bit = 0x3C, src-code need 7-bit => 0x1E */
		I2C_BOARD_INFO(LSM303DLH_MAG_DEV_NAME, 0x3c >> 1),
		.platform_data = &lsm303dlh_mag_platform_data,
	},
#endif
#ifdef CONFIG_LEDS_AS3677
	{
		/* Config-spec is 8-bit = 0x80, src-code need 7-bit => 0x40 */
		I2C_BOARD_INFO("as3677", 0x80 >> 1),
		.platform_data = &as3677_pdata,
		.irq = 92,
	},
#endif
#ifdef CONFIG_LEDS_AS3676
	{
		/* Config-spec is 8-bit = 0x80, src-code need 7-bit => 0x40 */
		I2C_BOARD_INFO("as3676", 0x80 >> 1),
		.platform_data = &as3676_platform_data,
	},
#endif
#ifdef CONFIG_LEDS_AS3676_VENDOR
	{
		/* Config-spec is 8-bit = 0x80, src-code need 7-bit => 0x40 */
		I2C_BOARD_INFO("as3676", 0x80 >> 1),
		.platform_data = &as3676_platform_data,
	},
#endif
#ifdef CONFIG_INPUT_APDS9702
	{
		/* Config-spec is 8-bit = 0xA8, src-code need 7-bit => 0x54 */
		I2C_BOARD_INFO(APDS9702_NAME, 0xA8 >> 1),
		.platform_data = &apds9702_pdata,
		.type = APDS9702_NAME,
	},
#endif
#ifdef CONFIG_INPUT_NOA3402
	{
		/* Config-spec is 8-bit = 0x6E, src-code need 7-bit => 0x37 */
		I2C_BOARD_INFO(NOA3402_NAME, 0x6E >> 1),
		.platform_data = &noa3402_pdata,
		.type = NOA3402_NAME,
	},
#endif
#ifdef CONFIG_LM3560
	{
		/* Config-spec is 8-bit = 0xA6, src-code need 7-bit => 0x53 */
		I2C_BOARD_INFO(LM3560_DRV_NAME, 0xA6 >> 1),
		.platform_data = &lm3560_platform_data,
	},
#endif
#ifdef CONFIG_LM3561
	{
		/* Config-spec is 8-bit = 0xA6, src-code need 7-bit => 0x53 */
		I2C_BOARD_INFO(LM3561_DRV_NAME, 0xA6 >> 1),
		.platform_data = &lm3561_platform_data,
	},
#endif
#ifdef CONFIG_LEDS_LM3533
	{
		/* Config-spec is 8-bit = 0x6C, src-code need 7-bit => 0x36 */
		I2C_BOARD_INFO(LM3533_DEV_NAME, 0x36),
		.platform_data = &lm3533_pdata,
		.irq = -1,
	},
#endif
#ifdef CONFIG_SENSORS_TSL2772
	{
		/* Config-spec is 8-bit = 0x72, src-code need 7-bit => 0x39 */
		I2C_BOARD_INFO("tsl2772", 0x39),
		.platform_data = &tsl2772_data,
		.irq = GPIO_TO_IRQ(TSL2772_GPIO),
	},
#endif
};

#ifdef CONFIG_NFC_PN544
#define PN544_VREG "nfc_1v8"

#ifdef CONFIG_REGULATOR
struct regulator *r;
#endif

static int pn544_driver_opened(void)
{
#ifdef CONFIG_REGULATOR
	int ret;

	if (r == NULL) {
		r = regulator_get(NULL, PN544_VREG);
		if (IS_ERR(r)) {
			pr_err("%s: Not able to find regulator.\n", __func__);
			r = NULL;
			return -ENODEV;
		}
	}
	if (regulator_is_enabled(r)) {
		pr_err("%s: Regulator already enabled.\n", __func__);
		return -ENODEV;
	}

	ret = regulator_enable(r);
	if (ret < 0) {
		pr_err("%s: Not able to enable regulator. Error : %d\n",
		__func__, ret);
		regulator_disable(r);
		return -ENODEV;
	}
#endif
	return 0;
}

static void pn544_driver_closed(void)
{
#ifdef CONFIG_REGULATOR
	if (r != NULL) {
		if (regulator_is_enabled(r))
			regulator_disable(r);
		regulator_put(r);
		r = NULL;
	}
#endif
}

static int pn544_chip_config(enum pn544_state state, void *dev)
{
	switch (state) {
	case PN544_STATE_OFF:
		gpio_set_value(GPIO91_GPIO, 0);
		gpio_set_value(GPIO4_GPIO, 0);
		msleep(50);
		break;
	case PN544_STATE_ON:
		gpio_set_value(GPIO91_GPIO, 0);
		gpio_set_value(GPIO4_GPIO, 1);
		msleep(10);
		break;
	case PN544_STATE_FWDL:
		gpio_set_value(GPIO91_GPIO, 1);
		gpio_set_value(GPIO4_GPIO, 0);
		msleep(10);
		gpio_set_value(GPIO4_GPIO, 1);
		break;
	default:
		pr_err("%s: undefined state %d\n", __func__, state);
		return -EINVAL;
	}
	return 0;
}

static int pn544_gpio_request(void)
{
	int ret;

	ret = gpio_request(GPIO66_GPIO, "pn544_irq");
	if (ret)
		goto err_irq;
	ret = gpio_request(GPIO4_GPIO, "pn544_ven");
	if (ret)
		goto err_ven;
	ret = gpio_request(GPIO91_GPIO, "pn544_fw");
	if (ret)
		goto err_fw;
	return 0;
err_fw:
	gpio_free(GPIO4_GPIO);
err_ven:
	gpio_free(GPIO66_GPIO);
err_irq:
	pr_err("%s: gpio request err %d\n", __func__, ret);
	return ret;
}

static void pn544_gpio_release(void)
{
	gpio_free(GPIO4_GPIO);
	gpio_free(GPIO66_GPIO);
	gpio_free(GPIO91_GPIO);
}

static struct pn544_i2c_platform_data pn544_pdata = {
	.irq_type = IRQF_TRIGGER_RISING,
	.chip_config = pn544_chip_config,
	.driver_loaded = pn544_gpio_request,
	.driver_unloaded = pn544_gpio_release,
	.driver_opened = pn544_driver_opened,
	.driver_closed = pn544_driver_closed,
};
#endif

static struct i2c_board_info __initdata pdp_i2c3_devices[] = {
#ifdef CONFIG_NFC_PN544
	{
		/* Config-spec is 8-bit = 0x50, src-code need 7-bit => 0x28 */
		I2C_BOARD_INFO(PN544_DEVICE_NAME, 0x50 >> 1),
		.platform_data = &pn544_pdata,
		.irq = GPIO_TO_IRQ(GPIO66_GPIO)
	},
#endif
};

#ifdef CONFIG_TOUCHSCREEN_CYTTSP_SPI

#define CYTTSP_XRES_GPIO 94
#define CYTTSP_VREG "v-touch1"
#define CYTTSP_SPI_CS_GPIO 31
#define CYTTSP_VOLTAGE 2800000

static int cyttsp_wakeup(struct device *dev)
{
	int ret;
	struct cyttsp_platform_data *data = dev->platform_data;

	ret = gpio_direction_output(data->irq_gpio, 0);
	if (ret) {
		dev_err(dev, "%s: Failed to request gpio_direction_output\n",
			__func__);
		return ret;
	}
	msleep(50);
	gpio_set_value(data->irq_gpio, 0);
	msleep(1);
	gpio_set_value(data->irq_gpio, 1);
	msleep(1);
	gpio_set_value(data->irq_gpio, 0);
	msleep(1);
	gpio_set_value(data->irq_gpio, 1);
	printk(KERN_INFO "%s: wakeup\n", __func__);
	ret = gpio_direction_input(data->irq_gpio);
	if (ret) {
		dev_err(dev, "%s: Failed to request gpio_direction_input\n",
			__func__);
		return ret;
	}
	msleep(50);
	return 0;
}

static struct regulator *cyttsp_reg;
static int cyttsp_init(int on, struct device *dev)
{
	int ret = 0;
	struct cyttsp_platform_data *data = dev->platform_data;

	if (on && !cyttsp_reg) {
		cyttsp_reg = regulator_get(NULL, CYTTSP_VREG);
		if (IS_ERR(cyttsp_reg)) {
			dev_err(dev, "Failed to get regulator '%s'\n",
					CYTTSP_VREG);
			ret = -ENODEV;
			goto regulator_get_failed;
		}
		ret = regulator_set_voltage(cyttsp_reg, CYTTSP_VOLTAGE,
				CYTTSP_VOLTAGE);
		if (ret < 0) {
			dev_err(dev, "Failed to set voltage on '%s'\n",
			       CYTTSP_VREG);
			ret = -ENODEV;
			goto regulator_set_failed;
		}
		ret = regulator_enable(cyttsp_reg);
		if (ret < 0) {
			dev_err(dev, "Failed to enable regulator '%s'\n",
			       CYTTSP_VREG);
			ret = -ENODEV;
			goto regulator_set_failed;
		}
		ret = gpio_request(data->irq_gpio, "CYTTSP IRQ GPIO");
		if (ret) {
			dev_err(dev, "%s: Failed to request GPIO %d\n",
				__func__, data->irq_gpio);
			ret = -ENODEV;
			goto irq_gpio_req_failed;
		}
		ret = gpio_request(CYTTSP_XRES_GPIO, "CYTTSP XRES GPIO");
		if (ret) {
			dev_err(dev, "%s: Failed to request GPIO %d\n",
				__func__, CYTTSP_XRES_GPIO);
			ret = -ENODEV;
			goto xres_gpio_req_failed;
		}
		ret = gpio_request(CYTTSP_SPI_CS_GPIO, "CYTTSP_SPI_CS_GPIO");
		if (ret) {
			dev_err(dev, "%s: Failed to request GPIO %d\n",
				__func__, CYTTSP_SPI_CS_GPIO);
			ret = -ENODEV;
			goto cs_gpio_req_failed;
		}
		gpio_direction_input(data->irq_gpio);
		gpio_direction_output(CYTTSP_XRES_GPIO, 1);
		gpio_direction_output(CYTTSP_SPI_CS_GPIO, 1);
	} else if (!on && cyttsp_reg) {
		gpio_free(CYTTSP_SPI_CS_GPIO);
cs_gpio_req_failed:
		gpio_free(CYTTSP_XRES_GPIO);
xres_gpio_req_failed:
		gpio_free(data->irq_gpio);
irq_gpio_req_failed:
		regulator_disable(cyttsp_reg);
regulator_set_failed:
		regulator_put(cyttsp_reg);
regulator_get_failed:
		cyttsp_reg = NULL;
	} else {
		dev_err(dev, "%s: on %d but cyttsp_reg %p\n", __func__,
				on, cyttsp_reg);
		ret = -EINVAL;
	}
	return ret;
}

static int cyttsp_xres(void)
{
	int polarity;
	int rc;

	rc = gpio_direction_input(CYTTSP_XRES_GPIO);
	if (rc) {
		pr_err("%s: failed to set direction input, %d\n",
		       __func__, rc);
		return -EIO;
	}
	polarity = gpio_get_value(CYTTSP_XRES_GPIO);
	pr_debug("%s: %d\n", __func__, polarity);
	rc = gpio_direction_output(CYTTSP_XRES_GPIO, polarity ^ 1);
	if (rc) {
		pr_err("%s: failed to set direction output, %d\n",
		       __func__, rc);
		return -EIO;
	}
	msleep(1);
	gpio_set_value(CYTTSP_XRES_GPIO, polarity);
	return 0;
}

#ifdef CONFIG_TOUCHSCREEN_CYTTSP_KEY
#define TT_KEY_BACK_FLAG	0x01
#define TT_KEY_MENU_FLAG	0x02
#define TT_KEY_HOME_FLAG	0x04

static struct input_dev *input_dev_cyttsp_key;

static int __init cyttsp_key_init(void)
{
	input_dev_cyttsp_key = input_allocate_device();
	if (!input_dev_cyttsp_key) {
		pr_err("Error, unable to alloc cyttsp key device\n");
		return -ENOMEM;
	}
	input_dev_cyttsp_key->name = "cyttsp_key";
	input_set_capability(input_dev_cyttsp_key, EV_KEY, KEY_MENU);
	input_set_capability(input_dev_cyttsp_key, EV_KEY, KEY_BACK);
	input_set_capability(input_dev_cyttsp_key, EV_KEY, KEY_HOME);
	if (input_register_device(input_dev_cyttsp_key)) {
		pr_err("Unable to register cyttsp key device\n");
		input_free_device(input_dev_cyttsp_key);
		return -ENODEV;
	}
	return 0;
}
module_init(cyttsp_key_init);

static int cyttsp_key_rpc_callback(u8 data[], int size)
{
	static u8 last;
	u8 toggled = last ^ data[0];

	if (toggled & TT_KEY_MENU_FLAG)
		input_report_key(input_dev_cyttsp_key, KEY_MENU,
			!!(*data & TT_KEY_MENU_FLAG));

	if (toggled & TT_KEY_BACK_FLAG)
		input_report_key(input_dev_cyttsp_key, KEY_BACK,
			!!(*data & TT_KEY_BACK_FLAG));

	if (toggled & TT_KEY_HOME_FLAG)
		input_report_key(input_dev_cyttsp_key, KEY_HOME,
			!!(*data & TT_KEY_HOME_FLAG));

	last = data[0];
	return 0;
}
#endif /* CONFIG_TOUCHSCREEN_CYTTSP_KEY */

static void cyttsp_spi_cs_control(u32 command)
{
	/* set the FRM signal, which is CS  - TODO */
	if (command == SSP_CHIP_SELECT)
		gpio_set_value(CYTTSP_SPI_CS_GPIO, 0);
	else if (command == SSP_CHIP_DESELECT)
		gpio_set_value(CYTTSP_SPI_CS_GPIO, 1);
}

struct pl022_config_chip cyttsp_chip_info = {
	.iface = SSP_INTERFACE_MOTOROLA_SPI,

	/* we can act as master only */
	.hierarchy = SSP_MASTER,

	/* Only valid in slave mode
	.slave_tx_disable = 1,
	*/

	/* clock freq. 133330000 / (cpsdvsr * (scr +1))
	*  cpsdvsr = 165, src=2,  freq=401596
	*  cpsdvsr = 128, src=0,  freq=1041640
	*  If 0 the spi master will calculate a value
	*  based on the slave speed
	*/
	.clk_freq = {
		.cpsdvsr = 0,
		.scr = 0,
	},
	.com_mode = INTERRUPT_TRANSFER,
	.rx_lev_trig = SSP_RX_1_OR_MORE_ELEM,
	.tx_lev_trig = SSP_TX_1_OR_MORE_EMPTY_LOC,
	.ctrl_len = SSP_BITS_8,
	.wait_state = SSP_MWIRE_WAIT_ZERO,
	.duplex = SSP_MICROWIRE_CHANNEL_FULL_DUPLEX,
	/*
	.clkdelay =
	*/
	.cs_control = cyttsp_spi_cs_control,
};
#endif

#ifdef CONFIG_RMI4_SPI
#define RMI4_SPI_CS_GPIO	31
#define RMI4_SPI_IRQ_GPIO	88
#define RMI4_SPI_XRES_GPIO	94

#define RMI4_SPI_VREG_VDD "vio-display" /* Shared with display */

#define RMI4_SPI_VDD_VOLTAGE 2800000

static struct regulator *rmi4_vdd;

static int rmi4_gpio_init(struct device *dev, int on)
{
	int ret = 0;

	if (on) {
		ret = gpio_request(RMI4_SPI_IRQ_GPIO, "RMI_SPI IRQ GPIO");
		if (ret) {
			dev_err(dev, "%s: Failed to request IRQ GPIO %d\n",
				__func__, RMI4_SPI_IRQ_GPIO);
			return ret;
		}

		ret = gpio_request(RMI4_SPI_CS_GPIO, "RMI_SPI CHIP SEL");
		if (ret) {
			dev_err(dev,
				"%s: Failed to request CS gpio %d\n", __func__,
				RMI4_SPI_CS_GPIO);
			goto err_cs_request;
		}

		ret = gpio_request(RMI4_SPI_XRES_GPIO, "RMI_SPI XRES");
		if (ret) {
			dev_err(dev, "%s: Failed to request reset gpio %d\n",
				__func__, RMI4_SPI_XRES_GPIO);
			goto err_xres_request;
		}

		/* Pullup is configured per product in *-pins.c */
		gpio_direction_input(RMI4_SPI_IRQ_GPIO);
		gpio_direction_output(RMI4_SPI_CS_GPIO, 1);
		gpio_direction_output(RMI4_SPI_XRES_GPIO, 1);
	} else {
		gpio_free(RMI4_SPI_XRES_GPIO);
err_xres_request:
		gpio_free(RMI4_SPI_CS_GPIO);
err_cs_request:
		gpio_free(RMI4_SPI_IRQ_GPIO);
	}

	return ret;
}

static int rmi4_reg_init(struct device *dev, int on)
{
	int ret = 0;

	if (!rmi4_vdd)
		rmi4_vdd = regulator_get(NULL, RMI4_SPI_VREG_VDD);

	if (IS_ERR(rmi4_vdd)) {
		dev_err(dev,
			"%s - Failed to get VDD regulator '%s. Error : %ld'\n",
			__func__, RMI4_SPI_VREG_VDD, IS_ERR(rmi4_vdd) * -1);
		return -ENODEV;
	}

	if (on) {
		regulator_set_voltage(rmi4_vdd, RMI4_SPI_VDD_VOLTAGE,
				      RMI4_SPI_VDD_VOLTAGE);
		ret = regulator_enable(rmi4_vdd);
		if (ret < 0) {
			dev_err(dev,
				"%s - Failed to enable VDD regulator '%s'. "
				"Error : %d\n",
				__func__, RMI4_SPI_VREG_VDD, ret);
		}

	} else {
		regulator_disable(rmi4_vdd);
		regulator_put(rmi4_vdd);
	}

	return ret;
}

static int rmi4_gpio_config(struct device *dev, bool configure)
{
	int ret = rmi4_reg_init(dev, configure);
	if (ret)
		return ret;

	ret = rmi4_gpio_init(dev, configure);
	if (ret)
		rmi4_reg_init(dev, 0);

	return ret;
}

static int rmi4_force_chip_sel(const void *cs_assert_data, const bool assert)
{
	/* RMI4 driver expects 1 on success! */
	return !gpio_direction_output(RMI4_SPI_CS_GPIO, !assert);
}

static struct rmi_device_platform_data synaptics_platform_data = {
	.driver_name = "rmi-generic",
	.irq = RMI4_SPI_IRQ_GPIO,
	.irq_polarity = RMI_IRQ_ACTIVE_LOW,
	.gpio_config = rmi4_gpio_config,
	.spi_data =  {
		.cs_assert = rmi4_force_chip_sel,
		.block_delay_us = 30,
		.split_read_block_delay_us = 30,
		.read_delay_us = 18,
		.write_delay_us = 18,
		.split_read_byte_delay_us = 10,
		.pre_delay_us = 0,
		.post_delay_us = 0,
	}
};

static void rmi4_spi_cs_control(u32 command)
{
	/* Handled elsewhere to get the required behavior */
	/* This function is here to avoid warning log on SPI *
	 * transactions */
}

static struct pl022_config_chip rmi4_chip_info = {
	.iface = SSP_INTERFACE_MOTOROLA_SPI,
	.hierarchy = SSP_MASTER,

	/* clock freq. 133330000 / (cpsdvsr * (scr +1))
	*  cpsdvsr = 165, src=2,  freq=401596
	*  cpsdvsr = 128, src=0,  freq=1041640
	*  If 0 the spi master will calculate a value
	*  based on the slave speed
	*/
	.clk_freq = {
		.cpsdvsr = 0,
		.scr = 0,
	},
	.com_mode = POLLING_TRANSFER,
	.rx_lev_trig = SSP_RX_1_OR_MORE_ELEM,
	.tx_lev_trig = SSP_TX_1_OR_MORE_EMPTY_LOC,
	.ctrl_len = SSP_BITS_8,
	.wait_state = SSP_MWIRE_WAIT_ZERO,
	.duplex = SSP_MICROWIRE_CHANNEL_FULL_DUPLEX,
	.cs_control = rmi4_spi_cs_control,
};
#endif /* CONFIG_RMI4_SPI */

#if defined(CONFIG_SEMC_GENERIC_RMI4_SPI_ADAPTOR) ||		\
	defined(CONFIG_SEMC_GENERIC_RMI4_SPI_ADAPTOR_MODULE)

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/rmi4/rmi4_early_suspend.h>
#endif

#define RMI4_SPI_CS_GPIO	31
#define RMI4_SPI_IRQ_GPIO	88
#define RMI4_SPI_XRES_GPIO	94

#define RMI4_SPI_VREG_VDD "vio-display" /* Shared with display */

#define RMI4_SPI_VDD_VOLTAGE 2800000

static struct regulator *rmi4_vdd;

static int rmi4_gpio_init(struct device *dev, int on)
{
	int ret = 0;
	int reset;

	if (on) {
		ret = gpio_request(RMI4_SPI_CS_GPIO, "RMI_SPI CHIP SEL");
		if (ret) {
			dev_err(dev,
				"%s: Failed to request CS gpio %d\n", __func__,
				RMI4_SPI_CS_GPIO);
			goto err_cs_request;
		}

		ret = gpio_request(RMI4_SPI_XRES_GPIO, "RMI_SPI XRES");
		if (ret) {
			dev_err(dev, "%s: Failed to request reset gpio %d\n",
				__func__, RMI4_SPI_XRES_GPIO);
			goto err_xres_request;
		}

		/* Pullup is configured per product in *-pins.c */
		gpio_direction_output(RMI4_SPI_CS_GPIO, 1);

		/* Performing chip reset */
		/* This is temporary until we get response from Synaptics on
		 * how this should be done. Currently, pulling the line low once
		 * for 350ms doesn't work, so we kick the reset line a couple of
		 * times. This seems to work */
		for (reset = 0; reset < 10; reset++) {
			dev_dbg(dev, "%s - Resetting chip\n", __func__);
			gpio_direction_output(RMI4_SPI_XRES_GPIO, 0);
			msleep(10);
			gpio_direction_output(RMI4_SPI_XRES_GPIO, 1);
			msleep(10);
			dev_dbg(dev, "%s - Reset done\n", __func__);
		}

	} else {
		gpio_free(RMI4_SPI_XRES_GPIO);
err_xres_request:
		gpio_free(RMI4_SPI_CS_GPIO);
	}

err_cs_request:
	return ret;
}

static int rmi4_reg_init(struct device *dev, int on)
{
	int ret = 0;

	if (!rmi4_vdd)
		rmi4_vdd = regulator_get(NULL, RMI4_SPI_VREG_VDD);

	if (IS_ERR(rmi4_vdd)) {
		dev_err(dev,
			"%s - Failed to get VDD regulator '%s. Error : %ld'\n",
			__func__, RMI4_SPI_VREG_VDD, IS_ERR(rmi4_vdd) * -1);
		return -ENODEV;
	}

	if (on) {
		regulator_set_voltage(rmi4_vdd, RMI4_SPI_VDD_VOLTAGE,
				      RMI4_SPI_VDD_VOLTAGE);
		ret = regulator_enable(rmi4_vdd);
		if (ret < 0) {
			dev_err(dev,
				"%s - Failed to enable VDD regulator '%s'. "
				"Error : %d\n",
				__func__, RMI4_SPI_VREG_VDD, ret);
		} else {
			msleep(10);
		}
	} else {
		regulator_disable(rmi4_vdd);
		regulator_put(rmi4_vdd);
		rmi4_vdd = NULL;
	}

	return ret;
}

static int rmi4_gpio_config(struct device *dev, bool configure)
{
	int ret = rmi4_reg_init(dev, configure);
	if (ret)
		return ret;

	ret = rmi4_gpio_init(dev, configure);
	if (ret)
		rmi4_reg_init(dev, 0);

	return ret;
}

static int rmi4_force_chip_sel(struct device *dev, bool assert)
{
	struct rmi4_spi_adapter_platform_data *pdata =
		dev_get_platdata(dev);

	if (true == pdata->assert_level_low)
		assert = !assert;

	return gpio_direction_output(RMI4_SPI_CS_GPIO, assert);
}

static struct rmi4_function_data rmi4_extra_functions[] = {
#ifdef CONFIG_HAS_EARLYSUSPEND
	{
		.func_name = RMI4_3250_E_SUSP_NAME,
		.func_id = 0x01,
		.func_data = NULL,
	},
#endif
};

static struct rmi4_core_device_data rmi4_core_data = {
	.core_name = RMI4_CORE_DRIVER_NAME,
	.attn_gpio = RMI4_SPI_IRQ_GPIO,
	.irq_polarity = IRQF_TRIGGER_FALLING,
	.irq_is_shared = true,
	.num_functions = ARRAY_SIZE(rmi4_extra_functions),
	.func_data = rmi4_extra_functions,
};

static struct rmi4_spi_adapter_platform_data synaptics_platform_data = {
	.attn_gpio = RMI4_SPI_IRQ_GPIO,
	.irq_polarity = IRQF_TRIGGER_FALLING,
	.irq_is_shared = true,
	.gpio_config = rmi4_gpio_config,
	.assert_level_low = true,
	.spi_v2 = {
		.cs_assert = rmi4_force_chip_sel,
		.block_delay_us = 30,
		.split_read_block_delay_us = 30,
		.read_delay_us = 30,
		.write_delay_us = 30,
		.split_read_byte_delay_us = 10,
		.pre_delay_us = 0,
		.post_delay_us = 0,
	},
	.cdev_data = &rmi4_core_data,
};

static void rmi4_spi_cs_control(u32 command)
{
	/* Handled elsewhere to get the required behavior */
	/* This function is here to avoid warning log on SPI *
	 * transactions */
}

static struct pl022_config_chip rmi4_chip_info = {
	.iface = SSP_INTERFACE_MOTOROLA_SPI,
	.hierarchy = SSP_MASTER,

	/* clock freq. 133330000 / (cpsdvsr * (scr +1))
	*  cpsdvsr = 165, src=2,  freq=401596
	*  cpsdvsr = 128, src=0,  freq=1041640
	*  If 0 the spi master will calculate a value
	*  based on the slave speed
	*/
	.clk_freq = {
		.cpsdvsr = 0,
		.scr = 0,
	},
	.com_mode = POLLING_TRANSFER,
	.rx_lev_trig = SSP_RX_1_OR_MORE_ELEM,
	.tx_lev_trig = SSP_TX_1_OR_MORE_EMPTY_LOC,
	.ctrl_len = SSP_BITS_8,
	.wait_state = SSP_MWIRE_WAIT_ZERO,
	.duplex = SSP_MICROWIRE_CHANNEL_FULL_DUPLEX,
	.cs_control = rmi4_spi_cs_control,
};
#endif /* CONFIG_SEMC_GENERIC_RMI4_SPI_ADAPTOR */

/*
 * MSP-SPI
 */

#define NUM_MSP_CLIENTS 10

static struct stm_msp_controller pdp_msp2_spi_data = {
	.id		= MSP_2_CONTROLLER,
	.num_chipselect	= NUM_MSP_CLIENTS,
	.base_addr	= U8500_MSP2_BASE,
	.device_name	= "msp2",
};


#include "board-mop500.h"

#ifdef CONFIG_USB_ANDROID

static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
#ifdef CONFIG_USB_ANDROID_MTP_ARICENT
	"mtp",
#endif
#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
	"usb_mass_storage",
#endif
#ifdef CONFIG_USB_ANDROID_ACCESSORY
	"accessory",
#endif
#ifdef CONFIG_USB_ANDROID_ADB
	"adb",
#endif
#ifdef CONFIG_USB_ANDROID_GG
	"gg",
#endif
};

#ifdef CONFIG_USB_ANDROID_ACCESSORY
static char *usb_functions_accessory[] = {"accessory"};
static char *usb_functions_accessory_adb[] = {"accessory", "adb"};
#endif

#ifdef CONFIG_USB_ANDROID_GG
#define STARTUP_REASON_INDUS_LOG	(1<<29)
#define GG_PID				0xD14C
#endif

static char *usb_functions_gg[] = {
	"gg",
};

static char *usb_functions_mtp_msc_adb[] = {
	"mtp",
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_eng[] = {
	"acm",
	"mtp",
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_mtp_eng[] = {
	"acm",
	"mtp",
	"adb",
};

static char *usb_functions_mtp[] = {
	"mtp",
};

static char *usb_functions_mtp_adb[] = {
	"mtp",
	"adb",
};

static char *usb_functions_mtp_msc[] = {
	"mtp",
	"usb_mass_storage",
};
static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
#ifdef CONFIG_USB_ANDROID_PHONET
	"phonet",
#endif
};

static char *usb_functions_msc[] = {
	"usb_mass_storage",
};

static struct android_usb_product usb_products[] = {
	{
		.product_id	= 0x0000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp),
		.functions	= usb_functions_mtp,
	},
	{
		.product_id	= 0x4000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_msc),
		.functions	= usb_functions_mtp_msc,
		.msc_mode	= STORAGE_MODE_CDROM,
	},
	{
		.product_id	= 0x5000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_adb),
		.functions	= usb_functions_mtp_adb,
	},
	{
		.product_id	= 0x7000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
	},
	{
		.product_id	= 0x8000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
		.functions	= usb_functions_rndis_adb,
	},
	{
		.product_id	= 0x8146,
		.num_functions	= ARRAY_SIZE(usb_functions_eng),
		.functions	= usb_functions_eng,
		.msc_mode	= STORAGE_MODE_MSC,
	},
	{
		.product_id	= 0x9146,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_eng),
		.functions	= usb_functions_mtp_eng,
	},
	{
		.product_id	= 0xA000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_msc),
		.functions	= usb_functions_mtp_msc,
		.msc_mode	= STORAGE_MODE_MSC,
	},
	{
		.product_id	= 0xB000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_msc_adb),
		.functions	= usb_functions_mtp_msc_adb,
		.msc_mode	= STORAGE_MODE_MSC,
	},
	{
		.product_id	= 0xE000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_msc),
		.functions	= usb_functions_msc,
	},
	{
		.product_id	= 0xD14C,
		.num_functions	= ARRAY_SIZE(usb_functions_gg),
		.functions	= usb_functions_gg,
	},
#ifdef CONFIG_USB_ANDROID_ACCESSORY
	{
		.vendor_id	= USB_ACCESSORY_VENDOR_ID,
		.product_id	= USB_ACCESSORY_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_accessory),
		.functions	= usb_functions_accessory,
	},
	{
		.vendor_id	= USB_ACCESSORY_VENDOR_ID,
		.product_id	= USB_ACCESSORY_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_accessory_adb),
		.functions	= usb_functions_accessory_adb,
	},
#endif
};

struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x0FCE,
	.product_id	= 0x0000 | CONFIG_USB_PRODUCT_SUFFIX,
	.version	= 0x0100,
	.product_name	= "SEMC HSUSB Device",
	.manufacturer_name = "SEMC",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
	/* .serial_number is set by setup_serial_number() */
};

struct platform_device ux500_android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
	.platform_data = &android_usb_pdata,
	},
};

#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
	.vendor		= "SEMC",
	.product	= "Mass Storage",
	.release	= 0x0100,

	.cdrom_nluns	= 1,
	.cdrom_vendor	= "SEMC",
	.cdrom_product	= "CD-ROM",
	.cdrom_release	= 0x0100,

	/* EUI-64 based identifier format */
	.eui64_id = {
		.ieee_company_id = {0x00, 0x0A, 0xD9},
		.vendor_specific_ext_field = {0x00, 0x00, 0x00, 0x00, 0x00},
	},
};

struct platform_device ux500_usb_mass_storage_device = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &mass_storage_pdata,
	},
};
#endif /* CONFIG_USB_ANDROID_MASS_STORAGE */

#ifdef CONFIG_USB_ANDROID_GG
static void init_usb_gg(void)
{
	printk(KERN_INFO "init_usb_gg: Enable USB GG\n");
	android_usb_pdata.product_id = GG_PID;
	android_enable_usb_gg();
}
#endif

#define STARTUP_REASON_PWRKEY       (0x01)
#define STARTUP_REASON_WDOG         (0x02)
#define STARTUP_REASON_VBUS         (0x04)

/* Old S1 bit layout */
/* #define STARTUP_REASON_PWRKEY       (0x01) */
#define STARTUP_REASON_WDOG_V1      (0x10)
#define STARTUP_REASON_USB          (0x20)

static unsigned long startup_reason = 0xffffffff;
static unsigned long warmboot_reason = 0xffffffff;

static int __init warmboot_reason_setup(char* warmboot)
{
	int rval = 0;
	rval = strict_strtoul(warmboot, 0, &warmboot_reason);
	if (!rval)
		printk(KERN_INFO "%s: 0x%lx\n", __func__, warmboot_reason);
	return 1;
}

__setup("warmboot=", warmboot_reason_setup);

static int __init startup_reason_setup(char *startup)
{
	int rval = 0;
	rval = strict_strtoul(startup, 0, &startup_reason);
	if (!rval)
		printk(KERN_INFO "%s: 0x%lx\n", __func__, startup_reason);
	return 1;
}

__setup("startup=", startup_reason_setup);

static int __init usb_early_config()
{
	if (startup_reason == 0xffffffff) {
		printk(KERN_ERR "%s: warmboot=0x%lx startup=0x%lx\n",
			__func__, warmboot_reason, startup_reason);
		return 0;
	}

	/* Set MSC only composition in charge only mode. No 'warmboot='
	   found on cmdline means old S1 format is used. */
	if (warmboot_reason == 0xffffffff) {
		if ((startup_reason & STARTUP_REASON_USB) &&
		    !(startup_reason & STARTUP_REASON_PWRKEY) &&
		    !(startup_reason & STARTUP_REASON_WDOG_V1))
			android_usb_pdata.product_id = 0xE000 |
				CONFIG_USB_PRODUCT_SUFFIX;
	} else {
		if ((startup_reason & STARTUP_REASON_VBUS) &&
		    !(startup_reason & STARTUP_REASON_PWRKEY) &&
		    (!warmboot_reason || warmboot_reason == 0xcafe))
			android_usb_pdata.product_id = 0xE000 |
				CONFIG_USB_PRODUCT_SUFFIX;
	}

#ifdef CONFIG_USB_ANDROID_GG
	if (startup_reason & STARTUP_REASON_INDUS_LOG)
		init_usb_gg();
#endif
	return 0;
}

arch_initcall(usb_early_config);

#ifdef CONFIG_USB_ANDROID_RNDIS
static struct usb_ether_platform_data rndis_pdata = {
	/* ethaddr is filled by setup_serial_number */
	.vendorID	= 0x0FCE,
	.vendorDescr	= "SEMC",
};

static struct platform_device usb_rndis_device = {
	.name		= "rndis",
	.id		= -1,
	.dev		= {
		.platform_data = &rndis_pdata,
	},
};
#endif /* CONFIG_USB_ANDROID_RNDIS */

#ifdef CONFIG_USB_ANDROID_RNDIS
static struct usb_ether_platform_data usb_rndis_pdata = {
	.ethaddr	= {0x01, 0x11, 0x22, 0x33, 0x44, 0x55},
	.vendorID	= 0x04cc,
	.vendorDescr = "ST Ericsson",
};

struct platform_device ux500_usb_rndis_device = {
	.name = "rndis",
	.id = -1,
	.dev = {
		.platform_data = &usb_rndis_pdata,
	},
};
#endif /* CONFIG_USB_ANDROID_RNDIS */

#endif /* CONFIG_USB_ANDROID */

#define U8500_I2C_CONTROLLER(id, _slsu, _tft, _rft, clk, t_out, _sm) \
static struct nmk_i2c_controller pdp_i2c##id##_data = { \
	/*				\
	 * slave data setup time, which is	\
	 * 250 ns,100ns,10ns which is 14,6,2	\
	 * respectively for a 48 Mhz	\
	 * i2c clock			\
	 */				\
	.slsu		= _slsu,	\
	/* Tx FIFO threshold */		\
	.tft		= _tft,		\
	/* Rx FIFO threshold */		\
	.rft		= _rft,		\
	/* std. mode operation */	\
	.clk_freq	= clk,		\
	/* Slave response timeout(ms) */\
	.timeout	= t_out,	\
	.sm		= _sm,		\
}

/*
 * The board uses 4 i2c controllers, initialize all of
 * them with slave data setup time of 250 ns,
 * Tx & Rx FIFO threshold values as 1 and standard
 * mode of operation
 */
U8500_I2C_CONTROLLER(0, 0xe, 1, 8, 400000, 200, I2C_FREQ_MODE_FAST);
U8500_I2C_CONTROLLER(1, 0xe, 1, 8, 400000, 200, I2C_FREQ_MODE_FAST);
U8500_I2C_CONTROLLER(2, 0xe, 1, 8, 400000, 200, I2C_FREQ_MODE_FAST);
U8500_I2C_CONTROLLER(3, 0xe, 1, 8, 400000, 200, I2C_FREQ_MODE_FAST);

#ifdef CONFIG_HSI
static struct hsi_board_info __initdata u8500_hsi_devices[] = {
	{
		.name = "hsi_char",
		.hsi_id = 0,
		.port = 0,
		.tx_cfg = {
			.mode = HSI_MODE_STREAM,
			.channels = 2,
			.speed = 100000,
			{.arb_mode = HSI_ARB_RR},
		},
		.rx_cfg = {
			.mode = HSI_MODE_STREAM,
			.channels = 2,
			.speed = 200000,
			{.flow = HSI_FLOW_SYNC},
		},
	},
	{
		.name = "hsi_test",
		.hsi_id = 0,
		.port = 0,
		.tx_cfg = {
			.mode = HSI_MODE_FRAME,
			.channels = 2,
			.speed = 100000,
			{.arb_mode = HSI_ARB_RR},
		},
		.rx_cfg = {
			.mode = HSI_MODE_FRAME,
			.channels = 2,
			.speed = 200000,
			{.flow = HSI_FLOW_SYNC},
		},
	},
	{
		.name = "cfhsi_v3_driver",
		.hsi_id = 0,
		.port = 0,
		.tx_cfg = {
			.mode = HSI_MODE_STREAM,
			.channels = 2,
			.speed = 20000,
			{.arb_mode = HSI_ARB_RR},
		},
		.rx_cfg = {
			.mode = HSI_MODE_STREAM,
			.channels = 2,
			.speed = 200000,
			{.flow = HSI_FLOW_SYNC},
		},
	},
};
#endif

#ifdef CONFIG_AB8500_GPIO
static struct ab8500_gpio_platform_data ab8500_gpio_pdata = {
	.gpio_base		= AB8500_PIN_GPIO1,
	.irq_base		= MOP500_AB8500_VIR_GPIO_IRQ_BASE,
	/* initial_pin_config is the initial configuration of ab8500 pins.
	 * The pins can be configured as GPIO or alt functions based
	 * on value present in GpioSel1 to GpioSel6 and AlternatFunction
	 * register. This is the array of 7 configuration settings.
	 * One has to compile time decide these settings. Below is the
	 * explaination of these setting
	 * GpioSel1 = 0x0F => Pin GPIO1 (SysClkReq2)
	 *                    Pin GPIO2 (SysClkReq3)
	 *                    Pin GPIO3 (SysClkReq4)
	 *                    Pin GPIO4 (SysClkReq6)
	 * GpioSel2 = 0x1E => Pins GPIO10 to GPIO13 are configured as GPIO
	 * GpioSel3 = 0x80 => Pin GPIO24 (SysClkReq7) is configured as GPIO
	 * GpioSel4 = 0x01 => Pin GPIO25 (SysClkReq8) is configured as GPIO
	 * GpioSel5 = 0x78 => Pin GPIO36 (ApeSpiClk)
			      Pin GPIO37 (ApeSpiCSn)
			      Pin GPIO38 (ApeSpiDout)
			      Pin GPIO39 (ApeSpiDin) are configured as GPIO
	 * GpioSel6 = 0x02 => Pin GPIO42 (SysClkReq5) is configured as GPIO
	 * AlternaFunction = 0x00 => If Pins GPIO10 to 13 are not configured
	 * as GPIO then this register selectes the alternate fucntions
	 */
	.initial_pin_config     = {0x0F, 0x1E, 0x80, 0x01, 0x78, 0x02, 0x00},

	/* initial_pin_direction allows for the initial GPIO direction to
	 * be set.
	 */
	.initial_pin_direction  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

	/*
	 * initial_pin_pullups allows for the intial configuration of the
	 * GPIO pullup/pulldown configuration.
	 */
	.initial_pin_pullups    = {0xE0, 0x01, 0x00, 0x00, 0x00, 0x00},
};
#endif

static struct ab8500_sysctrl_platform_data ab8500_sysctrl_pdata = {
	/* SysClkReq1RfClkBuf - SysClkReq8RfClkBuf */
	.initial_req_buf_config
			= {0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00},
	};

#ifdef CONFIG_AB8500_DENC
static struct ab8500_denc_platform_data ab8500_denc_pdata = {
	.ddr_enable = true,
	.ddr_little_endian = false,
};
#endif

static struct regulator_init_data *u8500_regulators[U8500_NUM_REGULATORS] = {
	[U8500_REGULATOR_VAPE]			= &u8500_vape_regulator,
	[U8500_REGULATOR_VARM]			= &u8500_varm_regulator,
	[U8500_REGULATOR_VMODEM]		= &u8500_vmodem_regulator,
	[U8500_REGULATOR_VPLL]			= &u8500_vpll_regulator,
	[U8500_REGULATOR_VSMPS1]		= &u8500_vsmps1_regulator,
	[U8500_REGULATOR_VSMPS2]		= &u8500_vsmps2_regulator,
	[U8500_REGULATOR_VSMPS3]		= &u8500_vsmps3_regulator,
	[U8500_REGULATOR_VRF1]			= &u8500_vrf1_regulator,
	[U8500_REGULATOR_SWITCH_SVAMMDSP]	= &u8500_svammdsp_regulator,
	[U8500_REGULATOR_SWITCH_SVAMMDSPRET]	= &u8500_svammdspret_regulator,
	[U8500_REGULATOR_SWITCH_SVAPIPE]	= &u8500_svapipe_regulator,
	[U8500_REGULATOR_SWITCH_SIAMMDSP]	= &u8500_siammdsp_regulator,
	[U8500_REGULATOR_SWITCH_SIAMMDSPRET]	= &u8500_siammdspret_regulator,
	[U8500_REGULATOR_SWITCH_SIAPIPE]	= &u8500_siapipe_regulator,
	[U8500_REGULATOR_SWITCH_SGA]		= &u8500_sga_regulator,
	[U8500_REGULATOR_SWITCH_B2R2_MCDE]	= &u8500_b2r2_mcde_regulator,
	[U8500_REGULATOR_SWITCH_ESRAM12]	= &u8500_esram12_regulator,
	[U8500_REGULATOR_SWITCH_ESRAM12RET]	= &u8500_esram12ret_regulator,
	[U8500_REGULATOR_SWITCH_ESRAM34]	= &u8500_esram34_regulator,
	[U8500_REGULATOR_SWITCH_ESRAM34RET]	= &u8500_esram34ret_regulator,
};

static struct platform_device u8500_regulator_dev = {
	.name = "u8500-regulators",
	.id   = 0,
	.dev  = {
		.platform_data = u8500_regulators,
	},
};

#ifdef CONFIG_MODEM_U8500
static struct platform_device u8500_modem_dev = {
	.name = "u8500-modem",
	.id   = 0,
	.dev  = {
		.platform_data = NULL,
	},
};
#endif

static struct ab8500_audio_platform_data ab8500_audio_plat_data = {
	.ste_gpio_altf_init = msp13_i2s_init,
	.ste_gpio_altf_exit = msp13_i2s_exit,
};

struct mfd_cell ab8500_extra_devs[] = {
#ifdef CONFIG_SIMPLE_REMOTE_PLATFORM
	{
		.name = SIMPLE_REMOTE_PF_NAME,
		.num_resources = ARRAY_SIZE(simple_remote_resources),
		.resources = simple_remote_resources,
		.platform_data = &simple_remote_pf_data,
		.data_size = sizeof(simple_remote_pf_data),
	}
#endif /* CONFIG_SIMPLE_REMOTE_PLATFORM */
};

static void ab8500_extra_init(struct ab8500 *ab)
{
	int ret = mfd_add_devices(ab->dev, 0, ab8500_extra_devs,
				  ARRAY_SIZE(ab8500_extra_devs), NULL,
				  ab->irq_base);

	if (ret)
		pr_err("%s - Failed to register extra AB8500 MFD devices\n",
		       __func__);
	else
		pr_info("%s - Successfully registered extra AB8500 MFD "
			"devices\n", __func__);
}

/*
 * NOTE! The regulator configuration below must be in exactly the same order as
 * the regulator description in the driver, see drivers/regulator/ab8500.c
 */
static struct ab8500_platform_data ab8500_platdata = {
	.irq_base = MOP500_AB8500_IRQ_BASE,
	.pm_power_off = true,
	.thermal_time_out = 20, /* seconds */
	.regulator = &ab8500_regulator_plat_data,
#ifdef CONFIG_AB8500_DENC
	.denc = &ab8500_denc_pdata,
#endif
	.audio = &ab8500_audio_plat_data,
	.battery = &ab8500_bm_data,
	.charger = &ab8500_charger_plat_data,
	.btemp = &ab8500_btemp_plat_data,
	.fg = &ab8500_fg_plat_data,
	.chargalg = &ab8500_chargalg_plat_data,
#ifdef CONFIG_AB8500_GPIO
	.gpio = &ab8500_gpio_pdata,
#endif
	.sysctrl = &ab8500_sysctrl_pdata,
	.init = ab8500_extra_init,
};

static struct resource ab8500_resources[] = {
	[0] = {
		.start = IRQ_DB8500_AB8500,
		.end = IRQ_DB8500_AB8500,
		.flags = IORESOURCE_IRQ
	}
};

static struct platform_device ux500_ab8500_device = {
	.name = "ab8500-i2c",
	.id = 0,
	.dev = {
		.platform_data = &ab8500_platdata,
	},
	.num_resources = 1,
	.resource = ab8500_resources,
};

#ifdef CONFIG_MFD_CG2900
#define CG2900_GBF_ENA_RESET_GPIO	22
#define WLAN_PMU_EN_GPIO		86
#define CG2900_BT_CTS_GPIO		0

enum cg2900_gpio_pull_sleep cg2900_sleep_gpio[21] = {
	CG2900_NO_PULL,		/* GPIO 0:  PTA_CONFX */
	CG2900_PULL_DN,		/* GPIO 1:  PTA_STATUS */
	CG2900_NO_PULL,		/* GPIO 2:  UART_CTSN */
	CG2900_PULL_UP,		/* GPIO 3:  UART_RTSN */
	CG2900_PULL_UP,		/* GPIO 4:  UART_TXD */
	CG2900_NO_PULL,		/* GPIO 5:  UART_RXD */
	CG2900_PULL_DN,		/* GPIO 6:  IOM_DOUT */
	CG2900_NO_PULL,		/* GPIO 7:  IOM_FSC */
	CG2900_NO_PULL,		/* GPIO 8:  IOM_CLK */
	CG2900_NO_PULL,		/* GPIO 9:  IOM_DIN */
	CG2900_PULL_DN,		/* GPIO 10: PWR_REQ */
	CG2900_PULL_DN,		/* GPIO 11: HOST_WAKEUP */
	CG2900_PULL_DN,		/* GPIO 12: IIS_DOUT */
	CG2900_NO_PULL,		/* GPIO 13: IIS_WS */
	CG2900_NO_PULL,		/* GPIO 14: IIS_CLK */
	CG2900_NO_PULL,		/* GPIO 15: IIS_DIN */
	CG2900_PULL_DN,		/* GPIO 16: PTA_FREQ */
	CG2900_PULL_DN,		/* GPIO 17: PTA_RF_ACTIVE */
	CG2900_NO_PULL,		/* GPIO 18: NotConnected (J6428) */
	CG2900_NO_PULL,		/* GPIO 19: EXT_DUTY_CYCLE */
	CG2900_NO_PULL,		/* GPIO 20: EXT_FRM_SYNCH */
};

static struct platform_device ux500_cg2900_device = {
	.name = "cg2900",
};

#ifdef CONFIG_MFD_CG2900_CHIP
static struct platform_device ux500_cg2900_chip_device = {
	.name = "cg2900-chip",
	.dev = {
		.parent = &ux500_cg2900_device.dev,
	},
};
#endif /* CONFIG_MFD_CG2900_CHIP */

#ifdef CONFIG_MFD_STLC2690_CHIP
static struct platform_device ux500_stlc2690_chip_device = {
	.name = "stlc2690-chip",
	.dev = {
		.parent = &ux500_cg2900_device.dev,
	},
};
#endif /* CONFIG_MFD_STLC2690_CHIP */

#ifdef CONFIG_MFD_CG2900_TEST
static struct cg2900_platform_data cg2900_test_platform_data = {
	.bus = HCI_VIRTUAL,
	.gpio_sleep = cg2900_sleep_gpio,
};

static struct platform_device ux500_cg2900_test_device = {
	.name = "cg2900-test",
	.dev = {
		.parent = &ux500_cg2900_device.dev,
		.platform_data = &cg2900_test_platform_data,
	},
};
#endif /* CONFIG_MFD_CG2900_TEST */

#ifdef CONFIG_MFD_CG2900_UART
static struct resource cg2900_uart_resources[] = {
	{
		.start = CG2900_GBF_ENA_RESET_GPIO,
		.end = CG2900_GBF_ENA_RESET_GPIO,
		.flags = IORESOURCE_IO,
		.name = "gbf_ena_reset",
	},
#if defined(CONFIG_WLAN_PMUEN_UX500)
	{
		.start = WLAN_PMU_EN_GPIO,
		.end = WLAN_PMU_EN_GPIO,
		.flags = IORESOURCE_IO,
		.name = "pmu_en",
	},
#endif
	{
		.start = CG2900_BT_CTS_GPIO,
		.end = CG2900_BT_CTS_GPIO,
		.flags = IORESOURCE_IO,
		.name = "cts_gpio",
	},
	{
		.start = GPIO_TO_IRQ(CG2900_BT_CTS_GPIO),
		.end = GPIO_TO_IRQ(CG2900_BT_CTS_GPIO),
		.flags = IORESOURCE_IRQ,
		.name = "cts_irq",
	},
};

static pin_cfg_t cg2900_uart_enabled[] = {
	GPIO0_U0_CTSn   | PIN_INPUT_PULLUP,
	GPIO1_U0_RTSn   | PIN_OUTPUT_HIGH,
	GPIO2_U0_RXD    | PIN_INPUT_PULLUP,
	GPIO3_U0_TXD    | PIN_OUTPUT_HIGH
};

static pin_cfg_t cg2900_uart_disabled[] = {
	GPIO0_GPIO   | PIN_INPUT_PULLUP,	/* CTS pull up. */
	GPIO1_GPIO   | PIN_OUTPUT_HIGH,		/* RTS high-flow off. */
	GPIO2_GPIO   | PIN_INPUT_PULLUP,	/* RX pull down. */
	GPIO3_GPIO   | PIN_OUTPUT_LOW		/* TX low - break on. */
};

static struct cg2900_platform_data cg2900_uart_platform_data = {
	.bus = HCI_UART,
	.gpio_sleep = cg2900_sleep_gpio,
	.uart = {
		.n_uart_gpios = 4,
		.uart_enabled = cg2900_uart_enabled,
		.uart_disabled = cg2900_uart_disabled,
	},
};

static struct platform_device ux500_cg2900_uart_device = {
	.name = "cg2900-uart",
	.dev = {
		.platform_data = &cg2900_uart_platform_data,
		.parent = &ux500_cg2900_device.dev,
	},
	.num_resources = ARRAY_SIZE(cg2900_uart_resources),
	.resource = cg2900_uart_resources,
};
#endif /* CONFIG_MFD_CG2900_UART */
#endif /* CONFIG_MFD_CG2900 */

/*
 * SPI3
 */
#if defined(CONFIG_TOUCHSCREEN_CYTTSP_SPI) ||			\
	defined(CONFIG_RMI4_SPI) ||				\
	defined(CONFIG_SEMC_GENERIC_RMI4_SPI_ADAPTOR) ||	\
	defined(CONFIG_SEMC_GENERIC_RMI4_SPI_ADAPTOR_MODULE)
#define NUM_SPI3_CLIENTS 1
static struct pl022_ssp_controller pdp_spi3_data = {
	.bus_id		= SPI023_3_CONTROLLER,
	.num_chipselect	= NUM_SPI3_CLIENTS,
};
#endif

static struct spi_board_info u8500_spi_devices[] = {
#ifdef CONFIG_TOUCHSCREEN_CYTTSP_SPI
	{
		.modalias = CY_SPI_NAME,
		/* Need to be 1.4 MHz - lower speed does
		 * not work when APE goes from 100% to 50%
		 */
#ifdef CONFIG_OLIVE_SP1
		.max_speed_hz = 1000000,
#else
		.max_speed_hz = 1400000,
#endif
		.chip_select = 0,
		.bus_num = SPI023_3_CONTROLLER,
		.controller_data = &cyttsp_chip_info,
		.platform_data = &cyttsp_data,
	},
#endif
#ifdef CONFIG_RMI4_SPI
	{
		.modalias = "rmi-spi",
		.max_speed_hz = 2000000,
		.chip_select = 0,
		.bus_num = SPI023_3_CONTROLLER,
		.controller_data = &rmi4_chip_info,
		.platform_data = &synaptics_platform_data,
	},
#endif
#if defined(CONFIG_SEMC_GENERIC_RMI4_SPI_ADAPTOR) ||		\
	defined(CONFIG_SEMC_GENERIC_RMI4_SPI_ADAPTOR_MODULE)
	{
		.modalias = "rmi-spi",
		.max_speed_hz = 2000000,
		.chip_select = 0,
		.bus_num = SPI023_3_CONTROLLER,
		.controller_data = &rmi4_chip_info,
		.platform_data = &synaptics_platform_data,
	},
#endif

};

/* Force feedback vibrator device */
static struct platform_device ste_ff_vibra_device = {
	.name = "ste_ff_vibra"
};

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static struct resource ram_console_resources[] = {
	[0] = {
		.start  = RAM_CONSOLE_START,
		.end    = RAM_CONSOLE_START + RAM_CONSOLE_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct platform_device ram_console_device = {
	.name           = "ram_console",
	.id             = -1,
};

static void ram_console_reserve_mem(void)
{
	if (reserve_bootmem(RAM_CONSOLE_START, RAM_CONSOLE_SIZE,
						BOOTMEM_EXCLUSIVE) != 0) {
		printk(KERN_ERR "ram_console reserve memory failed\n");
		return;
	}

	ram_console_device.num_resources  = ARRAY_SIZE(ram_console_resources);
	ram_console_device.resource       = ram_console_resources;
}
#endif

#define PRCC_K_SOFTRST_SET	0x18
#define PRCC_K_SOFTRST_CLEAR	0x1C
static struct ux500_pins *uart0_pins;
static void ux500_pl011_reset(void)
{
	void __iomem *prcc_rst_set, *prcc_rst_clr;

	prcc_rst_set = (void __iomem *)IO_ADDRESS(U8500_CLKRST1_BASE +
			PRCC_K_SOFTRST_SET);
	prcc_rst_clr = (void __iomem *)IO_ADDRESS(U8500_CLKRST1_BASE +
			PRCC_K_SOFTRST_CLEAR);

	/* Activate soft reset PRCC_K_SOFTRST_CLEAR */
	writel((readl(prcc_rst_clr) | 0x1), prcc_rst_clr);
	udelay(1);

	/* Release soft reset PRCC_K_SOFTRST_SET */
	writel((readl(prcc_rst_set) | 0x1), prcc_rst_set);
	udelay(1);
}

static void ux500_pl011_init(void)
{
	int ret;

	if (!uart0_pins) {
		uart0_pins = ux500_pins_get("uart0");
		if (!uart0_pins) {
			pr_err("pl011: uart0 pins_get failed\n");
			return;
		}
	}

	ret = ux500_pins_enable(uart0_pins);
	if (ret)
		pr_err("pl011: uart0 pins_enable failed\n");
}

static void ux500_pl011_exit(void)
{
	int ret;

	if (uart0_pins) {
		ret = ux500_pins_disable(uart0_pins);
		if (ret)
			pr_err("pl011: uart0 pins_disable failed\n");
	}
}

static struct uart_amba_plat_data ux500_pl011_pdata = {
	.init = ux500_pl011_init,
	.exit = ux500_pl011_exit,
	.reset = ux500_pl011_reset,
};

#ifdef CONFIG_U8500_SIM_DETECT
static struct sim_detect_platform_data sim_detect_pdata = {
	.irq_num		= MOP500_AB8500_VIR_GPIO_IRQ(6),
};
struct platform_device u8500_sim_detect_device = {
	.name	= "sim_detect",
	.id	= 0,
	.dev	= {
			.platform_data          = &sim_detect_pdata,
	},
};
#endif

static struct platform_device *u8500_platform_devices[] __initdata = {
	/*TODO - add platform devices here */
#ifdef CONFIG_SENSORS1P_MOP
	&sensors1p_device,
#endif
#ifdef CONFIG_STM_TRACE
	&ux500_stm_device,
#endif
};

static struct platform_device *platform_devs[] __initdata = {
#ifdef CONFIG_U8500_SIM_DETECT
	&u8500_sim_detect_device,
#endif
	&u8500_shrm_device,
	&ste_ff_vibra_device,
	&ux500_mmio_device,
	&ux500_musb_device,
	&ux500_hwmem_device,
	&ux500_mcde_device,
	&ux500_b2r2_device,
	&ux500_thsens_device,
#ifdef CONFIG_STE_TRACE_MODEM
	&u8500_trace_modem,
#endif
#ifdef CONFIG_USB_ANDROID
	&ux500_android_usb_device,
#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
	&ux500_usb_mass_storage_device,
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	&usb_rndis_device,
#endif
#endif
#ifdef CONFIG_DB8500_MLOADER
	&mloader_fw_device,
#endif
#ifdef CONFIG_HSI
	&u8500_hsi_device,
#endif
#ifdef CONFIG_MODEM_U8500
	&u8500_modem_dev,
#endif
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	&ram_console_device,
#endif
};
static void __init pdp_i2c_init(void)
{
	db8500_add_i2c0(&pdp_i2c0_data);
	db8500_add_i2c1(&pdp_i2c1_data);
	db8500_add_i2c2(&pdp_i2c2_data);
	db8500_add_i2c3(&pdp_i2c3_data);

	i2c_register_board_info(0, ARRAY_AND_SIZE(pdp_i2c0_devices));
	i2c_register_board_info(1, ARRAY_AND_SIZE(pdp_i2c1_devices));
	i2c_register_board_info(2, ARRAY_AND_SIZE(pdp_i2c2_devices));
	i2c_register_board_info(3, ARRAY_AND_SIZE(pdp_i2c3_devices));
}

static void __init pdp_spi_init(void)
{
	db8500_add_msp2_spi(&pdp_msp2_spi_data);
#if defined(CONFIG_TOUCHSCREEN_CYTTSP_SPI) ||			\
	defined(CONFIG_RMI4_SPI) ||				\
	defined(CONFIG_SEMC_GENERIC_RMI4_SPI_ADAPTOR) ||	\
	defined(CONFIG_SEMC_GENERIC_RMI4_SPI_ADAPTOR_MODULE)
	db8500_add_spi3(&pdp_spi3_data);
#endif
}

static void __init pdp_uart_init(void)
{
	db8500_add_uart0(&ux500_pl011_pdata);
	db8500_add_uart1();
	db8500_add_uart2();
}

#ifdef CONFIG_TOUCHSCREEN_CYTTSP_SPI
static void __init cyttsp_data_set_callbacks(struct cyttsp_platform_data *pdata)
{
#ifdef CONFIG_TOUCHSCREEN_CYTTSP_KEY
	pdata->cust_spec = cyttsp_key_rpc_callback;
#endif /* CONFIG_TOUCHSCREEN_CYTTSP_KEY */
	pdata->wakeup = cyttsp_wakeup;
	pdata->init = cyttsp_init;
	pdata->reset = cyttsp_xres;
}
#endif /* CONFIG_TOUCHSCREEN_CYTTSP_SPI */

#define CONSOLE_NAME "ttyAMA"
#define CONSOLE_IX 2
#define CONSOLE_OPTIONS "115200n8"
static int __init setup_serial_console(char *console_flag)
{
	if (console_flag &&
		strlen(console_flag) >= 2 &&
		(console_flag[0] != '0' || console_flag[1] != '0'))
		add_preferred_console(CONSOLE_NAME,
					CONSOLE_IX,
					CONSOLE_OPTIONS);
	return 1;
}

/*
 * The S1 Boot configuration TA unit can specify that the serial console
 * enable flag will be passed as Kernel boot arg with tag babe09A9.
 */
__setup("semcandroidboot.babe09a9=", setup_serial_console);

#ifdef CONFIG_USB_ANDROID
static int __init setup_serial_number(char *serial_number)
{
	static char usb_serial_number[USB_SERIAL_NUMBER_LEN + 1];
	int serial_number_len;
#ifdef CONFIG_USB_ANDROID_RNDIS
	char *src = serial_number;
	int i = 0;

	/* create a MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	rndis_pdata.ethaddr[0] = 0x02;
	if (src != NULL) {
		for (i = 0; *src; i++) {
			/* XOR the USB serial across the remaining bytes */
			rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
		}
	}
#endif
	serial_number_len = strlen(serial_number);
	if (serial_number_len != USB_SERIAL_NUMBER_LEN)
		printk(KERN_ERR "Serial number length was %d, expected %d.",
			serial_number_len, USB_SERIAL_NUMBER_LEN);

	strncpy(usb_serial_number, serial_number, USB_SERIAL_NUMBER_LEN);
	usb_serial_number[USB_SERIAL_NUMBER_LEN] = '\0';
	android_usb_pdata.serial_number = usb_serial_number;
	mass_storage_pdata.serial_number = usb_serial_number;

	printk(KERN_INFO "USB serial number: %s\n",
		android_usb_pdata.serial_number);

	return 1;
}

__setup("androidboot.serialno=", setup_serial_number);
#endif
static void __init pdp_init_machine(void)
{
#ifdef CONFIG_REGULATOR
	platform_device_register(&u8500_regulator_dev);
#endif
	u8500_init_devices();

	/*
	 * NOTE: Users of following devices (uart, i2c, msp, spi)
	 * should register only after these devices are registered
	 */
	pdp_i2c_init();
	mop500_msp_init();
	pdp_spi_init();
	pdp_uart_init();

	platform_add_devices(platform_devs,
			ARRAY_SIZE(platform_devs));

	mop500_pins_init();

	platform_device_register(&ux500_ab8500_device);

#ifdef CONFIG_MFD_CG2900
#ifdef CONFIG_MFD_CG2900_TEST
	dcg2900_init_platdata(&cg2900_test_platform_data);
#endif /* CONFIG_MFD_CG2900_TEST */
#ifdef CONFIG_MFD_CG2900_UART
	dcg2900_init_platdata(&cg2900_uart_platform_data);
#endif /* CONFIG_MFD_CG2900_UART */

	platform_device_register(&ux500_cg2900_device);
#ifdef CONFIG_MFD_CG2900_UART
	platform_device_register(&ux500_cg2900_uart_device);
#endif /* CONFIG_MFD_CG2900_UART */
#ifdef CONFIG_MFD_CG2900_TEST
	platform_device_register(&ux500_cg2900_test_device);
#endif /* CONFIG_MFD_CG2900_TEST */
#ifdef CONFIG_MFD_CG2900_CHIP
	platform_device_register(&ux500_cg2900_chip_device);
#endif /* CONFIG_MFD_CG2900_CHIP */
#ifdef CONFIG_MFD_STLC2690_CHIP
	platform_device_register(&ux500_stlc2690_chip_device);
#endif /* CONFIG_MFD_STLC2690_CHIP */
#endif /* CONFIG_MFD_CG2900 */

	pdp_wlan_init();

#ifdef CONFIG_TOUCHSCREEN_CYTTSP_SPI
	cyttsp_data_set_callbacks(&cyttsp_data);
#endif /* CONFIG_TOUCHSCREEN_CYTTSP_SPI */

#ifdef CONFIG_KEYBOARD_NOMADIK_SKE
	db8500_add_ske_keypad(get_ske_keypad_data());
#endif

#ifdef CONFIG_HSI
	hsi_register_board_info(u8500_hsi_devices,
				ARRAY_SIZE(u8500_hsi_devices));
#endif

#ifdef CONFIG_MOP500_NUIB
	pdp_nuib_init();
#endif

#ifdef CONFIG_ANDROID_STE_TIMED_VIBRA
	mop500_vibra_init();
#endif

	platform_add_devices(u8500_platform_devices,
			     ARRAY_SIZE(u8500_platform_devices));

	spi_register_board_info(u8500_spi_devices,
				ARRAY_SIZE(u8500_spi_devices));
}

void __init pdp_map_io(void)
{
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	ram_console_reserve_mem();
#endif
	u8500_map_io();
}

MACHINE_START(NOMADIK, "riogrande")
	/* Maintainer: Sony Ericsson */
	.phys_io	= U8500_UART2_BASE,
	.io_pg_offst	= (IO_ADDRESS(U8500_UART2_BASE) >> 18) & 0xfffc,
	.boot_params	= 0x00000100,
	.map_io		= pdp_map_io,
	.init_irq	= ux500_init_irq,
	.timer		= &u8500_timer,
	.init_machine	= pdp_init_machine,
MACHINE_END
