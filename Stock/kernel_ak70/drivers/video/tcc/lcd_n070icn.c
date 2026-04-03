/*
 * lcd_n070icn.c
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

#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/power_supply.h>

#include <mach/tca_tco.h>

#include <linux/earlysuspend.h>

#include "lcd_n070icn.h"
 
static struct mutex panel_lock;

static struct early_suspend lcd_moudle_early_suspend;

static int debug = 0;
#define dprintk(msg...)	if (debug) { printk( "lcd_n070: " msg); }

//#define DONOT_SLEEP

#ifdef DONOT_SLEEP
static struct wake_lock lcd_n070icn_lock;
#endif

#define POWER_IS_ON(pwr)	((pwr) <= FB_BLANK_NORMAL)

//#define RGB565

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

int n070icn_get_power_state(void)
{
  dprintk("[lcd] %s\n", __func__);

	return g_lcd_power_state;	
}
EXPORT_SYMBOL(n070icn_get_power_state);
#endif

struct tc358768xbg_data{
	struct i2c_client		*client;
};

struct i2c_client		*gclient;


static unsigned short int tc358768xbg_read(u16 reg)
{
	struct i2c_adapter *adapter = to_i2c_adapter(gclient->dev.parent);

  int ret;
  u8 send_data[2];
	u8 recv_data[2];
	u16 read_val;

  struct i2c_msg msg[] = {
      { .addr = gclient->addr,
        .flags = 0, 
        .buf = send_data, 
        .len = 2 
      },
      { .addr = gclient->addr,
        .flags = I2C_M_RD, 
        .buf = recv_data, 
        .len = 2 
      },
  }; 


	send_data[0]= reg >> 8;
	send_data[1]= reg & 0xff;
	
  ret = i2c_transfer(adapter, msg, 2);

	read_val =  (recv_data[0] <<8 ) + (recv_data[1] & 0xFF); 		
		
  //dprintk("tc358768xbg 0x%04x => 0x%04x\n\n", reg, read_val);
  
  if(ret <= 0) {
      dprintk("error (addr %02x reg %04x error (ret == %i)\n",
             gclient->addr, reg, ret);
      if (ret < 0)
          return ret;
      else
          return -EREMOTEIO;
  }  
    return read_val;
}

static int tc358768xbg_write(u16 reg, u16 val)
{    

		struct i2c_adapter *adapter = to_i2c_adapter(gclient->dev.parent);
    int ret;
    u8 buf[4] = { reg >> 8, reg & 0xff, val >> 8,  val & 0xff};
    struct i2c_msg msg = {
        .addr = gclient->addr, 
				.flags = 0,
        .buf = buf, 
        .len = 4,
    };     
           
    ret = i2c_transfer(adapter, &msg, 1);
           
    if (ret != 1) {
        dprintk("error (addr %02x %02x <- %02x %02x, err = %i)\n",                                                                                                                                                                                                                  
               msg.buf[0], msg.buf[1], msg.buf[2], msg.buf[3], ret);
        if (ret < 0)
            return ret;
        else
            return -EREMOTEIO;
    }      
    return 0;                                                                                                   
}  

static void tx_dcs_short_write_command(unsigned short cmd)
{	
	tc358768xbg_write(0x0602,0x1005); 
	tc358768xbg_write(0x0604,0x0000); 
	tc358768xbg_write(0x0610,cmd); 
	tc358768xbg_write(0x0600,0x0001); 
	udelay(100);	//Wait until packet sending finish
}

static void tx_long_write_command(char* command, unsigned short num)
{

	unsigned short data1,data2,data3,data4 = 0;

	//int count;
	//printf("=====%s num:%d\n",__FUNCTION__, num);
	//for(count=0; count < num; count++)
	//	printf("%s data[%d]:0x%x\n",__FUNCTION__,count, command[count]);
	
	tc358768xbg_write(0x0602,0x4029); //generic long wire 
	//tc358768xbg_write(0x0602,0x4039); //DCS long wire 
	
	tc358768xbg_write(0x0604,num); //num
			
	if(num == 1)
		tc358768xbg_write(0x0610,command[0]); 
	if(num == 2){
		data1 = (command[1] << 8) + (command[0] & 0xFF);
		tc358768xbg_write(0x0610,data1); 
	}
	if(num == 3){
		data1 = (command[1] << 8) + (command[0] & 0xFF);
		data2 = (command[2] & 0xFF);
		tc358768xbg_write(0x0610,data1); 
		tc358768xbg_write(0x0612,data2); 
	}
	if(num == 4){
		data1 = (command[1] << 8) + (command[0] & 0xFF);
		data2 = (command[3] << 8) + (command[2] & 0xFF);
		tc358768xbg_write(0x0610,data1); 
		tc358768xbg_write(0x0612,data2); 
	}
	if(num == 5){
		data1 = (command[1] << 8) + (command[0] & 0xFF);
		data2 = (command[3] << 8) + (command[2] & 0xFF);
		data3 = (command[4] & 0xFF);
		tc358768xbg_write(0x0610,data1); 
		tc358768xbg_write(0x0612,data2); 
		tc358768xbg_write(0x0614,data3); 
	}
	if(num == 6){
		data1 = (command[1] << 8) + (command[0] & 0xFF);
		data2 = (command[3] << 8) + (command[2] & 0xFF);
		data3 = (command[5] << 8) + (command[4] & 0xFF);
		tc358768xbg_write(0x0610,data1); 
		tc358768xbg_write(0x0612,data2); 
		tc358768xbg_write(0x0614,data3); 
	}
	if(num == 7){
		data1 = (command[1] << 8) + (command[0] & 0xFF);
		data2 = (command[3] << 8) + (command[2] & 0xFF);
		data3 = (command[5] << 8) + (command[4] & 0xFF);
		data4 = (command[6] & 0xFF);
		tc358768xbg_write(0x0610,data1); 
		tc358768xbg_write(0x0612,data2); 
		tc358768xbg_write(0x0614,data3); 
		tc358768xbg_write(0x0616,data4); 
	}
	if(num == 8){
		data1 = (command[1] << 8) + (command[0] & 0xFF);
		data2 = (command[3] << 8) + (command[2] & 0xFF);
		data3 = (command[5] << 8) + (command[4] & 0xFF);
		data4 = (command[7] << 8) + (command[6] & 0xFF);
		tc358768xbg_write(0x0610,data1); 
		tc358768xbg_write(0x0612,data2); 
		tc358768xbg_write(0x0614,data3); 
		tc358768xbg_write(0x0616,data4); 
	}

	tc358768xbg_write(0x0600,0x0001); 

	udelay(100); //Wait until packet sending finish (unit us)

}

static void lcd_Peripheral_set(void)
{	
	tx_long_write_command(cmd0, sizeof(cmd0));    
	tx_long_write_command(cmd1, sizeof(cmd1));    
	tx_long_write_command(cmd2, sizeof(cmd2));    
	tx_long_write_command(cmd3, sizeof(cmd3));    
	tx_long_write_command(cmd4, sizeof(cmd4));    
	tx_long_write_command(cmd5, sizeof(cmd5));    
	tx_long_write_command(cmd6, sizeof(cmd6));    
	tx_long_write_command(cmd7, sizeof(cmd7));    
	tx_long_write_command(cmd8, sizeof(cmd8));    
	tx_long_write_command(cmd9, sizeof(cmd9));    
	tx_long_write_command(cmd10, sizeof(cmd10));  
	tx_long_write_command(cmd11, sizeof(cmd11));  
	tx_long_write_command(cmd12, sizeof(cmd12));  
	tx_long_write_command(cmd13, sizeof(cmd13));  
	tx_long_write_command(cmd14, sizeof(cmd14));  
	tx_long_write_command(cmd15, sizeof(cmd15));  
	tx_long_write_command(cmd16, sizeof(cmd16));  
	tx_long_write_command(cmd17, sizeof(cmd17));  
	tx_long_write_command(cmd18, sizeof(cmd18));  
	tx_long_write_command(cmd19, sizeof(cmd19));  
	tx_long_write_command(cmd20, sizeof(cmd20));  
	tx_long_write_command(cmd21, sizeof(cmd21));  
	tx_long_write_command(cmd22, sizeof(cmd22));  
	tx_long_write_command(cmd23, sizeof(cmd23));  
	tx_long_write_command(cmd24, sizeof(cmd24));  
	tx_long_write_command(cmd25, sizeof(cmd25));  
	tx_long_write_command(cmd26, sizeof(cmd26));  
	tx_long_write_command(cmd27, sizeof(cmd27));  
	tx_long_write_command(cmd28, sizeof(cmd28));  
	tx_long_write_command(cmd29, sizeof(cmd29));  
	tx_long_write_command(cmd30, sizeof(cmd30));  
	tx_long_write_command(cmd31, sizeof(cmd31));  
	tx_long_write_command(cmd32, sizeof(cmd32));  
	tx_long_write_command(cmd33, sizeof(cmd33));  
	tx_long_write_command(cmd34, sizeof(cmd34));  
	tx_long_write_command(cmd35, sizeof(cmd35));  
	tx_long_write_command(cmd36, sizeof(cmd36));  
	tx_long_write_command(cmd37, sizeof(cmd37));  
	tx_long_write_command(cmd38, sizeof(cmd38));  
	tx_long_write_command(cmd39, sizeof(cmd39));  
	tx_long_write_command(cmd40, sizeof(cmd40));  
	tx_long_write_command(cmd41, sizeof(cmd41));  
	tx_long_write_command(cmd42, sizeof(cmd42));  
	tx_long_write_command(cmd43, sizeof(cmd43));  
	tx_long_write_command(cmd44, sizeof(cmd44));  
	tx_long_write_command(cmd45, sizeof(cmd45));  
	tx_long_write_command(cmd46, sizeof(cmd46));  
	tx_long_write_command(cmd47, sizeof(cmd47));  
	tx_long_write_command(cmd48, sizeof(cmd48));  
	tx_long_write_command(cmd49, sizeof(cmd49));  
	tx_long_write_command(cmd50, sizeof(cmd50));  
	tx_long_write_command(cmd51, sizeof(cmd51));  
	tx_long_write_command(cmd52, sizeof(cmd52));  
	tx_long_write_command(cmd53, sizeof(cmd53));  
	tx_long_write_command(cmd54, sizeof(cmd54));  
	tx_long_write_command(cmd55, sizeof(cmd55));  
	tx_long_write_command(cmd56, sizeof(cmd56));  
	tx_long_write_command(cmd57, sizeof(cmd57));  
	tx_long_write_command(cmd58, sizeof(cmd58));  
	tx_long_write_command(cmd59, sizeof(cmd59));  
	tx_long_write_command(cmd60, sizeof(cmd60));  
	tx_long_write_command(cmd61, sizeof(cmd61));  
	tx_long_write_command(cmd62, sizeof(cmd62));  
	tx_long_write_command(cmd63, sizeof(cmd63));  
	tx_long_write_command(cmd64, sizeof(cmd64));  
	tx_long_write_command(cmd65, sizeof(cmd65));  
	tx_long_write_command(cmd66, sizeof(cmd66));  
	tx_long_write_command(cmd67, sizeof(cmd67));  
	tx_long_write_command(cmd68, sizeof(cmd68));  
	tx_long_write_command(cmd69, sizeof(cmd69));  
	tx_long_write_command(cmd70, sizeof(cmd70));  
	tx_long_write_command(cmd71, sizeof(cmd71));  
	tx_long_write_command(cmd72, sizeof(cmd72));  
	tx_long_write_command(cmd73, sizeof(cmd73));  
	tx_long_write_command(cmd74, sizeof(cmd74));  
	tx_long_write_command(cmd75, sizeof(cmd75));  
	tx_long_write_command(cmd76, sizeof(cmd76));  
	tx_long_write_command(cmd77, sizeof(cmd77));  
	tx_long_write_command(cmd78, sizeof(cmd78));  
	tx_long_write_command(cmd79, sizeof(cmd79));  
	tx_long_write_command(cmd80, sizeof(cmd80));  
	tx_long_write_command(cmd81, sizeof(cmd81));  
	tx_long_write_command(cmd82, sizeof(cmd82));  
	tx_long_write_command(cmd83, sizeof(cmd83));  
	tx_long_write_command(cmd84, sizeof(cmd84));  
	tx_long_write_command(cmd85, sizeof(cmd85));  
	tx_long_write_command(cmd86, sizeof(cmd86));  
	tx_long_write_command(cmd87, sizeof(cmd87));  
	tx_long_write_command(cmd88, sizeof(cmd88));  
	tx_long_write_command(cmd89, sizeof(cmd89));  
	tx_long_write_command(cmd90, sizeof(cmd90));  
	tx_long_write_command(cmd91, sizeof(cmd91));  
	tx_long_write_command(cmd92, sizeof(cmd92));  
	tx_long_write_command(cmd93, sizeof(cmd93));  
	tx_long_write_command(cmd94, sizeof(cmd94));  
	tx_long_write_command(cmd95, sizeof(cmd95));  
	tx_long_write_command(cmd96, sizeof(cmd96));  
	tx_long_write_command(cmd97, sizeof(cmd97));  
	tx_long_write_command(cmd98, sizeof(cmd98));  
	tx_long_write_command(cmd99, sizeof(cmd99));  
	tx_long_write_command(cmd100, sizeof(cmd100));
	tx_long_write_command(cmd101, sizeof(cmd101));
	tx_long_write_command(cmd102, sizeof(cmd102));
	tx_long_write_command(cmd103, sizeof(cmd103));
	tx_long_write_command(cmd104, sizeof(cmd104));
	tx_long_write_command(cmd105, sizeof(cmd105));
	tx_long_write_command(cmd106, sizeof(cmd106));
	tx_long_write_command(cmd107, sizeof(cmd107));
	tx_long_write_command(cmd108, sizeof(cmd108));
	tx_long_write_command(cmd109, sizeof(cmd109));
	tx_long_write_command(cmd110, sizeof(cmd110));
	tx_long_write_command(cmd111, sizeof(cmd111));
	tx_long_write_command(cmd112, sizeof(cmd112));
	tx_long_write_command(cmd113, sizeof(cmd113));
	tx_long_write_command(cmd114, sizeof(cmd114));
	tx_long_write_command(cmd115, sizeof(cmd115));
	tx_long_write_command(cmd116, sizeof(cmd116));
	tx_long_write_command(cmd117, sizeof(cmd117));
	tx_long_write_command(cmd118, sizeof(cmd118));
	tx_long_write_command(cmd119, sizeof(cmd119));
	tx_long_write_command(cmd120, sizeof(cmd120));
	tx_long_write_command(cmd121, sizeof(cmd121));
	tx_long_write_command(cmd122, sizeof(cmd122));
	tx_long_write_command(cmd123, sizeof(cmd123));
	tx_long_write_command(cmd124, sizeof(cmd124));
	tx_long_write_command(cmd125, sizeof(cmd125));
	tx_long_write_command(cmd126, sizeof(cmd126));
	tx_long_write_command(cmd127, sizeof(cmd127));
	tx_long_write_command(cmd128, sizeof(cmd128));
	tx_long_write_command(cmd129, sizeof(cmd129));
	tx_long_write_command(cmd130, sizeof(cmd130));
	tx_long_write_command(cmd131, sizeof(cmd131));
	tx_long_write_command(cmd132, sizeof(cmd132));
	tx_long_write_command(cmd133, sizeof(cmd133));
	tx_long_write_command(cmd134, sizeof(cmd134));
	tx_long_write_command(cmd135, sizeof(cmd135));
	tx_long_write_command(cmd136, sizeof(cmd136));
	tx_long_write_command(cmd137, sizeof(cmd137));
	tx_long_write_command(cmd138, sizeof(cmd138));
	tx_long_write_command(cmd139, sizeof(cmd139));
	tx_long_write_command(cmd140, sizeof(cmd140));
	tx_long_write_command(cmd141, sizeof(cmd141));
	tx_long_write_command(cmd142, sizeof(cmd142));
	tx_long_write_command(cmd143, sizeof(cmd143));
	tx_long_write_command(cmd144, sizeof(cmd144));
	tx_long_write_command(cmd145, sizeof(cmd145));
	tx_long_write_command(cmd146, sizeof(cmd146));
	tx_long_write_command(cmd147, sizeof(cmd147));

	tx_dcs_short_write_command(0x0011);
	tx_dcs_short_write_command(0x0029);
}

static void tc358768xbg_reset(void)
{
	gpio_set_value(GPIO_MIPI_PWEN,0);
	gpio_set_value(TC358768XBG_RESX, 0);
	msleep(1);
	gpio_set_value(GPIO_MIPI_PWEN,1);
	gpio_set_value(TC358768XBG_RESX, 1);
}
static void tc358768xbg_init(void)
{

	tc358768xbg_reset();
	
	//lcd_reset(); //LCD Pannel reset
	
	//device ID
	//while(1){
	//	dprintk("%s: Device ID:0x%x\n",__FUNCTION__,tc358768xbg_read(0x0000));		
	//}

	//tc358768xbg Software Reset
	tc358768xbg_write(0x0002,0001);
	udelay(10);
	tc358768xbg_write(0x0002,0000);

	//tc358768xbg PLL,Clock Setting
	tc358768xbg_write(0x0016,	0x0023);
	tc358768xbg_write(0x0018,	0x0603);
	msleep(1);
	tc358768xbg_write(0x0018,	0x0613);
		

	//tc358768xbg DPI Input Control	
	tc358768xbg_write(0x0006,	0x0050);
	

	//tc358768xbg D-PHY Setting	
	tc358768xbg_write(0x0140,	0x0000);
	tc358768xbg_write(0x0142,	0x0000);
	tc358768xbg_write(0x0144,	0x0000);
	tc358768xbg_write(0x0146,	0x0000);
	tc358768xbg_write(0x0148,	0x0000);
	tc358768xbg_write(0x014A,	0x0000);
	tc358768xbg_write(0x014C,	0x0000);
	tc358768xbg_write(0x014E,	0x0000);
	tc358768xbg_write(0x0150,	0x0000);
	tc358768xbg_write(0x0152,	0x0000);
		                          
	tc358768xbg_write(0x0100,	0x0002);
	tc358768xbg_write(0x0102,	0x0000);
	tc358768xbg_write(0x0104,	0x0002);
	tc358768xbg_write(0x0106,	0x0000);
	tc358768xbg_write(0x0108,	0x0002);
	tc358768xbg_write(0x010A,	0x0000);
	tc358768xbg_write(0x010C,	0x0002);
	tc358768xbg_write(0x010E,	0x0000);
	tc358768xbg_write(0x0110,	0x0002);
	tc358768xbg_write(0x0112,0x0000);
		
	
	//tc358768xbg DSI-TX PPI Control		
	tc358768xbg_write(0x0210,	0x1068); 
	tc358768xbg_write(0x0212,	0x0000); 
	tc358768xbg_write(0x0214,	0x0003); 
	tc358768xbg_write(0x0216,	0x0000); 
	tc358768xbg_write(0x0218,	0x1503); 
	tc358768xbg_write(0x021A,	0x0000); 
	//tc358768xbg_write(0x021C,	0x0001); 
	//tc358768xbg_write(0x021E,	0x0000);
	tc358768xbg_write(0x0220,	0x0003);
	tc358768xbg_write(0x0222,	0x0000);
	tc358768xbg_write(0x0224,	0x4A38);
	tc358768xbg_write(0x0226,	0x0000);
	//tc358768xbg_write(0x0228,	0x0008);
	//tc358768xbg_write(0x022A,	0x0000);
	tc358768xbg_write(0x022C,	0x0001);
	tc358768xbg_write(0x022E,	0x0000);
	tc358768xbg_write(0x0230,	0x0005);
	tc358768xbg_write(0x0232,	0x0000);
	tc358768xbg_write(0x0234,	0x001F);
	tc358768xbg_write(0x0236,	0x0000);
	tc358768xbg_write(0x0238,	0x0001);
	tc358768xbg_write(0x023A,	0x0000);
	tc358768xbg_write(0x023C,	0x0003);
	tc358768xbg_write(0x023E,	0x0003);
	tc358768xbg_write(0x0204,	0x0001);
	tc358768xbg_write(0x0206,	0x0000);
		                              

	//tc358768xbg DSI-TX Timing Control		
	tc358768xbg_write(0x0620,	0x0001);
	tc358768xbg_write(0x0622,	0x0008);
	tc358768xbg_write(0x0624,	0x0004);
	tc358768xbg_write(0x0626,	0x0500);
	//tc358768xbg_write(0x0628,	0x01F9);
	//tc358768xbg_write(0x062A,	0x017B);
	//tc358768xbg_write(0x0628,	0x0200); //Pixel clock is 76Mhz
	//tc358768xbg_write(0x062A,	0x0180); //Pixel clock is 76Mhz
	tc358768xbg_write(0x0628,	0x01FE); //Pixel clock is 76.3Mhz
	tc358768xbg_write(0x062A,	0x017F); //Pixel clock is 76.3Mhz
#ifdef RGB565
	tc358768xbg_write(0x062C,	0x0640);
#else
	tc358768xbg_write(0x062C,	0x0960);
#endif

	tc358768xbg_write(0x0518,	0x0001);
	tc358768xbg_write(0x051A,	0x0000);
	
	lcd_Peripheral_set();

	//Set to HS mode		
	tc358768xbg_write(0x0500,	0x0086);
	tc358768xbg_write(0x0502,	0xA300);
	tc358768xbg_write(0x0500,	0x8000);
	tc358768xbg_write(0x0502,	0xC300);

#ifdef RGB565
	//Host: RGB(DPI) input start			
	tc358768xbg_write(0x0008,	0x0057);
	tc358768xbg_write(0x0050,	0x000E);
	tc358768xbg_write(0x0032,	0x0000);
	tc358768xbg_write(0x0004,	0x0044);

#else
	//Host: RGB(DPI) input start			
	tc358768xbg_write(0x0008,	0x0037);
	tc358768xbg_write(0x0050,	0x003E);
	tc358768xbg_write(0x0032,	0x0000);
	tc358768xbg_write(0x0004,	0x0044);
#endif	


}

static int lcd_n070icn_panel_init(struct lcd_panel *panel)
{
  dprintk("[lcd] %s\n", __func__);

  lcdc_initialize(panel, 1);    
	
	return 0;
}

static void n070icn_dev_init(void)
{
	dprintk("[lcd] %s\n", __func__);

	tc358768xbg_init();
}

static void lcd_on(void)
{  
		dprintk("[lcd] %s\n", __func__);

    n070icn_dev_init();
}

static int lcd_n070icn_set_power(struct lcd_panel *panel, int on, unsigned int lcd_num)
{
	struct lcd_platform_data *pdata = panel->dev->platform_data;
  dprintk("[lcd] %s: on:%d\n", __func__,on);

    mutex_lock(&panel_lock);

	 /*
	 	refer to Board-tcc8930.h, Board-tcc8930-panel.c
	 	
	 	.power_on	= GPIO_LCD_ON,				//TCC_GPE(30)
		.display_on	= GPIO_LCD_DISPLAY,	//0xFFFFFFFF
		.bl_on		= GPIO_LCD_BL,				//TCC_GPB(29)
		.reset		= GPIO_LCD_RESET,			//TCC_GPE(29)
	*/	

	if (on) {

		gpio_set_value(GPIO_MIPI_PWEN,1);
		gpio_set_value(pdata->power_on, 1);
    mdelay(50);
		gpio_set_value(pdata->reset, 1);
		
		lcd_on();	
		
		lcdc_initialize(panel, lcd_num);
		LCDC_IO_Set(1, panel->bus_width);                

		msleep(50);
	}
	else {

		//gpio_direction_output(pdata->bl_on, 1);
 		//tcc_gpio_config(pdata->bl_on,  GPIO_FN(0));
		//gpio_set_value(pdata->bl_on, 0);
		
		gpio_set_value(pdata->reset, 0);
		gpio_set_value(pdata->power_on, 0);
		gpio_set_value(GPIO_MIPI_PWEN,0);
		
		LCDC_IO_Disable(1, panel->bus_width);        		
	}

    panel->state = on;

    mutex_unlock(&panel_lock);
		
    if (on)
        panel->set_backlight_level(panel, panel->bl_level);
    
	return 0;
}


