/*
 * rotary_encoder.c
 *
 * (c) 2009 Daniel Mack <daniel@caiaq.de>
 * Copyright (C) 2011 Johan Hovold <jhovold@gmail.com>
 *
 * Modified by downer.kim <downer.kim@iriver.com>
 *
 * state machine code inspired by code from Tim Ruetz
 *
 * A generic driver for rotary encoders connected to GPIO lines.
 * See file:Documentation/input/rotary_encoder.txt for more information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/rotary_encoder.h>
#include <linux/slab.h>

#include <linux/irq.h>
#include <linux/delay.h>
#define DRV_NAME "gpio-encoder"
#define MAX_ENCODER_BUF     6
#define MAX_ENCODER_KEYCODE 2

#define EVENT_TIMER_INTERVAL  (jiffies + 5) /* [downer] A150106 */

static int g_irq_a, g_irq_b;

enum {
    _INPUT_LOW,
    _INPUT_HIGH
};

int encoder_keycodes[MAX_ENCODER_KEYCODE] ={
    KEY_VOLUMEUP,
    KEY_VOLUMEDOWN
};

struct rotary_encoder {
	struct input_dev *input;
	struct rotary_encoder_platform_data *pdata;

    struct timer_list event_timer; /* [downer] A150105 for event push */
    
	unsigned int axis;
	unsigned int pos;

	unsigned int irq_a;
	unsigned int irq_b;

	bool armed;
	unsigned char dir;	/* 0 - clockwise, 1 - CCW */

	char last_stable;
};

typedef struct {
    unsigned char head;
    unsigned char tail;
    char buf[MAX_ENCODER_BUF];
} _encoderQueueStruct;

_encoderQueueStruct encoderKeyBuf;
int encoder_port_a = _INPUT_HIGH;
int encoder_port_b = _INPUT_HIGH;

static int g_wheel_lock_irq_state = 0; /* [downer] A151208 */

void ak_wheel_lock_mode(int mode)
{
    if (mode) {
		if (!g_wheel_lock_irq_state) {		
			disable_irq(g_irq_a);
			disable_irq(g_irq_b);

			g_wheel_lock_irq_state = 1;
		}
    }
    else {
		if (g_wheel_lock_irq_state) {
			enable_irq(g_irq_a);
			enable_irq(g_irq_b);

			g_wheel_lock_irq_state = 0;
		}
    }
}
EXPORT_SYMBOL(ak_wheel_lock_mode);

unsigned char encoder_getKey(void)
{
    unsigned char data = 0xff;

    if (encoderKeyBuf.head != encoderKeyBuf.tail) {
        data = encoderKeyBuf.buf[encoderKeyBuf.tail];
        encoderKeyBuf.buf[encoderKeyBuf.tail++] = 0;

        encoderKeyBuf.tail %= MAX_ENCODER_BUF;
    }

    return data;
}

void encoder_data_push(unsigned char key)
{
    encoderKeyBuf.buf[encoderKeyBuf.head++] = key;
    encoderKeyBuf.head %= MAX_ENCODER_BUF;
}

//static int rotary_encoder_get_state(struct rotary_encoder_platform_data *pdata)
static int rotary_encoder_get_state(struct rotary_encoder *encoder)
{
	int a = !!gpio_get_value(encoder->pdata->gpio_a);
	int b = !!gpio_get_value(encoder->pdata->gpio_b);

	a ^= encoder->pdata->inverted_a;
	b ^= encoder->pdata->inverted_b;

#if 0
    if (a == _INPUT_LOW && b == _INPUT_LOW) {
        if ((encoder_port_a == _INPUT_HIGH) && (encoder_port_b == _INPUT_LOW)) {
            //printk("UP(b)\n");
            encoder->dir = KEY_VOLUMEUP;
        }
        else if ((encoder_port_a == _INPUT_LOW) && (encoder_port_b == _INPUT_HIGH)) {
            //printk("DOWN(b)\n");
            encoder->dir = KEY_VOLUMEDOWN;            
        }
    }
#endif
    
    encoder_port_a = a;
    encoder_port_b = b;
    
    return ((a << 1) | b);
}
extern void cs4398_volume_ramp_reset(void);

