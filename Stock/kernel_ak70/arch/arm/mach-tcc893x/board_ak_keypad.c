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
#include <linux/gpio_keys.h>
#include <linux/gpio_event.h>
#include <linux/rotary_encoder.h>
#include <asm/gpio.h>
#include <asm/mach-types.h>
#include <mach/bsp.h>
#include <mach/io.h>

#include <mach/ak_gpio_def.h>

#if defined(CONFIG_KEYBOARD_GPIO)
static struct gpio_keys_button ak240_gpio_keys_buttons[] = {
    {
        .code           = KEY_POWER,
        .gpio           = GPIO_PWR_ON_OFF,
        .desc           = "power-key",
        .active_low     = 0,
        .type           = EV_KEY,
        .wakeup         = 1,
        .can_disable    = 1,
#if 1 //jimi.test.1230
	.debounce_interval = 30,
#endif
    },
};

static struct gpio_keys_platform_data ak240_gpio_keys = {
    .buttons    = ak240_gpio_keys_buttons,
    .nbuttons   = ARRAY_SIZE(ak240_gpio_keys_buttons),
};

static struct platform_device ak240_gpio_pwr_key_device = {
    .name       = "gpio-keys",
    .id         = 0,
    .dev        = {
        .platform_data = &ak240_gpio_keys,
    }
};
#endif

#if defined(CONFIG_INPUT_GPIO)
/*******************************************************************
		AK240 GPIO KEY
*******************************************************************/
static const struct gpio_event_direct_entry ak240_gpio_keymap[] = {
#if (CUR_AK == MODEL_AK500N)

#else
	#if (CUR_AK_REV ==  AK380_HW_ES )
	{GPIO_KEY_BACK, KEY_HOMEPAGE}, //ysyim
	#else
	{GPIO_KEY_BACK, KEY_PREVIOUSSONG},
	#endif
	{GPIO_KEY_NEXT, KEY_NEXTSONG},
//	{GPIO_HOME_KEY, KEY_HOMEPAGE},
	{GPIO_KEY_PLAY_STOP, KEY_PLAYPAUSE}
#endif
};

static struct gpio_event_input_info ak240_gpio_key_input_info = {
	.info.func = gpio_event_input_func,
	.keymap = ak240_gpio_keymap,
	.keymap_size = ARRAY_SIZE(ak240_gpio_keymap),
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
	.debounce_time.tv.nsec = 20 * NSEC_PER_MSEC,
//	.flags = GPIOEDF_PRINT_KEYS /*GPIOEDF_PRINT_KEYS*/,
	.type = EV_KEY,
};

static struct gpio_event_info *ak240_gpio_key_info[] = {
	&ak240_gpio_key_input_info.info,
};

static struct gpio_event_platform_data ak240_gpio_key_data = {
	.name = "tcc-gpiokey",
	.info = ak240_gpio_key_info,
	.info_count = ARRAY_SIZE(ak240_gpio_key_info),
};

static struct platform_device ak240_gpio_key_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev = {
		.platform_data = &ak240_gpio_key_data,
	},
};
#endif

/*
  * [downer] A130903 for GPIO encoder volume up/down
  */
static struct rotary_encoder_platform_data ak240_gpio_encoder_info = {
    .steps  = 200,
    .axis   = ABS_VOLUME,
    .half_period = 1,
    .gpio_a = GPIO_KEY_VOL_UP,
    .gpio_b = GPIO_KEY_VOL_DOWN,
    .inverted_a = 0,
    .inverted_b = 0,
};

static struct platform_device ak240_gpio_encoder_device = {
    .name   = "gpio-encoder",
    .id     = 0,
    .dev    = {
        .platform_data = &ak240_gpio_encoder_info,
    },
};

#ifdef CONFIG_INPUT_METAL_TOUCH //ysyim
static struct platform_device metal_touch_device = {
	.name ="pic-metal_touch",
	.id = 0,
	.dev = {
		.platform_data = NULL,
	},
};
#endif

#ifdef CONFIG_WAKEUP_ASSIST
static struct platform_device wakeup_assist_device = {	
	.name   = "wakeup_assist",
};
#endif
static int __init ak240_keypad_init(void)
{
//	if (!machine_is_tcc893x())
	//	return 0;

#if defined(CONFIG_KEYBOARD_GPIO)
    platform_device_register(&ak240_gpio_pwr_key_device);
#endif
#if defined(CONFIG_INPUT_GPIO)
	platform_device_register(&ak240_gpio_key_device);
#endif

#if defined(CONFIG_INPUT_GPIO_ROTARY_ENCODER)
    platform_device_register(&ak240_gpio_encoder_device);  /* [downer] A130903 for GPIO encoder */
#endif
#ifdef CONFIG_WAKEUP_ASSIST		
	platform_device_register(&wakeup_assist_device); //2013.11.14-ysyim
#endif

	return 0;
}

device_initcall(ak240_keypad_init);

