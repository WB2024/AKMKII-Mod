/*
 * u_audio.c -- ALSA audio utilities for Gadget stack
 *
 * Copyright (C) 2008 Bryan Wu <cooloney@kernel.org>
 * Copyright (C) 2008 Analog Devices, Inc
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 * 
 * 2014.03, iriver.jimi.modified for ak2xx. 
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/random.h>
#include <linux/syscalls.h>

#include "u_audio.h"

/*
 * This component encapsulates the ALSA devices for USB audio gadget
 */

#define FILE_PCM_PLAYBACK	"/dev/snd/pcmC0D0p"
#define FILE_PCM_CAPTURE	"/dev/snd/pcmC0D0c"
#define FILE_CONTROL		"/dev/snd/controlC0"

static char *fn_play = FILE_PCM_PLAYBACK;
module_param(fn_play, charp, S_IRUGO);
MODULE_PARM_DESC(fn_play, "Playback PCM device file name");

static char *fn_cap = FILE_PCM_CAPTURE;
module_param(fn_cap, charp, S_IRUGO);
MODULE_PARM_DESC(fn_cap, "Capture PCM device file name");

static char *fn_cntl = FILE_CONTROL;
module_param(fn_cntl, charp, S_IRUGO);
MODULE_PARM_DESC(fn_cntl, "Control device file name");

static unsigned char * g_tmp_out;
static struct gaudio *the_card;

/*-------------------------------------------------------------------------*/

/**
 * Some ALSA internal helper functions
 */
static int snd_interval_refine_set(struct snd_interval *i, unsigned int val)
{
	struct snd_interval t;
	t.empty = 0;
	t.min = t.max = val;
	t.openmin = t.openmax = 0;
	t.integer = 1;
	return snd_interval_refine(i, &t);
}

static int _snd_pcm_hw_param_set(struct snd_pcm_hw_params *params,
				 snd_pcm_hw_param_t var, unsigned int val,
				 int dir)
{
	int changed;
	if (hw_is_mask(var)) {
		struct snd_mask *m = hw_param_mask(params, var);
		if (val == 0 && dir < 0) {
			changed = -EINVAL;
			snd_mask_none(m);
		} else {
			if (dir > 0)
				val++;
			else if (dir < 0)
				val--;
			changed = snd_mask_refine_set(
					hw_param_mask(params, var), val);
		}
	} else if (hw_is_interval(var)) {
		struct snd_interval *i = hw_param_interval(params, var);
		if (val == 0 && dir < 0) {
			changed = -EINVAL;
			snd_interval_none(i);
		} else if (dir == 0)
			changed = snd_interval_refine_set(i, val);
		else {
			struct snd_interval t;
			t.openmin = 1;
			t.openmax = 1;
			t.empty = 0;
			t.integer = 0;
			if (dir < 0) {
				t.min = val - 1;
				t.max = val;
			} else {
				t.min = val;
				t.max = val+1;
			}
			changed = snd_interval_refine(i, &t);
		}
	} else
		return -EINVAL;
	if (changed) {
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	return changed;
}
/*-------------------------------------------------------------------------*/

/**
 * Set default hardware params
 */
static int playback_default_hw_params(struct gaudio_snd_dev *snd)
{
	struct snd_pcm_substream *substream = snd->substream;
	struct snd_pcm_hw_params *params;
	snd_pcm_sframes_t result;

	snd->access = SNDRV_PCM_ACCESS_RW_INTERLEAVED;
	snd->channels = 2;

	#ifdef USB_DAC_FORCE_16BIT_TO_24BIT
	snd->convert_16bit_to_24bit = 0;
	#endif

	printk("%s -- Format: %d, Rate: %d\n", __func__, snd->format, snd->rate);
	params = kzalloc(sizeof(*params), GFP_KERNEL);
	if (!params)
		return -ENOMEM;

	_snd_pcm_hw_params_any(params);

	if( snd->format == SNDRV_PCM_FORMAT_S16_LE )
	{
		_snd_pcm_hw_param_set(params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, 16384, 0);
		_snd_pcm_hw_param_set(params, SNDRV_PCM_HW_PARAM_BUFFER_SIZE, 32768, 0);
	}
	else if( snd->format == SNDRV_PCM_FORMAT_S24_LE )
	{
//		_snd_pcm_hw_param_set(params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, 64 * 0x20, 0);
//		_snd_pcm_hw_param_set(params, SNDRV_PCM_HW_PARAM_BUFFER_SIZE, 64 * 0x100, 0);
		_snd_pcm_hw_param_set(params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, 2048, 0);
		_snd_pcm_hw_param_set(params, SNDRV_PCM_HW_PARAM_BUFFER_SIZE, 4096, 0);
	}

	_snd_pcm_hw_param_set(params, SNDRV_PCM_HW_PARAM_ACCESS,
			snd->access, 0);

	#ifdef USB_DAC_FORCE_16BIT_TO_24BIT
	if( snd->format == SNDRV_PCM_FORMAT_S16_LE ) {
		snd->format = SNDRV_PCM_FORMAT_S24_LE;
		snd->convert_16bit_to_24bit = 1;
	} 
	#endif
	
	_snd_pcm_hw_param_set(params, SNDRV_PCM_HW_PARAM_FORMAT,
			snd->format, 0);
	_snd_pcm_hw_param_set(params, SNDRV_PCM_HW_PARAM_CHANNELS,
			snd->channels, 0);
	_snd_pcm_hw_param_set(params, SNDRV_PCM_HW_PARAM_RATE,
			snd->rate, 0);

	snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_DROP, NULL);
	snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_HW_PARAMS, params);

	result = snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_PREPARE, NULL);
	if (result < 0) {
		ERROR(snd->card,
			"Preparing sound card failed: %d\n", (int)result);
		kfree(params);
		return result;
	}

	/* Store the hardware parameters */
	snd->access = params_access(params);
	snd->format = params_format(params);
	snd->channels = params_channels(params);
	snd->rate = params_rate(params);

	kfree(params);

	INFO(snd->card,
		"Hardware params: access %x, format %x, channels %d, rate %d\n",
		snd->access, snd->format, snd->channels, snd->rate);

	return 0;
}

