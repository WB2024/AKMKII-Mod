/*
 *  max17040_ext_battery.c
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
#include <linux/max17040_ext_battery.h>
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

#define MAX17040_DELAY		msecs_to_jiffies(1000*1)  // 1s
#define MAX17040_BATTERY_FULL	99

#define FULL_LEVEL_ADJUSTMENT 90  //  (mac % x FULL_LEVEL_ADJUSTMENT ) /100 =  NEW FULL_LEVEL_ADJUSTMENT

//#define CHANG_POWR_OFF_VOLTAGE
#ifdef CHANG_POWR_OFF_VOLTAGE
#define POWER_OFF_VOLTAGE	3200000
#endif

//----------------------------------------------------------------------------------
//#define rdebug(fmt,arg...) printk(KERN_CRIT "_[ext_battery] " fmt ,## arg)
   #define rdebug(fmt,arg...)
//----------------------------------------------------------------------------------

#define USE_ADAPTOR             /* process adaptor online and charging, need to be fixed ws board */

//#if (CUR_AK_REV <= AK240_HW_TP1)
//#define DONOT_SLEEP_USB_INSERTED
//#endif

//OCVTest = 56096 //0xDB20-mx100
#define OCVTest_High_Byte 0xDB
#define OCVTest_Low_Byte 0xF0
#define SOCCheckA   236 
#define SOCCheckB   238 
#define RCOMP0	93
#define MDOEL_SIZE  17  /* bytes */
struct switch_dev 		indicator_ext_dev;

struct class *iriver_ext_battery_class;

#ifdef DONOT_SLEEP_USB_INSERTED
static struct wake_lock vbus_wake_lock;
#endif
static struct wake_lock sleep_wakelock_delay;
static struct i2c_client		*gExt_bat_i2c_client;
static struct max17040_ext_chip		*gExt_chip;


static u32 elapsed_sec = 0;

static int debug_enable = 0;

static int org_soc;

static int fake_bat_level;	

int fake_ext_bat_enable=0;	

static int lcd_on =1;

static int doc_bat_start=0;

static struct early_suspend ext_bat_early_suspend;

#ifdef CONFIG_WAKEUP_ASSIST
extern void power_key_event(); //refer to wakeup_assist.c
#endif

extern int get_adapter_state(void); //detect adapter: refer to drd_otg.c

extern int get_current_pcfi_state(); //detect usb
static int init_fuel_gauge(void);
int start_ext_battery(void);
int stop_ext_battery(void);

static unsigned char model0[] = {
	0x40,
	0xA7,
	0x80,
	0xAE,
	0x90,
	0xB5,
	0xA0,
	0xB8,
	0x00,
	0xBA,
	0x10,
	0xBA,
	0xF0,
	0xBC,
	0x20,
	0xBD,
	0x00
};

static unsigned char model1[] = {
	0x50,
	0xBD,
	0xE0,
	0xBF,
	0xE0,
	0xC3,
	0xD0,
	0xC4,
	0xD0,
	0xC7,
	0x80,
	0xCA,
	0x30,
	0xCD,
	0x90,
	0xD1,
	0xF0
};

static unsigned char model2[] = {
	0x60,
	0x01, 
	0x80, 
	0x01, 
	0x80, 
	0x15, 
	0xE0, 
	0x1F, 
	0x40, 
	0x35, 
	0xC0, 
	0x2B, 
	0x00, 
	0x40, 
	0x20, 
	0x40, 
	0x40
};

static unsigned char model3[] = {
	0x70,
	0x2D,
	0xA0,
	0x18,
	0x80,
	0x12,
	0x20,
	0x18,
	0x20,
	0x15,
	0x20,
	0x11,
	0x80,
	0x0F,
	0x20,
	0x0F,
	0x20
};

/* fuel gauage table : voltage, gradient, intercept */
struct _with_charging_soc_table 
{
	int vol;
	int a;
	int b;
};

// with charger
struct _with_charging_soc_table tbl[8] =
{
	{4193000, 14500, 2748700},
	{4140000, 11000, 3086100},
	{3940000,  8500, 3278700},
	{3780000,  4100, 3535100},
	{3750000,  7100, 3381600},
	{3690000,  2400, 3586300},
	{3625000,  5000, 3544100},
	{3605000, 11900, 3460300},
};