static int lcd_n070icn_set_brightness(struct lcd_panel *panel, int level)
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
			level = level*15+80;  // (255-80)/11 = 15
		}
		
	  dprintk("%s level is:%d(%d)\n", __func__,level,org_level);
	  
//#if (CUR_AK == MODEL_AK500N)
// 	extern void set_bar_led_brightness(int val);
//	if(org_level != 0)
//		set_bar_led_brightness(org_level);
//#endif

    mutex_lock(&panel_lock);
         

    if (level == 0) {
#if (CUR_AK_REV <= AK500N_HW_ES1)            
        gpio_set_value(GPIO_PWR_ON_INDICATOR,0);
#endif
        tca_tco_pwm_ctrl(1, pdata->bl_on, MAX_BACKLIGTH, level);				
    }    
    else 
    {    
        if(panel->state)
        {
        	msleep(100);
	        tca_tco_pwm_ctrl(1, pdata->bl_on, MAX_BACKLIGTH, level);
#if (CUR_AK_REV <= AK500N_HW_ES1)                
            gpio_set_value(GPIO_PWR_ON_INDICATOR,1);
#endif
        }
    }    
         
    mutex_unlock(&panel_lock);
    return 0;

}

static struct lcd_panel n070icn_panel = {
#if 1	
 .name  = "N070ICN_GB1",
 .manufacturer = "InnoLux",
 .id  = PANEL_ID_N070ICN,
 .xres  = 800, // fix
 .yres  = 1280, // fix
 .width  = 800, // fix
 .height  = 1280, // fix
 .bpp  = 32, // fix
// .clk_freq = 505000,
// .clk_freq = 770000*2,
  .clk_freq = 700000*2, //76Mz
// .clk_freq = 770000,
 .clk_div = 2,//2,
 .bus_width = 24, // fix
 .lpw  = 40,
 .lpc  = 799, //fix
 .lswc  = 200,
 .lewc  = 50,
 .vdb  = 0,
 .vdf  = 0,
 .fpw1  = 4,
 .flc1  = 1279, // don't care
 .fswc1  = 4,//19,
 .fewc1  = 8,
 .fpw2  = 4,
 .flc2  = 1280, // don't care
 .fswc2  = 4,
 .fewc2  = 8,
 .sync_invert = IV_INVERT | IH_INVERT,
 .init  = lcd_n070icn_panel_init,
 .set_power = lcd_n070icn_set_power,
 .set_backlight_level = lcd_n070icn_set_brightness,
#else
 .name  = "N070ICN_GB1",
 .manufacturer = "InnoLux",
 .id  = PANEL_ID_N070ICN,
 .xres  = 800,// fix
 .yres  = 1280, // fix
 .width  = 800, // fix
 .height  = 1280, // fix
 .bpp  = 32, // fix
 //.clk_freq = 505000,
 .clk_freq = 770000*2,
 .clk_div = 2,//2,
 .bus_width = 24, // fix
 .lpw  = 40,
 .lpc  = 799, //fix
 .lswc  = 120,
 .lewc  = 30,
 .vdb  = 4,// Back porch VSYNC delay
 .vdf  = 8,// Front porch of VSYNC delay
 .fpw1  = 4,
 .flc1  = 1279, // don't care
 .fswc1  = 4,//19,
 .fewc1  = 2,
 .fpw2  = 1,
 .flc2  = 1279, // don't care
 .fswc2  = 8,
 .fewc2  = 2,
//.sync_invert = IV_INVERT | IH_INVERT | ID_INVERT,
 .sync_invert = IV_INVERT | IH_INVERT,
// .sync_invert = IP_INVERT,    
 .init  = lcd_n070icn_panel_init,
 .set_power = lcd_n070icn_set_power,
 .set_backlight_level = lcd_n070icn_set_brightness,
#endif
};

