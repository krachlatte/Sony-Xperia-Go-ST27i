/*
 * Copyright (C) ST-Ericsson AB 2010
 * Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
 *
 * Author: Marcus Lorentzon <marcus.xm.lorentzon@stericsson.com>
 * Author: Johan Olson <johan.olson@sonyericsson.com>
 * Author: Joakim Wesslen <joakim.wesslen@sonyericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/mfd/ab8500/denc.h>
#include <linux/workqueue.h>
#include <linux/dispdev.h>
#include <linux/clk.h>
#include <mach/devices.h>
#include <video/av8100.h>
#include <video/mcde_display.h>
#include <video/mcde_display-panel_dsi.h>
#include <video/mcde_display-av8100.h>
#include <video/mcde_display-ab8500.h>
#include <video/mcde_fb.h>
#include <video/mcde_dss.h>
#include <mach/prcmu.h>
#include <mach/display_panel.h>
#include <plat/pincfg.h>
#include "pins-db8500.h"
#include "pins.h"

#define DSI_UNIT_INTERVAL_0	0x9
#define DSI_UNIT_INTERVAL_1	0x9
#define DSI_UNIT_INTERVAL_2	0x5

#define DSI_PLL_FREQ_HZ		840320000
/* Based on PLL DDR Freq at 798,72 MHz */
#define HDMI_FREQ_HZ		33280000
#define TV_FREQ_HZ		38400000

#define DSI_HS_FREQ_HZ 420160000
#define DSI_LP_FREQ_HZ 38400000

#ifdef CONFIG_FB_MCDE

#define ROTATE_MAIN		0

#define DDR_APE_OPP_TIMER	200
#define DDR_QOS_VALUE		50

/* The initialization of hdmi disp driver must be delayed in order to
 * ensure that inputclk will be available (needed by hdmi hw) */
#ifdef CONFIG_DISPLAY_AV8100_TERTIARY
static struct delayed_work work_dispreg_hdmi;
#define DISPREG_HDMI_DELAY 6000
#endif

enum {
#ifdef CONFIG_DISPLAY_PANEL_DSI_PRIMARY
	PRIMARY_DISPLAY_ID,
#endif
#ifdef CONFIG_DISPLAY_PANEL_DSI_SECONDARY
	SECONDARY_DISPLAY_ID,
#endif
#ifdef CONFIG_DISPLAY_AV8100_TERTIARY
	AV8100_DISPLAY_ID,
#endif
#ifdef CONFIG_DISPLAY_AB8500_TERTIARY
	AB8500_DISPLAY_ID,
#endif
	MCDE_NR_OF_DISPLAYS
};
static int display_initialized_during_boot;

static int __init startup_graphics_setup(char *str)
{

	if (get_option(&str, &display_initialized_during_boot) != 1)
		display_initialized_during_boot = 0;

	switch (display_initialized_during_boot) {
	case 1:
		pr_info("Startup graphics support\n");
		break;
	case 0:
	default:
		pr_info("No startup graphics supported\n");
		break;
	};

	return 1;
}
__setup("startup_graphics=", startup_graphics_setup);

#if defined(CONFIG_DISPLAY_AB8500_TERTIARY) ||\
					defined(CONFIG_DISPLAY_AV8100_TERTIARY)
static struct mcde_col_transform rgb_2_yCbCr_transform = {
	.matrix = {
		{0x0042, 0x0081, 0x0019},
		{0xffda, 0xffb6, 0x0070},
		{0x0070, 0xffa2, 0xffee},
	},
	.offset = {0x10, 0x80, 0x80},
};
#endif

#if defined(CONFIG_DISPLAY_PANEL_DSI_PRIMARY) || \
    defined(CONFIG_DISPLAY_PANEL_DSI_SECONDARY)

