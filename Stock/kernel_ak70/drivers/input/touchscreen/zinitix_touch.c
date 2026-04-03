/*
 *
 * Zinitix touch driver
 *
 * Copyright (C) 2012 Zinitix Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 */


#include <linux/module.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/ioctl.h>
#include <linux/earlysuspend.h>
#include <linux/string.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <linux/uaccess.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>

//dean add
#include <mach/reg_physical.h>
#include <mach/irqs.h>
#include <mach/structures_smu_pmu.h>
#include <mach/gpio.h>
#include <mach/io.h>

#include "zinitix_touch.h"
#if MULTI_PROTOCOL_TYPE_B
#include <linux/input/mt.h>
#endif

#include "zinitix_touch_zxt_firmware.h"

//check touch pressed
int g_touch_press = 0;

// dean touch 좌표 데이터 보실려면 1로 
#define	ZINITIX_DEBUG		0

#ifdef CONFIG_PM_SLEEP
//#define	ZCONFIG_PM_SLEEP	1
#endif

#define	TSP_NORMAL_EVENT_MSG	0
static int m_ts_debug_mode = ZINITIX_DEBUG;

#define	SYSTEM_MAX_X_RESOLUTION	800
#define	SYSTEM_MAX_Y_RESOLUTION	480


/* interrupt pin number*/
#define GPIO_TOUCH_PIN_NUM	GPIO_TOUCH_INT // TCC_GPB(11)//GPIO_TSP_INT
/* interrupt pin IRQ number */
//#define GPIO_TOUCH_IRQ		IRQ_EINT3
//#define GPIO_LDO_EN_NUM		S5PV210_GPH0(4)

#define TSP_TYPE_COUNT	1
const u8 *m_pFirmware [TSP_TYPE_COUNT] = {(u8*)m_firmware_data,};
u8 m_FirmwareIdx = 0;

#if !USE_THREADED_IRQ
static struct workqueue_struct *zinitix_workqueue;
#endif

static struct workqueue_struct *zinitix_tmr_workqueue;

#define zinitix_debug_msg(fmt, args...)	\
	if (m_ts_debug_mode)	\
		printk(KERN_INFO "zinitix : [%-18s:%5d]" fmt, \
		__func__, __LINE__, ## args);

#define zinitix_printk(fmt, args...)	\
		printk(KERN_INFO "zinitix : [%-18s:%5d]" fmt, \
		__func__, __LINE__, ## args);

struct _ts_zinitix_coord {
	u16	x;
	u16	y;
	u8	width;
	u8	sub_status;
#if (TOUCH_POINT_MODE == 2)
	u8	minor_width;
	u8	angle;
#endif
};

struct _ts_zinitix_point_info {
	u16	status;
#if (TOUCH_POINT_MODE == 1)
	u16	event_flag;
#else
	u8	finger_cnt;
	u8	time_stamp;
#endif	
	struct _ts_zinitix_coord coord[MAX_SUPPORTED_FINGER_NUM];
};

#define TOUCH_V_FLIP	0x01
#define TOUCH_H_FLIP	0x02
#define TOUCH_XY_SWAP	0x04

struct _ts_capa_info {
	u8 is_zmt200;
	u16 ic_revision;
	u16 firmware_version;
	u16 firmware_minor_version;
	u16 reg_data_version;
	u16 x_resolution;
	u16 y_resolution;
	u32 ic_fw_size;
	u32 MaxX;
	u32 MaxY;
	u32 MinX;
	u32 MinY;
	u32 Orientation;
	u8 gesture_support;
	u16 multi_fingers;
	u16 button_num;
	u16 ic_int_mask;
	u16 x_node_num;
	u16 y_node_num;
	u16 total_node_num;
	u16 hw_id;
	u16 afe_frequency;	
	u8	i2s_checksum;

};

enum _ts_work_proceedure {
	TS_NO_WORK = 0,
	TS_NORMAL_WORK,
	TS_ESD_TIMER_WORK,
	TS_IN_EALRY_SUSPEND,
	TS_IN_SUSPEND,
	TS_IN_RESUME,
	TS_IN_LATE_RESUME,
	TS_IN_UPGRADE,
	TS_REMOVE_WORK,
	TS_SET_MODE,
	TS_HW_CALIBRAION,
	TS_GET_RAW_DATA,	
};



struct zinitix_touch_dev {
	struct input_dev *input_dev;
	struct task_struct *task;
	wait_queue_head_t	wait;
#if !USE_THREADED_IRQ	
	struct work_struct	work;
#endif
	struct work_struct	tmr_work;
	struct i2c_client *client;
	struct semaphore update_lock;
	u32 i2c_dev_addr;
	struct _ts_capa_info cap_info;
	char	phys[32];

	struct _ts_zinitix_point_info touch_info;
	struct _ts_zinitix_point_info reported_touch_info;
	u16 icon_event_reg;	
	u16 event_type;
	u32 int_gpio_num;
	u32 irq;
	u32 gpio_ldo_en_num;
	u8 button[MAX_SUPPORTED_BUTTON_NUM];
	u8 work_proceedure;
	struct semaphore work_proceedure_lock;
	u8 use_esd_timer;

	u16 debug_reg[8];	// for debug

	bool in_esd_timer;
	struct timer_list esd_timeout_tmr;

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	struct semaphore	raw_data_lock;
	u16 touch_mode;
	s16 cur_data[MAX_TRAW_DATA_SZ];
	u8 update;

};


#define ZINITIX_DRIVER_NAME	"zinitix_touch"

static struct i2c_device_id zinitix_idtable[] = {
	{ZINITIX_DRIVER_NAME, 0},
	{ }
};

#if TOUCH_I2C_REGISTER_HERE
static struct i2c_board_info __initdata Zxt_i2c_info[] = 
	{ 
		I2C_BOARD_INFO(ZINITIX_DRIVER_NAME, 0x40>>1),
	};
#endif





/*<= you must set key button mapping*/
u32 BUTTON_MAPPING_KEY[MAX_SUPPORTED_BUTTON_NUM] = {
	KEY_SEARCH, KEY_BACK, KEY_HOME, KEY_MENU,};



/* define i2c sub functions*/
static inline s32 ts_retry_send(struct i2c_client *client, u8 *buf, u16 length)
{
	s32 ret;
	int i;
	for(i=0; i<3; i++) {
		ret = i2c_master_send(client , (u8 *)buf , length);
		if (ret >= 0)
			return ret;
		udelay(10);
	}
	return -1;
}
static inline s32 ts_read_data(struct i2c_client *client,
	u16 reg, u8 *values, u16 length)
{
	s32 ret;
	/* select register*/
	ret = ts_retry_send(client , (u8 *)&reg, 2);
	if (ret < 0)
		return ret;
	/* for setup tx transaction. */
	udelay(DELAY_FOR_TRANSCATION);
	ret = i2c_master_recv(client , values , length);
	if (ret < 0)
		return ret;
	udelay(DELAY_FOR_POST_TRANSCATION);
	return length;
}

static inline s32 ts_write_data(struct i2c_client *client,
	u16 reg, u8 *values, u16 length)
{
	s32 ret;
	u8 pkt[10];	//max packet
	pkt[0] = (reg)&0xff;	//reg addr
	pkt[1] = (reg >> 8)&0xff;
	memcpy((u8*)&pkt[2], values, length);
	ret = ts_retry_send(client , (u8 *)pkt, length+2);

	if (ret < 0)
		return ret;
	udelay(DELAY_FOR_POST_TRANSCATION);
	return length;
}

static inline s32 ts_write_reg(struct i2c_client *client, u16 reg, u16 value)
{
	if (ts_write_data(client, reg, (u8 *)&value, 2) < 0)
		return I2C_FAIL;
	return I2C_SUCCESS;
}

static inline s32 ts_write_cmd(struct i2c_client *client, u16 reg)
{
	s32 ret;
	ret = ts_retry_send(client , (u8 *)&reg, 2);
	if (ret < 0)
		return I2C_FAIL;
	udelay(DELAY_FOR_POST_TRANSCATION);
	return I2C_SUCCESS;
}

static inline s32 ts_read_raw_data(struct i2c_client *client,
		u16 reg, u8 *values, u16 length)
{
	s32 ret;
	/* select register */
	ret = ts_retry_send(client , (u8 *)&reg, 2);	
	if (ret < 0)
		return ret;
	/* for setup tx transaction. */
	udelay(200);
	ret = i2c_master_recv(client , values , length);
	if (ret < 0)
		return ret;
	udelay(DELAY_FOR_POST_TRANSCATION);
	return length;
}


static inline s32 ts_read_firmware_data(struct i2c_client *client,
	u16 addr, u8 *values, u16 length)
{
	s32 ret;
	/* select register*/
	ret = ts_retry_send(client , (u8 *)&addr, 2);	
	if (ret < 0)
		return ret;
	/* for setup tx transaction. */
	mdelay(1);

	ret = i2c_master_recv(client , values , length);
	if (ret < 0)
		return ret;
	udelay(DELAY_FOR_POST_TRANSCATION);
	return length;
}


static int zinitix_touch_probe(struct i2c_client *client,
	const struct i2c_device_id *i2c_id);
static int zinitix_touch_remove(struct i2c_client *client);
static bool ts_init_touch(struct zinitix_touch_dev *touch_dev);
static void zinitix_clear_report_data(struct zinitix_touch_dev *touch_dev);
static void zinitix_parsing_data(struct zinitix_touch_dev *touch_dev);

#ifdef ZCONFIG_PM_SLEEP
static int zinitix_resume(struct device *dev);
static int zinitix_suspend(struct device *dev);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void zinitix_early_suspend(struct early_suspend *h);
static void zinitix_late_resume(struct early_suspend *h);
#endif

static void ts_esd_timer_start(u16 sec, struct zinitix_touch_dev *touch_dev);
static void ts_esd_timer_stop(struct zinitix_touch_dev *touch_dev);
static void ts_esd_timeout_handler(unsigned long data);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static int ts_misc_fops_ioctl(struct inode *inode, struct file *filp,
	unsigned int cmd, unsigned long arg);
#else
static long ts_misc_fops_ioctl(struct file *filp,
	unsigned int cmd, unsigned long arg);
#endif
static int ts_misc_fops_open(struct inode *inode, struct file *filp);
static int ts_misc_fops_close(struct inode *inode, struct file *filp);

static const struct file_operations ts_misc_fops = {
	.owner = THIS_MODULE,
	.open = ts_misc_fops_open,
	.release = ts_misc_fops_close,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
	.ioctl = ts_misc_fops_ioctl,
#else
	.unlocked_ioctl = ts_misc_fops_ioctl,
#endif
};

static struct miscdevice touch_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "zinitix_touch_misc",
	.fops = &ts_misc_fops,
};

#define TOUCH_IOCTL_BASE	0xbc
#define TOUCH_IOCTL_GET_DEBUGMSG_STATE		_IOW(TOUCH_IOCTL_BASE, 0, int)
#define TOUCH_IOCTL_SET_DEBUGMSG_STATE		_IOW(TOUCH_IOCTL_BASE, 1, int)
#define TOUCH_IOCTL_GET_CHIP_REVISION		_IOW(TOUCH_IOCTL_BASE, 2, int)
#define TOUCH_IOCTL_GET_FW_VERSION		_IOW(TOUCH_IOCTL_BASE, 3, int)
#define TOUCH_IOCTL_GET_REG_DATA_VERSION	_IOW(TOUCH_IOCTL_BASE, 4, int)
#define TOUCH_IOCTL_VARIFY_UPGRADE_SIZE		_IOW(TOUCH_IOCTL_BASE, 5, int)
#define TOUCH_IOCTL_VARIFY_UPGRADE_DATA		_IOW(TOUCH_IOCTL_BASE, 6, int)
#define TOUCH_IOCTL_START_UPGRADE		_IOW(TOUCH_IOCTL_BASE, 7, int)
#define TOUCH_IOCTL_GET_X_NODE_NUM		_IOW(TOUCH_IOCTL_BASE, 8, int)
#define TOUCH_IOCTL_GET_Y_NODE_NUM		_IOW(TOUCH_IOCTL_BASE, 9, int)
#define TOUCH_IOCTL_GET_TOTAL_NODE_NUM		_IOW(TOUCH_IOCTL_BASE, 10, int)
#define TOUCH_IOCTL_SET_RAW_DATA_MODE		_IOW(TOUCH_IOCTL_BASE, 11, int)
#define TOUCH_IOCTL_GET_RAW_DATA		_IOW(TOUCH_IOCTL_BASE, 12, int)
#define TOUCH_IOCTL_GET_X_RESOLUTION		_IOW(TOUCH_IOCTL_BASE, 13, int)
#define TOUCH_IOCTL_GET_Y_RESOLUTION		_IOW(TOUCH_IOCTL_BASE, 14, int)
#define TOUCH_IOCTL_HW_CALIBRAION		_IOW(TOUCH_IOCTL_BASE, 15, int)
#define TOUCH_IOCTL_GET_REG			_IOW(TOUCH_IOCTL_BASE, 16, int)
#define TOUCH_IOCTL_SET_REG			_IOW(TOUCH_IOCTL_BASE, 17, int)
#define TOUCH_IOCTL_SEND_SAVE_STATUS		_IOW(TOUCH_IOCTL_BASE, 18, int)
#define TOUCH_IOCTL_DONOT_TOUCH_EVENT		_IOW(TOUCH_IOCTL_BASE, 19, int)


struct zinitix_touch_dev *misc_touch_dev;

static bool ts_get_raw_data(struct zinitix_touch_dev *touch_dev)
{
	u32 total_node = touch_dev->cap_info.total_node_num;
	u32 sz;
	
	if(down_trylock(&touch_dev->raw_data_lock)) {
		zinitix_printk("fail to occupy sema(%d)\r\n", touch_dev->work_proceedure);
		touch_dev->touch_info.status = 0;
		return true;
	}

	zinitix_debug_msg("read raw data\r\n");
	sz = total_node*2 + sizeof(struct _ts_zinitix_point_info);

	if (ts_read_raw_data(touch_dev->client,
		ZINITIX_RAWDATA_REG,
		(char *)touch_dev->cur_data, sz) < 0) {
		zinitix_printk("error : read zinitix tc raw data\n");
		up(&touch_dev->raw_data_lock);
		return false;
	}	

	touch_dev->update = 1;
	memcpy((u8 *)(&touch_dev->touch_info),
		(u8 *)&touch_dev->cur_data[total_node],
		sizeof(struct _ts_zinitix_point_info));	
	up(&touch_dev->raw_data_lock);
	return true;
}

static void ts_check_int_before_enirq(struct zinitix_touch_dev *touch_dev)
{
	int i;
	if (!gpio_get_value(touch_dev->int_gpio_num)) {
		zinitix_printk("interrupt occured before request irq+\r\n");
		/* read garbage data */
		for(i=0; i<3 ;i++) {
			ts_write_cmd(touch_dev->client, ZINITIX_CLEAR_INT_STATUS_CMD);
			udelay(10);
		}
		ts_write_cmd(touch_dev->client, ZINITIX_CLEAR_INT_STATUS_CMD);
	}
}

static void ts_enable_irq(struct zinitix_touch_dev *touch_dev){
	ts_check_int_before_enirq(touch_dev);
	enable_irq(touch_dev->irq);
}
static bool i2c_checksum(struct zinitix_touch_dev *touch_dev, s16 *pChecksum, u16 wlength)
{
	s16 checksum_result;
	s16 checksum_cur;
	int i;
	
	checksum_cur = 0;
	for(i=0; i<wlength; i++) {
		checksum_cur += (s16)pChecksum[i];
	}
	if (ts_read_data(touch_dev->client,
		ZINITIX_I2C_CHECKSUM_RESULT,
		(u8 *)(&checksum_result), 2) < 0) {
		printk(KERN_INFO "error read i2c checksum rsult.-\r\n");
		return false;
	}
	if(checksum_cur != checksum_result) {
		printk(KERN_INFO "checksum error : %d, %d\n", checksum_cur, checksum_result);
		return false;
	}
	return true;
}


static bool ts_read_coord(struct zinitix_touch_dev *touch_dev)
{
#if (TOUCH_POINT_MODE == 1)
	int i;
#endif

	zinitix_debug_msg("ts_read_coord+\r\n");

	if (gpio_get_value(touch_dev->int_gpio_num)) {
		/*interrupt pin is high, not valid data.*/
		touch_dev->touch_info.status = 0;
		zinitix_debug_msg("woops... interrupt pin is high\r\n");
		return true;
	}

	//for  Debugging Tool

	if (touch_dev->touch_mode != TOUCH_POINT_MODE) {		
		if (ts_get_raw_data(touch_dev) == false)
			return false;
		goto continue_check_point_data;
	}



#if (TOUCH_POINT_MODE == 1)
	memset(&touch_dev->touch_info,
		0x0, sizeof(struct _ts_zinitix_point_info));

	if(touch_dev->cap_info.i2s_checksum)
		if (ts_write_reg(touch_dev->client,
			ZINITIX_I2C_CHECKSUM_WCNT, 2)) != I2C_SUCCESS)
			return false;


	if (ts_read_data(touch_dev->client,
		ZINITIX_POINT_STATUS_REG,
		(u8 *)(&touch_dev->touch_info), 4) < 0) {
		zinitix_debug_msg("error read point info using i2c.-\r\n");
		return false;
	}
	zinitix_debug_msg("status reg = 0x%x , event_flag = 0x%04x\r\n",
		touch_dev->touch_info.status, touch_dev->touch_info.event_flag);

	if(touch_dev->cap_info.i2s_checksum)
		if(i2c_checksum (touch_dev, (s16 *)(&touch_dev->touch_info), 2) == false)
			return false;

	if(touch_dev->touch_info.event_flag == 0 || touch_dev->touch_info.status == 0) 
		goto continue_check_point_data;

	if(touch_dev->cap_info.i2s_checksum)
		if (ts_write_reg(touch_dev->client,
			ZINITIX_I2C_CHECKSUM_WCNT, sizeof(struct _ts_zinitix_coord)/2)) != I2C_SUCCESS)
			return false;

	for (i = 0; i < touch_dev->cap_info.multi_fingers; i++) {
		if (zinitix_bit_test(touch_dev->touch_info.event_flag, i)) {
			udelay(20);
			if (ts_read_data(touch_dev->client,
				ZINITIX_POINT_STATUS_REG+2+(i*4),
				(u8 *)(&touch_dev->touch_info.coord[i]),
				sizeof(struct _ts_zinitix_coord)) < 0) {
				zinitix_debug_msg("error read point info\n");
				return false;
			}			
			if(touch_dev->cap_info.i2s_checksum)
				if(i2c_checksum (touch_dev, (s16 *)(&touch_dev->touch_info.coord[i]), sizeof(struct _ts_zinitix_coord)/2) == false)
					return false;
		}
	}

