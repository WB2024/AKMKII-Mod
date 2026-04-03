/*
 *  Copyright (C) 2010,Imagis Technology Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <asm/unaligned.h>

#include "ist3000.h"
#include "ist3000_config.h"


#define IST3000_FW_NAME         "ist3000.fw"

#define IST3000_MAX_MT_FINGERS  (10)
#define MAX_ERR_CNT             (2)

enum ist3000_commands {
	CMD_START_SCAN              = 0x00,
	CMD_UPGRADE_FW              = 0x01,
	CMD_ENTER_UPDATE            = 0x02,
	CMD_EXIT_UPDATE             = 0x03,
	CMD_UPDATE_SENSOR           = 0x04,
	CMD_UPDATE_CONFIG           = 0x05,
	CMD_ENTER_REG_ACCESS        = 0x07,
	CMD_EXIT_REG_ACCESS         = 0x08,
	CMD_SET_TA_MODE             = 0x0A,

	CMD_BACKUP_SENSOR           = 0x11,
	CMD_BACKUP_CONFIG           = 0x12,

	CMD_GET_COORD               = 0x20,

	CMD_GET_CHIP_ID             = 0x30,
	CMD_GET_FW_VER              = 0x31,
	CMD_GET_ALGORITHM_VER       = 0x32,
	CMD_GET_IST3000_STATUS      = 0x36,
	CMD_GET_CALIB_RESULT        = 0x37,

	CMD_FW_BEGIN_UPDATE         = 0xE1,
	CMD_FW_END_UPDATE           = 0xE0,
};

#define MULTI_MSG_MASK          (0x02)
#define PRESS_MSG_MASK          (0x01)
#define CALIB_MSG_MASK          (0xF0000FFF)
#define CALIB_MSG_VALID         (0x80000CAB)

#define IST3000_SENSOR_ADDR     (0x40009000)
#define IST3000_CONFIG_ADDR     (0x20000040)

#define STATUS_MSG_LEN          (128)
#define UPDATE_BY_BIN           (0x1)
#define WAIT_CALIB_CNT          (50)

#define CALIB_TO_GAP(n)         ((n >> 16) & 0xFFF)
#define CALIB_TO_STATUS(n)      ((n >> 12) & 0xF)


typedef union {
	struct {
		u32	y       : 10;
		u32	w       : 6;
		u32	x       : 10;
		u32	id      : 4;
		u32	udmg    : 2;
	} bit_field;
	u32 full_field;
} finger_info;

struct ist3000_data {
	struct i2c_client *	client;
	struct input_dev *	input_dev;
	struct early_suspend	early_suspend;
	u32			chip_id;
	u32			fw_ver;
	u32			alg_ver;
	int			num_fingers;
	finger_info		fingers[IST3000_MAX_MT_FINGERS];
};

#if IST3000_DETECT_TA
static int ist3000_ta_status = -1;
#endif

static int ist3000_fw_update_status = 0;
static int ist3000_fw_prev_version = 0;
static int ist3000_calib_status = 0;
static int ist3000_error_cnt = 0;

static struct ist3000_data *copy_data;

static DEFINE_MUTEX(ist3000_mutex);
static struct delayed_work work_reset_check;


static void ist3000_request_reset(void)
{
	ist3000_error_cnt++;

	if (ist3000_error_cnt >= MAX_ERR_CNT) {
		schedule_delayed_work(&work_reset_check, 0);
		DMSG("[>>> IST30xx] ist3000_request_reset is called\n");
		ist3000_error_cnt = 0;
	}
}


static void ist3000_start(struct ist3000_data *data)
{
#if IST3000_DETECT_TA
	if (ist3000_ta_status > -1) {
		ist3000_write_cmd(data->client, CMD_SET_TA_MODE,
				  &ist3000_ta_status, 1);

		DMSG("[>>> IST30xx] ist3000_start called : ta_mode : %d\n",
		     ist3000_ta_status);
	}
#endif

	ist3000_write_cmd(data->client, CMD_START_SCAN, NULL, 0);

	msleep(100);
}


static int ist3000_get_ver_info(struct ist3000_data *data)
{
	int ret;

	ist3000_fw_prev_version = data->fw_ver;
	data->fw_ver = 0;

	ret = ist3000_read_cmd(data->client, CMD_GET_CHIP_ID, &data->chip_id, 1);
	printk("%s(%d) chip_id : %x\n", __func__, __LINE__, data->chip_id);
	if (ret)
		return -EIO;

	ret = ist3000_read_cmd(data->client, CMD_GET_FW_VER, &data->fw_ver, 1);
	printk("%s(%d) fw_ver : %x\n", __func__, __LINE__, data->fw_ver);
	if (ret)
		return -EIO;

	ret = ist3000_read_cmd(data->client, CMD_GET_ALGORITHM_VER,
			       &data->alg_ver, 1);
	printk("%s(%d) alg_ver : %x\n", __func__, __LINE__, data->alg_ver);
	if (ret)
		return -EIO;

	return 0;
}


static int ist3000_init_touch_driver(struct ist3000_data *data)
{
	if(ist3000_get_ver_info(data) < 0)
		printk("%s(%d) error on getting version info. \n", __func__, __LINE__); 

	ist3000_start(data);

	return 0;
}


static void clear_input_data(struct ist3000_data *data)
{
	int i, pressure, count;

	for (i = 0, count = 0; i < IST3000_MAX_MT_FINGERS; i++) {
		if (data->fingers[i].bit_field.id == 0)
			continue;

		pressure = (data->fingers[i].bit_field.udmg & PRESS_MSG_MASK);
		if (pressure > 0) {
			input_report_abs(data->input_dev, ABS_MT_POSITION_X,
					 data->fingers[i].bit_field.x);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
					 data->fingers[i].bit_field.y);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,
					 0);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR,
					 0);
			input_mt_sync(data->input_dev);

			data->fingers[i].bit_field.id = 0;

			count++;
		}
	}

	if (count > 0)
		input_sync(data->input_dev);
}


static void report_input_data(struct ist3000_data *data, int finger_counts)
{
	int i, pressure, count;

	for (i = 0, count = 0; i < finger_counts; i++) {
		if ((data->fingers[i].bit_field.id == 0) ||
		    (data->fingers[i].bit_field.id > IST3000_MAX_MT_FINGERS) ||
		    (data->fingers[i].bit_field.x > IST3000_MAX_X) ||
		    (data->fingers[i].bit_field.y > IST3000_MAX_Y)) {
			DMSG("[>>> Error] [%d][%d] - [%d][%d]\n", i,
			     data->fingers[i].bit_field.id,
			     data->fingers[i].bit_field.x,
			     data->fingers[i].bit_field.y);

			data->fingers[i].bit_field.id = 0;
			ist3000_request_reset();
			continue;
		}

		pressure = data->fingers[i].bit_field.udmg & PRESS_MSG_MASK;

		DMSG("[%d][%d][%d] x : y : z = %03d, %03d, %04d\n", i,
		     data->fingers[i].bit_field.id, pressure,
		     data->fingers[i].bit_field.x,
		     data->fingers[i].bit_field.y,
		     data->fingers[i].bit_field.w);

		input_report_abs(data->input_dev, ABS_MT_POSITION_X,
				 data->fingers[i].bit_field.x);
		input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
				 data->fingers[i].bit_field.y);
		input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,
				 pressure);
		if(pressure>0) {
			input_report_key(data->input_dev, BTN_TOUCH, 1);
		} else {
			input_report_key(data->input_dev, BTN_TOUCH, 0);

		}
		
		input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR,
				 data->fingers[i].bit_field.w);
		input_mt_sync(data->input_dev);

		count++;
	}

	if (count > 0)
		input_sync(data->input_dev);
	DMSG("\n");
}


static irqreturn_t ist3000_irq_thread(int irq, void *ptr)
{
	int finger_cnt, i, ret;
	struct ist3000_data *data = ptr;
	u32 msg[IST3000_MAX_MT_FINGERS];

	DMSG("%s(%d)\n", __func__, __LINE__);
	memset(msg, 0, IST3000_MAX_MT_FINGERS);
	ret = ist3000_read_cmd(data->client, CMD_GET_COORD, msg, 1);
	if (ret) {
		ist3000_request_reset();
		return IRQ_HANDLED;
	}

	if (msg[0] == 0)
		return IRQ_HANDLED;

	if ((msg[0] & CALIB_MSG_MASK) == CALIB_MSG_VALID) {
		ist3000_calib_status = msg[0];
		return IRQ_HANDLED;
	}

	for (i = 0; i < IST3000_MAX_MT_FINGERS; i++)
		data->fingers[i].full_field = 0;

	finger_cnt = 1;
	data->fingers[0].full_field = msg[0];

	if (data->fingers[0].bit_field.udmg & MULTI_MSG_MASK) {
		finger_cnt = data->fingers[0].bit_field.y;

		if ((finger_cnt < 2) || (finger_cnt > IST3000_MAX_MT_FINGERS)) {
			ist3000_request_reset();
			return IRQ_HANDLED;
		}

		ret = ist3000_read_cmd(data->client, CMD_GET_COORD, msg, finger_cnt);
		if (ret) {
			ist3000_request_reset();
			return IRQ_HANDLED;
		}

		for (i = 0; i < finger_cnt; i++)
			data->fingers[i].full_field = msg[i];
	}

	if (finger_cnt > 0)
		report_input_data(data, finger_cnt);

	return IRQ_HANDLED;
}


static int ist3000_internal_suspend(struct ist3000_data *data)
{
	ist3000_power_off();
	return 0;
}


static int ist3000_internal_resume(struct ist3000_data *data)
{
	ist3000_power_on();
	ist3000_reset();
	ist3000_start(data);
	return 0;
}


#ifdef CONFIG_HAS_EARLYSUSPEND
#define ist3000_suspend NULL
#define ist3000_resume  NULL


static void ist3000_early_suspend(struct early_suspend *h)
{
	struct ist3000_data *data = container_of(h, struct ist3000_data,
						 early_suspend);

	mutex_lock(&ist3000_mutex);
	disable_irq(data->client->irq);
	ist3000_internal_suspend(data);
	mutex_unlock(&ist3000_mutex);
}


static void ist3000_late_resume(struct early_suspend *h)
{
	struct ist3000_data *data = container_of(h, struct ist3000_data,
						 early_suspend);

	mutex_lock(&ist3000_mutex);
	ist3000_internal_resume(data);
	enable_irq(data->client->irq);
	mutex_unlock(&ist3000_mutex);
}
#else


static int ist3000_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ist3000_data *data = i2c_get_clientdata(client);

	return ist3000_internal_suspend(data);
}


static int ist3000_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ist3000_data *data = i2c_get_clientdata(client);

	return ist3000_internal_resume(data);
}
#endif


struct class *ist3000_class;
struct device *ist3000_firm_dev;


static int ist3000_calib_wait(void)
{
	int i;

	ist3000_calib_status = 0;
	for (i = 0; i < WAIT_CALIB_CNT; i++) {
		msleep(100);

		if (ist3000_calib_status) {
			DMSG("[>>> IST30xx] ist3000_calib_wait is done\n");
			return 0;
		}
	}

	return -1;
}


static ssize_t
ist3000_fw_update(struct device *dev, struct device_attribute *attr,
		  const char *buf, size_t size)
{
	int ret;
	char *after;
	const struct firmware *request_fw = NULL;

	struct ist3000_data *data = copy_data;
	struct i2c_client *client = data->client;

	unsigned long mode = simple_strtoul(buf, &after, 10);

	if (mode != 1) {
		ist3000_fw_update_status = 3;
		return size;
	}

	ret = request_firmware(&request_fw, IST3000_FW_NAME, &client->dev);
	if (ret) {
		ist3000_fw_update_status = 3;
		return size;
	}

	mutex_lock(&ist3000_mutex);

	disable_irq(client->irq);

	ist3000_reset();

	enable_irq(client->irq);

	ist3000_fw_update_status = 1;

	ist3000_write_cmd(client, CMD_UPGRADE_FW, NULL, 0);

	/* need 2.5msec after CMD_UPGRADE_FW */
	msleep(3);

	ret = ist3000_fw_cmd(client, CMD_FW_BEGIN_UPDATE);
	if (ret) {
		ist3000_fw_update_status = 3;
		goto end;
	}

	ret = ist3000_fw_erase_all(client);
	if (ret) {
		ist3000_fw_update_status = 3;
		goto end;
	}

	ret = ist3000_fw_write(client, request_fw->data, request_fw->size);
	if (ret) {
		ist3000_fw_update_status = 3;
		goto end;
	}

	ret = ist3000_fw_compare(client, request_fw->data, request_fw->size);
	if (ret) {
		ist3000_fw_update_status = 3;
		goto end;
	}

	ret = ist3000_fw_cmd(client, CMD_FW_END_UPDATE);
	if (ret) {
		ist3000_fw_update_status = 3;
		goto end;
	}

	ist3000_fw_update_status = 2;