static int setup_performance(struct mcde_display_device *ddev)
{
	struct panel_device *dev =
		container_of(ddev, struct panel_device, base);
	struct panel_platform_data *pdata = dev->base.dev.platform_data;
	struct panel_record *rd;
	s32 ddr_qos_value;

	/* Remove DDR request if no update after DDR_APE_OPP_TIMER ms */
	cancel_delayed_work(&dev->ddr_ape_timeout_work);
	schedule_delayed_work(&dev->ddr_ape_timeout_work,
		msecs_to_jiffies(DDR_APE_OPP_TIMER));

	rd = dev_get_drvdata(&dev->base.dev);
	if (rd && rd->panel && rd->panel->ddr_qos_value)
		ddr_qos_value = rd->panel->ddr_qos_value;
	else
		ddr_qos_value = DDR_QOS_VALUE;

	if (!pdata->ddr_is_requested) {
		/* Continue even if we fail the request */
		if (prcmu_qos_add_requirement(PRCMU_QOS_DDR_OPP, "main_LCD",
								ddr_qos_value))
			dev_err(&ddev->dev, "DDR OPP %d failed\n",
								ddr_qos_value);
		else
			pdata->ddr_is_requested = true;
	}

	if (!pdata->ape_is_requested && rd && rd->panel &&
						rd->panel->ape_qos_value) {
		/* Continue even if we fail the request */
		if (prcmu_qos_add_requirement(PRCMU_QOS_APE_OPP, "main_LCD_ape",
						rd->panel->ape_qos_value))
			dev_err(&ddev->dev, "APE OPP %d failed\n",
						rd->panel->ape_qos_value);
		else
			pdata->ape_is_requested = true;
	}
	return 0;
}

static int extern_te_update(struct mcde_display_device *ddev,
							bool tripple_buffer)
{
	int ret = 0;
	bool switched_off_te;

	ret = setup_performance(ddev);

	if (ddev->power_mode != MCDE_DISPLAY_PM_ON && ddev->set_power_mode) {
		ret = ddev->set_power_mode(ddev, MCDE_DISPLAY_PM_INTERMEDIATE);
		if (ret < 0) {
			dev_warn(&ddev->dev,
				"%s:Failed to set power mode to intermediate\n",
				__func__);
		}
	}

	/* Temporary set SYNC_SRC to OFF in order to get first refresh out */
	if (ddev->power_mode != MCDE_DISPLAY_PM_ON) {
		mcde_chnl_update_sync_src(ddev->chnl_state, MCDE_SYNCSRC_OFF);
		switched_off_te = true;
		mcde_chnl_set_dirty(ddev->chnl_state);
	}

	ret = mcde_chnl_update(ddev->chnl_state, &ddev->update_area,
							tripple_buffer);

	/* Set sync_src back to TE0 */
	if (switched_off_te) {
		mcde_chnl_update_sync_src(ddev->chnl_state, MCDE_SYNCSRC_TE0);
		switched_off_te = false;
		mcde_chnl_set_dirty(ddev->chnl_state);
	}

	if (ret < 0) {
		dev_warn(&ddev->dev, "%s:Failed to update channel\n", __func__);
		return ret;
	}
	if (ddev->first_update && ddev->on_first_update)
		ddev->on_first_update(ddev);

	if (ddev->power_mode != MCDE_DISPLAY_PM_ON && ddev->set_power_mode) {
		ret = ddev->set_power_mode(ddev, MCDE_DISPLAY_PM_ON);
		if (ret < 0) {
			dev_warn(&ddev->dev,
				"%s:Failed to set power mode to on\n",
				__func__);
			return ret;
		}
	}

	dev_vdbg(&ddev->dev, "Overlay updated, chnl=%d\n", ddev->chnl_id);

	return 0;
}