static void rotary_encoder_report_event(struct rotary_encoder *encoder)
{
	struct rotary_encoder_platform_data *pdata = encoder->pdata;
    unsigned int code = 0xff;
    
	if (pdata->relative_axis) {
		input_report_rel(encoder->input,
				 pdata->axis, encoder->dir ? -1 : 1);
	} 
    else {
        code = encoder_getKey();

        if (code == KEY_VOLUMEUP || code == KEY_VOLUMEDOWN) {
            input_report_key(encoder->input, code, 1);
            input_report_key(encoder->input, code, 0);
            input_sync(encoder->input);
        }
    }
}
static void rotary_encoder_report_event2(struct rotary_encoder *encoder)
{
	struct rotary_encoder_platform_data *pdata = encoder->pdata;
    unsigned int code = 0xff;
	if (pdata->relative_axis) {
		input_report_rel(encoder->input,
				 pdata->axis, encoder->dir ? -1 : 1);
	} 
    else {
		unsigned int pos = encoder->pos;
		if (encoder->dir) {
			if (pdata->rollover)
				pos += pdata->steps;
			if (pos)
				pos--;
#if (CUR_AK == MODEL_AK300)
			input_report_key(encoder->input, KEY_VOLUMEDOWN, 1); 
			input_report_key(encoder->input, KEY_VOLUMEDOWN, 0);
			
#else
			input_report_key(encoder->input, KEY_VOLUMEUP, 1); 
			input_report_key(encoder->input, KEY_VOLUMEUP, 0);
			#endif
		} else {
			if (pdata->rollover || pos < pdata->steps)
				pos++;
#if (CUR_AK == MODEL_AK300)
			input_report_key(encoder->input, KEY_VOLUMEUP, 1); 
			input_report_key(encoder->input, KEY_VOLUMEUP, 0);
#else
			input_report_key(encoder->input, KEY_VOLUMEDOWN, 1); 
			input_report_key(encoder->input, KEY_VOLUMEDOWN, 0);
#endif
        }
		if (pdata->rollover)
			pos %= pdata->steps;
		encoder->pos = pos;
		input_report_abs(encoder->input, pdata->axis, encoder->pos);
    }
	
	#ifdef USE_VWR_TYPE1
	cs4398_volume_ramp_reset();
	#endif	
	input_sync(encoder->input);		
}

static void event_report_work(unsigned long data)    
{
    struct rotary_encoder *encoder = (struct rotary_encoder *)data;
    
    rotary_encoder_report_event(encoder);

    mod_timer(&encoder->event_timer, EVENT_TIMER_INTERVAL);
}

static irqreturn_t rotary_encoder_irq(int irq, void *dev_id)
{
	struct rotary_encoder *encoder = dev_id;
	int state;

	state = rotary_encoder_get_state(encoder);

	switch (state) {
	case 0x0:
		if (encoder->armed) {
			rotary_encoder_report_event2(encoder);
			encoder->armed = false;
		}
		break;

	case 0x1:
	case 0x2:
		if (encoder->armed)
			encoder->dir = state - 1;
		break;

	case 0x3:
		encoder->armed = true;
		break;
	}

	return IRQ_HANDLED;
}

spinlock_t encoder_lock;
static irqreturn_t rotary_encoder_half_period_irq(int irq, void *dev_id)
{
	struct rotary_encoder *encoder = dev_id;
	int state;
    
    spin_lock(&encoder_lock);

	state = rotary_encoder_get_state(encoder);

	switch (state) {
	case 0x00:
		if (state != encoder->last_stable) {
			if (encoder->dir == KEY_VOLUMEUP) {		
				//printk("UP\n");
#if (CUR_AK == MODEL_AK300)
				encoder_data_push(KEY_VOLUMEDOWN);
#else
                encoder_data_push(KEY_VOLUMEUP);
#endif
			}
			else if (encoder->dir == KEY_VOLUMEDOWN) {		
				//printk("DOWN\n");
#if (CUR_AK == MODEL_AK300)
				encoder_data_push(KEY_VOLUMEUP);
#else
                encoder_data_push(KEY_VOLUMEDOWN);
#endif
			}
            encoder->last_stable = state;
            encoder->dir = 0xff;
		}
		break;
	case 0x03:
        encoder->last_stable = state;                
        break;
	case 0x01:
        encoder->dir = KEY_VOLUMEDOWN;
        encoder->last_stable = state;
        break;
	case 0x02:
        encoder->dir = KEY_VOLUMEUP;
        encoder->last_stable = state;        
 		break;
	}
    
    spin_unlock(&encoder_lock);
    
	return IRQ_HANDLED;
}

