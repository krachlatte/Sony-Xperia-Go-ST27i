/*
 * Copyright (C) 2009 ST Ericsson.
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/usb/musb.h>
#ifdef CONFIG_USB_OTG_NOTIFICATION
#include <linux/usb/otg.h>
#endif
#include <linux/interrupt.h>
#include <linux/mfd/abx500.h>
#include <linux/mfd/ab8500.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/kernel_stat.h>

#include <mach/musb_db8500.h>
#include <mach/prcmu.h>
#include <plat/pincfg.h>
#include "pins.h"
#include "pm/cpufreq.h"
#include <linux/pm_qos_params.h>

static struct pm_qos_request_list *usb_pm_qos_latency;
static bool usb_pm_qos_is_latency_0;

/* Registers in Bank 0x05 */
#define AB8500_REGU_VUSB_CTRL_REG      0x82
#define AB8500_USB_LINE_STAT_REG       0x80
#define AB8500_USB_LINE_CTRL1_REG      0x81
#define AB8500_USB_LINE_CTRL2_REG      0x82
#define AB8500_USB_LINE_CTRL3_REG      0x83
#define AB8500_USB_LINE_CTRL4_REG      0x84
#define AB8500_USB_LINE_CTRL5_REG      0x85
#define AB8500_USB_OTG_CTRL_REG        0x87
#define AB8500_USB_OTG_STAT_REG        0x88
#define AB8500_USB_OTG_STAT_REG        0x88
#define AB8500_USB_CTRL_SPARE_REG      0x89
#define AB8500_USB_PHY_CTRL_REG        0x8A
#define AB8500_USB_ADP_CTRL_REG        0x93

/* Registers in Bank 0x0E */
#define AB8500_IT_LATCH12_REG          0x2B
#define AB8500_IT_MASK2_REG            0x41
#define AB8500_IT_MASK12_REG           0x4B
#define AB8500_IT_MASK20_REG           0x53
#define AB8500_IT_MASK21_REG           0x54
#define AB8500_IT_SOURCE2_REG          0x01
#define AB8500_IT_SOURCE20_REG         0x13
#define AB8500_IT_SOURCE21_REG         0x14

/* Registers in bank 0x11 */
#define AB8500_BANK12_ACCESS	       0x00

/* Registers in bank 0x12 */
#define AB8500_USB_PHY_TUNE1           0x05
#define AB8500_USB_PHY_TUNE2           0x06
#define AB8500_USB_PHY_TUNE3           0x07

#define DEVICE_NAME "musb_qos"

#define USB_LINK_STAT_DELAY            100
static struct completion usb_link_status_update;
static struct device *device;
static struct regulator *musb_vape_supply;
static struct regulator *musb_vintcore_supply, *musb_smps2_supply;
static struct clk *sysclock;
static struct ux500_pins *usb_gpio_pins;

static int boot_time_flag = USB_DISABLE;
static int irq_host_remove;
static int irq_device_remove;
static int irq_device_insert;
static int irq_link_status_update;
static int ab8500_rev;
/* Phy Status. Right now Used for Device only. */
static int phy_enable_stat = USB_DISABLE;
#ifdef CONFIG_USB_OTG_20
static int irq_adp_plug;
static int irq_adp_unplug;
#endif
/*
 * work queue for USB cable insertion processing
 */
static struct work_struct usb_host_remove;
static struct work_struct usb_device_insert;
static struct work_struct usb_device_remove;
static struct work_struct usb_lnk_status_update;
static struct work_struct usb_dedicated_charger_remove;
static struct workqueue_struct *usb_cable_wq;

static void usb_host_remove_work(struct work_struct *work);
static void usb_device_remove_work(struct work_struct *work);
static void usb_link_status_update_work(struct work_struct *work);
static void usb_dedicated_charger_remove_work(struct work_struct *work);
static void usb_device_phy_en(int enable);
static void usb_host_phy_en(int enable);

#include <linux/wakelock.h>
static struct wake_lock ab8500_musb_wakelock;
static struct wake_lock wakeup_wakelock;
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
enum stm_musb_states stm_musb_curr_state = USB_IDLE;

/*
 Flag for ER425908 WA
*/
#define IDGND_WA 1
#define RIDA_WA  0

/* ACA Modification */
static enum musb_link_status stm_usb_link_curr_state = USB_LINK_NOT_CONFIGURED;
static enum musb_link_status stm_usb_link_prev_state = USB_LINK_NOT_CONFIGURED;

#define USB_PROBE_DELAY 1000 /* 1 seconds */
#define USB_LIMIT (200) /* If we have more than 200 irqs per second */
static struct delayed_work work_usb_workaround;
static struct delayed_work work_usb_charger_det_enable;

void musb_set_session(int);
void stm_musb_force_disconnect(void);
#ifdef	CONFIG_PM
void stm_musb_context(int);
#endif

#ifdef CONFIG_USB_OTG_20
int musb_adp(void);
static void enable_adp(void)
{
	if (ab8500_rev == AB8500_REV_20)
		abx500_set_register_interruptible(device,
			AB8500_USB,
			AB8500_USB_ADP_CTRL_REG,
			AB8500_USB_ADP_ENABLE);
}
#endif

