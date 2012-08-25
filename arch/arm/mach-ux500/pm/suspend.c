/*
 * Copyright (C) STMicroelectronics 2009
 * Copyright (C) ST-Ericsson SA 2010-2011
 *
 * License Terms: GNU General Public License v2
 *
 * Authors: Rickard Andersson <rickard.andersson@stericsson.com>,
 *	    Jonas Aaberg <jonas.aberg@stericsson.com>,
 *          Sundar Iyer for ST-Ericsson.
 *
 */

#include <linux/suspend.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/regulator/ab8500-debug.h>

#include <mach/prcmu.h>
#include <mach/prcmu-regs.h>
#include <mach/prcmu-qos.h>
#include <mach/regulator.h>

#include "context.h"
#include "pm.h"
#include "suspend_dbg.h"

static void (*pins_suspend_force)(void);
static void (*pins_suspend_force_mux)(void);

void suspend_set_pins_force_fn(void (*force)(void), void (*force_mux)(void))
{
	pins_suspend_force = force;
	pins_suspend_force_mux = force_mux;
}

static atomic_t block_sleep = ATOMIC_INIT(0);

void suspend_block_sleep(void)
{
	atomic_inc(&block_sleep);
}

void suspend_unblock_sleep(void)
{
	atomic_dec(&block_sleep);
}

bool sleep_is_blocked(void)
{
	return (atomic_read(&block_sleep) != 0);
}

static int suspend(bool do_deepsleep)
{
	bool pins_force = pins_suspend_force_mux && pins_suspend_force;
	int ret = 0;

	if (sleep_is_blocked()) {
		pr_info("suspend/resume: interrupted by modem.\n");
		return -EBUSY;
	}

	nmk_gpio_clocks_enable();

	ux500_suspend_dbg_add_wake_on_uart();
	nmk_gpio_wakeups_suspend();

	/* configure the prcm for a sleep wakeup */
	prcmu_enable_wakeups(PRCMU_WAKEUP(ABB));

	context_vape_save();

	if (pins_force) {
		/*
		 * Save GPIO settings before applying power save
		 * settings
		 */
		context_gpio_save();

		/* Apply GPIO power save mux settings */
		context_gpio_mux_safe_switch(true);
		pins_suspend_force_mux();
		context_gpio_mux_safe_switch(false);

		/* Apply GPIO power save settings */
		pins_suspend_force();
	}

	ux500_pm_gic_decouple();

	if (ux500_pm_gic_pending_interrupt()) {
		pr_info("suspend/resume: pending interrupt\n");

		/* Recouple GIC with the interrupt bus */
		ux500_pm_gic_recouple();
		ret = -EBUSY;

		goto exit;
	}
	ux500_pm_prcmu_set_ioforce(true);

	if (do_deepsleep) {
		context_varm_save_common();
		context_varm_save_core();
		context_gic_dist_disable_unneeded_irqs();
		context_save_cpu_registers();

		/*
		 * Due to we have only 100us between requesting a powerstate
		 * and wfi, we clean the cache before as well to assure the
		 * final cache clean before wfi has as little as possible to
		 * do.
		 */
		context_clean_l1_cache_all();

		(void) prcmu_set_power_state(PRCMU_AP_DEEP_SLEEP,
					     false, false);
		context_save_to_sram_and_wfi(true);

		context_restore_cpu_registers();
		context_varm_restore_core();
		context_varm_restore_common();

	} else {

		context_clean_l1_cache_all();
		(void) prcmu_set_power_state(APEXECUTE_TO_APSLEEP,
					     false, false);
		dsb();
		__asm__ __volatile__("wfi\n\t" : : : "memory");
	}

	context_vape_restore();

	/* If GPIO woke us up then save the pins that caused the wake up */
	ux500_pm_gpio_save_wake_up_status();

	ux500_suspend_dbg_sleep_status(do_deepsleep);

	/* APE was turned off, restore IO ring */
	ux500_pm_prcmu_set_ioforce(false);

exit:
	if (pins_force) {
		/* Restore gpio settings */
		context_gpio_mux_safe_switch(true);
		context_gpio_restore_mux();
		context_gpio_mux_safe_switch(false);
		context_gpio_restore();
	}

	/* This is what cpuidle wants */
	prcmu_enable_wakeups(PRCMU_WAKEUP(ARM) | PRCMU_WAKEUP(RTC) |
			     PRCMU_WAKEUP(ABB));

	nmk_gpio_wakeups_resume();
	ux500_suspend_dbg_remove_wake_on_uart();

	nmk_gpio_clocks_disable();

	return ret;
}

static int ux500_suspend_enter(suspend_state_t state)
{

	if (ux500_suspend_enabled()) {
		if (ux500_suspend_deepsleep_enabled() &&
		    state == PM_SUSPEND_MEM)
			return suspend(true);
		if (ux500_suspend_sleep_enabled())
			return suspend(false);
	}

	ux500_suspend_dbg_add_wake_on_uart();
	/*
	 * Set IOFORCE in order to wake on GPIO the same way
	 * as in deeper sleep.
	 * (U5500 is not ready for IOFORCE)
	 */
	if (!cpu_is_u5500())
		ux500_pm_prcmu_set_ioforce(true);

	dsb();
	__asm__ __volatile__("wfi\n\t" : : : "memory");

	if (!cpu_is_u5500())
		ux500_pm_prcmu_set_ioforce(false);
	ux500_suspend_dbg_remove_wake_on_uart();

	return 0;
}

static int ux500_suspend_valid(suspend_state_t state)
{
	return state == PM_SUSPEND_MEM || state == PM_SUSPEND_STANDBY;
}

static int ux500_suspend_prepare_late(void)
{
	/* ESRAM to retention instead of OFF until ROM is fixed */
	(void)prcmu_config_esram0_deep_sleep(ESRAM0_DEEP_SLEEP_STATE_RET);
	ab8500_regulator_debug_force();
	ux500_regulator_suspend_debug();

	return 0;
}

static void ux500_suspend_wake(void)
{
	ux500_regulator_resume_debug();
	ab8500_regulator_debug_restore();
	(void)prcmu_config_esram0_deep_sleep(ESRAM0_DEEP_SLEEP_STATE_RET);
}

static int ux500_suspend_begin(suspend_state_t state)
{
	prcmu_qos_update_requirement(PRCMU_QOS_ARM_OPP,
				     "suspend", 100);
	return ux500_suspend_dbg_begin(state);
}

static void ux500_suspend_end(void)
{
	prcmu_qos_update_requirement(PRCMU_QOS_ARM_OPP,
				     "suspend", 25);
}

static struct platform_suspend_ops ux500_suspend_ops = {
	.enter	      = ux500_suspend_enter,
	.valid	      = ux500_suspend_valid,
	.prepare_late = ux500_suspend_prepare_late,
	.wake	      = ux500_suspend_wake,
	.begin	      = ux500_suspend_begin,
	.end	      = ux500_suspend_end,
};

static __init int ux500_suspend_init(void)
{
	ux500_suspend_dbg_init();

	prcmu_qos_add_requirement(PRCMU_QOS_ARM_OPP, "suspend", 25);

	suspend_set_ops(&ux500_suspend_ops);
	return 0;
}

device_initcall(ux500_suspend_init);