// without charger
struct _with_charging_soc_table tbl_a[10] =
{
	{4100000, 21500, 2012100},
	{4050000, 12200, 2916900},
	{3840000,  9900, 3130600},
	{3690000,  7500, 3299800},
	{3640000,  4700, 3448400},
	{3620000,  4000, 3476000},
	{3560000,  2400, 3534300},
	{3545000,  8000, 3473100},
	{3435000, 15100, 3409700},
	{3400000, 54100, 3344500},

};

static void UsbIndicator(u8 state)
{
//  printk("__%s state = %d \n", __func__, state);
	switch_set_state(&indicator_ext_dev, state);
}


void set_debug_ext_battery(int enable)
{
    if (enable)
        debug_enable = 1;
    else
        debug_enable = 0;
}
EXPORT_SYMBOL(set_debug_ext_battery);
//----------------------------------------------------------------------------------



struct max17040_ext_chip {
	struct i2c_client		*client;
	struct delayed_work		work;
	struct power_supply		battery;
	struct power_supply		ac;
	struct power_supply		usb;
	struct max17040_ext_platform_data	*pdata;

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

/*Shanghai ewada for usb status*/
extern struct class *iriver_class;

 
static int battery_soc_level = 50;
static void set_ext_battery_level(int level)
{
    battery_soc_level = level;        
}

 int get_ext_battery_level(void)
{
    return battery_soc_level;    
}
EXPORT_SYMBOL(get_ext_battery_level);

#ifdef USE_ADAPTOR
static int adt_online(void)
{
    int ret = 0;

    ret = get_adapter_state();

    return ret;
}


static int charger_online(void)
{
    int ret = 1;

    ret = !gpio_get_value(GPIO_CHG_DET);

    if (ret == 1)
        ret = 0;
    else
        ret = 1;

  //  if (ret && isconnected)
  //      ret = 0;  

    rdebug("%s %d \n", __func__, ret);

    return ret;
}
static int is_connected_charger(void)
{
    if (!gpio_get_value(GPIO_CHG_DET))
        return 0;
    
    return 1;
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

static int max17040_ext_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct max17040_ext_chip *chip = container_of(psy,
				struct max17040_ext_chip, battery);

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
			val->intval = 365;
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
	struct max17040_ext_chip *chip = container_of(psy,
				struct max17040_ext_chip, usb);

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
	struct max17040_ext_chip *chip = container_of(psy, struct max17040_ext_chip, ac);
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
static int max17040_ext_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}


static int max17040_ext_read_reg(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);

#if 0
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);
#endif

	return ret;
}

static void max17040_ext_reset(struct i2c_client *client)
{
	max17040_ext_write_reg(client, MAX17040_CMD_MSB, 0x54);
	max17040_ext_write_reg(client, MAX17040_CMD_LSB, 0x00);
}

static int max17040_ext_get_vcell(struct i2c_client *client)
{
	struct max17040_ext_chip *chip = i2c_get_clientdata(client);
	u8 msb;
	u8 lsb;

	msb = max17040_ext_read_reg(client, MAX17040_VCELL_MSB);
	lsb = max17040_ext_read_reg(client, MAX17040_VCELL_LSB);

	chip->vcell = (msb << 4) + (lsb >> 4);
#if (CUR_AK == MODEL_AK500N)	
	chip->vcell = (chip->vcell * 250) * 10; //MAX17041 : 2.50mV
#else
	chip->vcell = (chip->vcell * 125) * 10; //MAX17040 : 1.25mV
#endif

	return (chip->vcell/1000);
}

static int max17040_ext_get_version(struct i2c_client *client)
{
	u8 msb=0;
	u8 lsb=0;
	int fuel_gage_version=0;

	msb = max17040_ext_read_reg(client, MAX17040_VER_MSB);
	lsb = max17040_ext_read_reg(client, MAX17040_VER_LSB);

	fuel_gage_version = ((msb & 0xff) << 8) | (lsb & 0xff);	

	dev_info(&client->dev, "Ext MAX17040 Fuel-Gauge Ver %x%x(0x%x)\n", msb, lsb, fuel_gage_version);
	
	return fuel_gage_version;
}

int check_dock_battery_id(void)
{
	int id_version;
	id_version = max17040_ext_get_version(gExt_bat_i2c_client);
	if(id_version == 0x2)
		return true;
	else
		return false;
}
EXPORT_SYMBOL(check_dock_battery_id);

