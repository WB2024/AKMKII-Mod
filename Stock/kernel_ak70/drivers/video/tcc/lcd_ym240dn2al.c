/*
 * lcd_ym240dn2al.c -- support for tovis LCD
 *
 * Copyright (C) iriver inc.
 * downer.kim@iriver.com
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
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/mutex.h>
#include <asm/mach-types.h>

#include <mach/tcc_fb.h>
#include <mach/gpio.h>
#include <mach/tca_lcdc.h>
#include <mach/TCC_LCD_Interface.h>
#include <linux/delay.h>

static struct mutex panel_lock;

extern void lcdc_initialize(struct lcd_panel *lcd_spec, unsigned int lcdc_num);

static int ym240dn2al_panel_init(struct lcd_panel *panel)
{
    printk("%s\n", __func__);
    
	return 0;
}

static int ym240dn2al_set_power(struct lcd_panel *panel, int on, unsigned int lcd_num)
{
	struct lcd_platform_data *pdata = panel->dev->platform_data;

	printk("%s : %d \n", __func__, on);

    mutex_lock(&panel_lock);
    
	if (on) {
		gpio_set_value(pdata->power_on, 1);
		gpio_set_value(pdata->reset, 1);
        
		LCDC_IO_Set(1, panel->bus_width);        
		lcdc_initialize(panel, lcd_num);
	}
	else {
		mdelay(10);
		gpio_set_value(pdata->reset, 0);
		LCDC_IO_Disable(1, panel->bus_width);
		gpio_set_value(pdata->power_on, 0);        
	}

    panel->state = on;

    mutex_unlock(&panel_lock);

	if(on)
		panel->set_backlight_level(panel , panel->bl_level);
    
	return 0;
}

static int ym240dn2al_set_backlight_level(struct lcd_panel *panel, int level)
{
	struct lcd_platform_data *pdata = panel->dev->platform_data;

    int lev = 0, i = 0;

    if (level > 32)
        level = 32;
    
	printk("%s : %d\n", __func__, level);

    mutex_lock(&panel_lock);

    panel->bl_level = level;
    
	if (level == 0) {
		gpio_set_value(pdata->bl_on, 0);
	} else {
        gpio_set_value(pdata->bl_on, 1);
        
        for (i = 0; i < lev; i++) {
            gpio_set_value(pdata->bl_on, 0);
            udelay(10);
            gpio_set_value(pdata->bl_on, 1);
            udelay(10);            
        }
	}

    udelay(500);

    mutex_unlock(&panel_lock);
    
	return 0;
}

static struct lcd_panel ym240dn2al_panel = {
	.name		= "YM240DN2AL",
	.manufacturer	= "Samsung",
	.id		= PANEL_ID_YM240DN2AL,
	.xres		= 320,
	.yres		= 240,
	.width		= 320,
	.height		= 240,
	.bpp		= 32,
	.clk_freq	= 120000,//100000,
	.clk_div	= 11,//2,
	.bus_width	= 24,
	.lpw		= 2,
	.lpc		= 319,
	.lswc		= 17,//9,
	.lewc		= 3,
	.vdb		= 0,
	.vdf		= 0,
	.fpw1		= 1,
	.flc1		= 239,//480,
	.fswc1		= 19,
	.fewc1		= 0,//102,
	.fpw2		= 0,
	.flc2		= 239,//480,
	.fswc2		= 1,
	.fewc2		= 19,//102,
	.sync_invert	= IV_INVERT | IH_INVERT | IP_INVERT,
	.init		= ym240dn2al_panel_init,
	.set_power	= ym240dn2al_set_power,
	.set_backlight_level = ym240dn2al_set_backlight_level,
};

static int ym240dn2al_probe(struct platform_device *pdev)
{
	mutex_init(&panel_lock);

	ym240dn2al_panel.state = 1;
	ym240dn2al_panel.dev = &pdev->dev;
	tccfb_register_panel(&ym240dn2al_panel);
	return 0;
}

static int ym240dn2al_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver ym240dn2al_lcd = {
	.probe	= ym240dn2al_probe,
	.remove	= ym240dn2al_remove,
	.driver	= {
		.name	= "ym240dn2al_lcd",
		.owner	= THIS_MODULE,
	},
};

static __init int ym240dn2al_init(void)
{
	return platform_driver_register(&ym240dn2al_lcd);
}

static __exit void ym240dn2al_exit(void)
{
	platform_driver_unregister(&ym240dn2al_lcd);
}

subsys_initcall(ym240dn2al_init);
module_exit(ym240dn2al_exit);

MODULE_DESCRIPTION("YM240DN2AL LCD driver");
MODULE_LICENSE("GPL");