static void lcd_n070icn_early_suspend(struct early_suspend *h)
{	
	dprintk("%s\n",__FUNCTION__);
	//gpio_set_value(GPIO_PWR_ON_INDICATOR,0);
}

static void lcd_n070icn_late_resume(struct early_suspend *h)
{
	dprintk("%s\n",__FUNCTION__);
	//gpio_set_value(GPIO_PWR_ON_INDICATOR,1);
}


static int n070icn_probe(struct platform_device *pdev)
{
  dprintk("[lcd] %s\n", __func__);
    
	mutex_init(&panel_lock);
    
	
	n070icn_panel.state = 1;
	n070icn_panel.dev = &pdev->dev;
	tccfb_register_panel(&n070icn_panel);

#ifdef DONOT_SLEEP	
  wake_lock_init(&lcd_n070icn_lock, WAKE_LOCK_SUSPEND, "lcd_n070icn_lock");   
	wake_lock(&lcd_n070icn_lock);
#endif

	lcd_moudle_early_suspend.suspend = lcd_n070icn_early_suspend;
	lcd_moudle_early_suspend.resume = lcd_n070icn_late_resume;
	lcd_moudle_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;	
	register_early_suspend(&lcd_moudle_early_suspend);

#if (CUR_AK_REV <= AK500N_HW_ES1)        
	gpio_request(GPIO_PWR_ON_INDICATOR, "GPIO_PWR_ON_INDICATOR");
	tcc_gpio_config(GPIO_PWR_ON_INDICATOR, GPIO_FN(0));
	gpio_direction_output(GPIO_PWR_ON_INDICATOR, 1);	
#endif
    
	gpio_request(GPIO_BLU_EN, "GPIO_GPIO_BLU_EN");
	tcc_gpio_config(GPIO_BLU_EN, GPIO_FN(0));
	gpio_direction_output(GPIO_BLU_EN, 1);	
	
	return 0;
}

