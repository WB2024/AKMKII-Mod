/*
 *  max17040_battery.c
 *  fuel-gauge systems for lithium-ion (Li+) batteries
 *
 *  Copyright (C) 2009 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
 *
 *  Copyright (C) 2010 iriver Co.,ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/max17040_battery.h>
#include <linux/switch.h> 
#include <linux/slab.h>

#include <asm/mach/irq.h>
#include <asm/io.h>
#include <mach/gpio.h>
#include <mach/ak_gpio_def.h>

#include <linux/earlysuspend.h>

#define MAX17040_VCELL_MSB	0x02
#define MAX17040_VCELL_LSB	0x03
#define MAX17040_SOC_MSB	0x04
#define MAX17040_SOC_LSB	0x05
#define MAX17040_MODE_MSB	0x06
#define MAX17040_MODE_LSB	0x07
#define MAX17040_VER_MSB	0x08
#define MAX17040_VER_LSB	0x09
#define MAX17040_RCOMP_MSB	0x0C
#define MAX17040_RCOMP_LSB	0x0D
#define MAX17040_CMD_MSB	0xFE
#define MAX17040_CMD_LSB	0xFF

#define MAX17040_DELAY		msecs_to_jiffies(1000*1)  //5sec
#define MAX17040_BATTERY_FULL	99

#define FULL_LEVEL_ADJUSTMENT 96

//#define CHANG_POWR_OFF_VOLTAGE
#ifdef CHANG_POWR_OFF_VOLTAGE
#define POWER_OFF_VOLTAGE	6500000
#endif

//----------------------------------------------------------------------------------
//#define rdebug(fmt,arg...) printk(KERN_CRIT "_[battery] " fmt ,## arg)
   #define rdebug(fmt,arg...)
//----------------------------------------------------------------------------------

#define USE_ADAPTOR             /* process adaptor online and charging, need to be fixed ws board */

#define CHARGER_CLOCK_DELAY		msecs_to_jiffies(1000*30) 

//#if (CUR_AK_REV <= AK240_HW_TP1)
//#define DONOT_SLEEP_USB_INSERTED
//#endif

struct switch_dev 		indicator_dev;

struct class *iriver_battery_class;

#ifdef DONOT_SLEEP_USB_INSERTED
static struct wake_lock vbus_wake_lock;
#endif
static struct wake_lock sleep_wakelock_delay;

static u32 elapsed_sec = 0;

static int debug_enable = 0;

static int org_soc;

static int fake_bat_level;	

static int fake_bat_enable=0;	

static int lcd_on =1;

static int g_irq;

static int enable_charger_reset=1;

static struct early_suspend bat_early_suspend;

struct delayed_work charger_reset_time;

#ifdef CONFIG_WAKEUP_ASSIST
extern void power_key_event(); //refer to wakeup_assist.c
#endif

extern int get_adapter_state(void); //detect adapter: refer to drd_otg.c

extern int get_current_pcfi_state(); //detect usb

void UsbIndicator(u8 state)
{
//  printk("__%s state = %d \n", __func__, state);
	switch_set_state(&indicator_dev, state);
}


void set_debug_battery(int enable)
{
    if (enable)
        debug_enable = 1;
    else
        debug_enable = 0;
}
EXPORT_SYMBOL(set_debug_battery);
//----------------------------------------------------------------------------------



struct max17040_chip {
	struct i2c_client		*client;
	struct delayed_work		work;
	struct power_supply		battery;
	struct power_supply		ac;
	struct power_supply		usb;
	struct max17040_platform_data	*pdata;

	/* State Of Connect */
	int online;
	/* battery voltage */
	int vcell;
	/* battery capacity */
	int soc;
	/* State Of Charge */
	int status;
};
static bool isconnected;

/*Shanghai ewada for usb status*/
extern struct class *iriver_class;

 
static int battery_soc_level = 50;
void set_battery_level(int level)
{
    battery_soc_level = level;        
}

int get_battery_level(void)
{
    return battery_soc_level;    
}


#ifdef USE_ADAPTOR
static int adt_online(void)
{
    int ret = 0;

    ret = !gpio_get_value(GPIO_CHG_DET);

    rdebug("%s %d \n", __func__, ret);

    return ret;
}


static int charger_online(void)
{
		int ret = 0;

    ret = !gpio_get_value(GPIO_CHG_DET);

    rdebug("%s %d \n", __func__, ret);

    return ret;
}

