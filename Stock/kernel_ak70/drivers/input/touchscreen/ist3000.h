/*
 *  Copyright (C) 2010, Imagis Technology Co. Ltd. All Rights Reserved.
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

#ifndef __IST3000_H__
#define __IST3000_H__

#define IST3000_DEV_NAME            "IST3000"

#define IST3000_DEBUG               (0)
#define IST3000_DETECT_TA           (0)

#define IST3000_DEV_ID              (0xA0 >> 1)
#define IST3000_FW_DEV_ID           (0xA4 >> 1)

#define IST3000_MAX_X               (320)
#define IST3000_MAX_Y               (480)
#define IST3000_MAX_Z               (255)
#define IST3000_MAX_W               (15)

#if IST3000_DEBUG
#define DMSG(x ...) printk(x)
#else
#define DMSG(x ...)
#endif

/* ist3000_sys.c */
extern int ist3000_power_status;

int ist3000_read_cmd(struct i2c_client *client, u32 cmd, u32 *buf, u16 len);
int ist3000_write_cmd(struct i2c_client *client, u32 cmd, u32 *buf, u16 len);

int ist3000_power_on(void);
int ist3000_power_off(void);
int ist3000_reset(void);
int __devinit ist3000_init_system(void);

int ist3000_fw_cmd(struct i2c_client *client, const u8 cmd);
int ist3000_fw_write(struct i2c_client *client, const u8 *buf, u32 len);
int ist3000_fw_read(struct i2c_client *client, u32 len, u8 *buf);
int ist3000_fw_compare(struct i2c_client *client, const u8 *src, u32 len);
int ist3000_fw_erase_all(struct i2c_client *client);

#endif
