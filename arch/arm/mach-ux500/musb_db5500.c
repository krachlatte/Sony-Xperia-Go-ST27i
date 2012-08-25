/*
 * Copyright (C) ST-Ericsson SA 2011
 *
 * Author: Avinash Kumar <avinash.kumar@stericsson.com> for ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/usb/musb.h>
#include <linux/interrupt.h>
#include <linux/mfd/abx500.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <mach/musb_db5500.h>
#include <mach/prcmu.h>
#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android_composite.h>
#endif
#include <mach/devices.h>
#include <linux/mfd/abx500/ab5500.h>
#include <linux/mfd/abx500.h>
#include <plat/pincfg.h>
#include <mach/gpio.h>
#include <linux/usb-ab5500.h>
#include "pins.h"

#define AB5500_USB_DEVICE_DISABLE 0x0
#define AB5500_MAIN_WDOG_CTRL_REG      0x01
#define AB5500_USB_LINE_STAT_REG       0x80
#define AB5500_USB_PHY_CTRL_REG        0x8A
#define AB5500_MAIN_WATCHDOG_ENABLE 0x1
#define AB5500_MAIN_WATCHDOG_KICK 0x2
#define AB5500_MAIN_WATCHDOG_DISABLE 0x0
#define AB5500_SYS_CTRL2_BLOCK	0x2

#define DEVICE_NAME "musb_qos"
static struct completion usb_link_status_update;
static int irq_host_remove;
static int irq_device_remove;
static int irq_device_insert;
static int irq_link_status_update;
static struct regulator *musb_vape_supply;
/* Phy Status. Right now Used for Device only. */
static int phy_enable_stat = USB_DISABLE;
static struct device *device;
static struct clk *sysclock;
static int ab5500_rev;
/* Phy Status. Right now Used for Device only. */
#ifdef CONFIG_USB_OTG_20
static int irq_adp_plug;
static int irq_adp_unplug;
#endif

/*
 * work queue for USB cable insertion processing
 */
static struct work_struct usb_host_remove;
static struct work_struct usb_device_remove;
static struct work_struct usb_device_insert;

static struct work_struct usb_lnk_status_update;
static struct workqueue_struct *usb_cable_wq;

static enum musb_link_status stm_usb_link_curr_state = USB_LINK_NOT_CONFIGURED;

static void usb_host_remove_work(struct work_struct *work);
static void usb_link_status_update_work(struct work_struct *work);
static void usb_device_remove_work(struct work_struct *work);
static void usb_device_insert_work(struct work_struct *work);
static void usb_device_phy_en(int enable);
static void usb_host_phy_en(int enable);

static int usb_cs_gpio;
static struct ux500_pins *usb_gpio_pins;

#include <linux/wakelock.h>
static struct wake_lock ab5500_musb_wakelock;
static int ab5500_rev;

/* Phy Status. Right now Used for Device only. */
static int boot_time_flag = USB_DISABLE;

/**
 * stm_musb_states - Different states of musb_chip
 *
 * Used for USB cable plug-in state machine
 */
enum stm_musb_states {
	USB_IDLE,
	USB_DEVICE,
	USB_HOST,
	USB_DEDICATED_CHG,
};
static enum stm_musb_states stm_musb_curr_state = USB_IDLE;
void musb_platform_session_req()
{
   /* Discaharge the VBUS */
   /* TODO */
}

/**
 * usb_kick_watchdog() - Kick the watch dog timer
 *
 * This function used to Kick the watch dog timer
 */
static void usb_kick_watchdog(void)
{
	abx500_set_register_interruptible(device,
			AB5500_SYS_CTRL2_BLOCK,
			AB5500_MAIN_WDOG_CTRL_REG,
			AB5500_MAIN_WATCHDOG_ENABLE);

		udelay(WATCHDOG_DELAY_US);

	abx500_set_register_interruptible(device,
			AB5500_SYS_CTRL2_BLOCK,
			AB5500_MAIN_WDOG_CTRL_REG,
			(AB5500_MAIN_WATCHDOG_ENABLE
			 | AB5500_MAIN_WATCHDOG_KICK));

		udelay(WATCHDOG_DELAY_US);

	abx500_set_register_interruptible(device,
			AB5500_SYS_CTRL2_BLOCK,
			AB5500_MAIN_WDOG_CTRL_REG,
			AB5500_MAIN_WATCHDOG_DISABLE);

		udelay(WATCHDOG_DELAY_US);
}