/*
Charger Status Output. 
Active-low, open-drain output pulls low when the battery is in fast-charge or
prequal. Otherwise, CHG is high impedance.

0 : charging
1 : not charging
*/
static int charger_done(void)
{
    int ret = 0;

    if(adt_online())
	    ret = gpio_get_value(GPIO_CHG_DONE);
		
    rdebug("%s %d \n", __func__, ret);

    return ret;
}
#endif

static void enable_charger_reset_func(struct work_struct *work)
{
	printk("enable_charger_reset\n");
	enable_charger_reset=1;	
	cancel_delayed_work(&charger_reset_time);
}


void charger_reset(void)
{	
  printk("+%s\n", __func__);
	
	//disable_irq(g_irq);
#if(CUR_AK_REV >= AK500N_HW_ES1)
	gpio_set_value(GPIO_CHG_OFF, 1);
	msleep(1000);
	gpio_set_value(GPIO_CHG_OFF, 0);	
	msleep(1000);
#endif
	//enable_irq(g_irq);	
	enable_charger_reset=0;
	schedule_delayed_work(&charger_reset_time, CHARGER_CLOCK_DELAY);
	
  printk("-%s\n", __func__);

}
void check_charger_state(struct work_struct *data)                                        
{
	
	//disable_irq(g_irq);
	
	printk("[%s] ADT:%d CHG:%d enable_charger_reset:%d\n",__FUNCTION__,adt_online(),charger_done(),enable_charger_reset);
	
	if((adt_online())  && (get_battery_level()<100) && (enable_charger_reset==1))
		charger_reset();

	//enable_irq(g_irq);
}
static DECLARE_DELAYED_WORK(check_charger_state_work,check_charger_state); 

#if defined(CONFIG_INPUT_BMA150_SENSOR)
	extern int bma150_sensor_get_air_temp(void);
#endif

static int max17040_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct max17040_chip *chip = container_of(psy,
				struct max17040_chip, battery);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = chip->status;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = chip->online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = chip->vcell;
		if(psp  == POWER_SUPPLY_PROP_PRESENT)
			val->intval = 1; /* You must never run Odrioid1 without Battery. */
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = chip->soc;
		break;
	 case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	 case POWER_SUPPLY_PROP_HEALTH:
	 	if(chip->vcell  < 2850)
			val->intval = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		else
		 	 val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		#if defined(CONFIG_INPUT_BMA150_SENSOR)
			val->intval = bma150_sensor_get_air_temp();
		#else
			val->intval = 365;
		#endif
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int usb_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	int ret = 0;
	struct max17040_chip *chip = container_of(psy,
				struct max17040_chip, usb);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		//TODO:
		val->intval =  chip->online;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int adapter_get_property(struct power_supply *psy,
			enum power_supply_property psp,
			union power_supply_propval *val)
{
#ifdef USE_ADAPTOR
	struct max17040_chip *chip = container_of(psy, struct max17040_chip, ac);
#endif
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_ONLINE:
#ifdef USE_ADAPTOR
        val->intval = adt_online();
#else
		val->intval = 0;
#endif
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}
static int max17040_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}


static int max17040_read_reg(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);

#if 0
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);
#endif

	return ret;
}

static void max17040_reset(struct i2c_client *client)
{
	max17040_write_reg(client, MAX17040_CMD_MSB, 0x54);
	max17040_write_reg(client, MAX17040_CMD_LSB, 0x00);
}

static void max17040_get_vcell(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	u8 msb;
	u8 lsb;

	msb = max17040_read_reg(client, MAX17040_VCELL_MSB);
	lsb = max17040_read_reg(client, MAX17040_VCELL_LSB);

	chip->vcell = (msb << 4) + (lsb >> 4);

	chip->vcell = (chip->vcell * 250) * 10; //MAX17041 : 2.50mV
}

static int low_battery_power_off = 0;
//extern int get_request_poweroff(void); 