#else
	if(touch_dev->cap_info.i2s_checksum)
		if (ts_write_reg(touch_dev->client,
				ZINITIX_I2C_CHECKSUM_WCNT, 
				(sizeof(struct _ts_zinitix_point_info)/2)) != I2C_SUCCESS){
		zinitix_debug_msg("error write checksum wcnt.-\n");
		return false;
	}
		

	if (ts_read_data(touch_dev->client,
		ZINITIX_POINT_STATUS_REG,
		(u8 *)(&touch_dev->touch_info),
		sizeof(struct _ts_zinitix_point_info)) < 0) {
		zinitix_debug_msg("error read point info using i2c.-\n");
		return false;
	}

	if(touch_dev->cap_info.i2s_checksum)
		if(i2c_checksum (touch_dev, (s16 *)(&touch_dev->touch_info), sizeof(struct _ts_zinitix_point_info)/2) == false)
			return false;
	
#endif	

continue_check_point_data:


	if (touch_dev->touch_info.status == 0x0) {
		zinitix_debug_msg("periodical esd repeated int occured\r\n");
		if(ts_write_cmd(touch_dev->client, ZINITIX_CLEAR_INT_STATUS_CMD) != I2C_SUCCESS)
			return false;
		udelay(DELAY_FOR_SIGNAL_DELAY);		
		return true;
	}

	// error 
	if (zinitix_bit_test(touch_dev->touch_info.status, BIT_MUST_ZERO)) {
		zinitix_printk("must zero bit must cleared.(%04x)\r\n", touch_dev->touch_info.status);
		udelay(DELAY_FOR_SIGNAL_DELAY);		
		return false;
	}
	

	if (zinitix_bit_test(touch_dev->touch_info.status, BIT_ICON_EVENT)) {
		udelay(20);
		if (ts_read_data(touch_dev->client,
			ZINITIX_ICON_STATUS_REG,
			(u8 *)(&touch_dev->icon_event_reg), 2) < 0) {
			zinitix_printk("error read icon info using i2c.\n");
			return false;
		}
	}
	
	if(ts_write_cmd(touch_dev->client, ZINITIX_CLEAR_INT_STATUS_CMD) != I2C_SUCCESS)
		return false;
	udelay(DELAY_FOR_SIGNAL_DELAY);
	zinitix_debug_msg("ts_read_coord-\r\n");	
	return true;

}


static irqreturn_t ts_int_handler(int irq, void *dev)
{

	struct zinitix_touch_dev *touch_dev = (struct zinitix_touch_dev *)dev;
	/* zinitix_debug_msg("interrupt occured +\r\n"); */
	if(touch_dev == NULL)	return IRQ_HANDLED;
	/* remove pending interrupt */
	if (gpio_get_value(touch_dev->int_gpio_num)) {
		zinitix_debug_msg("invalid interrupt occured +\r\n");
		return IRQ_HANDLED;
	}
#if USE_THREADED_IRQ	
	return IRQ_WAKE_THREAD;
#else	
	disable_irq_nosync(irq);
	queue_work(zinitix_workqueue, &touch_dev->work);
	return IRQ_HANDLED;
#endif	
}


static bool ts_power_sequence(struct zinitix_touch_dev *touch_dev)
{
	int retry = 0;
	u16 chip_code;
	
retry_power_sequence:	
	if (ts_write_reg(touch_dev->client, 0xc000, 0x0001) != I2C_SUCCESS){
		zinitix_printk("power sequence error (vendor cmd enable)\n");
		goto fail_power_sequence;
	}
	udelay(10);
	if (ts_read_data(touch_dev->client, 0xcc00, (u8 *)&chip_code, 2) < 0) {
		zinitix_printk("fail to read chip code\n");
		goto fail_power_sequence;
	}
	zinitix_printk("chip code = 0x%x\n", chip_code);
	if(chip_code == 0xf400)
		touch_dev->cap_info.is_zmt200 = 0;
	else
		touch_dev->cap_info.is_zmt200 = 1;
	
	udelay(10);	
	if (ts_write_cmd(touch_dev->client, 0xc004) != I2C_SUCCESS){
		zinitix_printk("power sequence error (intn clear)\n");
		goto fail_power_sequence;
	}
	udelay(10);
	if (ts_write_reg(touch_dev->client, 0xc002, 0x0001) != I2C_SUCCESS){
		zinitix_printk("power sequence error (nvm init)\n");
		goto fail_power_sequence;
	}
	mdelay(2);
	if (ts_write_reg(touch_dev->client, 0xc001, 0x0001) != I2C_SUCCESS){
		zinitix_printk("power sequence error (program start)\n");
		goto fail_power_sequence;
	}
	msleep(FIRMWARE_ON_DELAY);	//wait for checksum cal
	return true;
	
fail_power_sequence:
	if(retry++ < 3) {
		msleep(CHIP_ON_DELAY);
		zinitix_printk("retry = %d\n", retry);		
		goto retry_power_sequence;
	}
	return false;
}

static bool ts_power_control(struct zinitix_touch_dev *touch_dev, u8 ctl)
{

    volatile PGPIO pGPIO = (volatile PGPIO)tcc_p2v(HwGPIO_BASE);		//dean add


	if (ctl == POWER_OFF) {
		//dean change    gpio_set_value(touch_dev->gpio_ldo_en_num, 0);
		pGPIO->GPFDAT.bREG.GP01 = 0;

		
		msleep(CHIP_OFF_DELAY);
	} else if (ctl == POWER_ON_SEQUENCE) {
		//dean change    gpio_set_value(touch_dev->gpio_ldo_en_num, 1);
		pGPIO->GPFDAT.bREG.GP01 = 1;

		msleep(CHIP_ON_DELAY);		
		//zxt power on sequence
		return ts_power_sequence(touch_dev);
	}	
	else if (ctl == POWER_ON) {
		//dean change    gpio_set_value(touch_dev->gpio_ldo_en_num, 1);
		pGPIO->GPFDAT.bREG.GP01 = 1;

		msleep(CHIP_ON_DELAY);	
	}	
	
	return true;
}

static bool ts_mini_init_touch(struct zinitix_touch_dev *touch_dev)
{
	int i;
#if USE_CHECKSUM
	u16 chip_check_sum;
#endif	

	if (touch_dev == NULL) {
		zinitix_printk("error (touch_dev == NULL?)\r\n");
		return false;
	}

	zinitix_debug_msg("check checksum\r\n");

#if USE_CHECKSUM
	if (ts_read_data(touch_dev->client,
		ZINITIX_CHECKSUM_RESULT, (u8 *)&chip_check_sum, 2) < 0) 
		goto fail_mini_init;
	if( chip_check_sum != 0x55aa ) {
		zinitix_printk("firmware checksum error(0x%04x)\r\n", chip_check_sum);
		goto fail_mini_init;
	}		
#endif

	if (ts_write_cmd(touch_dev->client,
		ZINITIX_SWRESET_CMD) != I2C_SUCCESS) {
		zinitix_printk("fail to write reset command\r\n");
		goto fail_mini_init;
	}


	/* initialize */
	if (ts_write_reg(touch_dev->client,
		ZINITIX_X_RESOLUTION,
		(u16)(touch_dev->cap_info.x_resolution)) != I2C_SUCCESS)
		goto fail_mini_init;

	if (ts_write_reg(touch_dev->client,
		ZINITIX_Y_RESOLUTION,
		(u16)( touch_dev->cap_info.y_resolution)) != I2C_SUCCESS)
		goto fail_mini_init;

	zinitix_debug_msg("touch max x = %d\r\n", touch_dev->cap_info.x_resolution);
	zinitix_debug_msg("touch max y = %d\r\n", touch_dev->cap_info.y_resolution);

	if (ts_write_reg(touch_dev->client,
		ZINITIX_BUTTON_SUPPORTED_NUM,
		(u16)touch_dev->cap_info.button_num) != I2C_SUCCESS)
		goto fail_mini_init;

	if (ts_write_reg(touch_dev->client,
		ZINITIX_SUPPORTED_FINGER_NUM,
		(u16)MAX_SUPPORTED_FINGER_NUM) != I2C_SUCCESS)
		goto fail_mini_init;
	
	if (ts_write_reg(touch_dev->client,
		ZINITIX_INITIAL_TOUCH_MODE, TOUCH_POINT_MODE) != I2C_SUCCESS)
		goto fail_mini_init;


	if (ts_write_reg(touch_dev->client,
		ZINITIX_TOUCH_MODE, touch_dev->touch_mode) != I2C_SUCCESS)
		goto fail_mini_init;


	/* soft calibration */
	if (ts_write_cmd(touch_dev->client,
		ZINITIX_CALIBRATE_CMD) != I2C_SUCCESS)
		goto fail_mini_init;
	

	if (touch_dev->touch_mode != TOUCH_POINT_MODE) {
		if (ts_write_reg(touch_dev->client,
			ZINITIX_DELAY_RAW_FOR_HOST,
			RAWDATA_DELAY_FOR_HOST) != I2C_SUCCESS){			
			zinitix_printk("Fail to set ZINITIX_DELAY_RAW_FOR_HOST.\r\n");
			goto fail_mini_init;
		}
	}
	if (ts_write_reg(touch_dev->client,
		ZINITIX_PERIODICAL_INTERRUPT_INTERVAL,
		ZINITIX_SCAN_RATE_HZ
		*ZINITIX_ESD_TIMER_INTERVAL) != I2C_SUCCESS)
		goto fail_mini_init;

	if (touch_dev->use_esd_timer) {
		ts_esd_timer_start(ZINITIX_CHECK_ESD_TIMER, touch_dev);
		zinitix_debug_msg("esd timer start\r\n");
	}
	
	if (ts_write_reg(touch_dev->client,
		ZINITIX_INT_ENABLE_FLAG,
		touch_dev->cap_info.ic_int_mask) != I2C_SUCCESS)
		goto fail_mini_init;

	zinitix_debug_msg("successfully mini initialized\r\n");
	return true;

fail_mini_init:

	zinitix_printk("early mini init fail\n");
	ts_power_control(touch_dev, POWER_OFF);	
	ts_power_control(touch_dev, POWER_ON_SEQUENCE);

	if(ts_init_touch(touch_dev) == false) {
		zinitix_debug_msg("fail initialized\r\n");
		return false;
	}

	if (touch_dev->use_esd_timer) {
		ts_esd_timer_start(ZINITIX_CHECK_ESD_TIMER, touch_dev);
		zinitix_debug_msg("esd timer start\r\n");
	}
	return true;
}