static int low_battery_power_off = 0;
//extern int get_request_poweroff(void); 

static int max17040_ext_get_soc(struct i2c_client *client)
{
	struct max17040_ext_chip *chip = i2c_get_clientdata(client);
	unsigned char SOC1, SOC2;

	SOC1 = max17040_ext_read_reg(client, MAX17040_SOC_MSB);
	SOC2 = max17040_ext_read_reg(client, MAX17040_SOC_LSB);

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
	chip->soc=(chip->soc*100)/FULL_LEVEL_ADJUSTMENT; 
	

	if(chip->soc > 100)
		chip->soc = 100;

#ifdef CHANG_POWR_OFF_VOLTAGE
	if(chip->soc ==0)	
		chip->soc = 1;
	
    if(chip->vcell <= POWER_OFF_VOLTAGE)
		chip->soc=0;
#endif
	
	//only test
	if(fake_ext_bat_enable)
		chip->soc = fake_bat_level;

	set_ext_battery_level(chip->soc);

    if (chip->soc == 0)
        low_battery_power_off = 1;    

    if (debug_enable)
        printk("_[ext_battery] vcell=%d level=%d(%d) SOC1=%d SOC2=%d usb=%d adt=%d battery_full=%d time=%d \n\n", chip->vcell, chip->soc, org_soc, SOC1, SOC2, chip->usb_online  , adt_online(),  charger_done(), elapsed_sec++); 

	return chip->soc;
}

