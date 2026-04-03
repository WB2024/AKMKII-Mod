/* arch/arm/mach-tcc893x/board-tcc8930st-keypad.c
 *
 * Copyright (C) 2011 Telechips, Inc.
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

#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_event.h>
#include <asm/gpio.h>
#include <asm/mach-types.h>
#include <mach/bsp.h>
#include <mach/io.h>

#include <mach/ak_gpio_def.h>
/*******************************************************************
		AK201 SWITCH JACK 
*******************************************************************/

static struct platform_device ak_gpio_switch_device = {
	.name ="ak-switch-gpio",
	.id = 0,
	.dev = {
		.platform_data = NULL,
	},
};

static int __init ak_gpio_jack_switch_init(void)
{
//	if (!machine_is_tcc893x())
//		return 0;

	platform_device_register(&ak_gpio_switch_device);


	return 0;
}

device_initcall(ak_gpio_jack_switch_init);
