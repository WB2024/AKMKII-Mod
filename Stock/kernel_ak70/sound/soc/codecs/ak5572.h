/*
 * ak5572.h  --  audio driver for ak5572
 *
 * Copyright (C) 2015 Asahi Kasei Microdevices Corporation
 *  Author                Date        Revision
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                      15/10/21    1.0
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef _AK5572_H
#define _AK5572_H

#define AK5572_00_POWER_MANAGEMENT1    0x00
#define AK5572_01_POWER_MANAGEMENT2    0x01
#define AK5572_02_CONTROL1             0x02
#define AK5572_03_CONTROL2             0x03
#define AK5572_04_CONTROL3             0x04
#define AK5572_05_DSD                  0x05

#define AK5572_MAX_REGISTERS	(AK5572_05_DSD + 1)

/* Bitfield Definitions */

/* AK5572_02_CONTROL1 (0x03) Fields */
#define AK5572_DIF					0x04
#define AK5572_DIF_MSB_MODE	       (0 << 2)
#define AK5572_DIF_I2S_MODE		   (1 << 2)

#define AK5572_BITS					0x02
#define AK5572_DIF_24BIT_MODE	   (0 << 1)
#define AK5572_DIF_32BIT_MODE      (1 << 1)

#define AK5572_CKS					0x78
#define AK5572_CKS_128FS_192KHZ	    (0 << 3)
#define AK5572_CKS_192FS_192KHZ	    (1 << 3)
#define AK5572_CKS_256FS_48KHZ      (2 << 3)
#define AK5572_CKS_256FS_96KHZ	    (3 << 3)
#define AK5572_CKS_384FS_96KHZ	    (4 << 3)
#define AK5572_CKS_384FS_48KHZ	    (5 << 3)
#define AK5572_CKS_512FS_48KHZ	    (6 << 3)
#define AK5572_CKS_768FS_48KHZ	    (7 << 3)
#define AK5572_CKS_64FS_384KHZ	    (8 << 3)
#define AK5572_CKS_32FS_768KHZ	    (9 << 3)
#define AK5572_CKS_96FS_384KHZ	    (10 << 3)
#define AK5572_CKS_48FS_768KHZ	    (11 << 3)
#define AK5572_CKS_64FS_768KHZ	    (11 << 3)
#define AK5572_CKS_1024FS_8KHZ	    (12 << 3)
#define AK5572_CKS_AUTO	            (15 << 3)

#endif