static void max17040_ext_get_online(struct i2c_client *client)
{
	struct max17040_ext_chip *chip = i2c_get_clientdata(client);

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

static void max17040_ext_get_status(struct i2c_client *client)
{
	struct max17040_ext_chip *chip = i2c_get_clientdata(client);

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

static void max17040_ext_work(struct work_struct *work)
{
	struct max17040_ext_chip *chip;
	int old_usb_online, old_online, old_vcell, old_soc;

	chip = container_of(work, struct max17040_ext_chip, work.work);

	old_online = chip->online;
	old_usb_online = chip->usb_online;

	old_vcell = chip->vcell;
	old_soc = chip->soc; 

	max17040_ext_get_online(chip->client);
	max17040_ext_get_vcell(chip->client);
	max17040_ext_get_soc(chip->client);
	max17040_ext_get_status(chip->client);

	if(doc_bat_start){
		chip->vcell= 0;
		chip->soc = 0;
		doc_bat_start = 0;
	}
	
	//if((old_vcell != chip->vcell) || (old_soc != chip->soc)) {
	if((old_soc != chip->soc)) {
		//printk("ext power_supply_changed: old_vcell:%d, chip->vcell:%d, old_soc:%d, chip->soc:%d\n",old_vcell, chip->vcell, old_soc, chip->soc);
		power_supply_changed(&chip->battery);
	}
    
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

		//power_supply_changed(&chip->ac);
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

		//power_supply_changed(&chip->usb);
    }
#else
	if(old_usb_online != chip->usb_online)
		power_supply_changed(&chip->usb);
#endif

	schedule_delayed_work(&chip->work, MAX17040_DELAY);
}

static enum power_supply_property max17040_ext_battery_props[] = {
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
		set_debug_ext_battery(1);
	}	else {
		set_debug_ext_battery(0);
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

	fake_ext_bat_enable = 1;

	//sscanf(buf,"%x %s %x",&phy_add,command,&set_value);	
	sscanf(buf,"%d\n",&fake_bat_level);	

	printk("fake_bat_level:%d\n",fake_bat_level);

#if 0
	if(fake_bat_level == 100)
		start_ext_battery();
	else if(fake_bat_level == 200)
		stop_ext_battery();	
#endif

	return count;
}
static DEVICE_ATTR(fake_battery_level, 0664, fake_battery_level_show, fake_battery_level_store);

static int ext_battery_create_file(struct class *cls){	
	struct device *dev = NULL;	
	int ret = -ENODEV;	

	//create "sys/class/iriver/battery"
	dev = device_create(cls, NULL, 0, NULL, "ext_battery");	
	if (IS_ERR(dev)) {		
		pr_err("ext_battery_create_file: failed to create device(battery)\n");
		return -ENODEV;	
	}	

	//create "sys/class/iriver/ext_battery/battery_log"
	//ex)  cd sys/class/iriver/ext_battery
	//	   cat ext_battery_log	==> ext_battery_show
	//       echo "1" > ext_battery_log ==> ext_battery_store
	ret = device_create_file(dev, &dev_attr_battery_log);	
	if (unlikely(ret < 0))		
		pr_err("ext_battery_create_file: failed to create device file, %s\n", dev_attr_battery_log.attr.name);

	//create "sys/class/iriver/battery/fake_battery_level"
	//ex)  cd sys/class/iriver/battery
	//	   cat fake_battery_level	==> fake_battery_level_show
	//      echo "10" > fake_battery_level ==> fake_battery_level_store
	ret = device_create_file(dev, &dev_attr_fake_battery_level);	
	if (unlikely(ret < 0))		
		pr_err("battery_create_file: failed to create device file, %s\n", dev_attr_fake_battery_level.attr.name);
	
	return 0;
}

static void ext_battery_early_suspend(struct early_suspend *h)
{	
	printk("%s\n",__FUNCTION__);		

	lcd_on = 0;
}

static void ext_battery_late_resume(struct early_suspend *h)
{	
	printk("%s\n",__FUNCTION__);		

	lcd_on = 1;
}

int do_quick_start(void)
{		
	unsigned char fdata[5] = {0x00,};
 	int fdata_size;
	
	// quick_start mode
	printk("[EXT_MAX17040] === do_quick_start === \n");

	fdata[0] = 0x06;
	fdata[1] = 0x40;
	fdata[2] = 0x00;
	fdata_size = 3;
	i2c_master_send(gExt_bat_i2c_client, fdata, fdata_size);	
	
	msleep(500);

	fdata[0] = 0x06;
	fdata[1] = 0x00;
	fdata[2] = 0x00;
	fdata_size = 3;
	i2c_master_send(gExt_bat_i2c_client, fdata, fdata_size);

	return 0;
}

static int check_quick_start(void)
{
	int i = 0, cal_soc, soc, vcell;
  unsigned char fdata[5] = {0x00,};
  int soc_diff;


   printk("[EXT_MAX17040] Check Quick Start \n");

	vcell = max17040_ext_get_vcell(gExt_bat_i2c_client)*1000;
	soc = max17040_ext_get_soc(gExt_bat_i2c_client);
	 
	printk("[EXT_MAX17040] soc = %d, VCELL = %d mV\n", soc, vcell);

    if (adt_online())  
    {
	    printk("[EXT_MAX17040] With charger \n");
	    
	    if(vcell <= tbl[0].vol) i=0;
	    else if(vcell > tbl[0].vol && vcell <= tbl[1].vol) i=1;
	    else if(vcell > tbl[1].vol && vcell <= tbl[2].vol) i=2;
	    else if(vcell > tbl[2].vol && vcell <= tbl[3].vol) i=3;
	    else if(vcell > tbl[3].vol && vcell <= tbl[4].vol) i=4;
	    else if(vcell > tbl[4].vol && vcell <= tbl[5].vol) i=5;
	    else if(vcell > tbl[5].vol && vcell <= tbl[6].vol) i=6;
	    else if(vcell > tbl[6].vol && vcell <= tbl[7].vol) i=7;
	    else if(vcell > tbl[7].vol) i=8;

	    cal_soc = (vcell - tbl[i].b) / tbl[i].a;

			//printk("[EXT_MAX17040] vcell:%d tbl[%d].vol:%d soc:%d%% cal_soc:%d%%\n", vcell, i, tbl[i].vol, soc, cal_soc);
			printk("[EXT_MAX17040] vcell:%d tbl[%d].vol:%d  %d = (%d - %d) / %d \n", vcell,i, tbl[i].vol , cal_soc, vcell, tbl[i].b, tbl[i].a );
    }
    else
    {
	    printk("[EXT_MAX17040] Without charger \n");

	    if(vcell >= tbl_a[0].vol) i=0;
	    else if(vcell < tbl_a[0].vol && vcell >= tbl_a[1].vol) i=1;
	    else if(vcell < tbl_a[1].vol && vcell >= tbl_a[2].vol) i=2;
	    else if(vcell < tbl_a[2].vol && vcell >= tbl_a[3].vol) i=3;
	    else if(vcell < tbl_a[3].vol && vcell >= tbl_a[4].vol) i=4;
	    else if(vcell < tbl_a[4].vol && vcell >= tbl_a[5].vol) i=5;
	    else if(vcell < tbl_a[5].vol && vcell >= tbl_a[6].vol) i=6;
	    else if(vcell < tbl_a[6].vol && vcell >= tbl_a[7].vol) i=7;
	    else if(vcell < tbl_a[7].vol && vcell >= tbl_a[8].vol) i=8;
	    else if(vcell < tbl_a[8].vol) i=9;


	    cal_soc = (vcell - tbl_a[i].b) / tbl_a[i].a;

			printk("[EXT_MAX17040] vcell:%d tbl_a[%d].vol:%d soc:%d%% cal_soc:%d%%\n", vcell, i, tbl_a[i].vol, soc, cal_soc);
    }

    if (soc >= cal_soc)
        soc_diff = soc - cal_soc;
    else
        soc_diff = cal_soc - soc ;
        
    printk("[EXT_MAX17040] soc_diff = %d \n", soc_diff);

		return soc_diff;

}

static int init_fuel_gauge(void)
{
	//struct i2c_adapter *adapter = to_i2c_adapter(gExt_bat_i2c_client->dev.parent);
	
	int fdata_size,ret;
	unsigned char fdata[5];
	unsigned char read_cmd[1];
	unsigned char SOC1, SOC2;
	unsigned char OriginalRCOMP1, OriginalRCOMP2;
	unsigned char OriginalOCV1, OriginalOCV2;
	int StartingRCOMP; 
	int vcell,soc;

	StartingRCOMP=RCOMP0;

	 printk("[EXT_MAX17040] +init_fuel_gauge \n");
	
    // 1.Unlock Model Access
    printk("[EXT_MAX17040] 1.Unlock Model Access \n");
    fdata[0] = 0x3E;
    fdata[1] = 0x4A;
    fdata[2] = 0x57;
    fdata_size = 3;
  	 ret = i2c_master_send(gExt_bat_i2c_client, fdata, fdata_size);
      
    // 2.Read Original RCOMP and OCV Register
    printk("[EXT_MAX17040] 2.Read Original RCOMP and OCV Register \n");
    read_cmd[0] = 0x0C;
    fdata_size = 4;
    ret = i2c_master_send(gExt_bat_i2c_client, read_cmd, 1);
	 
	gExt_bat_i2c_client->flags = I2C_M_RD;
	ret = i2c_master_recv(gExt_bat_i2c_client, fdata, fdata_size);

    OriginalRCOMP1 = fdata[0];
    OriginalRCOMP2 = fdata[1];
    OriginalOCV1 = fdata[2];
    OriginalOCV2 = fdata[3];
    printk("[EXT_MAX17040] OriginalRCOMP1 = %d StartingRCOMP = %d OriginalRCOMP2 = %d OriginalOCV1 = %d OriginalOCV2 = %d \n", OriginalRCOMP1, StartingRCOMP, OriginalRCOMP2, OriginalOCV1, OriginalOCV2);

#if 1
    // Compare OriginalRCOMP with StartingRCOMP
    if (OriginalRCOMP1 == StartingRCOMP)
    {
        printk("[EXT_MAX17040] Already initialized \n");    
        
        // Lock Model Access
        printk("[EXT_MAX17040] Lock Model Access \n");
        fdata[0] = 0x3E;
        fdata[1] = 0x00;
        fdata[2] = 0x00;
        fdata_size = 3;   
        ret = i2c_master_send(gExt_bat_i2c_client, fdata, fdata_size);
		 
		if(check_quick_start() < 90){			
			printk("[EXT_MAX17040] do not quick star \n");
			goto init_done;				
		}else{
			printk("[EXT_MAX17040] ==>do quick start \n");
			goto quick_start;
		}

    }    
#endif

    // 3.Write OCV
    printk("[EXT_MAX17040] 3.Write OCV \n");
    fdata[0] = 0x0E;
    fdata[1] = OCVTest_High_Byte;
    fdata[2] = OCVTest_Low_Byte;
    fdata_size = 3;
    ret = i2c_master_send(gExt_bat_i2c_client, fdata, fdata_size);
	
    // 4.Write RCOMP
    printk("[EXT_MAX17040] 4.Write RCOMP \n");
    fdata[0] = 0x0C;
    fdata[1] = 0xFF;
    fdata[2] = 0x00;
    fdata_size = 3;
    ret = i2c_master_send(gExt_bat_i2c_client, fdata, fdata_size);
	
    // 5.Write Model
    printk("[EXT_MAX17040] 5.Write Model \n");
    ret = i2c_master_send(gExt_bat_i2c_client, model0, MDOEL_SIZE);
    ret = i2c_master_send(gExt_bat_i2c_client, model1, MDOEL_SIZE);
    ret = i2c_master_send(gExt_bat_i2c_client, model2, MDOEL_SIZE);
    ret = i2c_master_send(gExt_bat_i2c_client, model3, MDOEL_SIZE);
	
    // 6.Delay at least 150ms
    msleep(200);
    
    // 7.Write OCV
    printk("[EXT_MAX17040] 7.Write OCV \n");
    fdata[0] = 0x0E;
    fdata[1] = OCVTest_High_Byte;
    fdata[2] = OCVTest_Low_Byte;
    fdata_size = 3;
    ret = i2c_master_send(gExt_bat_i2c_client, fdata, fdata_size);

	 // 8.Delay between 150ms and 600ms
    msleep(300);
    

    // 9.Read SOC Register and Compare to expected result
    printk("[EXT_MAX17040] 9.Read SOC Register and Compare to expected result \n");
    read_cmd[0] = 0x04;
    fdata_size = 2;
    ret = i2c_master_send(gExt_bat_i2c_client, read_cmd, 1);
	 
	gExt_bat_i2c_client->flags = I2C_M_RD;
	ret = i2c_master_recv(gExt_bat_i2c_client, fdata, fdata_size);
	
    SOC1 = fdata[0];
    SOC2 = fdata[1];

    printk("[EXT_MAX17040] SOC1(%d) have to be %d~%d \n", SOC1, SOCCheckA, SOCCheckB);
        if (SOC1 >= SOCCheckA && SOC1 < SOCCheckB)
        printk("[EXT_MAX17040] Model was loaded successful\n");
    else
        printk("[EXT_MAX17040] Model was not loaded successful\n");


    // 10.Restore RCOMP and OCV
    printk("[EXT_MAX17040] 10.Restore RCOMP and OCV \n");
    fdata[0] = 0x0C;
    fdata[1] = OriginalRCOMP1;
    fdata[2] = OriginalRCOMP2;
    fdata[3] = OriginalOCV1;
    fdata[4] = OriginalOCV2;
    fdata_size = 5;
    ret = i2c_master_send(gExt_bat_i2c_client, fdata, fdata_size);
	    
    // 11.Lock Model Access
    printk("[EXT_MAX17040] 11.Lock Model Access \n");
    fdata[0] = 0x3E;
    fdata[1] = 0x00;
    fdata[2] = 0x00;
    fdata_size = 3;
    ret = i2c_master_send(gExt_bat_i2c_client, fdata, fdata_size);
	
    // Update RCOMP              
    printk("[EXT_MAX17040] Update RCOMP  \n");
    fdata[0] = 0x0C;
    fdata[1] = StartingRCOMP;
    fdata[2] = 0x00;
    fdata_size = 3;
    ret = i2c_master_send(gExt_bat_i2c_client, fdata, fdata_size);
	
 	msleep(100);

quick_start:
		do_quick_start();
		
init_done:
	printk("[EXT_MAX17040] -init_fuel_gauge \n");

	vcell = max17040_ext_get_vcell(gExt_bat_i2c_client);
	 soc = max17040_ext_get_soc(gExt_bat_i2c_client);

	printk("[EXT_MAX17040] SOC = %d% VCELL = %d mV\n", soc , vcell);

}

static int __devinit max17040_ext_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max17040_ext_chip *chip;
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, chip);

	chip->battery.name		= "ext_battery";
	chip->battery.type		= POWER_SUPPLY_TYPE_EXT_BATTERY;
	chip->battery.get_property	= max17040_ext_get_property;
	chip->battery.properties	= max17040_ext_battery_props;
	chip->battery.num_properties	= ARRAY_SIZE(max17040_ext_battery_props);
	chip->battery.external_power_changed = NULL;

