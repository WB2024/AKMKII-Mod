/*
 * ak5572.c  --  audio driver for AK5572
 *
 * Copyright (C) 2015 Asahi Kasei Microdevices Corporation
 *  Author                Date        Revision
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                      15/10/21	    1.0
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/of_gpio.h>

#include "ak5572.h"
#include <sound/ak5572_pdata.h>

//#define AK5572_PDN_GPIO
//#define AK5572_PD_SUSPEND
//#define AK5572_VERSIONUP

#define AK5572_DEBUG			//used at debug mode

#ifdef AK5572_DEBUG
#define akdbgprt printk
#else
#define akdbgprt(format, arg...) do {} while (0)
#endif

/* AK5572 Codec Private Data */
struct ak5572_priv {
	struct snd_soc_codec codec;
	struct i2c_client *i2c;
	u8 reg_cache[AK5572_MAX_REGISTERS];
 	int fs;     // Sampling Frequency
	int rclk;	// Master Clock
	int pdn_gpio;
};

/* ak5572 register cache & default register settings */
#if 1 //J151030
static const u8 ak5572_reg[AK5572_MAX_REGISTERS] = {
	0x03,	/*	0x00	AK5572_00_POWER_MANAGEMENT1  */
	0x01,	/*	0x01	AK5572_01_POWER_MANAGEMENT2  */
	0x1b,	/*	0x02	AK5572_02_CONTROL1           */
	0x00,	/*	0x03	AK5572_03_CONTROL2		     */
	0x00,	/*	0x04	AK5572_04_CONTROL3	         */
	0x00,	/*	0x05	AK5572_05_DSD			 	*/
};
#else
static const u8 ak5572_reg[AK5572_MAX_REGISTERS] = {
	0x03,	/*	0x00	AK5572_00_POWER_MANAGEMENT1  */
	0x01,	/*	0x01	AK5572_01_POWER_MANAGEMENT2  */
	0x01,	/*	0x02	AK5572_02_CONTROL1           */
	0x00,	/*	0x03	AK5572_03_CONTROL2		     */
	0x00,	/*	0x04	AK5572_04_CONTROL3	         */
	0x00,	/*	0x05	AK5572_05_DSD			 	*/
};
#endif

static const struct {
	int readable;   /* Mask of readable bits */
	int writable;   /* Mask of writable bits */
} ak5572_access_masks[] = {
    { 0xFF, 0x03 },	//0x00
    { 0xFF, 0x03 },	//0x01
    { 0xFF, 0x7F },	//0x02
    { 0xFF, 0x60 },	//0x03
    { 0xFF, 0x83 },	//0x04
    { 0xFF, 0x2F },	//0x05
};

static const char *mono_texts[] = {
	"Stereo", "Mono", 
};

static const char *tdm_texts[] = {
	"Off", "TDM128",  "TDM256", "TDM512",
};

static const char *digfil_texts[] = {
	"Sharp Roll-Off", "Show Roll-Off", 
	"Short Delay Sharp Roll-Off", "Short Delay Show Roll-Off",
};

static const struct soc_enum ak4605_adcset_enum[] = {
	SOC_ENUM_SINGLE(AK5572_01_POWER_MANAGEMENT2, 1, ARRAY_SIZE(mono_texts), mono_texts),
	SOC_ENUM_SINGLE(AK5572_03_CONTROL2, 5, ARRAY_SIZE(tdm_texts), tdm_texts),
	SOC_ENUM_SINGLE(AK5572_04_CONTROL3, 0, ARRAY_SIZE(digfil_texts), digfil_texts),
};

static const char *dsdon_texts[] = {
	"PCM", "DSD", 
};

static const char *dsdsel_texts[] = {
	"64fs", "128fs", "256fs" 
};

static const char *dckb_texts[] = {
	"Falling", "Rising", 
};

static const char *dcks_texts[] = {
	"512fs", "768fs", 
};

