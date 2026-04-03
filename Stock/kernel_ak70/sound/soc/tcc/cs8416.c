/*
 * CS8416.c  --  
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

#define USE_TIMER_PLL_LOCK_CHECK

static struct i2c_client *g_i2c_client_CS8416;

#define CS8416_NAME "CS8416"
//#define RX_TIMER_DELAY	msecs_to_jiffies(2*1)  //2msec
#define RX_TIMER_DELAY	msecs_to_jiffies(100*1)  //2msec
#define RX_TIMER_DELAY_2	msecs_to_jiffies(2*1)  //2msec
#define RX_TIMER_DELAY_200	msecs_to_jiffies(400*1)  //2msec
extern AUDIO_IN_TYPE get_cur_audio_in_type(void);


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


int CS8416_WRITE(int ch,unsigned int reg, unsigned int val)
{
		if(!g_i2c_client_CS8416) {
			DBG_PRINT("CS8416","ERROR \n");
			return -1;
		}
		return write_reg(g_i2c_client_CS8416,reg,val);
}


int CS8416_READ(int ch,unsigned char reg)
{
		if(!g_i2c_client_CS8416) {
			DBG_PRINT("CS8416","ERROR \n");

			return -1;
		}
		return read_reg(g_i2c_client_CS8416, reg);
}
extern void cs4398_FM(int freq);

struct CS8416_reg 
{
	unsigned char reg;
	unsigned char value;
};


static struct CS8416_reg g_CS8416_reg[] =
{
{0x00,  0x00},
{0x01,  0x92},
{0x02,  0x00},	//GPIO0 : Fixed low level
{0x03,  0x50},	//GPIO1 : RERR, GPIO2 : None
//{0x03,  0x00},
//{0x04,  0x89},
{0x04,  0x9B},	
{0x05,  0x85},	//CS8416 Master, I2S
{0x06,  0x13},   	//RERR Conditions : Unlock, Parity
{0x07,  0x04},	//INT : RERR
{0x08,  0x00},
{0x09,  0x04}		//Falling edge ==> Falling INT.이므로 평상시 LOW 이고 에러발생 요인시 HIGH가 되어 
				// LOW로 떨어지는 것이다. stream 변경이 아닌 200usec 이내의 HIGH 구간은 무시 하도록 한다.

};



void init_reg_CS8416(unsigned char rxSel)
{
	int i;

		for(i=0;i < ARRAY_SIZE(g_CS8416_reg);i++) {
			write_reg(g_i2c_client_CS8416,g_CS8416_reg[i].reg,g_CS8416_reg[i].value);
		}

		if (rxSel == 0){	//BNC
			//write_reg(g_i2c_client_CS8416,0x02, 0x00);		////GPIO0 : Fixed low level
			write_reg(g_i2c_client_CS8416,0x04, 0x83);
		}else if (rxSel == 1){	//XLR
			//write_reg(g_i2c_client_CS8416,0x02, 0x0C);		////GPIO0 : Fixed high level
			write_reg(g_i2c_client_CS8416,0x04, 0x8B);
		}else if (rxSel == 3){	//optical
			//write_reg(g_i2c_client_CS8416,0x02, 0x00);		////GPIO0 : Fixed low level		
			write_reg(g_i2c_client_CS8416,0x04, 0x9B);
		}else if (rxSel == 5){	//BNC
			//write_reg(g_i2c_client_CS8416,0x02, 0x00);		////GPIO0 : Fixed low level		
			write_reg(g_i2c_client_CS8416,0x04, 0xAB);			
		}

		//printk("CS8416 State : %x \n",read_reg(g_i2c_client_CS8416,0x7F));
		#if (CUR_DAC==CODEC_CS4398)
		cs4398_FM(96000);
		#endif
		
}

extern int g_optical_rx_status;

struct cs8416_chip {
	struct delayed_work		work;
	struct work_struct wq;

	int irq;
};


#ifdef USE_INTERRUPT_PLL_LOCK
static irqreturn_t cs8416_irq_handler(int irq, void *data)
{
	//if (g_optical_rx_status == 1)
	//{
	//	ak_set_hw_mute(AMT_HP_MUTE,1);		//mute !!
	//	toggle_flag = 1;
	//}

    return IRQ_HANDLED;
}
#endif

#ifdef USE_TIMER_PLL_LOCK_CHECK
int toggle_flag = 0;

static void rx_check_timer_work(struct work_struct *work)
{
	struct cs8416_chip *_chip;
	unsigned char  uc_read_value =0;
	_chip = container_of(work, struct cs8416_chip, work.work);


	if  (g_optical_rx_status == 1)
	{
		if ((read_reg(g_i2c_client_CS8416,0x0B) & 0x78)) {  //check input bits

			uc_read_value=read_reg(g_i2c_client_CS8416,0x0B) & 0x02;		//digital silence bit !!!

			if ((uc_read_value == 0x02)&&(	toggle_flag == 0))
			{
				toggle_flag = 1;

				//	printk("GPIO_RX_PLL_UNLOCK high Timer \n");
				ak_set_hw_mute(AMT_HP_MUTE,1);		// RX 일때.. mute
				write_reg(g_i2c_client_CS8416,0x01, 0xD6);		//sdout mute

				msleep_interruptible(50);
			}
			else if ((uc_read_value == 0x00)&&(toggle_flag ==1))
			{
				toggle_flag = 0;

				//	printk("GPIO_RX_PLL_UNLOCK low Timer \n");
				ak_set_hw_mute(AMT_HP_MUTE,0);		// RX 일때.. mute 해제 !!
				write_reg(g_i2c_client_CS8416,0x01, 0x92);		//sdout not mute
			}
		} 
		else 
			ak_set_hw_mute(AMT_HP_MUTE,1);
		
	}

	if ( toggle_flag == 1)
		schedule_delayed_work(&_chip->work, RX_TIMER_DELAY_200);
	else if ( toggle_flag == 0)
		schedule_delayed_work(&_chip->work, RX_TIMER_DELAY_2);
	else		
		schedule_delayed_work(&_chip->work, RX_TIMER_DELAY);
}
#endif

static int __devinit CS8416_probe(struct i2c_client *		client,
				   const struct i2c_device_id * id)
{
	struct cs8416_chip *_chip;
	

	_chip = kzalloc(sizeof(*_chip), GFP_KERNEL);
	if (!_chip)
		goto  fail;

	//  RX init value NO POWER   &   LOW current
	gpio_request(GPIO_SPDIF_RX_ON, "GPIO_SPDIF_RX_ON");
	tcc_gpio_config(GPIO_SPDIF_RX_ON,GPIO_FN(0));
	gpio_direction_output(GPIO_SPDIF_RX_ON,0);			// NO power


	gpio_request(GPIO_SPDIF_RX_RST, "GPIO_SPDIF_RX_RST");
	tcc_gpio_config(GPIO_SPDIF_RX_RST,GPIO_FN(0));
	gpio_direction_output(GPIO_SPDIF_RX_RST,0);		// Low current

///////////////////////////////////////////////////////////////////////
#ifdef USE_TIMER_PLL_LOCK_CHECK
	tcc_gpio_config_ext_intr(INT_EINT9, EXTINT_GPIOG_12);
	gpio_request(GPIO_RX_PLL_UNLOCK, "GPIO_RX_PLL_UNLOCK");
	tcc_gpio_config(GPIO_RX_PLL_UNLOCK, GPIO_FN(0) |GPIO_PULLUP);
	gpio_direction_input(GPIO_RX_PLL_UNLOCK);
	tcc_gpio_config_ext_intr(INT_EINT9, EXTINT_GPIOG_12);


	_chip->irq = gpio_to_irq(GPIO_RX_PLL_UNLOCK);

#ifdef USE_INTERRUPT_PLL_LOCK
	if (request_irq(_chip->irq, cs8416_irq_handler, IRQF_TRIGGER_RISING, "rx_irq_detect", _chip)) 
	{
		printk("request rising edge of irq#%d failed!\n", _chip->irq);	
		toggle_flag = 0;
	}
#endif

	INIT_DELAYED_WORK_DEFERRABLE(&_chip->work, rx_check_timer_work);
	schedule_delayed_work(&_chip->work, RX_TIMER_DELAY);	
#endif	
///////////////////////////////////////////////////////////////////////	
	
	if(!strcmp(id->name,CS8416_NAME)) {
		g_i2c_client_CS8416 = client;			
		i2c_set_clientdata(client, NULL);

//		init_reg_CS8416();

	}
	
	return 0;
fail:
	kfree(_chip);
	return 0;
}

#ifdef CONFIG_PM

static int CS8416_suspend(struct i2c_client *client,
		pm_message_t state)
{
	//CS8416_pwr(0);
	return 0;
}

static int CS8416_resume(struct i2c_client *client)
{

	//CS8416_reg_init();
	return 0;
}

#else
#define CS8416_suspend NULL
#define CS8416_resume NULL
#endif


static int __devexit CS8416_remove(struct i2c_client *client)
{


	return 0;
}


static struct i2c_device_id CS8416_idtable[] = {
	{ CS8416_NAME, 0 },
	{},
};



MODULE_DEVICE_TABLE(i2c, CS8416_idtable);

static const struct dev_pm_ops CS8416_pm_ops = {
	#ifdef CONFIG_PM
	.suspend	= CS8416_suspend,
	.resume		= CS8416_resume,
	#else
	.suspend	= NULL,
	.resume		= NULL,
	#endif
};


static struct i2c_driver CS8416_i2c_driver = {
	.id_table	= CS8416_idtable,
	.probe		= CS8416_probe,
	.remove		= __devexit_p(CS8416_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= CS8416_NAME,
		.pm	= NULL,
	},
};



static int __init CS8416_init(void)
{
	//#if(CUR_DAC == CODEC_ES9018)
	
	return i2c_add_driver(&CS8416_i2c_driver);

	//#else
	//return 0;
	//#endif
}


static void __exit CS8416_exit(void)
{
	i2c_del_driver(&CS8416_i2c_driver);
}

module_init(CS8416_init);
module_exit(CS8416_exit);

MODULE_DESCRIPTION("CS8416 SPDIF");
MODULE_LICENSE("GPL");


#endif