void musb_platform_session_req(void)
{
   /* Discaharge the VBUS */
   /* TODO */
}

/*
 * musb_platform_device_en(): Enable/Disable the device.

 */

void musb_platform_device_en(int enable)
{
	int ret;

	if ((enable == 1) && (phy_enable_stat == USB_DISABLE)) {
		stm_musb_curr_state = USB_DEVICE;
		usb_device_phy_en(USB_ENABLE);
		return;
	}

	if ((enable == 1) && (phy_enable_stat == USB_ENABLE)) {
		/* Phy already enabled. no need to do anything. */
		return;
	}

	if ((enable == 0) && (phy_enable_stat == USB_DISABLE)) {
		/* Phy already disabled. no need to do anything. */
		return;
	}

	if ((enable == 0) && (phy_enable_stat == USB_ENABLE)) {
		/* Phy enabled. Disable it */
		abx500_set_register_interruptible(device,
			AB8500_USB,
			AB8500_USB_PHY_CTRL_REG,
			AB8500_USB_DEVICE_DISABLE);
		prcmu_qos_update_requirement(PRCMU_QOS_APE_OPP,
			DEVICE_NAME, 50);
		prcmu_release_usb_wakeup_state();
		regulator_disable(musb_vape_supply);
		regulator_disable(musb_vintcore_supply);
		regulator_set_optimum_mode(musb_vintcore_supply, 0);
		ret = regulator_set_voltage(musb_vintcore_supply,
					    0, 1350000);
		if (ret < 0)
			printk(KERN_ERR "Failed to set the Vintcore"
					" to 0 V .. 1.35 V, ret=%d\n",
					ret);
		regulator_disable(musb_smps2_supply);
		clk_disable(sysclock);
		phy_enable_stat = USB_DISABLE;

		return;
	}
}


static void usb_charger_det_enable(struct work_struct *work)
{
	int i, ret;
	u8 val;

	/*
	 * Until the IT source register is read the UsbLineStatus
	 * register is not updated.
	 */
	ret = abx500_get_register_interruptible(device,
		AB8500_INTERRUPT, AB8500_IT_SOURCE21_REG, &val);
	if (ret < 0) {
		dev_err(device, "%s ab8500 read 0x0e14 failed\n", __func__);
		return;
	}
	ret = abx500_get_register_interruptible(device,
	  AB8500_USB, AB8500_USB_LINE_STAT_REG, &val);
	if (ret < 0) {
		dev_err(device, "%s ab8500 read 0x580 failed\n", __func__);
		return;
	}
	if (val != 0)
		return;	/* connection type already detected,
			   so we do not need workaround */

	dev_info(device, "USB cable detect workaround activated\n");
	val = 0;
	ret = abx500_get_register_interruptible(device,
		AB8500_INTERRUPT, AB8500_IT_SOURCE2_REG, &val);
	if (ret < 0) {
		dev_err(device, "%s ab8500 read 0x0e01 failed\n", __func__);
		return;
	}
	if (!(val & 0x80))
		return;	/* workaround will not be activated because
			   vbus is not detected */

	ret = abx500_set_register_interruptible(device,
			AB8500_USB, AB8500_USB_PHY_CTRL_REG, 0x00);
	if (ret < 0) {
		dev_err(device, "%s ab8500 write 0x58A failed\n", __func__);
		return;
	}
	ret = abx500_set_register_interruptible(device,
			AB8500_USB, AB8500_USB_LINE_CTRL2_REG, 0x01);
	if (ret < 0) {
		dev_err(device, "%s ab8500 write 0x582 failed\n", __func__);
		return;
	}

	for (i = 10; i != 0; i--) {
		val = 0;
		ret = abx500_get_register_interruptible(device,
			AB8500_INTERRUPT, AB8500_IT_LATCH12_REG, &val);
		if (ret < 0) {
			dev_err(device, "%s ab8500 read 0x0e2b failed\n",
				__func__);
			return;
		}
		if (val & 0x80)
			break;
		msleep(2);
	}

	ret = abx500_set_register_interruptible(device,
			AB8500_USB, AB8500_USB_LINE_CTRL2_REG, 0x00);
	if (ret < 0) {
		dev_err(device, "%s ab8500 write 0x582 failed\n", __func__);
		return;
	}

	ret = abx500_get_register_interruptible(device,
			AB8500_USB, AB8500_USB_LINE_STAT_REG, &val);
	if (ret < 0) {
		dev_err(device, "%s ab8500 read 0x580 failed\n", __func__);
		return;
	}
}