static const struct soc_enum ak4605_dsdset_enum[] = {
	SOC_ENUM_SINGLE(AK5572_04_CONTROL3, 7, ARRAY_SIZE(dsdon_texts), dsdon_texts),
	SOC_ENUM_SINGLE(AK5572_05_DSD, 0, ARRAY_SIZE(dsdsel_texts), dsdsel_texts),
	SOC_ENUM_SINGLE(AK5572_05_DSD, 2, ARRAY_SIZE(dckb_texts), dckb_texts),
	SOC_ENUM_SINGLE(AK5572_05_DSD, 5, ARRAY_SIZE(dcks_texts), dcks_texts),
};

static const struct snd_kcontrol_new ak5572_snd_controls[] = {

	SOC_ENUM("Monaural Mode", ak4605_adcset_enum[0]),
	SOC_ENUM("TDM mode", ak4605_adcset_enum[1]),
	SOC_ENUM("Digital Filter", ak4605_adcset_enum[2]),

	SOC_ENUM("DSD Mode", ak4605_dsdset_enum[0]),
	SOC_ENUM("Frequency of DCLK", ak4605_dsdset_enum[1]),
	SOC_ENUM("Polarity of DCLK", ak4605_dsdset_enum[2]),
	SOC_ENUM("Master Clock Frequency at DSD Mode", ak4605_dsdset_enum[3]),

	SOC_SINGLE("DSD Phase Modulation", AK5572_05_DSD, 3, 1, 0),

};

static const struct snd_soc_dapm_widget ak5572_dapm_widgets[] = {

// Analog Input
	SND_SOC_DAPM_INPUT("AIN1"),
	SND_SOC_DAPM_INPUT("AIN2"),

	SND_SOC_DAPM_ADC("ADC Ch1", "Capture", AK5572_00_POWER_MANAGEMENT1, 0, 0),
	SND_SOC_DAPM_ADC("ADC Ch2", "Capture", AK5572_00_POWER_MANAGEMENT1, 1, 0),
};

static const struct snd_soc_dapm_route ak5572_intercon[] = {

	{"ADC Ch1", "NULL", "AIN1"},
	{"ADC Ch2", "NULL", "AIN2"},

};


static int ak5572_set_mcki(struct snd_soc_codec *codec, int fs, int rclk)
{
	u8  mode;
	int mcki_rate;

	akdbgprt("\t[AK5572] %s fs=%d rclk=%d\n",__FUNCTION__, fs, rclk);

	if ((fs != 0)&&(rclk != 0)) {
        if ( (rclk % fs)) return(-EINVAL);
		mcki_rate = rclk/fs;

		mode = snd_soc_read(codec, AK5572_02_CONTROL1);
		mode &= ~AK5572_CKS;

		if ( fs > 400000 ) {
			switch(mcki_rate) {
				case 32:
				case 48:
					mode |= AK5572_CKS_AUTO;
					break;
				case 64:
					mode |= AK5572_CKS_64FS_768KHZ;
					break;
				default:
					return(-EINVAL);
			}
		}
		else if ( fs > 200000 ) {
			switch(mcki_rate) {
				case 64:
				case 96:
					mode |= AK5572_CKS_AUTO;
					break;
				default:
					return(-EINVAL);
			}
		}
		else if ( fs > 108000 ) {
			switch(mcki_rate) {
				case 128:
				case 192:
					mode |= AK5572_CKS_AUTO;
					break;
				default:
					return(-EINVAL);
			}
		}
		else if ( fs > 54000 ) {
			switch(mcki_rate) {
				case 256:
				case 384:
					mode |= AK5572_CKS_AUTO;
					break;
				default:
					return(-EINVAL);
			}
		}
		else  {
			switch(mcki_rate) {
				case 256:
					mode |= AK5572_CKS_256FS_48KHZ;
					break;
				case 384:
					mode |= AK5572_CKS_384FS_48KHZ;
					break;
				case 1024:
					if ( fs > 32000 ) return(-EINVAL);
				case 512:
				case 768:
					mode |= AK5572_CKS_AUTO;
					break;
				default:
					return(-EINVAL);
			}
		}

		snd_soc_update_bits(codec, AK5572_02_CONTROL1, AK5572_CKS,  mode);
	}
	else {
		snd_soc_update_bits(codec, AK5572_02_CONTROL1, AK5572_CKS, AK5572_CKS_AUTO);
	}

	return(0);

}

