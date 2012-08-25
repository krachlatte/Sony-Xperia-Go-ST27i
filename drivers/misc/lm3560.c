/* drivers/misc/lm3560.c
 *
 * Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * Author: Kazunari YOSHINO <Kazunari.X.Yoshino@sonyericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
*/

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/lm3560.h>
#include <linux/kernel.h>
#include <linux/slab.h>

/*
 ******************************************************************************
 * - Value declaration
 * - Prototype declaration
 ******************************************************************************
 */

/*
 * Regitster address
 */
#define LM3560_REG_ENABLE		0x10
#define LM3560_REG_PRIVACY		0x11
#define LM3560_REG_GPIO			0x20
#define LM3560_REG_VIN_MONITOR		0x80
#define LM3560_REG_TORCH_BRIGHT		0xA0
#define LM3560_REG_FLASH_BRIGHT		0xB0
#define LM3560_REG_FLASH_DURATION	0xC0
#define LM3560_REG_FLAG			0xD0
#define LM3560_REG_CFG_1		0xE0
#define LM3560_REG_CFG_2		0xF0

/*
 * Mask/Value of Enable register
 */
/* EN0 bit & EN1 bit */
#define LM3560_ENABLE_EN_MASK			(0x03 << 0)
#define LM3560_ENABLE_EN_SHUTDOWN		(0x00 << 0)
#define LM3560_ENABLE_EN_TORCH_MODE		(0x02 << 0)
#define LM3560_ENABLE_EN_FLASH_MODE		(0x03 << 0)
#define LM3560_ENABLE_EN_PRIVACY_MODE		(0x01 << 0)

/* LED1 Enable bit */
#define LM3560_ENABLE_LED1_MASK			(0x01 << 3)
#define LM3560_ENABLE_LED1_OFF			(0x00 << 3)
#define LM3560_ENABLE_LED1_ON			(0x01 << 3)

/* LED2 Enable bit */
#define LM3560_ENABLE_LED2_MASK			(0x01 << 4)
#define LM3560_ENABLE_LED2_OFF			(0x00 << 4)
#define LM3560_ENABLE_LED2_ON			(0x01 << 4)

/* Privacy Terminate bit */
#define LM3560_PRIVACY_TERMINATE_MASK		(0x01 << 3)
#define LM3560_PRIVACY_TERMINATE_TURN_BACK	(0x00 << 3)
#define LM3560_PRIVACY_TERMINATE_REMAIN_OFF	(0x01 << 3)

/* LED1 Privacy bit */
#define LM3560_PRIVACY_LED1_MASK		(0x01 << 4)
#define LM3560_PRIVACY_LED1_OFF			(0x00 << 4)
#define LM3560_PRIVACY_LED1_ON			(0x01 << 4)

/* LED2 Privacy bit */
#define LM3560_PRIVACY_LED2_MASK		(0x01 << 5)
#define LM3560_PRIVACY_LED2_OFF			(0x00 << 5)
#define LM3560_PRIVACY_LED2_ON			(0x01 << 5)

/* Blink bit */
#define LM3560_PRIVACY_BLINK_MASK		(0x03 << 6)
#define LM3560_PRIVACY_BLINK_DISABLE		(0x00 << 6)
#define LM3560_PRIVACY_BLINK_128MS		(0x01 << 6)
#define LM3560_PRIVACY_BLINK_256MS		(0x02 << 6)
#define LM3560_PRIVACY_BLINK_512MS		(0x03 << 6)

/*
 * Mask/Value of Configuration register 1
 */
/* STROBE Input Enable bit */
#define LM3560_CFG1_STROBE_INPUT_MASK		(0x01 << 2)
#define LM3560_CFG1_STROBE_INPUT_DISABLE	(0x00 << 2)
#define LM3560_CFG1_STROBE_INPUT_ENABLE		(0x01 << 2)

/* STROBE Polarity bit */
#define LM3560_CFG1_STROBE_POLARITY_MASK	(0x01 << 3)
#define LM3560_CFG1_STROBE_POLARITY_LOW		(0x00 << 3)
#define LM3560_CFG1_STROBE_POLARITY_HIGH	(0x01 << 3)

/* NTC Mode Enable bit */
#define LM3560_CFG1_NTC_MASK			(0x01 << 4)
#define LM3560_CFG1_NTC_INDICATOR_OUTPUT	(0x00 << 4)
#define LM3560_CFG1_NTC_NTC_THERMISTOR_INPUT	(0x01 << 4)

/* TX1 Polarity bit */
#define LM3560_CFG1_TX1_POLARITY_MASK		(0x01 << 5)
#define LM3560_CFG1_TX1_POLARITY_LOW		(0x00 << 5)
#define LM3560_CFG1_TX1_POLARITY_HIGH		(0x01 << 5)

/* TX2 Polarity bit */
#define LM3560_CFG1_TX2_POLARITY_MASK		(0x01 << 6)
#define LM3560_CFG1_TX2_POLARITY_LOW		(0x00 << 6)
#define LM3560_CFG1_TX2_POLARITY_HIGH		(0x01 << 6)

/* Hardware Torch Mode Enable bit */
#define LM3560_CFG1_HW_TORCH_MODE_MASK		(0x01 << 7)
#define LM3560_CFG1_HW_TORCH_MODE_DISABLE	(0x00 << 7)
#define LM3560_CFG1_HW_TORCH_MODE_ENABLE	(0x01 << 7)

/*
 * Mask of Torch Brightness Register
 */
#define LM3560_TORCH_BRIGHT_MASK		(0x3f)

