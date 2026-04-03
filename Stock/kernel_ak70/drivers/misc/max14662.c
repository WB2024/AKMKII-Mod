/*
 * MAX14662.c  --  
 *
 * Copyright 2015 Iriver
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

#if (CUR_AK == MODEL_AK380) || (CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)

static struct i2c_client *g_i2c_client_MAX14662_0; //connect to preamp
static struct i2c_client *g_i2c_client_MAX14662_1; //connect to volume ctrl(cs3318)
static struct i2c_client *g_i2c_client_MAX14662_2; //connect go adc(ak5572)

#define MAX14662_0_NAME "MAX14662_0"
#define MAX14662_1_NAME "MAX14662_1"
#define MAX14662_2_NAME "MAX14662_2"

static int write_reg (struct i2c_client *client, u8 data)
{
	int ret = 0;
	uint8_t buf[2];
	buf[0] = 0; //J151021 always 0x00 for max14462
	buf[1] = data;

	//printk ("reg:%d, data:0x%x\n", reg, data);

	ret = i2c_master_send(client, buf, 2);
	if (ret < 0)  {
		printk ("Write Error addr:%x\n", client->addr);
		return -1;
	}

	return 0;
}

static int read_reg (struct i2c_client *client)
{
	int ret = 0;
	uint8_t buf;

	client->flags = I2C_M_RD;
	ret = i2c_master_recv(client, buf, 1);
	if (ret < 0) {
		printk ("Read Error addr:%x\n", client->addr);
		return -1;
	} 

	return buf;
}


#if 0
int MAX14662_WRITE(unsigned int val)
{
		if(!g_i2c_client_MAX14662) {
			DBG_PRINT("MAX14662","ERROR \n");
			return -1;
		}
		return write_reg(g_i2c_client_MAX14662,val);
}


int MAX14662_READ()
{
		if(!g_i2c_client_MAX14662) {
			DBG_PRINT("MAX14662","ERROR \n");

			return -1;
		}
		return read_reg(g_i2c_client_MAX14662);
}
#endif

struct MAX14662_reg 
{
	unsigned char reg;
	unsigned char value;
};


static struct MAX14662_reg g_MAX14662_0_reg[] =
{
{0x00,  0x00},
};
static struct MAX14662_reg g_MAX14662_1_reg[] =
{
{0x00,  0x00},
};
static struct MAX14662_reg g_MAX14662_2_reg[] =
{
{0x00,  0x00},
};

/*
void display_all_reg(void)
{
	int add,reg;

	for(add=1; add <=0x1 ;add++) {
		reg = read_reg(g_i2c_client_MAX14662_0);
	}
	for(add=1; add <=0x1 ;add++) {
		reg = read_reg(g_i2c_client_MAX14662_1);
	}
	for(add=1; add <=0x1 ;add++) {
		reg = read_reg(g_i2c_client_MAX14662_2);
	}

}
*/

void init_reg_MAX14662(void)
{
	int i;

	for(i=0;i < ARRAY_SIZE(g_MAX14662_0_reg);i++) {
		write_reg(g_i2c_client_MAX14662_0,g_MAX14662_0_reg[i].value);
	}
	for(i=0;i < ARRAY_SIZE(g_MAX14662_1_reg);i++) {
		write_reg(g_i2c_client_MAX14662_1,g_MAX14662_1_reg[i].value);
	}
	for(i=0;i < ARRAY_SIZE(g_MAX14662_2_reg);i++) {
		write_reg(g_i2c_client_MAX14662_2,g_MAX14662_2_reg[i].value);
	}

}

typedef enum {
	AK_REC_SWITCH_MIC_IN=0,
	AK_REC_SWITCH_LINE_IN,
	AK_REC_SWITCH_VOL_CTRL,
}AK_REC_SWITCH_EXT; 
	
int set_recorder_switch_power(AK_REC_SWITCH_EXT switch_idx, int on)
{
	switch(switch_idx) {
	case AK_REC_SWITCH_MIC_IN:
		g_MAX14662_0_reg[0].value = on;
		write_reg(g_i2c_client_MAX14662_0,g_MAX14662_0_reg[0].value);
		break;

	case AK_REC_SWITCH_LINE_IN:
		g_MAX14662_1_reg[0].value = on;
		write_reg(g_i2c_client_MAX14662_1,g_MAX14662_1_reg[0].value);
		break;

	case AK_REC_SWITCH_VOL_CTRL:
		g_MAX14662_2_reg[0].value = on;
		write_reg(g_i2c_client_MAX14662_2,g_MAX14662_2_reg[0].value);
		break;

	default:
		printk("%s %d error! there is no switch idx(%d)\n");
		break; 
	}
	return 0;
}
EXPORT_SYMBOL(set_recorder_switch_power);

