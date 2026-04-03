/*
 * SA9227.c  --  
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

#if (CUR_AK == MODEL_AK380) || (CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)

static struct i2c_client *g_i2c_client_SA9227; //connect to preamp
#define SA9227_NAME "SA9227"

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

struct SA9227_reg 
{
	unsigned char reg;
	unsigned char value;
};


static struct SA9227_reg g_SA92276_reg[] =
{
{0x10,  0x00},
{0x11,  0x00},
{0x12,  0x00},
{0x13,  0x00},
{0x1b,  0x00},

{0x1c,  0x00},
{0x1d,  0x00},
{0x1e,  0x00},
{0x50,  0x00},
{0x51,  0x00},

{0x52,  0x00},
{0x53,  0x00},
{0x54,  0x00},
{0x55,  0x00},
{0x56,  0x00},

{0x57,  0x00},
{0x58,  0x00},
{0x59,  0x00},
};


void display_sa9227_reg(void)
{
	u8 add,reg;

	for(add=0x10; add <=0x1e ;add++) {
		reg = read_reg(g_i2c_client_SA9227, add);
		printk("--%s:addr[0x%x]==>reg[0x%x]\n--", __func__, add, reg);
	}

	for(add=0x50; add <=0x59 ;add++) {
		reg = read_reg(g_i2c_client_SA9227, add);
		printk("--%s:addr[0x%x]==>reg[0x%x]\n--", __func__, add, reg);
	}

}


void init_reg_SA9227(void)
{
#if 1 //test.set bps
	u8 reg;
	reg = read_reg(g_i2c_client_SA9227, 0x12);
	printk("--%s:addr[0x12]==>reg[0x%x]\n--", __func__,  reg);

	write_reg(g_i2c_client_SA9227, 0x12, 0x70);

	reg = read_reg(g_i2c_client_SA9227, 0x12);
	printk("--%s:addr[0x12]==>reg[0x%x]\n--", __func__,  reg);
#endif
}


int init_SA9227(void)
{
	int sa9227_id=0;
	int i;
	
	printk("init_sa9227\n");

	display_sa9227_reg();

	init_reg_SA9227(); //test
	return 0;
}
EXPORT_SYMBOL(init_SA9227);


static int __devinit SA9227_probe(struct i2c_client *		client,
				   const struct i2c_device_id * id)
{
		printk("SA9227__probe1\n");

	if(!strcmp(id->name,SA9227_NAME)) {
		g_i2c_client_SA9227 = client;			
		i2c_set_clientdata(client, NULL);
		printk("SA9227__probe\n");
	
	}

	return 0;
}

#ifdef CONFIG_PM

static int SA9227_suspend(struct i2c_client *client,
		pm_message_t state)
{
	return 0;
}

static int SA9227_resume(struct i2c_client *client)
{
	return 0;
}

#else
#define SA9227_suspend NULL
#define SA9227_resume NULL
#endif


static int __devexit SA9227_remove(struct i2c_client *client)
{
	return 0;
}


static struct i2c_device_id SA9227_idtable[] = {
	{ SA9227_NAME, 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, SA9227_idtable);

static const struct dev_pm_ops SA9227_pm_ops = {
	.suspend	= SA9227_suspend,
	.resume		= SA9227_resume,
};


static struct i2c_driver SA9227_i2c_driver = {
	.id_table	= SA9227_idtable,
	.probe		= SA9227_probe,
	.suspend	= SA9227_suspend,
	.resume		= SA9227_resume,
	.remove		= __devexit_p(SA9227_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= SA9227_NAME,
		.pm	= NULL,//SA9227_pm_ops,
	},
};



static int __init SA9227_init(void)
{

	printk("SA9227 usb sound ic \n");

	return i2c_add_driver(&SA9227_i2c_driver);


}


static void __exit SA9227_exit(void)
{
	i2c_del_driver(&SA9227_i2c_driver);
}

module_init(SA9227_init);
module_exit(SA9227_exit);

MODULE_DESCRIPTION("SA9227 usb sound ic");
MODULE_LICENSE("GPL");


#endif