static int panel_update(struct mcde_display_device *ddev, bool tripple_buffer)
{
	int ret = 0;

	ret = setup_performance(ddev);

	/* TODO: Dirty */
	if (ddev->prepare_for_update) {
		/* TODO: Send dirty rectangle */
		ret = ddev->prepare_for_update(ddev, 0, 0,
			ddev->native_x_res, ddev->native_y_res);
		if (ret < 0) {
			dev_warn(&ddev->dev,
				"%s:Failed to prepare for update\n", __func__);
			return ret;
		}
	}

	if (ddev->power_mode != MCDE_DISPLAY_PM_ON && ddev->set_power_mode) {
		ret = ddev->set_power_mode(ddev, MCDE_DISPLAY_PM_INTERMEDIATE);
		if (ret < 0) {
			dev_warn(&ddev->dev,
				"%s:Failed to set power mode to intermediate\n",
				__func__);
		}
	}

	/* TODO: Calculate & set update rect */
	ret = mcde_chnl_update(ddev->chnl_state, &ddev->update_area,
		tripple_buffer);
	if (ret < 0) {
		dev_warn(&ddev->dev, "%s:Failed to update channel\n", __func__);
		return ret;
	}
	if (ddev->first_update && ddev->on_first_update)
		ddev->on_first_update(ddev);

	if (ddev->power_mode != MCDE_DISPLAY_PM_ON && ddev->set_power_mode) {
		ret = ddev->set_power_mode(ddev, MCDE_DISPLAY_PM_ON);
		if (ret < 0) {
			dev_warn(&ddev->dev,
				"%s:Failed to set power mode to on\n",
				__func__);
			return ret;
		}
	}

	dev_vdbg(&ddev->dev, "Overlay updated, chnl=%d\n", ddev->chnl_id);

	return 0;
}

static int panel_platform_reset(struct mcde_display_device *ddev, bool level)
{
	int ret = 0;
	struct panel_platform_data *pdata = ddev->dev.platform_data;

	dev_dbg(&ddev->dev, "%s: Reset display driver, level = %d\n",
							__func__, level);

	if (pdata->reset_gpio) {
		ret = gpio_request(pdata->reset_gpio, NULL);
		if (ret) {
			dev_warn(&ddev->dev,
				"%s:Failed to request gpio %d\n",
				__func__, pdata->reset_gpio);
			goto out;
		}
		gpio_set_value(pdata->reset_gpio, level);
	}

out:
	if (pdata->reset_gpio)
		gpio_free(pdata->reset_gpio);
	return ret;
}

static int panel_platform_enable(struct mcde_display_device *ddev)
{
	int ret = 0;
	struct panel_device *dev =
		container_of(ddev, struct panel_device, base);
	struct panel_platform_data *pdata = dev->base.dev.platform_data;

	dev_dbg(&ddev->dev, "%s: Reset & power on display driver\n",
								__func__);
	if (pdata->regulator_id && !dev->regulator) {
		dev->regulator = regulator_get(NULL, pdata->regulator_id);
		if (IS_ERR(dev->regulator)) {
			ret = PTR_ERR(dev->regulator);
			dev_err(&ddev->dev,
				"%s:Failed to get regulator '%s'\n",
				__func__, pdata->regulator_id);
			dev->regulator = NULL;
			goto out;
		}
		if (pdata->max_supply_voltage != 0) {
			ret = regulator_set_voltage(dev->regulator,
						    pdata->min_supply_voltage,
						    pdata->max_supply_voltage);
			if (ret < 0) {
				dev_err(&ddev->dev,
						"%s: Failed to set voltage\n",
						__func__);
				goto out1;
			}
		}
		ret = regulator_enable(dev->regulator);
		if (ret < 0) {
			dev_err(&ddev->dev, "%s:Failed to enable regulator\n",
								__func__);
			goto out1;
		}
	}

	if (pdata->io_regulator_id && !dev->io_regulator) {
		dev->io_regulator = regulator_get(NULL, pdata->io_regulator_id);
		if (IS_ERR(dev->io_regulator)) {
			ret = PTR_ERR(dev->io_regulator);
			dev_err(&ddev->dev,
				"%s:Failed to get IO regulator '%s'\n",
				__func__, pdata->io_regulator_id);
			dev->io_regulator = NULL;
			goto out1;
		}
		/* Do not set any voltage. This is an IO regulator used by
		 * many chips. It should not be changed */
		ret = regulator_enable(dev->io_regulator);
		if (ret < 0) {
			dev_err(&ddev->dev,
				"%s:Failed to enable IO regulator\n", __func__);
			goto out2;
		}
	}

	if (pdata->skip_init) {
		dev_info(&ddev->dev,
			"%s: Display already initialized during boot\n",
			__func__);
		pdata->skip_init = false;
	}

	if (dev->base.port->sync_src == MCDE_SYNCSRC_TE0)
		ddev->update = extern_te_update;
	else
		ddev->update = panel_update;

	/* TODO: Remove when DSI send command uses interrupts */
	ddev->prepare_for_update = NULL;
	return 0;
out2:
	regulator_put(dev->io_regulator);
	dev->io_regulator = NULL;
out1:
	regulator_put(dev->regulator);
	dev->regulator = NULL;
out:
	return ret;
}