static void zinitix_touch_tmr_work(struct work_struct *work)
{
	struct zinitix_touch_dev *touch_dev =
		container_of(work, struct zinitix_touch_dev, tmr_work);

	zinitix_printk("tmr queue work ++\r\n");
	if (touch_dev == NULL) {
		zinitix_printk("touch dev == NULL ?\r\n");
		goto fail_time_out_init;
	}
	if(down_trylock(&touch_dev->work_proceedure_lock)) {
		zinitix_printk("fail to occupy sema(%d)\r\n", touch_dev->work_proceedure);
		ts_esd_timer_start(ZINITIX_CHECK_ESD_TIMER, touch_dev);
		return;
	}
	if (touch_dev->work_proceedure != TS_NO_WORK) {
		zinitix_printk("other process occupied (%d)\r\n",
			touch_dev->work_proceedure);
		up(&touch_dev->work_proceedure_lock);
		return;
	}
	touch_dev->work_proceedure = TS_ESD_TIMER_WORK;

	disable_irq(touch_dev->irq);
	zinitix_printk("error. timeout occured. maybe ts device dead. so reset & reinit.\r\n");
	ts_power_control(touch_dev, POWER_OFF);
	ts_power_control(touch_dev, POWER_ON_SEQUENCE);

	zinitix_debug_msg("clear all reported points\r\n");
	zinitix_clear_report_data(touch_dev);
	if (ts_mini_init_touch(touch_dev) == false)
		goto fail_time_out_init;

	touch_dev->work_proceedure = TS_NO_WORK;
	ts_enable_irq(touch_dev);
	up(&touch_dev->work_proceedure_lock);
	zinitix_printk("tmr queue work ----\r\n");
	return;
fail_time_out_init:
	zinitix_printk("tmr work : restart error\r\n");
	ts_esd_timer_start(ZINITIX_CHECK_ESD_TIMER, touch_dev);
	touch_dev->work_proceedure = TS_NO_WORK;
	ts_enable_irq(touch_dev);
	up(&touch_dev->work_proceedure_lock);
}

static void ts_esd_timer_start(u16 sec, struct zinitix_touch_dev *touch_dev)
{
	if(touch_dev == NULL)	return;
	if (touch_dev->esd_timeout_tmr.data != NULL)
		del_timer(&touch_dev->esd_timeout_tmr);

	init_timer(&touch_dev->esd_timeout_tmr);
	touch_dev->esd_timeout_tmr.data = (unsigned long)(touch_dev);
	touch_dev->esd_timeout_tmr.function = ts_esd_timeout_handler;
	touch_dev->esd_timeout_tmr.expires = jiffies + HZ*sec;
	add_timer(&touch_dev->esd_timeout_tmr);
}

static void ts_esd_timer_stop(struct zinitix_touch_dev *touch_dev)
{
	if(touch_dev == NULL)	return;
	if (touch_dev->esd_timeout_tmr.data != NULL)
		del_timer(&touch_dev->esd_timeout_tmr);
	touch_dev->esd_timeout_tmr.data = NULL;
}

static void ts_esd_timeout_handler(unsigned long data)
{
	struct zinitix_touch_dev *touch_dev = (struct zinitix_touch_dev *)data;
	if(touch_dev == NULL)	return;
	touch_dev->esd_timeout_tmr.data = NULL;
	queue_work(zinitix_tmr_workqueue, &touch_dev->tmr_work);
}

static void ts_select_type_hw(struct zinitix_touch_dev *touch_dev) {
	int i;
	u16 newHWID;

	for(i = 0; i< TSP_TYPE_COUNT; i++) {		
		if(touch_dev->cap_info.is_zmt200 == 0)
			newHWID = (u16) (m_pFirmware[i][0x6b12] | (m_pFirmware[i][0x6b13]<<8));
		else		
			newHWID = (u16) (m_pFirmware[i][0x57d2] | (m_pFirmware[i][0x57d3]<<8));
		if(touch_dev->cap_info.hw_id == newHWID)
			break;
	}
	
	m_FirmwareIdx = i;
	if(i == TSP_TYPE_COUNT)
		m_FirmwareIdx = 0;

	printk(KERN_INFO "touch tsp type = %d, %d\n", i, m_FirmwareIdx);

}


#if TOUCH_ONESHOT_UPGRADE
static bool ts_check_need_upgrade(struct zinitix_touch_dev *touch_dev, 
	u16 curVersion, u16 curMinorVersion, u16 curRegVersion, u16 curHWID)
{
	u16	newVersion;
	u16	newMinorVersion;	
	u16	newRegVersion;
	u16	newChipCode;
	u16	newHWID;
	u8 *firmware_data;

	ts_select_type_hw(touch_dev);
	firmware_data = (u8*)m_pFirmware[m_FirmwareIdx];

	curVersion = curVersion&0xff;
	newVersion = (u16) (firmware_data[52] | (firmware_data[53]<<8));
	newVersion = newVersion&0xff;
	newMinorVersion = (u16) (firmware_data[56] | (firmware_data[57]<<8));
	newRegVersion = (u16) (firmware_data[60] | (firmware_data[61]<<8));
	newChipCode = (u16) (firmware_data[64] | (firmware_data[65]<<8));
	if(touch_dev->cap_info.is_zmt200 == 0)
		newHWID = (u16) (firmware_data[0x6b12] | (firmware_data[0x6b13]<<8));
	else		
		newHWID = (u16) (firmware_data[0x57d2] | (firmware_data[0x57d3]<<8));
	
#if CHECK_HWID
	zinitix_printk("cur HW_ID = 0x%x, new HW_ID = 0x%x\n",
		curHWID, newHWID);
	if (curHWID != newHWID)
		return false;
#endif

	zinitix_printk("cur version = 0x%x, new version = 0x%x\n",
		curVersion, newVersion);
	if (curVersion < newVersion)
		return false;
	
	zinitix_printk("cur minor version = 0x%x, new minor version = 0x%x\n",
			curMinorVersion, newMinorVersion);
	if (curMinorVersion < newMinorVersion)
		return true;
	else if (curMinorVersion > newMinorVersion)
		return false;

	zinitix_printk("cur reg data version = 0x%x, new reg data version = 0x%x\n",
			curRegVersion, newRegVersion);
	if (curRegVersion < newRegVersion)
		return true;
	
	return false;
}
#endif

#define TC_SECTOR_SZ		8

static void ts_check_hwid_in_fatal_state(struct zinitix_touch_dev *touch_dev)
{
	u16 flash_addr;
	u8 check_data[80];
	int i;
	u16 chip_code;
	int retry = 0;
	u16 hw_id0, hw_id1_1, hw_id1_2;

	touch_dev->cap_info.hw_id = 0;

retry_check_hwid:
	
	ts_power_control(touch_dev, POWER_OFF);
	ts_power_control(touch_dev, POWER_ON);
	
	if (ts_write_reg(touch_dev->client, 0xc000, 0x0001) != I2C_SUCCESS){
		zinitix_printk("power sequence error (vendor cmd enable)\n");
		goto fail_check_hwid;
	}
	udelay(10);
	if (ts_read_data(touch_dev->client, 0xcc00, (u8 *)&chip_code, 2) < 0) {
		zinitix_printk("fail to read chip code\n");
		goto fail_check_hwid;
	}
	zinitix_printk("chip code = 0x%x\n", chip_code);
	if(chip_code == 0xf400)
		touch_dev->cap_info.is_zmt200 = 0;
	else
		touch_dev->cap_info.is_zmt200 = 1;
				
	udelay(10); 
	if (ts_write_cmd(touch_dev->client, 0xc004) != I2C_SUCCESS){
		zinitix_printk("power sequence error (intn clear)\n");
		goto fail_check_hwid;
	}
	udelay(10);
	if (ts_write_reg(touch_dev->client, 0xc002, 0x0001) != I2C_SUCCESS){
		zinitix_printk("power sequence error (nvm init)\n");
		goto fail_check_hwid;
	}
	mdelay(5);
	zinitix_printk(KERN_INFO "init flash\n");
	if (ts_write_reg(touch_dev->client, 0xc003, 0x0000) != I2C_SUCCESS){
		zinitix_printk("fail to write nvm vpp on\n");
		goto fail_check_hwid;
	}

	if (ts_write_reg(touch_dev->client, 0xc104, 0x0000) != I2C_SUCCESS){
		zinitix_printk("fail to write nvm wp disable\n");
		goto fail_check_hwid;
	}

	zinitix_printk(KERN_INFO "init flash\n");
	if (ts_write_cmd(touch_dev->client, ZINITIX_INIT_FLASH) != I2C_SUCCESS) {
		zinitix_printk(KERN_INFO "failed to init flash\n");
		goto fail_check_hwid;
	}
	
	zinitix_printk(KERN_INFO "read firmware data\n");
	for (i = 0; i < 80; i+=TC_SECTOR_SZ) {
		if (ts_read_firmware_data(touch_dev->client,
			ZINITIX_READ_FLASH,
			(u8*)&check_data[i], TC_SECTOR_SZ) < 0) {
			zinitix_printk(KERN_INFO "error : read zinitix tc firmare\n");
			goto fail_check_hwid;
		}
	}

	hw_id0 = check_data[48] + check_data[49]*256;		
	hw_id1_1 = check_data[70];
	hw_id1_2 = check_data[71];	
	
	zinitix_printk(KERN_INFO "eeprom hw id = %d, %d, %d\n", hw_id0, hw_id1_1, hw_id1_2);
	
	if(hw_id1_1 == hw_id1_2 && hw_id0 != hw_id1_1)
		touch_dev->cap_info.hw_id = hw_id1_1;
	else
		touch_dev->cap_info.hw_id = hw_id0; 	
	
	zinitix_printk(KERN_INFO "hw id = %d\n", touch_dev->cap_info.hw_id);	

	mdelay(5);		
	return;

fail_check_hwid:
	if(retry++ < 3) {
		zinitix_printk(KERN_INFO "fail to check hw id(%d)\n");
		mdelay(5);
		goto retry_check_hwid;
	}
	zinitix_printk(KERN_INFO "fail to check hw id(%d)\n");
}

static u8 ts_upgrade_firmware(struct zinitix_touch_dev *touch_dev,
	const u8 *firmware_data, u32 size)
{
	u16 flash_addr;
	u8 *verify_data;
	int retry_cnt = 0;
	int i;
	int page_sz = 64;
	u16 chip_code;

	
	verify_data = kzalloc(size, GFP_KERNEL);
	if (verify_data == NULL) {
		zinitix_printk(KERN_ERR "cannot alloc verify buffer\n");
		return false;
	}

retry_upgrade:
	
	ts_power_control(touch_dev, POWER_OFF);
	ts_power_control(touch_dev, POWER_ON);

	if (ts_write_reg(touch_dev->client, 0xc000, 0x0001) != I2C_SUCCESS){
		zinitix_printk("power sequence error (vendor cmd enable)\n");
		goto fail_upgrade;
	}
	udelay(10);
	if (ts_read_data(touch_dev->client, 0xcc00, (u8 *)&chip_code, 2) < 0) {
		zinitix_printk("fail to read chip code\n");
		goto fail_upgrade;
	}
	zinitix_printk("chip code = 0x%x\n", chip_code);
	if(chip_code == 0xf400) {
		touch_dev->cap_info.is_zmt200 = 0;
		page_sz = 128;
	} else {
		touch_dev->cap_info.is_zmt200 = 1;
		page_sz = 64;		
	}
		
	udelay(10);	
	if (ts_write_cmd(touch_dev->client, 0xc004) != I2C_SUCCESS){
		zinitix_printk("power sequence error (intn clear)\n");
		goto fail_upgrade;
	}
	udelay(10);
	if (ts_write_reg(touch_dev->client, 0xc002, 0x0001) != I2C_SUCCESS){
		zinitix_printk("power sequence error (nvm init)\n");
		goto fail_upgrade;
	}

	mdelay(5); 

	zinitix_printk(KERN_INFO "init flash\n");
	if (ts_write_reg(touch_dev->client, 0xc003, 0x0001) != I2C_SUCCESS){
		zinitix_printk("fail to write nvm vpp on\n");
		goto fail_upgrade;
	}

	if (ts_write_reg(touch_dev->client, 0xc104, 0x0001) != I2C_SUCCESS){
		zinitix_printk("fail to write nvm wp disable\n");
		goto fail_upgrade;
	}
	
	if (ts_write_cmd(touch_dev->client, ZINITIX_INIT_FLASH) != I2C_SUCCESS) {
		zinitix_printk(KERN_INFO "failed to init flash\n");
		goto fail_upgrade;
	}
	
	zinitix_printk(KERN_INFO "writing firmware data\n");
	for (flash_addr = 0; flash_addr < size; ) {
		for (i = 0; i < page_sz/TC_SECTOR_SZ; i++) {
			//zinitix_debug_msg("write :addr=%04x, len=%d\n",	flash_addr, TC_SECTOR_SZ);
			if (ts_write_data(touch_dev->client,
				ZINITIX_WRITE_FLASH,
				(u8 *)&firmware_data[flash_addr],TC_SECTOR_SZ) < 0) {
				zinitix_printk(KERN_INFO"error : write zinitix tc firmare\n");
				goto fail_upgrade;
			}
			flash_addr += TC_SECTOR_SZ;
			udelay(100);
		}
		if(touch_dev->cap_info.is_zmt200 == 0)
			mdelay(30);	//for fuzing delay
		else
			mdelay(15);
	}

	if (ts_write_reg(touch_dev->client, 0xc003, 0x0000) != I2C_SUCCESS){
		zinitix_printk("nvm write vpp off\n");
		goto fail_upgrade;
	}

	if (ts_write_reg(touch_dev->client, 0xc104, 0x0000) != I2C_SUCCESS){
		zinitix_printk("nvm wp enable\n");
		goto fail_upgrade;
	}

	zinitix_printk(KERN_INFO "init flash\n");
	if (ts_write_cmd(touch_dev->client, ZINITIX_INIT_FLASH) != I2C_SUCCESS) {
		zinitix_printk(KERN_INFO "failed to init flash\n");
		goto fail_upgrade;
	}
	
	zinitix_printk(KERN_INFO "read firmware data\n");
	for (flash_addr = 0; flash_addr < size; ) {
		for (i = 0; i < page_sz/TC_SECTOR_SZ; i++) {
			//zinitix_debug_msg("read :addr=%04x, len=%d\n", flash_addr, TC_SECTOR_SZ);
			if (ts_read_firmware_data(touch_dev->client,
				ZINITIX_READ_FLASH,
				(u8*)&verify_data[flash_addr], TC_SECTOR_SZ) < 0) {
				zinitix_printk(KERN_INFO "error : read zinitix tc firmare\n");
				goto fail_upgrade;
			}
			flash_addr += TC_SECTOR_SZ;
		}
	}
	/* verify */
	printk(KERN_INFO "verify firmware data\n");
	if (memcmp((u8 *)&firmware_data[0], (u8 *)&verify_data[0], size) == 0) {
		zinitix_printk(KERN_INFO "upgrade finished\n");
		kfree(verify_data);
		ts_power_control(touch_dev, POWER_OFF);
		ts_power_control(touch_dev, POWER_ON_SEQUENCE);
		
		return true;
	}
	
	
fail_upgrade:
	ts_power_control(touch_dev, POWER_OFF);

	if (retry_cnt++ < ZINITIX_INIT_RETRY_CNT) {
		zinitix_printk(KERN_INFO "upgrade fail : so retry... (%d)\n", retry_cnt);
		goto retry_upgrade;		
	}

	if (verify_data != NULL)
		kfree(verify_data);

	zinitix_printk(KERN_INFO "upgrade fail..\n");
	return false;


}

