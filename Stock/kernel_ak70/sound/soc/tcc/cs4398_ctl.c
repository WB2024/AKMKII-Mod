/*
 * cs4398_ctl.c  --  
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


#if(CUR_AK==MODEL_AK500N) 
//jimi.140807
extern CONN_HP_TYPE is_hp_connected(void); 
extern AUDIO_OUT_TYPE get_cur_audio_out_type(void);
#endif

#if(CUR_DAC==CODEC_CS4398)



//-	Right DAC I2C Add : 1001111, ATAPI Register : 00101
//-	Left DAC I2C Add : 1001100, ATAPI Register : 01010

#ifdef CODEC_CS4398_DUAL
static struct i2c_client *g_i2c_client_cs4398_1;   //  CH R 
#endif

static struct i2c_client *g_i2c_client_cs4398_2;   //  CH L

#define CS4398_1_NAME "CS4398_1"
#define CS4398_2_NAME "CS4398_2"




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


int CS4398_WRITE(int ch,unsigned int reg, unsigned int val)
{

	#ifdef CODEC_CS4398_DUAL  
	if(ch==0) {
		if(!g_i2c_client_cs4398_1) {
			DBG_PRINT("CS4308","ERROR \n");
			return -1;
		}
		return write_reg(g_i2c_client_cs4398_1,reg,val);
	} else 
	#endif
	
	{
		if(!g_i2c_client_cs4398_2) {
			DBG_PRINT("CS4308","ERROR \n");
			return -1;
		}
		return write_reg(g_i2c_client_cs4398_2,reg,val);

	}
}


int CS4398_READ(int ch,unsigned char reg)
{
	#ifdef CODEC_CS4398_DUAL  
	if(ch==0) {
		if(!g_i2c_client_cs4398_1) {
			DBG_PRINT("CS4308","ERROR \n");

			return -1;
		}
		return read_reg(g_i2c_client_cs4398_1, reg);
	} else 
	#endif
	
	{

		if(!g_i2c_client_cs4398_2) {
			DBG_PRINT("CS4308","ERROR \n");
		
			return -1;
		}
		return read_reg(g_i2c_client_cs4398_2, reg);
	}
}

static int g_cs4398_lvol = -1;
static int g_cs4398_rvol = -1;

void READ_ALL_REG(void)
{
	unsigned char add;

	for(add=0; add<=0x9; add++){
		printk("CS4398_1[0x%x]: 0x%02x\n",add,CS4398_READ(0, add));
		printk("CS4398_2[0x%x]: 0x%02x\n",add,CS4398_READ(1, add));
	}
}

// 2016.06.08 MANTIS 0039593 
//터치로 볼륨 컨트롤할경우에 소리 부자연스러운 문제 수정. 
//터치로 볼륨 컨트롤 할경우에 VOLUME RAMP 기능을 Soft Ramp Zero Crossing 로 셋팅하고
// 3초후에 Soft Ramp 로 바꿈
//#define PRINT_MSG2

#ifdef USE_VOLUME_WHEEL_RAMP

#ifdef USE_VWR_TYPE1

struct timer_list g_cs4398_timer;

static int g_volume_ramp_counter = -1;

#define TIMER_INTERVAL msecs_to_jiffies(200)
	

static void cs4398_volume_ramp_work(unsigned long data)    
{

	if(g_volume_ramp_counter++==3) {
		#ifdef PRINT_MSG2
		CPRINT("K RAMP OFF\n");
		#endif
		
		CS4398_WRITE(1,0x7,0b10110000);
	}
	
    mod_timer(&g_cs4398_timer, jiffies + TIMER_INTERVAL);
}
void cs4398_volume_ramp_reset(void)
{
	g_volume_ramp_counter = 0;
}


extern int is_touch_pressed(void);
#endif



void cs4398_ramp_mode(int mode)
{
	//CPRINT("RAMP %d\n",mode);

	if(mode == CS4398_RAMP_SR) {
		CS4398_WRITE(0,0x7,0b10110000);
		CS4398_WRITE(1,0x7,0b10110000);
	} else {
		CS4398_WRITE(0,0x7,0b11110000);
		CS4398_WRITE(1,0x7,0b11110000);
	}

}


#endif


void cs4398_set_volume(int mode, int lvol,int rvol)
{
	unsigned short codec_lvol,codec_rvol;
	unsigned short lvol_lsb,lvol_msb,rvol_lsb,rvol_msb;
//	int start_offset = 20;
//	DBG_CALLER("\n");

	if(mode ==1) {
		g_cs4398_lvol = lvol;
		g_cs4398_rvol = rvol;
	}
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

#if (CUR_AK == MODEL_AK500N)
	if ((get_cur_audio_out_type() != AUDIO_OUT_TYPE_FIX_UNBAL &&  get_cur_audio_out_type() != AUDIO_OUT_TYPE_AUDIO_OUT1) 
		&& (get_cur_audio_out_type() != AUDIO_OUT_TYPE_VAR_UNBAL &&  get_cur_audio_out_type() != AUDIO_OUT_TYPE_AUDIO_OUT2))
	{
#endif
		if(lvol != 0) { 
			codec_lvol += MAX_VOLUME_OFFSET;
		}

		if(rvol !=0) {
			codec_rvol += MAX_VOLUME_OFFSET;
		}
#if (CUR_AK == MODEL_AK500N)
	}
#endif

	#ifdef PRINT_VOLUME_VALUE
	CPRINT("(lvol:%d(%x) rvol:%d(%x))\n", lvol,codec_lvol,rvol,codec_rvol); 		
	#endif


	


#ifdef  CODEC_CS4398_DUAL	

	CS4398_WRITE(1,0x5,codec_lvol);
	CS4398_WRITE(1,0x6,codec_lvol);

	CS4398_WRITE(0,0x5,codec_rvol);
	CS4398_WRITE(0,0x6,codec_rvol);

	
#else

#ifdef USE_VWR_TYPE1
	{

		if(is_touch_pressed()) {
			g_volume_ramp_counter = -1;
			#ifdef PRINT_MSG2	
			CPRINT("T RAMP ON\n");
			#endif
			
			CS4398_WRITE(1,0x7,0b11110000);
		} else {
			if(g_volume_ramp_counter==0) {
				#ifdef PRINT_MSG2
				CPRINT("K RAMP ON \n");		
				#endif
				
				CS4398_WRITE(1,0x7,0b11110000);
			} else {
				#ifdef PRINT_MSG2
				CPRINT("T RAMP OFF 2\n");
				#endif
				
				CS4398_WRITE(1,0x7,0b10110000);
			}
		}
	}
	#endif
	
	CS4398_WRITE(1,0x5,codec_lvol);
	CS4398_WRITE(1,0x6,codec_rvol);
	
#endif
}

void cs4398_reg_freeze(int freeze)
{
	if(freeze)  {
		CS4398_WRITE(1,0x8,0b01000000);
		CS4398_WRITE(0,0x8,0b01000000);
	} else {
		CS4398_WRITE(1,0x8,0b01100000);
		CS4398_WRITE(0,0x8,0b01100000);
	}
}

void cs4398_volume_mute(void)
{
	CS4398_WRITE(1,0x5,255);
	CS4398_WRITE(1,0x6,255);

	CS4398_WRITE(0,0x5,255);
	CS4398_WRITE(0,0x6,255);
}

void cs4398_resume_volume(void)
{
//jimi.0807
#if (CUR_AK==MODEL_AK500N)
	if(is_hp_connected() == CONN_HP_TYPE_NONE) {
		 cs4398_set_volume(0, 150, 150);
		 return; 
	}
#endif
		
	if(g_cs4398_lvol != -1)  {
		cs4398_set_volume(1,g_cs4398_lvol,g_cs4398_rvol);
	}
}

void cs4398_get_volume(int *lvol,int *rvol)
{
	*lvol = g_cs4398_lvol;
	*rvol = g_cs4398_rvol;
}

struct cs4398_reg 
{
unsigned char reg;
unsigned char value;
};

#ifdef CODEC_CS4398_DUAL  
static struct cs4398_reg g_cs4398_1_reg[] =
{
{2,  0b00010000},
{3,  0b00000101},
{4,  0b00000000},
{5,  0b10000000},
{6,  0b10000000},
//{7,  0b00000000},
//{7,  0b01000000},  /* 2014-1-9 */