static int panel_platform_disable(struct mcde_display_device *ddev)
{
	struct panel_device *dev =
		container_of(ddev, struct panel_device, base);
	struct panel_platform_data *pdata = dev->base.dev.platform_data;

	dev_dbg(&ddev->dev, "%s: Reset & power off display driver\n",
								__func__);

	/* Remove DDR and APE request */
	cancel_delayed_work(&dev->ddr_ape_timeout_work);
	prcmu_qos_remove_requirement(PRCMU_QOS_DDR_OPP, "main_LCD");
	pdata->ddr_is_requested = false;
	prcmu_qos_remove_requirement(PRCMU_QOS_APE_OPP, "main_LCD_ape");
	pdata->ape_is_requested = false;

	if (dev->regulator) {
		if (regulator_disable(dev->regulator) < 0)
			dev_err(&ddev->dev, "%s:Failed to disable regulator\n",
								__func__);
		regulator_put(dev->regulator);
		dev->regulator = NULL;
	}
	if (dev->io_regulator) {
		if (regulator_disable(dev->io_regulator) < 0)
			dev_err(&ddev->dev,
				"%s:Failed to disable IO regulator\n",
				__func__);
		regulator_put(dev->io_regulator);
		dev->io_regulator = NULL;
	}
	return 0;
}

static void work_ddr_ape_timeout_function(struct work_struct *work)
{
	struct panel_device *dev = container_of(work,
		struct panel_device, ddr_ape_timeout_work.work);
	struct panel_platform_data *pdata = dev->base.dev.platform_data;

	dev_vdbg(&dev->base.dev, "%s: display update timeout\n", __func__);
	prcmu_qos_remove_requirement(PRCMU_QOS_DDR_OPP, "main_LCD");
	pdata->ddr_is_requested = false;
	prcmu_qos_remove_requirement(PRCMU_QOS_APE_OPP, "main_LCD_ape");
	pdata->ape_is_requested = false;
}

#endif /* CONFIG_DISPLAY_PANEL_DSI_PRIMARY) ||
	  CONFIG_DISPLAY_PANEL_DSI_SECONDARY */

#ifdef CONFIG_DISPLAY_PANEL_DSI_PRIMARY
static struct mcde_port panel_port0 = {
	.type = MCDE_PORTTYPE_DSI,
	.mode = MCDE_PORTMODE_CMD,
	.pixel_format = MCDE_PORTPIXFMT_DSI_24BPP,
	.ifc = DSI_CMD_MODE,
	.link = 0,
	.sync_src = MCDE_SYNCSRC_TE0,
	.update_auto_trig = false,
	.phy = {
		.dsi = {
			.virt_id = 0,
			.num_data_lanes = 2,
			.ui = DSI_UNIT_INTERVAL_0,
			.clk_cont = false,
			.host_eot_gen = true,
			.data_lanes_swap = false,
			.hs_freq = DSI_HS_FREQ_HZ,
			.lp_freq = DSI_LP_FREQ_HZ,
		},
	},
};

