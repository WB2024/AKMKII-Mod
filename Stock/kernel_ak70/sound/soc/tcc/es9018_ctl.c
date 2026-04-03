/*
 * ES9018_ctl.c  --  
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


#if (CUR_AK == MODEL_AK1000N)

//-	Right DAC I2C Add : 1001111, ATAPI Register : 00101
//-	Left DAC I2C Add : 1001100, ATAPI Register : 01010

static struct i2c_client *g_i2c_client_ES9018_1;
static struct i2c_client *g_i2c_client_ES9018_2;

#define ES9018_1_NAME "ES9018_1"
#define ES9018_2_NAME "ES9018_2"




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


int ES9018_WRITE(int ch,unsigned int reg, unsigned int val)
{
	if(ch==0) {
		if(!g_i2c_client_ES9018_1) {
			DBG_PRINT("ES9018","ERROR \n");
			return -1;
		}
		return write_reg(g_i2c_client_ES9018_1,reg,val);
	} else {
		if(!g_i2c_client_ES9018_2) {
			DBG_PRINT("ES9018","ERROR \n");
			return -1;
		}
		return write_reg(g_i2c_client_ES9018_2,reg,val);

	}
}


int ES9018_READ(int ch,unsigned char reg)
{
	if(ch==0) {
		if(!g_i2c_client_ES9018_1) {
			DBG_PRINT("ES9018","ERROR \n");

			return -1;
		}
		return read_reg(g_i2c_client_ES9018_1, reg);
	} else {

		if(!g_i2c_client_ES9018_2) {
			DBG_PRINT("ES9018","ERROR \n");
		
			return -1;
		}
		return read_reg(g_i2c_client_ES9018_2, reg);
	}
}

#define MAX_VOLUME_IDX (200)
static int g_ES9018_lvol = -1;
static int g_ES9018_rvol = -1;

void ES9018_set_volume(int lvol,int rvol)
{
	unsigned char codec_lvol,codec_rvol;
	unsigned short lvol_lsb,lvol_msb,rvol_lsb,rvol_msb;
//	int start_offset = 20;

	g_ES9018_lvol = lvol;
	g_ES9018_rvol = rvol;

	if(lvol <0) lvol = 0;
	if(lvol > MAX_VOLUME_IDX) lvol= MAX_VOLUME_IDX;

	if(rvol <0) rvol = 0;
	if(rvol > MAX_VOLUME_IDX) rvol= MAX_VOLUME_IDX;

	#if 0
	codec_lvol = (MAX_VOLUME_IDX-lvol) * 256 / MAX_VOLUME_IDX;
	codec_rvol = (MAX_VOLUME_IDX-rvol) * 256 / MAX_VOLUME_IDX;
	#else
	if(lvol==0) {
		codec_lvol = 0xff;
		
	} else {
		codec_lvol = (MAX_VOLUME_IDX-lvol);

	}

	if(rvol==0) {
		codec_rvol = 0xff;
	} else {
		codec_rvol = (MAX_VOLUME_IDX-rvol);
	}

	#endif

	#ifdef PRINT_VOLUME_VALUE
	CPRINT("(lvol:%d rvol:%d)\n", lvol,rvol); 		
	#endif
	
//	ES9018_WRITE(0,0x5,codec_lvol);
//	ES9018_WRITE(0,0x6,codec_lvol);

//	ES9018_WRITE(1,0x5,codec_rvol);
//	ES9018_WRITE(1,0x6,codec_rvol);

}


void ES9018_volume_mute(void)
{
//	ES9018_WRITE(0,0x5,255);
//	ES9018_WRITE(0,0x6,255);
	
//	ES9018_WRITE(1,0x5,255);
//	ES9018_WRITE(1,0x6,255);
}

void ES9018_resume_volume(void)
{
	if(g_ES9018_lvol != -1)  {
		ES9018_set_volume(g_ES9018_lvol,g_ES9018_rvol);
	}
}

struct ES9018_reg 
{
unsigned char reg;
unsigned char value;
};


static struct ES9018_reg g_ES9018_1_reg[] =
{
{0,  0b00000000},
{1,  0b00000000},
{2,  0b00000000},
{3,  0b00000000},
{4,  0b00000000},
{5,  0b00000000},
{6,  0b00000000},   
{7,  0b00000000},
{8,  0x68},
{9,  0x4},
{10,  0b11111100},
{11,  0x8D},
{12,  0x20},
{13,  0b00000000},
{14,  0x09},   
{15,  0b00000000},
{16,  0b00000000},
{17,  0x1C},			//  20140121   {17,  0b10010101},
{18,  0b00000001},
{19,  0b00000000},

{20,  0b11111111},
{21,  0b11111111},
{22,  0b11111111},
{23,  0b01111111},

{24,  0x30},
{25,  0x2},
{26,  0},
//{27,  0x2},
//{28,  0},
//{29,  0},
//{30,  0},
//{31,  0},
{32,  0},
{33,  0},
{34,  0},
{35,  0},
{36,  0},
{37,  0},
{38,  0},
{39,  0},
{40,  0},
{41,  0}
/*
{42,  0},
{43,  0},
{44,  0},
{45,  0},
{46,  0x1},
{47,  0},

{48,  0},
{49,  0},
{50,  0},
{51,  0},
{52,  0},
{53,  0},
{54,  0},
{55,  0},
{56,  0},
{57,  0},
{58,  0},
{59,  0},
{60,  0},
{61,  0},
{62,  0},
{63,  0},
{64,  0},
{65,  0},
{66,  0},
{67,  0},
{68,  0},
{69,  0},
{70,  0},
{71,  0}
*/
};