static int ak5572_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct ak5572_priv *ak5572 = snd_soc_codec_get_drvdata(codec);
	u8 bits;

	akdbgprt("\t[AK5572] %s(%d)\n",__FUNCTION__,__LINE__);

	/* set master/slave audio interface */
	bits = snd_soc_read(codec, AK5572_02_CONTROL1);
	bits &= ~AK5572_BITS;

	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
		case SNDRV_PCM_FORMAT_S24_LE:
			bits |= AK5572_DIF_24BIT_MODE;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			bits |= AK5572_DIF_32BIT_MODE;
			break;
		default:
			return -EINVAL;
	}

	ak5572->fs = params_rate(params);
	snd_soc_update_bits(codec, AK5572_02_CONTROL1, AK5572_BITS, bits);

	ak5572_set_mcki(codec, ak5572->fs, ak5572->rclk);

	return 0;

}

static int ak5572_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
		unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct ak5572_priv *ak5572 = snd_soc_codec_get_drvdata(codec);

	akdbgprt("\t[AK5572] %s(%d)\n",__FUNCTION__,__LINE__);

	ak5572->rclk = freq;
	ak5572_set_mcki(codec, ak5572->fs, ak5572->rclk);

	return 0;
}

static int ak5572_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{

	struct snd_soc_codec *codec = dai->codec;
	u8 format;
	int ret;

	akdbgprt("\t[AK5572] %s(%d)\n",__FUNCTION__,__LINE__);

	/* set master/slave audio interface */
	format = snd_soc_read(codec, AK5572_02_CONTROL1);
	format &= ~AK5572_DIF;

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_I2S:
			format |= AK5572_DIF_I2S_MODE;
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			format |= AK5572_DIF_MSB_MODE;
			break;
		default:
			return -EINVAL;
	}

	ret = snd_soc_update_bits(codec, AK5572_02_CONTROL1, AK5572_DIF, format);

	return ret;
}


static int ak5572_set_dai_mute(struct snd_soc_dai *dai, int mute)
{
    struct snd_soc_codec *codec = dai->codec;
	struct ak5572_priv *ak5572 = snd_soc_codec_get_drvdata(codec);
	int ndt;
		

	if (mute) {
		ndt = 0;
		if ( ak5572->fs != 0 ) ndt = 583000 / ak5572->fs;
		if ( ndt < 5 ) ndt = 5;
		msleep(ndt);
	}

	return 0;

}

static int ak5572_volatile(struct snd_soc_codec *codec, unsigned int reg)
{
	int	ret;

#ifdef AK5572_DEBUG
	ret = 1;
#else
	switch (reg) {
//		case :
//			ret = 1;
		default:
			ret = 0;
			break;
	}
#endif
	return(ret);
}

static int ak5572_readable(struct snd_soc_codec *codec, unsigned int reg)
{

	if (reg >= ARRAY_SIZE(ak5572_access_masks))
		return 0;
	return ak5572_access_masks[reg].readable != 0;
}

/*
* Read ak5572 register cache
 */
static inline u32 ak5572_read_reg_cache(struct snd_soc_codec *codec, u16 reg)
{
    u8 *cache = codec->reg_cache;
    BUG_ON(reg > ARRAY_SIZE(ak5572_reg));
    return (u32)cache[reg];
}

