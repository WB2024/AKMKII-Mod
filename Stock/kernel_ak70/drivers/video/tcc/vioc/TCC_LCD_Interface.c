
/****************************************************************************
 *   FileName    : TCC_LCD_interfface.c
 *   Description : 
 ****************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips, Inc.
 *   ALL RIGHTS RESERVED
 *
 ****************************************************************************/
/*****************************************************************************
*
* Header Files Include
*
******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>

#include <asm/io.h>

#include <mach/bsp.h>
#include <mach/gpio.h>
#include <mach/TCC_LCD_Interface.h>


#if 1
#define dprintk(msg...)	 { printk( "VIOC_interface: " msg); }
#else
#define dprintk(msg...)	 
#endif

void LCDC_IO_Set (char lcdctrl_num, unsigned bit_per_pixel)
{
	int i;

	if(lcdctrl_num)	{
		tcc_gpio_config(TCC_GPB(1), GPIO_FN1|GPIO_OUTPUT|GPIO_LOW);		//LHSYNC
		tcc_gpio_config(TCC_GPB(2), GPIO_FN1|GPIO_OUTPUT|GPIO_LOW);		//LVSYNC
		tcc_gpio_config(TCC_GPB(0), GPIO_FN1|GPIO_OUTPUT|GPIO_LOW);	// LPXCLK
		tcc_gpio_config(TCC_GPB(19), GPIO_FN1|GPIO_OUTPUT|GPIO_LOW);	//LACBIAS
	}

	for (i = 0 ; i <= bit_per_pixel; i++) {                
		if(lcdctrl_num)	{
			if (i != 16) {
				tcc_gpio_config(TCC_GPB(3 + i), GPIO_FN1|GPIO_OUTPUT|GPIO_LOW);
			}
		}
	}
}

#define LCD_PORT_DISABLE	0	
void LCDC_IO_Disable (char lcdctrl_num, unsigned bit_per_pixel)
{
	int i;

	if(lcdctrl_num)	{
		tcc_gpio_config(TCC_GPB(1), GPIO_FN0|GPIO_OUTPUT|GPIO_LOW);		//LHSYNC
		tcc_gpio_config(TCC_GPB(2), GPIO_FN0|GPIO_OUTPUT|GPIO_LOW);		//LVSYNC
		tcc_gpio_config(TCC_GPB(0), GPIO_FN0|GPIO_OUTPUT|GPIO_LOW);	// LPXCLK
		tcc_gpio_config(TCC_GPB(19), GPIO_FN0|GPIO_OUTPUT|GPIO_LOW);	//LACBIAS
	}

	for (i = 0 ; i <= bit_per_pixel; i++) {                
		if(lcdctrl_num)	{
			if (i != 16) {
				tcc_gpio_config(TCC_GPB(3 + i), GPIO_FN0|GPIO_OUTPUT|GPIO_LOW);
			}
		}
	}
}





