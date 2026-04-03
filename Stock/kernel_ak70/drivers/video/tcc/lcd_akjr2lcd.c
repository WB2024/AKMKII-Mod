/*
 * lcd_akjr2.c
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
#include <asm/io.h>

#include <mach/tcc_fb.h>
#include <mach/gpio.h>
#include <mach/tca_lcdc.h>
#include <mach/TCC_LCD_Interface.h>
#include <mach/ak_gpio_def.h>
#include <mach/board_astell_kern.h>
#include <mach/vioc_disp.h>
#include <mach/bsp.h>

#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/power_supply.h>

#include <mach/tca_tco.h>

#include <linux/earlysuspend.h>

#include <mach/board-tcc8930.h>

 
static struct mutex panel_lock;

static struct early_suspend lcd_moudle_early_suspend;

static int debug = 0;

#define PWM_CH 0

#define dbg_printf(msg...)	if (debug) { printk( "lcd_akjr2: " msg); }

//#define DONOT_SLEEP

#ifdef DONOT_SLEEP
static struct wake_lock lcd_akjr2_lock;
#endif

#define PCLK_33MHZ

//(slave address=7"b0000_111X)
#define SLAVE_ADDR_TC358768XBG	0x0E  ////i2c_xfer fucntion process W:0x0E/R:0x0F 
#define I2C_CH I2C_CH_MASTER2

/* dcs read/write */
#define DTYPE_DCS_WRITE		0x05	/* short write, 0 parameter */
#define DTYPE_DCS_WRITE1	0x15	/* short write, 1 parameter */
#define DTYPE_DCS_READ		0x06	/* read */
#define DTYPE_DCS_LWRITE	0x39	/* long write */

/* generic read/write */
#define DTYPE_GEN_WRITE		0x03	/* short write, 0 parameter */
#define DTYPE_GEN_WRITE1	0x13	/* short write, 1 parameter */
#define DTYPE_GEN_WRITE2	0x23	/* short write, 2 parameter */
#define DTYPE_GEN_LWRITE	0x29	/* long write */
#define DTYPE_GEN_READ		0x04	/* long read, 0 parameter */
#define DTYPE_GEN_READ1		0x14	/* long read, 1 parameter */
#define DTYPE_GEN_READ2		0x24	/* long read, 2 parameter */

#define DTYPE_TEAR_ON		0x35	/* set tear on */
#define DTYPE_MAX_PKTSIZE	0x37	/* set max packet size */
#define DTYPE_NULL_PKT		0x09	/* null packet, no data */
#define DTYPE_BLANK_PKT		0x19	/* blankiing packet, no data */

#define DTYPE_CM_ON		0x02	/* color mode off */
#define DTYPE_CM_OFF		0x12	/* color mode on */
#define DTYPE_PERIPHERAL_OFF	0x22
#define DTYPE_PERIPHERAL_ON	0x32

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define mipi_dsi_send_packet(data_type, data) _mipi_dsi_send_packet(data_type, data, ARRAY_SIZE(data))


//#define RGB565

#define LCD_PW_EN			TCC_GPG(4)
#define MIPI_PWR_EN 		TCC_GPF(3)
#define MIPI_RESET			TCC_GPE(22)
#define LED_PWM				TCC_GPE(16)  //PAD NAME: GPIO_E20 
#define LCD_RESET				TCC_GPD(9) 
//#define LCD_RESET			TCC_GPB(28) 


extern void lcdc_initialize(struct lcd_panel *lcd_spec, unsigned int lcdc_num);
static unsigned char enter_sleep_mode[] = {0x10};
static unsigned char exit_sleep_mode[] = {0x11};
static unsigned char set_diaplay_on[] = {0x29};

//Data type : 0x15 - DCS write , parameter 1, prarmeter val 0xC0                                                                         
//Command type: 0x36 - set_address_mode
//Parameter 1: 0xC0 
static unsigned char set_addr_mode[] = {0x36,0x00};

//Data type : 0x15 - DCS write , parameter 1, prarmeter val 0x70                                                                         
//Command type: 0x3A - set_pixel_format
//Parameter 1: 0x70 
static unsigned char set_pixel[] = {0x3A, 0x70};

