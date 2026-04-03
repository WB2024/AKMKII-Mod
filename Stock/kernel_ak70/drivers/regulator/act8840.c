/*
 * act8840.c  --  Voltage and current regulation for the ACT8840
 *
 * downer.kim@iriver.com
 *
 * This program is free software;
 *
 */
#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/regulator/act8840.h>

#include <mach/gpio.h>
#include <mach/ak_gpio_def.h>

#include <linux/earlysuspend.h>

#include <linux/delay.h>

//#define DEBUG 1

#if defined(DEBUG)
#define dbg(msg...) printk("ACT8840: " msg)
#else
#define dbg(msg...)
#endif

#define ACT8840_MAX_UV  4400000
#define ACT8840_MIN_UV   800000

#define NUM_OUPUT	ARRAY_SIZE(act8840_reg)

#define ACT8840_OUT_1_050V    0x12 // Audio play-3.8V: 313mA   291mA(USB remove)  / 3.7V:  327mA , 315mA,  296mA(USBA remove) 
#define ACT8840_OUT_1_075V    0x13 
#define ACT8840_OUT_1_100V    0x14 //332mA : sleep->wakeup-NG
#define ACT8840_OUT_1_125V    0x15 //338mA : sleep->wakeup-OK
#define ACT8840_OUT_1_150V    0x16 //sleep->wakeup-OK
#define ACT8840_OUT_1_175V 		0x17
#define ACT8840_OUT_1_20V 		0x18 // 336mA  /3.7V: 356mA 341mA  320mA(USB remove) 
#define ACT8840_OUT_1_25V 		0x19 
#define ACT8840_OUT_1_30V		0x1A
#define ACT8840_OUT_1_35V		0x1B
#define ACT8840_OUT_1_40V		0x1C
#define ACT8840_OUT_1_45V 		0x1D
#define ACT8840_OUT_1_50V 		0x1E
#define ACT8840_OUT_1_55V 		0x1F
#define ACT8840_OUT_1_60V 		0x20
#define ACT8840_OUT_1_65V 		0x21
#define ACT8840_OUT_1_70V  		0x22
#define ACT8840_OUT_1_75V  		0x23
#define ACT8840_OUT_1_80V  		0x24
#define ACT8840_OUT_2_70V		0x33
#define ACT8840_OUT_2_80V		0x34
#define ACT8840_OUT_2_90V		0x35
#define ACT8840_OUT_2_30V		0x36
#define ACT8840_OUT_2_31V		0x37
#define ACT8840_OUT_2_32V		0x38
#define ACT8840_OUT_3_30V		0x39	

/********************************************************************
	Define Types
********************************************************************/
struct act8840_voltage_t {
	int uV;
	u8  val;
};

struct act8840_data {
	struct i2c_client *client;
	unsigned int min_uV;
	unsigned int max_uV;
	struct regulator_dev *rdev[0];
};

static struct i2c_client *act8440_i2c_client = NULL;

static struct early_suspend pmic_early_suspend;

/********************************************************************
	I2C Command & Values
********************************************************************/
/* Registers */
#define ACT8840_DCDC1_REG                0x10 //REG1
#define ACT8840_DCDC2_REG                0x21 //REG2
#define ACT8840_DCDC3_REG                0x30 //REG3
#define ACT8840_DCDC4_REG                0x41 //REG4
#define ACT8840_LDO5_REG                 0x50	//REG5
#define ACT8840_LDO6_REG                 0x58 //REG6
#define ACT8840_LDO7_REG                 0x60 //REG7
#define ACT8840_LDO8_REG                 0x68 //REG8
#define ACT8840_LDO9_REG                 0x70 //REG9
#define ACT8840_LDO10_REG                 0x80 //REG10
#define ACT8840_LDO11_REG                 0x90 //REG11
#define ACT8840_LDO12_REG                 0xA0 //REG12