/**
 * Playback audio buffer data by ALSA PCM device
 */
static size_t u_audio_playback(struct gaudio *card, void *buf, size_t count)
{
	int i,j;
	unsigned char *src_data= (unsigned char*)buf;
	struct gaudio_snd_dev	*snd = &card->playback;
	struct snd_pcm_substream *substream = snd->substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	mm_segment_t old_fs;
	ssize_t result;
	snd_pcm_sframes_t frames;

try_again:
	if(prereq_enable || !snd) return 0; //0421

	if (runtime->status->state == SNDRV_PCM_STATE_XRUN ||
		runtime->status->state == SNDRV_PCM_STATE_SUSPENDED) {
		result = snd_pcm_kernel_ioctl(substream,
				SNDRV_PCM_IOCTL_PREPARE, NULL);

		if (result < 0) {
			ERROR(card, "Preparing sound card failed: %d\n",
					(int)result);
			return result;
		}
	}
	
	#ifdef USB_DAC_FORCE_16BIT_TO_24BIT
	
    if(snd->convert_16bit_to_24bit) {		
        unsigned int i, a;

        uint8_t *src_resample = (uint8_t*)src_data;
        int32_t *dst_resample = (int32_t*)g_tmp_out;
		memset(g_tmp_out, 0, 87381);//65536*4/3 
        
        for(i = 0, a = 0; i < count; i += 2, a++) {
            int32_t x = (int32_t) ((src_resample[i] & 0xff) | 
                           (int32_t) ((src_resample[i+1] & 0xff) << 8));
            x = (x << 16) >> 8; // sign extension
            dst_resample[a] = (int32_t)x;
        }
		count = count * 2;
    }
	else  
	#endif	

	{
		if(snd->format == SNDRV_PCM_FORMAT_S24_LE) {
			{
				j=0;
				memset(g_tmp_out, 0, 87381);//65536*4/3 

				for(i=0;i< count;i+=3) {
					g_tmp_out[j]     = src_data [i];
					g_tmp_out[j+1]   = src_data [i+1];
					g_tmp_out[j+2]   = src_data [i+2];
					g_tmp_out[j+3]   = 0;
					j+=4;
				}
				count = count * 4 / 3;
				//count -= (count%4);
			}
		}
	}

	
	frames = bytes_to_frames(runtime, count);
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	#ifdef USB_DAC_FORCE_16BIT_TO_24BIT
	if(snd->convert_16bit_to_24bit) 
		result = snd_pcm_lib_write(snd->substream, g_tmp_out, frames);
	#else
	if(snd->format != SNDRV_PCM_FORMAT_S24_LE) 
		result = snd_pcm_lib_write(snd->substream, buf, frames);
	#endif
	else 
		result = snd_pcm_lib_write(snd->substream, g_tmp_out, frames);

	if (result != frames && !prereq_enable ) {
		ERROR(card, "Playback error: %d\n", (int)result);
		set_fs(old_fs);
		goto try_again;
	}
	set_fs(old_fs);

	return 0;
}

