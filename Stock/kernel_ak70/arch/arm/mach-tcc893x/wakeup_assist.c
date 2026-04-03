/*
 * Wakeup assist driver
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/err.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <mach/reg_physical.h>
#include <mach/ak_gpio_def.h>
#include <linux/gpio.h>

#define DEV_NAME "wakeup_assist"

static int wakeup_assist_keycode[] = { KEY_POWER, KEY_PLAYPAUSE };

static struct input_dev *input_key;
	
unsigned int skip_power_key_event=0;
EXPORT_SYMBOL(skip_power_key_event);

static int __devinit wakeup_assist_probe(struct platform_device *pdev)
{
	int error;
	struct input_dev *input_dev;

	printk("%s\n",__FUNCTION__);

	input_dev = input_key= input_allocate_device();

	if (!input_dev)
		return -ENOMEM;

	input_dev->name = DEV_NAME;
	input_dev->id.bustype = BUS_HOST;
	input_dev->dev.parent = &pdev->dev;

	input_dev->evbit[0] = BIT_MASK(EV_KEY);

	input_set_capability(input_dev, EV_MSC, MSC_SCAN);

	input_dev->keycode = wakeup_assist_keycode;
	input_dev->keycodesize = sizeof(wakeup_assist_keycode[0]);
	input_dev->keycodemax = ARRAY_SIZE(wakeup_assist_keycode);

	__set_bit(wakeup_assist_keycode[0], input_dev->keybit);
	__clear_bit(KEY_RESERVED, input_dev->keybit);

	error = input_register_device(input_dev);
	if (error) {
		input_free_device(input_dev);
		return error;
	}	
	
	platform_set_drvdata(pdev, input_dev);

	return 0;

}

static int __devexit wakeup_assist_remove(struct platform_device *pdev)
{
	struct input_dev *input_dev = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	input_unregister_device(input_dev);
	input_free_device(input_dev);

	return 0;
}

void power_key_event()
{
		input_report_key(input_key, wakeup_assist_keycode[0], 0x1);
		input_sync(input_key);
		input_report_key(input_key, wakeup_assist_keycode[0], 0x0);
		input_sync(input_key);	
}
EXPORT_SYMBOL(power_key_event);

void playpause_key_event()
{
	input_report_key(input_key, wakeup_assist_keycode[1], 0x1);
	input_sync(input_key);
	input_report_key(input_key, wakeup_assist_keycode[1], 0x0);
	input_sync(input_key);	
} EXPORT_SYMBOL(playpause_key_event);

static int wakeup_assist_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct input_dev *input_dev = platform_get_drvdata(pdev);
	unsigned int reg, short_press;
#if 0
    reg =   ((PPMU)tcc_p2v(HwPMU_BASE))->PMU_WKSTS0.nREG;
		
	short_press = !gpio_get_value(GPIO_PWR_ON_OFF ); 

	//PMU_WKUPSTS0 SRCS[12] : GPIO_B[13](Powerkey)
	if ((reg & 0x1000)) {
		printk("%s, power key(%d)\n",__FUNCTION__, short_press);

		power_key_event();
		
		//refer to Gpio_input.c
		if(short_press)
			skip_power_key_event=0;
		else
			skip_power_key_event=1;  //prohibit  power key from taking double event in Gpio_input.c
	}

	reg = ((PPMU)tcc_p2v(HwPMU_BASE))->PMU_WKCLR0.nREG;
	
	reg = (reg | (1<<12));

	((PPMU)tcc_p2v(HwPMU_BASE))->PMU_WKCLR0.nREG = reg;
#endif
	return 0;
	
}

static const struct dev_pm_ops wakeup_assist_pm_ops = {
	.resume		= wakeup_assist_resume,
};

static struct platform_driver wakeup_assist_driver = {
	.probe		= wakeup_assist_probe,
	.remove		= __devexit_p(wakeup_assist_remove),
	.driver		= {
		.name	= DEV_NAME,
		.owner	= THIS_MODULE,
		.pm	= &wakeup_assist_pm_ops,
	},
};

static int __init wakeup_assist_init(void)
{
	return platform_driver_register(&wakeup_assist_driver);
}
module_init(wakeup_assist_init);

static void __exit wakeup_assist_exit(void)
{
	platform_driver_unregister(&wakeup_assist_driver);
}
module_exit(wakeup_assist_exit);

MODULE_DESCRIPTION("Wakeup assist driver");
MODULE_AUTHOR("Eunki Kim <eunki_kim@samsung.com>");
MODULE_LICENSE("GPL");