#define ACT8840_DCDC1_ONOFF_REG               0x12	//REG1
#define ACT8840_DCDC2_ONOFF_REG               0x22	//REG2
#define ACT8840_DCDC3_ONOFF_REG               0x32	//REG3
#define ACT8840_DCDC4_ONOFF_REG               0x42	//REG4
#define ACT8840_LDO5_ONOFF_REG                 0x51	//REG5
#define ACT8840_LDO6_ONOFF_REG                 0x59 //REG6
#define ACT8840_LDO7_ONOFF_REG                 0x61 //REG7
#define ACT8840_LDO8_ONOFF_REG                 0x69 //REG8
#define ACT8840_LDO9_ONOFF_REG                 0x71 //REG9
#define ACT8840_LDO10_ONOFF_REG                 0x81 //REG10
#define ACT8840_LDO11_ONOFF_REG                 0x91 //REG11
#define ACT8840_LDO12_ONOFF_REG                 0xA1 //REG12



static struct act8840_voltage_t dcdc_voltages[] = {                           
	{  800000, 0x08 },  {  825000, 0x09 },  {  850000, 0x0A }, {  875000, 0x0B }, 
	{  900000, 0x0C },  {  925000, 0x0D },  {  950000, 0x0E }, {  975000, 0x0F }, 
	{ 1000000, 0x10 }, { 1025000, 0x11 }, { 1050000, 0x12 }, { 1075000, 0x13 }, 
	{ 1100000, 0x14 }, { 1125000, 0x15 }, { 1150000, 0x16 }, { 1175000, 0x17 },	
	{ 1200000, 0x18 }, { 1250000, 0x19 }, { 1300000, 0x1A }, { 1350000, 0x1B }, 
	{ 1400000, 0x1C }, { 1450000, 0x1D }, { 1500000, 0x1E }, { 1550000, 0x1F },	
	{ 1600000, 0x20 }, { 1650000, 0x21 }, { 1700000, 0x22 }, { 1750000, 0x23 }, 
	{ 1800000, 0x24 }, { 1850000, 0x25 }, { 1900000, 0x26 }, { 1950000, 0x27 }, 
	{ 2000000, 0x28 }, { 2050000, 0x29 }, { 2100000, 0x2A }, { 2150000, 0x2B }, 
	{ 2200000, 0x2C }, { 2250000, 0x2D }, { 2300000, 0x2E }, { 2350000, 0x2F },	
	{ 2400000, 0x30 }, { 2500000, 0x31 }, { 2600000, 0x32 }, { 2700000, 0x33 }, 
	{ 2800000, 0x34 }, { 2900000, 0x35 }, { 3000000, 0x36 }, { 3100000, 0x37 }, 
	{ 3200000, 0x38 }, { 3300000, 0x39 }, { 3400000, 0x3A }, { 3500000, 0x3B }, 
	{ 3600000, 0x3C }, { 3700000, 0x3D }, { 3800000, 0x3E }, { 3900000, 0x3F }, 
};                                                                            


#define NUM_DCDC	ARRAY_SIZE(dcdc_voltages)
#define NUM_LDO 	ARRAY_SIZE(dcdc_voltages)

extern int get_wifi_state(void);

/********************************************************************
	Functions
********************************************************************/
static inline int act8840_read(struct i2c_client *client,
                                int reg, uint8_t *val)
{
        int ret;

        ret = i2c_smbus_read_byte_data(client, reg);
        if (ret < 0) {
                dev_err(&client->dev, "failed reading at 0x%02x\n", reg);
                return ret;
        }

        *val = (uint8_t)ret;
        return 0;
}

static inline int act8840_reads(struct i2c_client *client, int reg,
                                 int len, uint8_t *val)
{
        int ret;

        ret = i2c_smbus_read_i2c_block_data(client, reg, len, val);
        if (ret < 0) {
                dev_err(&client->dev, "failed reading from 0x%02x\n", reg);
                return ret;
        }
        return 0;
}


static inline int act8840_write(struct i2c_client *client,
                                 int reg, uint8_t val)
{
        int ret;

        ret = i2c_smbus_write_byte_data(client, reg, val);
        if (ret < 0) {
                dev_err(&client->dev, "failed writing 0x%02x to 0x%02x\n",
                                val, reg);
                return ret;
        }
        return 0;
}

