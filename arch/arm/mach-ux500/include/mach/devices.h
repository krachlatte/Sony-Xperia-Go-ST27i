/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef __ASM_ARCH_DEVICES_H__
#define __ASM_ARCH_DEVICES_H__

struct platform_device;
struct amba_device;

extern struct platform_device u5500_gpio_devs[];
extern struct platform_device u8500_gpio_devs[];

extern struct platform_device ux500_mcde_device;
extern struct platform_device u8500_shrm_device;
extern struct platform_device ux500_b2r2_device;
extern struct platform_device u8500_trace_modem;
extern struct platform_device ux500_hwmem_device;
extern struct platform_device ux500_dma_device;
extern struct platform_device ux500_hash1_device;
extern struct platform_device ux500_cryp1_device;
extern struct platform_device ux500_musb_device;
extern struct platform_device u5500_pwm0_device;
extern struct platform_device u5500_pwm1_device;
extern struct platform_device u5500_pwm2_device;
extern struct platform_device u5500_pwm3_device;
extern struct platform_device ux500_wdt_device;
extern struct platform_device ux500_prcmu_wdt_device;
extern struct platform_device mloader_fw_device;
extern struct platform_device ux500_thsens_device;
extern struct platform_device ux500_stm_device;
extern struct platform_device ux500_mmio_device;
extern struct platform_device u8500_hsi_device;
#define ARRAY_AND_SIZE(x)	(x), ARRAY_SIZE(x)

/**
 * Touchpanel related macros declaration
 */
#define TOUCH_GPIO_PIN 	84

#define TOUCH_XMAX 384
#define TOUCH_YMAX 704

#define PRCMU_CLOCK_OCR 0x1CC
#define TSC_EXT_CLOCK_9_6MHZ 0x840000
#endif