/*
 * LM3560 Mask of Flash Brightness Register
 */
#define LM3560_FLASH_BRIGHT_MASK		(0xff)

/*
 * LM3560 Mask of Privacy Brightness Register
 */
#define LM3560_PRIVACY_BRIGHT_MASK		(0xff)

/*
 * LM3560 Mask of Configuration Register 1
 */
#define LM3560_CFG_1_MASK			(0x7f)

/*
 * Mask of Flash Duration register
 */
#define LM3560_FLASH_DURATION_MASK		(0x1f)

/* Current Limit bit */
#define LM3560_FLASH_DURATION_CL_MASK		(0x03 << 5)
#define LM3560_FLASH_DURATION_CL_1400MA		(0x00 << 5)
#define LM3560_FLASH_DURATION_CL_2100MA		(0x01 << 5)
#define LM3560_FLASH_DURATION_CL_2800MA		(0x02 << 5)
#define LM3560_FLASH_DURATION_CL_3500MA		(0x03 << 5)

/*
 * Mask of GPIO Register
 */
/* TX2/INT/GPIO2 pin control bit */
#define LM3560_GPIO_CTRL_MASK			(0x01 << 3)
#define LM3560_GPIO_CTRL_FLASH			(0x00 << 3)

/* TX2/INT/GPIO2 pin data bit */
#define LM3560_GPIO_DATA_MASK			(0x01 << 5)

/* Flash duration of libhardware_legacy */
#define LM3560_FLASH_CURRENT_LEGACY		(125000)
#define LM3560_FLASH_DURATION_LEGACY		(128000)


#define LM3560_FLAGS_FLASH_EXPIRE	(0x01)

/* H/W control */
#define LM3560_HW_RESET		0
#define LM3560_HW_ACTIVE	1

#define STROBE_TRIGGER_SHIFT 5

struct led_limits {
	unsigned long torch_current_min;
	unsigned long torch_current_max;
	unsigned long flash_current_min;
	unsigned long flash_current_max;
	unsigned long flash_duration_min;
	unsigned long flash_duration_max;
	unsigned long privacy_current_min;
	unsigned long privacy_current_max;
};

/* Each value below is for each LED output. So if 2 outputs are used,
 * limits will be double based on led_nums set in board-file.
*/
const struct led_limits lm3560_limits = {
	31250,
	250000,
	62500,
	1000000,
	32000,
	1024000,
	3125,
	25000
};

struct lm3560_drv_data {
	struct i2c_client *client;
	/* The value except T0-T4 bits of Flash Duration register */
	u8 reg_flash_duration_limit;
	/* The value except StrobeInputEnable bit of Configuration Register1 */
	u8 reg_cfg1;
	/* The value except DP0-DP2 bits of Privacy register */
	u8 reg_privacy;
	int led_nums;
	int privacy_led_nums;
	int torch_current_shift;
	int flash_current_shift;
	int strobe_trigger_shift;
};


static int lm3560_get_reg_data(struct lm3560_drv_data *data,
				u8 addr, u8 *value)
{
	s32 result;

	result = i2c_smbus_read_i2c_block_data(
					data->client,
					addr,
					1,
					value);
	if (result < 0) {
		dev_err(&data->client->dev,
			"%s(): Failed to read register(0x%02x). "
				"errno=%d\n",
				__func__, addr, result);
		return -EIO;
	}

	return 0;
}

static int lm3560_set_reg_data(struct lm3560_drv_data *data,
				u8 addr, u8 mask, u8 value)
{
	u8 current_value;
	s32 result;

	if (mask != 0xFF) {
		result = i2c_smbus_read_i2c_block_data(
						data->client,
						addr,
						1,
						&current_value);
		if (result < 0) {
			dev_err(&data->client->dev,
				"%s(): Failed to read register(0x%02x)"
					". errno=%d\n",
					__func__, addr, result);
			return -EIO;
		}

		value = (current_value & ~mask) | value;
	}
	/* For debug-purpose, get info on what is written to chip */
	dev_dbg(&data->client->dev,
		"%s write register(0x%02x) data(0x%02x)\n",
		__func__, addr, value);

	result = i2c_smbus_write_i2c_block_data(
					data->client,
					addr,
					1,
					&value);
	if (result < 0) {
		dev_err(&data->client->dev,
			"%s(): Failed to write register(0x%02x). "
				"errno=%d\n",
				__func__, addr, result);
		return -EIO;
	}

	return 0;
}

static int lm3560_set_flash_sync(struct lm3560_drv_data *data,
				enum lm3560_sync_state setting)
{
	if (setting == LM3560_SYNC_ON)
		return lm3560_set_reg_data(data,
				LM3560_REG_CFG_1,
				LM3560_CFG_1_MASK,
				data->reg_cfg1
				| LM3560_CFG1_STROBE_INPUT_ENABLE);
	else
		return lm3560_set_reg_data(data,
				LM3560_REG_CFG_1,
				LM3560_CFG_1_MASK,
				data->reg_cfg1
				| LM3560_CFG1_STROBE_INPUT_DISABLE);

}

static int lm3560_get_privacy_current(struct lm3560_drv_data *data,
				    unsigned long *get_current)
{

	int result;
	u8 reg_current;
	result = lm3560_get_reg_data(data,
				     LM3560_REG_PRIVACY,
				     &reg_current);
	if (result)
		return result;

	*get_current = ((reg_current & 0x07) + 1)
		* lm3560_limits.privacy_current_min;

	return result;

}