//Data type : 0x23 -Generic write , parameter 2                                                                         
//Parameter 1: 0xB0 
//Parameter 2: 0x04
static unsigned char pannel_manufacture_access[] = {0xB0, 0x04};
static unsigned char pannel_pixel_format[] = {0xB3, 0x00, 0x80};
static unsigned char pannel_dsi_control[] = {0xB6, 0x32, 0x83};
static unsigned char pannel_driving_set[] = {0xC0, 0x01, 0x56, 0x65, 0x00, 0x00, 0x00};
static unsigned char pannel_V_timimg_set[] = {0xC1, 0x00, 0x10, 0x00, 0x01, 0x12};
static unsigned char pannel_control_set[] = {0xC3, 0x00, 0x05};
static unsigned char pannel_ltps_mode[] = {0xC4, 0x03}; 
static unsigned char pannel_H_timming_set[] = {0xC5, 0x40, 0x01, 0x06, 0x01, 0x76, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x1E, 0x04, 0x00, 0x00}; 
//static unsigned char pannel_gamma_setA[] = {0xC8, 0x0D, 0x1B, 0x19, 0x1C, 0x20, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 
//static unsigned char pannel_gamma_setB[] = {0xC9, 0x0D, 0x1F, 0x1F, 0x1E, 0x22, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 
//static unsigned char pannel_gamma_setC[] = {0xCA, 0xCD, 0x54, 0x4E, 0x0F, 0x17, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 
static unsigned char pannel_gamma_setA[] = {0xC8, 0x0D, 0x1B, 0x19, 0x1C, 0x20, 0x15, 0x00}; 
static unsigned char pannel_gamma_setB[] = {0xC9, 0x0D, 0x1F, 0x1F, 0x1E, 0x22, 0x17, 0x00}; 
static unsigned char pannel_gamma_setC[] = {0xCA, 0xCD, 0x54, 0x4E, 0x0F, 0x17, 0x14, 0x00}; 