static inline int act8840_writes(struct i2c_client *client, int reg,
                                  int len, uint8_t *val)
{
        int ret;

        ret = i2c_smbus_write_i2c_block_data(client, reg, len, val);
        if (ret < 0) {
                dev_err(&client->dev, "failed writings to 0x%02x\n", reg);
                return ret;
        }
        return 0;
}

/* [downer] A140115 for core voltage control in SDXC inserted */
extern int get_uhs_sdcard_inserted(void);
static int act8840_dcdc_set_voltage(struct regulator_dev *rdev, int min_uV, int max_uV, unsigned *sel)
{
	struct act8840_data* act8840 = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	u8 reg, value;
	int i, ret;

	dbg("act8840_dcdc_set_voltage ID: %d\n", id);
	
	switch (id) {
    case ACT8840_ID_DCDC1:
        reg = ACT8840_DCDC1_REG;
        break;
    case ACT8840_ID_DCDC2:
        reg = ACT8840_DCDC2_REG;
        
        if (get_uhs_sdcard_inserted() && (min_uV == 1000000))
            min_uV = 1050000;
        
        break;
    case ACT8840_ID_DCDC3:
        reg = ACT8840_DCDC3_REG;
        break;
    case ACT8840_ID_DCDC4:
        reg = ACT8840_DCDC4_REG;
        break;
    default:
        return -EINVAL;
	}

	for (i = 0; i < NUM_DCDC; i++) {
		if (dcdc_voltages[i].uV >= min_uV) {
			value = dcdc_voltages[i].val;
			break;
		}
	}

	if (i == NUM_DCDC)
		return -EINVAL;

	dbg("changing voltage reg:0x%x to %dmV(%d)\n", reg, min_uV / 1000, dcdc_voltages[i].uV / 1000);    

	ret = i2c_smbus_write_byte_data(act8840->client, reg, value);

	//dbg("0x%x\n", i2c_smbus_read_byte_data(act8840->client, reg));
	
	return ret;
}

static int act8840_dcdc_get_voltage(struct regulator_dev *rdev)
{
	struct act8840_data* act8840 = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	u8 reg;
	int ret, i, voltage = 0;
	
	dbg("act8840_dcdc_get_voltage-id: %d\n", (id));

	switch (id) {
    case ACT8840_ID_DCDC1:
        reg = ACT8840_DCDC1_REG;
        break;
    case ACT8840_ID_DCDC2:
        reg = ACT8840_DCDC2_REG;
        break;
    case ACT8840_ID_DCDC3:
        reg = ACT8840_DCDC3_REG;
        break;
    case ACT8840_ID_DCDC4:
        reg = ACT8840_DCDC4_REG;
        break;       
    default:
        return -EINVAL;
	}

	ret = i2c_smbus_read_byte_data(act8840->client, reg);

	if (ret < 0)
		return -EINVAL;

	for (i = 0; i < NUM_DCDC; i++) {
		if (dcdc_voltages[i].val == ret) {
			voltage = dcdc_voltages[i].uV;
			break;
		}
	}

	dbg("reg:0x%x value:%dmV\n", reg, voltage/1000);	
	
	return voltage;	
}

static struct regulator_ops act8840_dcdc_ops = {
	.set_voltage = act8840_dcdc_set_voltage,
	.get_voltage = act8840_dcdc_get_voltage,
};

int act8840_ldo_onoff(const char *ldoname,int onoff)
{
	struct regulator *dac_regulator;

	dbg(" %s,%s,%d\n",__FUNCTION__,ldoname,onoff);
	
	dac_regulator = regulator_get(NULL, ldoname);

	if (IS_ERR(dac_regulator)) {		
		printk(KERN_ERR "failed to get resource %s\n", ldoname);		
		return PTR_ERR(dac_regulator);	
	}

	dbg("%d\n",onoff);
	
	if(onoff)
		regulator_enable(dac_regulator);
	else
		regulator_disable(dac_regulator);

	return 0;
	
}
EXPORT_SYMBOL(act8840_ldo_onoff);

