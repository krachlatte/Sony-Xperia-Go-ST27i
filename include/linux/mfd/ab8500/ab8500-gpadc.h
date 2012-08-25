/*
 * Copyright (C) 2010 ST-Ericsson SA
 * Licensed under GPLv2.
 *
 * Author: Arun R Murthy <arun.murthy@stericsson.com>
 * Author: Daniel Willerud <daniel.willerud@stericsson.com>
 */

#ifndef	_AB8500_GPADC_H
#define _AB8500_GPADC_H

#include <linux/mfd/abx500.h>

struct ab8500_gpadc;

#ifdef CONFIG_AB8500_GPADC

struct ab8500_gpadc *ab8500_gpadc_get(void);
int ab8500_gpadc_convert(struct ab8500_gpadc *gpadc, u8 channel);
int ab8500_gpadc_read_raw(struct ab8500_gpadc *gpadc, u8 channel);
int ab8500_gpadc_ad_to_voltage(struct ab8500_gpadc *gpadc,
    u8 channel, int ad_value);

#else /* CONFIG_AB8500_GPADC */

inline struct ab8500_gpadc *ab8500_gpadc_get(void)
{
	return 0;
}

int ab8500_gpadc_convert(struct ab8500_gpadc *gpadc, u8 channel)
{
	return 0;
}

int ab8500_gpadc_read_raw(struct ab8500_gpadc *gpadc, u8 channel)
{
	return 0;
}

int ab8500_gpadc_ad_to_voltage(struct ab8500_gpadc *gpadc,
			u8 channel, int ad_value)
{
	return 0;
}

#endif /* CONFIG_AB8500_GPADC */

#endif /* _AB8500_GPADC_H */
