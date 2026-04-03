/*
 * lcd_s6e63m0x01.c -- support for S6E63M0X1 AMOLED 
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
#include <mach/ak_gpio_def.h>

#include "lcd_s6e63m0x01.h"

static struct mutex panel_lock;

#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

extern void lcdc_initialize(struct lcd_panel *lcd_spec, unsigned int lcdc_num);

#ifdef USE_DYNAMIC_CHARGING_CURRENT 
//jimi.130120
#include <mach/board_astell_kern.h>
extern INTERNAL_PLAY_MODE ak_get_internal_play_mode(void);
//jimi.test.1220
extern int get_adapter_state(void);
extern int tcc_dwc3_vbus_ctrl(int on);
extern void tcc_usb30_phy_off(void);
extern void tcc_usb30_phy_on(void);
extern int g_charge_current_sw;

//jimi.test.lcd_power_state.1224
static int g_lcd_power_state = 1; //0:off, 1:on
int s6e63m0x01_get_power_state(void)
{
	return g_lcd_power_state;	
}
EXPORT_SYMBOL(s6e63m0x01_get_power_state);
#endif

static void init_spi_bus(void)
{
    gpio_request(GPIO_LCD_SCLK, "GPIO_LCD_SCLK");
    gpio_request(GPIO_LCD_SFRM, "GPIO_LCD_SFRM");
    gpio_request(GPIO_LCD_SDO, "GPIO_LCD_SDO");    
    
    tcc_gpio_config(GPIO_LCD_SCLK, GPIO_FN(0));
    tcc_gpio_config(GPIO_LCD_SFRM, GPIO_FN(0));
    tcc_gpio_config(GPIO_LCD_SDO, GPIO_FN(0));

    gpio_direction_output(GPIO_LCD_SCLK, 1);
    gpio_direction_input(GPIO_LCD_SFRM);
    gpio_direction_output(GPIO_LCD_SDO, 1);
}

static void send_lcd_reg_cmd(unsigned char reg)
{
    int i;

    gpio_set_value(GPIO_LCD_SCLK, 0); // SCLK low
    udelay(1);
    
    gpio_set_value(GPIO_LCD_SDO, 0); // SDO low -> Command
    gpio_set_value(GPIO_LCD_SCLK, 1); // SCLK high
    
    udelay(1);
    
    for (i = 0; i < 8; i++) {
        gpio_set_value(GPIO_LCD_SCLK, 0); // SCLK low
        udelay(1);

        if (reg & 0x80)
            gpio_set_value(GPIO_LCD_SDO, 1); // SDO high
        else
            gpio_set_value(GPIO_LCD_SDO, 0); // SDO low

        gpio_set_value(GPIO_LCD_SCLK, 1); // SCLK high

        reg <<= 1;
        udelay(1);
    }
}

static void send_lcd_reg_data(unsigned char data)
{
    int i;

    gpio_set_value(GPIO_LCD_SCLK, 0); // SCLK low
    udelay(1);
    
    gpio_set_value(GPIO_LCD_SDO, 1); // SDO high -> data
    gpio_set_value(GPIO_LCD_SCLK, 1); // SCLK high
    
    udelay(1);
    
    for (i = 0; i < 8; i++) {
        gpio_set_value(GPIO_LCD_SCLK, 0); // SCLK low
        udelay(1);

        if (data & 0x80)
            gpio_set_value(GPIO_LCD_SDO, 1); // SDO high
        else
            gpio_set_value(GPIO_LCD_SDO, 0); // SDO low

        gpio_set_value(GPIO_LCD_SCLK, 1); // SCLK high

        data <<= 1;
        udelay(1);
    }
}

static void lcd_panel_condition_set(void)
{
    send_lcd_reg_cmd(0xF8);
    send_lcd_reg_data(0x01); // 1
    send_lcd_reg_data(0x27); // 2
    send_lcd_reg_data(0x27); // 3
    send_lcd_reg_data(0x07); // 4
    send_lcd_reg_data(0x07); // 5
    send_lcd_reg_data(0x54); // 6
    send_lcd_reg_data(0x9F); // 7
    send_lcd_reg_data(0x63); // 8
    send_lcd_reg_data(0x86); // 9
    send_lcd_reg_data(0x1A); // 10
    send_lcd_reg_data(0x33); // 11
    send_lcd_reg_data(0x0D); // 12
    send_lcd_reg_data(0x00); // 13
    send_lcd_reg_data(0x00); // 14   
}

static void lcd_display_condition_set(void)
{
    send_lcd_reg_cmd(0xF2);
    send_lcd_reg_data(0x02); // Line 800
    send_lcd_reg_data(0x03); // VBP 3 HSYNC
    send_lcd_reg_data(0x1C); // VFP 28 HSYNC
    send_lcd_reg_data(0x10); // HBP 16 DOTCLK
    send_lcd_reg_data(0x10); // HFP 16 DOTCLK

    send_lcd_reg_cmd(0xF7);
    send_lcd_reg_data(0x00); // GTCON: 00h normal / 03h: revert display
    send_lcd_reg_data(0x00); // Display Mode: 16M
//    send_lcd_reg_data(0xF0); // Vsync/Hsync: L-active, DOTCLK: Falling edge active, HE: H-active
    send_lcd_reg_data(0x30); // Vsync/Hsync: L-active, DOTCLK: Falling edge active, HE: H-active

    send_lcd_reg_cmd(0xF3);
    send_lcd_reg_data(0x0F);
    send_lcd_reg_data(0x00);
    send_lcd_reg_data(0x00);
    send_lcd_reg_data(0x00);
    send_lcd_reg_data(0x00);
    send_lcd_reg_data(0x00);
    send_lcd_reg_data(0x01);
    send_lcd_reg_data(0x02);            
}

static void lcd_gamma_condition_set(void)
{
    send_lcd_reg_cmd(0xFA);
    send_lcd_reg_data(0x02); // Gamma set update disable(Bright gamma offset 1)

    send_lcd_reg_data(0x17); // R V1 Gamma
    send_lcd_reg_data(0x13); // G V1 Gamma
    send_lcd_reg_data(0x15); // B V1 Gamma
    send_lcd_reg_data(0xCF); // R V19 Gamma
    send_lcd_reg_data(0xCC); // G V19 Gamma
    send_lcd_reg_data(0xC9); // B V19 Gamma
    send_lcd_reg_data(0xC9); // R V43 Gamma
    send_lcd_reg_data(0xCB); // G V43 Gamma
    send_lcd_reg_data(0xC4); // B V43 Gamma
    send_lcd_reg_data(0xBF); // R V87 Gamma
    send_lcd_reg_data(0xBF); // G V87 Gamma
    send_lcd_reg_data(0xB9); // B V87 Gamma
    send_lcd_reg_data(0xC8); // R V171 Gamma
    send_lcd_reg_data(0xCA); // G V171 Gamma
    send_lcd_reg_data(0xC6); // B V171 Gamma
    send_lcd_reg_data(0x00); // R V255 Gamma
    send_lcd_reg_data(0xBB); // R V255 Gamma
    send_lcd_reg_data(0x00); // G V255 Gamma
    send_lcd_reg_data(0xB4); // G V255 Gamma
    send_lcd_reg_data(0x00); // B V255 Gamma
    send_lcd_reg_data(0xDD); // B V255 Gamma
}

static void lcd_etc_condition_set(void)
{
    send_lcd_reg_cmd(0xF6);
    send_lcd_reg_data(0x00);
    send_lcd_reg_data(0x8E);
    send_lcd_reg_data(0x0F);

    send_lcd_reg_cmd(0xB3);    
    send_lcd_reg_data(0x0C);

    send_lcd_reg_cmd(0xB5);        
    send_lcd_reg_data(0x08); // 1
    send_lcd_reg_data(0x08); // 2
    send_lcd_reg_data(0x08); // 3
    send_lcd_reg_data(0x08); // 4
    send_lcd_reg_data(0x10); // 5
    send_lcd_reg_data(0x10); // 6
    send_lcd_reg_data(0x20); // 7
    send_lcd_reg_data(0x20); // 8
    send_lcd_reg_data(0x2E); // 9
    send_lcd_reg_data(0x19); // 10
    send_lcd_reg_data(0x2E); // 11
    send_lcd_reg_data(0x27); // 12
    send_lcd_reg_data(0x21); // 13
    send_lcd_reg_data(0x1E); // 14
    send_lcd_reg_data(0x1A); // 15
    send_lcd_reg_data(0x19); // 16
    send_lcd_reg_data(0x2C); // 17
    send_lcd_reg_data(0x27); // 18
    send_lcd_reg_data(0x24); // 19
    send_lcd_reg_data(0x21); // 20
    send_lcd_reg_data(0x3B); // 21
    send_lcd_reg_data(0x34); // 22
    send_lcd_reg_data(0x30); // 23
    send_lcd_reg_data(0x2C); // 24
    send_lcd_reg_data(0x28); // 25
    send_lcd_reg_data(0x26); // 26
    send_lcd_reg_data(0x24); // 27
    send_lcd_reg_data(0x22); // 28
    send_lcd_reg_data(0x21); // 29
    send_lcd_reg_data(0x1F); // 30
    send_lcd_reg_data(0x1E); // 31
    send_lcd_reg_data(0x1C); // 32

    send_lcd_reg_cmd(0xB6);        
    send_lcd_reg_data(0x00); // 1
    send_lcd_reg_data(0x00); // 2
    send_lcd_reg_data(0x11); // 3
    send_lcd_reg_data(0x22); // 4
    send_lcd_reg_data(0x33); // 5
    send_lcd_reg_data(0x44); // 6
    send_lcd_reg_data(0x44); // 7
    send_lcd_reg_data(0x44); // 8
    send_lcd_reg_data(0x55); // 9
    send_lcd_reg_data(0x55); // 10
    send_lcd_reg_data(0x66); // 11
    send_lcd_reg_data(0x66); // 12
    send_lcd_reg_data(0x66); // 13
    send_lcd_reg_data(0x66); // 14
    send_lcd_reg_data(0x66); // 15
    send_lcd_reg_data(0x66); // 16

    send_lcd_reg_cmd(0xB7);        
    send_lcd_reg_data(0x08); // 1
    send_lcd_reg_data(0x08); // 2
    send_lcd_reg_data(0x08); // 3
    send_lcd_reg_data(0x08); // 4
    send_lcd_reg_data(0x10); // 5
    send_lcd_reg_data(0x10); // 6
    send_lcd_reg_data(0x20); // 7
    send_lcd_reg_data(0x20); // 8
    send_lcd_reg_data(0x2E); // 9
    send_lcd_reg_data(0x19); // 10
    send_lcd_reg_data(0x2E); // 11
    send_lcd_reg_data(0x27); // 12
    send_lcd_reg_data(0x21); // 13
    send_lcd_reg_data(0x1E); // 14
    send_lcd_reg_data(0x1A); // 15
    send_lcd_reg_data(0x19); // 16
    send_lcd_reg_data(0x2C); // 17
    send_lcd_reg_data(0x27); // 18
    send_lcd_reg_data(0x24); // 19
    send_lcd_reg_data(0x21); // 20
    send_lcd_reg_data(0x3B); // 21
    send_lcd_reg_data(0x34); // 22
    send_lcd_reg_data(0x30); // 23
    send_lcd_reg_data(0x2C); // 24
    send_lcd_reg_data(0x28); // 25
    send_lcd_reg_data(0x26); // 26
    send_lcd_reg_data(0x24); // 27
    send_lcd_reg_data(0x22); // 28
    send_lcd_reg_data(0x21); // 29
    send_lcd_reg_data(0x1F); // 30
    send_lcd_reg_data(0x1E); // 31
    send_lcd_reg_data(0x1C); // 32

    send_lcd_reg_cmd(0xB8);        
    send_lcd_reg_data(0x00); // 1
    send_lcd_reg_data(0x00); // 2
    send_lcd_reg_data(0x11); // 3
    send_lcd_reg_data(0x22); // 4
    send_lcd_reg_data(0x33); // 5
    send_lcd_reg_data(0x44); // 6
    send_lcd_reg_data(0x44); // 7
    send_lcd_reg_data(0x44); // 8
    send_lcd_reg_data(0x55); // 9
    send_lcd_reg_data(0x55); // 10
    send_lcd_reg_data(0x66); // 11
    send_lcd_reg_data(0x66); // 12
    send_lcd_reg_data(0x66); // 13
    send_lcd_reg_data(0x66); // 14
    send_lcd_reg_data(0x66); // 15
    send_lcd_reg_data(0x66); // 16

    send_lcd_reg_cmd(0xB9);        
    send_lcd_reg_data(0x08); // 1
    send_lcd_reg_data(0x08); // 2
    send_lcd_reg_data(0x08); // 3
    send_lcd_reg_data(0x08); // 4
    send_lcd_reg_data(0x10); // 5
    send_lcd_reg_data(0x10); // 6
    send_lcd_reg_data(0x20); // 7
    send_lcd_reg_data(0x20); // 8
    send_lcd_reg_data(0x2E); // 9
    send_lcd_reg_data(0x19); // 10
    send_lcd_reg_data(0x2E); // 11
    send_lcd_reg_data(0x27); // 12
    send_lcd_reg_data(0x21); // 13
    send_lcd_reg_data(0x1E); // 14
    send_lcd_reg_data(0x1A); // 15
    send_lcd_reg_data(0x19); // 16
    send_lcd_reg_data(0x2C); // 17
    send_lcd_reg_data(0x27); // 18
    send_lcd_reg_data(0x24); // 19
    send_lcd_reg_data(0x21); // 20
    send_lcd_reg_data(0x3B); // 21
    send_lcd_reg_data(0x34); // 22
    send_lcd_reg_data(0x30); // 23
    send_lcd_reg_data(0x2C); // 24
    send_lcd_reg_data(0x28); // 25
    send_lcd_reg_data(0x26); // 26
    send_lcd_reg_data(0x24); // 27
    send_lcd_reg_data(0x22); // 28
    send_lcd_reg_data(0x21); // 29
    send_lcd_reg_data(0x1F); // 30
    send_lcd_reg_data(0x1E); // 31
    send_lcd_reg_data(0x1C); // 32

    send_lcd_reg_cmd(0xBA);        
    send_lcd_reg_data(0x00); // 1
    send_lcd_reg_data(0x00); // 2
    send_lcd_reg_data(0x11); // 3
    send_lcd_reg_data(0x22); // 4
    send_lcd_reg_data(0x33); // 5
    send_lcd_reg_data(0x44); // 6
    send_lcd_reg_data(0x44); // 7
    send_lcd_reg_data(0x44); // 8
    send_lcd_reg_data(0x55); // 9
    send_lcd_reg_data(0x55); // 10
    send_lcd_reg_data(0x66); // 11
    send_lcd_reg_data(0x66); // 12
    send_lcd_reg_data(0x66); // 13
    send_lcd_reg_data(0x66); // 14
    send_lcd_reg_data(0x66); // 15
    send_lcd_reg_data(0x66); // 16
}

static void lcd_standby_off(void)
{
    send_lcd_reg_cmd(0x11);
//    mdelay(120);
}

static void lcd_standby_on(void)
{
    send_lcd_reg_cmd(0x10);
//    mdelay(120);
}

static void lcd_display_on(void)
{
    send_lcd_reg_cmd(0x29);
}

static void lcd_acl_on(void)
{
    send_lcd_reg_cmd(0xC0);
    send_lcd_reg_data(0x01);
}

static void lcd_acl_off(void)
{
    send_lcd_reg_cmd(0xC0);
    send_lcd_reg_data(0x00);
}

static void lcd_elvss_on(void)
{
    send_lcd_reg_cmd(0xB1);
    send_lcd_reg_data(0x0B);
}

static void lcd_elvss_off(void)
{
    send_lcd_reg_cmd(0xB1);
    send_lcd_reg_data(0x0A);
}

static int s6e63m0x01_panel_init(struct lcd_panel *panel)
{
    lcdc_initialize(panel, 1);    
	return 0;
}

static void s6e63m0x01_dev_init(void)
{
    init_spi_bus();

    gpio_set_value(GPIO_LCD_SFRM, 0);     
    
    /*
     * PANEL CONDITION SET
     */
    lcd_panel_condition_set();

    /*
     * DISPLAY CONDITION SET
     */
    lcd_display_condition_set();

    /*
     * GAMMA CONDITION SET
     */
    lcd_gamma_condition_set();

    /*
     * ETC CONDITION SET
     */
    lcd_etc_condition_set();

    /*
     * STANDBY OFF
     */
    lcd_standby_off();

    /*
     * DISPLAY ON
     */
    lcd_display_on();

    gpio_set_value(GPIO_LCD_SFRM, 1); 
}