static struct ES9018_reg g_ES9018_2_reg[] =
{
{0,  0b00000000},
{1,  0b00000000},
{2,  0b00000000},
{3,  0b00000000},
{4,  0b00000000},
{5,  0b00000000},
{6,  0b00000000},   
{7,  0b00000000},
{8,  0x68},
{9,  0x4},
{10,  0b11001100},
{11,  0x8D},
{12,  0x20},
{13,  0b00000000},
{14,  0x09},   
{15,  0b00000000},
{16,  0b00000000},
{17,  0x1C},			//  20140121   {17,  0b10010101},
{18,  0b00000001},
{19,  0b00000000},

{20,  0b11111111},
{21,  0b11111111},
{22,  0b11111111},
{23,  0b01111111},

{24,  0x30},
{25,  0x2},
{26,  0},
//{27,  0x2},
//{28,  0},
//{29,  0},
//{30,  0},
//{31,  0},
{32,  0},
{33,  0},
{34,  0},
{35,  0},
{36,  0},
{37,  0},
{38,  0},
{39,  0},
{40,  0},
{41,  0}
/*
{42,  0},
{43,  0},
{44,  0},
{45,  0},
{46,  0x1},
{47,  0},

{48,  0},
{49,  0},
{50,  0},
{51,  0},
{52,  0},
{53,  0},
{54,  0},
{55,  0},
{56,  0},
{57,  0},
{58,  0},
{59,  0},
{60,  0},
{61,  0},
{62,  0},
{63,  0},
{64,  0},
{65,  0},
{66,  0},
{67,  0},
{68,  0},
{69,  0},
{70,  0},
{71,  0}
*/

};


void init_reg_ES9018(int ch)
{
int i;

	if(ch==0) {

		for(i=0;i < ARRAY_SIZE(g_ES9018_1_reg);i++) {
			write_reg(g_i2c_client_ES9018_1,g_ES9018_1_reg[i].reg,g_ES9018_1_reg[i].value);
		}


		printk("ES9018_1 State : %x \n",read_reg(g_i2c_client_ES9018_1,27));

	} else {
	
		for(i=0;i < ARRAY_SIZE(g_ES9018_2_reg);i++) {
			write_reg(g_i2c_client_ES9018_2,g_ES9018_2_reg[i].reg,g_ES9018_2_reg[i].value);
		}

		printk("ES9018_2 State : %x \n",read_reg(g_i2c_client_ES9018_2,27));
		
	}

}

// SPEED mode.
static int g_fm_freq = -1;

void ES9018_FM(int freq)
{

//	00 - Single-Speed Mode (30 to 50 kHz sample rates)
//	01 - Double-Speed Mode (50 to 100 kHz sample rates)
//	10 - Quad-Speed Mode (100 to 200 kHz sample rates)
//	11 - Direct Stream Digital Mode
//	if(g_fm_freq==freq) {
//		return;
//	}
	
	g_fm_freq= freq;
	
	if(freq<= 48000)  {
		ES9018_WRITE(0,0x2, 0b00010000);
		ES9018_WRITE(1,0x2, 0b00010000);
	} else if(freq > 96000)  {
		ES9018_WRITE(0,0x2, 0b00010010);
		ES9018_WRITE(1,0x2, 0b00010010);
	} else {
		ES9018_WRITE(0,0x2, 0b00010001);
		ES9018_WRITE(1,0x2, 0b00010001);
	}
}

void ES9018_FM_reinit(void)
{

//	if(g_fm_freq) {
//		ES9018_FM(g_fm_freq);
//	}
}

void ES9018_mute(int onoff)
{

	//printk("---------------------------------->%s, %d\n", __FUNCTION__, onoff);
	if(onoff== 1) {

		ES9018_WRITE(0,10, 0b11001111);
		ES9018_WRITE(1,10, 0b11001111);

	} else {

		ES9018_WRITE(0,10, 0b11001110);
		ES9018_WRITE(1,10, 0b11001110);

	}

}

void ES9018_pwr(int onoff)
{
	int lvalue,rvalue;	
	
	if(onoff== 1) {
		DBG_ERR("d","pwr on\n");
		
		ES9018_mute(1);
		//msleep_interruptible(10);		
	//	ES9018_WRITE(0,0x8, 0b01000000);
	//	ES9018_WRITE(1,0x8, 0b01000000);
		//msleep_interruptible(10);
		ES9018_mute(0);	
	} else {
		DBG_ERR("d","pwr off\n");
		ES9018_mute(1);	
	//	ES9018_WRITE(0,0x8, 0b11000000);
	//	ES9018_WRITE(1,0x8, 0b11000000);
	}

}