#if 0
	chip->ac.name		= "ext_ac";
	chip->ac.type		= POWER_SUPPLY_TYPE_MAINS;
	chip->ac.get_property	= adapter_get_property;
	chip->ac.properties	= adapter_get_props;
	chip->ac.num_properties	= ARRAY_SIZE(adapter_get_props);
	chip->ac.external_power_changed = NULL;

	chip->usb.name		= "ext_usb";
	chip->usb.type		= POWER_SUPPLY_TYPE_USB;
	chip->usb.get_property	= usb_get_property;
	chip->usb.properties	= usb_get_props;
	chip->usb.num_properties	= ARRAY_SIZE(usb_get_props);
	chip->usb.external_power_changed = NULL;
#endif
	
	ret = power_supply_register(&client->dev, &chip->battery);
	if (ret)
		goto err_battery_failed;
#if 0
	ret = power_supply_register(&client->dev, &chip->ac);
	if(ret)
		goto err_ac_failed;

	ret = power_supply_register(&client->dev, &chip->usb);
	if(ret)
		goto err_usb_failed;
#endif

	//max17040_ext_get_version(client);

	ext_battery_create_file(iriver_class);

	gExt_bat_i2c_client = client;

	gExt_chip= chip;
	
	INIT_DELAYED_WORK_DEFERRABLE(&chip->work, max17040_ext_work);
	//schedule_delayed_work(&gExt_chip->work, MAX17040_DELAY);

	return 0;
	