static unsigned char pannel_power_set_CP[] = {0xD0, 0x22, 0x24, 0xA3, 0xB8}; 
static unsigned char pannel_test_mode1[] = {0xD1, 0x00, 0x00, 0x00, 0x00, 0x00}; 
static unsigned char pannel_power_set_SA[] = {0xD2, 0xB3, 0x00}; 
static unsigned char pannel_power_set_IPSC[] = {0xD3, 0x33, 0x03}; 
static unsigned char pannel_vreg_set[] = {0xD5, 0x06}; 
static unsigned char pannel_test_mode2[] = {0xD6, 0x01}; 
//static unsigned char pannel_sequencer_timming_control_power_on[] = {0xD7, 0x09, 0x00, 0x84, 0x81, 0x61, 0xBC, 0xB5, 0x05}; 
static unsigned char pannel_sequencer_timming_control_power_on[] = {0xD7, 0x09, 0x00, 0x84, 0x81, 0x61, 0xBC, 0xB5 }; 
static unsigned char pannel_sequencer_timming_control_power_off[] = {0xD8, 0x04, 0x25, 0x90, 0x4C, 0x92, 0x00}; 
static unsigned char pannel_power_on_off[] = {0xD9, 0x5B, 0x7F, 0x00}; 
static unsigned char pannel_vcs_set[] = {0xDD, 0x3C}; 
static unsigned char pannel_vcomdc_set[] = {0xDE, 0x42}; 
static unsigned char pannel_manufacture_access_protect[] = {0xB0, 0x03}; 


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
		
  //dbg_printf("tc358768xbg 0x%04x => 0x%04x\n\n", reg, read_val);
  
  if(ret <= 0) {
      dbg_printf("error (addr %02x reg %04x error (ret == %i)\n",
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
        dbg_printf("error (addr %02x %02x <- %02x %02x, err = %i)\n",                                                                                                                                                                                                                  
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

static unsigned int tc358768_wr_reg_32bits(unsigned int data) 
{	
	unsigned short reg, val;
	reg = (data >> 16) & 0xFFFF;
	val = data & 0xFFFF;

	//printk("%s: data:0x%08x, 0x%04x = 0x%04x\n",__FUNCTION__,data, reg, val);
	
	tc358768xbg_write(reg,val);

	return 0;
}

static unsigned int tc358768_rd_reg_32bits(unsigned int addr) 
{
	unsigned int val;
	//return i2c_read_32bits(addr);
	val=tc358768xbg_read(addr);

	//printf("[%s]val:0x%x\n", __FUNCTION__,val);
	return val;
}

#define tc358768_wr_regs_32bits(reg_array)  _tc358768_wr_regs_32bits(reg_array, ARRAY_SIZE(reg_array))
static int _tc358768_wr_regs_32bits(unsigned int reg_array[], int n) 
{
	int i = 0;

	for(i = 0; i < n; i++) {
		//dbg_printf("[%s]0x%x, %d\n", __func__, reg_array[i],n);
		if(reg_array[i] < 0x00020000) {
		    if(reg_array[i] < 20000)
		    	udelay(reg_array[i]);
		    else {
		    	mdelay(reg_array[i]/1000);
		    }
		} else {
			tc358768_wr_reg_32bits(reg_array[i]);
		}
	}
	return 0;
}

static int tc358768_command_tx_less8bytes(unsigned char type, unsigned char *regs, int n) 
{
	int i = 0;
	unsigned int command[] = {
			0x06020000,
			0x06040000,
			0x06100000,
			0x06120000,
			0x06140000,
			0x06160000,
	};

	//if(n <= 2)
	if((type !=DTYPE_DCS_LWRITE) && (type !=DTYPE_GEN_LWRITE)) 
		command[0] |= 0x1000;   //short packet
	else {
		command[0] |= 0x4000;   //long packet
		command[1] |= n;		//word count byte
	}

	command[0] |= type;         //data type
	
	dbg_printf("[%s]0x%08x\n", __FUNCTION__,command[0]);
	dbg_printf("[%s]0x%08x\n", __FUNCTION__,command[1]);

	for(i = 0; i < (n + 1)/2; i++) {
		command[i+2] |= regs[i*2];
		if((i*2 + 1) < n)
			command[i+2] |= regs[i*2 + 1] << 8;
		dbg_printf("0x%08x\n",command[i+2]);
	}

	_tc358768_wr_regs_32bits(command, (n + 1)/2 + 2);
	tc358768_wr_reg_32bits(0x06000001);   //Packet Transfer
	//wait until packet is out
#if 1
	udelay(100);
#else
	i = 100;
	while(tc358768_rd_reg_32bits(0x0600) & 0x01) {
		if(i-- == 0)
			break;
	}
#endif	

	return 0;
}

//low power mode only for tc358768a
static int tc358768_command_tx_more8bytes_lp(unsigned char type, unsigned char regs[], int n) 
{
	int i = 0;
	unsigned int dbg_data = 0x00E80000, temp = 0;
	unsigned int command[] = {
			0x00080001,
			0x00500000,    //Data ID setting
			0x00220000,    //Transmission byte count= byte
			0x00E08000,	   //Enable I2C/SPI write to VB
	};

	command[1] |= type;        //data type
	command[2] |= n & 0xffff;           //Transmission byte count

	dbg_printf("[%s]0x%08x\n", __FUNCTION__,command[1]);
	dbg_printf("[%s]0x%08x\n", __FUNCTION__,command[2]);
	
	tc358768_wr_regs_32bits(command);

	for(i = 0; i < (n + 1)/2; i++) {
		temp = dbg_data | regs[i*2];
		if((i*2 + 1) < n)
			temp |= (regs[i*2 + 1] << 8);
		dbg_printf("0x%08x\n", temp);
		tc358768_wr_reg_32bits(temp);
	}
	if((n % 4 == 1) ||  (n % 4 == 2))     //4 bytes align
		tc358768_wr_reg_32bits(dbg_data);

	tc358768_wr_reg_32bits(0x00E0E000);     //Start command transmisison
	udelay(1000);
	tc358768_wr_reg_32bits(0x00E02000);	 //Keep Mask High to prevent short packets send out
	tc358768_wr_reg_32bits(0x00E00000);	 //Stop command transmission. This setting should be done just after above setting to prevent multiple output
	udelay(10);
	return 0;
}


static int _tc358768_send_packet(unsigned char type, unsigned char regs[], int byte_size) 
{
	if(byte_size <= 8) {
		tc358768_command_tx_less8bytes(type, regs, byte_size);
	} else {
		tc358768_command_tx_more8bytes_lp(type, regs, byte_size);
	}

	dbg_printf("\n");
	return 0;
}

void _mipi_dsi_send_packet(unsigned char type, unsigned char *regs, int byte_size)
{
		_tc358768_send_packet(type, regs, byte_size);	
}


static void lcd_Peripheral_set(void)
{	
	mipi_dsi_send_packet(DTYPE_DCS_WRITE1,set_addr_mode);
	mipi_dsi_send_packet(DTYPE_DCS_WRITE1,set_pixel);

	mipi_dsi_send_packet(DTYPE_GEN_WRITE2,pannel_manufacture_access);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_pixel_format);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_dsi_control);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_driving_set);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_V_timimg_set);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_control_set);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_ltps_mode);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_H_timming_set);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_gamma_setA);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_gamma_setB);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_gamma_setC);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_power_set_CP);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_test_mode1);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_power_set_SA);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_power_set_IPSC);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_vreg_set);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_test_mode2);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_sequencer_timming_control_power_on);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_sequencer_timming_control_power_off);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_power_on_off);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_vcs_set);
	mipi_dsi_send_packet(DTYPE_GEN_LWRITE,pannel_vcomdc_set);
	mipi_dsi_send_packet(DTYPE_GEN_WRITE2,pannel_manufacture_access_protect);

	//mipi_dsi_send_packet(DTYPE_DCS_WRITE,exit_sleep_mode);
	//mdelay(120);
	//mipi_dsi_send_packet(DTYPE_DCS_WRITE,set_diaplay_on);
}