static bool ts_hw_calibration(struct zinitix_touch_dev *touch_dev)
{
	u16	chip_eeprom_info;
	int time_out = 0;

	if (ts_write_reg(touch_dev->client,
		ZINITIX_TOUCH_MODE, 0x07) != I2C_SUCCESS)
		return false;
	mdelay(10);
	ts_write_cmd(touch_dev->client,	ZINITIX_CLEAR_INT_STATUS_CMD);
	mdelay(10);
	ts_write_cmd(touch_dev->client,	ZINITIX_CLEAR_INT_STATUS_CMD);	
	mdelay(50);
	ts_write_cmd(touch_dev->client,	ZINITIX_CLEAR_INT_STATUS_CMD);	
	mdelay(10);
	if (ts_write_cmd(touch_dev->client,
		ZINITIX_CALIBRATE_CMD) != I2C_SUCCESS)
		return false;
	if (ts_write_cmd(touch_dev->client,
		ZINITIX_CLEAR_INT_STATUS_CMD) != I2C_SUCCESS)
		return false;
	mdelay(10);
	ts_write_cmd(touch_dev->client,	ZINITIX_CLEAR_INT_STATUS_CMD);
		
	/* wait for h/w calibration*/
	do {
		mdelay(500);
		ts_write_cmd(touch_dev->client,
				ZINITIX_CLEAR_INT_STATUS_CMD);			
		if (ts_read_data(touch_dev->client,
			ZINITIX_EEPROM_INFO_REG,
			(u8 *)&chip_eeprom_info, 2) < 0)
			return false;
		zinitix_debug_msg("touch eeprom info = 0x%04X\r\n",
			chip_eeprom_info);
		if (!zinitix_bit_test(chip_eeprom_info, 0))
			break;
		if(time_out++ == 4){
			ts_write_cmd(touch_dev->client,	ZINITIX_CALIBRATE_CMD);
			mdelay(10);
			ts_write_cmd(touch_dev->client,
				ZINITIX_CLEAR_INT_STATUS_CMD);						
			zinitix_printk("h/w calibration retry timeout.\n");
		}
		if(time_out++ > 10){
			zinitix_printk("[error] h/w calibration timeout.\n");
			break;						
		}
	} while (1);
	
	if (ts_write_reg(touch_dev->client,
		ZINITIX_TOUCH_MODE, TOUCH_POINT_MODE) != I2C_SUCCESS)
		return false;

	if (touch_dev->cap_info.ic_int_mask != 0)
		if (ts_write_reg(touch_dev->client,
			ZINITIX_INT_ENABLE_FLAG,
			touch_dev->cap_info.ic_int_mask)
			!= I2C_SUCCESS)
			return false;	
	ts_write_reg(touch_dev->client, 0xc003, 0x0001);
	ts_write_reg(touch_dev->client, 0xc104, 0x0001);
	udelay(100);
	if (ts_write_cmd(touch_dev->client,
		ZINITIX_SAVE_CALIBRATION_CMD) != I2C_SUCCESS)
		return false;
	mdelay(1000);	
	ts_write_reg(touch_dev->client, 0xc003, 0x0000);
	ts_write_reg(touch_dev->client, 0xc104, 0x0000);
	return true;				

}


static bool ts_init_touch(struct zinitix_touch_dev *touch_dev)
{
	u16 reg_val;
	int i;
	u16 ic_revision;
	u16 firmware_version;
	u16 minor_firmware_version;	
	u16 reg_data_version;
	u16 chip_eeprom_info;
#if USE_CHECKSUM	
	u16 chip_check_sum;
	u8 checksum_err;
#endif	
	int retry_cnt = 0;

	if (touch_dev == NULL) {
		zinitix_printk("error touch_dev == null?\r\n");
		return false;
	}

retry_init:

	for(i = 0; i < ZINITIX_INIT_RETRY_CNT; i++) {		
		if (ts_read_data(touch_dev->client,
				ZINITIX_EEPROM_INFO_REG,
				(u8 *)&chip_eeprom_info, 2) < 0) {
			zinitix_printk("fail to read eeprom info(%d)\r\n", i);
			mdelay(10);
			continue;
		} else
			break;
	}
	if (i == ZINITIX_INIT_RETRY_CNT) {		
		goto fail_init;
	}


#if USE_CHECKSUM		
	zinitix_debug_msg("check checksum\r\n");			

	checksum_err = 0;

	for (i = 0; i < ZINITIX_INIT_RETRY_CNT; i++) {
		if (ts_read_data(touch_dev->client,
			ZINITIX_CHECKSUM_RESULT, (u8 *)&chip_check_sum, 2) < 0) {
			mdelay(10);
			continue;
		}

		zinitix_debug_msg("0x%04X\r\n",	chip_check_sum);

		if(chip_check_sum == 0x55aa)				
			break;
		else {
			if(chip_eeprom_info == 1 && chip_check_sum == 1) {
				zinitix_printk("it might fail to boot.\n");		
				goto fail_init
			}
			checksum_err = 1;
			break;
		}
	}
		
	if (i == ZINITIX_INIT_RETRY_CNT || checksum_err) {
		zinitix_printk("fail to check firmware data\r\n");
		if(checksum_err == 1 && retry_cnt < ZINITIX_INIT_RETRY_CNT)
			retry_cnt = ZINITIX_INIT_RETRY_CNT;
		goto fail_init;
	}

#endif

	if (ts_write_cmd(touch_dev->client,
		ZINITIX_SWRESET_CMD) != I2C_SUCCESS) {
		zinitix_printk("fail to write reset command\r\n");
		goto fail_init;
	}

	touch_dev->cap_info.button_num = SUPPORTED_BUTTON_NUM;

	reg_val = 0;
	zinitix_bit_set(reg_val, BIT_PT_CNT_CHANGE);
	zinitix_bit_set(reg_val, BIT_DOWN);
	zinitix_bit_set(reg_val, BIT_MOVE);
	zinitix_bit_set(reg_val, BIT_UP);
	if (touch_dev->cap_info.button_num > 0)
		zinitix_bit_set(reg_val, BIT_ICON_EVENT);
	touch_dev->cap_info.ic_int_mask = reg_val;

	if (ts_write_reg(touch_dev->client,
		ZINITIX_INT_ENABLE_FLAG, 0x0) != I2C_SUCCESS)
		goto fail_init;

	zinitix_debug_msg("send reset command\r\n");
	if (ts_write_cmd(touch_dev->client,
		ZINITIX_SWRESET_CMD) != I2C_SUCCESS)
		goto fail_init;

	/* get chip revision id */
	if (ts_read_data(touch_dev->client,
		ZINITIX_CHIP_REVISION,
		(u8 *)&ic_revision, 2) < 0) {
		zinitix_printk("fail to read chip revision\r\n");
		goto fail_init;
	}
	zinitix_printk("touch chip revision id = %x\r\n",
		ic_revision);

	if(touch_dev->cap_info.is_zmt200 == 0)
		touch_dev->cap_info.ic_fw_size = 32*1024;
	else
		touch_dev->cap_info.ic_fw_size = 24*1024;

	//for  Debugging Tool
	if (ts_read_data(touch_dev->client,
		ZINITIX_HW_ID,
		(u8 *)&touch_dev->cap_info.hw_id, 2) < 0)
		goto fail_init;
	
	zinitix_printk("touch chip hw id = 0x%04x\r\n",
			touch_dev->cap_info.hw_id);
	
	if (ts_read_data(touch_dev->client,
		ZINITIX_TOTAL_NUMBER_OF_X,
		(u8 *)&touch_dev->cap_info.x_node_num, 2) < 0)
		goto fail_init;
	zinitix_printk("touch chip x node num = %d\r\n",
		touch_dev->cap_info.x_node_num);
	if (ts_read_data(touch_dev->client,
		ZINITIX_TOTAL_NUMBER_OF_Y,
		(u8 *)&touch_dev->cap_info.y_node_num, 2) < 0)
		goto fail_init;
	zinitix_printk("touch chip y node num = %d\r\n",
		touch_dev->cap_info.y_node_num);

	touch_dev->cap_info.total_node_num =
		touch_dev->cap_info.x_node_num*touch_dev->cap_info.y_node_num;
	zinitix_printk("touch chip total node num = %d\r\n",
		touch_dev->cap_info.total_node_num);

	if (ts_read_data(touch_dev->client,
			ZINITIX_AFE_FREQUENCY,
			(u8 *)&touch_dev->cap_info.afe_frequency, 2) < 0)
			goto fail_init;
	zinitix_debug_msg("afe frequency = %d\r\n",
			touch_dev->cap_info.afe_frequency);	

	//--------------------------------------------------------
	/* get chip firmware version */
	if (ts_read_data(touch_dev->client,
		ZINITIX_FIRMWARE_VERSION,
		(u8 *)&firmware_version, 2) < 0)
		goto fail_init;
	zinitix_printk("touch chip firmware version = %x\r\n",
		firmware_version);

	if (ts_read_data(touch_dev->client,
		ZINITIX_MINOR_FW_VERSION,
		(u8 *)&minor_firmware_version, 2) < 0)
		goto fail_init;
	zinitix_printk("touch chip firmware version = %x\r\n",
		minor_firmware_version);

#if TOUCH_ONESHOT_UPGRADE

	if (ts_read_data(touch_dev->client,
		ZINITIX_DATA_VERSION_REG,
		(u8 *)&reg_data_version, 2) < 0)
		goto fail_init;
	zinitix_debug_msg("touch reg data version = %d\r\n",
		reg_data_version);

	if (ts_check_need_upgrade(touch_dev, firmware_version, minor_firmware_version,
		reg_data_version, touch_dev->cap_info.hw_id) == true) {
		zinitix_printk("start upgrade firmware\n");
		
		if(ts_upgrade_firmware(touch_dev, m_pFirmware[m_FirmwareIdx],
			touch_dev->cap_info.ic_fw_size) == false)
			goto fail_init;

		if(ts_hw_calibration(touch_dev) == false)
			goto fail_init;

		/* disable chip interrupt */
		if (ts_write_reg(touch_dev->client,
			ZINITIX_INT_ENABLE_FLAG, 0) != I2C_SUCCESS)
			goto fail_init;

		/* get chip firmware version */
		if (ts_read_data(touch_dev->client,
			ZINITIX_FIRMWARE_VERSION,
			(u8 *)&firmware_version, 2) < 0)
			goto fail_init;
		zinitix_printk("touch chip firmware version = %x\r\n",
			firmware_version);

		if (ts_read_data(touch_dev->client,
			ZINITIX_MINOR_FW_VERSION,
			(u8 *)&minor_firmware_version, 2) < 0)
			goto fail_init;
		zinitix_printk("touch chip firmware version = %x\r\n",
			minor_firmware_version);
	}
#endif

	if (ts_read_data(touch_dev->client,
		ZINITIX_DATA_VERSION_REG,
		(u8 *)&reg_data_version, 2) < 0)
		goto fail_init;
	zinitix_debug_msg("touch reg data version = %d\r\n",
		reg_data_version);

	if (ts_read_data(touch_dev->client,
		ZINITIX_EEPROM_INFO_REG,
		(u8 *)&chip_eeprom_info, 2) < 0)
		goto fail_init;
	zinitix_debug_msg("touch eeprom info = 0x%04X\r\n", chip_eeprom_info);

	if (zinitix_bit_test(chip_eeprom_info, 0)) { /* hw calibration bit*/

		if(ts_hw_calibration(touch_dev) == false)
			goto fail_init;

		/* disable chip interrupt */
		if (ts_write_reg(touch_dev->client,
			ZINITIX_INT_ENABLE_FLAG, 0) != I2C_SUCCESS)
			goto fail_init;

	}


	touch_dev->cap_info.ic_revision = (u16)ic_revision;
	touch_dev->cap_info.firmware_version = (u16)firmware_version;
	touch_dev->cap_info.firmware_minor_version = (u16)minor_firmware_version;
	touch_dev->cap_info.reg_data_version = (u16)reg_data_version;


	/* initialize */
	touch_dev->cap_info.x_resolution = SYSTEM_MAX_X_RESOLUTION;
	touch_dev->cap_info.y_resolution = SYSTEM_MAX_Y_RESOLUTION;
	if (ts_write_reg(touch_dev->client,
		ZINITIX_X_RESOLUTION, (u16)(touch_dev->cap_info.x_resolution)) != I2C_SUCCESS)
		goto fail_init;

	if (ts_write_reg(touch_dev->client,
		ZINITIX_Y_RESOLUTION, (u16)(touch_dev->cap_info.y_resolution)) != I2C_SUCCESS)
		goto fail_init;

	

	zinitix_debug_msg("touch max x = %d\r\n", touch_dev->cap_info.x_resolution);
	zinitix_debug_msg("touch max y = %d\r\n", touch_dev->cap_info.y_resolution);

	touch_dev->cap_info.MinX = (u32)0;
	touch_dev->cap_info.MinY = (u32)0;
	touch_dev->cap_info.MaxX = (u32)touch_dev->cap_info.x_resolution;
	touch_dev->cap_info.MaxY = (u32)touch_dev->cap_info.y_resolution;

	if (ts_write_reg(touch_dev->client,
		ZINITIX_BUTTON_SUPPORTED_NUM,
		(u16)touch_dev->cap_info.button_num) != I2C_SUCCESS)
		goto fail_init;

	if (ts_write_reg(touch_dev->client,
		ZINITIX_SUPPORTED_FINGER_NUM,
		(u16)MAX_SUPPORTED_FINGER_NUM) != I2C_SUCCESS)
		goto fail_init;
	touch_dev->cap_info.multi_fingers = MAX_SUPPORTED_FINGER_NUM;

	zinitix_debug_msg("max supported finger num = %d\r\n",
		touch_dev->cap_info.multi_fingers);
	touch_dev->cap_info.gesture_support = 0;
	zinitix_debug_msg("set other configuration\r\n");

	if (ts_write_reg(touch_dev->client,
		ZINITIX_INITIAL_TOUCH_MODE, TOUCH_POINT_MODE) != I2C_SUCCESS)
		goto fail_init;

	if (ts_write_reg(touch_dev->client,
		ZINITIX_TOUCH_MODE, touch_dev->touch_mode) != I2C_SUCCESS)
		goto fail_init;

	//==================================================
	if (ts_read_data(touch_dev->client, ZINITIX_INTERNAL_FLAG_02,
			(u8 *)&reg_val, 2) < 0)
			goto fail_init;

	touch_dev->cap_info.i2s_checksum = !(!zinitix_bit_test(reg_val, 15));
	zinitix_printk("use i2s checksum = %d\r\n",
			touch_dev->cap_info.i2s_checksum);			
	
//	if(touch_dev->cap_info.i2s_checksum)
//		if (ts_write_reg(touch_dev->client,
//				ZINITIX_I2C_CHECKSUM_WCNT, (sizeof(struct _ts_zinitix_point_info)/2)) != I2C_SUCCESS)
//				goto fail_init;
		

	/* soft calibration */
	if (ts_write_cmd(touch_dev->client,
		ZINITIX_CALIBRATE_CMD) != I2C_SUCCESS)
		goto fail_init;
	

	if (touch_dev->touch_mode != TOUCH_POINT_MODE) { /* Test Mode */	
		if (ts_write_reg(touch_dev->client,
			ZINITIX_DELAY_RAW_FOR_HOST,
			RAWDATA_DELAY_FOR_HOST) != I2C_SUCCESS) {
			zinitix_printk("Fail to set ZINITIX_DELAY_RAW_FOR_HOST.\r\n");
			goto fail_init;
		}

	} 
	if (ts_write_reg(touch_dev->client,
		ZINITIX_PERIODICAL_INTERRUPT_INTERVAL,
		ZINITIX_SCAN_RATE_HZ
		*ZINITIX_ESD_TIMER_INTERVAL) != I2C_SUCCESS)
		goto fail_init;

	ts_read_data(touch_dev->client, ZINITIX_PERIODICAL_INTERRUPT_INTERVAL,
		(u8 *)&reg_val, 2);
	zinitix_debug_msg("esd timer register = %d\r\n", reg_val);
	
	if (ts_write_reg(touch_dev->client,
		ZINITIX_INT_ENABLE_FLAG,
		touch_dev->cap_info.ic_int_mask) != I2C_SUCCESS)
		goto fail_init;


	zinitix_debug_msg("successfully initialized\r\n");
	return true;

fail_init:

	if (++retry_cnt <= ZINITIX_INIT_RETRY_CNT) {
		ts_power_control(touch_dev, POWER_OFF);		
		ts_power_control(touch_dev, POWER_ON_SEQUENCE);
		
		zinitix_debug_msg("retry to initiallize(retry cnt = %d)\r\n",
			retry_cnt);
		goto	retry_init;
	} else if(retry_cnt == ZINITIX_INIT_RETRY_CNT+1) {
		if(touch_dev->cap_info.is_zmt200 == 0)
			touch_dev->cap_info.ic_fw_size = 32*1024;
		else
			touch_dev->cap_info.ic_fw_size = 24*1024;


		zinitix_debug_msg("retry to initiallize(retry cnt = %d)\r\n", retry_cnt);
#if TOUCH_FORCE_UPGRADE
		ts_check_hwid_in_fatal_state(touch_dev);
		ts_select_type_hw(touch_dev);
		if(ts_upgrade_firmware(touch_dev, m_pFirmware[m_FirmwareIdx],
			touch_dev->cap_info.ic_fw_size) == false) {
			zinitix_printk("upgrade fail\n");
			return false;
		}
		mdelay(100);
		// hw calibration and make checksum			
		if(ts_hw_calibration(touch_dev) == false) {
			zinitix_printk("failed to initiallize\r\n");
			return false;
		}
		goto retry_init;
#endif	  
	}

	zinitix_printk("failed to initiallize\r\n");
	return false;
}

