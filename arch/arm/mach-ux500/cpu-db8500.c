/*
 * Copyright (C) 2008-2009 ST-Ericsson SA
 *
 * Author: Srinidhi KASAGAR <srinidhi.kasagar@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 */
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/amba/bus.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/interrupt.h>

#include <asm/pmu.h>
#include <asm/mach/map.h>
#include <mach/hardware.h>
#include <mach/setup.h>
#include <mach/devices.h>
#include <mach/prcmu.h>
#include <mach/reboot_reasons.h>
#include <mach/dbx500-reset-reasons.h>

static struct resource db8500_pmu_resources[] = {
	[0] = {
		.start		= IRQ_DB8500_PMU,
		.end		= IRQ_DB8500_PMU,
		.flags		= IORESOURCE_IRQ,
	},
};

/*
 * The PMU IRQ lines of two cores are wired together into a single interrupt.
 * Bounce the interrupt to the other core if it's not ours.
 */
static irqreturn_t db8500_pmu_handler(int irq, void *dev, irq_handler_t handler)
{
	irqreturn_t ret = handler(irq, dev);
	int other = !smp_processor_id();

	if (ret == IRQ_NONE && cpu_online(other))
		irq_set_affinity(irq, cpumask_of(other));

	/*
	 * We should be able to get away with the amount of IRQ_NONEs we give,
	 * while still having the spurious IRQ detection code kick in if the
	 * interrupt really starts hitting spuriously.
	 */
	return ret;
}

static struct arm_pmu_platdata db8500_pmu_platdata = {
	.handle_irq		= db8500_pmu_handler,
};

static struct platform_device db8500_pmu_device = {
	.name			= "arm-pmu",
	.id			= ARM_PMU_DEVICE_CPU,
	.num_resources		= ARRAY_SIZE(db8500_pmu_resources),
	.resource		= db8500_pmu_resources,
	.dev.platform_data	= &db8500_pmu_platdata,
};

static struct platform_device *platform_devs[] __initdata = {
	&u8500_gpio_devs[0],
	&u8500_gpio_devs[1],
	&u8500_gpio_devs[2],
	&u8500_gpio_devs[3],
	&u8500_gpio_devs[4],
	&u8500_gpio_devs[5],
	&u8500_gpio_devs[6],
	&u8500_gpio_devs[7],
	&u8500_gpio_devs[8],
	&ux500_wdt_device,
	&ux500_prcmu_wdt_device,
	&db8500_pmu_device,
};

#include "devices-db8500.h"

static struct map_desc u8500_io_desc[] __initdata = {
	__IO_DEV_DESC(U8500_MTU0_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_MTU1_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_SCU_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_TWD_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_UART0_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_UART1_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_UART2_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_RTC_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_MSP0_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_MSP1_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_MSP2_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_PRCMU_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_GPIO0_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_GPIO1_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_GPIO2_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_GPIO3_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_PRCMU_BASE, SZ_8K),
	__IO_DEV_DESC(U8500_STM_REG_BASE, SZ_4K),
	{IO_ADDRESS(U8500_BACKUPRAM0_BASE),
		__phys_to_pfn(U8500_BACKUPRAM0_BASE), SZ_8K, MT_BACKUP_RAM},
	__MEM_DEV_DESC(U8500_BOOT_ROM_BASE, SZ_1M),
	__IO_DEV_DESC(U8500_CLKRST1_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_CLKRST2_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_CLKRST3_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_CLKRST5_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_CLKRST6_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_GIC_CPU_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_GIC_DIST_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_L2CC_BASE, SZ_4K),
	__IO_DEV_DESC(U8500_PRCMU_TCDM_BASE, SZ_4K),
};

struct reboot_reason reboot_reasons_hw[] = {
	{"modem",	HW_RESET_MODEM},
	{"ape-restart",	HW_RESET_APE_RESTART},
	{"a9-restart",	HW_RESET_A9_RESTART},
	{"por",		HW_RESET_POR}, /* Normal Boot */
	{"secure-wd",	HW_RESET_SECURE_WD},
	{"ape",		HW_RESET_APE},
	{"ape-sw",	HW_RESET_APE_SOFTWARE},
	{"a9-cpu1-wd",	HW_RESET_A9_CPU1_WD},
	{"a9-cpu0-wd",	HW_RESET_A9_CPU0_WD},
};