int init_MAX14662(void)
{
	int max14662_id=0;
	int i;
	
	printk("init_max14662\n");

	init_reg_MAX14662();
#if 1 //test
	//initialize chip with value 0xff (switch on)
	g_MAX14662_0_reg[0].value = 0xff;
	g_MAX14662_1_reg[0].value = 0xff;
	g_MAX14662_2_reg[0].value = 0xff;

	write_reg(g_i2c_client_MAX14662_0,g_MAX14662_0_reg[0].value);
	write_reg(g_i2c_client_MAX14662_1,g_MAX14662_1_reg[0].value);
	write_reg(g_i2c_client_MAX14662_2,g_MAX14662_2_reg[0].value);
#endif

#if 0 //test 
	for(i=0; i<10; i++) {
		write_reg(g_i2c_client_MAX14662_0,g_MAX14662_0_reg[0].value);
		write_reg(g_i2c_client_MAX14662_1,g_MAX14662_1_reg[0].value);
		write_reg(g_i2c_client_MAX14662_2,g_MAX14662_2_reg[0].value);
		
		g_MAX14662_0_reg[0].value = (g_MAX14662_0_reg[0].value)? 0x00:0xff;
		g_MAX14662_1_reg[0].value = (g_MAX14662_1_reg[0].value)? 0x00:0xff;
		g_MAX14662_2_reg[0].value = (g_MAX14662_2_reg[0].value)? 0x00:0xff;
		printk("init_MAX14662_0 ----------- reg value : %d\n", g_MAX14662_0_reg[0].value);
		printk("init_MAX14662_1 ----------- reg value : %d\n", g_MAX14662_1_reg[0].value);
		printk("init_MAX14662_2 ----------- reg value : %d\n", g_MAX14662_2_reg[0].value);
		msleep_interruptible(10000);
	}
#endif

	return 0;
}
EXPORT_SYMBOL(init_MAX14662);


static int __devinit MAX14662_probe(struct i2c_client *		client,
				   const struct i2c_device_id * id)
{

	if(!strcmp(id->name,MAX14662_0_NAME)) {
		g_i2c_client_MAX14662_0 = client;			
		i2c_set_clientdata(client, NULL);
		printk("MAX14662_0_probe\n");
	
	}
	else if(!strcmp(id->name,MAX14662_1_NAME)) {
		g_i2c_client_MAX14662_1 = client;			
		i2c_set_clientdata(client, NULL);
		printk("MAX14662_1_probe\n");
	
	} 
	else if(!strcmp(id->name,MAX14662_2_NAME)) {
		g_i2c_client_MAX14662_2 = client;			
		i2c_set_clientdata(client, NULL);
		printk("MAX14662_2_probe\n");
	
	}
	
	return 0;
}

#ifdef CONFIG_PM

static int MAX14662_suspend(struct i2c_client *client,
		pm_message_t state)
{
	return 0;
}

static int MAX14662_resume(struct i2c_client *client)
{
	return 0;
}

#else
#define MAX14662_suspend NULL
#define MAX14662_resume NULL
#endif


static int __devexit MAX14662_remove(struct i2c_client *client)
{
	return 0;
}


static struct i2c_device_id MAX14662_0_idtable[] = {
	{ MAX14662_0_NAME, 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, MAX14662_0_idtable);

static const struct dev_pm_ops MAX14662_pm_ops = {
	.suspend	= MAX14662_suspend,
	.resume		= MAX14662_resume,
};


static struct i2c_driver MAX14662_0_i2c_driver = {
	.id_table	= MAX14662_0_idtable,
	.probe		= MAX14662_probe,
	.suspend	= MAX14662_suspend,
	.resume		= MAX14662_resume,
	.remove		= __devexit_p(MAX14662_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= MAX14662_0_NAME,
		.pm	= NULL,//MAX14662_pm_ops,
	},
};


static struct i2c_device_id MAX14662_1_idtable[] = {
	{ MAX14662_1_NAME, 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, MAX14662_1_idtable);

static struct i2c_driver MAX14662_1_i2c_driver = {
	.id_table	= MAX14662_1_idtable,
	.probe		= MAX14662_probe,
	.suspend	= MAX14662_suspend,
	.resume		= MAX14662_resume,
	.remove		= __devexit_p(MAX14662_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= MAX14662_1_NAME,
		.pm	= NULL, //MAX14662_pm_ops,
	},
};

static struct i2c_device_id MAX14662_2_idtable[] = {
	{ MAX14662_2_NAME, 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, MAX14662_2_idtable);

static struct i2c_driver MAX14662_2_i2c_driver = {
	.id_table	= MAX14662_2_idtable,
	.probe		= MAX14662_probe,
	.suspend	= MAX14662_suspend,
	.resume		= MAX14662_resume,
	.remove		= __devexit_p(MAX14662_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= MAX14662_2_NAME,
		.pm	= NULL, //MAX14662_pm_ops,
	},
};


static int __init MAX14662_init(void)
{

	printk("MAX14662 switch control \n");

	i2c_add_driver(&MAX14662_0_i2c_driver);
	i2c_add_driver(&MAX14662_1_i2c_driver);

	return i2c_add_driver(&MAX14662_2_i2c_driver);


}


static void __exit MAX14662_exit(void)
{
	i2c_del_driver(&MAX14662_2_i2c_driver);
	i2c_del_driver(&MAX14662_1_i2c_driver);
	i2c_del_driver(&MAX14662_0_i2c_driver);
}

module_init(MAX14662_init);
module_exit(MAX14662_exit);

MODULE_DESCRIPTION("MAX14662 switch IC");
MODULE_LICENSE("GPL");


#endif
