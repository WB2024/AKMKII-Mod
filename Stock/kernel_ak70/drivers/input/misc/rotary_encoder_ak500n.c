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

//PPIC
#include <asm/mach/map.h>
#include <mach/bsp.h>

//irq_set_irq_type
#include <linux/irq.h>

#include <mach/tca_tco.h>
#include <linux/delay.h>

#include <linux/earlysuspend.h>

#define DRV_NAME "gpio-encoder"
#define MAX_ENCODER_BUF     2
#define MAX_ENCODER_KEYCODE 2

#define EVENT_TIMER_INTERVAL  (jiffies + 3)
#define EVENT_PUSH_INTERVAL     msecs_to_jiffies(60)

//#define USE_WQ

static int debug = 0;
#define dprintk(msg...)	if (debug) { printk( "wheel key: " msg); }

static int g_irq_a, g_irq_b;

static int fake_wheel_level=0;	

static int fake_wheel_enable=0;	

static int wheel_inc_val=500;

static int lcd_on = 1;

extern struct class *iriver_class;

static struct early_suspend wheel_early_suspend;

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

#ifdef USE_WQ
    struct delayed_work event_wq;  /* [downer] A140929 for event push */
#else
    struct timer_list event_timer; /* [downer] A150105 for event push */
#endif
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
int old_encoder_port_a = _INPUT_HIGH;
int old_encoder_port_b = _INPUT_HIGH;

//extern void wheel_led_control(int volume);
void wheel_led_control(int volume)
{	

/*	
	//volume = (volume*170)/100;	
	
	if(volume < 30)
		volume = volume*100;	
	else if(volume < 60)
		volume = volume*150;	
	else if(volume < 90)
		volume = volume*200;	
	else if(volume < 120)
		volume = volume*250;	
	else
		volume = volume*300;	
*/
//	if(!fake_wheel_enable)
//		volume = volume*100;	


	dprintk("wheel_led_control:%d\n",volume);	

//	tca_tco_pwm_ctrl(0, GPIO_WHEEL_LED_PWM, 500, volume);	 					
}
EXPORT_SYMBOL(wheel_led_control);

static ssize_t fake_wheel_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("%s\n",__FUNCTION__);

	fake_wheel_enable = 0;
	
	printk("fake_wheel_level:%d\n",fake_wheel_level);
	
	return 0;
}
static ssize_t fake_wheel_level_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	printk("%s\n",__FUNCTION__);

	fake_wheel_enable = 1;

	//sscanf(buf,"%x %s %x",&phy_add,command,&set_value);	
	sscanf(buf,"%d\n",&fake_wheel_level);	

	wheel_led_control(fake_wheel_level);
	
	printk("fake_wheel_level:%d\n",fake_wheel_level);
	
	return count;
}
static DEVICE_ATTR(fake_wheel_level, 0664, fake_wheel_level_show, fake_wheel_level_store);

static ssize_t wheel_inc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("%s\n",__FUNCTION__);
	
	return sprintf(buf, "%s\n", wheel_inc_val);
}
static ssize_t wheel_inc_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{

	sscanf(buf,"%d\n",&wheel_inc_val);
	
	return count;
}
static DEVICE_ATTR(wheel_inc, 0664, wheel_inc_show, wheel_inc_store);

int wheel_create_file(struct class *cls){	
	struct device *dev = NULL;	
	int ret = -ENODEV;	

	//create "sys/class/iriver/wheel"
	dev = device_create(cls, NULL, 0, NULL, "wheel");	
	if (IS_ERR(dev)) {		
		pr_err("wheel_create_file: failed to create device(wheel)\n");
		return -ENODEV;	
	}	

	//create "sys/class/iriver/wheel/wheel_inc"
	//ex)  cd /sys/class/iriver/wheel
	//     echo "100" > wheel_inc ==> wheel_store
	//	   cat wheel_inc	==> wheel_show
	ret = device_create_file(dev, &dev_attr_wheel_inc);	
	if (unlikely(ret < 0))		
		pr_err("battery_create_file: failed to create device file, %s\n", dev_attr_wheel_inc.attr.name);
	

	//create "sys/class/iriver/wheel/fake_wheel_level"
	//ex)  cd sys/class/iriver/wheel
	//	   cat fake_wheel_level	==> fake_wheel_level_show
	//      echo "10" > fake_wheel_level ==> fake_wheel_level_store
	ret = device_create_file(dev, &dev_attr_fake_wheel_level);	
	if (unlikely(ret < 0))		
		pr_err("wheel_create_file: failed to create device file, %s\n", dev_attr_fake_wheel_level.attr.name);
	
	return 0;
}