err_usb_failed:
	//power_supply_unregister(&chip->ac);
err_ac_failed:
	power_supply_unregister(&chip->battery);
err_battery_failed:
	dev_err(&client->dev, "failed: power supply register\n");
	i2c_set_clientdata(client, NULL);
	kfree(chip);

	return ret;
}

int start_ext_battery(void)
{	
	printk("start_ext_battery\n");
	doc_bat_start = 1;
	init_fuel_gauge();
	max17040_ext_get_version(gExt_bat_i2c_client);
	schedule_delayed_work(&gExt_chip->work, 1);	
	return 0;
}
EXPORT_SYMBOL(start_ext_battery);
	
int stop_ext_battery(void)
{
	printk("stop_ext_battery\n");
	doc_bat_start = 0;
	cancel_delayed_work(&gExt_chip->work);
	return 0;
}
EXPORT_SYMBOL(stop_ext_battery);

static int __devexit max17040_ext_remove(struct i2c_client *client)
{
	struct max17040_ext_chip *chip = i2c_get_clientdata(client);

	//power_supply_unregister(&chip->usb);
	//power_supply_unregister(&chip->ac);
	power_supply_unregister(&chip->battery);
	cancel_delayed_work(&chip->work);
	i2c_set_clientdata(client, NULL);
	kfree(chip);
	return 0;
}

