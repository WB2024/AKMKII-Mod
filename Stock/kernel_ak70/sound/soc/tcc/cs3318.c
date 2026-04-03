/*
 * CS3318.c  --  
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

#include <mach/bsp.h>

#include <mach/ak_gpio_def.h>

#include <mach/board-tcc8930.h>

extern int detect_amp_unbal(void);
extern int detect_amp_bal(void);

#if (CUR_AK == MODEL_AK1000N) ||(CUR_AK == MODEL_AK500N) ||(CUR_AK == MODEL_AK380) || (CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)

static struct i2c_client *g_i2c_client_CS3318;

#define CS3318_NAME "CS3318"


#define MAX_UI_VOLUME_IDX (150)		// range 0~150
//extern AUDIO_OUT_TYPE get_cur_audio_out_type(void);
#define LOW_GAIN 1
#define HIGH_GAIN 2


static int g_cs3318_lvol = -1;
static int g_cs3318_rvol = -1;
static int g_gain = -1;



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


int CS3318_WRITE(int ch,unsigned int reg, unsigned int val)
{
		if(!g_i2c_client_CS3318) {
			DBG_PRINT("CS3318","ERROR \n");
			return -1;
		}
		return write_reg(g_i2c_client_CS3318,reg,val);
}


int CS3318_READ(int ch,unsigned char reg)
{
		if(!g_i2c_client_CS3318) {
			DBG_PRINT("CS3318","ERROR \n");

			return -1;
		}
		return read_reg(g_i2c_client_CS3318, reg);
}


struct CS3318_reg 
{
	unsigned char reg;
	unsigned char value;
};


static struct CS3318_reg g_CS3318_reg[] =
{
//{0x00,  0x00},
//{0x01,  0x90},
//{0x02,  0x0B},
//{0x03,  0x50},
//{0x04,  0x89},
//{0x05,  0x85},
//{0x06,  0x13},   
{0x0A,  0x00},
//{0x0B,  0x0F},
{0x0B,  0x3F}, // add h/w mute
{0x0D,  0x00},
{0x0E,  0x00},
{0x10,  0x00},
{0x11,  0xFF},
{0x13,  0x00},
{0x14,  0xFF},
{0x16,  0x00},	
{0x17,  0xFF}	
};

void display_all_reg(void)
{
	int add,reg;

	for(add=1; add <=0x1C ;add++) {
		reg = read_reg(g_i2c_client_CS3318,add);
		//printk("CS3318: [0x%x]= 0x%x\n",add,reg);
	}
}

void init_reg_CS3318(void)
{
	int i;

		for(i=0;i < ARRAY_SIZE(g_CS3318_reg);i++) {
			write_reg(g_i2c_client_CS3318, g_CS3318_reg[i].reg, g_CS3318_reg[i].value);
		}

		//printk("CS3318 State : %x \n",read_reg(g_i2c_client_CS3318, 0x1D));
}

// [downer] A150220
extern int g_main_lvol, g_main_rvol, g_amp_gain;

int init_CS3318_rec(void)
{
	int cs3318_id=0;
	
	printk("init_CS3318\n");
	gpio_set_value(EXT_GPIO_P02,1); //VOL_CTL_EN

	msleep_interruptible(10);
	
	gpio_set_value(EXT_GPIO_P03, 0);	//VOL_CTL_RST
	msleep_interruptible(10);
	gpio_set_value(EXT_GPIO_P03, 1);	
	msleep_interruptible(10);

	init_reg_CS3318();

	CS3318_set_volume(g_gain, g_cs3318_lvol, g_cs3318_rvol);

	cs3318_id = read_reg(g_i2c_client_CS3318,0x1C);
	printk("CS3318 CHIP ID : %x \n",cs3318_id);
	
	cs3318_id = (cs3318_id >> 4) & 0xF;

    /* [downer] A150520 AMP 초기화 시 볼륨 재설정 */
    ak4490_set_volume(0, MAX_VOLUME_IDX,MAX_VOLUME_IDX);  
    CS3318_set_volume(0, 200, 200);
    
	if(cs3318_id == 0b0110) //check cs3318 device chip id
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(init_CS3318_rec);


