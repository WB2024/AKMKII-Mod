/* linux/arch/arm/mach-tcc893x/board-tcc8930-pmic.c
 *
 * Copyright (C) 2012 Telechips, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/regulator/machine.h>
#include <mach/gpio.h>
#include <mach/bsp.h>
#include <asm/mach-types.h>
#include "devices.h"
#include <mach/board-tcc8930.h>

#if defined(CONFIG_REGULATOR)
#if defined(CONFIG_REGULATOR_ACT8840)
#include <linux/regulator/act8840.h>

static struct regulator_init_data act8840_dcdc1_info = {
	.constraints = {
		.name = "vdd_dcdc1 range",
		.min_uV =  800000,
		.max_uV = 3900000,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data act8840_dcdc2_info = {
	.constraints = {
		.name = "vdd_dcdc2 range",
		.min_uV =  700000,
		.max_uV = 3900000,
		.always_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data act8840_dcdc3_info = {
	.constraints = {
		.name = "vdd_dcdc3 range",
		.min_uV =  700000,
		.max_uV = 3900000,
		.always_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data act8840_dcdc4_info = {
	.constraints = {
		.name = "vdd_dcdc4 range",
		.min_uV = 1000000,
		.max_uV = 3300000,
		.always_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,        
//		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data act8840_ldo5_info = {
	.constraints = {
		.name = "vdd_ldo5 range",
		.min_uV = 3300000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data act8840_ldo6_info = {
	.constraints = {
		.name = "vdd_ldo6 range",
		.min_uV = 1800000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data act8840_ldo7_info = {
	.constraints = {
		.name = "vdd_ldo7 range",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data act8840_ldo8_info = {
	.constraints = {
		.name = "vdd_ldo8 range",
		.min_uV = 3300000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};


static struct regulator_init_data act8840_ldo9_info = {
	.constraints = {
		.name = "vdd_ldo9 range",
		.min_uV = 1200000,
		.max_uV = 1200000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data act8840_ldo10_info = {
	.constraints = {
		.name = "vdd_ldo10 range",
		.min_uV = 3300000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data act8840_ldo11_info = {
	.constraints = {
		.name = "vdd_ldo11 range",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data act8840_ldo12_info = {
	.constraints = {
		.name = "vdd_ldo12 range",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct act8840_subdev_data act8840_subdev[] = {
	{
		.id   = ACT8840_ID_DCDC1,
		.platform_data = &act8840_dcdc1_info,
	},
	{
		.id   = ACT8840_ID_DCDC2,
		.platform_data = &act8840_dcdc2_info,
	},
	{
		.id   = ACT8840_ID_DCDC3,
		.platform_data = &act8840_dcdc3_info,
	},
	{
		.id   = ACT8840_ID_DCDC4,
		.platform_data = &act8840_dcdc4_info,
	},
	{
		.id   = ACT8840_ID_LDO5,
		.platform_data = &act8840_ldo5_info,
	},
	{
		.id   = ACT8840_ID_LDO6,
		.platform_data = &act8840_ldo6_info,
	},
	{
		.id   = ACT8840_ID_LDO7,
		.platform_data = &act8840_ldo7_info,
	},
	{
		.id   = ACT8840_ID_LDO8,
		.platform_data = &act8840_ldo8_info,
	},
	{
		.id   = ACT8840_ID_LDO9,
		.platform_data = &act8840_ldo9_info,
	},
	{
		.id   = ACT8840_ID_LDO10,
		.platform_data = &act8840_ldo10_info,
	},
	{
		.id   = ACT8840_ID_LDO11,
		.platform_data = &act8840_ldo11_info,
	},
	{
		.id   = ACT8840_ID_LDO12,
		.platform_data = &act8840_ldo12_info,
	},

};

static int act8840_irq_init(void)
{

#if 0
	if(system_rev == 0x1000){
		tcc_gpio_config(TCC_GPE(29), GPIO_FN(0)|GPIO_PULL_DISABLE);  // GPIOE[31]: input mode, disable pull-up/down
		tcc_gpio_config_ext_intr(PMIC_IRQ, EXTINT_GPIOE_29);

		gpio_request(TCC_GPE(29), "PMIC_IRQ");
		gpio_direction_input(TCC_GPE(29));
	}else if(system_rev == 0x2000 || system_rev == 0x3000){
		tcc_gpio_config(TCC_GPE(27), GPIO_FN(0)|GPIO_PULL_DISABLE);  // GPIOE[31]: input mode, disable pull-up/down
		tcc_gpio_config_ext_intr(PMIC_IRQ, EXTINT_GPIOE_27);

		gpio_request(TCC_GPE(27), "PMIC_IRQ");
		gpio_direction_input(TCC_GPE(27));
	}
#endif 	

	return 0;
}

static struct act8840_platform_data act8840_info = {
	.num_subdevs = ARRAY_SIZE(act8840_subdev),
	.subdevs     = act8840_subdev,
	.init_irq    = act8840_irq_init,
};
#endif



#if defined(CONFIG_REGULATOR_AXP192)
#include <linux/regulator/axp192.h>

static struct regulator_init_data axp192_dcdc1_info = {
	.constraints = {
		.name = "vdd_dcdc1 range",
		.min_uV =  700000,
		.max_uV = 3500000,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE|REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data axp192_dcdc2_info = {
	.constraints = {
		.name = "vdd_dcdc2 range",
		.min_uV =  700000,
		.max_uV = 2275000,
		.always_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data axp192_dcdc3_info = {
	.constraints = {
		.name = "vdd_dcdc3 range",
		.min_uV =  700000,
		.max_uV = 3500000,
		.always_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data axp192_ldo2_info = {
	.constraints = {
		.name = "vdd_ldo2 range",
		.min_uV = 1800000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data axp192_ldo3_info = {
	.constraints = {
		.name = "vdd_ldo3 range",
		.min_uV = 1800000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data axp192_ldo4_info = {
	.constraints = {
		.name = "vdd_ldo4 range",
		.min_uV = 1800000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct axp192_subdev_data axp192_subdev[] = {
	{
		.id   = AXP192_ID_DCDC1,
		.platform_data = &axp192_dcdc1_info,
	},
	{
		.id   = AXP192_ID_DCDC2,
		.platform_data = &axp192_dcdc2_info,
	},
	{
		.id   = AXP192_ID_DCDC3,
		.platform_data = &axp192_dcdc3_info,
	},
	{
		.id   = AXP192_ID_LDO2,
		.platform_data = &axp192_ldo2_info,
	},
	{
		.id   = AXP192_ID_LDO3,
		.platform_data = &axp192_ldo3_info,
	},
	{
		.id   = AXP192_ID_LDO4,
		.platform_data = &axp192_ldo4_info,
	},
};

static int axp192_irq_init(void)
{

	if(system_rev == 0x1000){
		tcc_gpio_config(TCC_GPE(29), GPIO_FN(0)|GPIO_PULL_DISABLE);  // GPIOE[31]: input mode, disable pull-up/down
		tcc_gpio_config_ext_intr(PMIC_IRQ, EXTINT_GPIOE_29);

		gpio_request(TCC_GPE(29), "PMIC_IRQ");
		gpio_direction_input(TCC_GPE(29));
	}else if(system_rev == 0x2000 || system_rev == 0x3000){
		tcc_gpio_config(TCC_GPE(27), GPIO_FN(0)|GPIO_PULL_DISABLE);  // GPIOE[31]: input mode, disable pull-up/down
		tcc_gpio_config_ext_intr(PMIC_IRQ, EXTINT_GPIOE_27);

		gpio_request(TCC_GPE(27), "PMIC_IRQ");
		gpio_direction_input(TCC_GPE(27));
	}
	return 0;
}

static struct axp192_platform_data axp192_info = {
	.num_subdevs = ARRAY_SIZE(axp192_subdev),
	.subdevs     = axp192_subdev,
	.init_irq    = axp192_irq_init,
};
#endif


#if defined(CONFIG_REGULATOR_RT5027)
#include <linux/regulator/rt5027.h>

static struct regulator_init_data rt5027_dcdc1_info = {
	.constraints = {
		.name = "vdd_dcdc1 range",
		.min_uV =  700000,
		.max_uV = 2275000,
		.always_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rt5027_dcdc2_info = {
	.constraints = {
		.name = "vdd_dcdc2 range",
		.min_uV =  700000,
		.max_uV = 3500000,
		.always_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rt5027_dcdc3_info = {
	.constraints = {
		.name = "vdd_dcdc3 range",
		.min_uV =  700000,
		.max_uV = 3500000,
		.always_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rt5027_ldo1_info = {
	.constraints = {
		.name = "vdd_ldo1 range",
		.min_uV = 1500000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rt5027_ldo2_info = {
	.constraints = {
		.name = "vdd_ldo2 range",
		.min_uV = 700000,
		.max_uV = 3500000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rt5027_ldo3_info = {
	.constraints = {
		.name = "vdd_ldo3 range",
		.min_uV =  700000,
		.max_uV = 3500000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rt5027_ldo4_info = {
	.constraints = {
		.name = "vdd_ldo4 range",
		.min_uV = 1000000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rt5027_ldo5_info = {
	.constraints = {
		.name = "vdd_ldo5 range",
		.min_uV = 1000000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rt5027_ldo6_info = {
	.constraints = {
		.name = "vdd_ldo6 range",
		.min_uV = 1000000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rt5027_ldo7_info = {
	.constraints = {
		.name = "vdd_ldo7 range",
		.min_uV = 1000000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rt5027_ldo8_info = {
	.constraints = {
		.name = "vdd_ldo8 range",
		.min_uV = 1000000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct rt5027_subdev_data rt5027_subdev[] = {
	{
		.id   = RT5027_ID_DCDC1,
		.platform_data = &rt5027_dcdc1_info,
	},
	{
		.id   = RT5027_ID_DCDC2,
		.platform_data = &rt5027_dcdc2_info,
	},
	{
		.id   = RT5027_ID_DCDC3,
		.platform_data = &rt5027_dcdc3_info,
	},
	{
		.id   = RT5027_ID_LDO1,
		.platform_data = &rt5027_ldo1_info,
	},
	{
		.id   = RT5027_ID_LDO2,
		.platform_data = &rt5027_ldo2_info,
	},
	{
		.id   = RT5027_ID_LDO3,
		.platform_data = &rt5027_ldo3_info,
	},
	{
		.id   = RT5027_ID_LDO4,
		.platform_data = &rt5027_ldo4_info,
	},
	{
		.id   = RT5027_ID_LDO5,
		.platform_data = &rt5027_ldo5_info,
	},
	{
		.id   = RT5027_ID_LDO6,
		.platform_data = &rt5027_ldo6_info,
	},
	{
		.id   = RT5027_ID_LDO7,
		.platform_data = &rt5027_ldo7_info,
	},
	{
		.id   = RT5027_ID_LDO8,
		.platform_data = &rt5027_ldo8_info,
	},
};

static int rt5027_irq_init(void)
{

	if(system_rev == 0x1000){
		tcc_gpio_config(TCC_GPE(29), GPIO_FN(0)|GPIO_PULL_DISABLE);  // GPIOE[31]: input mode, disable pull-up/down
		tcc_gpio_config_ext_intr(PMIC_IRQ, EXTINT_GPIOE_29);

		gpio_request(TCC_GPE(29), "PMIC_IRQ");
		gpio_direction_input(TCC_GPE(29));
	}else if(system_rev == 0x2000 || system_rev == 0x3000){
		tcc_gpio_config(TCC_GPE(27), GPIO_FN(0)|GPIO_PULL_DISABLE);  // GPIOE[31]: input mode, disable pull-up/down
		tcc_gpio_config_ext_intr(PMIC_IRQ, EXTINT_GPIOE_27);

		gpio_request(TCC_GPE(27), "PMIC_IRQ");
		gpio_direction_input(TCC_GPE(27));
	}
	return 0;
}

static struct rt5027_platform_data rt5027_info = {
	.num_subdevs = ARRAY_SIZE(rt5027_subdev),
	.subdevs     = rt5027_subdev,
	.init_irq    = rt5027_irq_init,
};
#endif

#if defined(CONFIG_REGULATOR_RN5T614)
#include <linux/regulator/rn5t614.h>

static struct regulator_init_data rn5t614_dcdc1_info = {
	.constraints = {
		.name = "vdd_dcdc1 range",
		.min_uV =  950000,
		.max_uV = 1500000,
		.always_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rn5t614_dcdc2_info = {
	.constraints = {
		.name = "vdd_dcdc2 range",
		.min_uV =  950000,
		.max_uV = 1500000,
		.always_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rn5t614_dcdc3_info = {
	.constraints = {
		.name = "vdd_dcdc3 range",
		.min_uV = 1800000,
		.max_uV = 3300000,
		.always_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rn5t614_ldo2_info = {
	.constraints = {
		.name = "vdd_ldo2 range",
		.min_uV =  900000,
		.max_uV = 1300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rn5t614_ldo3_info = {
	.constraints = {
		.name = "vdd_ldo3 range",
		.min_uV =  900000,
		.max_uV = 1300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rn5t614_ldo4_info = {
	.constraints = {
		.name = "vdd_ldo4 range",
		.min_uV = 1800000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rn5t614_ldo5_info = {
	.constraints = {
		.name = "vdd_ldo5 range",
		.min_uV = 1800000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rn5t614_ldo6_info = {
	.constraints = {
		.name = "vdd_ldo6 range",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rn5t614_ldo7_info = {
	.constraints = {
		.name = "vdd_ldo7 range",
		.min_uV = 1200000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct regulator_init_data rn5t614_ldo8_info = {
	.constraints = {
		.name = "vdd_ldo8 range",
		.min_uV = 1800000,
		.max_uV = 3300000,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = 0,
};

static struct rn5t614_subdev_data rn5t614_subdev[] = {
	{
		.id   = RN5T614_ID_DCDC1,
		.platform_data = &rn5t614_dcdc1_info,
	},
	{
		.id   = RN5T614_ID_DCDC2,
		.platform_data = &rn5t614_dcdc2_info,
	},
	{
		.id   = RN5T614_ID_DCDC3,
		.platform_data = &rn5t614_dcdc3_info,
	},
	{
		.id   = RN5T614_ID_LDO2,
		.platform_data = &rn5t614_ldo2_info,
	},
	{
		.id   = RN5T614_ID_LDO3,
		.platform_data = &rn5t614_ldo3_info,
	},
	{
		.id   = RN5T614_ID_LDO4,
		.platform_data = &rn5t614_ldo4_info,
	},
	{
		.id   = RN5T614_ID_LDO5,
		.platform_data = &rn5t614_ldo5_info,
	},
	{
		.id   = RN5T614_ID_LDO6,
		.platform_data = &rn5t614_ldo6_info,
	},
	{
		.id   = RN5T614_ID_LDO7,
		.platform_data = &rn5t614_ldo7_info,
	},
	{
		.id   = RN5T614_ID_LDO8,
		.platform_data = &rn5t614_ldo8_info,
	},
};

static int rn5t614_port_init(int irq_num)
{
	if(system_rev == 0x1000){
		tcc_gpio_config(TCC_GPE(29), GPIO_FN(0)|GPIO_PULL_DISABLE);  // GPIOE[31]: input mode, disable pull-up/down
		tcc_gpio_config_ext_intr(PMIC_IRQ, EXTINT_GPIOE_29);

		gpio_request(TCC_GPE(29), "PMIC_IRQ");
		gpio_direction_input(TCC_GPE(29));
	}else if(system_rev == 0x2000 || system_rev == 0x3000){
		tcc_gpio_config(TCC_GPE(27), GPIO_FN(0)|GPIO_PULL_DISABLE);  // GPIOE[31]: input mode, disable pull-up/down
		tcc_gpio_config_ext_intr(PMIC_IRQ, EXTINT_GPIOE_27);

		gpio_request(TCC_GPE(27), "PMIC_IRQ");
		gpio_direction_input(TCC_GPE(27));
	}
	return 0;
}

static struct rn5t614_platform_data rn5t614_info = {
	.num_subdevs   = ARRAY_SIZE(rn5t614_subdev),
	.subdevs       = rn5t614_subdev,
	.init_port     = rn5t614_port_init,
};
#endif

static struct i2c_board_info __initdata i2c_pmic_devices[] = {

#if defined(CONFIG_REGULATOR_ACT8840)
	{
		I2C_BOARD_INFO("act8840", 0x5A),
		.irq           = PMIC_IRQ,
		.platform_data = &act8840_info,
	},
#endif

#if defined(CONFIG_REGULATOR_AXP192)
	{
		I2C_BOARD_INFO("axp192", 0x34),
		.irq           = PMIC_IRQ,
		.platform_data = &axp192_info,
	},
#endif
#if defined(CONFIG_REGULATOR_RT5027)
	{
		I2C_BOARD_INFO("rt5027", 0x34),
		.irq           = PMIC_IRQ,
		.platform_data = &rt5027_info,
	},
#endif
#if defined(CONFIG_REGULATOR_RN5T614)
	{
		I2C_BOARD_INFO("rn5t614", 0x32),
		.irq           = PMIC_IRQ,
		.platform_data = &rn5t614_info,
	},
#endif
};

static struct regulator_consumer_supply consumer_cpu = {
	.supply = "vdd_cpu",
};

static struct regulator_consumer_supply consumer_core = {
	.supply = "vdd_core",
};

static struct regulator_consumer_supply consumer_io = {
	.supply = "vdd_io",
};

static struct regulator_consumer_supply consumer_mem = {
	.supply = "vdd_mem",
};

static struct regulator_consumer_supply consumer_ldo5  = {
	.supply = "vdd_ldo5_osc_1v8",
};

static struct regulator_consumer_supply consumer_ldo6 = {
	.supply = "vdd_ldo6_1v8",
};

static struct regulator_consumer_supply consumer_ldo7  = {
	.supply = "vdd_ldo7_movi_1v8",
};

static struct regulator_consumer_supply consumer_ldo8 = {
	.supply = "vdd_ldo8_wifi_3v3",
};

static struct regulator_consumer_supply consumer_ldo9  = {
	.supply = "vdd_ldo9_usb_1v2",
};

static struct regulator_consumer_supply consumer_ldo10 = {
	.supply = "vdd_ldo10_usb_3v3",
};

static struct regulator_consumer_supply consumer_ldo11  = {
	.supply = "vdd_ldo11_sd30_3v3",
};

static struct regulator_consumer_supply consumer_ldo12 = {
	.supply = "vdd_ldo12_1v8",
};

#endif

#if defined(CONFIG_BATTERY_MAX17040)
static struct i2c_board_info __initdata i2c_battery_devices[] = {

	{
		I2C_BOARD_INFO("max17040", 0x36) //0110110
		//.platform_data = &max17040_data,
	},
};
#endif

#if defined(CONFIG_EXT_BATTERY_MAX17040)
static struct i2c_board_info __initdata i2c_ext_battery_devices[] = {

	{
		I2C_BOARD_INFO("max17040_ext", 0x36) //0110110
	},
};
#endif

//iriver-ysyim
#if defined(CONFIG_LCD_N070ICN)  || defined(CONFIG_LCD_AKJR2DISP)
static struct i2c_board_info __initdata i2c_rgb2mipi_devices[] = {

	{
		I2C_BOARD_INFO("tc358768xbg", 0x07) //(slave address=7"b0000_111X)
	},
};
#endif
void __init tcc8930_init_pmic(void)
{
//	if (!machine_is_tcc893x())
		//
//		return;

	
#if defined(CONFIG_REGULATOR_ACT8840)
	act8840_dcdc1_info.num_consumer_supplies = 1;
	act8840_dcdc1_info.consumer_supplies = &consumer_cpu;

	act8840_dcdc2_info.num_consumer_supplies = 1;
	act8840_dcdc2_info.consumer_supplies = &consumer_core;	

	act8840_dcdc3_info.num_consumer_supplies = 1;
	act8840_dcdc3_info.consumer_supplies = &consumer_io;

	act8840_dcdc4_info.num_consumer_supplies = 1;
	act8840_dcdc4_info.consumer_supplies = &consumer_mem;
    
	act8840_ldo5_info.num_consumer_supplies = 1;
	act8840_ldo5_info.consumer_supplies = &consumer_ldo5;

	act8840_ldo6_info.num_consumer_supplies = 1;
	act8840_ldo6_info.consumer_supplies = &consumer_ldo6;
 	
	act8840_ldo7_info.num_consumer_supplies = 1;
	act8840_ldo7_info.consumer_supplies = &consumer_ldo7;
	
	act8840_ldo8_info.num_consumer_supplies = 1;
	act8840_ldo8_info.consumer_supplies = &consumer_ldo8;

	act8840_ldo9_info.num_consumer_supplies = 1;
	act8840_ldo9_info.consumer_supplies = &consumer_ldo9;

	act8840_ldo10_info.num_consumer_supplies = 1;
	act8840_ldo10_info.consumer_supplies = &consumer_ldo10;
 	
	act8840_ldo11_info.num_consumer_supplies = 1;
	act8840_ldo11_info.consumer_supplies = &consumer_ldo11;
	
	act8840_ldo12_info.num_consumer_supplies = 1;
	act8840_ldo12_info.consumer_supplies = &consumer_ldo12;
	
#endif

#if defined(CONFIG_REGULATOR_AXP192)
	axp192_dcdc1_info.num_consumer_supplies = 1;
	axp192_dcdc1_info.consumer_supplies = &consumer_cpu;
	axp192_dcdc2_info.num_consumer_supplies = 1;
	axp192_dcdc2_info.consumer_supplies = &consumer_core;
#endif

#if defined(CONFIG_REGULATOR_RT5027)
	rt5027_dcdc1_info.num_consumer_supplies = 1;
	rt5027_dcdc1_info.consumer_supplies = &consumer_cpu;
	rt5027_dcdc2_info.num_consumer_supplies = 1;
	rt5027_dcdc2_info.consumer_supplies = &consumer_core;
#endif


#if defined(CONFIG_REGULATOR)
	i2c_register_board_info(2, i2c_pmic_devices, ARRAY_SIZE(i2c_pmic_devices));
#endif

#if defined(CONFIG_BATTERY_MAX17040)
	i2c_register_board_info(2, i2c_battery_devices, ARRAY_SIZE(i2c_battery_devices));
#endif

#if defined(CONFIG_EXT_BATTERY_MAX17040)
	i2c_register_board_info(0, i2c_ext_battery_devices, ARRAY_SIZE(i2c_ext_battery_devices));
#endif

#if defined(CONFIG_LCD_N070ICN)
	i2c_register_board_info(3, i2c_rgb2mipi_devices, ARRAY_SIZE(i2c_rgb2mipi_devices));
#endif

#if defined(CONFIG_LCD_AKJR2DISP)
	i2c_register_board_info(2, i2c_rgb2mipi_devices, ARRAY_SIZE(i2c_rgb2mipi_devices));
#endif

}

//device_initcall(tcc8930_init_pmic);