end:
	ist3000_init_touch_driver(data);
	ist3000_calib_wait();
	mutex_unlock(&ist3000_mutex);
	release_firmware(request_fw);

	return size;
}


static ssize_t ist3000_fw_status(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int count;

	struct ist3000_data *data = copy_data;

	if (ist3000_fw_update_status == 1)
		count = sprintf(buf, "Downloading\n");
	else if (ist3000_fw_update_status == 2)
		count = sprintf(buf, "Update okay, from %x to %x, status : %d, gap : %d\n",
				ist3000_fw_prev_version, data->fw_ver,
				CALIB_TO_STATUS(ist3000_calib_status), CALIB_TO_GAP(ist3000_calib_status));
	else if (ist3000_fw_update_status == 3)
		count = sprintf(buf, "Update failed\n");
	else
		count = sprintf(buf, "Pass\n");

	return count;
}


static ssize_t ist3000_fw_version(struct device *dev, struct device_attribute *attr,
				  char *buf)
{
	int count;
	struct ist3000_data *data = copy_data;

	count = sprintf(buf, "chip id : 0x%x, algorithm ver : 0x%x, firmware ver : 0x%x\n",
			data->chip_id, data->alg_ver, data->fw_ver);
	return count;
}


struct device *ist3000_param_dev;


