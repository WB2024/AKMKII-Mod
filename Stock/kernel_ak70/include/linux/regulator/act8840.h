/*
 * act8840.h  --  Eight Channel ActivePath Power Management IC
 *
 * Copyright (C) 2011 by Telechips
 *
 * This program is free software
 *
 */

#ifndef REGULATOR_ACT8840
#define REGULATOR_ACT8840

#include <linux/regulator/machine.h>

enum {
	ACT8840_ID_DCDC1 = 0,
	ACT8840_ID_DCDC2,
	ACT8840_ID_DCDC3,
	ACT8840_ID_DCDC4,  //VDD_1V5_MEM
	ACT8840_ID_LDO5,
	ACT8840_ID_LDO6,
	ACT8840_ID_LDO7,
	ACT8840_ID_LDO8,
	ACT8840_ID_LDO9,
	ACT8840_ID_LDO10,
	ACT8840_ID_LDO11,
	ACT8840_ID_LDO12,
	ACT8840_ID_MAX
};

/**
 * ACT8840_subdev_data - regulator data
 * @id: regulator Id
 * @name: regulator cute name (example for V3: "vcc_core")
 * @platform_data: regulator init data (constraints, supplies, ...)
 */
struct act8840_subdev_data {
	int                         id;
	char                        *name;
	struct regulator_init_data  *platform_data;
};

/**
 * act8840_platform_data - platform data for act8840
 * @num_subdevs: number of regulators used (may be 1 or 2)
 * @subdevs: regulator used
 *           At most, there will be a regulator for V3 and one for V6 voltages.
 * @init_irq: main chip irq initialize setting.
 */
struct act8840_platform_data {
	int num_subdevs;
	struct act8840_subdev_data *subdevs;
	int (*init_irq)(int irq_num);
};

#endif