static int lm3560_set_privacy_current(struct lm3560_drv_data *data,
					unsigned long privacy_current)
{
	u8 current_bits_value;
	u8 reg_value;

	if ((privacy_current < lm3560_limits.privacy_current_min) ||
	    (privacy_current > lm3560_limits.privacy_current_max)) {
		dev_err(&data->client->dev,
			"%s(): Value(%luuA) should be from %luuA to %luuA\n",
			__func__, privacy_current,
			lm3560_limits.privacy_current_min,
			lm3560_limits.privacy_current_max);

		if (privacy_current < lm3560_limits.privacy_current_min)
			privacy_current = lm3560_limits.privacy_current_min;
		else if (privacy_current > lm3560_limits.privacy_current_max)
			privacy_current = lm3560_limits.privacy_current_max;

		dev_err(&data->client->dev,
			"%s(): Value is now set to %luuA\n",
			__func__, privacy_current);
	}

	/* Convert current value to register value */
	current_bits_value =
		(privacy_current / lm3560_limits.privacy_current_min) - 1;

	reg_value = current_bits_value | data->reg_privacy;

	return lm3560_set_reg_data(data,
				LM3560_REG_PRIVACY,
				LM3560_PRIVACY_BRIGHT_MASK,
				reg_value);

}

static int lm3560_check_status(struct lm3560_drv_data *data, u8 *return_status)
{
	u8 status = 0;
	int error;

	*return_status = 0;

	/* set Tx2/GPIO2 Control as flash interrupt input */
	error = lm3560_set_reg_data(data,
				LM3560_REG_GPIO,
				LM3560_GPIO_CTRL_MASK,
				LM3560_GPIO_CTRL_FLASH);
	if (error)
		return error;

	error = lm3560_get_reg_data(data, LM3560_REG_FLAG, &status);
	if (error)
		return error;

	*return_status &= status;

	return error;

}

static int lm3560_privacy_mode(struct lm3560_drv_data *data,
				int setting)
{
	int result;

	if (setting)
		result = lm3560_set_reg_data(data,
					     LM3560_REG_ENABLE,
					     LM3560_ENABLE_EN_MASK,
					     LM3560_ENABLE_EN_PRIVACY_MODE);
	else
		result = lm3560_set_reg_data(data,
				     LM3560_REG_ENABLE,
				     LM3560_ENABLE_EN_MASK,
				     LM3560_ENABLE_EN_SHUTDOWN);

	return result;
}

static int lm3560_torch_mode(struct lm3560_drv_data *data,
				unsigned setting)
{
	int result;

	if (setting)
		result = lm3560_set_reg_data(data,
					LM3560_REG_ENABLE,
					LM3560_ENABLE_EN_MASK,
					LM3560_ENABLE_EN_TORCH_MODE);
	else
		result = lm3560_set_reg_data(data,
				LM3560_REG_ENABLE,
				LM3560_ENABLE_EN_MASK,
				LM3560_ENABLE_EN_SHUTDOWN);

	return result;
}

static int lm3560_flash_mode(struct lm3560_drv_data *data,
				unsigned setting)
{
	int result;

	if (setting)
		result = lm3560_set_reg_data(data,
					LM3560_REG_ENABLE,
					LM3560_ENABLE_EN_MASK,
					LM3560_ENABLE_EN_FLASH_MODE);
	else
		result = lm3560_set_reg_data(data,
					LM3560_REG_ENABLE,
					LM3560_ENABLE_EN_MASK,
					LM3560_ENABLE_EN_SHUTDOWN);

	return result;
}

static int lm3560_get_torch_current(struct lm3560_drv_data *data,
				    unsigned long *get_current)
{

	u8 leds = 1, reg_current;
	int result;

	if (2 == data->led_nums) {
		result = lm3560_get_reg_data(data,
				     LM3560_REG_ENABLE,
				     &leds);
		if (result)
			return result;

		leds = leds & LM3560_ENABLE_LED2_MASK ? 2 : 1;

	}
	result = lm3560_get_reg_data(data,
				     LM3560_REG_TORCH_BRIGHT,
				     &reg_current);
	if (result)
		return result;

	*get_current = ((reg_current & 0x07) + 1)
		* lm3560_limits.torch_current_min * leds;

	return result;

}

static int lm3560_set_torch_current(struct lm3560_drv_data *data,
				    unsigned long request_current)

{
	u8 current_bits_value;
	int result;
	int leds = 1;

	if ((request_current < (lm3560_limits.torch_current_min
		* data->led_nums)) || (request_current >
		(lm3560_limits.torch_current_max * data->led_nums))) {
		dev_err(&data->client->dev,
			"%s(): Value (%luuA) should be from %luuA to %luuA\n",
			__func__, request_current,
			lm3560_limits.torch_current_min * data->led_nums,
			lm3560_limits.torch_current_max * data->led_nums);

		if (request_current < (lm3560_limits.torch_current_min
				* data->led_nums))
			request_current = (lm3560_limits.torch_current_min
				* data->led_nums);
		else if (request_current > (lm3560_limits.torch_current_max
				* data->led_nums))
			request_current = (lm3560_limits.torch_current_max
				* data->led_nums);

		dev_err(&data->client->dev,
			"%s(): Value is now set to %luuA\n",
			__func__, request_current);
	}

	if (2 == data->led_nums) {
		/* If request current is more than "minimum current * 2LED" */
		if (request_current >= (lm3560_limits.torch_current_min
				* data->led_nums)) {
			result = lm3560_set_reg_data(data,
						     LM3560_REG_ENABLE,
						     LM3560_ENABLE_LED2_MASK,
						     LM3560_ENABLE_LED2_ON);
			if (result == 0)
				leds = 2;
			else
				return result;
		} else {
			result = lm3560_set_reg_data(data,
						     LM3560_REG_ENABLE,
						     LM3560_ENABLE_LED2_MASK,
						     LM3560_ENABLE_LED2_OFF);
			if (result == 0)
				leds = 1;
			else
				return result;
		}
	}


	/* Convert current value to register value (Round-down fraction) */
	current_bits_value =
		request_current	/
		(lm3560_limits.torch_current_min * leds)  - 1;

	current_bits_value = (current_bits_value << data->torch_current_shift)
		| current_bits_value;

	return lm3560_set_reg_data(data,
				     LM3560_REG_TORCH_BRIGHT,
				     LM3560_TORCH_BRIGHT_MASK,
				     current_bits_value);

}