static int n070icn_remove(struct platform_device *pdev)
{
  dprintk("[lcd] %s\n", __func__);
	return 0;
}

static struct platform_driver n070icn_lcd = {
	.probe	= n070icn_probe,
	.remove	= n070icn_remove,
	.driver	= {
		.name	= "n070icn_lcd",
		.owner	= THIS_MODULE,
	},
};

static const struct i2c_device_id tc358768xbg_id[] = {
	{ "tc358768xbg", 0 },
	{ }
};
	

static int tc358768xbg_probe(struct i2c_client *client, const struct i2c_device_id *i2c_id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct tc358768xbg_data *data;
		
	dprintk("[lcd] %s\n", __func__);	  

	gpio_request(TC358768XBG_RESX, "TC358768XBG_RESX");
	tcc_gpio_config(TC358768XBG_RESX, GPIO_FN(0));
	gpio_direction_output(TC358768XBG_RESX, 1);


	gpio_request(GPIO_MIPI_PWEN, "GPIO_MIPI_PWEN");
	tcc_gpio_config(GPIO_MIPI_PWEN, GPIO_FN(0));
	gpio_direction_output(GPIO_MIPI_PWEN, 1);
	

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WORD_DATA))
	    return -EIO;
	         
	data = kzalloc(sizeof(struct tc358768xbg_data), GFP_KERNEL);
	if (!data)
	    return -ENOMEM;

  data->client = client;
  i2c_set_clientdata(client, data);			

	gclient = client;
	