struct panel_device panel_display0 = {
	.base = {
		.name = MCDE_DISPLAY_PANEL_NAME,
		.id = PRIMARY_DISPLAY_ID,
		.port = &panel_port0,
		.chnl_id = MCDE_CHNL_A,
		.fifo = MCDE_FIFO_A,
#ifdef CONFIG_MCDE_DISPLAY_PRIMARY_16BPP
		.default_pixel_format = MCDE_OVLYPIXFMT_RGB565,
#else
		.default_pixel_format = MCDE_OVLYPIXFMT_RGBA8888,
#endif
		/* These are setup via the panel files */
		.native_x_res = 0,
		.native_y_res = 0,
#ifdef CONFIG_DISPLAY_GENERIC_DSI_PRIMARY_VSYNC
		.synchronized_update = true,
#else
		.synchronized_update = false,
#endif
		.deep_standby_as_power_off = true,
		.platform_reset = panel_platform_reset,
		.platform_enable = panel_platform_enable,
		.platform_disable = panel_platform_disable,
		/* TODO: Remove rotation buffers once ESRAM driver
		 * is completed */
		.rotbuf1 = U8500_ESRAM_BASE + 0x20000 * 4 + 0x2000,
		.rotbuf2 = U8500_ESRAM_BASE + 0x20000 * 4 + 0x10000,
		.dev = {
			.platform_data = &panel_display0_pdata,
		},
	}
};
#endif /* CONFIG_DISPLAY_PANEL_DSI_PRIMARY */

#ifdef CONFIG_DISPLAY_PANEL_DSI_SECONDARY
static struct mcde_port panel_port1 = {
	.type = MCDE_PORTTYPE_DSI,
	.mode = MCDE_PORTMODE_CMD,
	.pixel_format = MCDE_PORTPIXFMT_DSI_24BPP,
	.ifc = DSI_CMD_MODE,
	.link = 1,
	.sync_src = MCDE_SYNCSRC_BTA,
	.update_auto_trig = false,
	.phy = {
		.dsi = {
			.virt_id = 0,
			.num_data_lanes = 2,
			.ui = DSI_UNIT_INTERVAL_0,
			.clk_cont = false,
			.data_lanes_swap = false,
		},
	},
};

struct panel_device panel_display1 = {
	.base = {
		.name = MCDE_DISPLAY_PANEL_NAME,
		.id = SECONDARY_DISPLAY_ID,
		.port = &panel_port1,
		.chnl_id = MCDE_CHNL_A,
		.fifo = MCDE_FIFO_A,
#ifdef CONFIG_MCDE_DISPLAY_PRIMARY_16BPP
		.default_pixel_format = MCDE_OVLYPIXFMT_RGB565,
#else
		.default_pixel_format = MCDE_OVLYPIXFMT_RGBA8888,
#endif
		/* These are setup via the panel files */
		.native_x_res = 0,
		.native_y_res = 0,
#ifdef CONFIG_DISPLAY_GENERIC_DSI_PRIMARY_VSYNC
		.synchronized_update = true,
#else
		.synchronized_update = false,
#endif
		.deep_standby_as_power_off = true,
		.platform_reset = panel_platform_reset,
		.platform_enable = panel_platform_enable,
		.platform_disable = panel_platform_disable,
		/* TODO: Remove rotation buffers once ESRAM driver
		 * is completed */
		.rotbuf1 = U8500_ESRAM_BASE + 0x20000 * 4 + 0x2000,
		.rotbuf2 = U8500_ESRAM_BASE + 0x20000 * 4 + 0x10000,
		.dev = {
			.platform_data = &panel_display1_pdata,
		},
	}
};
#endif /* CONFIG_DISPLAY_PANEL_DSI_SECONDARY */

#ifdef CONFIG_DISPLAY_GENERIC_DSI_SECONDARY
static struct mcde_port subdisplay_port = {
	.type = MCDE_PORTTYPE_DSI,
	.mode = MCDE_PORTMODE_CMD,
	.pixel_format = MCDE_PORTPIXFMT_DSI_24BPP,
	.ifc = 1,
	.link = 1,
	.sync_src = MCDE_SYNCSRC_BTA,
	.update_auto_trig = false,
	.phy = {
		.dsi = {
			.virt_id = 0,
			.num_data_lanes = 2,
			.ui = DSI_UNIT_INTERVAL_1,
			.clk_cont = false,
			.data_lanes_swap = false,
		},
	},

};

static struct mcde_display_generic_platform_data generic_subdisplay_pdata = {
	.reset_gpio = EGPIO_PIN_14,
	.reset_delay = 1,
#ifdef CONFIG_REGULATOR
	.regulator_id = "v-display",
	.min_supply_voltage = 2500000, /* 2.5V */
	.max_supply_voltage = 2800000 /* 2.8V */
#endif
};

