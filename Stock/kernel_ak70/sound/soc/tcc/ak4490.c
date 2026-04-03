/*
 * ak4490_ctl.c  --  
 *
 * Copyright 2010 Iriver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
 


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <asm/unaligned.h>


#include <mach/gpio.h>
#include <mach/board_astell_kern.h>



#if(CUR_DAC==CODEC_AK4490)

#if(CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320)
#define CODEC_AK4490_DUAL  
#endif

#ifdef CODEC_AK4490_DUAL
static struct i2c_client *g_i2c_client_ak4490_1;   //  CH R 
#endif

static struct i2c_client *g_i2c_client_ak4490_2;   //  CH L

#define AK4490_1_NAME "AK4490_1"
#define AK4490_2_NAME "AK4490_2"

#ifndef AK_AUDIO_MODULE_USED //tonny : To avoid build error
extern int ak_dock_check_device(void); //iriver-ysyim
extern int ak_dock_power_state(void);
#endif

static int g_ak4490_lvol = -1;
static int g_ak4490_rvol = -1;


// L: 0 0 1 0 0 CAD1(0) CAD0(1) R/W(0)
// R: 0 0 1 0 0 CAD1(0) CAD0(0) R/W(0)



struct ak4490_reg 
{
unsigned char reg;
unsigned char value;
};


#if(CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320)
static struct ak4490_reg g_ak4490_1_reg[] =
{
#if(CUR_AK == MODEL_AK380)
{0,  0b00001111},
#else
{0,  0b10001111},
#endif
{1,  0b00100010},
{2,  0b00001000},
{3,  0b00000000},
{4,  0b00000000},
{5,  0b00000000},
{6,  0b00000000},
{7,  0b00000001},
{8,  0b00000000},
{9,  0b00000000}
};


static struct ak4490_reg g_ak4490_2_reg[] =
{
#if(CUR_AK == MODEL_AK380)
{0,  0b00001111},
#else
{0,  0b10001111},
#endif
{1,  0b00100010},
{2,  0b00001010},
{3,  0b00000000},
{4,  0b00000000},
{5,  0b00000000},
{6,  0b00000000},
{7,  0b00000001},
{8,  0b00000000},
{9,  0b00000000}
};

#else

static struct ak4490_reg g_ak4490_2_reg[] =
{
{0,  0b10001111},
{1,  0b00100010},
{2,  0b00000000},
{3,  0b00000000},
{4,  0b00000000},
{5,  0b00000000},
{6,  0b00000000},
{7,  0b00000001},
{8,  0b00000000},
{9,  0b00000000}
};
#endif


static int write_reg (struct i2c_client *client, u8 reg, u8 data)
{
	int ret = 0;
	uint8_t buf[2];
	buf[0] = reg;
	buf[1] = data;

	//printk ("reg:%d, data:0x%x\n", reg, data);

	ret = i2c_master_send(client, buf, 2);
	if (ret < 0)  {
		printk ("Write Error addr:%x\n", client->addr);
		return -1;
	}

	return 0;
}


static int read_reg (struct i2c_client *client, u8 reg)
{
	int ret = 0;
	uint8_t buf[2];
	
	//printk ("read_reg reg:%d\n", reg);
	
	buf[0] = reg;
	ret = i2c_master_send(client, buf, 1);
	if (ret < 0)  {
		printk ("Read Error addr:%x\n", client->addr);
		return -1;
	}

	client->flags = I2C_M_RD;
	ret = i2c_master_recv(client, buf, 1);
	if (ret < 0) {
		printk ("Read Error addr:%x\n", client->addr);
		return -1;
	} 

	return buf[0];
}


int AK4490_WRITE(int ch,unsigned int reg, unsigned int val)
{
	#ifdef CODEC_AK4490_DUAL  
	if(ch==0) {
		if(!g_i2c_client_ak4490_1) {
			DBG_PRINT("AK4490","ERROR \n");
			return -1;
		}
		return write_reg(g_i2c_client_ak4490_1,reg,val);
	} else 
	#endif
	
	{
		if(!g_i2c_client_ak4490_2) {
			DBG_PRINT("AK4490","ERROR \n");
			return -1;
		}
		return write_reg(g_i2c_client_ak4490_2,reg,val);

	}
}


int AK4490_READ(int ch,unsigned char reg)
{
	#ifdef CODEC_AK4490_DUAL  
	if(ch==0) {
		if(!g_i2c_client_ak4490_1) {
			DBG_PRINT("CS4308","ERROR \n");

			return -1;
		}
		return read_reg(g_i2c_client_ak4490_1, reg);
	} else 
	#endif
	
	{

		if(!g_i2c_client_ak4490_2) {
			DBG_PRINT("CS4308","ERROR \n");
		
			return -1;
		}
		return read_reg(g_i2c_client_ak4490_2, reg);
	}
}


void ak4490_set_volume(int mode, int lvol,int rvol)
{
	unsigned short codec_lvol,codec_rvol;
	unsigned short lvol_lsb,lvol_msb,rvol_lsb,rvol_msb;
	int vol_offset = 105;

	if(mode ==1) {
		g_ak4490_lvol = lvol;
		g_ak4490_rvol = rvol;
	}
	
	if(lvol <0) lvol = 0;
	if(lvol > MAX_VOLUME_IDX) lvol= MAX_VOLUME_IDX;

	if(rvol <0) rvol = 0;
	if(rvol > MAX_VOLUME_IDX) rvol= MAX_VOLUME_IDX;

	if(lvol==0) {
		codec_lvol = 0x0;
		//codec_lvol = 100;
	} else {
		codec_lvol = (lvol + vol_offset);
	}

	if(rvol==0) {
		codec_rvol = 0x0;
		//codec_rvol = 100;
	} else {
		codec_rvol = (rvol + vol_offset);
	}


	#if 1//def PRINT_VOLUME_VALUE
	CPRINT("(lvol:%d(%x) rvol:%d(%x))\n", lvol,codec_lvol,rvol,codec_rvol); 		
	#endif

#ifdef  CODEC_AK4490_DUAL	
	AK4490_WRITE(0,0x3,codec_lvol);
	AK4490_WRITE(0,0x4,codec_lvol);

	AK4490_WRITE(1,0x3,codec_rvol);
	AK4490_WRITE(1,0x4,codec_rvol);
#else
	AK4490_WRITE(1,0x3,codec_lvol);
	AK4490_WRITE(1,0x4,codec_rvol);
#endif

}

void ak4490_reg_freeze(int freeze)
{
/*
	if(freeze)  {
		AK4490_WRITE(1,0x8,0b01000000);
		AK4490_WRITE(0,0x8,0b01000000);
	} else {
		AK4490_WRITE(1,0x8,0b01100000);
		AK4490_WRITE(0,0x8,0b01100000);
	}
*/
}