unsigned int reboot_reasons_hw_size = ARRAY_SIZE(reboot_reasons_hw);

/*
 * Functions to differentiate between later ASICs
 * We look into the end of the ROM to locate the hardcoded ASIC ID.
 * This is only needed to differentiate between minor revisions and
 * process variants of an ASIC, the major revisions are encoded in
 * the cpuid.
 */

#define U8500_ASIC_ID_LOC_ED_V1 (U8500_BOOT_ROM_BASE + 0x1FFF4)
#define U8500_ASIC_ID_LOC_V2    (U8500_BOOT_ROM_BASE + 0x1DBF4)
#define U8500_ASIC_REV_ED	0x01
#define U8500_ASIC_REV_V10	0xA0
#define U8500_ASIC_REV_V11	0xA1
#define U8500_ASIC_REV_V20	0xB0
#define U8500_ASIC_REV_V21	0xB1
#define U8500_ASIC_REV_V22	0xB2
#define U8500_ASIC_REV_V2B	0xBB
#define U8500_ASIC_REV_V2C	0xBC

static void __init get_db8500_asic_id(void)
{
	u32 asicid;

	if (!cpu_is_u8500v20_or_later())
		asicid = readl(__io_address(U8500_ASIC_ID_LOC_ED_V1));
	else
		asicid = readl(__io_address(U8500_ASIC_ID_LOC_V2));

	dbx500_id.process = (asicid >> 24);
	dbx500_id.partnumber = (asicid >> 16) & 0xFFFFU;
	dbx500_id.revision = asicid & 0xFFU;
}

bool cpu_is_u8500v20(void)
{
	return (dbx500_id.revision == U8500_ASIC_REV_V20);
}
EXPORT_SYMBOL(cpu_is_u8500v20);

bool cpu_is_u8500v21(void)
{
	return (dbx500_id.revision == U8500_ASIC_REV_V21);
}
EXPORT_SYMBOL(cpu_is_u8500v21);

bool cpu_is_u8500v22(void)
{
	return (dbx500_id.revision == U8500_ASIC_REV_V22);
}
EXPORT_SYMBOL(cpu_is_u8500v22);

bool cpu_is_u8500v2B(void)
{
	return (dbx500_id.revision == U8500_ASIC_REV_V2B);
}
EXPORT_SYMBOL(cpu_is_u8500v2B);

bool cpu_is_u8500v2C(void)
{
	return (dbx500_id.revision == U8500_ASIC_REV_V2C);
}
EXPORT_SYMBOL(cpu_is_u8500v2C);

void __init u8500_map_io(void)
{

	iotable_init(u8500_io_desc, ARRAY_SIZE(u8500_io_desc));

	/* Read out the ASIC ID as early as we can */
	get_db8500_asic_id();

	/* Display some ASIC boilerplate */
	pr_info("DB8500: process: %02x, revision ID: 0x%02x\n",
		dbx500_id.process, dbx500_id.revision);
	if (!cpu_is_u8500v20_or_later())
		panic("DB8500: hw version is too old and not supported\n");
	else if (cpu_is_u8500v20())
		pr_info("DB8500: version 2.0\n");
	else if (cpu_is_u8500v21())
		pr_info("DB8500: version 2.1\n");
	else if (cpu_is_u8500v22())
		pr_info("DB8500: version 2.2\n");
	else if (cpu_is_u8500v2B())
		pr_info("DB8500: version 2.2B\n");
	else if (cpu_is_u8500v2C())
		pr_info("DB8500: version 2.2C\n");
	else
		pr_err("ASIC: UNKNOWN SILICON VERSION!\n");

	_PRCMU_BASE = __io_address(U8500_PRCMU_BASE);
}

/*
 * This function is called from the board init
 */
void __init u8500_init_devices(void)
{
	struct amba_device *dev;

	ux500_init_devices();

	db8500_dma_init();

	dev = db8500_add_rtc();
	if (!IS_ERR(dev))
		device_init_wakeup(&dev->dev, true);

	/* Register the platform devices */
	platform_add_devices(platform_devs, ARRAY_SIZE(platform_devs));

}