#if 0
	while(1){
		//dprintk("g_i2c_addr: 0x%x\n",g_i2c_addr);
		dprintk("%s: Device ID:0x%x\n",__FUNCTION__,tc358768xbg_read(0x0000));		
		msleep(2000);
		//tc358768xbg_read(0x0000);
		//tc358768xbg_read(client,0x0000);

		dprintk("Display on\n");
		tx_dcs_short_write_command(0x0028); //Display off
		tx_dcs_short_write_command(0x0010); //enter_sleep_mode

		dprintk("Display off\n");
		msleep(2000);
		tx_dcs_short_write_command(0x0011); //exit_sleep_mode
		msleep(120); //Delay 120ms or more
		tx_dcs_short_write_command(0x0029); //Display on
		
	}
#endif

	return 0;
}

static int tc358768xbg_remove(struct i2c_client *client)
{
  dprintk("[lcd] %s\n", __func__);
	return 0;
}

static int tc358768xbg_suspend(struct i2c_client *client, pm_message_t mesg)
{
  dprintk("[lcd] %s\n", __func__);

	return 0;
}

static int tc358768xbg_resume(struct i2c_client *client)
{
  dprintk("[lcd] %s\n", __func__);
	
	return 0;
}

static struct i2c_driver tc358768xbg_i2c_driver = {
	.probe = tc358768xbg_probe,
	.remove = tc358768xbg_remove,
	.suspend	= tc358768xbg_suspend,	
	.resume	 = tc358768xbg_resume,
	.driver		= {
		.name	= "tc358768xbg",
	},
	.id_table	= tc358768xbg_id,
};
MODULE_DEVICE_TABLE(i2c, tc358768xbg_i2c_driver);

static __init int n070icn_init(void)
{
  dprintk("[lcd] %s\n", __func__);

	i2c_add_driver(&tc358768xbg_i2c_driver);
	return platform_driver_register(&n070icn_lcd);
}

static __exit void n070icn_exit(void)
{
	platform_driver_unregister(&n070icn_lcd);
}

subsys_initcall(n070icn_init);
module_exit(n070icn_exit);

MODULE_DESCRIPTION("N070ICN LCD driver");
MODULE_LICENSE("GPL");