static int lm3560_get_flash_current(struct lm3560_drv_data *data,
				    unsigned long *get_current)
{

	u8 leds = 1, reg_current;
	int result;

	if (2 == data->led_nums) {
		result = lm3560_get_reg_data(data,
				     LM3560_REG_ENABLE,
				     &leds);
		if (result != 0)
			return result;

		leds = leds & LM3560_ENABLE_LED2_MASK ? 2 : 1;

	}

	result = lm3560_get_reg_data(data,
				     LM3560_REG_FLASH_BRIGHT,
				     &reg_current);
	if (result)
		return result;

	*get_current = ((reg_current & 0x0f) + 1)
		* lm3560_limits.flash_current_min * leds;

	return result;

}

static int lm3560_set_flash_current(struct lm3560_drv_data *data,
				    unsigned long flash_current)

{
	u8 current_bits_value;
	int result = 0, leds = 1;

	if ((flash_current < (lm3560_limits.flash_current_min
		* data->led_nums)) ||
	    (flash_current > (lm3560_limits.flash_current_max
		* data->led_nums))) {
		dev_err(&data->client->dev,
			"%s(): Value (%luuA) should be from %luuA to %luuA.\n",
			__func__, flash_current,
			lm3560_limits.flash_current_min * data->led_nums,
			lm3560_limits.flash_current_max * data->led_nums);

		if (flash_current < (lm3560_limits.flash_current_min
				* data->led_nums))
			flash_current = (lm3560_limits.flash_current_min
				* data->led_nums);
		else if (flash_current > (lm3560_limits.flash_current_max
				* data->led_nums))
			flash_current = (lm3560_limits.flash_current_max
				* data->led_nums);

		dev_err(&data->client->dev,
			"%s(): Value is now set to %luuA\n",
			__func__, flash_current);
	}

	if (2 == data->led_nums) {
		/* if request current is more than "minimum current * 2LED" */
		if (flash_current >= (lm3560_limits.flash_current_min
				* data->led_nums)) {
			result = lm3560_set_reg_data(data,
						     LM3560_REG_ENABLE,
						     LM3560_ENABLE_LED2_MASK,
						     LM3560_ENABLE_LED2_ON);
			if (!result)
				leds = 2;
			else
				return result;
		} else {
			result = lm3560_set_reg_data(data,
						     LM3560_REG_ENABLE,
						     LM3560_ENABLE_LED2_MASK,
						     LM3560_ENABLE_LED2_OFF);
			if (!result)
				leds = 1;
			else
				return result;
		}
	}

	/* Convert current value to register value (Round-down fraction) */
	current_bits_value =
		flash_current /
		(lm3560_limits.flash_current_min * leds) - 1;

	current_bits_value = (current_bits_value << data->flash_current_shift)
		| current_bits_value;

	return lm3560_set_reg_data(data,
			LM3560_REG_FLASH_BRIGHT,
			LM3560_FLASH_BRIGHT_MASK,
			current_bits_value);

}

static int lm3560_get_flash_duration(struct lm3560_drv_data *data,
				    unsigned long *flash_duration)
{
	u8 reg_duration;
	int result;
	result = lm3560_get_reg_data(data,
				     LM3560_REG_FLASH_DURATION,
				     &reg_duration);
	if (result != 0)
		return result;

	*flash_duration = ((reg_duration & LM3560_FLASH_DURATION_MASK) + 1)
		* lm3560_limits.flash_duration_min;

	return result;

}


static int lm3560_set_flash_duration(struct lm3560_drv_data *data,
					unsigned long flash_duration)
{
	u8 duration_bits_value;

	if ((flash_duration < lm3560_limits.flash_duration_min) ||
	    (flash_duration > lm3560_limits.flash_duration_max)) {
		dev_err(&data->client->dev,
			"%s(): Value (%luus) should be from %luus to %luus\n",
			__func__, flash_duration,
			lm3560_limits.flash_duration_min,
			lm3560_limits.flash_duration_max);

		if (flash_duration < lm3560_limits.flash_duration_min)
			flash_duration = lm3560_limits.flash_duration_min;
		else if (flash_duration > lm3560_limits.flash_duration_max)
			flash_duration = lm3560_limits.flash_duration_max;

		dev_err(&data->client->dev,
			"%s(): Value is now set to %luus\n",
			__func__, flash_duration);
	}

	/* Convert duration value to register value. (Round-up fraction) */
	duration_bits_value =
		(flash_duration - 1) / lm3560_limits.flash_duration_min;

	return lm3560_set_reg_data(data, LM3560_REG_FLASH_DURATION,
			     LM3560_FLASH_DURATION_MASK, duration_bits_value);

}

static int lm3560_init_enable_register(struct lm3560_drv_data *data,
					struct lm3560_platform_data *pdata)
{
	int result;
	u8 value = 0;