static void tc358768xbg_reset(void)
{
	gpio_set_value(LCD_PW_EN,1);
	gpio_set_value(MIPI_PWR_EN,1);
	gpio_set_value(MIPI_RESET, 0);
	msleep(50);
	gpio_set_value(MIPI_RESET, 1); //RESX : TC358768XBG reset
	msleep(120);
}

void lcd_reset()
{	
	gpio_set_value(LCD_RESET,1);
	msleep(1); 
	gpio_set_value(LCD_RESET,0);
	msleep(1); 
	gpio_set_value(LCD_RESET,1);
	msleep(200);;	
}

static void tc358768xbg_init(void)
{

	tc358768xbg_reset();
	
	lcd_reset(); //LCD Pannel reset
	
	//device ID
	//while(1){
	//	dbg_printf("%s: Device ID:0x%x\n",__FUNCTION__,tc358768xbg_read(0x0000));		
	//}

	//tc358768xbg Software Reset
	tc358768xbg_write(0x0002,	0x0001);	//SYSctl, S/W Reset
	udelay(10);
	tc358768xbg_write(0x0002,	0x0000);	//SYSctl, S/W Reset release

	//tc358768xbg PLL,Clock Setting
	tc358768xbg_write(0x0016,	0x2057);  	//PLL Control Register 0 (PLL_PRD,PLL_FBD)
	tc358768xbg_write(0x0018,	0x0603);	//PLL_FRS,PLL_LBWS, PLL oscillation enable
	udelay(1000);
	tc358768xbg_write(0x0018,	0x0613);	//PLL_FRS,PLL_LBWS, PLL clock out enable
		

	//tc358768xbg DPI Input Control	
	tc358768xbg_write(0x0006,	0x0064); //FIFO Control Register	
	

	//tc358768xbg D-PHY Setting	
	tc358768xbg_write(0x0140,	0x0000);	//D-PHY Clock lane enable	
	tc358768xbg_write(0x0142,	0x0000);
	tc358768xbg_write(0x0144,	0x0000);	//D-PHY Data lane0 enable	
	tc358768xbg_write(0x0146,	0x0000);
	tc358768xbg_write(0x0148,	0x0000);	//D-PHY Data lane1 enable	
	tc358768xbg_write(0x014A,	0x0000);
	//akjr2에서는 data lane 2,4 사용 안함
	//tc358768xbg_write(0x014C,	0x0000);	//D-PHY Data lane2 enable		
	//tc358768xbg_write(0x014E,	0x0000);
	//tc358768xbg_write(0x0150,	0x0000); //D-PHY Data lane3 enable	
	//tc358768xbg_write(0x0152,	0x0000);
		                          
	tc358768xbg_write(0x0100,	0x0002); 	//D-PHY Clock lane control
	tc358768xbg_write(0x0102,	0x0000);
	tc358768xbg_write(0x0104,	0x0002);	//D-PHY Data lane0 control
	tc358768xbg_write(0x0106,	0x0000);
	tc358768xbg_write(0x0108,	0x0002);	//D-PHY Data lane1 control
	tc358768xbg_write(0x010A,	0x0000);
	//akjr2에서는 data lane 2,4 사용 안함
	//tc358768xbg_write(0x010C,	0x0002);	//D-PHY Data lane2 control
	//tc358768xbg_write(0x010E,	0x0000);
	//tc358768xbg_write(0x0110,	0x0002);	//D-PHY Data lane3 control
	//tc358768xbg_write(0x0112,	0x0000);
		
	
	//tc358768xbg DSI-TX PPI Control		
	tc358768xbg_write(0x0210,	0x0BB8);	//LINEINITCNT
	tc358768xbg_write(0x0212,	0x0000); 
	tc358768xbg_write(0x0214,	0x0002); 	//LPTXTIMECNT
	tc358768xbg_write(0x0216,	0x0000); 
	tc358768xbg_write(0x0218,	0x0D02); 	//TCLK_HEADERCNT
	tc358768xbg_write(0x021A,	0x0000); 
	//tc358768xbg_write(0x021C,	0x0000); 
	//tc358768xbg_write(0x021E,	0x0000);
	tc358768xbg_write(0x0220,	0x0003);	//THS_HEADERCNT
	tc358768xbg_write(0x0222,	0x0000);
	tc358768xbg_write(0x0224,	0x4A38);	//TWAKEUPCNT
	tc358768xbg_write(0x0226,	0x0000);
	//tc358768xbg_write(0x0228,	0x0006);	//TCLK_POSTCNT
	//tc358768xbg_write(0x022A,	0x0000);
	tc358768xbg_write(0x022C,	0x0001);	//THS_TRAILCNT
	tc358768xbg_write(0x022E,	0x0000);
	tc358768xbg_write(0x0230,	0x0005);	//HSTXVREGCNT
	tc358768xbg_write(0x0232,	0x0000);
	tc358768xbg_write(0x0234,	0x0007);	//HSTXVREGEN
	tc358768xbg_write(0x0236,	0x0000);
	tc358768xbg_write(0x0238,	0x0001);	//DSI clock Enable/Disable during LP
	tc358768xbg_write(0x023A,	0x0000);
	tc358768xbg_write(0x023C,	0x0001);	//BTACNTRL1
	tc358768xbg_write(0x023E,	0x0002);
	tc358768xbg_write(0x0204,	0x0001);	//STARTCNTRL
	tc358768xbg_write(0x0206,	0x0000);
		                              

	//tc358768xbg DSI-TX Timing Control		
	tc358768xbg_write(0x0620,	0x0001);	//Sync Pulse/Sync Event mode setting
	tc358768xbg_write(0x0622,	0x0004);	//V Control Register1
	tc358768xbg_write(0x0624,	0x0003);	//V Control Register2
	tc358768xbg_write(0x0626,	0x0356);	//V Control Register3

	//Pixel clock is 33Mhz
	tc358768xbg_write(0x0628,	0x0078);	//H Control Register1
	tc358768xbg_write(0x062A,	0x005A); 	//H Control Register2


#ifdef RGB565
	tc358768xbg_write(0x062C,	0x03C0);	//H Control Register3
#else
	tc358768xbg_write(0x062C,	0x05A0);	//H Control Register3
#endif

	tc358768xbg_write(0x0518,	0x0001);	//DSI Start
	tc358768xbg_write(0x051A,	0x0000);
	
	//LCDD (Peripheral) Setting	
	lcd_Peripheral_set();

	

	//Set to HS mode		
	tc358768xbg_write(0x0500,	0x0082);	//DSI lane setting, DSI mode=HS
	tc358768xbg_write(0x0502,	0xA300);	//bit set
	tc358768xbg_write(0x0500,	0x8000);	//Switch to DSI mode
	tc358768xbg_write(0x0502,	0xC300);

#ifdef RGB565
	//Host: RGB(DPI) input start			
	tc358768xbg_write(0x0008,	0x0057);
	tc358768xbg_write(0x0050,	0x000E);
	tc358768xbg_write(0x0032,	0x0000);
	tc358768xbg_write(0x0004,	0x0044);

#else
	//Host: RGB(DPI) input start			
	tc358768xbg_write(0x0008,	0x0037);	//DSI-TX Format setting
	tc358768xbg_write(0x0050,	0x003E);	//DSI-TX Pixel stream packet Data Type setting
	tc358768xbg_write(0x0032,	0x0000);	//HSYNC Polarity
	tc358768xbg_write(0x0004,	0x0044);	//Configuration Control Register
#endif	

	mipi_dsi_send_packet(DTYPE_DCS_WRITE,exit_sleep_mode);
	mdelay(120);
	mipi_dsi_send_packet(DTYPE_DCS_WRITE,set_diaplay_on);

}

