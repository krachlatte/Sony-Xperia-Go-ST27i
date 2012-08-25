/*
 * Copyright (C) 2009 ST-Ericsson.
 *
 * U8500 hardware definitions
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __MACH_HARDWARE_H
#define __MACH_HARDWARE_H

/*
 * Macros to get at IO space when running virtually
 * We dont map all the peripherals, let ioremap do
 * this for us. We map only very basic peripherals here.
 */
#define U8500_IO_VIRTUAL	0xf0000000
#define U8500_IO_PHYSICAL	0xa0000000

/* This macro is used in assembly, so no cast */
#define IO_ADDRESS(x)           \
	(((x) & 0x0fffffff) + (((x) >> 4) & 0x0f000000) + U8500_IO_VIRTUAL)

/* typesafe io address */
#define __io_address(n)		__io(IO_ADDRESS(n))
/* Used by some plat-nomadik code */
#define io_p2v(n)		__io_address(n)

#include <mach/db8500-regs.h>
#include <mach/db5500-regs.h>

/* ST-Ericsson modified pl022 id */
#define SSP_PER_ID                      0x01080022
#define SSP_PER_MASK                    0x0fffffff

/* SSP specific declaration */
#define SPI_PER_ID                      0x00080023
#define SPI_PER_MASK                    0x0fffffff

/* MSP specific declaration */
#define MSP_PER_ID			0x00280021
#define MSP_PER_MASK			0x00ffffff

/* DMA specific declaration */
#define DMA_PER_ID			0x8A280080
#define DMA_PER_MASK			0xffffffff

#define GPIO_PER_ID                     0x1f380060
#define GPIO_PER_MASK                   0xffffffff

/* RTC specific declaration */
#define RTC_PER_ID                      0x00280031
#define RTC_PER_MASK                    0x00ffffff

/* SDMMC specific declarations */
#define SDI_PER_ID			0x00480180
#define SDI_PER_MASK			0x00ffffff

/*
 * FIFO offsets for IPs
 */
#define I2C_TX_REG_OFFSET	0x10
#define I2C_RX_REG_OFFSET	0x18
#define UART_TX_RX_REG_OFFSET	0
#define MSP_TX_RX_REG_OFFSET	0
#define SSP_TX_RX_REG_OFFSET	0x8
#define SPI_TX_RX_REG_OFFSET	0x8
#define SD_MMC_TX_RX_REG_OFFSET 0x80
#define CRYP1_RX_REG_OFFSET	0x10
#define CRYP1_TX_REG_OFFSET	0x8
#define HASH1_TX_REG_OFFSET	0x4

#define MSP_0_CONTROLLER 1
#define MSP_1_CONTROLLER 2
#define MSP_2_CONTROLLER 3
#define MSP_3_CONTROLLER 4

#define SSP_0_CONTROLLER 4
#define SSP_1_CONTROLLER 5

#define SPI023_0_CONTROLLER 6
#define SPI023_1_CONTROLLER 7
#define SPI023_2_CONTROLLER 8
#define SPI023_3_CONTROLLER 9

#ifndef __ASSEMBLY__

extern void __iomem *_PRCMU_BASE;

#include <asm/cputype.h>
#include <asm/mach-types.h>

#include <linux/kernel.h>

static inline bool ux500_is_svp(void)
{
	return machine_is_svp5500();
}

static inline bool cpu_is_u8500(void)
{
#ifdef CONFIG_UX500_SOC_DB8500
       return 1;
#else
       return 0;
#endif
}

static inline bool cpu_is_u5500(void)
{
#ifdef CONFIG_UX500_SOC_DB5500
       return 1;
#else
       return 0;
#endif
}

#ifdef CONFIG_UX500_SOC_DB5500
bool cpu_is_u5500v1(void);
bool cpu_is_u5500v2(void);
#else
static inline bool cpu_is_u5500v1(void) { return false; }
static inline bool cpu_is_u5500v2(void) { return false; }
#endif

#ifdef CONFIG_UX500_SOC_DB8500
bool cpu_is_u8500v20(void);
bool cpu_is_u8500v21(void);
bool cpu_is_u8500v22(void);
bool cpu_is_u8500v2B(void);
bool cpu_is_u8500v2C(void);

static inline bool cpu_is_u8500v20_or_later(void)
{
	/* Only v2+ is supported */
	return cpu_is_u8500();
}
#else
static inline bool cpu_is_u8500v20(void) { return false; }
static inline bool cpu_is_u8500v21(void) { return false; }
static inline bool cpu_is_u8500v22(void) { return false; }
static inline bool cpu_is_u8500v2B(void) { return false; }
static inline bool cpu_is_u8500v2C(void) { return false; }
static inline bool cpu_is_u8500v20_or_later(void) { return false; }
#endif

/* Keep this greppable for SoC porters */
static inline void ux500_unknown_soc(void)
{
	pr_err("Unsupported db version!\n");
	BUG();
}
#endif				/* __ASSEMBLY__ */

#endif				/* __MACH_HARDWARE_H */