static ssize_t ist3000_sensor_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t size)
{
	int ret;
	u32 mode;
	u32 *param;
	char *after;
	const struct firmware *request_fw = NULL;
	struct ist3000_data *data = copy_data;
	struct i2c_client *client = data->client;

	mode = simple_strtoul(buf, &after, 10);

	if (mode & UPDATE_BY_BIN) {
		ret = request_firmware(&request_fw, IST3000_SENSOR_BIN, &client->dev);
		if (ret)
			return size;

		param = (u32 *)request_fw->data;
	} else {
		param = ist3000_sensor_param;
	}

	mutex_lock(&ist3000_mutex);

	ist3000_fw_update_status = 1;

	disable_irq(client->irq);
	ist3000_reset();
	enable_irq(client->irq);

	ret = ist3000_write_cmd(client, CMD_ENTER_UPDATE, NULL, 0);
	if (ret) {
		ist3000_fw_update_status = 3;
		goto end;
	}

	msleep(40);

	ret = ist3000_write_cmd(client, CMD_UPDATE_SENSOR, param, IST3000_SENSOR_LEN);
	if (ret) {
		ist3000_fw_update_status = 3;
		goto end;
	}

	ret = ist3000_write_cmd(client, CMD_EXIT_UPDATE, NULL, 0);
	if (ret) {
		ist3000_fw_update_status = 3;
		goto end;
	}

	ist3000_calib_wait();
	ist3000_fw_update_status = 2;

end:
	mutex_unlock(&ist3000_mutex);
	if (mode & UPDATE_BY_BIN)
		release_firmware(request_fw);

	return size;
}