static void zinitix_clear_report_data(struct zinitix_touch_dev *touch_dev)
{
	int i;
	u8 reported = 0;
	u8 sub_status;

	
	for (i = 0; i < touch_dev->cap_info.button_num; i++) {
		if (touch_dev->button[i] == ICON_BUTTON_DOWN) {
			touch_dev->button[i] = ICON_BUTTON_UP;
			input_report_key(touch_dev->input_dev,
				BUTTON_MAPPING_KEY[i], 0);
			reported = true;
			zinitix_debug_msg("button up = %d \r\n", i);
		}
	}
		

	for (i = 0; i < touch_dev->cap_info.multi_fingers; i++) {
		sub_status = touch_dev->reported_touch_info.coord[i].sub_status;
		if (zinitix_bit_test(sub_status, SUB_BIT_EXIST)) {
#if MULTI_PROTOCOL_TYPE_B
			input_mt_slot(touch_dev->input_dev, i);
			input_mt_report_slot_state(touch_dev->input_dev,
				MT_TOOL_FINGER, 0);

			input_report_abs(touch_dev->input_dev,
					ABS_MT_TRACKING_ID, -1);
#else
			input_report_abs(touch_dev->input_dev,
					ABS_MT_TRACKING_ID, i);
			input_report_abs(touch_dev->input_dev,
				ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(touch_dev->input_dev,
				ABS_MT_WIDTH_MAJOR, 0);		
			input_report_abs(touch_dev->input_dev,
				ABS_MT_POSITION_X,
				touch_dev->reported_touch_info.coord[i].x);
			input_report_abs(touch_dev->input_dev,
				ABS_MT_POSITION_Y,
				touch_dev->reported_touch_info.coord[i].y);
			input_mt_sync(touch_dev->input_dev);
#endif

			reported = true;
			if (!m_ts_debug_mode && TSP_NORMAL_EVENT_MSG)
				printk(KERN_INFO "[TSP] R %02d\r\n", i);
		}
		touch_dev->reported_touch_info.coord[i].sub_status = 0;
	}
	if (reported) {
		g_touch_press = 0;
#if !MULTI_PROTOCOL_TYPE_B
		input_report_key(touch_dev->input_dev, BTN_TOUCH, 0);
#endif
		input_sync(touch_dev->input_dev);
	}

}


static void zinitix_parsing_data(struct zinitix_touch_dev *touch_dev)
{
	
	int i;
	u8 reported = false;
	u8 sub_status;
	u8 prev_sub_status;
	u32 x, y;
	u32 w;
	u32 tmp;
	u8 read_result = 1;
	
	if(touch_dev == NULL) {
		printk(KERN_INFO "zinitix_parsing_data : touch_dev == NULL?\n");  //ysyim
		return;
	}

	if(down_trylock(&touch_dev->work_proceedure_lock)) {
		zinitix_printk("fail to occupy sema(%d)\r\n", touch_dev->work_proceedure);
		ts_write_cmd(touch_dev->client, ZINITIX_CLEAR_INT_STATUS_CMD);
		return;
	}

	
	if (touch_dev->work_proceedure != TS_NO_WORK) {
		zinitix_debug_msg("zinitix_touch_thread : \
			[warning] other process occupied..(%d)\r\n", touch_dev->work_proceedure);
		udelay(DELAY_FOR_SIGNAL_DELAY);
		if (!gpio_get_value(touch_dev->int_gpio_num)) {
			ts_write_cmd(touch_dev->client,
				ZINITIX_CLEAR_INT_STATUS_CMD);
			udelay(DELAY_FOR_SIGNAL_DELAY);
		}

		goto end_parsing_data;
	}

	if (touch_dev->use_esd_timer) {
		ts_esd_timer_stop(touch_dev);
		zinitix_debug_msg("esd timer stop\r\n");
	}
		
	touch_dev->work_proceedure = TS_NORMAL_WORK;

	i = 0;
	if(ts_read_coord(touch_dev) == false || touch_dev->touch_info.status == 0xffff
		|| touch_dev->touch_info.status == 0x1) {
		// more retry
		for(i=1; i< 50; i++) {	// about 10ms
			if(!(ts_read_coord(touch_dev) == false|| touch_dev->touch_info.status == 0xffff
			|| touch_dev->touch_info.status == 0x1))
				break;
		}

		if(i==50)
			read_result = 0;		
	}
		
	if (!read_result) {
		zinitix_debug_msg("couldn't read touch_dev coord. maybe chip is dead\r\n");
		ts_power_control(touch_dev, POWER_OFF);
		ts_power_control(touch_dev, POWER_ON_SEQUENCE);
		
		zinitix_debug_msg("clear all reported points\r\n");
		zinitix_clear_report_data(touch_dev);
		ts_mini_init_touch(touch_dev);
		goto continue_parsing_data;
	}

	/* invalid : maybe periodical repeated int. */
	if (touch_dev->touch_info.status == 0x0)
		goto continue_parsing_data;

	// added by sohnet (20130418) 
	if(i!= 0 && zinitix_bit_test(touch_dev->touch_info.status, BIT_PT_EXIST)) {
		zinitix_printk("may be corrupt data, so skip this.\n");
		goto continue_parsing_data;
	}

	reported = false;

	if (zinitix_bit_test(touch_dev->touch_info.status, BIT_ICON_EVENT)) {
		for (i = 0; i < touch_dev->cap_info.button_num; i++) {
			if (zinitix_bit_test(touch_dev->icon_event_reg,
				(BIT_O_ICON0_DOWN+i))) {
				touch_dev->button[i] = ICON_BUTTON_DOWN;
				input_report_key(touch_dev->input_dev,
					BUTTON_MAPPING_KEY[i], 1);
				reported = true;
				zinitix_debug_msg("button down = %d\r\n", i);
				if (!m_ts_debug_mode && TSP_NORMAL_EVENT_MSG)
					printk(KERN_INFO "[TSP] button down = %d\r\n", i);
			}
		}

		for (i = 0; i < touch_dev->cap_info.button_num; i++) {
			if (zinitix_bit_test(touch_dev->icon_event_reg,
				(BIT_O_ICON0_UP+i))) {
				touch_dev->button[i] = ICON_BUTTON_UP;
				input_report_key(touch_dev->input_dev,
					BUTTON_MAPPING_KEY[i], 0);
				reported = true;
				
				zinitix_debug_msg("button up = %d\r\n", i);
				if (!m_ts_debug_mode && TSP_NORMAL_EVENT_MSG)
					printk(KERN_INFO "[TSP] button down = %d\r\n", i);
			}
		}
	}

	/* if button press or up event occured... */
	if (reported == true || !zinitix_bit_test(touch_dev->touch_info.status, BIT_PT_EXIST)) {
		g_touch_press = 0;

		for (i = 0; i < touch_dev->cap_info.multi_fingers; i++) {
			prev_sub_status =
			touch_dev->reported_touch_info.coord[i].sub_status;
			if (zinitix_bit_test(prev_sub_status, SUB_BIT_EXIST)) {
				zinitix_debug_msg("finger [%02d] up\r\n", i);

#if MULTI_PROTOCOL_TYPE_B
				input_mt_slot(touch_dev->input_dev, i);
				input_mt_report_slot_state(touch_dev->input_dev,
					MT_TOOL_FINGER, 0);

				input_report_abs(touch_dev->input_dev,
						ABS_MT_TRACKING_ID, -1);
#else
				input_report_abs(touch_dev->input_dev,
						ABS_MT_TRACKING_ID, i);
				input_report_abs(touch_dev->input_dev,
					ABS_MT_TOUCH_MAJOR, 0);
				input_report_abs(touch_dev->input_dev,
					ABS_MT_WIDTH_MAJOR, 0);		
				input_report_abs(touch_dev->input_dev,
					ABS_MT_POSITION_X,
					touch_dev->reported_touch_info.coord[i].x);
				input_report_abs(touch_dev->input_dev,
					ABS_MT_POSITION_Y,
					touch_dev->reported_touch_info.coord[i].y);
				input_mt_sync(touch_dev->input_dev);
#endif				

				if (!m_ts_debug_mode && TSP_NORMAL_EVENT_MSG)
					printk(KERN_INFO "[TSP] R %02d\r\n", i);
			}
		}
		memset(&touch_dev->reported_touch_info,
			0x0, sizeof(struct _ts_zinitix_point_info));
#if !MULTI_PROTOCOL_TYPE_B
		input_report_key(touch_dev->input_dev, BTN_TOUCH, 0);
#endif		
		input_sync(touch_dev->input_dev);
		if(reported == true)	// for button event
			udelay(100);
		goto continue_parsing_data;
	}

		
	g_touch_press = 1;

	for (i = 0; i < touch_dev->cap_info.multi_fingers; i++) {
		sub_status = touch_dev->touch_info.coord[i].sub_status;
		
		prev_sub_status =
					touch_dev->reported_touch_info.coord[i].sub_status;

//		if (zinitix_bit_test(sub_status, SUB_BIT_DOWN)
//			|| zinitix_bit_test(sub_status, SUB_BIT_MOVE)
//			 || zinitix_bit_test(sub_status, SUB_BIT_EXIST)) {
		if (zinitix_bit_test(sub_status, SUB_BIT_EXIST)) {

			x = touch_dev->touch_info.coord[i].x;
			y = touch_dev->touch_info.coord[i].y;
			w = touch_dev->touch_info.coord[i].width;
			
			 /* transformation from touch to screen orientation */
			if (touch_dev->cap_info.Orientation & TOUCH_V_FLIP)
				y = touch_dev->cap_info.MaxY
					+ touch_dev->cap_info.MinY - y;
			if (touch_dev->cap_info.Orientation & TOUCH_H_FLIP)
				x = touch_dev->cap_info.MaxX
					+ touch_dev->cap_info.MinX - x;
			if (touch_dev->cap_info.Orientation & TOUCH_XY_SWAP)
				zinitix_swap_v(x, y, tmp);

			if(x > touch_dev->cap_info.MaxX ||	y > touch_dev->cap_info.MaxY) {
				//zinitix_printk("invalid coord # %d : x=%d, y=%d\r\n", i, x, y); //ysyim
				continue;
			}
						
			touch_dev->touch_info.coord[i].x = x;
			touch_dev->touch_info.coord[i].y = y;
			zinitix_debug_msg("finger [%02d] x = %d, y = %d\r\n",
				i, x, y);

			if (w == 0)
				w = 1;

			if (!m_ts_debug_mode && !zinitix_bit_test(prev_sub_status, SUB_BIT_EXIST) && TSP_NORMAL_EVENT_MSG)
				printk(KERN_INFO "[TSP] P %02d\r\n", i);

#if MULTI_PROTOCOL_TYPE_B
			input_mt_slot(touch_dev->input_dev, i);
			input_mt_report_slot_state(touch_dev->input_dev,
				MT_TOOL_FINGER, 1);
			if (!zinitix_bit_test(prev_sub_status, SUB_BIT_EXIST))
#endif
			input_report_abs(touch_dev->input_dev,
					ABS_MT_TRACKING_ID, i);
			
			input_report_abs(touch_dev->input_dev,
				ABS_MT_TOUCH_MAJOR, (u32)w);
			input_report_abs(touch_dev->input_dev,
				ABS_MT_WIDTH_MAJOR, (u32)w);
#if (TOUCH_POINT_MODE == 2)
			input_report_abs(touch_dev->input_dev,
				ABS_MT_TOUCH_MINOR, (u32)touch_dev->touch_info.coord[i].minor_width);
			input_report_abs(touch_dev->input_dev,
				ABS_MT_ORIENTATION, touch_dev->touch_info.coord[i].angle - 90);
#endif

			
			input_report_abs(touch_dev->input_dev,
				ABS_MT_POSITION_X, x);
			input_report_abs(touch_dev->input_dev,
				ABS_MT_POSITION_Y, y);
#if !MULTI_PROTOCOL_TYPE_B			
			input_mt_sync(touch_dev->input_dev);
#endif
		}	else if (zinitix_bit_test(sub_status, SUB_BIT_UP)||
			zinitix_bit_test(prev_sub_status, SUB_BIT_EXIST)) {
			zinitix_debug_msg("finger [%02d] up\r\n", i);
			
			memset(&touch_dev->touch_info.coord[i],
				0x0, sizeof(struct _ts_zinitix_coord));
#if MULTI_PROTOCOL_TYPE_B			
			input_mt_slot(touch_dev->input_dev, i);
			input_mt_report_slot_state(touch_dev->input_dev,
				MT_TOOL_FINGER, 0);

			input_report_abs(touch_dev->input_dev,
					ABS_MT_TRACKING_ID, -1);
#else			
			input_report_abs(touch_dev->input_dev,
					ABS_MT_TRACKING_ID, i);
			input_report_abs(touch_dev->input_dev,
				ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(touch_dev->input_dev,
				ABS_MT_WIDTH_MAJOR, 0); 	
			input_report_abs(touch_dev->input_dev,
				ABS_MT_POSITION_X,
				touch_dev->reported_touch_info.coord[i].x);
			input_report_abs(touch_dev->input_dev,
				ABS_MT_POSITION_Y,
				touch_dev->reported_touch_info.coord[i].y);
			input_mt_sync(touch_dev->input_dev);			
#endif								

			if (!m_ts_debug_mode && TSP_NORMAL_EVENT_MSG)
				printk(KERN_INFO "[TSP] R %02d\r\n", i);
	
		} else {
			memset(&touch_dev->touch_info.coord[i],
				0x0, sizeof(struct _ts_zinitix_coord));
		}
		
	}
	memcpy((char *)&touch_dev->reported_touch_info,
		(char *)&touch_dev->touch_info,
		sizeof(struct _ts_zinitix_point_info));
#if !MULTI_PROTOCOL_TYPE_B
	input_report_key(touch_dev->input_dev, BTN_TOUCH, 1);
#endif
	input_sync(touch_dev->input_dev);
	
continue_parsing_data:
	
	/* check_interrupt_pin, if high, enable int & wait signal */
	if (touch_dev->work_proceedure == TS_NORMAL_WORK) {
		if (touch_dev->use_esd_timer) {
			ts_esd_timer_start(ZINITIX_CHECK_ESD_TIMER, touch_dev);
			zinitix_debug_msg("esd tmr start\n");
		}
		touch_dev->work_proceedure = TS_NO_WORK;
	}
end_parsing_data:	
	up(&touch_dev->work_proceedure_lock);	

}


#if USE_THREADED_IRQ
static irqreturn_t zinitix_touch_work(int irq, void *data)
{
	struct zinitix_touch_dev* touch_dev = (struct zinitix_touch_dev*)data;
	zinitix_debug_msg("threaded irq signalled\r\n");
	zinitix_parsing_data(touch_dev);
	return IRQ_HANDLED;
}
#else
static void zinitix_touch_work(struct work_struct *work)
{

	struct zinitix_touch_dev *touch_dev =
		container_of(work, struct zinitix_touch_dev, work);

	zinitix_debug_msg("signalled\r\n");
	zinitix_parsing_data(touch_dev);
	enable_irq(touch_dev->irq);
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void zinitix_late_resume(struct early_suspend *h)
{

	struct zinitix_touch_dev *touch_dev = misc_touch_dev;
	//touch_dev = container_of(h, struct zinitix_touch_dev, early_suspend);

	if (touch_dev == NULL)
		return;
	zinitix_printk("late resume++\r\n");

	down(&touch_dev->work_proceedure_lock);
	if (touch_dev->work_proceedure != TS_IN_RESUME
		&& touch_dev->work_proceedure != TS_IN_EALRY_SUSPEND) {
		zinitix_printk("invalid work proceedure (%d)\r\n",
			touch_dev->work_proceedure);
		up(&touch_dev->work_proceedure_lock);
		return;
	}
#ifdef ZCONFIG_PM_SLEEP	
	ts_write_cmd(touch_dev->client, ZINITIX_WAKEUP_CMD);
	mdelay(1);
#else
	ts_power_control(touch_dev, POWER_ON_SEQUENCE);
#endif	
	if (ts_mini_init_touch(touch_dev) == false)
		goto fail_late_resume;
	ts_enable_irq(touch_dev);
	touch_dev->work_proceedure = TS_NO_WORK;
	up(&touch_dev->work_proceedure_lock);
	zinitix_printk("late resume--\n");
	return;
fail_late_resume:
	zinitix_printk("failed to late resume\n");
	ts_enable_irq(touch_dev);
	touch_dev->work_proceedure = TS_NO_WORK;
	up(&touch_dev->work_proceedure_lock);
	return;
}


static void zinitix_early_suspend(struct early_suspend *h)
{
	struct zinitix_touch_dev *touch_dev = misc_touch_dev;
	//touch_dev = container_of(h, struct zinitix_touch_dev, early_suspend);

	if (touch_dev == NULL)
		return;

	zinitix_printk("early suspend++\n");

	disable_irq(touch_dev->irq);
	if (touch_dev->use_esd_timer)
		flush_work(&touch_dev->tmr_work);
#if !USE_THREADED_IRQ	
	flush_work(&touch_dev->work);
#endif

	down(&touch_dev->work_proceedure_lock);
	if (touch_dev->work_proceedure != TS_NO_WORK) {
		zinitix_printk("invalid work proceedure (%d)\r\n",
			touch_dev->work_proceedure);
		up(&touch_dev->work_proceedure_lock);
		ts_enable_irq(touch_dev);
		return;
	}
	touch_dev->work_proceedure = TS_IN_EALRY_SUSPEND;

	zinitix_debug_msg("clear all reported points\r\n");
	zinitix_clear_report_data(touch_dev);

	if (touch_dev->use_esd_timer) {
		ts_write_reg(touch_dev->client,
			ZINITIX_PERIODICAL_INTERRUPT_INTERVAL, 0);
		ts_esd_timer_stop(touch_dev);
		zinitix_printk("ts_esd_timer_stop\n");
	}

#ifdef ZCONFIG_PM_SLEEP
	ts_write_reg(touch_dev->client, ZINITIX_INT_ENABLE_FLAG, 0x0);

	udelay(100);
	if (ts_write_cmd(touch_dev->client, ZINITIX_SLEEP_CMD) != I2C_SUCCESS) {
		zinitix_printk("failed to enter into sleep mode\n");
		up(&touch_dev->work_proceedure_lock);
		return;
	}
#else
	ts_power_control(touch_dev, POWER_OFF);	
#endif	
	zinitix_printk("early suspend--\n");
	up(&touch_dev->work_proceedure_lock);
	return;
}

#endif	/* CONFIG_HAS_EARLYSUSPEND */



#ifdef ZCONFIG_PM_SLEEP

static int zinitix_resume(struct device *dev)
{
	//struct i2c_client *client = to_i2c_client(dev);
	struct zinitix_touch_dev *touch_dev = misc_touch_dev; //i2c_get_clientdata(client);
	if (touch_dev == NULL)
		return -1;

	zinitix_printk("resume++\n");
	down(&touch_dev->work_proceedure_lock);
	if (touch_dev->work_proceedure != TS_IN_SUSPEND) {
		zinitix_printk("invalid work proceedure (%d)\r\n",
			touch_dev->work_proceedure);
		up(&touch_dev->work_proceedure_lock);
		return 0;
	}

	ts_power_control(touch_dev, POWER_ON_SEQUENCE);

#ifdef CONFIG_HAS_EARLYSUSPEND
	touch_dev->work_proceedure = TS_IN_RESUME;
#else
	touch_dev->work_proceedure = TS_NO_WORK;
	if (ts_mini_init_touch(touch_dev) == false)
		zinitix_printk("failed to resume\n");
	ts_enable_irq(touch_dev);
#endif


	zinitix_printk("resume--\n");
	up(&touch_dev->work_proceedure_lock);
	return 0;
}

static int zinitix_suspend(struct device *dev)
{
	//struct i2c_client *client = to_i2c_client(dev);
	struct zinitix_touch_dev *touch_dev = misc_touch_dev; //i2c_get_clientdata(client);

	if (touch_dev == NULL)
		return -1;

	zinitix_printk("suspend++\n");
	
#ifndef CONFIG_HAS_EARLYSUSPEND	
	disable_irq(touch_dev->irq);	
#endif	
	if (touch_dev->use_esd_timer)
		flush_work(&touch_dev->tmr_work);
#if !USE_THREADED_IRQ	
	flush_work(&touch_dev->work);
#endif
	down(&touch_dev->work_proceedure_lock);
	if (touch_dev->work_proceedure != TS_NO_WORK
		&& touch_dev->work_proceedure != TS_IN_EALRY_SUSPEND) {
		zinitix_printk("invalid work proceedure (%d)\r\n",
			touch_dev->work_proceedure);
		up(&touch_dev->work_proceedure_lock);
#ifndef CONFIG_HAS_EARLYSUSPEND	
		ts_enable_irq(touch_dev);
#endif					
		return 0;
	}

#ifndef CONFIG_HAS_EARLYSUSPEND	
	zinitix_clear_report_data(touch_dev);

	if (touch_dev->use_esd_timer) {
		ts_esd_timer_stop(touch_dev);
		zinitix_printk("ts_esd_timer_stop\n");
	}

	ts_write_cmd(touch_dev->client, ZINITIX_SLEEP_CMD);
	udelay(100);
#endif
	ts_power_control(touch_dev, POWER_OFF);
	touch_dev->work_proceedure = TS_IN_SUSPEND;

	zinitix_printk("suspend--\n");
	up(&touch_dev->work_proceedure_lock);
	return 0;
}

static SIMPLE_DEV_PM_OPS(zinitix_dev_pm_ops,
			 zinitix_suspend, zinitix_resume);
#endif



static bool ts_set_touchmode(u16 value){
	int i;

	disable_irq(misc_touch_dev->irq);

	down(&misc_touch_dev->work_proceedure_lock);
	if (misc_touch_dev->work_proceedure != TS_NO_WORK) {
		printk(KERN_INFO "other process occupied.. (%d)\n",
			misc_touch_dev->work_proceedure);
		ts_enable_irq(misc_touch_dev);
		up(&misc_touch_dev->work_proceedure_lock);		
		return -1;
	}	

	misc_touch_dev->work_proceedure = TS_SET_MODE;	

	if(value == TOUCH_DND_MODE) {
		if (ts_write_reg(misc_touch_dev->client, ZINITIX_DND_N_COUNT, SEC_DND_N_COUNT)!=I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : Fail to set ZINITIX_DND_N_COUNT %d.\r\n", SEC_DND_N_COUNT);
		if (ts_write_reg(misc_touch_dev->client, ZINITIX_AFE_FREQUENCY, SEC_DND_FREQUENCY)!=I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : Fail to set ZINITIX_AFE_FREQUENCY %d.\r\n", SEC_DND_FREQUENCY);
	} else if(misc_touch_dev->touch_mode == TOUCH_DND_MODE)
	{
		if (ts_write_reg(misc_touch_dev->client, ZINITIX_AFE_FREQUENCY, 
			misc_touch_dev->cap_info.afe_frequency)!=I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : Fail to set ZINITIX_AFE_FREQUENCY %d.\r\n", misc_touch_dev->cap_info.afe_frequency);
	}
		

	if(value == TOUCH_SEC_NORMAL_MODE)
		misc_touch_dev->touch_mode = TOUCH_POINT_MODE;
	else
		misc_touch_dev->touch_mode = value;

	printk(KERN_INFO "[zinitix_touch] tsp_set_testmode, touchkey_testmode = %d\r\n", misc_touch_dev->touch_mode);

	if(misc_touch_dev->touch_mode != TOUCH_POINT_MODE) {
		if (ts_write_reg(misc_touch_dev->client,
			ZINITIX_DELAY_RAW_FOR_HOST,
			RAWDATA_DELAY_FOR_HOST) != I2C_SUCCESS)
			zinitix_printk("Fail to set ZINITIX_DELAY_RAW_FOR_HOST.\r\n");

	} 

	if (ts_write_reg(misc_touch_dev->client, ZINITIX_TOUCH_MODE, misc_touch_dev->touch_mode)!=I2C_SUCCESS)
		printk(KERN_INFO "[zinitix_touch] TEST Mode : Fail to set ZINITX_TOUCH_MODE %d.\r\n", misc_touch_dev->touch_mode);


	// clear garbage data
	for(i=0; i < 10; i++) {
		mdelay(20);
		ts_write_cmd(misc_touch_dev->client, ZINITIX_CLEAR_INT_STATUS_CMD);
	}

	misc_touch_dev->work_proceedure = TS_NO_WORK;
	ts_enable_irq(misc_touch_dev);
	up(&misc_touch_dev->work_proceedure_lock);
	return 1;	      
}

static int ts_upgrade_sequence(const u8 *firmware_data)
{
	disable_irq(misc_touch_dev->irq);
	down(&misc_touch_dev->work_proceedure_lock);
	misc_touch_dev->work_proceedure = TS_IN_UPGRADE;

	if (misc_touch_dev->use_esd_timer)
		ts_esd_timer_stop(misc_touch_dev);
	zinitix_debug_msg("clear all reported points\r\n");
	zinitix_clear_report_data(misc_touch_dev);

	printk(KERN_INFO "start upgrade firmware\n");

	if (ts_upgrade_firmware(misc_touch_dev,
		firmware_data,
		misc_touch_dev->cap_info.ic_fw_size) == false) {
		ts_enable_irq(misc_touch_dev);
		misc_touch_dev->work_proceedure = TS_NO_WORK;
		up(&misc_touch_dev->work_proceedure_lock);
		return -1;
	}

	if (ts_init_touch(misc_touch_dev) == false) {
		ts_enable_irq(misc_touch_dev);
		misc_touch_dev->work_proceedure = TS_NO_WORK;
		up(&misc_touch_dev->work_proceedure_lock);
		return -1;
	}


	if (misc_touch_dev->use_esd_timer) {
		ts_esd_timer_start(ZINITIX_CHECK_ESD_TIMER,
			misc_touch_dev);
		zinitix_debug_msg("esd timer start\r\n");
	}

	ts_enable_irq(misc_touch_dev);
	misc_touch_dev->work_proceedure = TS_NO_WORK;
	up(&misc_touch_dev->work_proceedure_lock);
	return 0;
}



static int ts_misc_fops_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int ts_misc_fops_close(struct inode *inode, struct file *filp)
{
	return 0;
}




#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static int ts_misc_fops_ioctl(struct inode *inode,
	struct file *filp, unsigned int cmd,
	unsigned long arg)
#else
static long ts_misc_fops_ioctl(struct file *filp,
	unsigned int cmd, unsigned long arg)
#endif
{
	void __user *argp = (void __user *)arg;
	struct _raw_ioctl raw_ioctl;
	u8 *u8Data;
	int ret = 0;
	size_t sz = 0;
	u16 version;
	u16 mode;

	struct _reg_ioctl reg_ioctl;
	u16 val;
	int nval = 0;

	if (misc_touch_dev == NULL)
		return -1;

	/* zinitix_debug_msg("cmd = %d, argp = 0x%x\n", cmd, (int)argp); */

	switch (cmd) {

	case TOUCH_IOCTL_GET_DEBUGMSG_STATE:
		ret = m_ts_debug_mode;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_SET_DEBUGMSG_STATE:
		if (copy_from_user(&nval, argp, 4)) {
			printk(KERN_INFO "[zinitix_touch] error : copy_from_user\n");
			return -1;
		}
		if (nval)
			printk(KERN_INFO "[zinitix_touch] on debug mode (%d)\n",
				nval);
		else
			printk(KERN_INFO "[zinitix_touch] off debug mode (%d)\n",
				nval);
		m_ts_debug_mode = nval;
		break;

	case TOUCH_IOCTL_GET_CHIP_REVISION:
		ret = misc_touch_dev->cap_info.ic_revision;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_FW_VERSION:
		ret = misc_touch_dev->cap_info.firmware_version;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_REG_DATA_VERSION:
		ret = misc_touch_dev->cap_info.reg_data_version;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_VARIFY_UPGRADE_SIZE:
		if (copy_from_user(&sz, argp, sizeof(size_t)))
			return -1;

		printk(KERN_INFO "firmware size = %d\r\n", sz);
		if (misc_touch_dev->cap_info.ic_fw_size != sz) {
			printk(KERN_INFO "firmware size error\r\n");
			return -1;
		}
		break;

	case TOUCH_IOCTL_VARIFY_UPGRADE_DATA:
		if (copy_from_user(m_pFirmware[0],
			argp, misc_touch_dev->cap_info.ic_fw_size))
			return -1;

		version = (u16) (m_pFirmware[0][52] | (m_pFirmware[0][53]<<8));		

		printk(KERN_INFO "firmware version = %x\r\n", version);

		if (copy_to_user(argp, &version, sizeof(version)))
			return -1;
		break;

	case TOUCH_IOCTL_START_UPGRADE:
		return ts_upgrade_sequence((u8*)m_pFirmware[0]);

	case TOUCH_IOCTL_GET_X_RESOLUTION:
		ret = misc_touch_dev->cap_info.x_resolution;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_Y_RESOLUTION:
		ret = misc_touch_dev->cap_info.y_resolution;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_X_NODE_NUM:
		ret = misc_touch_dev->cap_info.x_node_num;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_Y_NODE_NUM:
		ret = misc_touch_dev->cap_info.y_node_num;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_TOTAL_NODE_NUM:
		ret = misc_touch_dev->cap_info.total_node_num;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_HW_CALIBRAION:
		ret = -1;
		disable_irq(misc_touch_dev->irq);
		down(&misc_touch_dev->work_proceedure_lock);
		if (misc_touch_dev->work_proceedure != TS_NO_WORK) {
			printk(KERN_INFO"other process occupied.. (%d)\r\n",
				misc_touch_dev->work_proceedure);
			up(&misc_touch_dev->work_proceedure_lock);
			return -1;
		}
		misc_touch_dev->work_proceedure = TS_HW_CALIBRAION;
		mdelay(100);

		/* h/w calibration */
		if(ts_hw_calibration(misc_touch_dev) == true)
			ret = 0;

		mode = misc_touch_dev->touch_mode;
		if (ts_write_reg(misc_touch_dev->client,
			ZINITIX_TOUCH_MODE, mode) != I2C_SUCCESS) {
			printk(KERN_INFO "fail to set touch mode %d.\n",
				mode);
			goto fail_hw_cal;
		}

		if (ts_write_cmd(misc_touch_dev->client,
			ZINITIX_SWRESET_CMD) != I2C_SUCCESS)
			goto fail_hw_cal;

		ts_enable_irq(misc_touch_dev);
		misc_touch_dev->work_proceedure = TS_NO_WORK;
		up(&misc_touch_dev->work_proceedure_lock);
		return ret;
fail_hw_cal:
		ts_enable_irq(misc_touch_dev);
		misc_touch_dev->work_proceedure = TS_NO_WORK;
		up(&misc_touch_dev->work_proceedure_lock);
		return -1;

	case TOUCH_IOCTL_SET_RAW_DATA_MODE:
		if (misc_touch_dev == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}
		if (copy_from_user(&nval, argp, 4)) {
			printk(KERN_INFO "[zinitix_touch] error : copy_from_user\r\n");
			misc_touch_dev->work_proceedure = TS_NO_WORK;
			return -1;
		}
		ts_set_touchmode((u16)nval);	
		
		return 0;

	case TOUCH_IOCTL_GET_REG:
		if (misc_touch_dev == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}
		down(&misc_touch_dev->work_proceedure_lock);
		if (misc_touch_dev->work_proceedure != TS_NO_WORK) {
			printk(KERN_INFO "other process occupied.. (%d)\n",
				misc_touch_dev->work_proceedure);
			up(&misc_touch_dev->work_proceedure_lock);
			return -1;
		}

		misc_touch_dev->work_proceedure = TS_SET_MODE;

		if (copy_from_user(&reg_ioctl,
			argp, sizeof(struct _reg_ioctl))) {
			misc_touch_dev->work_proceedure = TS_NO_WORK;
			up(&misc_touch_dev->work_proceedure_lock);
			printk(KERN_INFO "[zinitix_touch] error : copy_from_user\n");
			return -1;
		}

		if (ts_read_data(misc_touch_dev->client,
			reg_ioctl.addr, (u8 *)&val, 2) < 0)
			ret = -1;

		nval = (int)val;

		if (copy_to_user(reg_ioctl.val, (u8 *)&nval, 4)) {
			misc_touch_dev->work_proceedure = TS_NO_WORK;
			up(&misc_touch_dev->work_proceedure_lock);
			printk(KERN_INFO "[zinitix_touch] error : copy_to_user\n");
			return -1;
		}

		zinitix_debug_msg("read : reg addr = 0x%x, val = 0x%x\n",
			reg_ioctl.addr, nval);

		misc_touch_dev->work_proceedure = TS_NO_WORK;
		up(&misc_touch_dev->work_proceedure_lock);
		return ret;

	case TOUCH_IOCTL_SET_REG:

		if (misc_touch_dev == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}
		down(&misc_touch_dev->work_proceedure_lock);
		if (misc_touch_dev->work_proceedure != TS_NO_WORK) {
			printk(KERN_INFO "other process occupied.. (%d)\n",
				misc_touch_dev->work_proceedure);
			up(&misc_touch_dev->work_proceedure_lock);
			return -1;
		}

		misc_touch_dev->work_proceedure = TS_SET_MODE;
		if (copy_from_user(&reg_ioctl,
				argp, sizeof(struct _reg_ioctl))) {
			misc_touch_dev->work_proceedure = TS_NO_WORK;
			up(&misc_touch_dev->work_proceedure_lock);
			printk(KERN_INFO "[zinitix_touch] error : copy_from_user\n");
			return -1;
		}

		if (copy_from_user(&val, reg_ioctl.val, 4)) {
			misc_touch_dev->work_proceedure = TS_NO_WORK;
			up(&misc_touch_dev->work_proceedure_lock);
			printk(KERN_INFO "[zinitix_touch] error : copy_from_user\n");
			return -1;
		}

		if (ts_write_reg(misc_touch_dev->client,
			reg_ioctl.addr, val) != I2C_SUCCESS)
			ret = -1;

		zinitix_debug_msg("write : reg addr = 0x%x, val = 0x%x\r\n",
			reg_ioctl.addr, val);
		misc_touch_dev->work_proceedure = TS_NO_WORK;
		up(&misc_touch_dev->work_proceedure_lock);
		return ret;

	case TOUCH_IOCTL_DONOT_TOUCH_EVENT:

		if (misc_touch_dev == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}
		down(&misc_touch_dev->work_proceedure_lock);
		if (misc_touch_dev->work_proceedure != TS_NO_WORK) {
			printk(KERN_INFO"other process occupied.. (%d)\r\n",
				misc_touch_dev->work_proceedure);
			up(&misc_touch_dev->work_proceedure_lock);
			return -1;
		}

		misc_touch_dev->work_proceedure = TS_SET_MODE;
		if (ts_write_reg(misc_touch_dev->client,
			ZINITIX_INT_ENABLE_FLAG, 0) != I2C_SUCCESS)
			ret = -1;
		zinitix_debug_msg("write : reg addr = 0x%x, val = 0x0\r\n",
			ZINITIX_INT_ENABLE_FLAG);

		misc_touch_dev->work_proceedure = TS_NO_WORK;
		up(&misc_touch_dev->work_proceedure_lock);
		return ret;

	case TOUCH_IOCTL_SEND_SAVE_STATUS:
		if (misc_touch_dev == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}
		down(&misc_touch_dev->work_proceedure_lock);
		if (misc_touch_dev->work_proceedure != TS_NO_WORK) {
			printk(KERN_INFO"other process occupied.. (%d)\r\n",
				misc_touch_dev->work_proceedure);
			up(&misc_touch_dev->work_proceedure_lock);
			return -1;
		}
		misc_touch_dev->work_proceedure = TS_SET_MODE;
		ret = 0;
		ts_write_reg(misc_touch_dev->client, 0xc003, 0x0001);
		ts_write_reg(misc_touch_dev->client, 0xc104, 0x0001);
		if (ts_write_cmd(misc_touch_dev->client,
			ZINITIX_SAVE_STATUS_CMD) != I2C_SUCCESS)
			ret =  -1;
		
		mdelay(1000);	/* for fusing eeprom */
		ts_write_reg(misc_touch_dev->client, 0xc003, 0x0000);
		ts_write_reg(misc_touch_dev->client, 0xc104, 0x0000);

		misc_touch_dev->work_proceedure = TS_NO_WORK;
		up(&misc_touch_dev->work_proceedure_lock);
		return ret;

	case TOUCH_IOCTL_GET_RAW_DATA:
		if (misc_touch_dev == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}

		if (misc_touch_dev->touch_mode == TOUCH_POINT_MODE)
			return -1;

		down(&misc_touch_dev->raw_data_lock);
		if (misc_touch_dev->update == 0) {
			up(&misc_touch_dev->raw_data_lock);
			return -2;
		}

		if (copy_from_user(&raw_ioctl,
			argp, sizeof(raw_ioctl))) {
			up(&misc_touch_dev->raw_data_lock);
			printk(KERN_INFO "[zinitix_touch] error : copy_from_user\r\n");
			return -1;
		}

		misc_touch_dev->update = 0;

		u8Data = (u8 *)&misc_touch_dev->cur_data[0];
		
		if (copy_to_user(raw_ioctl.buf, (u8 *)u8Data,
			raw_ioctl.sz)) {
			up(&misc_touch_dev->raw_data_lock);
			return -1;
		}

		up(&misc_touch_dev->raw_data_lock);
		return 0;

	default:
		break;
	}
	return 0;
}


static void zinitix_reset(void)
{   
    volatile PGPIO pGPIO = (volatile PGPIO)tcc_p2v(HwGPIO_BASE);    

    printk("%s\n", __func__);

    pGPIO->GPFDAT.bREG.GP00 = 0;
    msleep_interruptible(10);
    pGPIO->GPFDAT.bREG.GP00 = 1;
    msleep_interruptible(10); 
}



static int zinitix_touch_probe(struct i2c_client *client,
		const struct i2c_device_id *i2c_id)
{
	int ret = 0;
	struct zinitix_touch_dev *touch_dev;
	int i;


	zinitix_debug_msg("zinitix_touch_probe+\r\n");

	printk(KERN_INFO "[zinitix touch] driver version = %s\r\n",
		TS_DRVIER_VERSION);


	if (strcmp(client->name, ZINITIX_DRIVER_NAME) != 0) {
		printk(KERN_INFO "not zinitix driver. \r\n");
		return -1;
	}

	zinitix_debug_msg("i2c check function \r\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "error : not compatible i2c function \r\n");
		goto err_check_functionality;
	}

	zinitix_debug_msg("touch data alloc \r\n");
	touch_dev = kzalloc(sizeof(struct zinitix_touch_dev), GFP_KERNEL);
	if (!touch_dev) {
		printk(KERN_ERR "unabled to allocate touch data \r\n");
		goto err_alloc_dev_data;
	}
	touch_dev->client = client;
	i2c_set_clientdata(client, touch_dev);

#if !USE_THREADED_IRQ
	INIT_WORK(&touch_dev->work, zinitix_touch_work);
	zinitix_debug_msg("touch workqueue create \r\n");
	zinitix_workqueue = create_singlethread_workqueue("zinitix_workqueue");
	if (!zinitix_workqueue) {
		printk(KERN_ERR "unabled to create touch thread \r\n");
		goto err_kthread_create_failed;
	}
#endif

	zinitix_debug_msg("allocate input device \r\n");
	touch_dev->input_dev = input_allocate_device();
	if (touch_dev->input_dev == 0) {
		printk(KERN_ERR "unabled to allocate input device \r\n");
		goto err_input_allocate_device;
	}

	/* initialize zinitix touch ic */
	// dean 
//	tcc_gpio_config(TCC_GPF(0), GPIO_OUTPUT | GPIO_FN0); // TOUCH_RST
	#if(CUR_AK_REV == AK70_HW_EVM)
	tcc_gpio_config(GPIO_TSP_PWEN, GPIO_OUTPUT | GPIO_FN0); // TSP_PWEN
	#endif
	
	tcc_gpio_config(GPIO_TOUCH_INT, GPIO_INPUT | GPIO_FN0); // TSP_INT
	
		

	tcc_gpio_config_ext_intr(client->irq, EXTINT_GPIOE_14);
	//gpio_request(TCC_GPB(11), "TS_INT");

	
	
	/* configure touchscreen interrupt gpio */
	ret = gpio_request(GPIO_TOUCH_PIN_NUM, "zinitix_irq_gpio");
	if (ret) {
		zinitix_printk("unable to request gpio.\r\n");
		goto err_power_sequence;
	}
	ret = gpio_direction_input(GPIO_TOUCH_PIN_NUM);
	touch_dev->int_gpio_num = GPIO_TOUCH_PIN_NUM;
#ifdef GPIO_LDO_EN_NUM
	/* configure touchscreen ldo gpio */
	ret = gpio_request(GPIO_LDO_EN_NUM, "zinitix_ldo_gpio");
	if (ret) {
		zinitix_printk("unable to request ldo gpio.\r\n");
		goto err_power_sequence;
	}
	ret = gpio_direction_output(GPIO_LDO_EN_NUM, 1);
	touch_dev->gpio_ldo_en_num = GPIO_LDO_EN_NUM;
#endif	
	// power on
	if(ts_power_control(touch_dev, POWER_ON_SEQUENCE)== false){
		zinitix_printk("power on sequence error : no ts device?\n");
		goto err_power_sequence;
	}

	//dean
	//zinitix_reset();


	memset(&touch_dev->reported_touch_info,
		0x0, sizeof(struct _ts_zinitix_point_info));


	/* init touch mode */
	touch_dev->touch_mode = TOUCH_POINT_MODE;
	misc_touch_dev = touch_dev;

	if(ts_init_touch(touch_dev) == false) {
		goto err_input_register_device;
	}

	for (i = 0; i < MAX_SUPPORTED_BUTTON_NUM; i++)
		touch_dev->button[i] = ICON_BUTTON_UNCHANGE;
		
	touch_dev->use_esd_timer = 0;

	INIT_WORK(&touch_dev->tmr_work, zinitix_touch_tmr_work);
	zinitix_tmr_workqueue =
		create_singlethread_workqueue("zinitix_tmr_workqueue");
	if (!zinitix_tmr_workqueue) {
		printk(KERN_ERR "unabled to create touch tmr work queue \r\n");
		goto err_kthread_create_failed;
	}
	
	if (ZINITIX_ESD_TIMER_INTERVAL) {
		touch_dev->use_esd_timer = 1;		
		ts_esd_timer_start(ZINITIX_CHECK_ESD_TIMER, touch_dev);
		printk(KERN_INFO " ts_esd_timer_start\n");
	}


	sprintf(touch_dev->phys, "input(ts)");
	touch_dev->input_dev->name = ZINITIX_DRIVER_NAME;
	touch_dev->input_dev->id.bustype = BUS_I2C;
	touch_dev->input_dev->id.vendor = 0x0001;
	touch_dev->input_dev->phys = touch_dev->phys;
	touch_dev->input_dev->id.product = 0x0002;
	touch_dev->input_dev->id.version = 0x0100;

	set_bit(EV_SYN, touch_dev->input_dev->evbit);
	set_bit(EV_KEY, touch_dev->input_dev->evbit);
	set_bit(EV_ABS, touch_dev->input_dev->evbit);
	set_bit(INPUT_PROP_DIRECT, touch_dev->input_dev->propbit);	
	

	for (i = 0; i < MAX_SUPPORTED_BUTTON_NUM; i++)
		set_bit(BUTTON_MAPPING_KEY[i], touch_dev->input_dev->keybit);

	if (touch_dev->cap_info.Orientation & TOUCH_XY_SWAP) {
		input_set_abs_params(touch_dev->input_dev, ABS_MT_POSITION_Y,
			touch_dev->cap_info.MinX,
			touch_dev->cap_info.MaxX + ABS_PT_OFFSET,
			0, 0);
		input_set_abs_params(touch_dev->input_dev, ABS_MT_POSITION_X,
			touch_dev->cap_info.MinY,
			touch_dev->cap_info.MaxY + ABS_PT_OFFSET,
			0, 0);
	} else {
		input_set_abs_params(touch_dev->input_dev, ABS_MT_POSITION_X,
			touch_dev->cap_info.MinX,
			touch_dev->cap_info.MaxX + ABS_PT_OFFSET,
			0, 0);
		input_set_abs_params(touch_dev->input_dev, ABS_MT_POSITION_Y,
			touch_dev->cap_info.MinY,
			touch_dev->cap_info.MaxY + ABS_PT_OFFSET,
			0, 0);
	}

	input_set_abs_params(touch_dev->input_dev, ABS_MT_TOUCH_MAJOR,
		0, 255, 0, 0);	
	input_set_abs_params(touch_dev->input_dev, ABS_MT_WIDTH_MAJOR,
		0, 255, 0, 0);

#if (TOUCH_POINT_MODE == 2)	
	input_set_abs_params(touch_dev->input_dev, ABS_MT_TOUCH_MINOR,
			0, 255, 0, 0);	
	input_set_abs_params(touch_dev->input_dev, ABS_MT_ORIENTATION,
		-90, 90, 0, 0);		
#endif	
	
#if MULTI_PROTOCOL_TYPE_B
	set_bit(MT_TOOL_FINGER, touch_dev->input_dev->keybit);
	input_mt_init_slots(touch_dev->input_dev, touch_dev->cap_info.multi_fingers);
#else
	set_bit(BTN_TOUCH, touch_dev->input_dev->keybit);
#endif


	input_set_abs_params(touch_dev->input_dev, ABS_MT_TRACKING_ID, 
		0, touch_dev->cap_info.multi_fingers, 0, 0);
	
	zinitix_debug_msg("register %s input device \r\n",
		touch_dev->input_dev->name);
	input_set_drvdata(touch_dev->input_dev, touch_dev);
	ret = input_register_device(touch_dev->input_dev);
	if (ret) {
		printk(KERN_ERR "unable to register %s input device\r\n",
			touch_dev->input_dev->name);
		goto err_input_register_device;
	}

	//configure irq

#ifdef	GPIO_TOUCH_IRQ
	touch_dev->irq = GPIO_TOUCH_IRQ;
#else
	touch_dev->irq = gpio_to_irq(touch_dev->int_gpio_num);
	if (touch_dev->irq < 0)
		printk(KERN_INFO "error. gpio_to_irq(..) function is not \
			supported? you should define GPIO_TOUCH_IRQ.\r\n");
#endif
	zinitix_debug_msg("request irq (irq = %d, pin = %d) \r\n",
		touch_dev->irq, touch_dev->int_gpio_num);

	touch_dev->work_proceedure = TS_NO_WORK;
	sema_init(&touch_dev->work_proceedure_lock, 1);

	ts_check_int_before_enirq(touch_dev);
	
#if USE_THREADED_IRQ
	//ret = request_threaded_irq(touch_dev->irq, ts_int_handler, zinitix_touch_work, 
	ret = request_threaded_irq(touch_dev->irq, NULL, zinitix_touch_work, 
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT , ZINITIX_DRIVER_NAME, touch_dev);
#else		
	ret = request_irq(touch_dev->irq, ts_int_handler,
		IRQF_TRIGGER_FALLING, ZINITIX_DRIVER_NAME, touch_dev);
//		IRQF_TRIGGER_LOW, ZINITIX_DRIVER_NAME, touch_dev);
#endif
	if (ret) {
		printk(KERN_ERR "unable to register irq.(%s)\r\n",
			touch_dev->input_dev->name);
		goto err_request_irq;
	}
	dev_info(&client->dev, "zinitix touch probe.\r\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
	touch_dev->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	touch_dev->early_suspend.suspend = zinitix_early_suspend;
	touch_dev->early_suspend.resume = zinitix_late_resume;
	register_early_suspend(&touch_dev->early_suspend);
#endif


	sema_init(&touch_dev->raw_data_lock, 1);
	
	ret = misc_register(&touch_misc_device);
	if (ret)
		zinitix_debug_msg("Fail to register touch misc device.\n");



	return 0;

err_request_irq:
	input_unregister_device(touch_dev->input_dev);
err_input_register_device:
	input_free_device(touch_dev->input_dev);
err_power_sequence:	
err_kthread_create_failed:
err_input_allocate_device:
	kfree(touch_dev);
err_alloc_dev_data:
err_check_functionality:
	touch_dev = NULL;
	misc_touch_dev = NULL;
	return -ENODEV;
}


static int zinitix_touch_remove(struct i2c_client *client)
{
	struct zinitix_touch_dev *touch_dev = i2c_get_clientdata(client);
	if(touch_dev == NULL)	return 0;

	zinitix_debug_msg("zinitix_touch_remove+ \r\n");
	down(&touch_dev->work_proceedure_lock);
#if !USE_THREADED_IRQ	
	if (touch_dev->work_proceedure != TS_NO_WORK)
		flush_work(&touch_dev->work);
#endif
	touch_dev->work_proceedure = TS_REMOVE_WORK;

	if (touch_dev->use_esd_timer != 0) {
		flush_work(&touch_dev->tmr_work);
		ts_write_reg(touch_dev->client,
			ZINITIX_PERIODICAL_INTERRUPT_INTERVAL, 0);
		ts_esd_timer_stop(touch_dev);
		zinitix_debug_msg(KERN_INFO " ts_esd_timer_stop\n");
		destroy_workqueue(zinitix_tmr_workqueue);
	}

	if (touch_dev->irq)
		free_irq(touch_dev->irq, touch_dev);

	misc_deregister(&touch_misc_device);


#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&touch_dev->early_suspend);
#endif

#if !USE_THREADED_IRQ
	destroy_workqueue(zinitix_workqueue);
#endif
	if (gpio_is_valid(touch_dev->int_gpio_num) != 0)
		gpio_free(touch_dev->int_gpio_num);

	input_unregister_device(touch_dev->input_dev);
	input_free_device(touch_dev->input_dev);
	up(&touch_dev->work_proceedure_lock);
	kfree(touch_dev);

	return 0;
}


static struct i2c_driver zinitix_touch_driver = {
	.probe	= zinitix_touch_probe,
	.remove	= zinitix_touch_remove,
	.id_table	= zinitix_idtable,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= ZINITIX_DRIVER_NAME,
#ifdef ZCONFIG_PM_SLEEP		
		.pm	= &zinitix_dev_pm_ops,
#endif		
	},
};

static int __devinit zinitix_touch_init(void)
{

#if TOUCH_I2C_REGISTER_HERE	
	i2c_register_board_info(0, Zxt_i2c_info, ARRAY_SIZE(Zxt_i2c_info));
#endif
	return i2c_add_driver(&zinitix_touch_driver);
}

static void __exit zinitix_touch_exit(void)
{
	i2c_del_driver(&zinitix_touch_driver);
}


module_init(zinitix_touch_init);
module_exit(zinitix_touch_exit);

MODULE_DESCRIPTION("zinitix touch-screen device driver using i2c interface");
MODULE_AUTHOR("sohnet <seonwoong.jang@zinitix.com>");
MODULE_LICENSE("GPL");