static int ak5572_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level)
{

	akdbgprt("\t[AK5572] %s bios level=%d\n",__FUNCTION__, (int)level);

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
	case SND_SOC_BIAS_STANDBY:
		break;
	case SND_SOC_BIAS_OFF:
		snd_soc_write(codec, AK5572_00_POWER_MANAGEMENT1, 0x00);
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

#define AK5572_RATES SNDRV_PCM_RATE_8000_192000

#define AK5572_FORMATS		SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE


static struct snd_soc_dai_ops ak5572_dai_ops = {
	.hw_params	= ak5572_hw_params,
	.set_sysclk	= ak5572_set_dai_sysclk,
	.set_fmt	= ak5572_set_dai_fmt,
	.digital_mute	= ak5572_set_dai_mute,

};

struct snd_soc_dai_driver ak5572_dai[] = {   
	{										 
		.name = "ak5572-aif1",
		.capture = {
		       .stream_name = "Capture",
		       .channels_min = 1,
		       .channels_max = 2,
		       .rates = AK5572_RATES,
		       .formats = AK5572_FORMATS,
		},
		.ops = &ak5572_dai_ops,
	},										 
};

static int ak5572_init_reg(struct snd_soc_codec *codec)
{
	int  ret;
#ifdef AK5572_PDN_GPIO
	struct ak5572_priv *ak5572 = snd_soc_codec_get_drvdata(codec);
#endif 

	akdbgprt("\t[AK5572] %s(%d)\n",__FUNCTION__,__LINE__);

	msleep(10);
#ifdef AK5572_PDN_GPIO
	gpio_set_value(ak5572->pdn_gpio, 0);
	mdelay(1);
	gpio_set_value(ak5572->pdn_gpio, 1);	
	mdelay(1);
#endif

	ret = snd_soc_write(codec, AK5572_00_POWER_MANAGEMENT1, 0x0);
	if ( ret < 0 ) return(ret);

	ret = snd_soc_update_bits(codec, AK5572_02_CONTROL1, AK5572_CKS, AK5572_CKS_AUTO);
	if ( ret < 0 ) return(ret);

	ak5572_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return(0);

}

#ifdef CONFIG_OF
static int ak5572_i2c_parse_dt(struct i2c_client *i2c, struct ak5572_priv *ak5572)
{
	struct device *dev = &i2c->dev;
	struct device_node *np = dev->of_node;
	int ret;

	if (!np)
		return -1;

	ak5572->pdn_gpio = of_get_named_gpio(np, "ak5572,pdn-gpio", 0);
	if (ak5572->pdn_gpio < 0) {
		ak5572->pdn_gpio = -1;
		return -1;
	}

	if( !gpio_is_valid(ak5572->pdn_gpio) ) {
		printk(KERN_ERR "ak5572 pdn pin(%u) is invalid\n", ak5572->pdn_gpio);
		return -1;
	}

	
	ret = gpio_request(ak5572->pdn_gpio, "PDNA");
	if (ret) {
		printk("%s: failed to request reset gpio PDNA\n", __func__);
		return -EBUSY;
	}

	ret = gpio_direction_output(ak5572->pdn_gpio, 0);
	if (ret < 0) {
		printk("Unable to set PDNA Pin output\n");
		gpio_free(ak5572->pdn_gpio);
		return -ENODEV;
	}

	return 0;
}
#endif

static int ak5572_probe(struct snd_soc_codec *codec)
{
	struct ak5572_priv *ak5572 = snd_soc_codec_get_drvdata(codec);
	struct ak5572_platform_data *pdata = codec->dev->platform_data;	
	int ret = 0;

	akdbgprt("\t[AK5572] %s(%d)\n",__FUNCTION__,__LINE__);

	ret = snd_soc_codec_set_cache_io(codec, 8, 8, SND_SOC_I2C);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}

#ifdef AK5572_PDN_GPIO
	ret = -1;
#ifdef CONFIG_OF
	if (ak5572->i2c->dev.of_node) {
		printk("Read PDN pin from device tree\n");
		ret = ak5572_i2c_parse_dt(ak5572->i2c, ak5572);
	}
#endif
	if ( ret < 0 ) {
		ak5572->pdn_gpio = pdata->pdn_gpio;
		ret = gpio_request(ak5572->pdn_gpio, "ak5572 pdn");
		gpio_direction_output(ak5572->pdn_gpio, 0);
	}
#endif

	ret = ak5572_init_reg(codec);

	ak5572->fs = 48000;
	ak5572->rclk = 0;

    return ret;

}