static int act8840_ldo_is_enabled(struct regulator_dev * rdev)
{
	struct act8840_data* act8840 = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	u8 reg, value;

	switch (id) {
		case ACT8840_ID_LDO5:
			reg = ACT8840_LDO5_ONOFF_REG; //REG5
			break;
		case ACT8840_ID_LDO6:
			reg = ACT8840_LDO6_ONOFF_REG; //REG6
			break;
		case ACT8840_ID_LDO7:
			reg = ACT8840_LDO7_ONOFF_REG;	//REG7
			break;
		case ACT8840_ID_LDO8:
			reg = ACT8840_LDO8_ONOFF_REG;  //REG8
			break;	
		case ACT8840_ID_LDO9:
			reg = ACT8840_LDO9_ONOFF_REG;  //REG9
			break;
		case ACT8840_ID_LDO10:
			reg = ACT8840_LDO10_ONOFF_REG;  //REG10
			break;
		case ACT8840_ID_LDO11:
			reg = ACT8840_LDO11_ONOFF_REG;  //REG11
			break;
		case ACT8840_ID_LDO12:
			reg = ACT8840_LDO12_ONOFF_REG;  //REG12
			break;	
		default:
			return -EINVAL;
	}
	
	value = i2c_smbus_read_byte_data(act8840->client, reg);

	dbg("+%s-id:%d reg:0x%x value:0x%x, 0x%x\n",__FUNCTION__, id, reg, value, value & (1 << 7));
	
	return 	value & (1 << 7);			
	
}

static int act8840_ldo_enable(struct regulator_dev * rdev)
{
	struct act8840_data* act8840 = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	u8 reg, value;
	int ret;

    printk("%s -- ID: %d\n", __func__, id);

	switch (id) {
		case ACT8840_ID_LDO5:
			reg = ACT8840_LDO5_ONOFF_REG; //REG5
			break;
		case ACT8840_ID_LDO6:
			reg = ACT8840_LDO6_ONOFF_REG; //REG6
			break;
		case ACT8840_ID_LDO7:
			reg = ACT8840_LDO7_ONOFF_REG;	//REG7
			break;
		case ACT8840_ID_LDO8:
			reg = ACT8840_LDO8_ONOFF_REG;  //REG8
			break;	
		case ACT8840_ID_LDO9:
			reg = ACT8840_LDO9_ONOFF_REG;  //REG9
			break;
		case ACT8840_ID_LDO10:
			reg = ACT8840_LDO10_ONOFF_REG;  //REG10
			break;
		case ACT8840_ID_LDO11:
			reg = ACT8840_LDO11_ONOFF_REG;  //REG11
			break;
		case ACT8840_ID_LDO12:
			reg = ACT8840_LDO12_ONOFF_REG;  //REG12
			break;	
		default:
			return -EINVAL;
	}
	
	value = i2c_smbus_read_byte_data(act8840->client, reg);

	dbg("+%s-reg:0x%x value:0x%x\n",__FUNCTION__, reg, value);
	
	value |= (1 << 7);		

	dbg("+%s-reg:0x%x value:0x%x\n",__FUNCTION__, reg, value);
	
	ret = i2c_smbus_write_byte_data(act8840->client, reg, value);

	return ret;
	
}