static int lcd_akjr2_panel_init(struct lcd_panel *panel)
{
  dbg_printf("[lcd] %s\n", __func__);

  lcdc_initialize(panel, 1);    
	
	return 0;
}

static void akjr2_dev_init(void)
{
	dbg_printf("[lcd] %s\n", __func__);

	tc358768xbg_init();
}

static void lcd_on(void)
{  
		dbg_printf("[lcd] %s\n", __func__);

    akjr2_dev_init();
}

static int lcd_akjr2_set_power(struct lcd_panel *panel, int on, unsigned int lcd_num)
{
	struct lcd_platform_data *pdata = panel->dev->platform_data;
  dbg_printf("[lcd] %s: on:%d\n", __func__,on);

    mutex_lock(&panel_lock);

	 /*
	 	refer to Board-tcc8930.h, Board-tcc8930-panel.c
	 	
	 	.power_on	= GPIO_LCD_ON,				//TCC_GPE(23)
		.display_on	= GPIO_LCD_DISPLAY,	//0xFFFFFFFF
		.bl_on		= GPIO_LCD_BL,				//TCC_GPB(29)
		.reset		= GPIO_LCD_RESET,			//TCC_GPD(9)
	*/	

	if (on) {

		gpio_set_value(MIPI_PWR_EN,1);
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
		gpio_set_value(MIPI_PWR_EN,0);
		
		LCDC_IO_Disable(1, panel->bus_width);        		
	}

    panel->state = on;

    mutex_unlock(&panel_lock);
		
    if (on)
        panel->set_backlight_level(panel, panel->bl_level);
    
	return 0;
}

