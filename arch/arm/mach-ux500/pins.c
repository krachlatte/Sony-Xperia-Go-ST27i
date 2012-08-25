/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Rabin Vincent <rabin.vincent@stericsson.com> for ST-Ericsson
 * License terms: GNU General Public License (GPL), version 2
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <plat/pincfg.h>
#include <linux/gpio.h>

#include "pins.h"

static LIST_HEAD(pin_lookups);
static DEFINE_MUTEX(pin_lookups_mutex);
static DEFINE_SPINLOCK(pins_lock);

void __init ux500_pins_add(struct ux500_pin_lookup *pl, size_t num)
{
	mutex_lock(&pin_lookups_mutex);

	while (num--) {
		list_add_tail(&pl->node, &pin_lookups);
		pl++;
	}

	mutex_unlock(&pin_lookups_mutex);
}

struct ux500_pins *ux500_pins_get(const char *name)
{
	struct ux500_pins *pins = NULL;
	struct ux500_pin_lookup *pl;

	mutex_lock(&pin_lookups_mutex);

	list_for_each_entry(pl, &pin_lookups, node) {
		if (!strcmp(pl->name, name)) {
			pins = pl->pins;
			goto out;
		}
	}

out:
	mutex_unlock(&pin_lookups_mutex);
	return pins;
}

int ux500_pins_enable(struct ux500_pins *pins)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&pins_lock, flags);

	if (pins->usage++ == 0)
		ret = nmk_config_pins(pins->cfg, pins->num);

	spin_unlock_irqrestore(&pins_lock, flags);
	return ret;
}

int ux500_pins_disable(struct ux500_pins *pins)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&pins_lock, flags);

	if (WARN_ON(pins->usage == 0))
		goto out;

	if (--pins->usage == 0)
		ret = nmk_config_pins_sleep(pins->cfg, pins->num);

out:
	spin_unlock_irqrestore(&pins_lock, flags);
	return ret;
}

void ux500_pins_put(struct ux500_pins *pins)
{
	WARN_ON(!pins);
}

void __init ux500_offchip_gpio_init(struct ux500_pins *pins)
{
	int err;
	int i;
	int gpio;
	int output;
	int value;
	pin_cfg_t cfg;

	for (i=0; i < pins->num; i++) {
		cfg = pins->cfg[i];
		gpio = PIN_NUM(cfg);
		output = PIN_DIR(cfg);
		value = PIN_VAL(cfg);

		err = gpio_request(gpio, "offchip_gpio_init");
		if (err < 0) {
			pr_err("pins: gpio_request for gpio=%d failed with"
				"err: %d\n", gpio, err);
			/* Pin already requested. Try to configure rest. */
			continue;
		}

		if (!output) {
			err = gpio_direction_input(gpio);
			if (err < 0)
				pr_err("pins: gpio_direction_input for gpio=%d"
					"failed with err: %d\n", gpio, err);
		} else {
			err = gpio_direction_output(gpio, value);
			if (err < 0)
				pr_err("pins: gpio_direction_output for gpio="
					"%d failed with err: %d\n", gpio, err);
		}
		gpio_free(gpio);
	}
}