#ifdef USE_VWR_TYPE2
{7,  0b11110000},
#else
{7,  0b10110000},
#endif	

{8,  0b01000000},   //  Freeze Controls (Freeze) Bit 5  if 0 : simultaneously   /* 2013-9-11 */
{9,  0b00001000}
};
#endif


static struct cs4398_reg g_cs4398_2_reg[] =
{

{2,  0b00010000},

#ifdef CODEC_CS4398_DUAL  
{3,  0b00001010},
#else  			// for AK200
{3,  0b00001001},
#endif
{4,  0b00000000},
{5,  0b10000000},
{6,  0b10000000},
//{7,  0b00000000},
//{7,  0b01000000},
#ifdef USE_VWR_TYPE2
{7,  0b11110000},
#else
{7,  0b10110000},
#endif

{8,  0b01000000},
{9,  0b00001000}
};


void init_reg(int ch)
{
int i;
	#ifdef CODEC_CS4398_DUAL  
	if(ch==0) {

		for(i=0;i < ARRAY_SIZE(g_cs4398_1_reg);i++) {
			write_reg(g_i2c_client_cs4398_1,g_cs4398_1_reg[i].reg,g_cs4398_1_reg[i].value);
		}
		
	} else 
	#endif
	
	{
	
		for(i=0;i < ARRAY_SIZE(g_cs4398_2_reg);i++) {
			write_reg(g_i2c_client_cs4398_2,g_cs4398_2_reg[i].reg,g_cs4398_2_reg[i].value);
		}

	}

}