void ak_wheel_lock_mode(int mode)
{
    if (mode) {
        disable_irq(g_irq_a);
        disable_irq(g_irq_b);
    }
    else {
        enable_irq(g_irq_a);
        enable_irq(g_irq_b);
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

static int rotary_encoder_get_state(struct rotary_encoder_platform_data *pdata)
{
	int a = !!gpio_get_value(pdata->gpio_a);
	int b = !!gpio_get_value(pdata->gpio_b);

	a ^= pdata->inverted_a;
	b ^= pdata->inverted_b;

	return ((a << 1) | b);
}

static void rotary_encoder_report_event(struct rotary_encoder *encoder)
{
	struct rotary_encoder_platform_data *pdata = encoder->pdata;
    unsigned int code = 0xff;
    
	if (pdata->relative_axis) {
		input_report_rel(encoder->input,
				 pdata->axis, encoder->dir ? -1 : 1);
        input_sync(encoder->input);        
	}
    else{
        /* [downer] A141014 */
        code = encoder_getKey();
        
        if (code == KEY_VOLUMEUP || code == KEY_VOLUMEDOWN) {
            input_report_key(encoder->input, code, 1);
            input_report_key(encoder->input, code, 0);
            input_sync(encoder->input);
        }
	}
}

#ifdef USE_WQ
static void event_report_work(struct work_struct *work)
#else
static void event_report_work(unsigned long data)    
#endif
{
#ifdef USE_WQ    
    struct rotary_encoder *encoder = container_of(work, struct rotary_encoder, event_wq);
#else
    struct rotary_encoder *encoder = (struct rotary_encoder *)data;
#endif
    
    rotary_encoder_report_event(encoder);

#ifndef USE_WQ
    mod_timer(&encoder->event_timer, EVENT_TIMER_INTERVAL);
#endif
}

static irqreturn_t rotary_encoder_irq(int irq, void *dev_id)
{
	struct rotary_encoder *encoder = dev_id;
	int state;

	state = rotary_encoder_get_state(encoder->pdata);

	switch (state) {
	case 0x0:
		if (encoder->armed) {
			rotary_encoder_report_event(encoder);
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
	
	encoder_port_a = gpio_get_value(GPIO_KEY_VOL_UP);
	encoder_port_b = gpio_get_value(GPIO_KEY_VOL_DOWN);

	if(irq == encoder->irq_a)
		irq_set_irq_type(irq, encoder_port_a ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
	else if(irq == encoder->irq_b)
		irq_set_irq_type(irq, encoder_port_b ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);

	state = rotary_encoder_get_state(encoder->pdata);
    
	switch (state) {
	case 0x00:
	case 0x03:
		if (state != encoder->last_stable) {
			if (encoder->dir){		
				dprintk("UP\n");
                encoder_data_push(KEY_VOLUMEUP);
				encoder->last_stable = state;

				fake_wheel_level = (fake_wheel_level + wheel_inc_val);

			}
            else {
				dprintk("DOWN\n");
                encoder_data_push(KEY_VOLUMEDOWN);
				encoder->last_stable = state;
                
				fake_wheel_level = (fake_wheel_level - wheel_inc_val);
			}	
		}
		break;

	case 0x01:
	case 0x02:
		encoder->dir = (encoder->last_stable + state) & 0x01;
 		break;
	}

#ifdef USE_WQ
    schedule_delayed_work(&encoder->event_wq, EVENT_PUSH_INTERVAL);
#endif
    
	spin_unlock(&encoder_lock);
	return IRQ_HANDLED;
}

static void wheel_led_dimming(struct work_struct *ignored)	
{		
	unsigned int count,volume;
	
	dprintk("\n+wheel_led_dimming\n");
	
	for(count=8; count<= 100; count++){
		if(count < 50){
			volume = count*100;	
			
		}else if(count < 60){
			volume = count*110;	
			
		}else if(count < 70){
			volume = count*120;	
			
		}else if(count < 80){
			volume = count*130;	
			
		}else if(count < 90){
			volume = count*150;	
			
		}else if(count < 100){
			volume = count*180;				
		}
		
		tca_tco_pwm_ctrl(0, GPIO_WHEEL_LED_PWM, 0xFFFF, volume);	
		
		if(count == 100){
			tca_tco_pwm_ctrl(0, GPIO_WHEEL_LED_PWM, 0xFFFF, 0xFFFF);		
		}
		
		if(!lcd_on)	{				
			tca_tco_pwm_ctrl(0, GPIO_WHEEL_LED_PWM, 0xFFFF, 0);	
			break;
		}
		
		msleep(1);
	}	

	dprintk("\n-wheel_led_dimming\n");

}

static DECLARE_WORK(wheel_led_type_work, wheel_led_dimming);

static void wheel_earlysuspend(struct early_suspend *h)
{	
	dprintk("======wheel_earlysuspend======\n");		

	lcd_on = 0;

	//cancel_delayed_work_sync(&wheel_led_type_work); 
	tca_tco_pwm_ctrl(0, GPIO_WHEEL_LED_PWM, 0xFFFF, 0);	 					
}

static void wheel_lateresume(struct early_suspend *h)
{

	dprintk("======wheel_lateresume=======\n");

	lcd_on = 1;
	
	schedule_work(&wheel_led_type_work);
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

    /* [downer] A140929 */
#ifdef USE_WQ
    INIT_DELAYED_WORK(&encoder->event_wq, event_report_work);
#else
    init_timer(&encoder->event_timer);
    encoder->event_timer.function = event_report_work;
    encoder->event_timer.data = (unsigned long)encoder;
    encoder->event_timer.expires = jiffies + EVENT_TIMER_INTERVAL;

    add_timer(&encoder->event_timer);
#endif
    
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
	if (pdata->half_period) {
		handler = &rotary_encoder_half_period_irq;
		encoder->last_stable = rotary_encoder_get_state(pdata);
	} else {
		handler = &rotary_encoder_irq;
	}

	tcc_gpio_config_ext_intr(INT_EINT1, EXTINT_GPIOA_05);
	err = request_irq(encoder->irq_a, handler,
			  IRQF_DISABLED | IRQF_TRIGGER_FALLING,
			  DRV_NAME, encoder);
	if (err) {
		dev_err(&pdev->dev, "unable to request IRQ %d\n",
			encoder->irq_a);
		goto exit_free_gpio_b;
	}

	tcc_gpio_config_ext_intr(INT_EINT3, EXTINT_GPIOA_06);
	err = request_irq(encoder->irq_b, handler,
            IRQF_DISABLED | IRQF_TRIGGER_FALLING,
			  DRV_NAME, encoder);
	if (err) {
		dev_err(&pdev->dev, "unable to request IRQ %d\n",
			encoder->irq_b);
		goto exit_free_irq_a;
	}

#if (CUR_AK == MODEL_AK500N)
	gpio_request(GPIO_WHEEL_LED, "GPIO_GPIO_WHEEL_LED");
	tcc_gpio_config(GPIO_WHEEL_LED,GPIO_FN(0));
	gpio_direction_output(GPIO_WHEEL_LED, 1); // output
	gpio_set_value(GPIO_WHEEL_LED, 1);	
	//gpio_set_value(GPIO_WHEEL_LED, 0);

	gpio_request(GPIO_WHEEL_LED_PWM, "GPIO_GPIO_WHEEL_LED_PWM");
	tcc_gpio_config(GPIO_WHEEL_LED_PWM,GPIO_FN(0));
	gpio_direction_output(GPIO_WHEEL_LED_PWM, 1); // output
	gpio_set_value(GPIO_WHEEL_LED_PWM, 1);


#endif

	wheel_early_suspend.suspend = wheel_earlysuspend;	
	wheel_early_suspend.resume = wheel_lateresume;	
	wheel_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;	
	register_early_suspend(&wheel_early_suspend);
	

	wheel_create_file(iriver_class);
	
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

#ifdef USE_WQ
    cancel_delayed_work(&encoder->event_wq);
#else
    del_timer(&encoder->event_timer);
#endif
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
