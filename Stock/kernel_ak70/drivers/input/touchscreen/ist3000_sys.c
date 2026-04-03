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
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/unaligned.h>

#include <mach/reg_physical.h>
#include <mach/irqs.h>
#include <mach/structures_smu_pmu.h>
#include <mach/gpio.h>
#include <mach/io.h>

#include "ist3000.h"

/* firmware commands */
#define CMD_FW_BEGIN_READ           (0x11)
#define CMD_FW_END_READ             (0x10)
#define CMD_FW_ERASE_ALL_PAGE       (0x8F)
#define CMD_FW_PROGRAM_PAGE         (0x91)

#define IST3000_ADDR_LEN            (4)
#define IST3000_DATA_LEN            (4)

#define EEPROM_PAGE_SIZE            (64)
#define EEPROM_BASE_ADDR            (0)

#define WRITE_CMD_MSG_LEN           (1)
#define READ_CMD_MSG_LEN            (2)

int ist3000_power_status = -1;


int ist3000_read_cmd(struct i2c_client *client, u32 cmd, u32 *buf, u16 len)
{
	int ret, i;
	u32 le_reg = cpu_to_be32(cmd);

	struct i2c_msg msg[READ_CMD_MSG_LEN] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = IST3000_ADDR_LEN,
			.buf = (u8 *)&le_reg,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = len * IST3000_DATA_LEN,
			.buf = (u8 *)buf,
		},
	}; 

	ret = i2c_transfer(client->adapter, &msg, READ_CMD_MSG_LEN);
//	if (ret != READ_CMD_MSG_LEN) {
	if (ret < 0) {
		printk(KERN_ERR "%s: i2c_transfer failed (%d)\n", __func__, ret);
		return -EIO;
	}

	for (i = 0; i < len; i++)
		buf[i] = cpu_to_be32(buf[i]);

	return 0;
}


int ist3000_write_cmd(struct i2c_client *client, u32 cmd, u32 *buf, u16 len)
{
	int i;
	int ret;
	struct i2c_msg msg;
	u8 msg_buf[IST3000_ADDR_LEN + IST3000_DATA_LEN * (len + 1)];

	put_unaligned_be32(cmd, msg_buf);

	if (len > 0) {
		for (i = 0; i < len; i++)
			put_unaligned_be32(buf[i], msg_buf + IST3000_ADDR_LEN +
					   (i * IST3000_DATA_LEN));
	} else {
		/* then add dummy data(4byte) */
		put_unaligned_be32(0, msg_buf + IST3000_ADDR_LEN);
		len = 1;
	}

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = IST3000_ADDR_LEN + (IST3000_DATA_LEN * len);
	msg.buf = msg_buf;

	ret = i2c_transfer(client->adapter, &msg, WRITE_CMD_MSG_LEN);
	if (ret != WRITE_CMD_MSG_LEN) {
		printk(KERN_ERR "%s: ist3000_write_cmd failed (%d)\n", __func__, ret);
		return -EIO;
	}

	return 0;
}


int ist3000_power_on(void)
{
	volatile PGPIO pGPIO = (volatile PGPIO)tcc_p2v(HwGPIO_BASE);

	printk("ist3000_power_on\n");
	if (ist3000_power_status != 1) {
		pGPIO->GPFDAT.bREG.GP00 = 1;
		pGPIO->GPFDAT.bREG.GP01 = 1;

		ist3000_power_status = 1;
		printk("[>>> IST30xx] ist3000_power_on is okay\n");
	}

	return 0;
}


int ist3000_power_off(void)
{
	volatile PGPIO pGPIO = (volatile PGPIO)tcc_p2v(HwGPIO_BASE);

	printk("ist3000_power_off\n");
	if (ist3000_power_status != 0) {
		pGPIO->GPFDAT.bREG.GP00 = 0;
		pGPIO->GPFDAT.bREG.GP01 = 0;

		ist3000_power_status = 0;
		printk("[>>> IST30xx] ist3000_power_off is okay\n");
	}

	return 0;
}


int ist3000_reset(void)
{
	volatile PGPIO pGPIO = (volatile PGPIO)tcc_p2v(HwGPIO_BASE);

	pGPIO->GPFDAT.bREG.GP00 = 0;
	msleep(100);
	pGPIO->GPFDAT.bREG.GP00 = 1;
	msleep(1000);

	printk("[>>> IST30xx] ist3000_reset is okay\n");
	return 0;
}


int ist3000_init_system(void)
{
	volatile PGPIO pGPIO = (volatile PGPIO)tcc_p2v(HwGPIO_BASE);
	volatile PPIC pPIC = (volatile PPIC)tcc_p2v(HwPIC_BASE);
	int ret;
	unsigned gpio = 0;
	unsigned flag = 0;

	printk("ist3000_init_system\n");

	tcc_gpio_config(TCC_GPF(0), GPIO_OUTPUT | GPIO_FN0);		// TOUCH_RST
	tcc_gpio_config(TCC_GPF(1), GPIO_OUTPUT | GPIO_FN0);		// TSP_PWEN
	tcc_gpio_config(TCC_GPB(11), GPIO_INPUT | GPIO_FN0);

	/* set external interrupt for touch-screen */
	tcc_gpio_config_ext_intr(INT_EINT11, EXTINT_GPIOB_11);

	pPIC->MODEA0.bREG.EINT11 = 0;		/* single edge */
	pPIC->MODE0.bREG.EINT11 = 0;		/* set edge trigger mode */
	pPIC->POL0.bREG.EINT11 = 1;			/* active-low */
	pPIC->CLR0.bREG.EINT11 = 1;			/* Clear pending status */
	pPIC->IEN0.bREG.EINT11 = 1;			/* Enable Interrupt */
	pPIC->INTMSK0.bREG.EINT11 = 0;		/* turn off the backlight to enable */

	ret = ist3000_power_on();
	if (ret) {
		printk(KERN_ERR "%s: ist3000_power_on failed (%d)\n", __func__, ret);
		return -EIO;
	}

	ret = ist3000_reset();
	if (ret) {
		printk(KERN_ERR "%s: ist3000_reset failed (%d)\n", __func__, ret);
		return -EIO;
	}

	return 0;
}