// SPEED mode.
static int g_fm_freq = -1;

void cs4398_FM(int freq)
{

//	00 - Single-Speed Mode (30 to 50 kHz sample rates)
//	01 - Double-Speed Mode (50 to 100 kHz sample rates)
//	10 - Quad-Speed Mode (100 to 200 kHz sample rates)
//	11 - Direct Stream Digital Mode
//	if(g_fm_freq==freq) {
//		return;
//	}
	DBG_ERR("d","FM:%d\n",freq);

	#if(AUDIO_CLOCK == CLK_AK8157A)
	ak8157a_set_samplerate(freq);
	#endif
			
	g_fm_freq= freq;
	
	if(freq<= 48000)  {
		CS4398_WRITE(0,0x2, 0b00010000);
		CS4398_WRITE(1,0x2, 0b00010000);
	} else if(freq > 96000)  {
		CS4398_WRITE(0,0x2, 0b00010010);
		CS4398_WRITE(1,0x2, 0b00010010);
	} else {
		CS4398_WRITE(0,0x2, 0b00010001);
		CS4398_WRITE(1,0x2, 0b00010001);
	}
}

void cs4398_FM_reinit(void)
{

	if(g_fm_freq) {
		cs4398_FM(g_fm_freq);
	}
}

void cs4398_mute(int onoff)
{

	printk("%s, %d\n", __FUNCTION__, onoff);

	if(onoff== 1) {
#if 1
		CS4398_WRITE(0,0x4, 0b00011000);
		CS4398_WRITE(1,0x4, 0b00011000);
#endif
//		CS4398_WRITE(0, 0x03, 0b00000000);
//		CS4398_WRITE(1, 0x03, 0b00000000);

	} else {

#if 1
		CS4398_WRITE(0,0x4, 0b00000000);
		CS4398_WRITE(1,0x4, 0b00000000);
#endif

		
//		CS4398_WRITE(0, 0x03, 0b00000101);
//		CS4398_WRITE(1, 0x03, 0b00000101);

//		CS4398_WRITE(0, 0x03, 0b00001001);
//		CS4398_WRITE(1, 0x03, 0b00001001);
	}

	#ifdef CODEC_CS4398_DUAL
	printk("cs4398_1_probed CHIP ID : %x \n",read_reg(g_i2c_client_cs4398_1,0x1));	
	#endif
	printk("cs4398_2_probed CHIP ID : %x \n",read_reg(g_i2c_client_cs4398_2,0x1));	
	
}

void cs4398_pwr(int onoff)
{
	int lvalue,rvalue;	
	
	if(onoff== 1) {
		DBG_ERR("d","pwr on\n");
		cs4398_mute(1);
		//msleep_interruptible(10);		
		CS4398_WRITE(0,0x8, 0b01000000);
		CS4398_WRITE(1,0x8, 0b01000000);
		//msleep_interruptible(10);
		cs4398_mute(0);	
	} else {
		DBG_ERR("d","pwr off\n");
		cs4398_mute(1);	
		CS4398_WRITE(0,0x8, 0b11000000);
		CS4398_WRITE(1,0x8, 0b11000000);
	}

}

void cs4398_reg_init(void)
{

	init_reg(0);
	init_reg(1);

	cs4398_resume_volume();	
}