static void lcd_on(void)
{
    s6e63m0x01_dev_init();
}

static int s6e63m0x01_set_power(struct lcd_panel *panel, int on, unsigned int lcd_num)
{
	struct lcd_platform_data *pdata = panel->dev->platform_data;

    mutex_lock(&panel_lock);
    
	if (on) {
		gpio_set_value(pdata->power_on, 1);
        mdelay(1);
		gpio_set_value(pdata->reset, 1);

        lcd_on();
		lcdc_initialize(panel, lcd_num);
        
		LCDC_IO_Set(1, panel->bus_width);                
	}
	else {
		gpio_set_value(pdata->reset, 0);
		gpio_set_value(pdata->power_on, 0);
        
		LCDC_IO_Disable(1, panel->bus_width);        
	}

    panel->state = on;

    if (on)
        panel->set_backlight_level(panel, panel->bl_level);
    
#ifdef USE_DYNAMIC_CHARGING_CURRENT 
//jimi.test.lcd_power_state.1224
    g_lcd_power_state = on; 
    //printk("jimi================>DISPLAY POWER ON/OFF (0x%x)\n", on);
    if( get_adapter_state() == 1  && ak_get_internal_play_mode() != INTERNAL_DSD_PLAY) {
	    if(g_lcd_power_state) {
#if 0  //Do not use
			printk("+USB ADAPTER STATE ( LCD ON ) ===========> LOW CHARGING CURRENT\n");
#if (CUR_AK >= MODEL_AK240) 	
			tcc_usb30_phy_off();
			tcc_dwc3_vbus_ctrl(0);
			msleep(100);
#endif
			gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
			tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_DAC_SEL, 1);

			msleep(100);

			gpio_request(GPIO_XMOS_USB_EN, "GPIO_XMOS_USB_EN");
			tcc_gpio_config(GPIO_XMOS_USB_EN,GPIO_FN(0));
			gpio_direction_output(GPIO_XMOS_USB_EN, 1);
	
#if (CUR_AK >= MODEL_AK240) 
			tcc_usb30_phy_off();
			tcc_dwc3_vbus_ctrl(0);
#endif
			g_charge_current_sw = 0;
			printk("-USB ADAPTER STATE ( LCD ON ) ===========> LOW CHARGING CURRENT\n");
#endif
	    }
	    else {
			printk("+USB ADAPTER STATE ( LCD OFF ) ===========> HIGH CHARGING CURRENT\n");

#if (CUR_AK >= MODEL_AK240) 
			tcc_usb30_phy_off();
			tcc_dwc3_vbus_ctrl(0);
			msleep(10);
#endif
			gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
			tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_DAC_SEL, 0);

			msleep(10);

#if (CUR_AK > MODEL_AK240)
			gpio_request(GPIO_USB_ON, "GPIO_USB_ON");
			tcc_gpio_config(GPIO_USB_ON,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_ON, 1);
#else
			gpio_request(GPIO_XMOS_USB_EN, "GPIO_XMOS_USB_EN");
			tcc_gpio_config(GPIO_XMOS_USB_EN,GPIO_FN(0));
			gpio_direction_output(GPIO_XMOS_USB_EN, 1);
#endif

#if (CUR_AK >= MODEL_AK240) 
			tcc_usb30_phy_off();
			tcc_dwc3_vbus_ctrl(0);
#endif

			g_charge_current_sw = 1;
			printk("-USB ADAPTER STATE ( LCD OFF ) ===========> HIGH CHARGING CURRENT\n");
	    }
    }