static int act8840_ldo_disable(struct regulator_dev * rdev)
{
	struct act8840_data* act8840 = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	u8 reg, value;
	int ret;

    printk("%s -- ID: %d\n", __func__, id);
    
	switch (id) {
		case ACT8840_ID_LDO5:
			reg = ACT8840_LDO5_ONOFF_REG; //REG5
			break;
		case ACT8840_ID_LDO6:
			reg = ACT8840_LDO6_ONOFF_REG; //REG6
			break;
		case ACT8840_ID_LDO7:
			reg = ACT8840_LDO7_ONOFF_REG;	//REG7
			break;
		case ACT8840_ID_LDO8:
			reg = ACT8840_LDO8_ONOFF_REG;  //REG8
			break;	
		case ACT8840_ID_LDO9:
			reg = ACT8840_LDO9_ONOFF_REG;  //REG9
			break;
		case ACT8840_ID_LDO10:
			reg = ACT8840_LDO10_ONOFF_REG;  //REG10
			break;
		case ACT8840_ID_LDO11:
			reg = ACT8840_LDO11_ONOFF_REG;  //REG11
			break;
		case ACT8840_ID_LDO12:
			reg = ACT8840_LDO12_ONOFF_REG;  //REG12
			break;	
		default:
			return -EINVAL;
	}
	
	value = i2c_smbus_read_byte_data(act8840->client, reg);

	dbg("+%s-reg:0x%x value:0x%x\n",__FUNCTION__, reg, value);

	value &= ~(1 << 7);		

	dbg("-%s-reg:0x%x value:0x%x\n",__FUNCTION__, reg, value);
	
	ret = i2c_smbus_write_byte_data(act8840->client, reg, value);

	return ret;
	
}

extern int get_lcd_state(void);
static int act8840_ldo_set_voltage(struct regulator_dev *rdev, int min_uV, int max_uV, unsigned *sel)
{
	struct act8840_data* act8840 = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	u8 reg, old_value, value;
	int i, ret;

#if 0//-jimi.test.0114
    printk("%s\n", __func__);
#endif
    
	switch (id) {
		case ACT8840_ID_LDO5:
			reg = ACT8840_LDO5_REG; //REG5
			break;
		case ACT8840_ID_LDO6:
			reg = ACT8840_LDO6_REG; //REG6
			break;
		case ACT8840_ID_LDO7:
			reg = ACT8840_LDO7_REG;	//REG7
			break;
		case ACT8840_ID_LDO8:
			reg = ACT8840_LDO8_REG;  //REG8
			break;
		case ACT8840_ID_LDO9:
			reg = ACT8840_LDO9_REG; //REG9
			break;
		case ACT8840_ID_LDO10:
			reg = ACT8840_LDO10_REG; //REG10
			break;
		case ACT8840_ID_LDO11:
			reg = ACT8840_LDO11_REG;	//REG11
            if (min_uV == 1800000) {      /* [downer] A140116 for SDXC insert in LCD off state */
                if (!get_lcd_state())
                    act8840_write(act8840->client, ACT8840_DCDC2_REG, ACT8840_OUT_1_050V);                    
            }
                
			break;
		case ACT8840_ID_LDO12:
			reg = ACT8840_LDO12_REG;  //REG12
			break;		
		default:
			return -EINVAL;
	}

	for (i = 0; i < NUM_LDO; i++) {
		if (dcdc_voltages[i].uV >= min_uV) {
			value = dcdc_voltages[i].val;
			break;
		}
	}

	if (i == NUM_LDO)
		return -EINVAL;

	old_value = i2c_smbus_read_byte_data(act8840->client, reg);

	dbg("old_value: 0x%x, new_value:0x%x\n", old_value, value);

	//dev_dbg(&client->dev, "changing voltage reg:0x%x to %dmV\n", reg, min_uV / 1000);
	dbg("changing voltage reg:0x%x to %dmV\n", reg, dcdc_voltages[i].uV / 1000);

    ret = i2c_smbus_write_byte_data(act8840->client, reg, value);
    
    mdelay(10);                /* [downer] A131210 */

    return ret;
}