/**
 * musb_platform_device_en(): Enable/Disable the device.
 * This function used to enable usb sysclk and supply
 */
void musb_platform_device_en(int enable)
{
	if ((enable == 1) && (phy_enable_stat == USB_DISABLE))  {

		usb_device_phy_en(USB_ENABLE);
		return;
	}
}

/**
 * This function is used to signal the completion of
 * USB Link status register update
 */
static irqreturn_t usb_link_status_update_handler(int irq, void *data)
{
	queue_work(usb_cable_wq, &usb_lnk_status_update);
	return IRQ_HANDLED;
}


/**
 * This function used to remove the voltage for USB device mode.
 */
static irqreturn_t usb_device_remove_handler(int irq, void *data)
{
	queue_work(usb_cable_wq, &usb_device_remove);
	return IRQ_HANDLED;
}

/**
 * This function used to remove the voltage for USB device mode.
 */
static irqreturn_t usb_device_insert_handler(int irq, void *data)
{
	queue_work(usb_cable_wq, &usb_device_insert);
	return IRQ_HANDLED;
}

/**
 * usb_host_remove_work : work handler for host cable insert.
 * @work: work structure
 *
 * This function is used to handle the host cable insert work.
 */
static void usb_host_remove_work(struct work_struct *work)
{
	/* disable usb chip Select */
	gpio_set_value(usb_cs_gpio, 0);


	usb_host_phy_en(USB_DISABLE);

}

/* Work created after an link status update handler*/
static void usb_link_status_update_work(struct work_struct *work)
{
	u8 val = 0;
	int ret = 0;
	int gpioval = 0;
	(void)abx500_get_register_interruptible(device,
			AB5500_BANK_USB, AB5500_USB_LINE_STAT_REG, &val);

	if (ab5500_rev == AB5500_2_0)
		val = (val & AB5500_USB_LINK_STATUS_V2) >> 3;
	 else
		val = (val & AB5500_USB_LINK_STATUS) >> 3;

	stm_usb_link_curr_state = (enum musb_link_status) val;

	switch (stm_usb_link_curr_state) {

	case USB_LINK_HM_IDGND:

		if (ab5500_rev == AB5500_2_0)
			break;

		/* enable usb chip Select */
		ret = gpio_direction_output(usb_cs_gpio, gpioval);
		if (ret < 0) {
			dev_err(device, "usb_cs_gpio: gpio direction failed\n");
			gpio_free(usb_cs_gpio);
			return ret;
		}
		gpio_set_value(usb_cs_gpio, 1);

		usb_host_phy_en(USB_ENABLE);

		musb_set_session();
		break;

	case USB_LINK_HM_IDGND_V2:

		if (!(ab5500_rev == AB5500_2_0))
			break;

		/* enable usb chip Select */
		ret = gpio_direction_output(usb_cs_gpio, gpioval);
		if (ret < 0) {
			dev_err(device, "usb_cs_gpio: gpio direction failed\n");
			gpio_free(usb_cs_gpio);
			return ret;
		}
		gpio_set_value(usb_cs_gpio, 1);

		usb_host_phy_en(USB_ENABLE);

		musb_set_session();
		break;
	default:
		break;
	}
}

/**
 * usb_device_insert_work : work handler for device cable insert.
 * @work: work structure
 *
 * This function is used to handle the device cable insert work.
 */
static void usb_device_insert_work(struct work_struct *work)
{
	int ret = 0, val = 1;

	stm_musb_curr_state = USB_DEVICE;

	usb_device_phy_en(USB_ENABLE);


	/* enable usb chip Select */
	ret = gpio_direction_output(usb_cs_gpio, val);
	if (ret < 0) {
		dev_err(device, "usb_cs_gpio: gpio direction failed\n");
		gpio_free(usb_cs_gpio);
		return ret;
	}
	gpio_set_value(usb_cs_gpio, 1);

}