static struct mcde_display_device generic_subdisplay = {
	.name = "mcde_disp_generic_subdisplay",
	.id = SECONDARY_DISPLAY_ID,
	.port = &subdisplay_port,
	.chnl_id = MCDE_CHNL_C1,
	.fifo = MCDE_FIFO_C1,
	.default_pixel_format = MCDE_OVLYPIXFMT_RGB565,
	.native_x_res = 864,
	.native_y_res = 480,
#ifdef CONFIG_DISPLAY_GENERIC_DSI_SECONDARY_VSYNC
	.synchronized_update = true,
#else
	.synchronized_update = false,
#endif
	.dev = {
		.platform_data = &generic_subdisplay_pdata,
	},
};
#endif /* CONFIG_DISPLAY_GENERIC_DSI_SECONDARY */


#ifdef CONFIG_DISPLAY_AB8500_TERTIARY
static struct mcde_port port_tvout1 = {
	.type = MCDE_PORTTYPE_DPI,
	.pixel_format = MCDE_PORTPIXFMT_DPI_24BPP,
	.ifc = 0,
	.link = 1,			/* channel B */
	.sync_src = MCDE_SYNCSRC_OFF,
	.update_auto_trig = true,
	.phy = {
		.dpi = {
			.bus_width = 4, /* DDR mode */
			.tv_mode = true,
			.clock_div = MCDE_PORT_DPI_NO_CLOCK_DIV,
		},
	},
};

static struct ab8500_display_platform_data ab8500_display_pdata = {
	.nr_regulators = 2,
	.regulator_id  = {"v-tvout", "v-ab8500-AV-switch"},
	.rgb_2_yCbCr_transform = &rgb_2_yCbCr_transform,
};

static struct ux500_pins *tvout_pins;

static int ab8500_platform_enable(struct mcde_display_device *ddev)
{
	int res = 0;

	if (!tvout_pins) {
		tvout_pins = ux500_pins_get("mcde-tvout");
		if (!tvout_pins)
			return -EINVAL;
	}

	dev_dbg(&ddev->dev, "%s\n", __func__);
	res = ux500_pins_enable(tvout_pins);
	if (res != 0)
		goto failed;

	return res;

failed:
	dev_warn(&ddev->dev, "Failure during %s\n", __func__);
	return res;
}

static int ab8500_platform_disable(struct mcde_display_device *ddev)
{
	int res;

	dev_dbg(&ddev->dev, "%s\n", __func__);

	res = ux500_pins_disable(tvout_pins);
	if (res != 0)
		goto failed;
	return res;

failed:
	dev_warn(&ddev->dev, "Failure during %s\n", __func__);
	return res;
}

struct mcde_display_device tvout_ab8500_display = {
	.name = "mcde_tv_ab8500",
	.id = AB8500_DISPLAY_ID,
	.port = &port_tvout1,
	.chnl_id = MCDE_CHNL_B,
	.fifo = MCDE_FIFO_B,
	.default_pixel_format = MCDE_OVLYPIXFMT_RGB565,
	.native_x_res = 720,
	.native_y_res = 576,
	/* .synchronized_update: Don't care: port is set to update_auto_trig */
	.dev = {
		.platform_data = &ab8500_display_pdata,
	},

	/*
	* We might need to describe the std here:
	* - there are different PAL / NTSC formats (do they require MCDE
	*   settings?)
	*/
	.platform_enable = ab8500_platform_enable,
	.platform_disable = ab8500_platform_disable,
};
#endif /* CONFIG_DISPLAY_AB8500_TERTIARY */