int init_CS3318(void)
{
	int cs3318_id=0;
	
	printk("init_CS3318\n");

	gpio_set_value(EXT_GPIO_P02,1); //VOL_CTL_EN

	msleep_interruptible(10);
	
	gpio_set_value(EXT_GPIO_P07, 0);	//VOL_CTL_RST
	msleep_interruptible(10);
	gpio_set_value(EXT_GPIO_P07, 1);	
	msleep_interruptible(10);

	init_reg_CS3318();

	CS3318_set_volume(g_gain, g_cs3318_lvol, g_cs3318_rvol);

	cs3318_id = read_reg(g_i2c_client_CS3318,0x1C);
	printk("CS3318 CHIP ID : %x \n",cs3318_id);
	
	cs3318_id = (cs3318_id >> 4) & 0xF;

    /* [downer] A150520 AMP 초기화 시 볼륨 재설정 */
    ak4490_set_volume(0, MAX_VOLUME_IDX,MAX_VOLUME_IDX);  
    CS3318_set_volume(g_amp_gain, g_main_lvol, g_main_rvol);
    
	if(cs3318_id == 0b0110) //check cs3318 device chip id
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(init_CS3318);


void CS3318_set_volume(int gain,int lvol,int rvol)
{
	unsigned char codec_lvol,codec_rvol;
	int amp_unbal_gain, amp_bal_gain,high_gain_val, low_gian_val,max_high_gain_val, max_low_gain_val;	
	//printk("CS3318_set_volume\n");
	
	g_cs3318_lvol = lvol;
	g_cs3318_rvol = rvol;
	g_gain = gain;
	
	if(lvol <0) lvol = 0;
	if(lvol > MAX_UI_VOLUME_IDX) lvol= MAX_UI_VOLUME_IDX;

	if(detect_amp_bal()){
		//amp bal-> max 149 ui->150 ui 
		high_gain_val = 85;  // 235-85 = 150 
		max_high_gain_val =  235;
		
		low_gian_val = 	60;  // 210-60 = 150
		max_low_gain_val = 210;
	}else{ //amp unbal
		//high_gain_val = 86;  // CS3318 vol: 254(150), 254(150) gain:2 ==> cs3318=254-18 = 236: 236-150=86
		//amp -> max 137 ui->150 ui 
		high_gain_val = 73;  // 223-73 = 150
		max_high_gain_val =  223;
		
		low_gian_val = 	60;  // 210-60 = 150
		max_low_gain_val = 210;
	}
		
	if(lvol==0) {
		codec_lvol = 18;  // 0001 0010 : -96db
		
	} else {

		if (gain == HIGH_GAIN)
			codec_lvol = (lvol + high_gain_val);    
		else //if (gain == LOW_GAIN) 
			codec_lvol = (lvol + low_gian_val );    
	}

	if(lvol==MAX_UI_VOLUME_IDX) {

		if (gain == HIGH_GAIN)
			codec_lvol = max_high_gain_val;   
		else //if (gain == LOW_GAIN) 
			codec_lvol = max_low_gain_val;    
	}	


	if(rvol <0) rvol = 0;
	if(rvol > MAX_UI_VOLUME_IDX) rvol= MAX_UI_VOLUME_IDX;


	if(rvol==0) {
		codec_rvol = 18;
		
	} else {

		if (gain == HIGH_GAIN)
			codec_rvol =  (rvol + high_gain_val);    // ui= 0~150  cs3318=254-18 = 236: 236-150=86
		else //if (gain == LOW_GAIN) 
			codec_rvol = (rvol + low_gian_val );    // ui=0~150 cs3318=210-18 = 192: 192-150=42
	}

	if(rvol==MAX_UI_VOLUME_IDX) {

		if (gain == HIGH_GAIN)
			codec_rvol = max_high_gain_val;   
		else //if (gain == LOW_GAIN) 
			codec_rvol = max_low_gain_val;    
	}	
	
	printk("CS3318 vol: %d%(%d), %d%(%d) gain:%d\n",lvol,codec_lvol,rvol,codec_rvol,gain);
	
	CS3318_WRITE(0,0x01,codec_lvol); //balanced L1
	CS3318_WRITE(0,0x02,codec_lvol); //balanced L2
	CS3318_WRITE(0,0x03,codec_rvol); //balanced R1
	CS3318_WRITE(0,0x04,codec_rvol); //balanced R2
	
	CS3318_WRITE(0,0x05,codec_lvol); //unbal L1
	CS3318_WRITE(0,0x06,codec_lvol); //unbal L2
	CS3318_WRITE(0,0x07,codec_rvol); //unbal R1
	CS3318_WRITE(0,0x08,codec_rvol); //unbal R2

	if(lvol==0 && rvol!=0 ) {  //left mute
		printk("CS3318 left mute\n");
		CS3318_WRITE(0,0x0A,0x33);	
	}	
	else if(lvol!=0 && rvol==0 ) {//right mute
		printk("CS3318 right mute\n");
		CS3318_WRITE(0,0x0A,0xCC);	
	}
	else if(lvol==0 && rvol==0) 
	{
		printk("CS3318 left/right mute\n");
		CS3318_WRITE(0,0x0A,0xFF);	// all channel Mute Set
		//CS3318_WRITE(0,0x0E,0x01);	// power Down all ==> 1: 148mA, 0: 345mA
	}
	else
	{
		CS3318_WRITE(0,0x0E,0x00);
		CS3318_WRITE(0,0x0A,0x00);	// all channel  un Mute 
	}

	//display_all_reg();
	
}

void CS3318_volume_mute(int val)
{
	if(val == 1)
		CS3318_WRITE(0,0x0A,0xFF);	// all channel Mute Set
	else if(val == 0)
		CS3318_WRITE(0,0x0A,0x00);
}
EXPORT_SYMBOL(CS3318_volume_mute);



static struct delayed_work g_power_on_work;
static struct delayed_work g_power_off_work;


static void CS3318_power_on_work(struct work_struct *work)
{
	

}
static void CS3318_power_off_work(struct work_struct *work)
{

}

void CS3318_schedule_power_ctl(int onoff)
{
//	if(onoff==1) {		
//		schedule_delayed_work(&g_power_on_work, msecs_to_jiffies(200));
//	} else {
//		schedule_delayed_work(&g_power_off_work, 0);
//	}
}
static int __devinit CS3318_probe(struct i2c_client *		client,
				   const struct i2c_device_id * id)
{

	if(!strcmp(id->name,CS3318_NAME)) {
		g_i2c_client_CS3318 = client;			
		i2c_set_clientdata(client, NULL);
		printk("CS3318_probe\n");
		//gpio_request(VAR_OPAMP_RST, "VAR_OPAMP_RST");
		//tcc_gpio_config(VAR_OPAMP_RST, GPIO_FN(0));
		//gpio_direction_output(VAR_OPAMP_RST, 0);	
		//msleep_interruptible(10);
		//gpio_direction_output(VAR_OPAMP_RST, 1);	
		//msleep_interruptible(10);
		
		//init_reg_CS3318();
		//printk("ES9018_1_probed CHIP ID : %x \n",read_reg(client,27));

		//INIT_DELAYED_WORK(&g_power_on_work, CS3318_power_on_work);
		//INIT_DELAYED_WORK(&g_power_off_work, CS3318_power_off_work);
		
	}
	
	return 0;
}

#ifdef CONFIG_PM

static int CS3318_suspend(struct i2c_client *client,
		pm_message_t state)
{
	//CS3318_pwr(0);
	return 0;
}

static int CS3318_resume(struct i2c_client *client)
{
	return 0;
}

#else
#define CS3318_suspend NULL
#define CS3318_resume NULL
#endif


static int __devexit CS3318_remove(struct i2c_client *client)
{


	return 0;
}


static struct i2c_device_id CS3318_idtable[] = {
	{ CS3318_NAME, 0 },
	{},
};



MODULE_DEVICE_TABLE(i2c, CS3318_idtable);

static const struct dev_pm_ops CS3318_pm_ops = {
	.suspend	= NULL,
	.resume		= NULL,
};


static struct i2c_driver CS3318_i2c_driver = {
	.id_table	= CS3318_idtable,
	.probe		= CS3318_probe,
	.suspend	= CS3318_suspend,
	.resume		= CS3318_resume,
	.remove		= __devexit_p(CS3318_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= CS3318_NAME,
		.pm	= NULL,
	},
};



static int __init CS3318_init(void)
{

	printk("CS3318 volume control \n");

	return i2c_add_driver(&CS3318_i2c_driver);


}


static void __exit CS3318_exit(void)
{
	i2c_del_driver(&CS3318_i2c_driver);
}

module_init(CS3318_init);
module_exit(CS3318_exit);

MODULE_DESCRIPTION("CS3318 volume IC");
MODULE_LICENSE("GPL");


#endif
