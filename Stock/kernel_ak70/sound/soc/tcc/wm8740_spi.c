/*
 * linux/drivers/spi/wm8740_spi.c
 *
 * Author:  <tonny>
 * Created: 1st December, 2009
 * Description: Driver for Telechips SPI Controllers
 *              SPI master mode
 *
 * Copyright (c) iriver, Inc.
 *
 *
 */
 

#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>

#include <asm/io.h>
#include <asm/dma.h>

#include <mach/bsp.h>
//#include "tcc/tca_spi_hwset.h"



#include <linux/fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/miscdevice.h>

#include <mach/gpio.h>


#include <mach/board_astell_kern.h>


#if(CUR_DAC == CODEC_WM8741)


#define REG_WM8740_M0				0x0000
#define REG_WM8740_M1				0x0001
#define REG_WM8740_M2				0x0002
#define REG_WM8740_M3				0x0003
#define REG_WM8740_M4				0x0006


#ifdef AK_USE_WM8741
#define WM8740_REG_TOTAL_NUM 	11
#else
#define WM8740_REG_TOTAL_NUM 	5
#endif

#if 0
#define IOC_IR_MAGIC				('w')

#define IOCTL_WM8740_INIT				_IOW(IOC_IR_MAGIC, 0xa0, int)
#define IOCTL_WM8740_RESET			_IO(IOC_IR_MAGIC, 0xa1)
#define IOCTL_WM8740_SET_CONFIG		_IOW(IOC_IR_MAGIC, 0xb0, int)
#define IOCTL_WM8740_TEST				_IOR(IOC_IR_MAGIC, 0xb1, int)
#define IOCTL_WM8740_SET_VOLUME		_IOWR(IOC_IR_MAGIC, 0xc0, int)
#else
#define IOC_IR_MAGIC					('w')

#define IOCTL_WM8740_INIT				_IOW(IOC_IR_MAGIC, 0xa0, int)
#define IOCTL_WM8740_RESET			_IO(IOC_IR_MAGIC, 0xa1)
#define IOCTL_WM8740_SET_CONFIG		_IOW(IOC_IR_MAGIC, 0xa2, int)
#define IOCTL_WM8740_TEST				_IOR(IOC_IR_MAGIC, 0xa3, int)
#define IOCTL_WM8740_SET_VOLUME		_IOWR(IOC_IR_MAGIC, 0xa4, int)
#define IOCTL_WM8740_SET_MUTE			_IOW(IOC_IR_MAGIC, 0xa5, int)

#endif

typedef struct {
        unsigned int volume;
} sWM8740;



#define DBGMSG		KERN_WARNING


#ifdef AK_USE_WM8741
/*
 * Register values.
 */
#define WM8741_DACLLSB_ATTENUATION              0x00
#define WM8741_DACLMSB_ATTENUATION              0x01
#define WM8741_DACRLSB_ATTENUATION              0x02
#define WM8741_DACRMSB_ATTENUATION              0x03
#define WM8741_VOLUME_CONTROL                   0x04
#define WM8741_FORMAT_CONTROL                   0x05
#define WM8741_FILTER_CONTROL                   0x06
#define WM8741_MODE_CONTROL_1                   0x07
#define WM8741_MODE_CONTROL_2                   0x08
#define WM8741_RESET                            0x09
#define WM8741_ADDITIONAL_CONTROL_1             0x20

#define WM8741_REGISTER_COUNT                   10
#define WM8741_MAX_REGISTER                     0x20

static const u16 wm8740_reg[WM8740_REG_TOTAL_NUM] = {
//new           default
0xff,    //	0xff,     /* R0  - DACLLSB Attenuation */
0xff,    //	0xff,     /* R1  - DACLMSB Attenuation */
0xff,    //	0xff,     /* R2  - DACRLSB Attenuation */
0xff,    //	0xff,     /* R3  - DACRMSB Attenuation */
0x0000,  //	0x0000,     /* R4  - Volume Control */
0x001A, //	0x0014, 	/* R5  - 16bit Format Control */
0x0000,  //	0x0000,     /* R6  - Filter Control */
0x54,  //	0x0000,     /* R7  - Mode Control 1 */    
0x0002,  //	0x0002,     /* R8  - Mode Control 2 */
0x0000,	 //	0x0000,	    /* R9  - Reset */
//	0x0002,     /* R32 - ADDITONAL_CONTROL_1 */
};

#else