static int act8840_ldo_get_voltage(struct regulator_dev *rdev)
{
	struct act8840_data* act8840 = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	u8 reg;	
	int ret, i, voltage = 0;

	dbg("act8840_ldo_get_voltage:%d\n",id);
	switch (id) {
		case ACT8840_ID_LDO5:
			reg = ACT8840_LDO5_REG;
			break;
		case ACT8840_ID_LDO6:
			reg = ACT8840_LDO6_REG;
			break;
		case ACT8840_ID_LDO7:
			reg = ACT8840_LDO7_REG;
			break;
		case ACT8840_ID_LDO8:
			reg = ACT8840_LDO8_REG;
			break;
		case ACT8840_ID_LDO9:
			reg = ACT8840_LDO9_REG;
			break;
		case ACT8840_ID_LDO10:
			reg = ACT8840_LDO10_REG;
			break;
		case ACT8840_ID_LDO11:
			reg = ACT8840_LDO11_REG;
			break;
		case ACT8840_ID_LDO12:
			reg = ACT8840_LDO12_REG;
			break;	
		default:
			return -EINVAL;
	}

	ret = i2c_smbus_read_byte_data(act8840->client, reg);

	if (ret < 0)
		return -EINVAL;

	for (i = 0; i < NUM_DCDC; i++) {
		if (dcdc_voltages[i].val == ret) {
			voltage = dcdc_voltages[i].uV;
			break;
		}
	}

	dbg("reg:0x%x value:%dmV\n", reg, voltage/1000);	
	
	return voltage;	
	
}

void act8840_power_off(void)
{	
	printk("%s\n",__FUNCTION__);

	gpio_request(GPIO_PMIC_HOLD, "GPIO_PMIC_HOLD");
	tcc_gpio_config(GPIO_PMIC_HOLD,GPIO_FN(0));
	gpio_direction_output(GPIO_PMIC_HOLD,0);
	//printk("GPIO_PMIC_HOLD-0\n");
	gpio_set_value(GPIO_PMIC_HOLD,0);	
}
EXPORT_SYMBOL(act8840_power_off);

static struct regulator_ops act8840_ldo_ops = {
	.is_enabled	= act8840_ldo_is_enabled,
	.enable	=  act8840_ldo_enable,	
	.disable	=  act8840_ldo_disable,
	.set_voltage = act8840_ldo_set_voltage,
	.get_voltage = act8840_ldo_get_voltage,
};

static struct regulator_desc act8840_reg[] = {
	{
		.name = "dcdc1",
		.id = ACT8840_ID_DCDC1,
		.ops = &act8840_dcdc_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = NUM_DCDC + 1,
		.owner = THIS_MODULE,
	},
	{
		.name = "dcdc2",
		.id = ACT8840_ID_DCDC2,
		.ops = &act8840_dcdc_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = NUM_DCDC + 1,
		.owner = THIS_MODULE,
	},
	{
		.name = "dcdc3",
		.id = ACT8840_ID_DCDC3,
		.ops = &act8840_dcdc_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = NUM_DCDC + 1,
		.owner = THIS_MODULE,
	},
	{
		.name = "dcdc4",
		.id = ACT8840_ID_DCDC4,
		.ops = &act8840_dcdc_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = NUM_DCDC + 1,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo5",
		.id = ACT8840_ID_LDO5,
		.ops = &act8840_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = NUM_LDO + 1,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo6",
		.id = ACT8840_ID_LDO6,
		.ops = &act8840_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = NUM_LDO + 1,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo7",
		.id = ACT8840_ID_LDO7,
		.ops = &act8840_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = NUM_LDO + 1,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo8",
		.id = ACT8840_ID_LDO8,
		.ops = &act8840_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = NUM_LDO + 1,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo9",
		.id = ACT8840_ID_LDO9,
		.ops = &act8840_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = NUM_LDO + 1,
		.owner = THIS_MODULE,
	},

	{
		.name = "ldo10",
		.id = ACT8840_ID_LDO10,
		.ops = &act8840_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = NUM_LDO + 1,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo11",
		.id = ACT8840_ID_LDO11,
		.ops = &act8840_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = NUM_LDO + 1,
		.owner = THIS_MODULE,
	},
	{
		.name = "ldo12",
		.id = ACT8840_ID_LDO12,
		.ops = &act8840_ldo_ops,
		.type = REGULATOR_VOLTAGE,
		.n_voltages = NUM_LDO + 1,
		.owner = THIS_MODULE,
	},

};


static void act8840_early_suspend(struct early_suspend *h)
{	

}

static void act8840_late_resume(struct early_suspend *h)
{

}