#ifdef CONFIG_DISPLAY_AV8100_TERTIARY
static struct mcde_port port2 = {
	.type = MCDE_PORTTYPE_DSI,
	.mode = MCDE_PORTMODE_CMD,
	.pixel_format = MCDE_PORTPIXFMT_DSI_24BPP,
	.ifc = 1,
	.link = 2,
#ifdef CONFIG_AV8100_HWTRIG_INT
	.sync_src = MCDE_SYNCSRC_TE0,
#endif
#ifdef CONFIG_AV8100_HWTRIG_I2SDAT3
	.sync_src = MCDE_SYNCSRC_TE1,
#endif
#ifdef CONFIG_AV8100_HWTRIG_DSI_TE
	.sync_src = MCDE_SYNCSRC_TE_POLLING,
#endif
#ifdef CONFIG_AV8100_HWTRIG_NONE
	.sync_src = MCDE_SYNCSRC_OFF,
#endif
	.update_auto_trig = true,
	.phy = {
		.dsi = {
			.virt_id = 0,
			.num_data_lanes = 2,
			.ui = DSI_UNIT_INTERVAL_2,
			.clk_cont = false,
			.data_lanes_swap = false,
		},
	},
	.hdmi_sdtv_switch = HDMI_SWITCH,
};

static struct mcde_display_hdmi_platform_data av8100_hdmi_pdata = {
	.reset_gpio = 0,
	.reset_delay = 1,
	.regulator_id = NULL, /* TODO: "display_main" */
	.cvbs_regulator_id = "v-av8100-AV-switch",
	.ddb_id = 1,
	.rgb_2_yCbCr_transform = &rgb_2_yCbCr_transform,
};

struct mcde_display_device av8100_hdmi = {
	.name = "av8100_hdmi",
	.id = AV8100_DISPLAY_ID,
	.port = &port2,
	.chnl_id = MCDE_CHNL_B,
	.fifo = MCDE_FIFO_B,
	.default_pixel_format = MCDE_OVLYPIXFMT_RGB888,
	.native_x_res = 1280,
	.native_y_res = 720,
	.dev = {
		.platform_data = &av8100_hdmi_pdata,
	},
};

static void delayed_work_dispreg_hdmi(struct work_struct *ptr)
{
	if (mcde_display_device_register(&av8100_hdmi))
		pr_warning("Failed to register av8100_hdmi\n");
}
#endif /* CONFIG_DISPLAY_AV8100_TERTIARY */

/*
* This function will create the framebuffer for the display that is registered.
*/
static int display_postregistered_callback(struct notifier_block *nb,
	unsigned long event, void *dev)
{
	struct mcde_display_device *ddev = dev;
	u16 width, height;
	u16 virtual_width, virtual_height;
	bool rotate;
	struct fb_info *fbi;
#ifdef CONFIG_DISPDEV
	struct mcde_fb *mfb;
#endif

	if (event != MCDE_DSS_EVENT_DISPLAY_REGISTERED)
		return 0;

	if (ddev->id < 0 || ddev->id >= MCDE_NR_OF_DISPLAYS)
		return 0;

	mcde_dss_get_native_resolution(ddev, &width, &height);

#ifdef CONFIG_DISPLAY_PANEL_DSI_PRIMARY
	rotate = (ddev->id == PRIMARY_DISPLAY_ID && ROTATE_MAIN);
	if (rotate) {
		u16 tmp = height;
		height = width;
		width = tmp;
	}
#endif

	virtual_width = width;
	virtual_height = height * 2;

#ifdef CONFIG_DISPLAY_AV8100_TERTIARY
#ifndef CONFIG_MCDE_DISPLAY_HDMI_FB_AUTO_CREATE
	if (ddev->id == AV8100_DISPLAY_ID)
		goto out;
#endif
#endif

	/* Create frame buffer */
	fbi = mcde_fb_create(ddev,
		width, height,
		virtual_width, virtual_height,
		ddev->default_pixel_format,
		rotate ? FB_ROTATE_CW : FB_ROTATE_UR);

	if (IS_ERR(fbi)) {
		dev_warn(&ddev->dev,
			"Failed to create fb for display %s\n",
					ddev->name);
		goto display_postregistered_callback_err;
	} else {
		dev_info(&ddev->dev, "Framebuffer created (%s)\n",
					ddev->name);
	}

#ifdef CONFIG_DISPDEV
	mfb = to_mcde_fb(fbi);

	/* Create a dispdev overlay for this display */
	if (dispdev_create(ddev, true, mfb->ovlys[0]) < 0) {
		dev_warn(&ddev->dev,
			"Failed to create disp for display %s\n",
					ddev->name);
		goto display_postregistered_callback_err;
	} else {
		dev_info(&ddev->dev, "Disp dev created for (%s)\n",
					ddev->name);
	}
#endif

#ifdef CONFIG_DISPLAY_AV8100_TERTIARY
#ifndef CONFIG_MCDE_DISPLAY_HDMI_FB_AUTO_CREATE
out:
#endif
#endif
	return 0;

display_postregistered_callback_err:
	return -1;
}

