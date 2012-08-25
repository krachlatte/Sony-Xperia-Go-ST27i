/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Par-Gunnar Hjalmdahl <par-gunnar.p.hjalmdahl@stericsson.com>
 * License terms: GNU General Public License (GPL), version 2.
 */

#ifndef __DEVICES_CG2900_H
#define __DEVICES_CG2900_H

#include <linux/mfd/cg2900.h>
#include <linux/clk.h>

#define CHIP_ENABLE_PDB_LOW_TIMEOUT	100 /* ms */
#define CHIP_ENABLE_PMU_EN_TIMEOUT	300 /* us */

struct dcg2900_info {
	int	gbf_gpio;
	int	pmuen_gpio;
	int	bt_gpio;
	bool	sleep_gpio_set;
	u8	gpio_0_7_pull_up;
	u8	gpio_8_15_pull_up;
	u8	gpio_16_20_pull_up;
	u8	gpio_0_7_pull_down;
	u8	gpio_8_15_pull_down;
	u8	gpio_16_20_pull_down;
	struct clk	*lpoclk;
};

extern void dcg2900_enable_chip(struct cg2900_chip_dev *dev);
extern void dcg2900_disable_chip(struct cg2900_chip_dev *dev);
extern int dcg2900_setup(struct cg2900_chip_dev *dev,
				struct dcg2900_info *info);

/**
 * dcg2900_init_platdata() - Initializes platform data with callback functions.
 * @data:	Platform data.
 */
extern void dcg2900_init_platdata(struct cg2900_platform_data *data);

#endif /* __DEVICES_CG2900_H */