	if (2 == pdata->led_nums) {
		switch (pdata->led_nums) {
		case 1:
			value = LM3560_ENABLE_LED1_ON | LM3560_ENABLE_LED2_OFF;
			break;
		case 2:
			value = LM3560_ENABLE_LED1_ON | LM3560_ENABLE_LED2_ON;
			break;
		default:
			dev_err(&data->client->dev,
				"%s(): led_nums(%d) is invald\n",
				__func__, pdata->led_nums);
			return -EINVAL;
		}
	}

	if (pdata->strobe_trigger)
		value |= (1 << STROBE_TRIGGER_SHIFT);

	result = lm3560_set_reg_data(data,
				     LM3560_REG_ENABLE,
				     LM3560_ENABLE_LED1_MASK
				     | LM3560_ENABLE_LED2_MASK
				     | (1 << STROBE_TRIGGER_SHIFT),
				     value);

	return result;
}

static int lm3560_init_privacy_register(struct lm3560_drv_data *data,
					struct lm3560_platform_data *pdata)
{

	switch (pdata->privacy_terminate) {
	case LM3560_PRIVACY_MODE_TURN_BACK:
		data->reg_privacy = LM3560_PRIVACY_TERMINATE_TURN_BACK;
		break;
	case LM3560_PRIVACY_MODE_REMAIN_OFF:
		data->reg_privacy = LM3560_PRIVACY_TERMINATE_REMAIN_OFF;
		break;
	default:
		dev_err(&data->client->dev,
			"%s(): privacy_terminate(%d) is invald\n",
			__func__, pdata->privacy_terminate);
		return -EINVAL;
	}

	switch (pdata->privacy_led_nums) {
	case 1:
		data->reg_privacy |=
			(LM3560_PRIVACY_LED1_ON | LM3560_PRIVACY_LED2_OFF);
		break;
	case 2:
		data->reg_privacy |=
			(LM3560_PRIVACY_LED1_ON | LM3560_PRIVACY_LED2_ON);
		break;
	default:
		dev_err(&data->client->dev,
			"%s(): privacy_led_nums(%d) is invald\n",
			__func__, pdata->privacy_led_nums);
		return -EINVAL;
	}

	if (pdata->privacy_blink_period == 0) {
		data->reg_privacy |= LM3560_PRIVACY_BLINK_DISABLE;

	} else if (pdata->privacy_blink_period <= 128000) {
		data->reg_privacy |= LM3560_PRIVACY_BLINK_128MS;

	} else if (pdata->privacy_blink_period <= 256000) {
		data->reg_privacy |= LM3560_PRIVACY_BLINK_256MS;

	} else if (pdata->privacy_blink_period <= 512000) {
		data->reg_privacy |= LM3560_PRIVACY_BLINK_512MS;

	} else { /* privacy_blink_period > 512000us */
		dev_err(&data->client->dev,
			"%s(): privacy_blink_period(%luus) is invald\n",
			__func__, pdata->privacy_blink_period);
		return -EINVAL;
	}

	return lm3560_set_reg_data(data,
				LM3560_REG_PRIVACY,
				LM3560_PRIVACY_TERMINATE_MASK
				| LM3560_PRIVACY_LED1_MASK
				| LM3560_PRIVACY_LED2_MASK
				| LM3560_PRIVACY_BLINK_MASK,
				data->reg_privacy);
}

static int lm3560_init_cfg1_register(struct lm3560_drv_data *data,
					struct lm3560_platform_data *pdata)
{
	int result;

	switch (pdata->strobe_polarity) {
	case LM3560_STROBE_POLARITY_LOW:
		data->reg_cfg1 = LM3560_CFG1_STROBE_POLARITY_LOW;
		break;
	case LM3560_STROBE_POLARITY_HIGH:
		data->reg_cfg1 = LM3560_CFG1_STROBE_POLARITY_HIGH;
		break;
	default:
		dev_err(&data->client->dev,
			"%s(): strobe_polarity(%d) is invald\n",
			__func__, pdata->strobe_polarity);
		return -EINVAL;
	}

	switch (pdata->ledintc_pin_setting) {
	case LM3560_LEDINTC_INDICATOR_OUTPUT:
		data->reg_cfg1 |= LM3560_CFG1_NTC_INDICATOR_OUTPUT;
		break;
	case LM3560_LEDINTC_NTC_THERMISTOR_INPUT:
		data->reg_cfg1 |= LM3560_CFG1_NTC_NTC_THERMISTOR_INPUT;
		break;
	default:
		dev_err(&data->client->dev,
			"%s(): ledintc_pin_setting(%d) is invald\n",
			__func__, pdata->ledintc_pin_setting);
		return -EINVAL;
	}

	switch (pdata->tx1_polarity) {
	case LM3560_TX1_POLARITY_LOW:
		data->reg_cfg1 |= LM3560_CFG1_TX1_POLARITY_LOW;
		break;
	case LM3560_TX1_POLARITY_HIGH:
		data->reg_cfg1 |= LM3560_CFG1_TX1_POLARITY_HIGH;
		break;
	default:
		dev_err(&data->client->dev,
			"%s(): tx1_polarity(%d) is invald\n",
			__func__, pdata->tx1_polarity);
		return -EINVAL;
	}

	switch (pdata->tx2_polarity) {
	case LM3560_TX2_POLARITY_LOW:
		data->reg_cfg1 |= LM3560_CFG1_TX2_POLARITY_LOW;
		break;
	case LM3560_TX2_POLARITY_HIGH:
		data->reg_cfg1 |= LM3560_CFG1_TX2_POLARITY_HIGH;
		break;
	default:
		dev_err(&data->client->dev,
			"%s(): tx2_polarity(%d) is invald\n",
			__func__, pdata->tx2_polarity);
		return -EINVAL;
	}