/**
 * usb_device_remove_work : work handler for device cable insert.
 * @work: work structure
 *
 * This function is used to handle the device cable insert work.
 */
static void usb_device_remove_work(struct work_struct *work)
{
	/* disable usb chip Select */
	gpio_set_value(usb_cs_gpio, 0);
	usb_device_phy_en(USB_DISABLE);
}

/**
 * usb_host_remove_handler() - Removed the USB host cable
 *
 * This function used to detect the USB host cable removed.
 */
static irqreturn_t usb_host_remove_handler(int irq, void *data)
{

	queue_work(usb_cable_wq, &usb_host_remove);
	return IRQ_HANDLED;
}

/**
 * usb_device_phy_en() - for enabling the 5V to usb gadget
 * @enable: to enabling the Phy for device.
 *
 * This function used to set the voltage for USB gadget mode.
 */
static void usb_device_phy_en(int enable)
{
	if (phy_enable_stat == enable)
		return;

		if (enable == USB_ENABLE) {
			wake_lock(&ab5500_musb_wakelock);
			ux500_pins_enable(usb_gpio_pins);
			clk_enable(sysclock);
			phy_enable_stat = USB_ENABLE;
			regulator_enable(musb_vape_supply);

			stm_musb_context(USB_ENABLE);

			usb_kick_watchdog();

			abx500_set_register_interruptible(device,
					AB5500_BANK_USB,
					AB5500_USB_PHY_CTRL_REG,
					AB5500_USB_DEVICE_ENABLE);


		} else { /* enable == USB_DISABLE */
			if (boot_time_flag)
				boot_time_flag = USB_DISABLE;

			/*
			 * Workaround for bug31952 in ABB cut2.0. Write 0x1
			 * before disabling the PHY.
			 */
			abx500_set_register_interruptible(device,
					AB5500_BANK_USB,
					AB5500_USB_PHY_CTRL_REG,
					AB5500_USB_DEVICE_ENABLE);

			udelay(100);

			abx500_set_register_interruptible(device,
					AB5500_BANK_USB,
					AB5500_USB_PHY_CTRL_REG,
					AB5500_USB_DEVICE_DISABLE);

			phy_enable_stat = USB_DISABLE;
			regulator_disable(musb_vape_supply);
			clk_disable(sysclock);

			stm_musb_context(USB_DISABLE);

			ux500_pins_disable(usb_gpio_pins);
			wake_unlock(&ab5500_musb_wakelock);
		}
}

/**
 * usb_host_phy_en() - for enabling the 5V to usb host
 * @enable: to enabling the Phy for host.
 *
 * This function used to set the voltage for USB host mode
 */
static void usb_host_phy_en(int enable)
{
	if (enable == USB_ENABLE) {
		wake_lock(&ab5500_musb_wakelock);
		ux500_pins_enable(usb_gpio_pins);
		clk_enable(sysclock);
		regulator_enable(musb_vape_supply);

		stm_musb_context(USB_ENABLE);

		usb_kick_watchdog();

		/* Enable the PHY */
		abx500_set_register_interruptible(device,
			AB5500_BANK_USB,
			AB5500_USB_PHY_CTRL_REG,
			AB5500_USB_HOST_ENABLE);
	} else { /* enable == USB_DISABLE */
		if (boot_time_flag)
			boot_time_flag = USB_DISABLE;

		/*
		 * Workaround for bug31952 in ABB cut2.0. Write 0x1
		 * before disabling the PHY.
		 */
		abx500_set_register_interruptible(device, AB5500_BANK_USB,
				 AB5500_USB_PHY_CTRL_REG,
				 AB5500_USB_HOST_ENABLE);

		udelay(100);

		abx500_set_register_interruptible(device, AB5500_BANK_USB,
				 AB5500_USB_PHY_CTRL_REG,
				 AB5500_USB_HOST_DISABLE);


		regulator_disable(musb_vape_supply);


		clk_disable(sysclock);

		stm_musb_context(USB_DISABLE);
		ux500_pins_disable(usb_gpio_pins);
		wake_unlock(&ab5500_musb_wakelock);
	}

}


/**
 * musb_phy_en : register USB callback handlers for ab5500
 * @mode: value for mode.
 *
 * This function is used to register USB callback handlers for ab5500.
 */
