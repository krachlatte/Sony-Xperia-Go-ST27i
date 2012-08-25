/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 *
 * U8500 specific PRCMU API.
 */
#ifndef __MACH_PRCMU_DB8500_H
#define __MACH_PRCMU_DB8500_H

#include <linux/bitops.h>

/*
 * Registers
 */

#define DB8500_PRCM_GPIOCR 0x138
#define DB8500_PRCM_GPIOCR_SPI2_SELECT BIT(23)

#define DB8500_PRCM_DSI_SW_RESET 0x324
#define DB8500_PRCM_DSI_SW_RESET_DSI0_SW_RESETN BIT(0)
#define DB8500_PRCM_DSI_SW_RESET_DSI1_SW_RESETN BIT(1)
#define DB8500_PRCM_DSI_SW_RESET_DSI2_SW_RESETN BIT(2)

#define DB8500_PRCM_DSI_GLITCHFREE_EN 0x534
#define DB8500_PRCM_DSI_GLITCHFREE_EN_DSI0_BYTE_CLK BIT(0)
#define DB8500_PRCM_DSI_GLITCHFREE_EN_DSI1_BYTE_CLK BIT(8)
#define DB8500_PRCM_DSI_GLITCHFREE_EN_DSI2_BYTE_CLK BIT(16)

/*
 * Definitions for autonomous power management configuration.
 */

#define PRCMU_AUTO_PM_OFF 0
#define PRCMU_AUTO_PM_ON 1

#define PRCMU_AUTO_PM_POWER_ON_HSEM BIT(0)
#define PRCMU_AUTO_PM_POWER_ON_ABB_FIFO_IT BIT(1)

enum prcmu_auto_pm_policy {
	PRCMU_AUTO_PM_POLICY_NO_CHANGE,
	PRCMU_AUTO_PM_POLICY_DSP_OFF_HWP_OFF,
	PRCMU_AUTO_PM_POLICY_DSP_OFF_RAMRET_HWP_OFF,
	PRCMU_AUTO_PM_POLICY_DSP_CLK_OFF_HWP_OFF,
	PRCMU_AUTO_PM_POLICY_DSP_CLK_OFF_HWP_CLK_OFF,
};

/**
 * struct prcmu_auto_pm_config - Autonomous power management configuration.
 * @sia_auto_pm_enable: SIA autonomous pm enable. (PRCMU_AUTO_PM_{OFF,ON})
 * @sia_power_on:       SIA power ON enable. (PRCMU_AUTO_PM_POWER_ON_* bitmask)
 * @sia_policy:         SIA power policy. (enum prcmu_auto_pm_policy)
 * @sva_auto_pm_enable: SVA autonomous pm enable. (PRCMU_AUTO_PM_{OFF,ON})
 * @sva_power_on:       SVA power ON enable. (PRCMU_AUTO_PM_POWER_ON_* bitmask)
 * @sva_policy:         SVA power policy. (enum prcmu_auto_pm_policy)
 */
struct prcmu_auto_pm_config {
	u8 sia_auto_pm_enable;
	u8 sia_power_on;
	u8 sia_policy;
	u8 sva_auto_pm_enable;
	u8 sva_power_on;
	u8 sva_policy;
};

/**
 * enum hw_acc_dev - enum for hw accelerators
 * @HW_ACC_SVAMMDSP: for SVAMMDSP
 * @HW_ACC_SVAPIPE:  for SVAPIPE
 * @HW_ACC_SIAMMDSP: for SIAMMDSP
 * @HW_ACC_SIAPIPE: for SIAPIPE
 * @HW_ACC_SGA: for SGA
 * @HW_ACC_B2R2: for B2R2
 * @HW_ACC_MCDE: for MCDE
 * @HW_ACC_ESRAM1: for ESRAM1
 * @HW_ACC_ESRAM2: for ESRAM2
 * @HW_ACC_ESRAM3: for ESRAM3
 * @HW_ACC_ESRAM4: for ESRAM4
 * @NUM_HW_ACC: number of hardware accelerators
 *
 * Different hw accelerators which can be turned ON/
 * OFF or put into retention (MMDSPs and ESRAMs).
 * Used with EPOD API.
 *
 * NOTE! Deprecated, to be removed when all users switched over to use the
 * regulator API.
 */