	switch (pdata->hw_torch_mode) {
	case LM3560_HW_TORCH_MODE_DISABLE:
		data->reg_cfg1 |= LM3560_CFG1_HW_TORCH_MODE_DISABLE;
		break;
	case LM3560_HW_TORCH_MODE_ENABLE:
		data->reg_cfg1 |= LM3560_CFG1_HW_TORCH_MODE_ENABLE;
		break;
	default:
		dev_err(&data->client->dev,
			"%s(): hw_torch_mode(%d) is invald\n",
			__func__, pdata->hw_torch_mode);
		return -EINVAL;
	}

	result = lm3560_set_reg_data(data,
				LM3560_REG_CFG_1,
				LM3560_CFG1_STROBE_POLARITY_MASK
				| LM3560_CFG1_NTC_MASK
				| LM3560_CFG1_TX1_POLARITY_MASK
				| LM3560_CFG1_TX2_POLARITY_MASK
				| LM3560_CFG1_HW_TORCH_MODE_MASK,
				data->reg_cfg1);

	if (result)
		return result;

	result = lm3560_set_flash_sync(data, pdata->flash_sync);

	return result;
}

static int lm3560_chip_init(struct lm3560_drv_data *data,
				struct lm3560_platform_data *pdata)
{
	int result;

	result = lm3560_init_enable_register(data, pdata);
	if (result)
		return result;

	if (1 <= data->privacy_led_nums) {
		result = lm3560_init_privacy_register(data, pdata);
		if (result)
			return result;
	}

	result =  lm3560_set_reg_data(data,
				LM3560_REG_FLASH_DURATION,
				LM3560_FLASH_DURATION_CL_MASK,
				data->reg_flash_duration_limit);
	if (result)
		return result;

	result = lm3560_init_cfg1_register(data, pdata);
	if (result)
		return result;

	return result;
}

/*
 ******************************************************************************
 * - Sysfs operations
 ******************************************************************************
 */

static ssize_t attr_torch_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	u8 value;
	int result;

	result = lm3560_get_reg_data(data, LM3560_REG_ENABLE, &value);

	if (result)
		return result;

	/* Get torch enable */
	value &= LM3560_ENABLE_EN_MASK;
	value = (value == LM3560_ENABLE_EN_TORCH_MODE) ? 1 : 0;

	return snprintf(buf, PAGE_SIZE, "%u\n", value);
}

static ssize_t attr_torch_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	unsigned long enable;
	int result;

	result = strict_strtoul(buf, 10, &enable);
	if (result) {
		dev_err(&data->client->dev,
			"%s(): strtoul failed, result=%d\n",
				__func__, result);
		return -EINVAL;
	}

	if (1 < enable) {
		dev_err(&data->client->dev,
			"%s(): 1 < enable, enable=%lu\n",
				__func__, enable);
		return -EINVAL;
	}

	result = lm3560_torch_mode(data, (unsigned)enable);

	if (result)
		return result;

	return size;
}

static ssize_t attr_torch_current_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	int result;
	unsigned long torch_current;

	result = lm3560_get_torch_current(data, &torch_current);

	if (result != 0)
		return result;

	return snprintf(buf, PAGE_SIZE, "%lu\n", torch_current);
}

static ssize_t attr_torch_current_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	unsigned long torch_current;
	int result;

	result = strict_strtoul(buf, 10, &torch_current);
	if (result) {
		dev_err(&data->client->dev,
			"%s(): strtoul failed, result=%d\n",
				__func__, result);
		return -EINVAL;
	}

	result = lm3560_set_torch_current(data, torch_current);

	if (result)
		return result;

	return size;
}

static ssize_t attr_flash_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	u8 value;
	int result;

	result = lm3560_get_reg_data(data, LM3560_REG_ENABLE, &value);

	if (result)
		return result;

	/* Get flash enable */
	value &= LM3560_ENABLE_EN_MASK;
	value = (value == LM3560_ENABLE_EN_FLASH_MODE) ? 1 : 0;

	return snprintf(buf, PAGE_SIZE, "%u\n", value);
}

static ssize_t attr_flash_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	unsigned long enable;
	int result;

	result = strict_strtoul(buf, 10, &enable);
	if (result) {
		dev_err(&data->client->dev,
			"%s(): strtoul failed, result=%d\n",
				__func__, result);
		return -EINVAL;
	}

	if (1 < enable) {
		dev_err(&data->client->dev,
			"%s(): 1 < enable, enable=%lu\n",
				__func__, enable);
		return -EINVAL;
	}

	result = lm3560_flash_mode(data, (unsigned)enable);

	if (result)
		return result;

	return size;
}

static ssize_t attr_flash_current_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);

	int result;
	unsigned long flash_current;

	result = lm3560_get_flash_current(data, &flash_current);

	if (result != 0)
		return result;

	return snprintf(buf, PAGE_SIZE, "%lu\n", flash_current);
}

static ssize_t attr_flash_current_store(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t size)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	unsigned long flash_current;
	int result = 0;

	result = strict_strtoul(buf, 10, &flash_current);
	if (result) {
		dev_err(&data->client->dev,
			"%s(): strtoul failed, result=%d\n",
				__func__, result);
		return -EINVAL;
	}

	result = lm3560_set_flash_current(data, flash_current);

	if (result)
		return result;

	return size;
}

