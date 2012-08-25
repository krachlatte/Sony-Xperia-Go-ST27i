/*
 * Copyright (C) 2009 ST-Ericsson.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * These symbols are needed for board-specific files to call their
 * own cpu-specific files
 */
#ifndef __ASM_ARCH_SETUP_H
#define __ASM_ARCH_SETUP_H

#include <asm/mach/time.h>
#include <linux/init.h>

struct sys_timer;
struct amba_device;

extern void __init u5500_map_io(void);
extern void __init u8500_map_io(void);

extern void __init ux500_init_devices(void);
extern void __init u5500_init_devices(void);
extern void __init u8500_init_devices(void);

extern void __init ux500_init_irq(void);

extern void __init amba_add_devices(struct amba_device *devs[], int num);

struct sys_timer;
extern struct sys_timer u8500_timer;
extern void __init db5500_dma_init(void);
extern void __init db8500_dma_init(void);

extern void __init u8500v2_msp_fixup(void);
extern void __iomem *gic_cpu_base_addr;

#define __IO_DEV_DESC(x, sz)	{		\
	.virtual	= IO_ADDRESS(x),	\
	.pfn		= __phys_to_pfn(x),	\
	.length		= sz,			\
	.type		= MT_DEVICE,		\
}

#define __MEM_DEV_DESC(x, sz)	{		\
	.virtual	= IO_ADDRESS(x),	\
	.pfn		= __phys_to_pfn(x),	\
	.length		= sz,			\
	.type		= MT_MEMORY,		\
}

/**
 * struct dbx500_asic_id - fields of the ASIC ID
 * @process: the manufacturing process, 0x40 is 40 nm
 *  0x00 is "standard"
 * @partnumber: hithereto 0xx500 for DBx500
 * @revision: version code in the series
 * This field definion is not formally defined but makes
 * sense.
 */
struct dbx500_asic_id {
	u8 process;
	u16 partnumber;
	u8 revision;
};

/* This isn't going to change at runtime */
extern struct dbx500_asic_id dbx500_id;

#endif /*  __ASM_ARCH_SETUP_H */
