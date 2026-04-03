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
#include <mach/board_astell_kern.h>
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

#if (CUR_AK == MODEL_AK500N)
#define FULL_LEVEL_ADJUSTMENT 96
#else
#define FULL_LEVEL_ADJUSTMENT 96
#endif

#if(CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII) 
#define LOW_LEVEL_ADJUSTMENT 0
#endif

//#define CHANG_POWR_OFF_VOLTAGE
#ifdef CHANG_POWR_OFF_VOLTAGE
#define POWER_OFF_VOLTAGE	3300000
#endif

//----------------------------------------------------------------------------------
//#define rdebug(fmt,arg...) printk(KERN_CRIT "_[battery] " fmt ,## arg)
   #define rdebug(fmt,arg...)
//----------------------------------------------------------------------------------

#define USE_ADAPTOR             /* process adaptor online and charging, need to be fixed ws board */

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
static u32 elapsed_sec2 = 0;

static int debug_enable = 0;

static int org_soc;

static int fake_bat_level;	

static int fake_bat_enable=0;	

static int lcd_on =1;

static int short_charge=0;

static struct early_suspend bat_early_suspend;

#ifdef CONFIG_WAKEUP_ASSIST
extern void power_key_event(); //refer to wakeup_assist.c
#endif

extern int get_adapter_state(void); //detect adapter: refer to drd_otg.c

extern int get_current_pcfi_state(); //detect usb

extern INTERNAL_PLAY_MODE ak_get_internal_play_mode(void);

extern int get_pcm_stoped(void);

extern int get_xmos_is_playing(void);
extern int get_pcm_is_playing(void);
extern int is_amp_on(void);
extern int ak_dock_check_device(void);

/* [downer] A160523 for CHG_DET event */
extern int ak_check_usb_plug(void);
extern int ak_check_usb_unplug(void);

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
	/* usb online */
	int usb_online;
};
static bool isconnected;

void isUSBconnected(bool usb_connect)
{
#if 0
    if (gpio_get_value(GPIO_CHG_DET) == 0) // connected usb or adaptor
	    isconnected = usb_connect;
    else
	    isconnected = 0;
#else
    isconnected = usb_connect;
#endif
}
//EXPORT_SYMBOL(isUSBconnected);

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

	ret = get_adapter_state();

    return ret;
}


static int charger_online(void)
{
    return ak_check_usb_plug();
}

static int is_connected_charger(void)
{
    return ak_check_usb_plug();
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

		return ret;
		
	#if (CUR_AK > MODEL_AK240)  
		    ret = !gpio_get_value(GPIO_CHG_DONE );
	#else
		if(is_connected_charger())	
		    ret = gpio_get_value(GPIO_CHG_DONE );
		else
			ret = !gpio_get_value(GPIO_CHG_DONE );
	#endif
	
    rdebug("%s %d \n", __func__, ret);

    return ret;
}
#endif

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
		val->intval =  chip->usb_online;
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
		//TODO:
#ifdef USE_ADAPTOR
//		if (chip->pdata->charger_online)
//			val->intval = chip->pdata->charger_online();

    	//val->intval = charger_online();
			val->intval = adt_online();
//      rdebug("%s val->intval = %d\n", __func__, val->intval);
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
#if (CUR_AK == MODEL_AK500N)	
	chip->vcell = (chip->vcell * 250) * 10; //MAX17041 : 2.50mV
#else
	chip->vcell = (chip->vcell * 125) * 10; //MAX17040 : 1.25mV
#endif

}

static void high_charge(void)
{
	if (debug_enable) 	printk("charge=700mA ");
	gpio_set_value(GPIO_CHG_CTL1, 1); 
	gpio_set_value(GPIO_CHG_CTL2, 1);
}
static void low_charge(void)
{
#if 0
	if (debug_enable) printk("charge: 450mA\n");
	gpio_set_value(GPIO_CHG_CTL1, 0); 
	gpio_set_value(GPIO_CHG_CTL2, 1);
#else
	if (debug_enable) printk("charge=370mA ");
	gpio_set_value(GPIO_CHG_CTL1, 1); 
	gpio_set_value(GPIO_CHG_CTL2, 0);

#endif
}

static void lowest_charge(void)
{
	if (debug_enable) printk("charge=110mA ");
	gpio_set_value(GPIO_CHG_CTL1, 0); 
	gpio_set_value(GPIO_CHG_CTL2, 0);
}