static ssize_t attr_flash_duration_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	int result;
	unsigned long flash_duration;

	result = lm3560_get_flash_duration(data, &flash_duration);
	if (result != 0)
		return result;

	return snprintf(buf, PAGE_SIZE, "%lu\n", flash_duration);
}

static ssize_t attr_flash_duration_store(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t size)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	unsigned long flash_duration;
	int result;

	result = strict_strtoul(buf, 10, &flash_duration);
	if (result) {
		dev_err(&data->client->dev,
			"%s(): strtoul failed, result=%d\n",
				__func__, result);
		return -EINVAL;
	}

	result = lm3560_set_flash_duration(data, flash_duration);

	if (result)
		return result;

	return size;
}

static ssize_t attr_flash_sync_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	int result;
	u8 reg_cfg1;

	result = lm3560_get_reg_data(data, LM3560_REG_CFG_1, &reg_cfg1);
	if (result)
		return result;

	reg_cfg1 &= LM3560_CFG1_STROBE_INPUT_MASK;

	return snprintf(buf, PAGE_SIZE, "%hu\n", reg_cfg1 ? 1 : 0);
}

static ssize_t attr_flash_sync_enable_store(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t size)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	unsigned long enable;
	int result;

	result = strict_strtoul(buf, 10, &enable);
	if (result) {
		dev_err(&data->client->dev,
			"%s(): strtoul failed, result=%d\n",
				__func__, result);
		return -EINVAL;
	}

	if (1 < enable) {
		dev_err(&data->client->dev,
			"%s(): 1 < enable, enable=%lu\n",
				__func__, enable);
		return -EINVAL;
	}

	result = lm3560_set_flash_sync(data,
			       enable ? LM3560_SYNC_ON : LM3560_SYNC_OFF);

	if (result)
		return result;

	return size;
}


static ssize_t attr_privacy_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);

	int result;
	u8 reg_en = 0;

	if (1 <= data->privacy_led_nums) {
		result = lm3560_get_reg_data(data,
					     LM3560_REG_ENABLE, &reg_en);
		if (result)
			return result;
	}

	return snprintf(buf, PAGE_SIZE, "%hd\n",
			reg_en & LM3560_ENABLE_EN_PRIVACY_MODE);

}
static ssize_t attr_privacy_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	unsigned long enable;
	int result;

	if (1 <= data->privacy_led_nums) {
		result = strict_strtoul(buf, 10, &enable);
		if (result)
			return -EINVAL;
		if (1 < enable)
			return -EINVAL;
		result = lm3560_privacy_mode(data, enable);
		if (result)
			return result;
	} else
		return -EINVAL;

	return size;
}

static ssize_t attr_privacy_current_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	int result;
	unsigned long privacy_current = 0;

	if (1 <= data->privacy_led_nums) {
		result = lm3560_get_privacy_current(data, &privacy_current);
		if (result != 0)
			return result;
	}

	return snprintf(buf, PAGE_SIZE, "%lu\n", privacy_current);
}

static ssize_t attr_privacy_current_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	unsigned long privacy_current;
	int result;

	if (1 <= data->privacy_led_nums) {
		result = strict_strtoul(buf, 10, &privacy_current);
		if (result)
			return -EINVAL;

		result = lm3560_set_privacy_current(data, privacy_current);

		if (result)
			return result;

	} else
		return -EINVAL;

	return size;
}

static ssize_t attr_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	int result;
	u8 status;

	result = lm3560_check_status(data, &status);

	if (result)
		return result;

	return snprintf(buf, PAGE_SIZE, "%d\n", status);

}
static struct device_attribute attributes[] = {
	__ATTR(torch_enable, 0660,
		attr_torch_enable_show, attr_torch_enable_store),
	__ATTR(torch_current, 0660,
		attr_torch_current_show, attr_torch_current_store),
	__ATTR(flash_enable, 0660,
		attr_flash_enable_show, attr_flash_enable_store),
	__ATTR(flash_current, 0660,
		attr_flash_current_show, attr_flash_current_store),
	__ATTR(flash_duration, 0660,
		attr_flash_duration_show, attr_flash_duration_store),
	__ATTR(flash_synchronization, 0660,
		attr_flash_sync_enable_show, attr_flash_sync_enable_store),
	__ATTR(privacy_enable, 0660,
		attr_privacy_enable_show, attr_privacy_enable_store),
	__ATTR(privacy_current, 0660,
		attr_privacy_current_show, attr_privacy_current_store),
	__ATTR(status, 0440, attr_status_show, NULL),
};

static int lm3560_create_sysfs_interfaces(struct device *dev)
{
	int i;
	int result;

	for (i = 0; i < ARRAY_SIZE(attributes); i++) {
		result = device_create_file(dev, &attributes[i]);
		if (result) {
			for (; i >= 0; i--)
				device_remove_file(dev, &attributes[i]);
			dev_err(dev, "%s(): Failed to create sysfs I/F\n",
					__func__);
			return result;
		}
	}

	return result;
}

static void remove_sysfs_interfaces(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(dev, &attributes[i]);
}

/*
 ******************************************************************************
 * - Device operation such as;
 *   probe, init/exit, remove
 ******************************************************************************
 */