void backlight_off(void)
{
	printk("backlight_off\n");	
	tcc_gpio_config(GPIO_LCD_BL, GPIO_FN(0));
	gpio_set_value(GPIO_LCD_BL, 0);
}
EXPORT_SYMBOL(backlight_off);

static int lcd_akjr2_set_brightness(struct lcd_panel *panel, int level)
{
#define MAX_BACKLIGTH 255
	int org_level;
    struct lcd_platform_data *pdata = panel->dev->platform_data;
	VIOC_DISP * pDISPBase;
		
	pDISPBase = (VIOC_DISP*)tcc_p2v(HwVIOC_DISP0);

#ifdef LCD_SET_BRIGHTNESS
	unsigned char hue = 0;
	unsigned char brightness = 0x30;
	unsigned char contrast = 0x19;

	BITCSET(pDISPBase->uLENH.nREG, 0x00FFFFFF, hue << 16 | brightness << 8 | contrast );
#endif
	
    panel->bl_level = level;
	org_level =level;
		
	if(level >= 11)
		level = MAX_BACKLIGTH;		
	else if(level <= 0)
		level = 0;			
	else{
		//level = level*15+80;  // (255-80)/11 = 15
		level = level*23; 
	}

	printk("%s level is:%d(%d)\n", __func__,level,org_level);

    mutex_lock(&panel_lock);
#if 1
	//pdata->bl_on-> Board-tcc8930.h #define GPIO_LCD_BL 			TCC_GPE(16)  //PAD NAME: GPIO_E20 
    if (level == 0) {
		//gpio_set_value(pdata->display_on, 0);
        tca_tco_pwm_ctrl(PWM_CH, pdata->bl_on, MAX_BACKLIGTH, level);				       
    }    
    else 
    {    
        if(panel->state)
        {
        	msleep(100);
			//gpio_set_value(pdata->display_on, 1);		
			tca_tco_pwm_ctrl(PWM_CH, pdata->bl_on, MAX_BACKLIGTH, level);
        }
    }    
#endif        
    mutex_unlock(&panel_lock);
    return 0;

}

static struct lcd_panel akjr2_panel = {

.name  = "LPM034A259A",
 .manufacturer = "IRIVER",
 .id  = PANEL_ID_AKJR2,
 .xres  = 480, // fix
 .yres  = 854, // fix
 .width  = 480, // fix
 .height  = 854, // fix
 .bpp  = 32, // fix