static ssize_t ist3000_sensor_show(struct device *dev, struct device_attribute *attr,
				   char *buf)
{
	int i;
	int ret;
	int count;
	u32 val;
	char msg[STATUS_MSG_LEN];
	struct ist3000_data *data = copy_data;
	struct i2c_client *client = data->client;

	/* enter reg access mode */
	ret = ist3000_write_cmd(client, CMD_ENTER_REG_ACCESS, NULL, 0);
	if (ret)
		return 0;

	msleep(40);

	count = sprintf(buf, "%s", "dump ist3000 sensor parameters\n");
	for (i = 0; i < IST3000_SENSOR_LEN; i++) {
		ist3000_read_cmd(client, IST3000_SENSOR_ADDR + (i * 4), &val, 1);
		count += snprintf(msg, STATUS_MSG_LEN, "0x%08x, ", val);
		strncat(buf, msg, STATUS_MSG_LEN);
		if (((i + 1) % 4) == 0) {
			count += snprintf(msg, STATUS_MSG_LEN, "\n");
			strncat(buf, msg, STATUS_MSG_LEN);
		}
	}

	/* exit reg access mode */
	ret = ist3000_write_cmd(client, CMD_EXIT_REG_ACCESS, NULL, 0);
	if (ret)
		return 0;

	ist3000_start(data);

	return count;
}