static void max17040_get_soc(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	unsigned char SOC1, SOC2;


    if (low_battery_power_off == 1 && !charger_online())
    {
		if(!lcd_on)	{
     	printk("_[battery] Low battery power off chip->soc = %d \n\n", chip->soc);
#ifdef CONFIG_WAKEUP_ASSIST		
        power_key_event();
#endif
    }

    }    

	SOC1 = max17040_read_reg(client, MAX17040_SOC_MSB);
	SOC2 = max17040_read_reg(client, MAX17040_SOC_LSB);

	/*
	셀분석 업데로 부터 받은 INI파일에서 bits= 항목 확인
	Bits=18
	SOCValue = ((SOC1 * 256) + SOC2) * 0.00390625
	chip->soc=((SOC1 <<8) + SOC2) /256; // 18-bit model : SOC_percent = ((SOC_1 << 8) + SOC_2) / 256;
	
	Bits=19
	SOCValue = ((SOC1 * 256) + SOC2) * 0.001953125
	chip->soc=((SOC1 <<8) + SOC2) /512; // 19-bit model : SOC_percent = ((SOC_1 << 8) + SOC_2) / 512;	
	*/

	chip->soc=((SOC1 <<8) + SOC2) /512; // 19-bit model : SOC_percent = ((SOC_1 << 8) + SOC_2) / 512;	

	org_soc=chip->soc;

	chip->soc=(chip->soc*100)/FULL_LEVEL_ADJUSTMENT; 
	

	if(chip->soc > 100)
		chip->soc = 100;

#ifdef CHANG_POWR_OFF_VOLTAGE
	if(chip->soc ==0)	
		chip->soc = 1;
	
    if(chip->vcell <= POWER_OFF_VOLTAGE)
		chip->soc=0;
#endif
		
    set_battery_level(chip->soc);

	//only test
	if(fake_bat_enable)
		chip->soc = fake_bat_level;

  if (chip->soc == 0)
     low_battery_power_off = 1;    


    if (debug_enable)
        printk("_[battery] vcell=%d level=%d(%d) SOC1=%d SOC2=%d adt=%d battery_full=%d time=%d \n\n", chip->vcell, chip->soc, org_soc, SOC1, SOC2, adt_online(),  charger_done(), elapsed_sec++); 

	return;
}

static void max17040_get_version(struct i2c_client *client)
{
	u8 msb;
	u8 lsb;

	msb = max17040_read_reg(client, MAX17040_VER_MSB);
	lsb = max17040_read_reg(client, MAX17040_VER_LSB);

	dev_info(&client->dev, "MAX17040 Fuel-Gauge Ver %d%d\n", msb, lsb);
}

static void max17040_get_online(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

#ifdef USE_ADAPTOR
	chip->online = charger_online();
#else
    chip->online = 1;
#endif
}

static void max17040_get_status(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

#ifdef USE_ADAPTOR
//	chip->online = charger_online();
	
	if (chip->online) {
        UsbIndicator(0); 

	//	if(charger_done())
	//		chip->status = POWER_SUPPLY_STATUS_FULL;
	//	else 
	
            chip->status = POWER_SUPPLY_STATUS_CHARGING;
	}
	else {
        UsbIndicator(0);
		chip->status = POWER_SUPPLY_STATUS_DISCHARGING;
	}
	
#else
	//chip->usb_online = isconnected;
	chip->online = charger_online();
	if(chip->soc > 101) {
		chip->status = POWER_SUPPLY_STATUS_UNKNOWN;
		return;
	}

	if(chip->online)
		chip->status = POWER_SUPPLY_STATUS_CHARGING;
	else
		chip->status = POWER_SUPPLY_STATUS_DISCHARGING;

	if (chip->soc > MAX17040_BATTERY_FULL)
		chip->status = POWER_SUPPLY_STATUS_FULL;

#endif
}

static void max17040_work(struct work_struct *work)
{
	struct max17040_chip *chip;
	int old_online, old_vcell, old_soc;

	chip = container_of(work, struct max17040_chip, work.work);

	old_online = chip->online;
	old_vcell = chip->vcell;
	old_soc = chip->soc; 

	max17040_get_online(chip->client);
	max17040_get_vcell(chip->client);
	max17040_get_soc(chip->client);
	max17040_get_status(chip->client);

	if((old_vcell != chip->vcell) || (old_soc != chip->soc)) 
		power_supply_changed(&chip->battery);

#if 1
#ifdef USE_ADAPTOR
	if (old_online != chip->online)	{
        if (chip->online) {
            printk("\x1b[1;36m A/C connect! \x1b[0m\n");        
			#ifdef DONOT_SLEEP_USB_INSERTED		
			if(!get_current_pcfi_state())
    		wake_lock(&vbus_wake_lock);
			#endif
			rdebug("wake_lock - adaptor\n");
		} else {
            printk("\x1b[1;36m A/C disconnect! \x1b[0m\n");                
    		/* give userspace some time to see the uevent and update
	    	 * LED state or whatnot...
		     */
    		#ifdef DONOT_SLEEP_USB_INSERTED		
				if(!get_current_pcfi_state())
		    	wake_lock_timeout(&vbus_wake_lock, HZ / 2);
			#endif	
            rdebug("wake_lock_timeout - adaptor\n");
        }

		power_supply_changed(&chip->ac);
	}
#endif
#endif
    
	schedule_delayed_work(&chip->work, MAX17040_DELAY);
}