static struct notifier_block display_nb = {
	.notifier_call = display_postregistered_callback,
};

int __init init_display_devices(void)
{
	int ret;

	ret = mcde_dss_register_notifier(&display_nb);
	if (ret)
		pr_warning("Failed to register dss notifier\n");

	if (!display_initialized_during_boot) {
		struct clk *clk_dsi_pll;
		struct clk *clk_hdmi;
		struct clk *clk_tv;

		/* Setup tv clk with correct frequency */
		clk_tv = clk_get(&ux500_mcde_device.dev, "tv");
		if (TV_FREQ_HZ != clk_round_rate(clk_tv, TV_FREQ_HZ))
			pr_warning("TVCLK freq differs\n");
		clk_set_rate(clk_tv, TV_FREQ_HZ);
		clk_put(clk_tv);

		/* Setup hdmi clk with correct frequency */
		clk_hdmi = clk_get(&ux500_mcde_device.dev, "hdmi");
		if (HDMI_FREQ_HZ != clk_round_rate(clk_hdmi, HDMI_FREQ_HZ))
			pr_warning("HDMI_FREQ_HZ differs\n");
		clk_set_rate(clk_hdmi, HDMI_FREQ_HZ);
		clk_put(clk_hdmi);

		/* Setup dsi pll with correct frequency */
		clk_dsi_pll = clk_get(&ux500_mcde_device.dev, "dsihs2");
		if (DSI_PLL_FREQ_HZ != clk_round_rate(clk_dsi_pll,
							DSI_PLL_FREQ_HZ))
			pr_warning("DSI_PLL_FREQ_HZ differs\n");
		clk_set_rate(clk_dsi_pll, DSI_PLL_FREQ_HZ);
		clk_put(clk_dsi_pll);
	}

#ifdef CONFIG_DISPLAY_PANEL_DSI_PRIMARY
	INIT_DELAYED_WORK_DEFERRABLE(&panel_display0.ddr_ape_timeout_work,
						work_ddr_ape_timeout_function);
	((struct panel_platform_data *)panel_display0.base.dev.platform_data)->
						ddr_is_requested = false;
	((struct panel_platform_data *)panel_display0.base.dev.platform_data)->
						ape_is_requested = false;
	/* TODO: enable this code if uboot graphics should be used
	 if (display_initialized_during_boot)
		((struct panel_platform_data *)panel_display0.
		base.dev.platform_data)->skip_init = true;
	 */
	ret = mcde_display_device_register(&panel_display0.base);
	if (ret)
		pr_warning("Failed to register display device 0\n");
#endif

#ifdef CONFIG_DISPLAY_PANEL_DSI_SECONDARY
	ret = mcde_display_device_register(&panel_display1.base);
	if (ret)
		pr_warning("Failed to register display device 1\n");
#endif

#ifdef CONFIG_DISPLAY_GENERIC_DSI_SECONDARY
	ret = mcde_display_device_register(&generic_subdisplay);
	if (ret)
		pr_warning("Failed to register generic sub display device\n");
#endif

#ifdef CONFIG_DISPLAY_AV8100_TERTIARY
	INIT_DELAYED_WORK_DEFERRABLE(&work_dispreg_hdmi,
			delayed_work_dispreg_hdmi);

	schedule_delayed_work(&work_dispreg_hdmi,
			msecs_to_jiffies(DISPREG_HDMI_DELAY));
#endif
#ifdef CONFIG_DISPLAY_AB8500_TERTIARY
	ret = mcde_display_device_register(&tvout_ab8500_display);
	if (ret)
		pr_warning("Failed to register ab8500 tvout device\n");
#endif

	return ret;
}

module_init(init_display_devices);
#endif