static ssize_t ist3000_config_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t size)
{
	int ret;
	u32 mode;
	u32 *param;
	char *after;
	const struct firmware *request_fw = NULL;
	struct ist3000_data *data = copy_data;
	struct i2c_client *client = data->client;

	mode = simple_strtoul(buf, &after, 10);

	if (mode & UPDATE_BY_BIN) {
		ret = request_firmware(&request_fw, IST3000_CONFIG_BIN, &client->dev);
		if (ret) 
			return size;		

		param = (u32 *)request_fw->data;
	} else {
		param = ist3000_config_param;
	}

	mutex_lock(&ist3000_mutex);

	ist3000_fw_update_status = 1;

	disable_irq(client->irq);
	ist3000_reset();
	enable_irq(client->irq);

	ret = ist3000_write_cmd(client, CMD_ENTER_UPDATE, NULL, 0);
	if (ret) {
		ist3000_fw_update_status = 3;
		goto end;
	}

	msleep(40);

	ret = ist3000_write_cmd(client, CMD_UPDATE_CONFIG, param, IST3000_CONFIG_LEN);
	if (ret) {
		ist3000_fw_update_status = 3;
		goto end;
	}

	ret = ist3000_write_cmd(client, CMD_EXIT_UPDATE, NULL, 0);
	if (ret) {
		ist3000_fw_update_status = 3;
		goto end;
	}

	ist3000_calib_wait();
	ist3000_fw_update_status = 2;

end:
	mutex_unlock(&ist3000_mutex);
	if (mode & UPDATE_BY_BIN)
		release_firmware(request_fw);

	return size;
}


static ssize_t ist3000_config_show(struct device *dev, struct device_attribute *attr,
				   char *buf)
{
	int i;
	int ret;
	int count;
	u32 val;
	char msg[STATUS_MSG_LEN];
	struct ist3000_data *data = copy_data;
	struct i2c_client *client = data->client;

	/* enter reg access mode */
	ret = ist3000_write_cmd(client, CMD_ENTER_REG_ACCESS, NULL, 0);
	if (ret)
		return 0;

	msleep(40);

	count = snprintf(msg, STATUS_MSG_LEN, "dump ist3000 config parameters\n");
	strncat(buf, msg, STATUS_MSG_LEN);
	for (i = 0; i < IST3000_CONFIG_LEN; i++) {
		ist3000_read_cmd(client, IST3000_CONFIG_ADDR + (i * 4), &val, 1);
		count += snprintf(msg, STATUS_MSG_LEN, "0x%08x, ", val);
		strncat(buf, msg, STATUS_MSG_LEN);
		if (((i + 1) % 4) == 0) {
			count += snprintf(msg, STATUS_MSG_LEN, "\n");
			strncat(buf, msg, STATUS_MSG_LEN);
		}
	}

	/* exit reg access mode */
	ret = ist3000_write_cmd(client, CMD_EXIT_REG_ACCESS, NULL, 0);
	if (ret)
		return 0;

	ist3000_start(data);

	return count;
}


void ist3000_set_ta_mode(bool charging)
{
#if IST3000_DETECT_TA
	if ((ist3000_ta_status == -1) || (charging == ist3000_ta_status))
		return;

	ist3000_ta_status = charging;
	schedule_delayed_work(&work_reset_check, 0);
#endif
}
EXPORT_SYMBOL(ist3000_set_ta_mode);


static void reset_work_func(struct work_struct *work)
{
	struct ist3000_data *data = copy_data;
	struct i2c_client *client = data->client;

	if ((data == NULL) || (client == NULL))
		return;

	mutex_lock(&ist3000_mutex);
	DMSG("+++ Enter reset_work_func\n");

	if ((ist3000_power_status == 1) && (ist3000_fw_update_status != 1)) {
		disable_irq(client->irq);

		clear_input_data(data);

		ist3000_reset();

		enable_irq(client->irq);

		ist3000_start(data);
	}

	DMSG("--- Exit reset_work_func\n");
	mutex_unlock(&ist3000_mutex);
}


/* sysfs : firmware */
static DEVICE_ATTR(firmware, S_IRUGO | S_IWUSR | S_IWGRP, ist3000_fw_status,
		   ist3000_fw_update);
static DEVICE_ATTR(version, S_IRUGO | S_IWUSR | S_IWGRP, ist3000_fw_version,
		   NULL);
/* sysfs : sensor */
static DEVICE_ATTR(sensor, S_IRUGO | S_IWUSR | S_IWGRP, ist3000_sensor_show,
		   ist3000_sensor_store);
/* sysfs : config */
static DEVICE_ATTR(config, S_IRUGO | S_IWUSR | S_IWGRP, ist3000_config_show,
		   ist3000_config_store);