enum hw_acc_dev{
	HW_ACC_SVAMMDSP,
	HW_ACC_SVAPIPE,
	HW_ACC_SIAMMDSP,
	HW_ACC_SIAPIPE,
	HW_ACC_SGA,
	HW_ACC_B2R2,
	HW_ACC_MCDE,
	HW_ACC_ESRAM1,
	HW_ACC_ESRAM2,
	HW_ACC_ESRAM3,
	HW_ACC_ESRAM4,
	NUM_HW_ACC
};

/**
 * enum hw_acc_state - State definition for hardware accelerator
 * @HW_NO_CHANGE: The hardware accelerator state must remain unchanged
 * @HW_OFF: The hardware accelerator must be switched off
 * @HW_OFF_RAMRET: The hardware accelerator must be switched off with its
 *               internal RAM in retention
 * @HW_ON: The hwa hardware accelerator hwa must be switched on
 *
 * NOTE! Deprecated, to be removed when all users switched over to use the
 * regulator API.
 */
enum hw_acc_state {
	HW_NO_CHANGE = 0x00,
	HW_OFF = 0x01,
	HW_OFF_RAMRET = 0x02,
	HW_ON = 0x04
};

/**
 * enum romcode_write - Romcode message written by A9 AND read by XP70
 * @RDY_2_DS: Value set when ApDeepSleep state can be executed by XP70
 * @RDY_2_XP70_RST: Value set when 0x0F has been successfully polled by the
 *                 romcode. The xp70 will go into self-reset
 */
enum romcode_write {
	RDY_2_DS = 0x09,
	RDY_2_XP70_RST = 0x10
};

/**
 * enum romcode_read - Romcode message written by XP70 and read by A9
 * @INIT: Init value when romcode field is not used
 * @FS_2_DS: Value set when power state is going from ApExecute to
 *          ApDeepSleep
 * @END_DS: Value set when ApDeepSleep power state is reached coming from
 *         ApExecute state
 * @DS_TO_FS: Value set when power state is going from ApDeepSleep to
 *           ApExecute
 * @END_FS: Value set when ApExecute power state is reached coming from
 *         ApDeepSleep state
 * @SWR: Value set when power state is going to ApReset
 * @END_SWR: Value set when the xp70 finished executing ApReset actions and
 *          waits for romcode acknowledgment to go to self-reset
 */
enum romcode_read {
	INIT = 0x00,
	FS_2_DS = 0x0A,
	END_DS = 0x0B,
	DS_TO_FS = 0x0C,
	END_FS = 0x0D,
	SWR = 0x0E,
	END_SWR = 0x0F
};

/**
 * enum ap_pwrst - current power states defined in PRCMU firmware
 * @NO_PWRST: Current power state init
 * @AP_BOOT: Current power state is apBoot
 * @AP_EXECUTE: Current power state is apExecute
 * @AP_DEEP_SLEEP: Current power state is apDeepSleep
 * @AP_SLEEP: Current power state is apSleep
 * @AP_IDLE: Current power state is apIdle
 * @AP_RESET: Current power state is apReset
 */
enum ap_pwrst {
	NO_PWRST = 0x00,
	AP_BOOT = 0x01,
	AP_EXECUTE = 0x02,
	AP_DEEP_SLEEP = 0x03,
	AP_SLEEP = 0x04,
	AP_IDLE = 0x05,
	AP_RESET = 0x06
};

#ifdef CONFIG_U8500_PRCMU

bool prcmu_is_u8400(void);

int prcmu_request_ape_opp_100_voltage(bool enable);

int prcmu_release_usb_wakeup_state(void);

void prcmu_configure_auto_pm(struct prcmu_auto_pm_config *sleep,
	struct prcmu_auto_pm_config *idle);
bool prcmu_is_auto_pm_enabled(void);