static int act8840_pmic_suspend(struct i2c_client *client, pm_message_t mesg)
{
	printk("%s\n",__FUNCTION__);

	if (get_wifi_state())
		return 0;

    act8840_write(client, ACT8840_DCDC3_REG, ACT8840_OUT_2_70V); /* [downer] A130912 I/O 2.7v down */
	return 0;
}

static int act8840_pmic_resume(struct i2c_client *client)
{
	printk("%s\n",__FUNCTION__);

	if (get_wifi_state())
		return 0;

    act8840_write(client, ACT8840_DCDC3_REG, ACT8840_OUT_2_80V); /* [downer] A130912 I/O 2.8v restore */
   //act8840_write(client, ACT8840_DCDC3_REG, ACT8840_OUT_3_30V); // 2015.02.10-ysyim :  test
	return 0;
}
static int act8840_pmic_probe(struct i2c_client *client, const struct i2c_device_id *i2c_id)
{
	struct regulator_dev **rdev;
	struct act8840_platform_data *pdata = client->dev.platform_data;
	struct act8840_data *act8840;
	int i, id, ret = -ENOMEM;
	
	printk("act8840_pmic_probe\n");
	
	act8840 = kzalloc(sizeof(struct act8840_data) +
			sizeof(struct regulator_dev *) * (NUM_OUPUT + 1),
			GFP_KERNEL);

	if (!act8840)
		goto out;

	act8840->client = client;
	act8440_i2c_client = client;
	
	act8840->min_uV = ACT8840_MIN_UV;
	act8840->max_uV = ACT8840_MAX_UV;

	rdev = act8840->rdev;

	i2c_set_clientdata(client, rdev);

	for (i = 0; i < pdata->num_subdevs && i <= NUM_OUPUT; i++) {
		id = pdata->subdevs[i].id;
		if (!pdata->subdevs[i].platform_data)
			continue;
		if (id >= ACT8840_ID_MAX) {
			dev_err(&client->dev, "invalid regulator id %d\n", id);
			goto err;
		}
		rdev[i] = regulator_register(&act8840_reg[id], &client->dev,
					     pdata->subdevs[i].platform_data,
					     act8840);
		if (IS_ERR(rdev[i])) {
			ret = PTR_ERR(rdev[i]);
			dev_err(&client->dev, "failed to register %s\n",
				act8840_reg[id].name);
			goto err;
		}
	}
	
	//2013.11.19 iriver-ysyim
	pmic_early_suspend.suspend = act8840_early_suspend;	
	pmic_early_suspend.resume = act8840_late_resume;	
	pmic_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;	
	register_early_suspend(&pmic_early_suspend);

	dev_info(&client->dev, "ACT8840 regulator driver loaded\n");

	return 0;

err:
	while (--i >= 0)
		regulator_unregister(rdev[i]);

	kfree(act8840);
out:
	act8440_i2c_client = NULL;	
	return ret;
}

static int act8840_pmic_remove(struct i2c_client *client)
{
	struct regulator_dev **rdev = i2c_get_clientdata(client);
	int i;

	for (i = 0; i <= NUM_OUPUT; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);
        
	kfree(rdev);
	i2c_set_clientdata(client, NULL);
	act8440_i2c_client = NULL;	
	
	return 0;
}

static const struct i2c_device_id act8840_id[] = {
	{ "act8840", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, act8840_id);

static struct i2c_driver act8840_pmic_driver = {
	.probe = act8840_pmic_probe,
	.remove = act8840_pmic_remove,
	.suspend	= act8840_pmic_suspend,	
	.resume	 = act8840_pmic_resume,
	.driver		= {
		.name	= "act8840",
	},
	.id_table	= act8840_id,
};

static int __init act8840_pmic_init(void)
{
	return i2c_add_driver(&act8840_pmic_driver);
}
subsys_initcall(act8840_pmic_init);

static void __exit act8840_pmic_exit(void)
{
	i2c_del_driver(&act8840_pmic_driver);
}
module_exit(act8840_pmic_exit);

/* Module information */
MODULE_DESCRIPTION("ACT8840 regulator driver");
MODULE_AUTHOR("iriver");
MODULE_LICENSE("GPL");