void ES9018_reg_init(void)
{
	init_reg_ES9018(0);
	init_reg_ES9018(1);

	//ES9018_resume_volume();	
}
static struct delayed_work g_power_on_work;
static struct delayed_work g_power_off_work;


static void ES9018_power_on_work(struct work_struct *work)
{
	
	ES9018_pwr(1);

	ES9018_resume_volume();

	ak_set_hw_mute(AMT_HP_MUTE,0);	
}
static void ES9018_power_off_work(struct work_struct *work)
{
//	cs4398_pwr(0);
}

void ES9018_schedule_power_ctl(int onoff)
{
	if(onoff==1) {		
		schedule_delayed_work(&g_power_on_work, msecs_to_jiffies(200));
	} else {
		schedule_delayed_work(&g_power_off_work, 0);
	}
}
static int __devinit ES9018_probe(struct i2c_client *		client,
				   const struct i2c_device_id * id)
{

	if(!strcmp(id->name,ES9018_1_NAME)) {
		g_i2c_client_ES9018_1 = client;			
		i2c_set_clientdata(client, NULL);

		init_reg_ES9018(0);
		//printk("ES9018_1_probed CHIP ID : %x \n",read_reg(client,27));

		INIT_DELAYED_WORK(&g_power_on_work, ES9018_power_on_work);
		INIT_DELAYED_WORK(&g_power_off_work, ES9018_power_off_work);
		
	} else {
		g_i2c_client_ES9018_2 = client;
		i2c_set_clientdata(client, NULL);

		//printk("ES9018_2_probed CHIP ID : %x \n",read_reg(client,27));	
		init_reg_ES9018(1);
	}

	return 0;
}

#ifdef CONFIG_PM

static int ES9018_suspend(struct i2c_client *client,
		pm_message_t state)
{
	ES9018_pwr(0);
	return 0;
}

static int ES9018_resume(struct i2c_client *client)
{

	ES9018_reg_init();
	return 0;
}

#else
#define wm8804_suspend NULL
#define wm8804_resume NULL
#endif


static int __devexit ES9018_remove(struct i2c_client *client)
{


	return 0;
}


static struct i2c_device_id ES9018_1_idtable[] = {
	{ ES9018_1_NAME, 0 },
	{},
};

static struct i2c_device_id ES9018_2_idtable[] = {
	{ ES9018_2_NAME, 0 },
	{},
};


MODULE_DEVICE_TABLE(i2c, ES9018_1_idtable);

static const struct dev_pm_ops ES9018_pm_ops = {
	#ifdef CONFIG_PM
	.suspend	= ES9018_suspend,
	.resume		= ES9018_resume,
	#else
	.suspend	= NULL,
	.resume		= NULL,
	#endif
};


static struct i2c_driver ES9018_1_i2c_driver = {
	.id_table	= ES9018_1_idtable,
	.probe		= ES9018_probe,
	.remove		= __devexit_p(ES9018_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= ES9018_1_NAME,
		.pm	= NULL,
	},
};

static struct i2c_driver ES9018_2_i2c_driver = {
	.id_table	= ES9018_2_idtable,
	.probe		= ES9018_probe,
	.remove		= __devexit_p(ES9018_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= ES9018_2_NAME,
		.pm	= &ES9018_pm_ops,
	},
};


static int __init ES9018_init(void)
{
	int ret;
	#if(CUR_DAC == CODEC_ES9018)

	
	gpio_request(GPIO_DAC_EN, "GPIO_DAC_EN");		
	gpio_request(GPIO_I2C_SEL, "GPIO_I2C_SEL");
	gpio_request(GPIO_DAC_RST, "GPIO_DAC_RST");	
	gpio_request(GPIO_5V5_EN, "GPIO_5V5_EN");	
	
	tcc_gpio_config(GPIO_I2C_SEL,GPIO_FN(0));
	gpio_direction_output(GPIO_I2C_SEL,0);
	

	printk("ES9018 dual dac\n");


	tcc_gpio_config(GPIO_5V5_EN, GPIO_FN(0));
       gpio_direction_output(GPIO_5V5_EN, 1);


       tcc_gpio_config(GPIO_DAC_EN, GPIO_FN(0));
       gpio_direction_output(GPIO_DAC_EN, 1);	


	tcc_gpio_config(GPIO_DAC_RST,GPIO_FN(0));
	gpio_direction_output(GPIO_DAC_RST,1);

	msleep_interruptible(10);

	gpio_direction_output(GPIO_DAC_RST,0);

	msleep_interruptible(10);

	

	i2c_add_driver(&ES9018_1_i2c_driver);
	return i2c_add_driver(&ES9018_2_i2c_driver);

	#else
	return 0;
	#endif
}


static void __exit ES9018_exit(void)
{
	i2c_del_driver(&ES9018_1_i2c_driver);
	i2c_del_driver(&ES9018_2_i2c_driver);
}

module_init(ES9018_init);
module_exit(ES9018_exit);

MODULE_DESCRIPTION("ES9018 DAC");
MODULE_LICENSE("GPL");


#endif