#endif

    mutex_unlock(&panel_lock);

	return 0;
}

static void s6e63m0x01_gamma_ctrl(const unsigned int *gamma)
{
    int i;

    gpio_set_value(GPIO_LCD_SFRM, 0);
    
    send_lcd_reg_cmd(0xFA);
    send_lcd_reg_data(0x02); // Gamma set update disable(Bright gamma offset 1)
    
    for (i = 0; i < GAMMA_TABLE_COUNT; i++) 
        send_lcd_reg_data(gamma[i]);

    send_lcd_reg_cmd(0xFA);
    send_lcd_reg_data(0x03); // Gamma set update enable

    gpio_set_value(GPIO_LCD_SFRM, 1);         
}

static int s6e63m0x01_set_brightness(struct lcd_panel *panel, int level)
{
    if (level > MAX_BRIGHTNESS)
        level = MAX_BRIGHTNESS;

    panel->bl_level = level;
    s6e63m0x01_gamma_ctrl(gamma_table.gamma_22_table[level]);
    
	return 0;
}

static struct lcd_panel s6e63m0x01_panel = {
	.name		= "S6E63M0X01",
	.manufacturer	= "Samsung",
	.id		= PANEL_ID_S6E63M0X01,
	.xres		= 480, // fix
	.yres		= 800, // fix
	.width		= 480, // fix
	.height		= 800, // fix
	.bpp		= 32, // fix
	.clk_freq	= 505000,
	.clk_div	= 2,//2,
	.bus_width	= 24, // fix
	.lpw		= 1,
	.lpc		= 479, //fix
	.lswc		= 16,
	.lewc		= 16,
	.vdb		= 3,
	.vdf		= 28,
	.fpw1		= 1,
	.flc1		= 799, // don't care
	.fswc1		= 2,
	.fewc1		= 2,
	.fpw2		= 1,
	.flc2		= 799, // don't care
	.fswc2		= 2,
	.fewc2		= 2,
	.sync_invert	= IP_INVERT,    
	.init		= s6e63m0x01_panel_init,
	.set_power	= s6e63m0x01_set_power,
	.set_backlight_level = s6e63m0x01_set_brightness,
};

static int s6e63m0x01_probe(struct platform_device *pdev)
{
    printk("%s\n", __func__);
    
	mutex_init(&panel_lock);
    
    init_spi_bus();
    
	s6e63m0x01_panel.state = 1;
	s6e63m0x01_panel.dev = &pdev->dev;
	tccfb_register_panel(&s6e63m0x01_panel);
    
	return 0;
}

static int s6e63m0x01_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver s6e63m0x01_lcd = {
	.probe	= s6e63m0x01_probe,
	.remove	= s6e63m0x01_remove,
	.driver	= {
		.name	= "s6e63m0x01_lcd",
		.owner	= THIS_MODULE,
	},
};

static __init int s6e63m0x01_init(void)
{
	return platform_driver_register(&s6e63m0x01_lcd);
}

static __exit void s6e63m0x01_exit(void)
{
	platform_driver_unregister(&s6e63m0x01_lcd);
}

subsys_initcall(s6e63m0x01_init);
module_exit(s6e63m0x01_exit);

MODULE_DESCRIPTION("S6E63M0X01 LCD driver");
MODULE_LICENSE("GPL");
