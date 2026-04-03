/*
 * CS8406.c  --  
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

#if (CUR_AK == MODEL_AK1000N) ||(CUR_AK == MODEL_AK500N)

static struct i2c_client *g_i2c_client_CS8406;

#define CS8406_NAME "CS8406"




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


int CS8406_WRITE(int ch,unsigned int reg, unsigned int val)
{
		if(!g_i2c_client_CS8406) {
			DBG_PRINT("CS8406","ERROR \n");
			return -1;
		}
		return write_reg(g_i2c_client_CS8406,reg,val);
}


int CS8406_READ(int ch,unsigned char reg)
{
		if(!g_i2c_client_CS8406) {
			DBG_PRINT("CS8406","ERROR \n");

			return -1;
		}
		return read_reg(g_i2c_client_CS8406, reg);
}


struct CS8406_reg 
{
	unsigned char reg;
	unsigned char value;
};


static struct CS8406_reg g_CS8406_reg[] =
{
//{0x00,  0x00},
{0x01,  0x03},
{0x02,  0x00},
{0x03,  0x00},

// [Fs 48k 일 때] 
//OMCK 12.288MHz 이면 0x40    
//OMCK 24.576MHz 이면 0x60  

//[Fs 96k 일 때]  
//OMCK 24.576MHz 이면 0x40

//[Fs 192k 일 때]
//OMCK 24.576MHz 이면 0x70
{0x04,  0x40},		

{0x05,  0x05},
{0x06,  0x13},   
//{0x07,  0x00},
//{0x08,  0x00},
{0x09,  0x82},
{0x0A,  0x00},
{0x0B,  0x82},	//INT 단자를 사용할 경우, Falling INT발생함.
{0x0C,  0x04},	//INT 단자를 사용할 경우 내부 상태에 따라 INT 단자 출력 함.
{0x0D,  0x00},
{0x0E,  0x04}   //INT 단자를 사용할 경우, Falling INT발생함.

};


//MCLK
//
//TX SPDIF를 설정할 때 참고해 주시길 바랍니다. 
//
// 192KHz clock   24.576MHZ     256 fs
// 176KHz clock   22.5MHZ       128 fs
//  96KHz  clock   12.2MHz       128 fs
//  88KHz clock    22.5MHZ       256 fs
//  48KHz  clock   12.2MHz       256 fs
//  44.1KHz  clock   11.2MHz     256 fs


void init_reg_CS8406(void)
{
		int i;

		//gpio_request(GPIO_TOSLINK_TX_PWEN, "GPIO_TOSLINK_TX_PWEN");
		gpio_request(GPIO_SPDIF_TX_RST, "GPIO_SPDIF_TX_RST");
		gpio_request(GPIO_SPDIF_TX_ON, "GPIO_TXD_RESET");
		


		//tcc_gpio_config(GPIO_TOSLINK_TX_PWEN,GPIO_FN(0));
		//gpio_direction_output(GPIO_TOSLINK_TX_PWEN,1);	
		tcc_gpio_config(GPIO_SPDIF_TX_ON,GPIO_FN(0));
		gpio_direction_output(GPIO_SPDIF_TX_ON,1);	

		msleep_interruptible(100);
		tcc_gpio_config(GPIO_SPDIF_TX_RST,GPIO_FN(0));
		gpio_direction_output(GPIO_SPDIF_TX_RST,0);
		msleep_interruptible(10);
		gpio_direction_output(GPIO_SPDIF_TX_RST,1);
		msleep_interruptible(10);	

		for(i=0;i < ARRAY_SIZE(g_CS8406_reg);i++) {
			write_reg(g_i2c_client_CS8406,g_CS8406_reg[i].reg,g_CS8406_reg[i].value);
		}

		printk("CS8406 State : %x \n",read_reg(g_i2c_client_CS8406,0x7F));
}


void init_reg_CS8406_192(void)
{
		write_reg(g_i2c_client_CS8406,0x04, 0x70);
		printk("CS8406 State  192 : %x \n",read_reg(g_i2c_client_CS8406,0x7F));
}

void init_reg_CS8406_176(void)
{

		write_reg(g_i2c_client_CS8406,0x04, 0x70);
		printk("CS8406 State 176 : %x \n",read_reg(g_i2c_client_CS8406,0x7F));
}

void init_reg_CS8406_48(void)
{
		write_reg(g_i2c_client_CS8406,0x04, 0x40);
		printk("CS8406 State 48 : %x \n",read_reg(g_i2c_client_CS8406,0x7F));
}

static struct delayed_work g_power_on_work;
static struct delayed_work g_power_off_work;


static void CS8406_power_on_work(struct work_struct *work)
{
	

}
static void CS8406_power_off_work(struct work_struct *work)
{

}

void CS8406_schedule_power_ctl(int onoff)
{
//	if(onoff==1) {		
//		schedule_delayed_work(&g_power_on_work, msecs_to_jiffies(200));
//	} else {
//		schedule_delayed_work(&g_power_off_work, 0);
//	}
}
static int __devinit CS8406_probe(struct i2c_client *		client,
				   const struct i2c_device_id * id)
{

	if(!strcmp(id->name,CS8406_NAME)) {
		g_i2c_client_CS8406 = client;			
		i2c_set_clientdata(client, NULL);

		init_reg_CS8406();
		//printk("ES9018_1_probed CHIP ID : %x \n",read_reg(client,27));

		//INIT_DELAYED_WORK(&g_power_on_work, CS8406_power_on_work);
		//INIT_DELAYED_WORK(&g_power_off_work, CS8406_power_off_work);
		
	}
	
	return 0;
}

#ifdef CONFIG_PM

static int CS8406_suspend(struct i2c_client *client,
		pm_message_t state)
{
	//CS8406_pwr(0);
	return 0;
}

static int CS8406_resume(struct i2c_client *client)
{

	//CS8406_reg_init();
	return 0;
}

#else
#define CS8406_suspend NULL
#define CS8406_resume NULL
#endif


static int __devexit CS8406_remove(struct i2c_client *client)
{


	return 0;
}


static struct i2c_device_id CS8406_idtable[] = {
	{ CS8406_NAME, 0 },
	{},
};



MODULE_DEVICE_TABLE(i2c, CS8406_idtable);

static const struct dev_pm_ops CS8406_pm_ops = {
	#ifdef CONFIG_PM
	.suspend	= CS8406_suspend,
	.resume		= CS8406_resume,
	#else
	.suspend	= NULL,
	.resume		= NULL,
	#endif
};


static struct i2c_driver CS8406_i2c_driver = {
	.id_table	= CS8406_idtable,
	.probe		= CS8406_probe,
	.remove		= __devexit_p(CS8406_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= CS8406_NAME,
		.pm	= NULL,
	},
};



static int __init CS8406_init(void)
{

	//#if(CUR_DAC == CODEC_ES9018)
	printk("CS8406_init\n");
	return i2c_add_driver(&CS8406_i2c_driver);

	//#else
	//return 0;
	//#endif
}


static void __exit CS8406_exit(void)
{
	i2c_del_driver(&CS8406_i2c_driver);
}

module_init(CS8406_init);
module_exit(CS8406_exit);

MODULE_DESCRIPTION("CS8406 SPDIF");
MODULE_LICENSE("GPL");


#endif