static void charging_policy(void)
{

/*			CHG_CTL_1				|			CHG_CTL_2
PC				L								|						L							: 110mA
AC				L								|						H							: 450mA
AC				H								|						L							: 370mA
AC				H								|						H					 		: 700mA
				GPIO_A(1)					|			GPIO_A(2)
*/

#if 0
	
	int soc = get_battery_level();

	if(adt_online()){
		elapsed_sec2++;
		//printk("%d\n",elapsed_sec2);
		if(get_xmos_is_playing()){ 

			if(elapsed_sec2 <= 20)
					high_charge();
			else if(elapsed_sec2 > 20 && elapsed_sec2 <= 100)
					low_charge();				
			else
					elapsed_sec2=0;			
		}
		else{
			elapsed_sec2 = 0;
			high_charge();
		}
			
	}
	else {
		elapsed_sec2 = 0;

		lowest_charge(); //110mA
	}
#else
	int soc = get_battery_level();

	if(adt_online()){		
		low_charge(); //370mA		
	}
	else{  // PC
		lowest_charge(); //110mA
	}

#endif
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
	Bits=18
	SOCValue = ((SOC1 * 256) + SOC2) * 0.00390625
	Bits=19
	SOCValue = ((SOC1 * 256) + SOC2) * 0.001953125
	*/
	
#if (CUR_AK == MODEL_AK500N )
	chip->soc=((SOC1 <<8) + SOC2) /256; // 18-bit model : SOC_percent = ((SOC_1 << 8) + SOC_2) / 256;
#else
	chip->soc=((SOC1 <<8) + SOC2) /512; // 19-bit model : SOC_percent = ((SOC_1 << 8) + SOC_2) / 512;	
#endif

	org_soc=chip->soc;


	#if(CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII) 			
	chip->soc = (((chip->soc-LOW_LEVEL_ADJUSTMENT)*100)*100)/((FULL_LEVEL_ADJUSTMENT-LOW_LEVEL_ADJUSTMENT)*100);  // 2%가 0%, 96%가 100%
	#else
	chip->soc = (((chip->soc-1)*100)*100)/((FULL_LEVEL_ADJUSTMENT-1)*100);  // 1%가 0%, 96%가 100%로 display
	#endif

	if(chip->soc > 100) chip->soc = 100;
	if(chip->soc <0 ) chip->soc = 0;

#ifdef CHANG_POWR_OFF_VOLTAGE
	if(chip->soc ==0)	
		chip->soc = 1;
	
    if(chip->vcell <= POWER_OFF_VOLTAGE)
		chip->soc=0;
#endif
	
	//only test
	if(fake_bat_enable)
		chip->soc = fake_bat_level;

    if (chip->soc == 0)
        low_battery_power_off = 1;    

	set_battery_level(chip->soc);

	charging_policy();
	
    if (debug_enable)
        printk("_[battery] vcell=%d level=%d(%d) SOC1_2=%d usb=%d adt=%d battery_full=%d time=%d \n\n", chip->vcell, chip->soc, org_soc, ((SOC1 <<8) + SOC2), chip->usb_online  , adt_online(),  charger_done(), elapsed_sec++); 

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
//	if (chip->pdata->charger_online)
//		chip->online = chip->pdata->charger_online();
//	else
//		chip->online = 0;

	chip->online = charger_online();

//  rdebug("%s chip->online = %d\n", __func__, chip->online);

#else
		chip->online = 1;
#endif
}

