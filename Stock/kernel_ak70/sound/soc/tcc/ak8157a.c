/*
 * ak8157a.c  --  
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



#if(AUDIO_CLOCK == CLK_AK8157A)
static struct i2c_client *g_i2c_client_ak8157a;

#define AK8157A_NAME "AK8157A"


struct ak8157a_reg 
{
unsigned char reg;
unsigned char value;
};


static struct ak8157a_reg g_ak8157a_reg[] =
{
{0,  0b00001111},
{1,  0b00100010},
};


#if 0
static int write_reg (struct i2c_client *client, u8 reg, u8 data)
{
	int ret = 0;
	uint8_t buf[2];
	buf[0] = reg;
	buf[1] = data;

	printk ("add:0x%x reg_w:0x%x, data:0x%x\n", client->addr, reg, data);

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
	
	printk ("add:0x%x reg_r:0x%x\n", client->addr, reg);
	
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
#else
static inline int read_reg(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_byte_data(client, reg);
}

static inline int write_reg(struct i2c_client *client, u8 reg, u8 value)
{
	return i2c_smbus_write_byte_data(client, reg, value);
}
#endif


int AK8157A_WRITE(int ch,unsigned int reg, unsigned int val)
{

	if(!g_i2c_client_ak8157a) {
		DBG_PRINT("AK8157A","ERROR \n");
		return -1;
	}
	return write_reg(g_i2c_client_ak8157a,reg,val);
}


int AK8157A_READ(int ch,unsigned char reg)
{
	if(!g_i2c_client_ak8157a) {
		DBG_PRINT("AK8157A","ERROR \n");
	
		return -1;
	}
	return read_reg(g_i2c_client_ak8157a, reg);
}



void ak8157a_set_samplerate(int rate)
{
	int val;
	int i;
		
	//gpio_direction_output(GPIO_PLL_RST, 1);
	//mdelay(1);
	//gpio_direction_output(GPIO_PLL_RST, 0);
	//mdelay(1);
	//gpio_direction_output(GPIO_PLL_RST, 1);
	
	AK8157A_WRITE(1,0x0,0);

	for(i=0; i<4; i++){
		g_i2c_client_ak8157a->addr=(0x10+i);
		val = AK8157A_READ(0,0);			
		if(val == 0x00)
			break;
	}

	printk("%s: %d, i2c_add:0x%x\n",__FUNCTION__, rate,g_i2c_client_ak8157a->addr);	

	if(rate <= 44100)
		AK8157A_WRITE(1,0x1,0x01);
	else if(rate == 48000)
		AK8157A_WRITE(1,0x1,0x42);
	else if(rate == 88200)
		AK8157A_WRITE(1,0x1,0x11);
	else if(rate == 96000)
		AK8157A_WRITE(1,0x1,0x12);
	else if(rate == 176400)
		AK8157A_WRITE(1,0x1,0x21);
	else if(rate == 192000)
		AK8157A_WRITE(1,0x1,0x22);
	else if(rate == 352000)
		AK8157A_WRITE(1,0x1,0x31);
	else if(rate == 384000)
		AK8157A_WRITE(1,0x1,0x32);

	//printk("ak8157a[0x00]: %x \n",AK8157A_READ(1, 0x00));
	printk("ak8157a[00H]:0x%x [01H]:0x%x\n",AK8157A_READ(1, 0x00),AK8157A_READ(1, 0x01));
}

void ak8157a_reg_init(void)
{
	int i;
	
	for(i=0;i < ARRAY_SIZE(g_ak8157a_reg);i++) 
	{
		write_reg(g_i2c_client_ak8157a,g_ak8157a_reg[i].reg,g_ak8157a_reg[i].value);
	}
}

static int ak8157a_detect(struct i2c_client *client,
			  struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int val1, val2;
	unsigned short address = client->addr;
	

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		return -ENODEV;
	}

	val1 = i2c_smbus_read_byte_data(client, 0x00);
	val2 = i2c_smbus_read_byte_data(client, 0x01);

	if((val1 != 0xe0)){				
		return -ENODEV;
	}

	printk("%s(i2c_add:0x%x [00H]0x%x, [01H]0x%x)\n", __FUNCTION__, client->addr, val1, val2);

	strlcpy(info->type, AK8157A_NAME, I2C_NAME_SIZE);


	return 0;
}

static int __devinit ak8157a_probe(struct i2c_client *		client,
				   const struct i2c_device_id * id)
{
	int ret;
	int i;

	printk("ak8157a_probe\n");


	g_i2c_client_ak8157a = client;
	i2c_set_clientdata(client, NULL);

//		init_reg(1);
	#if 0
	for(i=0;i<=9;i++) {
		ret = read_reg(client,i);
		printk("ak8157a_2 [%xH] : %x \n",i,ret);
		msleep(100);
	}
	#endif




	return 0;
}


static int ak8157a_suspend(struct i2c_client *client,
		pm_message_t state)
{
	return 0;
}


static int ak8157a_resume(struct i2c_client *client)
{
	return 0;
}



static int __devexit ak8157a_remove(struct i2c_client *client)
{
	return 0;
}



static struct i2c_device_id ak8157a_idtable[] = {
	{ AK8157A_NAME, 0 },
	{},
};


MODULE_DEVICE_TABLE(i2c, ak8157a_idtable);

static const struct dev_pm_ops ak8157a_pm_ops = {
	.suspend	= ak8157a_suspend,
	.resume		= ak8157a_resume,
};

/* Addresses to scan */
static const unsigned short normal_i2c[] = { 0x10, 0x11, 0x12, 0x13,
						I2C_CLIENT_END };

static struct i2c_driver ak8157a_i2c_driver = {
	.class		= I2C_CLASS_HWMON, //i2c_detect
	.id_table	= ak8157a_idtable,
	.probe		= ak8157a_probe,
	.remove		= __devexit_p(ak8157a_remove),
	.address_list	= normal_i2c,
	.detect		= ak8157a_detect, //i2c_detect
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= AK8157A_NAME,
		.pm	= &ak8157a_pm_ops,
	},
};




static int __init ak8157a_init(void)
{
	int ret;

	printk("ak8157a init\n");


	gpio_request(GPIO_PLL_RST, "GPIO_PLL_RST");
	tcc_gpio_config(GPIO_PLL_RST, GPIO_FN(0));
	gpio_direction_output(GPIO_PLL_RST, 1);

  i2c_add_driver(&ak8157a_i2c_driver);
    
  //ak8157a_reg_init();
    
  return 1;
}


static void __exit ak8157a_exit(void)
{
	i2c_del_driver(&ak8157a_i2c_driver);
}


module_init(ak8157a_init);
module_exit(ak8157a_exit);


MODULE_DESCRIPTION("AK8157A PLL");
MODULE_LICENSE("GPL");


#endif