static int __devinit cs4398_probe(struct i2c_client *		client,
				   const struct i2c_device_id * id)
{
	int ret;

	int i;
	#ifdef CODEC_CS4398_DUAL  
	if(!strcmp(id->name,CS4398_1_NAME)) {
		g_i2c_client_cs4398_1 = client;			
		i2c_set_clientdata(client, NULL);

//		init_reg(0);
		printk("cs4398_1_probed CHIP ID : %x \n",read_reg(client,0x1));	
	} else 
	#endif
	{
		g_i2c_client_cs4398_2 = client;
		i2c_set_clientdata(client, NULL);

		printk("cs4398_2_probed CHIP ID : %x \n",read_reg(client,0x1));	
//		init_reg(1);
	}

	#if 0
	for(i=0;i<3000;i++) {
		ret = read_reg(client,0x1);
		printk("cs4398_probed CHIP ID : %x \n",ret);
		msleep(100);
	}
	#endif

	return 0;
}

#ifdef CONFIG_PM

static int cs4398_suspend(struct i2c_client *client,
		pm_message_t state)
{

	return 0;
}

static int cs4398_resume(struct i2c_client *client)
{


	return 0;
}

#else
#define wm8804_suspend NULL
#define wm8804_resume NULL
#endif


static int __devexit cs4398_remove(struct i2c_client *client)
{


	return 0;
}

#ifdef CODEC_CS4398_DUAL  
static struct i2c_device_id cs4398_1_idtable[] = {
	{ CS4398_1_NAME, 0 },
	{},
};
#endif

static struct i2c_device_id cs4398_2_idtable[] = {
	{ CS4398_2_NAME, 0 },
	{},
};


MODULE_DEVICE_TABLE(i2c, cs4398_1_idtable);

static const struct dev_pm_ops cs4398_pm_ops = {
	#ifdef CONFIG_PM
	.suspend	= cs4398_suspend,
	.resume		= cs4398_resume,
	#else
	.suspend	= NULL,
	.resume		= NULL,
	#endif
};

#ifdef CODEC_CS4398_DUAL  
static struct i2c_driver cs4398_1_i2c_driver = {
	.id_table	= cs4398_1_idtable,
	.probe		= cs4398_probe,
	.remove		= __devexit_p(cs4398_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= CS4398_1_NAME,
		.pm	= NULL,
	},
};
#endif

static struct i2c_driver cs4398_2_i2c_driver = {
	.id_table	= cs4398_2_idtable,
	.probe		= cs4398_probe,
	.remove		= __devexit_p(cs4398_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= CS4398_2_NAME,
		.pm	= &cs4398_pm_ops,
	},
};


static int __init cs4398_init(void)
{

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300 || CUR_AK==MODEL_AK500N)                    
	gpio_request(GPIO_I2C_SEL, "GPIO_I2C_SEL");
	
	tcc_gpio_config(GPIO_I2C_SEL,GPIO_FN(0));
	gpio_direction_output(GPIO_I2C_SEL,0);
#endif

	printk("cs4398 dual dac\n");

	gpio_request(GPIO_DAC_RST, "GPIO_DAC_RST");

	tcc_gpio_config(GPIO_DAC_RST,GPIO_FN(0));
	gpio_direction_output(GPIO_DAC_RST,1);

	msleep(10);
	
	gpio_direction_output(GPIO_DAC_RST,0);

	#ifdef CODEC_CS4398_DUAL  
	i2c_add_driver(&cs4398_1_i2c_driver);
	#endif

	#ifdef USE_VWR_TYPE1
	
	init_timer(&g_cs4398_timer);
	g_cs4398_timer.function = cs4398_volume_ramp_work;
	g_cs4398_timer.expires = jiffies + TIMER_INTERVAL;	// 1 secs.
	add_timer(&g_cs4398_timer);

	#endif
	
    return i2c_add_driver(&cs4398_2_i2c_driver);
}


static void __exit cs4398_exit(void)
{
	#ifdef CODEC_CS4398_DUAL  
	i2c_del_driver(&cs4398_1_i2c_driver);
	#endif
	i2c_del_driver(&cs4398_2_i2c_driver);
}


module_init(cs4398_init);
module_exit(cs4398_exit);


MODULE_DESCRIPTION("CS4398 DAC");
MODULE_LICENSE("GPL");

#endif