int musb_phy_en(u8 mode)
{
	int ret = 0;
	if (!device)
		return -EINVAL;
	ab5500_rev = abx500_get_chip_id(device);

	sysclock = clk_get(device, "sysclk");
	if (IS_ERR(sysclock)) {
		ret = PTR_ERR(sysclock);
		sysclock = NULL;
		return ret;
	}

	musb_vape_supply = regulator_get(device, "v-ape");
	if (!musb_vape_supply) {
		dev_err(device, "Could not get v-ape supply\n");

		return -EINVAL;
	}

	/* create a thread for work */
	usb_cable_wq = create_singlethread_workqueue(
				"usb_cable_wq");
		if (usb_cable_wq == NULL)
			return -ENOMEM;

	INIT_WORK(&usb_host_remove, usb_host_remove_work);
	INIT_WORK(&usb_lnk_status_update,
				usb_link_status_update_work);

	/* Required for Host, Device and OTG mode */
	init_completion(&usb_link_status_update);

	ret = request_threaded_irq(irq_link_status_update,
		NULL, usb_link_status_update_handler,
		IRQF_NO_SUSPEND | IRQF_SHARED,
		"usb-link-status-update", device);
	if (ret < 0) {
		printk(KERN_ERR "failed to set the callback"
				" handler for usb charge"
				" detect done\n");
		return ret;
	}

	ret = request_threaded_irq(irq_device_remove, NULL,
		usb_device_remove_handler,
		IRQF_NO_SUSPEND | IRQF_SHARED,
		"usb-device-remove", device);
	if (ret < 0) {
		printk(KERN_ERR "failed to set the callback"
				" handler for usb device"
				" removal\n");
		return ret;
	}

	ret = request_threaded_irq(irq_device_insert, NULL,
		usb_device_insert_handler,
		IRQF_NO_SUSPEND | IRQF_SHARED,
		"usb-device-insert", device);
	if (ret < 0) {
		printk(KERN_ERR "failed to set the callback"
				" handler for usb device"
				" insertion\n");
		return ret;
	}


	ret = request_threaded_irq((irq_host_remove), NULL,
		usb_host_remove_handler,
		IRQF_NO_SUSPEND | IRQF_SHARED,
		"usb-host-remove", device);
	if (ret < 0) {
		printk(KERN_ERR "failed to set the callback"
				" handler for usb host"
				" removal\n");
		return ret;
	}

	INIT_WORK(&usb_device_remove, usb_device_remove_work);
	INIT_WORK(&usb_device_insert, usb_device_insert_work);

	usb_kick_watchdog();

	return 0;
}

/**
 * musb_force_detect : detect the USB cable during boot time.
 * @mode: value for mode.
 *
 * This function is used to detect the USB cable during boot time.
 */
int musb_force_detect(u8 mode)
{
	int ret;
	int val = 1;
	int usb_status = 0;
	int gpioval = 0;
	if (!device)
		return -EINVAL;

	abx500_set_register_interruptible(device,
			AB5500_BANK_USB,
			AB5500_USB_PHY_CTRL_REG,
			AB5500_USB_DEVICE_ENABLE);

	udelay(100);

	abx500_set_register_interruptible(device,
			AB5500_BANK_USB,
			AB5500_USB_PHY_CTRL_REG,
			AB5500_USB_DEVICE_DISABLE);

	abx500_set_register_interruptible(device,
			AB5500_BANK_USB,
			AB5500_USB_PHY_CTRL_REG,
			AB5500_USB_HOST_ENABLE);

	udelay(100);

	abx500_set_register_interruptible(device,
			AB5500_BANK_USB,
			AB5500_USB_PHY_CTRL_REG,
			AB5500_USB_HOST_DISABLE);

	usb_kick_watchdog();

	(void)abx500_get_register_interruptible(device,
			AB5500_BANK_USB, AB5500_USB_LINE_STAT_REG, &usb_status);

	usb_status = (usb_status & AB5500_USB_LINK_STATUS) >> 3;

	stm_usb_link_curr_state = (enum musb_link_status) usb_status;

	switch (stm_usb_link_curr_state) {

	case USB_LINK_STD_HOST_NC:
	case USB_LINK_STD_HOST_C_NS:
	case USB_LINK_STD_HOST_C_S:
	case USB_LINK_HOST_CHG_NM:
	case USB_LINK_HOST_CHG_HS:
	case USB_LINK_HOST_CHG_HS_CHIRP:
		boot_time_flag = USB_ENABLE;
		usb_device_phy_en(USB_ENABLE);

		/* enable usb chip Select */
		ret = gpio_direction_output(usb_cs_gpio, val);
		if (ret < 0) {
			dev_err(device, "usb_cs_gpio: gpio direction failed\n");
			gpio_free(usb_cs_gpio);
			return ret;
		}
		gpio_set_value(usb_cs_gpio, 1);

		break;

	case USB_LINK_HM_IDGND:
	case USB_LINK_HM_IDGND_V2:
		/* enable usb chip Select */
		ret = gpio_direction_output(usb_cs_gpio, gpioval);
		if (ret < 0) {
			dev_err(device, "usb_cs_gpio: gpio direction failed\n");
			gpio_free(usb_cs_gpio);
			return ret;
		}
		gpio_set_value(usb_cs_gpio, 1);
		boot_time_flag = USB_ENABLE;
		usb_host_phy_en(USB_ENABLE);

		musb_set_session();

		break;
	default:
		break;
	}

	return 0;
}