static const unsigned short wm8740_reg[WM8740_REG_TOTAL_NUM] = {
#if 0
	0x000a,
	0x000a,
	
	0x0008,	//24bit I2S
//	0x0009,	//24bit I2S, Soft Mute On

//	0x0001,	//24bit I2S
	0x0005,	//24bit I2S, Attenuation Control
	0x0000
#elif 0
	0x00FF,
	0x00FF,
	
	0x0000,	//16bit I2S, Soft Mute OFF
	0x0001,	//16bit I2S
	0x0000
#elif 0//last 16bit play
	0x01FF,
	0x01FF,
	
	0x0000,	//16bit I2S, Soft Mute OFF
	0x0001,	//16bit I2S
	0x0000
#elif 0
	0x01FF,
	0x01FF,
	
	0x0000,	//16bit I2S, Soft Mute OFF
	0x0005,	//16bit I2S
	0x0000
#elif 0
	0x01FF,
	0x01FF,
	
	0x0008,	//24bit I2S
//	0x0001,	//24bit I2S
	0x0005,	//24bit I2S, Attenuation Control
	0x0000
#elif 1
	0x01FF,
	0x01FF,
	
	0x0008,	//24bit I2S
	0x0001,	//24bit I2S
	0x0000
#elif 0// src play(last play success)
	0x01FF,
	0x01FF,
	
	0x000a,	//24bit I2S, De-emphasis control Enable
	0x0009,	//24bit I2S, zero detection circuit control
	0x0000
#else
#endif
};

#endif

static int latch = 0;


static void wm8741_udelay(int delay)
{
	#if 1
	udelay(delay * 2);
	#else
    volatile int loop;

    for(loop = 0;loop < delay * 1000 ;loop++)
        ;
	#endif
	
}


	
/*
 * read wm8740 register cache
 */
 static inline unsigned short wm8740_read_reg_cache(unsigned short reg)
{
	unsigned short *cache = &wm8740_reg;

	if (reg >= WM8740_REG_TOTAL_NUM)
		return -1;

	//printk("wm8740 cache value : 0x%x\n", cache[reg]);
	
	return cache[reg];
}


/*
 * write wm8740 register cache
 */
static inline void wm8740_write_reg_cache(u16 reg, unsigned short value)
{
	unsigned short *cache = &wm8740_reg;
	if (reg >= WM8740_REG_TOTAL_NUM)
		return;
	cache[reg] = value;
}



/******* DAC Control SPI PORT ************
ML/I2S (PIN 28) : GPIOB[8] SFRM(1)
MC/DM1 (PIN 27) : GPIO[[9] SCLK(1)
MD/DM0 (PIN 26) : GPIOB[11] SDO(1)
CSB/IWO (pin 23) : HW Control(HIGH/LOW)
****************************************/


/*******AK201 DAC Control SPI PORT ************
ML/I2S  	: GPIOG[12] SFRM(1)
MC/DM1  : GPIOG[[11] SCLK(1)
MD/DM0  : GPIOG[14] SDO(1)
CSB/IWO : GPIOA[10]  HW Control(HIGH/LOW)
****************************************/



static void spi_send_byte(unsigned char data)
{

	int i;


	for (i = 0; i < 8; i++)
	{
		if(latch == 15)
		{
			gpio_set_value(GPIO_ML_CSB,0); //ML/I2S Low	
		} 
		gpio_set_value(GPIO_MC_SCL,0);  //CLK Low

		wm8741_udelay(1);

		if(data & 0x80) {
			gpio_set_value(GPIO_MD_SDA,1);
		}
		else  {
			gpio_set_value(GPIO_MD_SDA,0);
		}
		wm8741_udelay(1);

		gpio_set_value(GPIO_MC_SCL,1);  	//CLK Hig
		
		wm8741_udelay(1);

		latch ++;

		data <<= 1;
	}

	gpio_set_value(GPIO_MC_SCL,0);  //CLK Low	

	if(latch == 16)
	{
		gpio_set_value(GPIO_ML_CSB,1);  //ML/I2S High
	}

}


static unsigned int spi_read_byte(void)
{

	int i;
	unsigned int val = 0;
#if 0
	for (i = 0; i < 8; i++) {
		val <<= 1;
		set_gpio_bit(ice, PONTIS_CS_CLK, 0);
		wm8741_udelay(1);
		if (snd_ice1712_gpio_read(ice) & PONTIS_CS_RDATA)
			val |= 1;
		wm8741_udelay(1);
		set_gpio_bit(ice, PONTIS_CS_CLK, 1);
		wm8741_udelay(1);
	}
#else
	for (i = 0; i < 8; i++) {
		val <<= 1;

		//ORG BITCLR(gpio_regs->GPBDAT, Hw9);			//CLK Low		
		gpio_set_value(GPIO_MC_SCL,0);

		wm8741_udelay(1);

		//ORG if (gpio_regs->GPBDAT & Hw11)
		//	val |= 1;
		if(gpio_get_value(GPIO_MD_SDA)) 
			val | 1;

		//ORG BITSET(gpio_regs->GPBDAT, Hw9);			//CLK Higt
		gpio_set_value(GPIO_MC_SCL,1);

		wm8741_udelay(1);
	}

#endif
	return val;
}