void ak4490_volume_mute(void)
{
//	AK4490_WRITE(1,0x5,0);
//	AK4490_WRITE(1,0x6,0);

//	AK4490_WRITE(0,0x5,0);
//	AK4490_WRITE(0,0x6,0);
}

void ak4490_resume_volume(void)
{		
	if(g_ak4490_lvol != -1)  {
		ak4490_set_volume(1, g_ak4490_lvol, g_ak4490_rvol);
	}
}
EXPORT_SYMBOL(ak4490_resume_volume);

void ak4490_get_volume(int *lvol,int *rvol)
{
	*lvol = g_ak4490_lvol;
	*rvol = g_ak4490_rvol;
}



void init_ak4490_reg(int ch)
{
	int i;
	
	#ifdef CODEC_AK4490_DUAL  
	if(ch==0) {

		for(i=0;i < ARRAY_SIZE(g_ak4490_1_reg);i++) {
			write_reg(g_i2c_client_ak4490_1,g_ak4490_1_reg[i].reg,g_ak4490_1_reg[i].value);
		}
		
	} else 
	#endif
	{
		for(i=0;i < ARRAY_SIZE(g_ak4490_2_reg);i++) {
			write_reg(g_i2c_client_ak4490_2,g_ak4490_2_reg[i].reg,g_ak4490_2_reg[i].value);
		}
	}
}

// SPEED mode.
static int g_fm_freq = -1;

