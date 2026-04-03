/*
 * pga2505.c  --  
 *
 * Copyright 2015 Iriver
 *
 * This program control pag2505 used by I2C to SPI used SC18IS602B 
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

static struct i2c_client *g_i2c_client_PGA2505_L;
static struct i2c_client *g_i2c_client_PGA2505_R;


#define PGA2505_NAME_L "PGA2505_L"
#define PGA2505_NAME_R "PGA2505_R"


#define MAX_UI_VOLUME_IDX (150)		// range 0~150
#define LOW_GAIN 1
#define HIGH_GAIN 2


static unsigned char g_pga2505_settingValue = 0x40;	//DC servo En, CM servo En, Zero Crossing Detect, Over randge det, GPO ~~
static unsigned char g_pga2505_preampGain = -1;
#define FUNCTION_ID_WRITE  0x01   //SS3, SS2, SS1, SS0	used SS0 = CS
#define FUNCTION_ID_SPI 0xF0	// config spi
#define FUNCTION_ID_SPI_MODE  0x0E	// 0000 1110 = SPI mode 3, 1843kHz
#define NULL_BYTE  0x00


static int write_reg (struct i2c_client *client, u8 reg, u8 data1, u8 data2, u8 nbyte)
{
	int ret = 0;
	uint8_t buf[3];
	buf[0] = reg;			//function ID 
	buf[1] = data1;
	buf[2] = data2;

	//printk ("reg:%d, data1:0x%x, , data2:0x%x\n", reg, data1, data2);

	ret = i2c_master_send(client, buf, nbyte);
	if (ret < 0)  {
		printk ("Write Error addr:%x\n", client->addr);
		return -1;
	}

	return 0;
}



int PGA2505_WRITE(int ch, u8 reg, u8 data1, u8 data2, u8 nbyte)
{

	if(ch==0) {
		if(!g_i2c_client_PGA2505_L) {
			DBG_PRINT("PGA2505_WRITE","ERROR \n");
			return -1;
		}
		return write_reg(g_i2c_client_PGA2505_L,reg,data1,data2,nbyte);
	}else{
		if(!g_i2c_client_PGA2505_R) {
			DBG_PRINT("PGA2505_WRITE","ERROR \n");
			return -1;
		}
		return write_reg(g_i2c_client_PGA2505_R,reg,data1,data2,nbyte);
	}
}






void init_reg_PGA2505(void)
{
	g_pga2505_preampGain	= 0x00;		// center value

	PGA2505_WRITE(0, FUNCTION_ID_SPI, FUNCTION_ID_SPI_MODE, NULL_BYTE, 2);
	PGA2505_WRITE(1, FUNCTION_ID_SPI, FUNCTION_ID_SPI_MODE, NULL_BYTE, 2);

	PGA2505_WRITE(0, FUNCTION_ID_WRITE, g_pga2505_settingValue, g_pga2505_preampGain, 3);
	PGA2505_WRITE(1, FUNCTION_ID_WRITE, g_pga2505_settingValue, g_pga2505_preampGain, 3);

	
}



static struct delayed_work g_power_on_work;
static struct delayed_work g_power_off_work;


static void PGA2505_power_on_work(struct work_struct *work)
{
	

}
static void PGA2505_power_off_work(struct work_struct *work)
{

}

void PGA2505_schedule_power_ctl(int onoff)
{
//	if(onoff==1) {		
//		schedule_delayed_work(&g_power_on_work, msecs_to_jiffies(200));
//	} else {
//		schedule_delayed_work(&g_power_off_work, 0);
//	}
}


static int __devinit PGA2505_probe(struct i2c_client *		client,
				   const struct i2c_device_id * id)
{

	if(!strcmp(id->name,PGA2505_NAME_L)) {
		g_i2c_client_PGA2505_L= client;			
		i2c_set_clientdata(client, NULL);
		printk("PGA2505_L_probe\n");
		
	}else{
		g_i2c_client_PGA2505_R= client;
		i2c_set_clientdata(client, NULL);
		printk("PGA2505_R_probe\n");

	}
	
	return 0;
}


#define PGA2505_suspend NULL
#define PGA2505_resume NULL


static int __devexit PGA2505_remove(struct i2c_client *client)
{
	return 0;
}


static struct i2c_device_id PGA2505_L_idtable[] = {
	{ PGA2505_NAME_L, 0 },
	{},
};

static struct i2c_device_id PGA2505_R_idtable[] = {
	{ PGA2505_NAME_R, 0 },
	{},
};



MODULE_DEVICE_TABLE(i2c, PGA2505_L_idtable);
//MODULE_DEVICE_TABLE(i2c, PGA2505_R_idtable);


static const struct dev_pm_ops PGA2505_pm_ops = {
	.suspend	= NULL,
	.resume		= NULL,
};


static struct i2c_driver PGA2505_L_i2c_driver = {
	.id_table		= PGA2505_L_idtable,
	.probe		= PGA2505_probe,
	.suspend		= PGA2505_suspend,
	.resume		= PGA2505_resume,
	.remove		= __devexit_p(PGA2505_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= PGA2505_NAME_L,
		.pm	= NULL,
	},
};



static struct i2c_driver PGA2505_R_i2c_driver = {
	.id_table		= PGA2505_R_idtable,
	.probe		= PGA2505_probe,
	.suspend		= PGA2505_suspend,
	.resume		= PGA2505_resume,
	.remove		= __devexit_p(PGA2505_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= PGA2505_NAME_R,
		.pm	= NULL,
	},
};

static int __init PGA2505_init(void)
{

	printk("PGA2505_init \n");

	
	i2c_add_driver(&PGA2505_L_i2c_driver);

	return i2c_add_driver(&PGA2505_R_i2c_driver);


}


static void __exit PGA2505_exit(void)
{
	i2c_del_driver(&PGA2505_L_i2c_driver);
	i2c_del_driver(&PGA2505_R_i2c_driver);
}

module_init(PGA2505_init);
module_exit(PGA2505_exit);

MODULE_DESCRIPTION("PGA2505 gain control IC");
MODULE_LICENSE("GPL");


#endif