static enum power_supply_property max17040_battery_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_STATUS,
	/*POWER_SUPPLY_PROP_ONLINE,*/
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_property adapter_get_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
};


static ssize_t battery_log_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("%s\n",__FUNCTION__);
	
	return sprintf(buf, "%s\n", debug_enable ? "on" : "off");
}
static ssize_t battery_log_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	if(*buf == '1'){
		set_debug_battery(1);
	}	else {
		set_debug_battery(0);
	}

	return count;
}
static DEVICE_ATTR(battery_log, 0664, battery_log_show, battery_log_store);


static ssize_t fake_battery_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("%s\n",__FUNCTION__);
	
	return 0;
}
static ssize_t fake_battery_level_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	printk("%s\n",__FUNCTION__);

	fake_bat_enable = 1;

	//sscanf(buf,"%x %s %x",&phy_add,command,&set_value);	
	sscanf(buf,"%d\n",&fake_bat_level);	

	printk("fake_bat_level:%d\n",fake_bat_level);
	
	return count;
}
static DEVICE_ATTR(fake_battery_level, 0664, fake_battery_level_show, fake_battery_level_store);

int battery_create_file(struct class *cls){	
	struct device *dev = NULL;	
	int ret = -ENODEV;	

	//create "sys/class/iriver/battery"
	dev = device_create(cls, NULL, 0, NULL, "battery");	
	if (IS_ERR(dev)) {		
		pr_err("battery_create_file: failed to create device(battery)\n");
		return -ENODEV;	
	}	

	//create "sys/class/iriver/battery/battery_log"
	//ex)  cd sys/class/iriver/battery
	//	   cat battery_log	==> battery_show
	//       echo "1" > battery_log ==> battery_store
	ret = device_create_file(dev, &dev_attr_battery_log);	
	if (unlikely(ret < 0))		
		pr_err("battery_create_file: failed to create device file, %s\n", dev_attr_battery_log.attr.name);

	//create "sys/class/iriver/battery/fake_battery_level"
	//ex)  echo "4" > sys/class/iriver/battery/fake_battery_level
	//	   cat fake_battery_level	==> fake_battery_level_show
	//      echo "10" > fake_battery_level ==> fake_battery_level_store
	ret = device_create_file(dev, &dev_attr_fake_battery_level);	
	if (unlikely(ret < 0))		
		pr_err("battery_create_file: failed to create device file, %s\n", dev_attr_fake_battery_level.attr.name);
	
	return 0;
}

static void battery_early_suspend(struct early_suspend *h)
{	
	printk("%s\n",__FUNCTION__);		

	lcd_on = 0;
}

static void battery_late_resume(struct early_suspend *h)
{	
	printk("%s\n",__FUNCTION__);		

	lcd_on = 1;
}

static irqreturn_t charger_status_irq(int irq, void *dev_id)
{

	schedule_delayed_work(&check_charger_state_work,200);   // call  check_charger_state  after delay
	
	return IRQ_HANDLED;
}
static int __devinit max17040_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max17040_chip *chip;
	irq_handler_t handler;
	int ret,err;
	int irq;
	
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, chip);

	chip->battery.name		= "battery";
	chip->battery.type		= POWER_SUPPLY_TYPE_BATTERY;
	chip->battery.get_property	= max17040_get_property;
	chip->battery.properties	= max17040_battery_props;
	chip->battery.num_properties	= ARRAY_SIZE(max17040_battery_props);
	chip->battery.external_power_changed = NULL;

	chip->ac.name		= "ac";
	chip->ac.type		= POWER_SUPPLY_TYPE_MAINS;
	chip->ac.get_property	= adapter_get_property;
	chip->ac.properties	= adapter_get_props;
	chip->ac.num_properties	= ARRAY_SIZE(adapter_get_props);
	chip->ac.external_power_changed = NULL;

	ret = power_supply_register(&client->dev, &chip->battery);
	if (ret)
		goto err_battery_failed;

	ret = power_supply_register(&client->dev, &chip->ac);
	if(ret)
		goto err_ac_failed;

	max17040_get_version(client);
	