static void max17040_get_status(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

#ifdef USE_ADAPTOR

	//chip->usb_online = isconnected;
	chip->usb_online = charger_online();
	
	if (is_connected_charger() == 0)
	    chip->usb_online = 0;    

#if (CUR_AK == MODEL_AK500N)
	chip->usb_online = gpio_get_value(GPIO_USB_ON);
#endif

	if (chip->online) 
	{
        UsbIndicator(0); 
        //printk("__%s Connected adpator \n\n", __func__);

		if(charger_done())
			chip->status = POWER_SUPPLY_STATUS_FULL;
		else 
		{
			//if(chip->soc > MAX17040_BATTERY_FULL)
			//	chip->status = POWER_SUPPLY_STATUS_FULL;
			//else
				chip->status = POWER_SUPPLY_STATUS_CHARGING;
		}
	}
	else if (chip->usb_online) 
	{
        UsbIndicator(1); 
        //printk("__%s Connected usb \n\n", __func__);

		if(charger_done())
			chip->status = POWER_SUPPLY_STATUS_FULL;
		else 
		{
			//if(chip->soc > MAX17040_BATTERY_FULL)
			//	chip->status = POWER_SUPPLY_STATUS_FULL;
			//else
				chip->status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		}
    }
	else 
	{
        UsbIndicator(0); 
        //printk("__%s Disconnected \n\n", __func__);

		chip->status = POWER_SUPPLY_STATUS_DISCHARGING;
	}
	
#else

	//chip->usb_online = isconnected;
	chip->usb_online = charger_online();
	if(chip->soc > 101) {
		chip->status = POWER_SUPPLY_STATUS_UNKNOWN;
		return;
	}

	if(chip->usb_online)
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
	int old_usb_online, old_online, old_vcell, old_soc;

	chip = container_of(work, struct max17040_chip, work.work);

	old_online = chip->online;
	old_usb_online = chip->usb_online;

	old_vcell = chip->vcell;
	old_soc = chip->soc; 

	max17040_get_online(chip->client);
	max17040_get_vcell(chip->client);
	max17040_get_soc(chip->client);
	max17040_get_status(chip->client);

	if((old_vcell != chip->vcell) || (old_soc != chip->soc)) 
		power_supply_changed(&chip->battery);
    
#ifdef USE_ADAPTOR
    #if 1
	if((old_online != chip->online) && (!chip->usb_online)) 
	{
        if (chip->online) {
			#ifdef DONOT_SLEEP_USB_INSERTED		
			if(!get_current_pcfi_state())
    		wake_lock(&vbus_wake_lock);
			#endif
			rdebug("wake_lock - adaptor\n");
		} else {
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
    #else
	if(old_online != chip->online)
		power_supply_changed(&chip->ac);
    #endif
#endif

#if 1
	if(old_usb_online != chip->usb_online)
    {
        if (chip->usb_online) {
	   		#ifdef DONOT_SLEEP_USB_INSERTED		
				if(!get_current_pcfi_state())
           wake_lock(&vbus_wake_lock);
			#endif	
			rdebug("wake_lock \n");
		} else {
    		/* give userspace some time to see the uevent and update
	    	 * LED state or whatnot...
		     */
   	   		#ifdef DONOT_SLEEP_USB_INSERTED		
					if(!get_current_pcfi_state())
		    		wake_lock_timeout(&vbus_wake_lock, HZ / 2);
					#endif			
           rdebug("wake_lock_timeout \n");
        }

		power_supply_changed(&chip->usb);
    }
#else
	if(old_usb_online != chip->usb_online)
		power_supply_changed(&chip->usb);
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

static enum power_supply_property usb_get_props[] = {
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
	//ex)  cd sys/class/iriver/battery
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

static int __devinit max17040_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max17040_chip *chip;
	int ret;

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

	chip->usb.name		= "usb";
	chip->usb.type		= POWER_SUPPLY_TYPE_USB;
	chip->usb.get_property	= usb_get_property;
	chip->usb.properties	= usb_get_props;
	chip->usb.num_properties	= ARRAY_SIZE(usb_get_props);
	chip->usb.external_power_changed = NULL;

	ret = power_supply_register(&client->dev, &chip->battery);
	if (ret)
		goto err_battery_failed;

	ret = power_supply_register(&client->dev, &chip->ac);
	if(ret)
		goto err_ac_failed;

	ret = power_supply_register(&client->dev, &chip->usb);
	if(ret)
		goto err_usb_failed;

	max17040_get_version(client);
	
	battery_create_file(iriver_class);

	//2013.11.19 iriver-ysyim
	bat_early_suspend.suspend = battery_early_suspend;	
	bat_early_suspend.resume = battery_late_resume;	
	bat_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;	
	register_early_suspend(&bat_early_suspend);

	gpio_request(GPIO_CHG_CTL1,"GPIO_CHG_CTL1");	
	tcc_gpio_config(GPIO_CHG_CTL1, GPIO_FN(0));
	gpio_direction_output(GPIO_CHG_CTL1, 0);		

	gpio_request(GPIO_CHG_CTL2,"GPIO_CHG_CTL2");	
	tcc_gpio_config(GPIO_CHG_CTL2, GPIO_FN(0));
	gpio_direction_output(GPIO_CHG_CTL2, 0);				

	gpio_request(GPIO_OTG_5V_EN,"GPIO_OTG_5V_EN");	
	tcc_gpio_config(GPIO_OTG_5V_EN, GPIO_FN(0));
	gpio_direction_output(GPIO_OTG_5V_EN, 0);	
	
	INIT_DELAYED_WORK_DEFERRABLE(&chip->work, max17040_work);
	schedule_delayed_work(&chip->work, MAX17040_DELAY);

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

	if(adt_online())
		high_charge();
	else
		lowest_charge();
	
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

	lowest_charge();

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
    
   // if (is_connected_charger() && isconnected)
    if (is_connected_charger())
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
