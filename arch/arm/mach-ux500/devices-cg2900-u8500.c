/*
 * arch/arm/mach-ux500/devices-cg2900-u8500.c
 *
 * Copyright (C) ST-Ericsson SA 2010
 * Authors:
 * Par-Gunnar Hjalmdahl (par-gunnar.p.hjalmdahl@stericsson.com) for ST-Ericsson.
 * Henrik Possung (henrik.possung@stericsson.com) for ST-Ericsson.
 * Josef Kindberg (josef.kindberg@stericsson.com) for ST-Ericsson.
 * Dariusz Szymszak (dariusz.xd.szymczak@stericsson.com) for ST-Ericsson.
 * Kjell Andersson (kjell.k.andersson@stericsson.com) for ST-Ericsson.
 * License terms:  GNU General Public License (GPL), version 2
 *
 * Board specific device support for the Linux Bluetooth HCI H:4 Driver
 * for ST-Ericsson connectivity controller.
 */

#include <asm/byteorder.h>
#include <asm-generic/errno-base.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/mfd/cg2900.h>
#include <linux/mfd/abx500/ab5500.h>
#include <plat/pincfg.h>

#include "devices-cg2900.h"

void dcg2900_enable_chip(struct cg2900_chip_dev *dev)
{
	struct dcg2900_info *info = dev->b_data;

	if (info->gbf_gpio == -1)
		return;

	/*
	 * - SET PMU_EN to high
	 * - Wait for 300 usecs
	 * - Set PDB to high.
	 */
	if (info->pmuen_gpio != -1) {
		/*
		 * We must first set PMU_EN pin high and then wait 300 us before
		 * setting the GBF_EN high.
		 */
		gpio_set_value(info->pmuen_gpio, 1);
		udelay(CHIP_ENABLE_PMU_EN_TIMEOUT);
	}

	gpio_set_value(info->gbf_gpio, 1);
}

void dcg2900_disable_chip(struct cg2900_chip_dev *dev)
{
	struct dcg2900_info *info = dev->b_data;

	if (info->gbf_gpio != -1)
		gpio_set_value(info->gbf_gpio, 0);
	if (info->pmuen_gpio != -1)
		gpio_set_value(info->pmuen_gpio, 0);
}

int dcg2900_setup(struct cg2900_chip_dev *dev,
					struct dcg2900_info *info)
{
	int err = 0;
	struct resource *resource;
	const char *gbf_name;
	const char *bt_name = NULL;
	const char *pmuen_name = NULL;

	resource = platform_get_resource_byname(dev->pdev, IORESOURCE_IO,
						"gbf_ena_reset");
	if (!resource) {
		dev_err(dev->dev, "GBF GPIO does not exist\n");
		err = -EINVAL;
		goto err_handling;
	}

	info->gbf_gpio = resource->start;
	gbf_name = resource->name;

	resource = platform_get_resource_byname(dev->pdev, IORESOURCE_IO,
						"bt_enable");
	/* BT Enable GPIO may not exist */
	if (resource) {
		info->bt_gpio = resource->start;
		bt_name = resource->name;
	}

	resource = platform_get_resource_byname(dev->pdev, IORESOURCE_IO,
						"pmu_en");
	/* PMU_EN GPIO may not exist */
	if (resource) {
		info->pmuen_gpio = resource->start;
		pmuen_name = resource->name;
	}

	/* Now setup the GPIOs */
	err = gpio_request(info->gbf_gpio, gbf_name);
	if (err < 0) {
		dev_err(dev->dev, "gpio_request %s failed with err: %d\n",
			gbf_name, err);
		goto err_handling;
	}

	err = gpio_direction_output(info->gbf_gpio, 0);
	if (err < 0) {
		dev_err(dev->dev,
			"gpio_direction_output %s failed with err: %d\n",
			gbf_name, err);
		goto err_handling_free_gpio_gbf;
	}

	if (!pmuen_name)
		goto set_bt_gpio;

	err = gpio_request(info->pmuen_gpio, pmuen_name);
	if (err < 0) {
		dev_err(dev->dev, "gpio_request %s failed with err: %d\n",
			pmuen_name, err);
		goto err_handling_free_gpio_gbf;
	}

	err = gpio_direction_output(info->pmuen_gpio, 0);
	if (err < 0) {
		dev_err(dev->dev,
			"gpio_direction_output %s failed with err: %d\n",
			pmuen_name, err);
		goto err_handling_free_gpio_pmuen;
	}

set_bt_gpio:
	if (!bt_name)
		goto finished;

	err = gpio_request(info->bt_gpio, bt_name);
	if (err < 0) {
		dev_err(dev->dev, "gpio_request %s failed with err: %d\n",
			bt_name, err);
		goto err_handling_free_gpio_pmuen;
	}

	err = gpio_direction_output(info->bt_gpio, 1);
	if (err < 0) {
		dev_err(dev->dev,
			"gpio_direction_output %s failed with err: %d\n",
			bt_name, err);
		goto err_handling_free_gpio_bt;
	}

finished:

	return 0;

err_handling_free_gpio_bt:
	gpio_free(info->bt_gpio);
	info->bt_gpio = -1;
err_handling_free_gpio_pmuen:
	if (info->pmuen_gpio != -1) {
		gpio_free(info->pmuen_gpio);
		info->pmuen_gpio = -1;
	}
err_handling_free_gpio_gbf:
	gpio_free(info->gbf_gpio);
	info->gbf_gpio = -1;
err_handling:

	return err;
}