static void usb_load(struct work_struct *work)
{
	int cpu;
	unsigned int num_irqs = 0;
	static unsigned int old_num_irqs = UINT_MAX;

	for_each_online_cpu(cpu)
		num_irqs += kstat_irqs_cpu(IRQ_DB8500_USBOTG, cpu);

	if ((num_irqs > old_num_irqs) &&
	    (num_irqs - old_num_irqs) > USB_LIMIT) {
		prcmu_qos_update_requirement(PRCMU_QOS_ARM_OPP,
					     "usb", 125);
		if (!usb_pm_qos_is_latency_0) {
			usb_pm_qos_latency =
			pm_qos_add_request(PM_QOS_CPU_DMA_LATENCY, 0);
			usb_pm_qos_is_latency_0 = true;
		}
	} else {
		prcmu_qos_update_requirement(PRCMU_QOS_ARM_OPP,
						"usb", 25);
		if (usb_pm_qos_is_latency_0) {
			pm_qos_remove_request(usb_pm_qos_latency);
			usb_pm_qos_is_latency_0 = false;
		}
	}
	old_num_irqs = num_irqs;

	schedule_delayed_work_on(0,
				 &work_usb_workaround,
				 msecs_to_jiffies(USB_PROBE_DELAY));
}

#ifdef CONFIG_USB_OTG_NOTIFICATION
static void send_aca_connection_event(enum usb_otg_event event)
{
	static int aca_conn_notification_sent;

	if (event == OTG_EVENT_ACA_CONNECTED &&
			!aca_conn_notification_sent) {
		otg_send_event(OTG_EVENT_ACA_CONNECTED);
		aca_conn_notification_sent = 1;
	} else if (event == OTG_EVENT_ACA_DISCONNECTED &&
			aca_conn_notification_sent) {
		otg_send_event(OTG_EVENT_ACA_DISCONNECTED);
		aca_conn_notification_sent = 0;
	}
}
#endif

/**
 * usb_host_phy_en() - for enabling the 5V to usb host
 * @enable: to enabling the Phy for host.
 *
 * This function used to set the voltage for USB host mode
 */
static void usb_host_phy_en(int enable)
{
	int volt = 0;
	int ret = -1;

	if (enable == USB_ENABLE) {
		wake_lock(&ab8500_musb_wakelock);
		ux500_pins_enable(usb_gpio_pins);
		clk_enable(sysclock);
		regulator_enable(musb_vape_supply);
		regulator_enable(musb_smps2_supply);

		/* Set Vintcore12 LDO to 1.3V */
		ret = regulator_set_voltage(musb_vintcore_supply,
						1300000, 1350000);
		if (ret < 0)
			printk(KERN_ERR "Failed to set the Vintcore"
					" to 1.3V, ret=%d\n", ret);
		ret = regulator_set_optimum_mode(musb_vintcore_supply,
						 28000);
		if (ret < 0)
			printk(KERN_ERR "Failed to set optimum mode"
					" (ret=%d)\n", ret);
		regulator_enable(musb_vintcore_supply);
		volt = regulator_get_voltage(musb_vintcore_supply);
		if ((volt != 1300000) && (volt != 1350000))
			printk(KERN_ERR "Vintcore is not"
					" set to 1.3V"
					" volt=%d\n", volt);
#ifdef	CONFIG_PM
		stm_musb_context(USB_ENABLE);
#endif

		/* Workaround for USB performance issue. */
		schedule_delayed_work_on(0,
				&work_usb_workaround,
				msecs_to_jiffies(USB_PROBE_DELAY));

		prcmu_qos_update_requirement(PRCMU_QOS_APE_OPP,
				DEVICE_NAME, 100);

		/* Enable the PHY */
		abx500_set_register_interruptible(device,
			AB8500_USB,
			AB8500_USB_PHY_CTRL_REG,
			AB8500_USB_HOST_ENABLE);
	} else { /* enable == USB_DISABLE */
		if (boot_time_flag)
			boot_time_flag = USB_DISABLE;

		/*
		 * Workaround for bug31952 in ABB cut2.0. Write 0x1
		 * before disabling the PHY.
		 */
		abx500_set_register_interruptible(device, AB8500_USB,
			     AB8500_USB_PHY_CTRL_REG,
			     AB8500_USB_HOST_ENABLE);

		udelay(200);

		abx500_set_register_interruptible(device, AB8500_USB,
			     AB8500_USB_PHY_CTRL_REG,
			     AB8500_USB_HOST_DISABLE);

		prcmu_qos_update_requirement(PRCMU_QOS_APE_OPP,
					DEVICE_NAME, 50);

		/* Workaround for USB performance issue. */
		cancel_delayed_work_sync(&work_usb_workaround);

		prcmu_release_usb_wakeup_state();
		regulator_disable(musb_vape_supply);
		regulator_disable(musb_smps2_supply);
		regulator_disable(musb_vintcore_supply);
		regulator_set_optimum_mode(musb_vintcore_supply, 0);
		/* Set Vintcore12 LDO to 0V to 1.35V */
		ret = regulator_set_voltage(musb_vintcore_supply,
						0000000, 1350000);
		if (ret < 0)
			printk(KERN_ERR "Failed to set the Vintcore"
					" to 0V to 1.35V,"
					" ret=%d\n", ret);
		clk_disable(sysclock);
		ux500_pins_disable(usb_gpio_pins);
		wake_unlock(&ab8500_musb_wakelock);
	}
}

/**
 * usb_host_remove_handler() - Removed the USB host cable
 *
 * This function used to detect the USB host cable removed.
 */