 .clk_freq = 330000*2, //32Mhz
 .clk_div = 1, // refert to 0x72000008 PXCLKDIV[0:7]  PXCLK = LCLK/(2*PXCLKDIV)
 .bus_width = 24, // fix
 .lpw  = 10-1,
 .lpc  = 480-1, //fix
 .lswc  = 30-1,
 .lewc  = 116-1,
 .vdb  = 0,
 .vdf  = 0,
 .fpw1  =1-1, 
 .flc1  = 854-1, 
 .fswc1  = 3-1,
 .fewc1  = 6-1,
 .fpw2  = 1-1,
 .flc2  = 854-1, 
 .fswc2  = 3-1,
 .fewc2  = 6-1,
 .sync_invert = IV_INVERT | IH_INVERT,
 .init  = lcd_akjr2_panel_init,
 .set_power = lcd_akjr2_set_power,
 .set_backlight_level = lcd_akjr2_set_brightness,

};

static void lcd_akjr2_early_suspend(struct early_suspend *h)
{	
	dbg_printf("%s\n",__FUNCTION__);
	//gpio_set_value(GPIO_PWR_ON_INDICATOR,0);
}

static void lcd_akjr2_late_resume(struct early_suspend *h)
{
	dbg_printf("%s\n",__FUNCTION__);
	//gpio_set_value(GPIO_PWR_ON_INDICATOR,1);
}


static int akjr2_probe(struct platform_device *pdev)
{
  dbg_printf("[lcd] %s\n", __func__);
    
	mutex_init(&panel_lock);
    
	
	akjr2_panel.state = 1;
	akjr2_panel.dev = &pdev->dev;
	tccfb_register_panel(&akjr2_panel);

#ifdef DONOT_SLEEP	
  wake_lock_init(&lcd_akjr2_lock, WAKE_LOCK_SUSPEND, "lcd_akjr2_lock");   
	wake_lock(&lcd_akjr2_lock);
#endif

	lcd_moudle_early_suspend.suspend = lcd_akjr2_early_suspend;
	lcd_moudle_early_suspend.resume = lcd_akjr2_late_resume;
	lcd_moudle_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;	
	register_early_suspend(&lcd_moudle_early_suspend);    
	
	return 0;
}

static int akjr2_remove(struct platform_device *pdev)
{
  dbg_printf("[lcd] %s\n", __func__);
	return 0;
}

static struct platform_driver akjr2_lcd = {
	.probe	= akjr2_probe,
	.remove	= akjr2_remove,
	.driver	= {
		.name	= "lcd_akjr2lcd",
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
		
	dbg_printf("[lcd] %s\n", __func__);	  

	gpio_request(MIPI_RESET, "MIPI_RESET");
	tcc_gpio_config(MIPI_RESET, GPIO_FN(0));
	gpio_direction_output(MIPI_RESET, 1);


	gpio_request(MIPI_PWR_EN, "MIPI_PWR_EN");
	tcc_gpio_config(MIPI_PWR_EN, GPIO_FN(0));
	gpio_direction_output(MIPI_PWR_EN, 1);
	

	gpio_request(GPIO_LCD_PWEN, "LCD_PWEN");
	tcc_gpio_config(GPIO_LCD_PWEN, GPIO_FN(0));
	gpio_direction_output(GPIO_LCD_PWEN, 1);


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
		//dbg_printf("g_i2c_addr: 0x%x\n",g_i2c_addr);
		dbg_printf("%s: Device ID:0x%x\n",__FUNCTION__,tc358768xbg_read(0x0000));		
		msleep(2000);
		//tc358768xbg_read(0x0000);
		//tc358768xbg_read(client,0x0000);

		dbg_printf("Display on\n");
		tx_dcs_short_write_command(0x0028); //Display off
		tx_dcs_short_write_command(0x0010); //enter_sleep_mode

		dbg_printf("Display off\n");
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
  dbg_printf("[lcd] %s\n", __func__);
	return 0;
}

static int tc358768xbg_suspend(struct i2c_client *client, pm_message_t mesg)
{
  dbg_printf("[lcd] %s\n", __func__);

	return 0;
}

static int tc358768xbg_resume(struct i2c_client *client)
{
  dbg_printf("[lcd] %s\n", __func__);
	
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

static __init int akjr2_init(void)
{
  dbg_printf("[lcd] %s\n", __func__);

	i2c_add_driver(&tc358768xbg_i2c_driver);
	return platform_driver_register(&akjr2_lcd);
}

static __exit void akjr2_exit(void)
{
	platform_driver_unregister(&akjr2_lcd);
}

subsys_initcall(akjr2_init);
module_exit(akjr2_exit);

MODULE_DESCRIPTION("AKJR2 LCD driver");
MODULE_LICENSE("GPL");
