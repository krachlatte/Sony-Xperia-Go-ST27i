/*
 * Copyright ST-Ericsson 2010.
 *
 * Author: Bibek Basu <@stericsson.com>
 * Licensed under GPLv2.
 */

#ifndef _AB8500_GPIO_H
#define _AB8500_GPIO_H

/*
 * Platform data to register a block: only the initial gpio/irq number.
 */

struct ab8500_gpio_platform_data {
	int gpio_base;
	u32 irq_base;
	u8  initial_pin_config[7];
	u8  initial_pin_direction[6];
	u8  initial_pin_pullups[6];
};

int ab8500_config_pull_up_or_down(struct device *dev,
				unsigned gpio, bool enable);

int ab8500_gpio_config_select(struct device *dev,
			      unsigned gpio, bool gpio_select);

int ab8500_gpio_config_get_select(struct device *dev,
				  unsigned gpio, bool *gpio_select);

#endif /* _AB8500_GPIO_H */