static int ak5572_remove(struct snd_soc_codec *codec)
{
#ifdef AK5572_PDN_GPIO
	struct ak5572_priv *ak5572 = snd_soc_codec_get_drvdata(codec);
#endif 

	ak5572_set_bias_level(codec, SND_SOC_BIAS_OFF);

#ifdef AK5572_PDN_GPIO
	gpio_set_value(ak5572->pdn_gpio, 0);
	mdelay(1);
	gpio_free(ak5572->pdn_gpio);
	mdelay(1);
#endif

	return 0;
}

#ifdef AK5572_VERSIONUP
static int ak5572_suspend(struct snd_soc_codec *codec)
#else
static int ak5572_suspend(struct snd_soc_codec *codec, pm_message_t state)
#endif
{
#ifdef AK5572_PDN_GPIO
	struct ak5572_priv *ak5572 = snd_soc_codec_get_drvdata(codec);
#endif 
	ak5572_set_bias_level(codec, SND_SOC_BIAS_OFF);

#ifdef AK5572_PDN_GPIO
	gpio_set_value(ak5572->pdn_gpio, 0);
	mdelay(1);
	snd_soc_cache_init(codec);
#else
#ifdef AK5572_PD_SUSPEND
	snd_soc_cache_init(codec);
#endif
#endif
	return 0;
}

static int ak5572_resume(struct snd_soc_codec *codec)
{

	ak5572_init_reg(codec);

	return 0;
}


struct snd_soc_codec_driver soc_codec_dev_ak5572 = {
	.probe = ak5572_probe,
	.remove = ak5572_remove,
	.suspend =	ak5572_suspend,
	.resume =	ak5572_resume,

#ifdef AK5572_VERSIONUP
	.idle_bias_off = true,
#endif

	.set_bias_level = ak5572_set_bias_level,
	.reg_cache_size = ARRAY_SIZE(ak5572_reg),
	.reg_word_size = sizeof(u8),
	.reg_cache_default = ak5572_reg,
	.readable_register = ak5572_readable,
	.volatile_register = ak5572_volatile,	

	.controls = ak5572_snd_controls,
	.num_controls = ARRAY_SIZE(ak5572_snd_controls),
	.dapm_widgets = ak5572_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(ak5572_dapm_widgets),
	.dapm_routes = ak5572_intercon,
	.num_dapm_routes = ARRAY_SIZE(ak5572_intercon),
};
EXPORT_SYMBOL_GPL(soc_codec_dev_ak5572);

//J151030+
static struct i2c_client * g_i2c_client_AK5572=0;

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


int AK5572_WRITE(int ch,unsigned int reg, unsigned int val)
{
                if(!g_i2c_client_AK5572) {
                        akdbgprt("AK5572 %s ERROR \n", __func__);
                        return -1;
                }
                return write_reg(g_i2c_client_AK5572,reg,val);
}

int AK5572_READ(int ch,unsigned char reg)
{
                if(!g_i2c_client_AK5572) {
                        akdbgprt("AK5572 %s ERROR \n", __func__);

                        return -1;
                }
                return read_reg(g_i2c_client_AK5572, reg);
}


