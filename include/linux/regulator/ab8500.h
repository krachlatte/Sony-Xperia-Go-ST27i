/*
 * Copyright (C) ST-Ericsson SA 2010
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * License Terms: GNU General Public License v2
 *
 * Authors: Sundar Iyer <sundar.iyer@stericsson.com> for ST-Ericsson
 *          Bengt Jonsson <bengt.g.jonsson@stericsson.com> for ST-Ericsson
 */

#ifndef __LINUX_MFD_AB8500_REGULATOR_H
#define __LINUX_MFD_AB8500_REGULATOR_H

#include <linux/platform_device.h>

/* AB8500 regulators */
enum ab8500_regulator_id {
	AB8500_LDO_AUX1,
	AB8500_LDO_AUX2,
	AB8500_LDO_AUX3,
	AB8500_LDO_INTCORE,
	AB8500_LDO_TVOUT,
	AB8500_LDO_AUDIO,
	AB8500_LDO_ANAMIC1,
	AB8500_LDO_ANAMIC2,
	AB8500_LDO_DMIC,
	AB8500_LDO_ANA,
	AB8500_SYSCLKREQ_2,
	AB8500_SYSCLKREQ_4,
	AB8500_SYSCLKREQ_6,
	AB8500_NUM_REGULATORS,
};

/* AB8500 register initialization */
struct ab8500_regulator_reg_init {
	int id;
	u8 mask;
	u8 value;
};

#define INIT_REGULATOR_REGISTER(_id, _mask, _value)	\
	{						\
		.id = _id,				\
		.mask = _mask,				\
		.value = _value,			\
	}

/* AB8500 registers */
enum ab8500_regulator_reg {
	AB8500_REGUREQUESTCTRL1,
	AB8500_REGUREQUESTCTRL2,
	AB8500_REGUREQUESTCTRL3,
	AB8500_REGUREQUESTCTRL4,
	AB8500_REGUSYSCLKREQ1HPVALID1,
	AB8500_REGUSYSCLKREQ1HPVALID2,
	AB8500_REGUHWHPREQ1VALID1,
	AB8500_REGUHWHPREQ1VALID2,
	AB8500_REGUHWHPREQ2VALID1,
	AB8500_REGUHWHPREQ2VALID2,
	AB8500_REGUSWHPREQVALID1,
	AB8500_REGUSWHPREQVALID2,
	AB8500_REGUSYSCLKREQVALID1,
	AB8500_REGUSYSCLKREQVALID2,
	AB8500_REGUMISC1,
	AB8500_VAUDIOSUPPLY,
	AB8500_REGUCTRL1VAMIC,
	AB8500_VSMPS1REGU,
	AB8500_VSMPS2REGU,
	AB8500_VPLLVANAREGU,
	AB8500_VREFDDR,
	AB8500_EXTSUPPLYREGU,
	AB8500_VAUX12REGU,
	AB8500_VRF1VAUX3REGU,
	AB8500_VSMPS1SEL1,
	AB8500_VSMPS1SEL2,
	AB8500_VSMPS1SEL3,
	AB8500_VSMPS2SEL1,
	AB8500_VSMPS2SEL2,
	AB8500_VSMPS2SEL3,
	AB8500_VAUX1SEL,
	AB8500_VAUX2SEL,
	AB8500_VRF1VAUX3SEL,
	AB8500_REGUCTRL2SPARE,
	AB8500_REGUCTRLDISCH,
	AB8500_REGUCTRLDISCH2,
	AB8500_NUM_REGULATOR_REGISTERS,
};

/* AB8500 external regulators */
enum ab8500_ext_regulator_id {
	AB8500_EXT_SUPPLY3,
	AB8500_NUM_EXT_REGULATORS,
};

struct ab8500_regulator_platform_data {
	int num_reg_init;
	struct ab8500_regulator_reg_init *reg_init;
	int num_regulator;
	struct regulator_init_data *regulator;
	int num_ext_regulator;
	struct regulator_init_data *ext_regulator;
};

#ifdef CONFIG_REGULATOR_AB8500_EXT
__devinit int ab8500_ext_regulator_init(struct platform_device *pdev);
__devexit int ab8500_ext_regulator_exit(struct platform_device *pdev);
#else
inline __devinit int ab8500_ext_regulator_init(struct platform_device *pdev) {}
inline __devexit int ab8500_ext_regulator_exit(struct platform_device *pdev) {}
#endif

#endif
