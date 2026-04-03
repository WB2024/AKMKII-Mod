/*
 * wm8740.h  --  WM8740 Soc Audio driver
 *
 * Copyright 2005 Openedhand Ltd.
 *
 * Author: Richard Purdie <richard@openedhand.com>
 *
 * Based on wm8740.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _WM8740_H
#define _WM8740_H

/* WM8740 register space */

#define REG_WM8740_LINVOL			0x00
#define REG_WM8740_RINVOL			0x01
#define REG_WM8740_LOUT1V			0x02
#define REG_WM8740_ROUT1V			0x03
#define REG_WM8740_APANA			0x04
#define REG_WM8740_APDIGI			0x05
#define REG_WM8740_PWR			0x06
#define REG_WM8740_IFACE			0x07
#define REG_WM8740_SRATE			0x08
#define REG_WM8740_ACTIVE			0x09
#define REG_WM8740_RESET			0x0f

#define WM8740_CACHEREGNUM 	10

#define WM8740_SYSCLK	0
#define WM8740_DAI		0


#endif