static char *intToBinary(int width,int i) 
{
  static char s1[64 + 1] = { '0', };
  int count = width;

  memset(s1,0,sizeof(s1));

  do { s1[--count] = '0' + (char) (i & 1);
	   i = i >> 1;
  } while (count);

  return s1;
}	
static int wm8740_spi_write(unsigned short reg, unsigned short value)
{
	unsigned char data[2];
	unsigned short old_reg_value;
	int read_data = 0;
	char test_buffer[128];
	
	//ORG gpio_regs->GPBEN |= Hw11;
	gpio_direction_output(GPIO_MD_SDA,1);

	#if 0	// wm8740
	data[0] = (reg << 1) | ((value >> 8) & 0x0001);
	data[1] = value & 0x00ff;
	#else // wm8741
	data[0] = (reg << 1) & ~0x1;
	data[1] = value & 0x00ff;
	#endif

#if defined (CONFIG_IRIVER_AK100) || defined (CONFIG_IRIVER_AK120)
	latch = 0;

	//printk("reg:%2d value:%4x %s\n",reg,value,intToBinary(8,&data[0]));


	spi_send_byte(data[0]);					// REG
	spi_send_byte(data[1]);					// DATA


	wm8741_udelay(1);
#endif

	wm8740_write_reg_cache(reg,value);
	return 1;
}


//
// API for itools.
// 
int WM8741_WRITE(unsigned short reg, unsigned short value)
{
	return wm8740_spi_write(reg,value);
}


int WM8741_READ(unsigned short reg)
{
	return wm8740_read_reg_cache(reg);
}




void init_wm8740(void)
{
int i;
	DBG_PRINT("WM8740","IOCTL_WM8740_INIT\n");
#ifdef AK_USE_WM8741
		// ak_set_snd_pwr_ctl(AK_AUDIO_POWER,AK_PWR_ON);
		// ak_set_audio_path(ADP_PLAYBACK);

		 gpio_request(GPIO_DAC_RST, "GPIO_DAC_RST");

		 tcc_gpio_config(GPIO_DAC_RST,GPIO_FN(0));
		 gpio_direction_output(GPIO_DAC_RST,1);

		msleep(10);
		
    	wm8740_spi_write(WM8741_RESET, 0);

		msleep(10);
		
		wm8740_set_volume(0,0);


		#if 0
		for(i=0;i <3 ;i ++) {
			if(i!=WM8741_RESET) {
				wm8740_spi_write((unsigned short)i, wm8740_reg[i]);
			}
			msleep(10);
		}
		#endif

#else
	wm8740_spi_write(REG_WM8740_M0, wm8740_reg[0]);
	wm8740_spi_write(REG_WM8740_M1, wm8740_reg[1]);

	#ifdef CONFIG_IRIVER_AK100
	wm8740_spi_write(REG_WM8740_M2, wm8740_reg[2]);
	#endif
	wm8740_spi_write(REG_WM8740_M3, wm8740_reg[3]);

	#ifdef CONFIG_IRIVER_AK100
	wm8740_spi_write(REG_WM8740_M4, wm8740_reg[4]);
	#endif

	msleep(100);
	wm8740_spi_write(REG_WM8740_M2, wm8740_reg[2] | 0x01);
	msleep(100);
	wm8740_spi_write(REG_WM8740_M2, wm8740_reg[2] & ~0x01);
#endif

}
#define MAX_VOLUME_IDX (200)

#define MAX_VOLUME_BALANCE_RATE (30)

/* 2013-8-21 balance added .*/

