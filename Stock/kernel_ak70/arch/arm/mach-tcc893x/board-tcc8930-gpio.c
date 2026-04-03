/*
 * linux/arch/arm/mach-tcc893x/board-tcc8930-gpio.c
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
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <mach/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c/pca953x.h>
#include <mach/board-tcc8930.h>
#include <mach/irqs.h>
#include <asm/mach-types.h>

static struct board_gpio_irq_config tcc8930_gpio_irqs[] = {
	{ GPIO_KEY_VOL_UP, INT_EINT1 }, // Volume up
	{ GPIO_WL_HOST_WAKE, INT_EINT2 },	// WIFI Data interrupt
    { GPIO_KEY_VOL_DOWN, INT_EINT3 }, // Volume down
    { GPIO_PWR_ON_OFF, INT_EINT4 }, // Power-Key
	{ GPIO_SD30_DET, INT_EINT5 }, // SD30_DET

#if (CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300) 
	{ GPIO_DSP_INT_HS1, INT_EINT7 }, // extended spi interrupt for audio dsp
#endif

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)                
    { GPIO_OTG_DET, INT_EINT9 }, // OTG_DET
#elif (CUR_AK == MODEL_AK500N)
   { GPIO_RX_PLL_UNLOCK, INT_EINT9 }, // OTG_DET
#endif

#if (CUR_AK == MODEL_AK500N)
	{ GPIO_USB_DET, INT_EINT10 }, // USB_DET    
	{ GPIO_CHG_DET, INT_EINT0},   // CHG_DET
#else
	{ GPIO_CHG_DET, INT_EINT10 }, // CHG_DET    
#endif

	{ GPIO_TOUCH_INT, INT_EINT11 }, // TSP_INT

	{ -1, -1 },
};

void __init tcc8930_init_gpio(void)
{
//	if (!machine_is_tcc893x())
//		return;
	
	board_gpio_irqs = tcc8930_gpio_irqs;
	printk(KERN_INFO "tcc8930 GPIO initialized\n");

#if (CUR_AK == MODEL_AK70)
//	tcc_gpio_config(GPIO_WIFI_BT_PWR_EN, GPIO_OUTPUT | GPIO_FN0);
//	gpio_set_value(GPIO_WIFI_BT_PWR_EN, 1);
#endif

#if 0
	//차 후 bootloader로 옮기자.
	printk("============= Connect XMOS to flash ==========\n");
	gpio_request(GPIO_XMOS_SPI_SEL, "GPIO_XMOS_SPI_SEL");
	tcc_gpio_config(GPIO_XMOS_SPI_SEL, GPIO_FN(0)); 
	gpio_direction_output(GPIO_XMOS_SPI_SEL, 1);
#endif
    
	return;
}