#if(CUR_AK_REV >= AK500N_HW_ES1)
	gpio_request(GPIO_CHG_OFF, "GPIO_CHG_OFF");
	tcc_gpio_config(GPIO_CHG_OFF,GPIO_FN(0));
	gpio_direction_output(GPIO_CHG_OFF, 1); // output
	gpio_set_value(GPIO_CHG_OFF, 0);	
#endif

	battery_create_file(iriver_class);

	handler = &charger_status_irq;
	irq = gpio_to_irq(GPIO_CHG_DET);
	g_irq = irq;
	tcc_gpio_config_ext_intr(INT_EINT0, EXTINT_GPIOG_16);
	err = request_irq(irq, handler,
				IRQF_DISABLED |  IRQF_TRIGGER_RISING, 
				  "max17040", NULL);

	if (err) {
		dev_err(&client->dev, "err:%d unable to request IRQ %d\n",err,	irq);
		//goto exit_free_irq_a;
	}

	//2013.11.19 iriver-ysyim
	bat_early_suspend.suspend = battery_early_suspend;	
	bat_early_suspend.resume = battery_late_resume;	
	bat_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;	
	register_early_suspend(&bat_early_suspend);
	
	INIT_DELAYED_WORK_DEFERRABLE(&chip->work, max17040_work);
	schedule_delayed_work(&chip->work, MAX17040_DELAY);

	INIT_DELAYED_WORK_DEFERRABLE(&charger_reset_time, enable_charger_reset_func);
	
	
	return 0;

err_usb_failed:
	power_supply_unregister(&chip->ac);
err_ac_failed:
	power_supply_unregister(&chip->battery);
err_battery_failed:
	dev_err(&client->dev, "failed: power supply register\n");
	i2c_set_clientdata(client, NULL);
	kfree(chip);

	return ret;
}

static int __devexit max17040_remove(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	power_supply_unregister(&chip->usb);
	power_supply_unregister(&chip->ac);
	power_supply_unregister(&chip->battery);
	cancel_delayed_work(&chip->work);
	i2c_set_clientdata(client, NULL);
	kfree(chip);
	return 0;
}

#if defined(CONFIG_PM)
static int max17040_suspend(struct i2c_client *client,
                            pm_message_t state)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	cancel_delayed_work(&chip->work);
    rdebug("%s\n", __func__);
    
    return 0;
}

static int max17040_resume(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

    /* [downer] A131204 */
	max17040_get_online(chip->client);
    power_supply_changed(&chip->usb);

	schedule_delayed_work(&chip->work, MAX17040_DELAY);

//	wake_lock_timeout(&sleep_wakelock_delay, 5 * HZ);  
	
    rdebug("%s\n", __func__);
    
	return 0;
}

#else

#define max17040_suspend NULL
#define max17040_resume NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id max17040_id[] = {
	{ "max17040", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max17040_id);

static struct i2c_driver max17040_i2c_driver = {
	.driver	= {
		.name	= "max17040",
	},
	.probe		= max17040_probe,
	.remove		= __devexit_p(max17040_remove),
	.suspend	= max17040_suspend,
	.resume		= max17040_resume,
	.id_table	= max17040_id,
};

#if 1 
#define DRIVER_NAME  "mtp"
static ssize_t print_switch_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "%s\n", DRIVER_NAME);
}

static ssize_t print_switch_state(struct switch_dev *sdev, char *buf)
{
    bool connect;
    
    if (adt_online())
        connect = 1;
    else
        connect = 0;

	return sprintf(buf, "%s\n", (connect ? "online" : "offline"));
//	return sprintf(buf, "%s\n", (isconnected ? "online" : "offline"));
}
#endif

static int __init max17040_init(void)
{
#if 1 
	indicator_dev.name = DRIVER_NAME;
	indicator_dev.print_name = print_switch_name;
	indicator_dev.print_state = print_switch_state;
	switch_dev_register(&indicator_dev);
#endif

	#ifdef DONOT_SLEEP_USB_INSERTED
	wake_lock_init(&vbus_wake_lock, WAKE_LOCK_SUSPEND, "vbus_present"); 
	#endif
	wake_lock_init(&sleep_wakelock_delay, WAKE_LOCK_SUSPEND, "sleep_delay"); 

	return i2c_add_driver(&max17040_i2c_driver);
}
module_init(max17040_init);

static void __exit max17040_exit(void)
{
	i2c_del_driver(&max17040_i2c_driver);
}
module_exit(max17040_exit);

MODULE_AUTHOR("Minkyu Kang <mk7.kang@samsung.com>");
MODULE_DESCRIPTION("MAX17040 Fuel Gauge");
MODULE_LICENSE("GPL");