/* NOTE! Use regulator framework instead */
int prcmu_set_hwacc(u16 hw_acc_dev, u8 state);

/* TODO: Check if anyone is using these. */
int prcmu_set_rc_a2p(enum romcode_write);
enum romcode_read prcmu_get_rc_p2a(void);
enum ap_pwrst prcmu_get_xp70_current_state(void);

/* TODO: Common API with DB5500? */
bool prcmu_has_arm_maxopp(void);

int prcmu_config_a9wdog(u8 num, bool sleep_auto_off);
int prcmu_enable_a9wdog(u8 id);
int prcmu_disable_a9wdog(u8 id);
int prcmu_kick_a9wdog(u8 id);
int prcmu_load_a9wdog(u8 id, u32 val);

u32 prcmu_read(unsigned int reg);
void prcmu_write(unsigned int reg, u32 value);
void prcmu_write_masked(unsigned int reg, u32 mask, u32 value);
void prcmu_set_comm_timeout(unsigned long timeout_mS);
void prcmu_temp_set_comm_timeout(unsigned long timeout_mS,
	unsigned long validfor_mS);

#else /* !CONFIG_U8500_PRCMU */

static inline bool prcmu_is_u8400(void)
{
	return false;
}

static inline int prcmu_request_ape_opp_100_voltage(bool enable)
{
	return 0;
}

static inline int prcmu_release_usb_wakeup_state(void)
{
	return 0;
}

static inline void prcmu_configure_auto_pm(struct prcmu_auto_pm_config *sleep,
	struct prcmu_auto_pm_config *idle)
{
}

static inline bool prcmu_is_auto_pm_enabled(void)
{
	return false;
}

static inline int prcmu_set_hwacc(u16 hw_acc_dev, u8 state)
{
	return 0;
}

static inline int prcmu_set_rc_a2p(enum romcode_write code)
{
	return 0;
}

static inline enum romcode_read prcmu_get_rc_p2a(void)
{
	return INIT;
}

static inline enum ap_pwrst prcmu_get_xp70_current_state(void)
{
	return AP_EXECUTE;
}

static inline bool prcmu_has_arm_maxopp(void)
{
	return false;
}

static inline int prcmu_config_a9wdog(u8 num, bool sleep_auto_off)
{
	return 0;
}

static inline int prcmu_enable_a9wdog(u8 id)
{
	return 0;
}

static inline int prcmu_disable_a9wdog(u8 id)
{
	return 0;
}

static inline int prcmu_kick_a9wdog(u8 id)
{
	return 0;
}

static inline int prcmu_load_a9wdog(u8 id, u32 val)
{
	return 0;
}

static inline u32 prcmu_read(unsigned int reg)
{
	return 0;
}

static inline void prcmu_write(unsigned int reg, u32 value)
{
}

static inline void prcmu_write_masked(unsigned int reg, u32 mask, u32 value)
{
}

static inline void prcmu_set_comm_timeout(unsigned long comm_tout)
{
}

void prcmu_temp_set_comm_timeout(unsigned long timeout,
	unsigned long validfor)
{
}

#endif /* CONFIG_U8500_PRCMU */

static inline void prcmu_set(unsigned int reg, u32 bits)
{
	prcmu_write_masked(reg, bits, bits);
}

static inline void prcmu_clear(unsigned int reg, u32 bits)
{
	prcmu_write_masked(reg, bits, 0);
}

/**
 * prcmu_enable_spi2 - Enables pin muxing for SPI2 on OtherAlternateC1.
 */
static inline void prcmu_enable_spi2(void)
{
	prcmu_set(DB8500_PRCM_GPIOCR, DB8500_PRCM_GPIOCR_SPI2_SELECT);
}

/**
 * prcmu_disable_spi2 - Disables pin muxing for SPI2 on OtherAlternateC1.
 */
static inline void prcmu_disable_spi2(void)
{
	prcmu_clear(DB8500_PRCM_GPIOCR, DB8500_PRCM_GPIOCR_SPI2_SELECT);
}

#endif /* __MACH_PRCMU_DB8500_H */