static int __devinit rotary_encoder_probe(struct platform_device *pdev)
{
	struct rotary_encoder_platform_data *pdata = pdev->dev.platform_data;
	struct rotary_encoder *encoder;
	struct input_dev *input;
	irq_handler_t handler;
	int err, i;

	if (!pdata) {
		dev_err(&pdev->dev, "missing platform data\n");
		return -ENOENT;
	}

	encoder = kzalloc(sizeof(struct rotary_encoder), GFP_KERNEL);
	input = input_allocate_device();
	if (!encoder || !input) {
		dev_err(&pdev->dev, "failed to allocate memory for device\n");
		err = -ENOMEM;
		goto exit_free_mem;
	}

	encoder->input = input;
	encoder->pdata = pdata;
	encoder->irq_a = gpio_to_irq(pdata->gpio_a);
	encoder->irq_b = gpio_to_irq(pdata->gpio_b);

    /* [downer] A131017 */
    g_irq_a = encoder->irq_a;
    g_irq_b = encoder->irq_b;
    
	/* create and register the input driver */
	input->name = pdev->name;
	input->id.bustype = BUS_HOST;
	input->dev.parent = &pdev->dev;
    input->keycode = encoder_keycodes;
    
	if (pdata->relative_axis) {
		input->evbit[0] = BIT_MASK(EV_REL);
		input->relbit[0] = BIT_MASK(pdata->axis);
	} 
    else {
        set_bit(EV_SYN, input->evbit);
        set_bit(EV_KEY, input->evbit);

        for (i = 0; i < MAX_ENCODER_KEYCODE; i++)
            input_set_capability(input, EV_KEY, ((int*)input->keycode)[i]);        
	}

    /* [downer] A150106 for event timer */
    init_timer(&encoder->event_timer);
    encoder->event_timer.function = event_report_work;
    encoder->event_timer.data = (unsigned long)encoder;
    encoder->event_timer.expires = jiffies + EVENT_TIMER_INTERVAL;

    add_timer(&encoder->event_timer);
    
	err = input_register_device(input);
	if (err) {
		dev_err(&pdev->dev, "failed to register input device\n");
		goto exit_free_mem;
	}

	/* request the GPIOs */
	err = gpio_request(pdata->gpio_a, DRV_NAME);
	if (err) {
		dev_err(&pdev->dev, "unable to request GPIO %d\n",
			pdata->gpio_a);
		goto exit_unregister_input;
	}

	err = gpio_direction_input(pdata->gpio_a);
	if (err) {
		dev_err(&pdev->dev, "unable to set GPIO %d for input\n",
			pdata->gpio_a);
		goto exit_unregister_input;
	}

	err = gpio_request(pdata->gpio_b, DRV_NAME);
	if (err) {
		dev_err(&pdev->dev, "unable to request GPIO %d\n",
			pdata->gpio_b);
		goto exit_free_gpio_a;
	}

	err = gpio_direction_input(pdata->gpio_b);
	if (err) {
		dev_err(&pdev->dev, "unable to set GPIO %d for input\n",
			pdata->gpio_b);
		goto exit_free_gpio_a;
	}

	/* request the IRQs */
	if (0) { //pdata->half_period) {
		handler = &rotary_encoder_half_period_irq;
		encoder->last_stable = rotary_encoder_get_state(encoder);
	} else {
		handler = &rotary_encoder_irq;
	}

	tcc_gpio_config_ext_intr(INT_EINT1, EXTINT_GPIOE_17);
	err = request_irq(encoder->irq_a, handler,
			  IRQF_DISABLED | IRQF_TRIGGER_FALLING,
			  DRV_NAME, encoder);
	if (err) {
		dev_err(&pdev->dev, "unable to request IRQ %d\n",
			encoder->irq_a);
		goto exit_free_gpio_b;
	}

	tcc_gpio_config_ext_intr(INT_EINT3, EXTINT_GPIOE_19);
	err = request_irq(encoder->irq_b, handler,
            IRQF_DISABLED | IRQF_TRIGGER_FALLING,
			  DRV_NAME, encoder);
	if (err) {
		dev_err(&pdev->dev, "unable to request IRQ %d\n",
			encoder->irq_b);
		goto exit_free_irq_a;
	}

	platform_set_drvdata(pdev, encoder);

	return 0;

exit_free_irq_a:
	free_irq(encoder->irq_a, encoder);
exit_free_gpio_b:
	gpio_free(pdata->gpio_b);
exit_free_gpio_a:
	gpio_free(pdata->gpio_a);
exit_unregister_input:
	input_unregister_device(input);
	input = NULL; /* so we don't try to free it */
exit_free_mem:
	input_free_device(input);
	kfree(encoder);
    
	return err;
}

static int __devexit rotary_encoder_remove(struct platform_device *pdev)
{
	struct rotary_encoder *encoder = platform_get_drvdata(pdev);
	struct rotary_encoder_platform_data *pdata = pdev->dev.platform_data;

    del_timer(&encoder->event_timer); /* [downer] A150106 */
    
	free_irq(encoder->irq_a, encoder);
	free_irq(encoder->irq_b, encoder);
    
	gpio_free(pdata->gpio_a);
	gpio_free(pdata->gpio_b);
    
	input_unregister_device(encoder->input);
	platform_set_drvdata(pdev, NULL);
    
	kfree(encoder);

	return 0;
}

static struct platform_driver rotary_encoder_driver = {
	.probe		= rotary_encoder_probe,
	.remove		= __devexit_p(rotary_encoder_remove),
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	}
};

static int __init rotary_encoder_init(void)
{
	return platform_driver_register(&rotary_encoder_driver);
}

static void __exit rotary_encoder_exit(void)
{
	platform_driver_unregister(&rotary_encoder_driver);
}

module_init(rotary_encoder_init);
module_exit(rotary_encoder_exit);

MODULE_ALIAS("platform:" DRV_NAME);
MODULE_DESCRIPTION("GPIO encoder driver");
MODULE_AUTHOR("Daniel Mack <daniel@caiaq.de>, Johan Hovold");
MODULE_LICENSE("GPL v2");
