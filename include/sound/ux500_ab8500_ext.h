/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Ola Lilja <ola.o.lilja@stericsson.com>
 *         for ST-Ericsson.
 *
 * License terms:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef UX500_AB8500_EXT_H
#define UX500_AB8500_EXT_H

#include <linux/mfd/ab8500/ab8500-gpadc.h>

int ux500_ab8500_audio_gpadc_measure(struct ab8500_gpadc *gpadc,
		u8 channel, bool mode, int *value);

void ux500_ab8500_audio_pwm_vibra(unsigned char speed_left_pos,
			unsigned char speed_left_neg,
			unsigned char speed_right_pos,
			unsigned char speed_right_neg);

#endif