static void prepare_msg_buffer(u8 *buf, u8 cmd, u16 addr)
{
	memset(buf, 0, sizeof(buf));

	buf[0] = cmd;
	buf[1] = (u8)((addr >> 8) & 0xFF);
	buf[2] = (u8)(addr & 0xFF);
}


int ist3000_fw_cmd(struct i2c_client *client, const u8 cmd)
{
	int ret;
	struct i2c_msg msg;

	msg.addr = IST3000_FW_DEV_ID;
	msg.flags = I2C_M_IGNORE_NAK;
	msg.len = 1;
	msg.buf = (u8 *)&cmd;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret != 1)
		return -EIO;

	/* need 2.5msec after this cmd */
	msleep(3);

	return 0;
}


int ist3000_fw_write(struct i2c_client *client, const u8 *buf, u32 len)
{
	int ret, i;
	struct i2c_msg msg;
	u32 fw_len, page_cnt;
	u16 addr;
	u8 msg_buf[EEPROM_PAGE_SIZE + 3];

	if (len % EEPROM_PAGE_SIZE)
		fw_len = (len / EEPROM_PAGE_SIZE + 1) * EEPROM_PAGE_SIZE;
	else
		fw_len = len;

	addr = EEPROM_BASE_ADDR;
	page_cnt = fw_len / EEPROM_PAGE_SIZE;

	for (i = 0; i < page_cnt; i++) {
		prepare_msg_buffer(msg_buf, CMD_FW_PROGRAM_PAGE, addr);

		memcpy(msg_buf + 3, buf, EEPROM_PAGE_SIZE);

		msg.addr = IST3000_FW_DEV_ID;
		msg.flags = I2C_M_IGNORE_NAK;
		msg.len = EEPROM_PAGE_SIZE + 3;
		msg.buf = msg_buf;

		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret != 1)
			return -EIO;

		addr += EEPROM_PAGE_SIZE;
		buf += EEPROM_PAGE_SIZE;

		msleep(2);
	}

	return 0;
}


int ist3000_fw_read(struct i2c_client *client, u32 len, u8 *buf)
{
	int ret, i;
	u16 addr = EEPROM_BASE_ADDR;
	u8 msg_buf[3]; /* Command + Dummy 16bit addr */
	struct i2c_msg msg;
	u32 fw_len, page_cnt;

	prepare_msg_buffer(msg_buf, CMD_FW_BEGIN_READ, addr);
	msg.addr = IST3000_FW_DEV_ID;
	msg.flags = I2C_M_IGNORE_NAK;
	msg.len = 3;
	msg.buf = msg_buf;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret != 1)
		return -EIO;

	if (len % EEPROM_PAGE_SIZE)
		fw_len = (len / EEPROM_PAGE_SIZE + 1) * EEPROM_PAGE_SIZE;
	else
		fw_len = len;

	page_cnt = fw_len / EEPROM_PAGE_SIZE;

	for (i = 0; i < page_cnt; i++) {
		msg.addr = IST3000_FW_DEV_ID;
		msg.flags = I2C_M_RD | I2C_M_IGNORE_NAK;
		msg.len = EEPROM_PAGE_SIZE;
		msg.buf = buf;

		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret != 1)
			return -EIO;

		addr += EEPROM_PAGE_SIZE;
		buf += EEPROM_PAGE_SIZE;
	}

	prepare_msg_buffer(msg_buf, CMD_FW_END_READ, addr);
	msg.addr = IST3000_FW_DEV_ID;
	msg.flags = I2C_M_IGNORE_NAK;
	msg.len = 3;
	msg.buf = msg_buf;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret != 1)
		return -EIO;

	return 0;
}


int ist3000_fw_compare(struct i2c_client *client, const u8 *src, u32 len)
{
	int ret, i;
	u8 *target;

	target = kzalloc(len, GFP_KERNEL);
	if (!target)
		return -ENOMEM;

	ret = ist3000_fw_read(client, len, target);
	if (ret) {
		kfree(target);
		return -EIO;
	}

	for (i = 0; i < len; i++) {
		if (src[i] != target[i]) {
			printk(KERN_ERR "compare Error [%d],  [%x : %x]\n", i, src[i], target[i]);
			kfree(target);
			return -EIO;
		}
	}

	kfree(target);

	return 0;
}


int ist3000_fw_erase_all(struct i2c_client *client)
{
	int ret;
	struct i2c_msg msg;

	/* EEPROM pase size + Command + Dummy 16bit addr */
	u8 msg_buf[EEPROM_PAGE_SIZE + 3];

	prepare_msg_buffer(msg_buf, CMD_FW_ERASE_ALL_PAGE, EEPROM_BASE_ADDR);

	msg.addr = IST3000_FW_DEV_ID;
	msg.flags = I2C_M_IGNORE_NAK;
	msg.len = EEPROM_PAGE_SIZE + 3;
	msg.buf = (u8 *)msg_buf;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret != 1)
		return -EIO;

	/* need 2.5msec after erasing eeprom */
	msleep(3);

	return 0;
}