#if defined(CONFIG_PM)
static int max17040_ext_suspend(struct i2c_client *client,
                            pm_message_t state)
{
	struct max17040_ext_chip *chip = i2c_get_clientdata(client);

	//cancel_delayed_work(&chip->work);
    rdebug("%s\n", __func__);
    
    return 0;
}

static int max17040_ext_resume(struct i2c_client *client)
{
	struct max17040_ext_chip *chip = i2c_get_clientdata(client);

    /* [downer] A131204 */
	max17040_ext_get_online(chip->client);
   // power_supply_changed(&chip->usb);

//	schedule_delayed_work(&chip->work, MAX17040_DELAY);

//	wake_lock_timeout(&sleep_wakelock_delay, 5 * HZ);  
	
    rdebug("%s\n", __func__);
    
	return 0;
}

#else

#define max17040_ext_suspend NULL
#define max17040_ext_resume NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id max17040_ext_id[] = {
	{ "max17040_ext", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max17040_ext_id);

static struct i2c_driver max17040_ext_i2c_driver = {
	.driver	= {
		.name	= "max17040_ext",
	},
	.probe		= max17040_ext_probe,
	.remove		= __devexit_p(max17040_ext_remove),
	.suspend	= max17040_ext_suspend,
	.resume		= max17040_ext_resume,
	.id_table	= max17040_ext_id,
};


static int __init max17040_ext_init(void)
{
	return  i2c_add_driver(&max17040_ext_i2c_driver);
}
module_init(max17040_ext_init);

static void __exit max17040_ext_exit(void)
{
	i2c_del_driver(&max17040_ext_i2c_driver);
}
module_exit(max17040_ext_exit);

MODULE_AUTHOR("Youshin YIM<youshin.yim@iriver.com>");
MODULE_DESCRIPTION("Doking Battery MAX17040 Fuel Gauge");
MODULE_LICENSE("GPL");