int init_ak5572_reg(void)
{
	int i=0;
#if 0 //disp reg before set
	for(i=0; i<6; i++) {
		printk("--%s:%d:before:reg[%d]=%d\n", __func__, __LINE__, i, AK5572_READ( 0, i));
	}
#endif

	for(i=0; i<6; i++) {
		AK5572_WRITE(0, i, ak5572_reg[i]);
	}

#if 1 //disp reg after set
	for(i=0; i<6; i++) {
		printk("--%s:%d:after:reg[%d]=%d\n", __func__, __LINE__, i, AK5572_READ( 0, i));
	}
#endif


	 
	return 0;
}
//J151026
int init_AK5572(void)
{
	akdbgprt("\t[AK5572] %s(%d)\n",__FUNCTION__,__LINE__);
	//1. ADC_PWR_EN
	//2. ADC_RST
	//5. PCM/DSD_CLOCK_ON
#if 0
	gpio_request(EXT_GPIO_P14,"EXT_GPIO14_ADC_POWER");
	gpio_direction_output(EXT_GPIO_P14, 1);
	msleep(10);
#endif

	gpio_request(EXT_GPIO_P04,"EXT_GPIO04_PCM_DSD_EN");
	gpio_direction_output(EXT_GPIO_P04, 0); //LOW ACTIVE
	msleep(10);

	gpio_request(EXT_GPIO_P05,"EXT_GPIO05_DSD_DATA_EN");
	gpio_direction_output(EXT_GPIO_P05, 1); //LOW ACTIVE, HIGH->PCM_DATA_EN
	msleep(10);

	gpio_request(EXT_GPIO_P00,"EXT_GPIO00_ADC_RST");
	gpio_direction_output(EXT_GPIO_P00, 0);
	msleep(10);

	gpio_request(EXT_GPIO_P00,"EXT_GPIO00_ADC_RST");
	gpio_direction_output(EXT_GPIO_P00, 1);
	msleep(10);

	init_ak5572_reg();
	return 0;
}
EXPORT_SYMBOL(init_AK5572);


//J151030-

static int ak5572_i2c_probe(struct i2c_client *i2c,
                            const struct i2c_device_id *id)
{
	struct ak5572_priv *ak5572;
	int ret=0;

	akdbgprt("\t[AK5572] %s(%d)\n",__FUNCTION__,__LINE__);

	ak5572 = kzalloc(sizeof(struct ak5572_priv), GFP_KERNEL);
	if (ak5572 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, ak5572);

	ak5572->i2c = i2c;

	//J151030
	g_i2c_client_AK5572 = i2c;

	ret = snd_soc_register_codec(&i2c->dev,
			&soc_codec_dev_ak5572, &ak5572_dai[0], ARRAY_SIZE(ak5572_dai));
	if (ret < 0){
		kfree(ak5572);
		akdbgprt("\t[AK5572 Error!] %s(%d)\n",__FUNCTION__,__LINE__);
	}
	return ret;
}

static int __devexit ak5572_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	kfree(i2c_get_clientdata(client));
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id ak5572_i2c_dt_ids[] = {
    { .compatible = "akm,ak5572"},
    { }
};
#endif


static const struct i2c_device_id ak5572_i2c_id[] = {
	{ "ak5572", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ak5572_i2c_id);

static struct i2c_driver ak5572_i2c_driver = {
	.driver = {
		.name = "ak5572",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(ak5572_i2c_dt_ids),
#endif
	},
	.probe = ak5572_i2c_probe,
	.remove = __devexit_p(ak5572_i2c_remove),
	.id_table = ak5572_i2c_id,
};

static int __init ak5572_modinit(void)
{

	akdbgprt("\t[AK5572] %s(%d)\n", __FUNCTION__,__LINE__);

	return i2c_add_driver(&ak5572_i2c_driver);
}

module_init(ak5572_modinit);

static void __exit ak5572_exit(void)
{
	i2c_del_driver(&ak5572_i2c_driver);
}
module_exit(ak5572_exit);


MODULE_AUTHOR("Junichi Wakasugi <wakasugi.jb@om.asahi-kasei.co.jp>");
MODULE_DESCRIPTION("ASoC ak5572 codec driver");
MODULE_LICENSE("GPL");