static int __devinit lm3560_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct lm3560_platform_data *pdata = client->dev.platform_data;
	struct lm3560_drv_data *data;
	int result;

	dev_dbg(&client->dev, "%s\n", __func__);

	if (!pdata) {
		dev_err(&client->dev,
			"%s: platform data is not complete.\n",
				__func__);
		return -EINVAL;
	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_I2C_BLOCK)) {
		dev_err(&client->dev,
			"%s: failed during i2c_check_functionality",
				__func__);
		return -EIO;
	}

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_err(&client->dev,
			"%s: failed during kzalloc",
				__func__);
		return -ENOMEM;
	}

	dev_set_drvdata(&client->dev, data);

	data->client = client;

	result = pdata->hw_enable();
	if (result) {
		dev_err(&client->dev,
			"%s: Failed to HW Enable.\n", __func__);
		goto err_setup;
	}

	if ((pdata->led_nums < 1) || (pdata->led_nums > 2)) {
		pdata->led_nums = 2;
		dev_err(&client->dev,
			"%s: Incorrect no of LED outputs. Now set to: %d\n"
			, __func__, pdata->led_nums);
	}
	data->led_nums = pdata->led_nums;
	data->privacy_led_nums = pdata->privacy_led_nums;
	data->torch_current_shift = 3;
	data->flash_current_shift = 4;

	if (pdata->current_limit >= 3500000) {
		data->reg_flash_duration_limit =
			LM3560_FLASH_DURATION_CL_3500MA;
	} else if (pdata->current_limit >= 2800000) {
		data->reg_flash_duration_limit =
			LM3560_FLASH_DURATION_CL_2800MA;
	} else if (pdata->current_limit >= 2100000) {
		data->reg_flash_duration_limit =
			LM3560_FLASH_DURATION_CL_2100MA;
	} else if (pdata->current_limit >= 1400000) {
		data->reg_flash_duration_limit =
			LM3560_FLASH_DURATION_CL_1400MA;
	} else {
		/* current_limit > 3500000uA || current_limit < 1400000uA */
		dev_err(&client->dev,
			"%s: current_limit(%luuA) is invalid\n",
			__func__, pdata->current_limit);
		result = -EINVAL;
		goto err_chip_init;
	}

	result = lm3560_chip_init(data, pdata);
	if (result) {
		dev_err(&client->dev,
			"%s: chip init failed",
				__func__);
		goto err_chip_init;
	}

	result = lm3560_create_sysfs_interfaces(&client->dev);
	if (result) {
		dev_err(&client->dev,
			"%s: create sysfs failed",
				__func__);
		goto err_chip_init;
	}

	dev_info(&client->dev, "%s: loaded\n", __func__);

	return 0;

err_chip_init:
	pdata->hw_disable();
err_setup:
	dev_set_drvdata(&client->dev, NULL);
	kfree(data);
	dev_err(&client->dev,
		"%s: failed with code %d.\n", __func__, result);
	return result;
}

static int __devexit lm3560_remove(struct i2c_client *client)
{
	struct lm3560_drv_data *data = dev_get_drvdata(&client->dev);
	struct lm3560_platform_data *pdata = client->dev.platform_data;

	remove_sysfs_interfaces(&client->dev);
	pdata->hw_disable();
	kfree(data);

	return 0;
}

#ifdef CONFIG_SUSPEND
static int lm3560_suspend(struct device *dev)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	struct lm3560_platform_data *pdata = data->client->dev.platform_data;
	int result;

	result = lm3560_set_reg_data(data, LM3560_REG_ENABLE,
				     LM3560_ENABLE_EN_MASK , 0);
	if (result) {
		dev_err(dev, "%s:set_reg_data error\n", __func__);
		goto exit_suspend;
	}

	result = pdata->hw_disable();
	if (result) {
		dev_err(&data->client->dev,
		"%s: Failed to HW Disable.\n", __func__);
		goto exit_suspend;
	}

	dev_info(dev,
		 "%s: Suspending lm3560 driver.\n", __func__);

exit_suspend:
	return result;
}

static int lm3560_resume(struct device *dev)
{
	struct lm3560_drv_data *data = dev_get_drvdata(dev);
	struct lm3560_platform_data *pdata = data->client->dev.platform_data;
	int result;

	result = pdata->hw_enable();
	if (result) {
		dev_err(dev, "%s: Failed to HW Enable.\n", __func__);
		goto exit_resume;
	}

	dev_info(dev, "%s: Reinit lm3560 chip.\n", __func__);
	result = lm3560_chip_init(data, pdata);
	if (result) {
		dev_err(dev, "%s:chip init error\n", __func__);
		goto exit_resume;
	}

	dev_info(dev, "%s: Resuming lm3560 driver.\n", __func__);

exit_resume:
	return result;
}
#else
#define lm3560_suspend NULL
#define lm3560_resume NULL
#endif

static const struct dev_pm_ops lm3560_pm = {
	.suspend = lm3560_suspend,
	.resume = lm3560_resume,
};

static const struct i2c_device_id lm3560_id[] = {
	{ LM3560_DRV_NAME, 0 },
};
MODULE_DEVICE_TABLE(i2c, lm3560_id);

static struct i2c_driver lm3560_driver = {
	.driver = {
		.name	= LM3560_DRV_NAME,
		.owner	= THIS_MODULE,
		.pm = &lm3560_pm,
	},
	.probe	= lm3560_probe,
	.remove	= __devexit_p(lm3560_remove),
	.id_table = lm3560_id,
};

static int __init lm3560_init(void)
{
	return i2c_add_driver(&lm3560_driver);
}

static void __exit lm3560_exit(void)
{
	i2c_del_driver(&lm3560_driver);
}

module_init(lm3560_init);
module_exit(lm3560_exit);

MODULE_AUTHOR("Kazunari YOSHINO <Kazunari.X.Yoshino@sonyericsson.com>");
MODULE_DESCRIPTION("LM3560 I2C LED driver");
MODULE_LICENSE("GPL");