static int __devinit ist3000_probe(struct i2c_client *		client,
				   const struct i2c_device_id * id)
{
	int ret;
	struct ist3000_data *data;
	struct input_dev *input_dev;

	printk("ist3000_probe\n");

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->num_fingers = IST3000_MAX_MT_FINGERS;

	data->client = client;
	i2c_set_clientdata(client, data);

	input_dev = input_allocate_device();
	if (!input_dev) {
		ret = -ENOMEM;
		printk(KERN_ERR "%s: input_allocate_device failed (%d)\n", __func__, ret);
		goto err_alloc_dev;
	}

	data->input_dev = input_dev;
	input_set_drvdata(input_dev, data);
	input_dev->name = "ist3000_ts_input";

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_2, input_dev->keybit);


	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, IST3000_MAX_X, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, IST3000_MAX_Y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, IST3000_MAX_Z, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, IST3000_MAX_W, 0, 0);

	ret = input_register_device(input_dev);
	if (ret) {
		input_free_device(input_dev);
		goto err_reg_dev;
	}

	copy_data = data;

	ret = ist3000_init_system();
	if (ret) {
		dev_err(&client->dev, "chip initialization failed\n");
		goto err_init_drv;
	}

	ist3000_init_touch_driver(data);

	ret = request_threaded_irq(client->irq, NULL, ist3000_irq_thread,
				   IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "ist3000_ts", data);
	if (ret)
		goto err_irq;

	/* /sys/class/touch */
	ist3000_class = class_create(THIS_MODULE, "touch");

	/* /sys/class/touch/firmware */
	ist3000_firm_dev = device_create(ist3000_class, NULL, 0, NULL, "firmware");

	/* /sys/class/touch/firmwware/firmware */
	if (device_create_file(ist3000_firm_dev, &dev_attr_firmware) < 0)
		printk(KERN_ERR "[TSP] Failed to create device file(%s)!\n",
		       dev_attr_firmware.attr.name);

	/* /sys/class/touch/firmware/version */
	if (device_create_file(ist3000_firm_dev, &dev_attr_version) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_version.attr.name);

	/* /sys/class/touch/parameters */
	ist3000_param_dev = device_create(ist3000_class, NULL, 0, NULL, "parameters");

	/* /sys/class/touch/parameters/sensor */
	if (device_create_file(ist3000_param_dev, &dev_attr_sensor) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_config.attr.name);

	/* /sys/class/touch/parameters/config */
	if (device_create_file(ist3000_param_dev, &dev_attr_config) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		       dev_attr_config.attr.name);

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = ist3000_early_suspend;
	data->early_suspend.resume = ist3000_late_resume;
	register_early_suspend(&data->early_suspend);
#endif

	INIT_DELAYED_WORK(&work_reset_check, reset_work_func);

#if IST3000_DETECT_TA
	ist3000_ta_status = 0;
#endif

	return 0;

err_irq:
err_init_drv:
	ist3000_power_off();
	input_unregister_device(input_dev);

err_reg_dev:
err_alloc_dev:
	kfree(data);
	return ret;
	return 0;
}


static int __devexit ist3000_remove(struct i2c_client *client)
{
	struct ist3000_data *data = i2c_get_clientdata(client);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif

	free_irq(client->irq, data);
	ist3000_power_off();

	input_unregister_device(data->input_dev);
	kfree(data);

	return 0;
}


static struct i2c_device_id ist3000_idtable[] = {
	{ IST3000_DEV_NAME, 0 },
	{},
};


MODULE_DEVICE_TABLE(i2c, ist3000_idtable);

static const struct dev_pm_ops ist3000_pm_ops = {
	.suspend	= ist3000_suspend,
	.resume		= ist3000_resume,
};


static struct i2c_driver ist3000_i2c_driver = {
	.id_table	= ist3000_idtable,
	.probe		= ist3000_probe,
	.remove		= __devexit_p(ist3000_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= IST3000_DEV_NAME,
		.pm	= &ist3000_pm_ops,
	},
};


static int __init ist3000_init(void)
{
	printk("ist3000_init\n");
	return i2c_add_driver(&ist3000_i2c_driver);
}


static void __exit ist3000_exit(void)
{
	i2c_del_driver(&ist3000_i2c_driver);
}

module_init(ist3000_init);
module_exit(ist3000_exit);

MODULE_DESCRIPTION("Imagis IST3000 touch driver");
MODULE_LICENSE("GPL");