static irqreturn_t usb_host_remove_handler(int irq, void *data)
{
#ifdef CONFIG_USB_OTG_NOTIFICATION
	send_aca_connection_event(OTG_EVENT_ACA_DISCONNECTED);
#endif

	if (stm_musb_curr_state == USB_HOST) {
		musb_set_session(0);
		/* Change the current state */
		stm_musb_curr_state = USB_IDLE;
		queue_work(usb_cable_wq, &usb_host_remove);
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}
/**
 * usb_device_phy_en() - for enabling the 5V to usb gadget
 * @enable: to enabling the Phy for device.
 *
 * This function used to set the voltage for USB gadget mode.
 */
static void usb_device_phy_en(int enable)
{
	int volt = 0;
	int ret = -1;

	if (phy_enable_stat == enable)
		return;

	if (enable == USB_ENABLE) {
		wake_lock(&ab8500_musb_wakelock);
		ux500_pins_enable(usb_gpio_pins);
		clk_enable(sysclock);
		phy_enable_stat = USB_ENABLE;
		regulator_enable(musb_vape_supply);
		regulator_enable(musb_smps2_supply);

		/* Set Vintcore12 LDO to 1.3V */
		ret = regulator_set_voltage(musb_vintcore_supply,
						1300000, 1350000);
		if (ret < 0)
			printk(KERN_ERR "Failed to set the Vintcore"
					" to 1.3V, ret=%d\n", ret);
		ret = regulator_set_optimum_mode(musb_vintcore_supply,
						 28000);
		if (ret < 0)
			printk(KERN_ERR "Failed to set optimum mode"
					" (ret=%d)\n", ret);
		regulator_enable(musb_vintcore_supply);
		volt = regulator_get_voltage(musb_vintcore_supply);
		if ((volt != 1300000) && (volt != 1350000))
			printk(KERN_ERR "Vintcore is not"
					" set to 1.3V"
					" volt=%d\n", volt);
#ifdef	CONFIG_PM
		stm_musb_context(USB_ENABLE);
#endif

		/* Workaround for USB performance issue. */
		schedule_delayed_work_on(0,
				 &work_usb_workaround,
				 msecs_to_jiffies(USB_PROBE_DELAY));

		prcmu_qos_update_requirement(PRCMU_QOS_APE_OPP,
				DEVICE_NAME, 100);

		abx500_set_register_interruptible(device,
				AB8500_USB,
				AB8500_USB_PHY_CTRL_REG,
				AB8500_USB_DEVICE_ENABLE);
	} else { /* enable == USB_DISABLE */
		/*
		 * Workaround: Sometimes the DISCONNECT interrupt is
		 * not generated in musb_core. Force a disconnect if
		 * necessary before we power down the PHY.
		 */
		stm_musb_force_disconnect();

		if (boot_time_flag)
			boot_time_flag = USB_DISABLE;

		/*
		 * Workaround for bug31952 in ABB cut2.0. Write 0x1
		 * before disabling the PHY.
		 */
		abx500_set_register_interruptible(device, AB8500_USB,
			     AB8500_USB_PHY_CTRL_REG,
			     AB8500_USB_DEVICE_ENABLE);

		udelay(200);

		abx500_set_register_interruptible(device,
			AB8500_USB,
			AB8500_USB_PHY_CTRL_REG,
			AB8500_USB_DEVICE_DISABLE);
		prcmu_qos_update_requirement(PRCMU_QOS_APE_OPP,
				DEVICE_NAME, 50);

		/* Workaround for USB performance issue. */
		cancel_delayed_work_sync(&work_usb_workaround);
		prcmu_qos_update_requirement(PRCMU_QOS_ARM_OPP,
					     "usb", 25);

		prcmu_release_usb_wakeup_state();
		phy_enable_stat = USB_DISABLE;
		regulator_disable(musb_vape_supply);
		regulator_disable(musb_smps2_supply);
		regulator_disable(musb_vintcore_supply);
		regulator_set_optimum_mode(musb_vintcore_supply, 0);
		/* Set Vintcore12 LDO to 0V to 1.35V */
		ret = regulator_set_voltage(musb_vintcore_supply,
						0000000, 1350000);
		if (ret < 0)
			printk(KERN_ERR "Failed to set the Vintcore"
					" to 0V to 1.35V,"
					" ret=%d\n", ret);
		clk_disable(sysclock);
#ifdef CONFIG_PM
		stm_musb_context(USB_DISABLE);
#endif
		ux500_pins_disable(usb_gpio_pins);
		wake_unlock(&ab8500_musb_wakelock);
	}
}

/**
 * usb_device_remove_handler() - remove the 5V to usb device
 *
 * This function used to remove the voltage for USB device mode.
 */
static irqreturn_t usb_device_remove_handler(int irq, void *data)
{
	if (stm_musb_curr_state == USB_DEVICE) {
		/* Change the current state */
		stm_musb_curr_state = USB_IDLE;
		queue_work(usb_cable_wq, &usb_device_remove);
		return IRQ_HANDLED;
	} else if (stm_musb_curr_state == USB_DEDICATED_CHG) {
		stm_musb_curr_state = USB_IDLE;
		if (ab8500_rev == AB8500_REV_20)
			queue_work(usb_cable_wq, &usb_dedicated_charger_remove);
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}


static  irqreturn_t  usb_device_insert_handler(int irq, void *data)
{

	schedule_delayed_work_on(0,
				 &work_usb_charger_det_enable,
				 msecs_to_jiffies(USB_LINK_STAT_DELAY));
	return IRQ_HANDLED;
}

/**
 * usb_link_status_update_handler() - USB Link status update complete
 *
 * This function is used to signal the completion of
 * USB Link status register update
 */
static irqreturn_t usb_link_status_update_handler(int irq, void *data)
{
	queue_work(usb_cable_wq, &usb_lnk_status_update);
	return IRQ_HANDLED;
}

#ifdef CONFIG_USB_OTG_20
static irqreturn_t irq_adp_plug_handler(int irq, void *data)
{
	int ret;

	ret = musb_adp();
	if (ret) {

		if (stm_musb_curr_state == USB_HOST)
			musb_set_session(1);
		if (stm_musb_curr_state == USB_DEVICE) {
			/*TODO*/
			/* Generate SRP */
		}
		if (stm_musb_curr_state == USB_IDLE)
			printk(KERN_INFO "No device is connected\n");
	}

	return IRQ_HANDLED;
}

static irqreturn_t irq_adp_unplug_handler(int irq, void *data)
{
	if (stm_musb_curr_state == USB_HOST) {
		stm_musb_curr_state = USB_IDLE;
		queue_work(usb_cable_wq, &usb_host_remove);
	}
	if (stm_musb_curr_state == USB_DEVICE) {
		stm_musb_curr_state = USB_IDLE;
		queue_work(usb_cable_wq, &usb_device_remove);
	}

	return IRQ_HANDLED;
}
#endif

/**
 * musb_phy_en : register USB callback handlers for ab8500
 * @mode: value for mode.
 *
 * This function is used to register USB callback handlers for ab8500.
 */
int musb_phy_en(u8 mode)
{
	int ret = -1;
	u8 save_val;

	if (!device)
		return -EINVAL;

	ab8500_rev = abx500_get_chip_id(device);
	if (ab8500_rev < 0) {
		dev_err(device, "get chip id failed\n");
		return ab8500_rev;
	}
	if (!((ab8500_rev == AB8500_REV_20)
	|| (ab8500_rev == AB8500_REV_30)
	|| (ab8500_rev == AB8500_REV_33))) {
		dev_err(device, "Unknown AB type!\n");
		return -ENODEV;
	}

	musb_vape_supply = regulator_get(device, "v-ape");
	if (IS_ERR(musb_vape_supply)) {
		dev_err(device, "Could not get %s:v-ape supply\n",
				dev_name(device));

		ret = PTR_ERR(musb_vape_supply);
		return ret;
	}
	musb_vintcore_supply = regulator_get(device, "v-intcore");
	if (IS_ERR(musb_vintcore_supply)) {
		dev_err(device, "Could not get %s:v-intcore12 supply\n",
				dev_name(device));

		ret = PTR_ERR(musb_vintcore_supply);
		return ret;
	}
	musb_smps2_supply = regulator_get(device, "musb_1v8");
	if (IS_ERR(musb_smps2_supply)) {
		dev_err(device, "Could not get %s:v-intcore12 supply\n",
				dev_name(device));

		ret = PTR_ERR(musb_smps2_supply);
		return ret;
	}
	sysclock = clk_get(device, "sysclk");
	if (IS_ERR(sysclock)) {
		ret = PTR_ERR(sysclock);
		sysclock = NULL;
		return ret;
	}

	/*
	 * When usb cable is not connected,set Qos for VAPE to 50.
	 * This is done to run APE at low OPP when usb is not used.
	 */
	prcmu_qos_add_requirement(PRCMU_QOS_APE_OPP, DEVICE_NAME, 50);

	if (mode == MUSB_HOST || mode == MUSB_OTG) {
		ret = request_threaded_irq(irq_host_remove, NULL,
			usb_host_remove_handler,
			IRQF_NO_SUSPEND | IRQF_SHARED,
			"usb-host-remove", device);
		if (ret < 0) {
			printk(KERN_ERR "failed to set the callback"
					" handler for usb host"
					" removal\n");
			return ret;
		}
	}
	if ((mode == MUSB_PERIPHERAL) || (mode == MUSB_OTG)) {
		ret = request_threaded_irq(irq_device_remove, NULL,
			usb_device_remove_handler,
			IRQF_NO_SUSPEND | IRQF_SHARED,
			"usb-device-remove", device);
		if (ret < 0) {
			printk(KERN_ERR "failed to set the callback"
					" handler for usb host"
					" removal\n");
			return ret;
		}
	}

	/* create a thread for work */
	usb_cable_wq = create_singlethread_workqueue(
			"usb_cable_wq");
	if (usb_cable_wq == NULL)
		return -ENOMEM;

	ret = request_threaded_irq(irq_device_insert, NULL,
			usb_device_insert_handler,
			IRQF_NO_SUSPEND | IRQF_SHARED,
			"usb-device-insert", device);
	if (ret < 0) {
		printk(KERN_ERR "failed to set the callback"
				" handler for usb device"
				" insert\n");
		return ret;
	}

	INIT_WORK(&usb_host_remove, usb_host_remove_work);
	INIT_WORK(&usb_device_remove, usb_device_remove_work);
	INIT_WORK(&usb_lnk_status_update,
			usb_link_status_update_work);

	if (ab8500_rev == AB8500_REV_20)
		INIT_WORK(&usb_dedicated_charger_remove,
			usb_dedicated_charger_remove_work);

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
#ifdef CONFIG_USB_OTG_20
	ret = request_threaded_irq(irq_adp_plug, NULL,
			irq_adp_plug_handler,
			IRQF_SHARED, "usb-adp-plug", device);
	if (ret < 0) {
		printk(KERN_ERR "failed to set the callback"
			" handler for usb adp"
			" plug\n");
		return ret;
	}
	ret = request_threaded_irq(irq_adp_unplug, NULL,
			irq_adp_unplug_handler,
			IRQF_SHARED, "usb-adp-unplug", device);
	if (ret < 0) {
		printk(KERN_ERR "failed to set the callback"
				" handler for usb adp"
				" unplug\n");
		return ret;
	}
#endif
	/* Write Phy tuning values */
	if ((ab8500_rev == AB8500_REV_30)
	|| (ab8500_rev == AB8500_REV_33)) {
		/* Enable the PBT/Bank 0x12 access
		 * Save old bank settings for
		 * later restore
		 */
		ret = abx500_get_register_interruptible(device,
							AB8500_DEVELOPMENT,
							AB8500_BANK12_ACCESS,
							&save_val);

		ret = abx500_set_register_interruptible(device,
						AB8500_DEVELOPMENT,
						AB8500_BANK12_ACCESS,
						0x01);
		if (ret < 0)
			printk(KERN_ERR "Failed to enable bank12"
					" access ret=%d\n", ret);

		ret = abx500_set_register_interruptible(device,
						AB8500_DEBUG,
						AB8500_USB_PHY_TUNE1,
						0xC8);
		if (ret < 0)
			printk(KERN_ERR "Failed to set PHY_TUNE1"
					" register ret=%d\n", ret);

		ret = abx500_set_register_interruptible(device,
						AB8500_DEBUG,
						AB8500_USB_PHY_TUNE2,
						0x00);
		if (ret < 0)
			printk(KERN_ERR "Failed to set PHY_TUNE2"
					" register ret=%d\n", ret);

		ret = abx500_set_register_interruptible(device,
						AB8500_DEBUG,
						AB8500_USB_PHY_TUNE3,
						0x78);
		if (ret < 0)
			printk(KERN_ERR "Failed to set PHY_TUNE3"
					" regester ret=%d\n", ret);

		/* Switch back to previous mode/disable Bank 0x12 access */
		ret = abx500_set_register_interruptible(device,
						AB8500_DEVELOPMENT,
						AB8500_BANK12_ACCESS,
						save_val);
		if (ret < 0)
			printk(KERN_ERR "Failed to switch bank12"
					" access ret=%d\n", ret);
	}
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
	u8 usb_status = 0;
	u8 val = 0;

	if (!device)
		return -EINVAL;

	/* Disabling PHY before selective enable or disable */
	abx500_set_register_interruptible(device,
			AB8500_USB,
			AB8500_USB_PHY_CTRL_REG,
			AB8500_USB_DEVICE_ENABLE);

	udelay(200);

	abx500_set_register_interruptible(device,
			AB8500_USB,
			AB8500_USB_PHY_CTRL_REG,
			AB8500_USB_DEVICE_DISABLE);

	abx500_set_register_interruptible(device,
			AB8500_USB,
			AB8500_USB_PHY_CTRL_REG,
			AB8500_USB_HOST_ENABLE);

	udelay(200);

	abx500_set_register_interruptible(device,
			AB8500_USB,
			AB8500_USB_PHY_CTRL_REG,
			AB8500_USB_HOST_DISABLE);

	if (mode == MUSB_HOST || mode == MUSB_OTG) {
		ret = abx500_get_register_interruptible(device,
			AB8500_INTERRUPT, AB8500_IT_SOURCE20_REG,
			&usb_status);
		if (ret < 0) {
			dev_err(device, "Read IT 20 failed\n");
			return ret;
		}

		if (usb_status & AB8500_SRC_INT_USB_HOST) {
			boot_time_flag = USB_ENABLE;
			/* Change the current state */
			stm_musb_curr_state = USB_HOST;
			usb_host_phy_en(USB_ENABLE);
		}
	}
	if (mode == MUSB_PERIPHERAL || mode == MUSB_OTG) {
		ret = abx500_get_register_interruptible(device,
			AB8500_INTERRUPT, AB8500_IT_SOURCE2_REG,
			&usb_status);
		if (ret < 0) {
			dev_err(device, "Read IT 2 failed\n");
			return ret;
		}

		if (usb_status & AB8500_SRC_INT_USB_DEVICE) {
			/* Check if it is a dedicated charger */
			(void)abx500_get_register_interruptible(device,
			AB8500_USB, AB8500_USB_LINE_STAT_REG, &val);

			val = (val & AB8500_USB_LINK_STATUS) >> 3;

			if (val == USB_LINK_DEDICATED_CHG) {
				/* Change the current state */
				stm_musb_curr_state = USB_DEDICATED_CHG;
			} else {
				boot_time_flag = USB_ENABLE;
				/* Change the current state */
				stm_musb_curr_state = USB_DEVICE;
				usb_device_phy_en(USB_ENABLE);
			}
		}
	}
	return 0;
}

/**
 * usb_host_remove_work : work handler for host cable insert.
 * @work: work structure
 *
 * This function is used to handle the host cable insert work.
 */
static void usb_host_remove_work(struct work_struct *work)
{
	usb_host_phy_en(USB_DISABLE);
}

/**
 * usb_device_remove_work : work handler for device cable insert.
 * @work: work structure
 *
 * This function is used to handle the device cable insert work.
 */
static void usb_device_remove_work(struct work_struct *work)
{
	usb_device_phy_en(USB_DISABLE);
}

/* Work created for PHY handling in case of dedicated charger disconnect */
static void usb_dedicated_charger_remove_work(struct work_struct *work)
{
	/*
	* Workaround for bug31952 in ABB cut2.0. Write 0x1
	* before disabling the PHY.
	*/
	abx500_set_register_interruptible(device, AB8500_USB,
				AB8500_USB_PHY_CTRL_REG,
				AB8500_USB_DEVICE_ENABLE);

	udelay(200);

	abx500_set_register_interruptible(device,
				AB8500_USB,
				AB8500_USB_PHY_CTRL_REG,
				AB8500_USB_DEVICE_DISABLE);
}

/* Work created after an link status update handler*/
static void usb_link_status_update_work(struct work_struct *work)
{
	u8 val = 0;

	(void)abx500_get_register_interruptible(device,
			AB8500_USB, AB8500_USB_LINE_STAT_REG, &val);

	val = (val & AB8500_USB_LINK_STATUS) >> 3;
	stm_usb_link_curr_state = (enum stm_musb_states) val;

	cancel_delayed_work_sync(&work_usb_charger_det_enable);

	switch (stm_usb_link_curr_state) {
	case USB_LINK_DEDICATED_CHG:
		stm_musb_curr_state = USB_DEDICATED_CHG;
		break;
	case USB_LINK_NOT_CONFIGURED:
	case USB_LINK_NOT_VALID_LINK:
#ifdef CONFIG_USB_OTG_NOTIFICATION
		send_aca_connection_event(OTG_EVENT_ACA_DISCONNECTED);
#endif
	case USB_LINK_ACA_RID_B:
		/* PHY is disabled */
		switch (stm_musb_curr_state) {
		case USB_DEVICE:
			/* Change the current state */
			stm_musb_curr_state = USB_IDLE;
			usb_device_phy_en(USB_DISABLE);
			break;
		case USB_HOST:
			/* Change the current state */
			stm_musb_curr_state = USB_IDLE;
			usb_host_phy_en(USB_DISABLE);
			break;
		case USB_IDLE:
		case USB_DEDICATED_CHG:
			break;
		}
		break;
	case USB_LINK_ACA_RID_C_NM:
	case USB_LINK_ACA_RID_C_HS:
	case USB_LINK_ACA_RID_C_HS_CHIRP:
#ifdef CONFIG_USB_OTG_NOTIFICATION
		send_aca_connection_event(OTG_EVENT_ACA_CONNECTED);
#endif
	case USB_LINK_STD_HOST_NC:
	case USB_LINK_STD_HOST_C_NS:
	case USB_LINK_STD_HOST_C_S:
	case USB_LINK_HOST_CHG_NM:
	case USB_LINK_HOST_CHG_HS:
	case USB_LINK_HOST_CHG_HS_CHIRP:
		/* Device PHY is enabled */
		switch (stm_musb_curr_state) {
		case USB_HOST:
			/* Change the current state */
			stm_musb_curr_state = USB_DEVICE;
			usb_host_phy_en(USB_DISABLE);
			usb_device_phy_en(USB_ENABLE);
			break;
		case USB_IDLE:
			/* Change the current state */
			stm_musb_curr_state = USB_DEVICE;
			usb_device_phy_en(USB_ENABLE);
			break;
		case USB_DEVICE:
		case USB_DEDICATED_CHG:
			break;
		}
		break;
	case USB_LINK_ACA_RID_A:
#ifdef CONFIG_USB_OTG_NOTIFICATION
		send_aca_connection_event(OTG_EVENT_ACA_CONNECTED);
#endif
	case USB_LINK_HM_IDGND:
		/* Host PHY is enabled */
		switch (stm_musb_curr_state) {
		case USB_DEVICE:
			/* Change the current state */
			stm_musb_curr_state = USB_HOST;
			usb_device_phy_en(USB_DISABLE);
			usb_host_phy_en(USB_ENABLE);
			if (stm_usb_link_curr_state == USB_LINK_HM_IDGND){
				musb_rid_a_wa(IDGND_WA);
			}
			if(stm_usb_link_curr_state == USB_LINK_ACA_RID_A){
				musb_rid_a_wa(RIDA_WA);
			}
			musb_set_session(1);
			break;
		case USB_IDLE:
			/* Change the current state */
			stm_musb_curr_state = USB_HOST;
			usb_host_phy_en(USB_ENABLE);
			if (stm_usb_link_curr_state == USB_LINK_HM_IDGND){
				musb_rid_a_wa(IDGND_WA);
			}
			if(stm_usb_link_curr_state == USB_LINK_ACA_RID_A){
				musb_rid_a_wa(RIDA_WA);
			}
			musb_set_session(1);
			break;
		case USB_HOST:
		case USB_DEDICATED_CHG:
			break;
		}
		break;
	case USB_LINK_OTG_HOST_NO_CURRENT:
	default:
		break;
	}
	wake_lock_timeout(&wakeup_wakelock, HZ / 2);
	stm_usb_link_prev_state = stm_usb_link_curr_state;
}

/*
 * musb_get_abx500_rev : Get the ABx500 revision number
 *
 * This function returns the ABx500 revision number.
 */
int musb_get_abx500_rev()
{
	return ab8500_rev;
}

static int __devinit ab8500_musb_probe(struct platform_device *pdev)
{
#ifdef CONFIG_USB_OTG_20
	u8 usb_status = 0;
	int ret;
#endif
	device = &pdev->dev;
	irq_host_remove = platform_get_irq_byname(pdev, "ID_WAKEUP_F");
	if (irq_host_remove < 0) {
		dev_err(&pdev->dev, "Host remove irq not found, err %d\n",
			irq_host_remove);
		return irq_host_remove;
	}

	irq_device_remove = platform_get_irq_byname(pdev, "VBUS_DET_F");
	if (irq_device_remove < 0) {
		dev_err(&pdev->dev, "Device remove irq not found, err %d\n",
			irq_device_remove);
		return irq_device_remove;
	}

	irq_device_insert = platform_get_irq_byname(pdev, "VBUS_DET_R");
	if (irq_device_insert < 0) {
		dev_err(&pdev->dev, "Device insert irq not found, err %d\n",
			irq_device_insert);
		return irq_device_insert;
	}

	irq_link_status_update = platform_get_irq_byname(pdev,
		"USB_LINK_STATUS");
	if (irq_link_status_update < 0) {
		dev_err(&pdev->dev, "USB Link status irq not found, err %d\n",
			irq_link_status_update);
		return irq_link_status_update;
	}

#ifdef CONFIG_USB_OTG_20
	enable_adp();
	irq_adp_plug = platform_get_irq_byname(pdev, "USB_ADP_PROBE_PLUG");
	if (irq_adp_plug < 0) {
		dev_err(&pdev->dev, "ADP Probe plug irq not found, err %d\n",
					irq_adp_plug);
		return irq_adp_plug;
	}
	irq_adp_unplug = platform_get_irq_byname(pdev, "USB_ADP_PROBE_UNPLUG");
	if (irq_adp_unplug < 0) {
			dev_err(&pdev->dev, "ADP Probe unplug irq not found,"
						" err %d\n",
						irq_adp_unplug);
		return irq_adp_unplug;
	}
	ret = abx500_get_register_interruptible(device,
				AB8500_INTERRUPT, AB8500_IT_SOURCE20_REG,
				&usb_status);
			if (ret < 0) {
				dev_err(device, "Read IT 2 failed\n");
				return ret;
			}

			if (usb_status & AB8500_USB_DEVICE_ENABLE) {
				boot_time_flag = USB_ENABLE;
				stm_musb_curr_state = USB_DEVICE;
				musb_phy_en(MUSB_HOST);
			}
#endif

	/* Aquire GPIO alternate config struct for USB */
	usb_gpio_pins = ux500_pins_get(dev_name(device));

	if (IS_ERR(usb_gpio_pins)) {
		dev_err(device, "Could not get %s:usb_gpio_pins structure\n",
				dev_name(device));

		return PTR_ERR(usb_gpio_pins);
	}

	INIT_DELAYED_WORK_DEFERRABLE(&work_usb_workaround,
				     usb_load);

	INIT_DELAYED_WORK_DEFERRABLE(&work_usb_charger_det_enable,
				     usb_charger_det_enable);
	prcmu_qos_add_requirement(PRCMU_QOS_ARM_OPP, "usb", 25);

	wake_lock_init(&ab8500_musb_wakelock, WAKE_LOCK_SUSPEND, "ab8500-usb");
	wake_lock_init(&wakeup_wakelock,
			WAKE_LOCK_SUSPEND, "ab8500-usb-wakeup");
	return 0;
}

static int __devexit ab8500_musb_remove(struct platform_device *pdev)
{
	ux500_pins_put(usb_gpio_pins);
	return 0;
}

static struct platform_driver ab8500_musb_driver = {
	.driver		= {
		.name	= "ab8500-usb",
		.owner	= THIS_MODULE,
	},
	.probe		= ab8500_musb_probe,
	.remove		= __devexit_p(ab8500_musb_remove),
};

static int __init ab8500_musb_init(void)
{
	return platform_driver_register(&ab8500_musb_driver);
}
module_init(ab8500_musb_init);

static void __exit ab8500_musb_exit(void)
{
	platform_driver_unregister(&ab8500_musb_driver);
}
module_exit(ab8500_musb_exit);

MODULE_LICENSE("GPL v2");