#if 0
void wm8740_set_volume(int vol,int balance)
{
	int lvol=vol,rvol=vol;
	int codec_lvol,codec_rvol;
	unsigned short lvol_lsb,lvol_msb,rvol_lsb,rvol_msb;

	if(balance > 0) {
		lvol -=  balance * 2;
	} else if(balance <0) {
		rvol +=  balance * 2; 
	}

	if(lvol <0) lvol = 0;
	if(lvol > MAX_VOLUME_IDX) lvol= MAX_VOLUME_IDX;

	if(rvol <0) rvol = 0;
	if(rvol > MAX_VOLUME_IDX) rvol= MAX_VOLUME_IDX;
	
	codec_lvol = (MAX_VOLUME_IDX-vol) * 1023 / MAX_VOLUME_IDX;
	codec_rvol = (MAX_VOLUME_IDX-vol) * 1023 / MAX_VOLUME_IDX;

	DBG_PRINT("WM8740","(lvol:%d rvol:%d balance:%d)\n", lvol,rvol,balance); 		
	
    lvol_lsb = codec_lvol & 0b11111;
	lvol_msb = (codec_lvol & 0b1111100000) >> 5;

    rvol_lsb = codec_rvol & 0b11111;
	rvol_msb = (codec_rvol & 0b1111100000) >> 5;
		
	wm8740_spi_write(WM8741_DACLLSB_ATTENUATION, lvol_lsb | 0b100000);
	wm8740_spi_write(WM8741_DACLMSB_ATTENUATION, lvol_msb | 0b100000);
	wm8740_spi_write(WM8741_DACRLSB_ATTENUATION, rvol_lsb | 0b100000);
	wm8740_spi_write(WM8741_DACRMSB_ATTENUATION, rvol_msb | 0b100000);
}
#else
void wm8740_set_volume(int lvol,int rvol)
{
	int codec_lvol,codec_rvol;
	unsigned short lvol_lsb,lvol_msb,rvol_lsb,rvol_msb;

	if(lvol <0) lvol = 0;
	if(lvol > MAX_VOLUME_IDX) lvol= MAX_VOLUME_IDX;

	if(rvol <0) rvol = 0;
	if(rvol > MAX_VOLUME_IDX) rvol= MAX_VOLUME_IDX;
	
	codec_lvol = (MAX_VOLUME_IDX-lvol) * 1023 / MAX_VOLUME_IDX;
	codec_rvol = (MAX_VOLUME_IDX-rvol) * 1023 / MAX_VOLUME_IDX;

	DBG_PRINT("WM8740","(lvol:%d rvol:%d)\n", lvol,rvol); 		
	
    lvol_lsb = codec_lvol & 0b11111;
	lvol_msb = (codec_lvol & 0b1111100000) >> 5;

    rvol_lsb = codec_rvol & 0b11111;
	rvol_msb = (codec_rvol & 0b1111100000) >> 5;
		
	wm8740_spi_write(WM8741_DACLLSB_ATTENUATION, lvol_lsb | 0b100000);
	wm8740_spi_write(WM8741_DACLMSB_ATTENUATION, lvol_msb | 0b100000);
	wm8740_spi_write(WM8741_DACRLSB_ATTENUATION, rvol_lsb | 0b100000);
	wm8740_spi_write(WM8741_DACRMSB_ATTENUATION, rvol_msb | 0b100000);
}


#endif
static int g_dac_volume = 120;

void wm8740_set_volume_ctl(int up_down)
{
	int dac_volume = g_dac_volume;
	
	dac_volume += up_down;

	if(dac_volume > MAX_VOLUME_IDX) return;
	if(dac_volume < 0)	return;

	g_dac_volume=dac_volume;
		
	wm8740_set_volume(g_dac_volume,g_dac_volume);
	DBG_PRINT("WM8740","DAC VOL:%d\n",g_dac_volume);
}

void wm8740_set_mute(int mute)
{
	DBG_PRINT("WM8740","(mute:%d)\n", mute); 		

	if(mute)
		wm8740_spi_write(REG_WM8740_M2, wm8740_reg[2] | 0x01);
	else
		wm8740_spi_write(REG_WM8740_M2, wm8740_reg[2] & ~0x01);
}

void wm8740_set_mode(void)
{
	wm8740_spi_write(WM8741_MODE_CONTROL_1,0x54);
}
static __init int wm8740_spi_init(void)
{
	int ret;
	int i;


	printk("---------------------------------------\n");
	printk("Bring up WM8741 DAC.\n"); 
	printk("---------------------------------------\n");

	gpio_request(GPIO_ML_CSB, "GPIO_ML_CSB");
	gpio_request(GPIO_MC_SCL, "GPIO_MC_SCL");
	gpio_request(GPIO_MD_SDA, "GPIO_MD_SDA");

	tcc_gpio_config(GPIO_ML_CSB,GPIO_FN(0));
	tcc_gpio_config(GPIO_MC_SCL,GPIO_FN(0));
	tcc_gpio_config(GPIO_MD_SDA,GPIO_FN(0));

	gpio_direction_output(GPIO_ML_CSB,1);
	gpio_direction_output(GPIO_MC_SCL,0);
	gpio_direction_output(GPIO_MD_SDA,0);

	init_wm8740();
}


static __init int wm8740_spi_exit(void)
{
	int ret;

	printk("---------------------------------------\n");
	printk("Func:%s\n", __FUNCTION__);
	printk("---------------------------------------\n");

	gpio_free(GPIO_ML_CSB);
	gpio_free(GPIO_MC_SCL);
	gpio_free(GPIO_MD_SDA);
}

module_init(wm8740_spi_init)
module_exit(wm8740_spi_exit)


MODULE_AUTHOR("Telechips Inc. SYS4-3 linux@telechips.com");
MODULE_DESCRIPTION("WM8740 SPI driver");
MODULE_LICENSE("GPL");
#endif


