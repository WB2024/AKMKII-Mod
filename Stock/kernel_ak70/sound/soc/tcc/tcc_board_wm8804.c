/*
 * linux/sound/soc/tcc/tcc_board.c
 *
 * Author:  <linux@telechips.com>
 * Created: Nov 30, 2007
 * Description: SoC audio for TCCxx
 *
 * Copyright (C) 2008-2009 Telechips 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/bsp.h>
#include <mach/tca_ckc.h>
#include <asm/mach-types.h>

#include "tcc/tca_tcchwcontrol.h"
#include "../codecs/wm8804.h"
#include "tcc-pcm.h"
#include "tcc-i2s.h"
#include <mach/board_astell_kern.h>


#define USE_SPDIF_WORKQUEUE

// /* audio clock in Hz - rounded from 12.235MHz */
//#define TCC83X_AUDIO_CLOCK 12288000


//#define joo_printk(f, a...)    printk("  ***** BOARD * [%s:%d] " f, __func__, __LINE__, ##a) 
#define joo_printk(f, a...)


static int tcc_startup(struct snd_pcm_substream *substream)
{
	joo_printk("\n");

	return 0;
}

/* we need to unmute the HP at shutdown as the mute burns power on tcc83x */
static void tcc_shutdown(struct snd_pcm_substream *substream)
{
    joo_printk("\n");
}

static int tcc_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned int clk = 0;
	int ret = 0;

	#ifdef PRINT_ALSA_MSG
	DBG_PRINT("WM8804","%d\n",params_rate(params));
	#endif


#ifdef _MTP_SOUND_BREAK_FIX
if(params_rate(params) < 96000)  {
	mtp_set_play_status(MPS_UNDER_96);
} else {
	mtp_set_play_status(MPS_OVER_96);
}
#endif	


	#ifdef AUDIO_POWER_CTL

	//wm8804_reinit();

//	cs4398_reg_init();
	#endif
 	
#if(CUR_DAC == CODEC_AK4490)
        ak4490_FM(params_rate(params));
#endif

	switch (params_rate(params)) {
    case 22000:
        clk = 22050 * 256;
        break;
    case 11000:
        clk = 11025 * 256;
        break;

	case 48000:
	case 96000:
		clk = 12288000;
		break;

	case 88200:
	case 44100:
		clk = 11289600;
		break;

    default:
        clk = params_rate(params) * 256;
        break;
	}


#ifdef AK_WM8804_MASTER
/* set codec DAI configuration */
ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
	SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
#else
	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
#endif

	if (ret < 0)
		return ret;

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* set the codec system clock for DAC and ADC */
    /*
	ret = codec_dai->dai_ops.set_sysclk(codec_dai, WM8805_SYSCLK, clk,
		SND_SOC_CLOCK_IN);
        */
