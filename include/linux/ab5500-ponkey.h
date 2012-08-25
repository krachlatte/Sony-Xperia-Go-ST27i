/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 * Author: Sundar Iyer <sundar.iyer@stericsson.com> for ST-Ericsson
 *
 * AB8500 Power-On Key handler
 */

#ifndef __AB8500_PONKEY_H__
#define __AB8500_PONKEY_H__

#include <linux/platform_device.h>

#ifdef CONFIG_INPUT_AB8500_FORCECRASH
extern int ab8500_forcecrash_init(struct platform_device *pdev);
extern void ab8500_forcecrash_exit(struct platform_device *pdev);
extern void ab5500_forced_key_detect(int);
extern void ab8500_forced_key_combo_detect(void);
#endif

#endif /*__AB8500_PONKEY_H__*/
