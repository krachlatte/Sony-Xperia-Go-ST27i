/*
 * Copyright (C) ST-Ericsson SA 2011
 *
 * Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <asm/mach-types.h>
#include <mach/irqs-board-mop500.h>
#include <mach/irqs.h>
#include <linux/clk.h>
#include <mach/cw1200_plat.h>

static void cw1200_release(struct device *dev);
static int cw1200_power_ctrl(const struct cw1200_platform_data *pdata,
			     bool enable);
static int cw1200_clk_ctrl(const struct cw1200_platform_data *pdata,
		bool enable);

static struct resource cw1200_href_resources[] = {
	{
		.start = 215,
		.end = 215,
		.flags = IORESOURCE_IO,
		.name = "cw1200_reset",
	},
#ifdef CONFIG_CW1200_USE_GPIO_IRQ
	{
		.start = NOMADIK_GPIO_TO_IRQ(216),
		.end = NOMADIK_GPIO_TO_IRQ(216),
		.flags = IORESOURCE_IRQ,
		.name = "cw1200_irq",
	},
#endif /* CONFIG_CW1200_USE_GPIO_IRQ */
};

static struct resource cw1200_href60_resources[] = {
	{
		.start = 85,
		.end = 85,
		.flags = IORESOURCE_IO,
		.name = "cw1200_reset",
	},
#ifdef CONFIG_CW1200_USE_GPIO_IRQ
	{
		.start = NOMADIK_GPIO_TO_IRQ(4),
		.end = NOMADIK_GPIO_TO_IRQ(4),
		.flags = IORESOURCE_IRQ,
		.name = "cw1200_irq",
	},
#endif /* CONFIG_CW1200_USE_GPIO_IRQ */
};

static struct resource cw1200_pdp_resources[] = {
	{
		.start = 20,
		.end = 20,
		.flags = IORESOURCE_IO,
		.name = "cw1200_reset",
	},
#ifdef CONFIG_CW1200_USE_GPIO_IRQ
	{
		.start = GPIO_TO_IRQ(169),
		.end = GPIO_TO_IRQ(169),
		.flags = IORESOURCE_IRQ,
		.name = "cw1200_irq",
	},
#endif /* CONFIG_CW1200_USE_GPIO_IRQ */
};

static struct cw1200_platform_data cw1200_platform_data = {
	.clk_ctrl = cw1200_clk_ctrl,
};

static struct clk *clk_dev;

static struct platform_device cw1200_device = {
	.name = "cw1200_wlan",
	.dev = {
		.platform_data = &cw1200_platform_data,
		.release = cw1200_release,
		.init_name = "cw1200_wlan",
	},
};

const struct cw1200_platform_data *cw1200_get_platform_data(void)
{
	return &cw1200_platform_data;
}
EXPORT_SYMBOL_GPL(cw1200_get_platform_data);

static int cw1200_power_ctrl(const struct cw1200_platform_data *pdata,
			     bool enable)
{
	static const char *vdd_name = "vdd";
	struct regulator *vdd;
	int ret = 0;

	vdd = regulator_get(&cw1200_device.dev, vdd_name);
	if (IS_ERR(vdd)) {
		ret = PTR_ERR(vdd);
		dev_warn(&cw1200_device.dev,
				"%s: Failed to get regulator '%s': %d\n",
				__func__, vdd_name, ret);
	} else {
		if (enable)
			ret = regulator_enable(vdd);
		else
			ret = regulator_disable(vdd);

		if (ret) {
			dev_warn(&cw1200_device.dev,
				"%s: Failed to %s regulator '%s': %d\n",
				__func__, enable ? "enable" : "disable",
				vdd_name, ret);
		}
		regulator_put(vdd);
	}
	return ret;
}

static int cw1200_clk_ctrl(const struct cw1200_platform_data *pdata,
		bool enable)
{
	static const char *clock_name = "sys_clk_out";
	int ret = 0;

	if (enable) {
		clk_dev = clk_get(&cw1200_device.dev, clock_name);
		if (IS_ERR(clk_dev)) {
			ret = PTR_ERR(clk_dev);
			dev_warn(&cw1200_device.dev,
				"%s: Failed to get clk '%s': %d\n",
				__func__, clock_name, ret);
		} else {
			ret = clk_enable(clk_dev);
			if (ret) {
				clk_put(clk_dev);
				dev_warn(&cw1200_device.dev,
					"%s: Failed to enable clk '%s': %d\n",
					__func__, clock_name, ret);
			}
		}
	} else {
		clk_disable(clk_dev);
		clk_put(clk_dev);
	}

	return ret;
}

int __init mop500_wlan_init(void)
{
	if (machine_is_u8500() ||
			machine_is_u5500() ||
			machine_is_nomadik()) {
		cw1200_device.num_resources =
				ARRAY_SIZE(cw1200_href_resources);
		cw1200_device.resource = cw1200_href_resources;
	} else if (machine_is_hrefv60()) {
		cw1200_device.num_resources =
				ARRAY_SIZE(cw1200_href60_resources);
		cw1200_device.resource = cw1200_href60_resources;
	} else {
		dev_err(&cw1200_device.dev,
				"Unsupported mach type %d "
				"(check mach-types.h)\n",
				__machine_arch_type);
		return -ENOTSUPP;
	}

	cw1200_platform_data.mmc_id = "mmc2";

	cw1200_platform_data.reset = &cw1200_device.resource[0];
#ifdef CONFIG_CW1200_USE_GPIO_IRQ
	cw1200_platform_data.irq = &cw1200_device.resource[1];
#endif /* #ifdef CONFIG_CW1200_USE_GPIO_IRQ */

	cw1200_device.dev.release = cw1200_release;


	return WARN_ON(platform_device_register(&cw1200_device));
}


int __init pdp_wlan_init(void)
{
	cw1200_device.num_resources = ARRAY_SIZE(cw1200_pdp_resources);
	cw1200_device.resource = cw1200_pdp_resources;

	cw1200_platform_data.mmc_id = "mmc2";

	cw1200_platform_data.reset = &cw1200_device.resource[0];
#ifdef CONFIG_CW1200_USE_GPIO_IRQ
	cw1200_platform_data.irq = &cw1200_device.resource[1];
#endif /* #ifdef CONFIG_CW1200_USE_GPIO_IRQ */

	cw1200_device.dev.release = cw1200_release;


	return WARN_ON(platform_device_register(&cw1200_device));
}

static void cw1200_release(struct device *dev)
{
	/* Do nothing for now */
}