#if 0 //tonny
	ret = snd_soc_dai_set_sysclk(codec_dai, WM8805_SYSCLK, clk,
		SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;
#endif
#if 0 //tonny
	ret = snd_soc_dai_set_sysclk(codec_dai, WM8804_MCLK_SETTING, params_rate(params), SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;
#else
	ret = snd_soc_dai_set_pll(codec_dai, 0, 0, params_rate(params), 0); //JHLIM
	if (ret < 0)
		return ret;
#endif

	/* set the I2S system clock as input (unused) */
//       ret = cpu_dai->dai_ops.set_sysclk(cpu_dai, PXA2XX_I2S_SYSCLK, 0,SND_SOC_CLOCK_IN);    
    //	ret = cpu_dai->dai_ops.set_sysclk(cpu_dai, 0, 0, SND_SOC_CLOCK_IN);
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, 0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;
 
	return 0;
}

static struct snd_soc_ops tcc_ops = {
	.startup = tcc_startup,
	.hw_params = tcc_hw_params,
	.shutdown = tcc_shutdown,
};




/*
 * Logic for a wm8804 as connected on a Sharp SL-C7x0 Device
 */
static int tcc_wm8804_init(struct snd_soc_pcm_runtime *rtd)
{
    return 0;
}

extern struct snd_soc_platform tcc_soc_platform;

#if 0//!defined(CONFIG_SND_TCC_DAI2SPDIF)
static int tcc_iec958_dummy_init(struct snd_soc_pcm_runtime *rtd)
{
    return 0;
}
#endif

/* tcc digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link tcc_dai[] = {
    {
        .name = "WM8804",
        .stream_name = "WM8804",
        .platform_name  = "tcc-pcm-audio",
        .cpu_dai_name   = "tcc-dai-i2s",

        .codec_name = "wm8804.0-003b",
        .codec_dai_name = "wm8804-spdif",
        .init = &tcc_wm8804_init,
        .ops = &tcc_ops,
    },
};


/* tcc audio machine driver */
static struct snd_soc_card tcc_soc_card = {
	.driver_name      = "Iriver Audio",
    .long_name = "Iriver Hi-Fi Audio",
	.dai_link  = tcc_dai,
	.num_links = ARRAY_SIZE(tcc_dai),
};


/* 
 * FIXME: This is a temporary bodge to avoid cross-tree merge issues. 
 * New drivers should register the wm8804 I2C device in the machine 
 * setup code (under arch/arm for ARM systems). 
 */
static int wm8804_i2c_register(void)
{
    struct i2c_board_info info;
    struct i2c_adapter *adapter;
    struct i2c_client *client;
    
    DBG_PRINT("WM8804","%s() %x \n", __func__,0x3b >> 1);

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = 0x3b;
    strlcpy(info.type, "wm8804", I2C_NAME_SIZE);

    adapter = i2c_get_adapter(0);

    if (!adapter) 
    {
        DBG_PRINT("WM8804", "can't get i2c adapter 0\n");
        return -ENODEV;
    }

    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if (!client) 
    {
        DBG_PRINT("WM8804","can't add i2c device at 0x%x\n", (unsigned int)info.addr);
        return -ENODEV;
    }
    return 0;
}


static struct platform_device *tcc_snd_device;
extern unsigned int tca_tcc_initport_wm8804(void);

#include <linux/kthread.h>

extern void wm8804_check_received_spdif(void);
#ifdef USE_SPDIF_WORKQUEUE
struct spdif_wq_struct
{
    struct delayed_work work;
    unsigned int poll_interval;
};

static struct workqueue_struct *spdif_wq = NULL;
static struct spdif_wq_struct *spdif_delayed_work = NULL;

static void spdif_spy_work(struct work_struct *work)
{
    if(ak_get_wm8804_spdif_mode() == SND_WM8804_SPDIF_BYPASS)
        wm8804_check_received_spdif();
    
    queue_delayed_work(spdif_wq, &spdif_delayed_work->work, msecs_to_jiffies(1000));
}
#else
int spdif_spy_thread(void *data)
{
 	while(1)
	{
		msleep(1000);
		//msleep(300);
        
		if(ak_get_wm8804_spdif_mode() == SND_WM8804_SPDIF_BYPASS)
		{
			wm8804_check_received_spdif();
		}

		if (kthread_should_stop())
			break;
	}
}
#endif

static int __init tcc_init_wm8804(void)
{
    int ret;

  //  if( !(machine_is_tcc8800() || machine_is_tcc8920() || machine_is_tcc893x()) ) {
   //     DBG_PRINT("WM8804","\n\n\n\n%s() do not execution....\n\n", __func__);
    //    return 0;
   // }
    
    DBG_PRINT("WM8804","TCC Board probe [%s]\n", __FUNCTION__);
    
//    if(machine_is_tcc893x()) 
        tcc_soc_card.name = "TCC893x EVM";

    ak_set_hw_mute(AMT_DAC_MUTE,1);
	ak_set_hw_mute(AMT_HP_MUTE,1);

    tca_tcc_initport();

	tca_tcc_initport_wm8804();

//  tca_ckc_pclk_enable(PERI_DAI, 1);	   // B090183, enable after clock control implement
//   tca_ckc_setiobus(RB_DAICDIFCONTROLLER, 1);
//  tca_ckc_pclk_enable(PERI_AUD, 1);	   // B090183, enable after clock control implement
 //  tca_ckc_setiobus(RB_ADMACONTROLLER, 1);
	#ifdef ENABLE_DEBUG_PORT
	tcc_gpio_config(GPIO_DBG_PORT,GPIO_OUTPUT | GPIO_FN0);
	#endif

    ret = wm8804_i2c_register();

	tcc_snd_device = platform_device_alloc("soc-audio", -1);
	if (!tcc_snd_device)
		return -ENOMEM;

	platform_set_drvdata(tcc_snd_device, &tcc_soc_card);

	ret = platform_device_add(tcc_snd_device);
	if (ret) {
        printk(KERN_ERR "Unable to add platform device\n");\
        platform_device_put(tcc_snd_device);
    }
    
#ifdef USE_SPDIF_WORKQUEUE
    spdif_delayed_work = (struct spdif_wq_struct *)kmalloc(sizeof(struct spdif_wq_struct), GFP_KERNEL);
    INIT_DELAYED_WORK(&spdif_delayed_work->work, spdif_spy_work);

    spdif_wq = create_workqueue("spdif_wq");
    if (spdif_wq == NULL) {
        kfree(spdif_delayed_work);
        return -ENOMEM;
    }
    queue_delayed_work(spdif_wq, &spdif_delayed_work->work, msecs_to_jiffies(1000));
#else
	kthread_run(spdif_spy_thread,NULL,"kthread");
#endif

	return ret;
}

static void __exit tcc_exit_wm8804(void)
{
#ifdef USE_SPDIF_WORKQUEUE
    destroy_workqueue(spdif_wq);
    kfree(spdif_delayed_work);
#endif
    
	platform_device_unregister(tcc_snd_device);
}

module_init(tcc_init_wm8804);
module_exit(tcc_exit_wm8804);

/* Module information */
MODULE_AUTHOR("linux <linux@telechips.com>");
MODULE_DESCRIPTION("ALSA SoC TCCxx Board");
MODULE_LICENSE("GPL");