void ak4490_FM(int freq)
{

// Serial Mode : Support PCM or DSD
// Parallel Mode : PCM Mode only

//	000 - Single-Speed Mode (30 to 54 kHz sample rates)
//	001 - Double-Speed Mode (54 to 108 kHz sample rates)
//	010 - Quad-Speed Mode (120 to 216 kHz sample rates)
//
//	100 - Ocg-Speed Mode (384 kHz sample rates)
//	101 - Quad-Speed Mode (768 kHz sample rates)

	ak8157a_set_samplerate(freq);

#if (CUR_AK != MODEL_AK380 && CUR_AK != MODEL_AK300)
	return;
#endif

	DBG_ERR("d","FM:%d\n",freq);

	g_fm_freq= freq;
	
	if(freq <= 48000)  {
#if 1 //sharp roll off
		AK4490_WRITE(0,0x1, 0b00100010);
		AK4490_WRITE(0,0x5, 0b00000000);
		AK4490_WRITE(1,0x1, 0b00100010);
		AK4490_WRITE(1,0x5, 0b00000000);
#else //slow roll off
		AK4490_WRITE(0,0x1, 0b00000010);
		AK4490_WRITE(0,0x5, 0b00000001);
		AK4490_WRITE(1,0x1, 0b00000010);
		AK4490_WRITE(1,0x5, 0b00000001);
#endif
	}else if(freq < 96000)  {
		AK4490_WRITE(0,0x1, 0b00101010);
		AK4490_WRITE(0,0x5, 0b00000000);
		AK4490_WRITE(1,0x1, 0b00101010);
		AK4490_WRITE(1,0x5, 0b00000000);

	}else if(freq == 96000)  {
		//short delay sharp roll off --> slow roll off
		AK4490_WRITE(0,0x1, 0b00001010);
		AK4490_WRITE(0,0x5, 0b00000001);
		AK4490_WRITE(1,0x1, 0b00001010);
		AK4490_WRITE(1,0x5, 0b00000001);
	}else if(freq <= 192000)  {
		AK4490_WRITE(0,0x1, 0b00110010);
		AK4490_WRITE(0,0x5, 0b00000000);
		AK4490_WRITE(1,0x1, 0b00110010);
		AK4490_WRITE(1,0x5, 0b00000000);
	}else if(freq <= 384000)  {
		AK4490_WRITE(0,0x1, 0b00100010);
		AK4490_WRITE(0,0x5, 0b00000010);
		AK4490_WRITE(1,0x1, 0b00100010);
		AK4490_WRITE(1,0x5, 0b00000010);
	}else if(freq <= 768000)  {
		AK4490_WRITE(0,0x1, 0b00101010);
		AK4490_WRITE(0,0x5, 0b00000010);
		AK4490_WRITE(1,0x1, 0b00101010);
		AK4490_WRITE(1,0x5, 0b00000010);
	}
	
}

void ak4490_FM_reinit(void)
{

	if(g_fm_freq) {
		ak4490_FM(g_fm_freq);
	}
}

void ak4490_mute(int onoff)
{
/* tonny
	//printk("---------------------------------->%s, %d\n", __FUNCTION__, onoff);
	if(onoff== 1) {
		AK4490_WRITE(0,0x1, 0b00011000);
		AK4490_WRITE(1,0x1, 0b00011000);
	} else {
		AK4490_WRITE(0,0x4, 0b00000000);
		AK4490_WRITE(1,0x4, 0b00000000);
	}
*/
}

void ak4490_pwr(int onoff)
{
	int lvalue,rvalue;	
/*	
	if(onoff== 1) {
		DBG_ERR("d","pwr on\n");
		ak4490_mute(1);
		//msleep_interruptible(10);		
		AK4490_WRITE(0,0x8, 0b01000000);
		AK4490_WRITE(1,0x8, 0b01000000);
		//msleep_interruptible(10);
		ak4490_mute(0);	
	} else {
		DBG_ERR("d","pwr off\n");
		ak4490_mute(1);	
		AK4490_WRITE(0,0x8, 0b11000000);
		AK4490_WRITE(1,0x8, 0b11000000);
	}
*/
}


void ak4490_save_cur_volume(void)
{
	// 2017.4.14 tonny : To void build error when model is AK70 with AK Module
	// AK70 does not use this function.
	// So there is not any code.
}
EXPORT_SYMBOL(ak4490_save_cur_volume);


void ak4490_reg_init(void)
{
	init_ak4490_reg(0);
	init_ak4490_reg(1);
#ifndef AK_AUDIO_MODULE_USED //tonny : To avoid build error because AK70 don't have DOCK.
	if((ak_dock_power_state()==DOCK_PWR_ON) && (ak_dock_check_device()==DEVICE_AMP)) //iriver-ysyim : AMPAI¡Æ©¡¢¯i MAX Volume¨ù©øA¢´ (A©øA¨ö ¨¬I¨¡AC¨ª¨ù¡© AMP Play¨öA ©ö¢çA| ©ö©¬¡íy)
		ak4490_set_volume(0, MAX_VOLUME_IDX,MAX_VOLUME_IDX);  
	else
#endif
		ak4490_resume_volume();	

//J151109
	printk("==%s:%d:lvol(%d);rvol(%d)===\n",__func__, __LINE__, g_ak4490_lvol,g_ak4490_rvol );
}

