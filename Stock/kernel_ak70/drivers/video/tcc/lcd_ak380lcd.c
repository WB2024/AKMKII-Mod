/*
 * lcd_ak380lcd.c -- support for S6E63M0X1 AMOLED 
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

#include <mach/tca_tco.h>

#include <linux/earlysuspend.h>

#include "lcd_ak380lcd.h"

static struct mutex panel_lock;

#define LCD_BL_EN TCC_GPB(30)
#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

static int debug = 1;
#define dprintk(msg...)	if (debug) { printk( "lcd_ak380: " msg); }

extern void lcdc_initialize(struct lcd_panel *lcd_spec, unsigned int lcdc_num);

static struct early_suspend lcd_moudle_early_suspend;

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
int ak380lcd_get_power_state(void)
{
	return g_lcd_power_state;	
}
EXPORT_SYMBOL(ak380lcd_get_power_state);
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
    gpio_direction_output(GPIO_LCD_SFRM, 1);
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

static void ak380lcd_panel_condition_set(void)
{
	printk("%s\n",__FUNCTION__);

	//Change to page1
	send_lcd_reg_cmd(0xFF); // EXTC Command Set enable register
	send_lcd_reg_data(0xFF);
	send_lcd_reg_data(0x98);
	send_lcd_reg_data(0x06);
	send_lcd_reg_data(0x04);
	send_lcd_reg_data(0x01); 

	//Display Setting
	send_lcd_reg_cmd(0x08); 	send_lcd_reg_data(0x10); // Output SDA
	send_lcd_reg_cmd(0x21);	send_lcd_reg_data(0x00); // DE = 1 Active			
	send_lcd_reg_cmd(0x30);	send_lcd_reg_data(0x02); // 480 * 800	
	send_lcd_reg_cmd(0x31); 	send_lcd_reg_data(0x02); // Column inversion	
	send_lcd_reg_cmd(0x60);	send_lcd_reg_data(0x07); // SDTI	
	send_lcd_reg_cmd(0x61); 	send_lcd_reg_data(0x00); // CRTI	
	send_lcd_reg_cmd(0x62); 	send_lcd_reg_data(0x09); // EQTI	
	send_lcd_reg_cmd(0x63);	send_lcd_reg_data(0x00); // PCTI
	
	//Set Gate Power
	send_lcd_reg_cmd(0x40);	send_lcd_reg_data(0x14); // BT 2.5/-2.5 pump for DDVDH-L	
	send_lcd_reg_cmd(0x41); 	send_lcd_reg_data(0x33);// DDVDH/ DDVDL clamp	
	send_lcd_reg_cmd(0x42); 	send_lcd_reg_data(0x01);// VGH/VGL
	send_lcd_reg_cmd(0x43);	send_lcd_reg_data(0x03);	
	send_lcd_reg_cmd(0x44);	send_lcd_reg_data(0x09);
	//send_lcd_reg_cmd(0x45); 		send_lcd_reg_data(0x1B);

	//Set VCOM
	send_lcd_reg_cmd(0x50); 	send_lcd_reg_data(0x60); // VGMP	
	send_lcd_reg_cmd(0x51); 	send_lcd_reg_data(0x60); // VGMN	
	send_lcd_reg_cmd(0x52); 	send_lcd_reg_data(0x00); // VCOM	
	send_lcd_reg_cmd(0x53); 	send_lcd_reg_data(0x52); // VCOM
	
	//==== GAMMA Positive ====//
	send_lcd_reg_cmd(0xA0);	send_lcd_reg_data(0x00);
	send_lcd_reg_cmd(0xA1);	send_lcd_reg_data(0x0C);
	send_lcd_reg_cmd(0xA2);	send_lcd_reg_data(0x13);
	send_lcd_reg_cmd(0xA3);	send_lcd_reg_data(0x0C);
	send_lcd_reg_cmd(0xA4);	send_lcd_reg_data(0x04);
	send_lcd_reg_cmd(0xA5);	send_lcd_reg_data(0x09);
	send_lcd_reg_cmd(0xA6);	send_lcd_reg_data(0x07);
	send_lcd_reg_cmd(0xA7);	send_lcd_reg_data(0x06);
	send_lcd_reg_cmd(0xA8);	send_lcd_reg_data(0x06);
	send_lcd_reg_cmd(0xA9);	send_lcd_reg_data(0x0A);
	send_lcd_reg_cmd(0xAA);	send_lcd_reg_data(0x11);
	send_lcd_reg_cmd(0xAB);	send_lcd_reg_data(0x08);
	send_lcd_reg_cmd(0xAC);	send_lcd_reg_data(0x0C);
	send_lcd_reg_cmd(0xAD);	send_lcd_reg_data(0x1B);
	send_lcd_reg_cmd(0xAE);	send_lcd_reg_data(0x1D);
	send_lcd_reg_cmd(0xAF);	send_lcd_reg_data(0x00);

	//==== GAMMA Negative ====//
	send_lcd_reg_cmd(0xC0);	send_lcd_reg_data(0x00);
	send_lcd_reg_cmd(0xC1);	send_lcd_reg_data(0x0C);
	send_lcd_reg_cmd(0xC2);	send_lcd_reg_data(0x13);
	send_lcd_reg_cmd(0xC3);	send_lcd_reg_data(0x0C);
	send_lcd_reg_cmd(0xC4);	send_lcd_reg_data(0x04);
	send_lcd_reg_cmd(0xC5);	send_lcd_reg_data(0x09);
	send_lcd_reg_cmd(0xC6);	send_lcd_reg_data(0x07);
	send_lcd_reg_cmd(0xC7);	send_lcd_reg_data(0x06);
	send_lcd_reg_cmd(0xC8);	send_lcd_reg_data(0x06);
	send_lcd_reg_cmd(0xC9);	send_lcd_reg_data(0x0A);
	send_lcd_reg_cmd(0xCA);	send_lcd_reg_data(0x11);
	send_lcd_reg_cmd(0xCB);	send_lcd_reg_data(0x08);
	send_lcd_reg_cmd(0xCC);	send_lcd_reg_data(0x0C);
	send_lcd_reg_cmd(0xCD);	send_lcd_reg_data(0x1B);
	send_lcd_reg_cmd(0xCE);	send_lcd_reg_data(0x0D);
	send_lcd_reg_cmd(0xCF);	send_lcd_reg_data(0x00);

	 //Change to page7
	send_lcd_reg_cmd(0xFF);
	send_lcd_reg_data(0xFF);
	send_lcd_reg_data(0x98);
	send_lcd_reg_data(0x06);
	send_lcd_reg_data(0x04);
	send_lcd_reg_data(0x07);

	send_lcd_reg_cmd(0x18); 	send_lcd_reg_data(0x1D);
	send_lcd_reg_cmd(0x17);	send_lcd_reg_data(0x22); 
	send_lcd_reg_cmd(0x02);	send_lcd_reg_data(0x77); 
	send_lcd_reg_cmd(0xE1);	send_lcd_reg_data(0x79); 

	//Change to page6
	send_lcd_reg_cmd(0xFF);
	send_lcd_reg_data(0xFF);
	send_lcd_reg_data(0x98);
	send_lcd_reg_data(0x06);
	send_lcd_reg_data(0x04);
	send_lcd_reg_data(0x06); 

	//GIP SET
	send_lcd_reg_cmd(0x00); 	send_lcd_reg_data(0xA0);
	send_lcd_reg_cmd(0x01);	send_lcd_reg_data(0x05);
	send_lcd_reg_cmd(0x02);	send_lcd_reg_data(0x00);
	send_lcd_reg_cmd(0x03);	send_lcd_reg_data(0x00);
	send_lcd_reg_cmd(0x04);	send_lcd_reg_data(0x01);
	send_lcd_reg_cmd(0x05);	send_lcd_reg_data(0x01);
	send_lcd_reg_cmd(0x06);	send_lcd_reg_data(0x88);
	send_lcd_reg_cmd(0x07);	send_lcd_reg_data(0x04);
	send_lcd_reg_cmd(0x08);	send_lcd_reg_data(0x01);
	send_lcd_reg_cmd(0x09);	send_lcd_reg_data(0x90);
	send_lcd_reg_cmd(0x0A);	send_lcd_reg_data(0x04);
	send_lcd_reg_cmd(0x0B);	send_lcd_reg_data(0x01);
	send_lcd_reg_cmd(0x0C);	send_lcd_reg_data(0x01);
	send_lcd_reg_cmd(0x0D);	send_lcd_reg_data(0x01);
	send_lcd_reg_cmd(0x0E);	send_lcd_reg_data(0x00);
	send_lcd_reg_cmd(0x0F);	send_lcd_reg_data(0x00);

	//GIP SET
	send_lcd_reg_cmd(0x10);	send_lcd_reg_data(0x55);
	send_lcd_reg_cmd(0x11);	send_lcd_reg_data(0x50);
	send_lcd_reg_cmd(0x12);	send_lcd_reg_data(0x01);
	send_lcd_reg_cmd(0x13);	send_lcd_reg_data(0x85);
	send_lcd_reg_cmd(0x14);	send_lcd_reg_data(0x85);
	send_lcd_reg_cmd(0x15);	send_lcd_reg_data(0xC0);
	send_lcd_reg_cmd(0x16);	send_lcd_reg_data(0x0B);
	send_lcd_reg_cmd(0x17);	send_lcd_reg_data(0x00);
	send_lcd_reg_cmd(0x18);	send_lcd_reg_data(0x00);
	send_lcd_reg_cmd(0x19);	send_lcd_reg_data(0x00);
	send_lcd_reg_cmd(0x1A);	send_lcd_reg_data(0x00);
	send_lcd_reg_cmd(0x1B);	send_lcd_reg_data(0x00);
	send_lcd_reg_cmd(0x1C);	send_lcd_reg_data(0x00);
	send_lcd_reg_cmd(0x1D);	send_lcd_reg_data(0x00);

	//GIP SET
	send_lcd_reg_cmd(0x20); 	send_lcd_reg_data(0x01);
	send_lcd_reg_cmd(0x21);	send_lcd_reg_data(0x23);
	send_lcd_reg_cmd(0x22);	send_lcd_reg_data(0x45);
	send_lcd_reg_cmd(0x23);	send_lcd_reg_data(0x67);
	send_lcd_reg_cmd(0x24);	send_lcd_reg_data(0x01);
	send_lcd_reg_cmd(0x25);	send_lcd_reg_data(0x23); //Blanking Porch Control1 //VFP=35	
	send_lcd_reg_cmd(0x26);	send_lcd_reg_data(0x45); //BLKPRH  //VBP=69 
	send_lcd_reg_cmd(0x27);	send_lcd_reg_data(0x67); //Blanking Porch 3) //HBP=103

	send_lcd_reg_cmd(0x30);	send_lcd_reg_data(0x02); //Resolution Control 480X800
	send_lcd_reg_cmd(0x31);	send_lcd_reg_data(0x22); //Display Inversion Control)
	send_lcd_reg_cmd(0x32);	send_lcd_reg_data(0x11);
	send_lcd_reg_cmd(0x33);	send_lcd_reg_data(0xAA);
	send_lcd_reg_cmd(0x34);	send_lcd_reg_data(0xBB);
	send_lcd_reg_cmd(0x35);	send_lcd_reg_data(0x66);
	send_lcd_reg_cmd(0x36);	send_lcd_reg_data(0x00);
	send_lcd_reg_cmd(0x37);	send_lcd_reg_data(0x22);
	send_lcd_reg_cmd(0x38);	send_lcd_reg_data(0x22);
	send_lcd_reg_cmd(0x39);	send_lcd_reg_data(0x22);
	send_lcd_reg_cmd(0x3A);	send_lcd_reg_data(0x22);
	send_lcd_reg_cmd(0x3B);	send_lcd_reg_data(0x22);
	send_lcd_reg_cmd(0x3C);	send_lcd_reg_data(0x22);
	send_lcd_reg_cmd(0x3D);	send_lcd_reg_data(0x22);
	send_lcd_reg_cmd(0x3E);	send_lcd_reg_data(0x22);
	send_lcd_reg_cmd(0x3F);	send_lcd_reg_data(0x22);
	send_lcd_reg_cmd(0x40);	send_lcd_reg_data(0x22);

	send_lcd_reg_cmd(0x52); 	send_lcd_reg_data(0x10); 
	send_lcd_reg_cmd(0x53); 	send_lcd_reg_data(0x10); //VGLO refer VGL_REG

	//Change to page0
	send_lcd_reg_cmd(0xFF);
	send_lcd_reg_data(0xFF);
	send_lcd_reg_data(0x98);
	send_lcd_reg_data(0x06);
	send_lcd_reg_data(0x04);
	send_lcd_reg_data(0x00); 

	//Display Control
	send_lcd_reg_cmd(0x36); 	send_lcd_reg_data(0x00); 
	send_lcd_reg_cmd(0x3A); 	send_lcd_reg_data(0x77); 	

	send_lcd_reg_cmd(0x20); 

	send_lcd_reg_cmd(0x11); //Exit Sleep
	mdelay(10);

	send_lcd_reg_cmd(0x29); // Display On

}


static void lcd_enter_sleep(void)
{
	send_lcd_reg_cmd(0x28); //Display Off
	mdelay(10);
	send_lcd_reg_cmd(0x10); //Sleep-In
	mdelay(120);
}

static void lcd_exit_sleep(void)
{
	send_lcd_reg_cmd(0x11); // Sleep-Out
	mdelay(120);
	send_lcd_reg_cmd(0x29); // Display On
	mdelay(10);
}

static int ak380lcd_panel_init(struct lcd_panel *panel)
{
   lcdc_initialize(panel, 1);    
	return 0;
}

static void ak380lcd_dev_init(void)
{
    init_spi_bus();

    gpio_set_value(GPIO_LCD_SFRM, 0);     

	ak380lcd_panel_condition_set();


    gpio_set_value(GPIO_LCD_SFRM, 1); 
}

static void lcd_on(void)
{
    ak380lcd_dev_init();
}

static int ak380lcd_set_power(struct lcd_panel *panel, int on, unsigned int lcd_num)
{
	struct lcd_platform_data *pdata = panel->dev->platform_data;

	dprintk("%s-%d\n",__FUNCTION__,on);

    mutex_lock(&panel_lock);
    
	if (on) {
		gpio_set_value(pdata->power_on, 1);
        mdelay(5);
		gpio_set_value(pdata->reset, 1);
        mdelay(5);
        
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

    mutex_unlock(&panel_lock);

//    if (on)
//        panel->set_backlight_level(panel, panel->bl_level);

	return 0;
}

void backlight_off(void)
{
	gpio_set_value(LCD_BL_EN, 0);
}
EXPORT_SYMBOL(backlight_off);

static int ak380lcd_set_brightness(struct lcd_panel *panel, int level)
{

   #define MAX_BACKLIGTH 255
	int org_level;
   struct lcd_platform_data *pdata = panel->dev->platform_data;
   panel->bl_level = level;
		org_level =level;
		
		if(level >= 11)
			level = MAX_BACKLIGTH;		
		else if(level <= 0)
			level = 0;			
		else{
			//level = level*15+80;  // (255-80)/11 = 15
			//level = level*20+30;  // (255-30)/11 = 20  :ÃÖ¼Ò ¹à±â 30 
			level = level*24;  // (255)/11 = 23.18  :ÃÖ¼Ò ¹à±â 24 
		}
		
	  dprintk("%s level is:%d(%d)\n", __func__,level,org_level);
	  

   mutex_lock(&panel_lock);
      

    if (level == 0) {
		gpio_set_value(pdata->display_on, 0);
        tca_tco_pwm_ctrl(1, pdata->bl_on, MAX_BACKLIGTH, level);				
    }    
    else 
    {    
        if(panel->state)
        {
         msleep(100);
      	  gpio_set_value(pdata->display_on, 1);
	      tca_tco_pwm_ctrl(1, pdata->bl_on, MAX_BACKLIGTH, level);
        }
    }    
     
    mutex_unlock(&panel_lock);

    return 0;
		

}

static struct lcd_panel ak380lcd_panel = {
	.name		= "LCD_AK380DISP",
	.manufacturer	= "IRIVER",
	.id		= PANEL_ID_AK380,
	.xres		= 480, 
	.yres		= 800, 
	.width		= 480, 
	.height		= 800, 
	.bpp		= 32, 
	
	.clk_freq	= 320000*2,
	.clk_div	= 2,
	.bus_width	= 24,

	.lpw		= 10, //HLW
	.lpc		= 479, 
	.lswc		= 93,//HBP
	.lewc		= 3, //HFP

	.vdb		= 0,
	.vdf		= 0,

	.fpw1		= 10, //FPW
	.flc1		= 799,  
	.fswc1		= 59, //VPB
	.fewc1		= 35, //VFP
	
	.fpw2		= 10,
	.flc2		= 799, 
	.fswc2		= 59,
	.fewc2		= 35,
//	.sync_invert	= IV_INVERT | IH_INVERT | ID_INVERT | IP_INVERT,    
	.sync_invert	=  ID_INVERT | IP_INVERT  ,    
	.init		= ak380lcd_panel_init,
	.set_power	= ak380lcd_set_power,
	.set_backlight_level = ak380lcd_set_brightness,
};

static void lcd_ak380_early_suspend(struct early_suspend *h)
{	
	dprintk("%s\n",__FUNCTION__);
}

static void lcd_ak380_late_resume(struct early_suspend *h)
{
	dprintk("%s\n",__FUNCTION__);
}

static int ak380lcd_probe(struct platform_device *pdev)
{
    printk("%s\n", __func__);
    
	mutex_init(&panel_lock);
    
    init_spi_bus();
    
	ak380lcd_panel.state = 1;
	ak380lcd_panel.dev = &pdev->dev;
	tccfb_register_panel(&ak380lcd_panel);

		
	lcd_moudle_early_suspend.suspend = lcd_ak380_early_suspend;
	lcd_moudle_early_suspend.resume = lcd_ak380_late_resume;
	lcd_moudle_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;	
	register_early_suspend(&lcd_moudle_early_suspend);

	
	return 0;
}

static int ak380lcd_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver ak380lcd_lcd = {
	.probe	= ak380lcd_probe,
	.remove	= ak380lcd_remove,
	.driver	= {
		.name	= "lcd_ak380lcd",
		.owner	= THIS_MODULE,
	},
};

static __init int ak380lcd_init(void)
{
	return platform_driver_register(&ak380lcd_lcd);
}

static __exit void ak380lcd_exit(void)
{
	platform_driver_unregister(&ak380lcd_lcd);
}

subsys_initcall(ak380lcd_init);
module_exit(ak380lcd_exit);

MODULE_DESCRIPTION("AK380 LCD driver");
MODULE_LICENSE("GPL");