static int u_audio_get_playback_channels(struct gaudio *card)
{
	return card->playback.channels;
}


static int u_audio_get_playback_rate(struct gaudio *card)
{
	return card->playback.rate;
}

/**
 * Open ALSA PCM and control device files
 * Initial the PCM or control device
 */
static int gaudio_open_snd_dev(struct gaudio *card)
{
	struct snd_pcm_file *pcm_file;
	struct gaudio_snd_dev *snd;

	if (!card)
		return -ENODEV;

	/* Open control device */
	snd = &card->control;
	snd->filp = filp_open(fn_cntl, O_RDWR, 0);
	if (IS_ERR(snd->filp)) {
		int ret = PTR_ERR(snd->filp);
		ERROR(card, "unable to open sound control device file: %s\n",
				fn_cntl);
		snd->filp = NULL;
		return ret;
	}
	snd->card = card;

	/* Open PCM playback device and setup substream */
	snd = &card->playback;
	snd->filp = filp_open(fn_play, O_WRONLY, 0);
	if (IS_ERR(snd->filp)) {
		ERROR(card, "No such PCM playback device: %s\n", fn_play);
		snd->filp = NULL;
	}
	pcm_file = snd->filp->private_data;
	snd->substream = pcm_file->substream;
	snd->card = card;
//0416.test+
	printk("format: %d, rate: %d\n", snd->format, snd->rate);
	snd->format = SNDRV_PCM_FORMAT_S16_LE;
#if 0 //(CUR_AK == MODEL_AK320)
	snd->rate = 96000; //J151007
#else
	snd->rate = 44100; //J160216 48000;
#endif
//0416.test-


	playback_default_hw_params(snd);

	g_tmp_out = kzalloc(87381, GFP_KERNEL); //65536*4/3
	if(!g_tmp_out)
		return -ENOMEM;

	return 0;
}

/**
 * Close ALSA PCM and control device files
 */
static int gaudio_close_snd_dev(struct gaudio *gau)
{
	struct gaudio_snd_dev	*snd;

	/* Close control device */
	snd = &gau->control;
	if (snd->filp)
		filp_close(snd->filp, current->files);

	/* Close PCM playback device and setup substream */
	snd = &gau->playback;
	if (snd->filp)
		filp_close(snd->filp, current->files);

	/* Release temporary buffer */
	if(g_tmp_out) 
		kfree(g_tmp_out);

	return 0;
}

/**
 * gaudio_setup - setup ALSA interface and preparing for USB transfer
 *
 * This sets up PCM, mixer or MIDI ALSA devices fore USB gadget using.
 *
 * Returns negative errno, or zero on success
 */
int __init gaudio_setup(struct gaudio *card)
{
	int	ret;

	ret = gaudio_open_snd_dev(card);
	if (ret)
		ERROR(card, "we need at least one control device\n");
	else if (!the_card)
		the_card = card;

	return ret;

}

/**
 * gaudio_cleanup - remove ALSA device interface
 *
 * This is called to free all resources allocated by @gaudio_setup().
 */
void gaudio_cleanup(void)
{
	if (the_card) {
		gaudio_close_snd_dev(the_card);
		the_card = NULL;
	}
}