/*
 * musb_get_abx500_rev : Get the ABx500 revision number
 *
 * This function returns the ABx500 revision number.
 */
int musb_get_abx500_rev()
{
	return abx500_get_chip_id(device);
}

static int __devinit ab5500_musb_probe(struct platform_device *pdev)
{
	int ret;
	struct ab5500_platform_usb_data *pdata = pdev->dev.platform_data;
	device = &pdev->dev;
	usb_cs_gpio = pdata->usb_cs;

	irq_host_remove = platform_get_irq_byname(pdev, "usb_idgnd_f");
	if (irq_host_remove < 0) {
		dev_err(&pdev->dev, "Host remove irq not found, err %d\n",
			irq_host_remove);
		return irq_host_remove;
	}

	irq_device_remove = platform_get_irq_byname(pdev, "VBUS_F");
	if (irq_device_remove < 0) {
		dev_err(&pdev->dev, "Device remove irq not found, err %d\n",
			irq_device_remove);
		return irq_device_remove;
	}

	irq_device_insert = platform_get_irq_byname(pdev, "VBUS_R");
	if (irq_device_remove < 0) {
		dev_err(&pdev->dev, "Device remove irq not found, err %d\n",
			irq_device_remove);
		return irq_device_remove;
	}

	irq_link_status_update = platform_get_irq_byname(pdev,
		"Link_Update");
	if (irq_link_status_update < 0) {
		dev_err(&pdev->dev, "USB Link status irq not found, err %d\n",
			irq_link_status_update);
		return irq_link_status_update;
	}

	ret = gpio_request(usb_cs_gpio, "usb-cs");
	if (ret < 0)
		dev_err(&pdev->dev, "usb gpio request fail\n");

	/* Aquire GPIO alternate config struct for USB */
	usb_gpio_pins = ux500_pins_get(dev_name(device));

	if (!usb_gpio_pins) {
		dev_err(device, "Could not get usb_gpio_pins structure\n");

		return -EINVAL;
	}
	/*
	 * wake lock is acquired when usb cable is connected and released when
	 * cable is removed
	 */
	wake_lock_init(&ab5500_musb_wakelock, WAKE_LOCK_SUSPEND, "ab5500-usb");

	return 0;
}

static int __devexit ab5500_musb_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver ab5500_musb_driver = {
	.driver		= {
		.name	= "ab5500-usb",
		.owner	= THIS_MODULE,
	},
	.probe		= ab5500_musb_probe,
	.remove		= __devexit_p(ab5500_musb_remove),
};

static int __init ab5500_musb_init(void)
{
	return platform_driver_register(&ab5500_musb_driver);
}
module_init(ab5500_musb_init);

static void __exit ab5500_musb_exit(void)
{
	platform_driver_unregister(&ab5500_musb_driver);
}
module_exit(ab5500_musb_exit);

MODULE_LICENSE("GPL v2");
