/*
 * Copyright (C) 2010 ST-Ericsson SA
 * Licensed under GPLv2.
 *
 * Author: Vijaya Kumar K <vijay.kilari@stericsson.com>
 */

#ifndef	_AB5500_GPADC_H
#define _AB5500_GPADC_H

#include <linux/mfd/abx500.h>

/*
 * Frequency of auto adc conversion
 */
#define MS1000		0x0
#define MS500		0x1
#define MS200		0x2
#define MS100		0x3
#define MS10		0x4

struct ab5500_gpadc;

/*
 * struct adc_auto_input - AB5500 GPADC auto trigger
 * @adc_mux                     Mux input
 * @freq                        freq of conversion
 * @min                         min value for trigger
 * @max                         max value for trigger
 * @auto_adc_callback           notification callback
 */
struct adc_auto_input {
	u8 mux;
	u8 freq;
	int min;
	int max;
	int (*auto_adc_callback)(int mux);
};

#ifdef CONFIG_AB5500_GPADC

struct ab5500_gpadc *ab5500_gpadc_get(const char *name);
int ab5500_gpadc_convert(struct ab5500_gpadc *gpadc, u8 input);
int ab5500_gpadc_convert_auto(struct ab5500_gpadc *gpadc,
			struct adc_auto_input *auto_input);

#else /* CONFIG_AB5500_GPADC */

inline struct ab5500_gpadc *ab5500_gpadc_get(const char *name)
{
	return 0;
}

inline int ab5500_gpadc_convert(struct ab5500_gpadc *gpadc,
				u8 input)
{
	return 0;
}

inline int ab5500_gpadc_convert_auto(struct ab5500_gpadc *gpadc,
				struct adc_auto_input *auto_input)
{
	return 0;
}

#endif /* CONFIG_AB5500_GPADC */

#endif /* _AB5500_GPADC_H */