static int __devinit ak4490_probe(struct i2c_client *		client,
				   const struct i2c_device_id * id)
{
	int ret;

printk("ak4490_probe\n");

	int i;
	#ifdef CODEC_AK4490_DUAL  
	if(!strcmp(id->name,AK4490_1_NAME)) {
		g_i2c_client_ak4490_1 = client;			
		i2c_set_clientdata(client, NULL);

//		init_reg(0);
		#if 0
		for(i=0;i<=9;i++) {
			ret = read_reg(client,i);
			printk("ak4490_1 [%xH] : %x \n",i,ret);
			msleep(100);
		}
		#endif
	
	} else 
	#endif
	{
		g_i2c_client_ak4490_2 = client;
		i2c_set_clientdata(client, NULL);

//		init_reg(1);
		#if 0
		for(i=0;i<=9;i++) {
			ret = read_reg(client,i);
			printk("ak4490_2 [%xH] : %x \n",i,ret);
			msleep(100);
		}
		#endif
	
	}



	return 0;
}

#ifdef CONFIG_PM

static int ak4490_suspend(struct i2c_client *client,
		pm_message_t state)
{

	return 0;
}

static int ak4490_resume(struct i2c_client *client)
{


	return 0;
}

#else
#define wm8804_suspend NULL
#define wm8804_resume NULL
#endif


static int __devexit ak4490_remove(struct i2c_client *client)
{


	return 0;
}

#ifdef CODEC_AK4490_DUAL  
static struct i2c_device_id ak4490_1_idtable[] = {
	{ AK4490_1_NAME, 0 },
	{},
};
#endif

static struct i2c_device_id ak4490_2_idtable[] = {
	{ AK4490_2_NAME, 0 },
	{},
};


MODULE_DEVICE_TABLE(i2c, ak4490_1_idtable);

static const struct dev_pm_ops ak4490_pm_ops = {
	#ifdef CONFIG_PM
	.suspend	= ak4490_suspend,
	.resume		= ak4490_resume,
	#else
	.suspend	= NULL,
	.resume		= NULL,
	#endif
};

#ifdef CODEC_AK4490_DUAL  
static struct i2c_driver ak4490_1_i2c_driver = {
	.id_table	= ak4490_1_idtable,
	.probe		= ak4490_probe,
	.remove		= __devexit_p(ak4490_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= AK4490_1_NAME,
		.pm	= NULL,
	},
};
#endif

static struct i2c_driver ak4490_2_i2c_driver = {
	.id_table	= ak4490_2_idtable,
	.probe		= ak4490_probe,
	.remove		= __devexit_p(ak4490_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= AK4490_2_NAME,
		.pm	= &ak4490_pm_ops,
	},
};


static int __init ak4490_init(void)
{
	int ret;

	printk("ak4490 dual dac\n");

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)                            
	gpio_request(GPIO_I2C_SEL, "GPIO_I2C_SEL");	
	tcc_gpio_config(GPIO_I2C_SEL,GPIO_FN(0));
	gpio_direction_output(GPIO_I2C_SEL,1);
	gpio_set_value(GPIO_I2C_SEL, 0);
#endif
    
	gpio_request(GPIO_DAC_RST, "GPIO_DAC_RST");
	tcc_gpio_config(GPIO_DAC_RST,GPIO_FN(0));
	gpio_direction_output(GPIO_DAC_RST,1);

#ifdef CODEC_AK4490_DUAL  
	i2c_add_driver(&ak4490_1_i2c_driver);
#endif
	
    i2c_add_driver(&ak4490_2_i2c_driver);
    
    ak4490_reg_init();
    
    return 1;
}


static void __exit ak4490_exit(void)
{
	#ifdef CODEC_AK4490_DUAL  
	i2c_del_driver(&ak4490_1_i2c_driver);
	#endif
	i2c_del_driver(&ak4490_2_i2c_driver);
}


module_init(ak4490_init);
module_exit(ak4490_exit);


MODULE_DESCRIPTION("AK4490 DAC");
MODULE_LICENSE("GPL");

#endif
