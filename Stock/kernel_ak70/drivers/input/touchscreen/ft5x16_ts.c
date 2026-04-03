/* 
 * drivers/input/touchscreen/ft5x16_ts.c
 *
 * FocalTech ft5x16 TouchScreen driver. 
 *
 * Copyright (c) 2010  Focal tech Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * VERSION      	DATE			AUTHOR        Note
 *    1.0		  2010-01-05			WenFS    only support mulititouch	Wenfs 2010-10-01
 *    2.0          2011-09-05                   Duxx      Add touch key, and project setting update, auto CLB command
 *    3.0		  2011-09-09			Luowj   
 *
 */

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>   
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>

#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/timer.h>

#include <mach/reg_physical.h>
#include <mach/irqs.h>
#include <mach/structures_smu_pmu.h>
#include <mach/gpio.h>
#include <mach/io.h>

#include "ft5x16_ts.h"

#if(CUR_AK_REV >= AK70_HW_WS)
void enable_tsp_pwen(int enable);
#endif


static struct i2c_client *this_client;

/* [downer] A131007 for touch-key */
#define TOUCH_RELEASE_TIMEOUT         (jiffies + 5)

#define CONFIG_FT5X16_MULTITOUCH 1

struct ts_event {
    u16 au16_x[CFG_MAX_TOUCH_POINTS];              //x coordinate
    u16 au16_y[CFG_MAX_TOUCH_POINTS];              //y coordinate
    u8  au8_touch_event[CFG_MAX_TOUCH_POINTS];     //touch event:  0 -- down; 1-- contact; 2 -- contact
    u8  au8_finger_id[CFG_MAX_TOUCH_POINTS];       //touch ID
	u16	pressure;
    u8  touch_point;
};

struct ft5x16_ts_data {
    struct input_dev	*input_dev;
	struct ts_event		event;
	struct work_struct 	pen_event_work;
	struct workqueue_struct *ts_workqueue;
	struct early_suspend	early_suspend;
    struct mutex device_mode_mutex;   /* Ensures that only one function can specify the Device Mode at a time. */
    struct timer_list release_timer;
};

//register address
#define FT5x16_REG_FW_VER 0xA6

#define FT5x16_TX_NUM	28
#define FT5x16_RX_NUM   16
//u16 g_RawData[FT5x0x_TX_NUM][FT5x16_RX_NUM];
static u8 ft5x16_enter_factory(struct ft5x16_ts_data *ft5x16_ts);
static u8 ft5x16_enter_work(struct ft5x16_ts_data *ft5x16_ts);

static int g_ts_id = 0;

enum {
    FT5316 = 0,
    FT5336,
};
    
// [downer] A131219
static int g_release_not_yet = 0;

#ifdef CFG_SUPPORT_TOUCH_KEY
int tsp_keycodes[CFG_NUMOFKEYS] ={
    KEY_HOMEPAGE,
};

char *tsp_keyname[CFG_NUMOFKEYS] ={
    "Home",
};

static int tsp_keystatus = KEY_RELEASE;
#endif

#ifdef USE_VWR_TYPE1

int is_touch_pressed(void)
{
	return g_release_not_yet;
}
#endif

static unsigned int tsp_ignore_cnt = 0;

static int ft5x16_i2c_rxdata(char *rxdata, int length)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rxdata,
		},
		{
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= length,
			.buf	= rxdata,
		},
	};

    //msleep(1);
	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);
	
	return ret;
}

static int ft5x16_i2c_txdata(char *txdata, int length)
{
	int ret;

	struct i2c_msg msg[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= length,
			.buf	= txdata,
		},
	};

   	//msleep(1);
	ret = i2c_transfer(this_client->adapter, msg, 1);
	if (ret < 0)
		pr_err("%s i2c write error: %d\n", __func__, ret);

	return ret;
}

static int ft5x16_write_reg(u8 addr, u8 para)
{
    u8 buf[3];
    int ret = -1;

    buf[0] = addr;
    buf[1] = para;
    ret = ft5x16_i2c_txdata(buf, 2);
    if (ret < 0) {
        pr_err("write reg failed! %#x ret: %d", buf[0], ret);
        return -1;
    }
    
    return 0;
}

static int ft5x16_read_reg(u8 addr, u8 *pdata)
{
	int ret;
	u8 buf[2];
	struct i2c_msg msgs[2];


	buf[0] = addr;    //register address
	
	msgs[0].addr = this_client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = buf;
	msgs[1].addr = this_client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 1;
	msgs[1].buf = buf;

	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

	*pdata = buf[0];
	return ret;
  
}

static unsigned char ft5x16_read_fw_ver(void)
{
	unsigned char ver;
    
	ft5x16_read_reg(FT5X16_REG_FIRMID, &ver);
	return(ver);
}

#if 1  //upgrade related
typedef enum
{
    ERR_OK,
    ERR_MODE,
    ERR_READID,
    ERR_ERASE,
    ERR_STATUS,
    ERR_ECC,
    ERR_DL_ERASE_FAIL,
    ERR_DL_PROGRAM_FAIL,
    ERR_DL_VERIFY_FAIL
}E_UPGRADE_ERR_TYPE;

typedef unsigned char         FTS_BYTE;     //8 bit
typedef unsigned short        FTS_WORD;    //16 bit
typedef unsigned int          FTS_DWRD;    //16 bit
typedef unsigned char         FTS_BOOL;    //8 bit

typedef struct _FTS_CTP_PROJECT_SETTING_T
{
    unsigned char uc_i2C_addr;             //I2C slave address (8 bit address)
    unsigned char uc_io_voltage;           //IO Voltage 0---3.3v;	1----1.8v
    unsigned char uc_panel_factory_id;     //TP panel factory ID
}FTS_CTP_PROJECT_SETTING_T;

#define FTS_NULL                0x0
#define FTS_TRUE                0x01
#define FTS_FALSE              0x0

#define I2C_CTPM_ADDRESS       0x70

static void ft5x16_power_on(void)
{
  // comment : on AK70 ES mute issue after suspend.
   // volatile PGPIO pGPIO = (volatile PGPIO)tcc_p2v(HwGPIO_BASE);
  //  pGPIO->GPFDAT.bREG.GP01 = 1;
 #if(CUR_AK_REV >= AK70_HW_WS)
	enable_tsp_pwen(1);
#endif

#if(CUR_AK_REV == AK70_HW_EVM)
	gpio_set_value(GPIO_TSP_PWEN, 1);
#endif

}

static void ft5x16_power_off(void)
{
    printk("%s\n", __func__);


#if(CUR_AK_REV >= AK70_HW_WS)
	enable_tsp_pwen(0);
#endif

#if(CUR_AK_REV == AK70_HW_EVM)
	gpio_set_value(GPIO_TSP_PWEN, 0);
#endif

	gpio_set_value(GPIO_TOUCH_RST, 0);
}

static void ft5x16_reset(void)
{   
    printk("%s\n", __func__);

	gpio_set_value(GPIO_TOUCH_RST, 0);
    msleep_interruptible(10);
	gpio_set_value(GPIO_TOUCH_RST, 1);
    msleep_interruptible(10); 
}

void delay_qt_ms(unsigned long  w_ms)
{
    unsigned long i;
    unsigned long j;

    for (i = 0; i < w_ms; i++) {
        for (j = 0; j < 1000; j++)
            udelay(1);
    }
}

/*
[function]: 
    callback: read data from ctpm by i2c interface,implemented by special user;
[parameters]:
    bt_ctpm_addr[in]    :the address of the ctpm;
    pbt_buf[out]        :data buffer;
    dw_lenth[in]        :the length of the data buffer;
[return]:
    FTS_TRUE     :success;
    FTS_FALSE    :fail;
*/
FTS_BOOL i2c_read_interface(FTS_BYTE bt_ctpm_addr, FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
{
    int ret;
    
    ret=i2c_master_recv(this_client, pbt_buf, dw_lenth);

    if(ret<=0) {
        printk("[FTS]i2c_read_interface error\n");
        return FTS_FALSE;
    }
  
    return FTS_TRUE;
}

/*
[function]: 
    callback: write data to ctpm by i2c interface,implemented by special user;
[parameters]:
    bt_ctpm_addr[in]    :the address of the ctpm;
    pbt_buf[in]        :data buffer;
    dw_lenth[in]        :the length of the data buffer;
[return]:
    FTS_TRUE     :success;
    FTS_FALSE    :fail;
*/
FTS_BOOL i2c_write_interface(FTS_BYTE bt_ctpm_addr, FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
{
    int ret;
    
    ret=i2c_master_send(this_client, pbt_buf, dw_lenth);
    if (ret <= 0) {
        printk("[FTS]i2c_write_interface error line = %d, ret = %d\n", __LINE__, ret);
        return FTS_FALSE;
    }

    return FTS_TRUE;
}

/*
[function]: 
    send a command to ctpm.
[parameters]:
    btcmd[in]        :command code;
    btPara1[in]    :parameter 1;    
    btPara2[in]    :parameter 2;    
    btPara3[in]    :parameter 3;    
    num[in]        :the valid input parameter numbers, if only command code needed and no parameters followed,then the num is 1;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
FTS_BOOL cmd_write(FTS_BYTE btcmd,FTS_BYTE btPara1,FTS_BYTE btPara2,FTS_BYTE btPara3,FTS_BYTE num)
{
    FTS_BYTE write_cmd[4] = {0};

    write_cmd[0] = btcmd;
    write_cmd[1] = btPara1;
    write_cmd[2] = btPara2;
    write_cmd[3] = btPara3;
    
    return i2c_write_interface(I2C_CTPM_ADDRESS, write_cmd, num);
}

/*
[function]: 
    write data to ctpm , the destination address is 0.
[parameters]:
    pbt_buf[in]    :point to data buffer;
    bt_len[in]        :the data numbers;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
FTS_BOOL byte_write(FTS_BYTE* pbt_buf, FTS_DWRD dw_len)
{
    return i2c_write_interface(I2C_CTPM_ADDRESS, pbt_buf, dw_len);
}

/*
[function]: 
    read out data from ctpm,the destination address is 0.
[parameters]:
    pbt_buf[out]    :point to data buffer;
    bt_len[in]        :the data numbers;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
FTS_BOOL byte_read(FTS_BYTE* pbt_buf, FTS_BYTE bt_len)
{
    return i2c_read_interface(I2C_CTPM_ADDRESS, pbt_buf, bt_len);
}

/*
[function]: 
    burn the FW to ctpm.
[parameters]:(ref. SPEC)
    pbt_buf[in]    :point to Head+FW ;
    dw_lenth[in]:the length of the FW + 6(the Head length);    
    bt_ecc[in]    :the ECC of the FW
[return]:
    ERR_OK        :no error;
    ERR_MODE    :fail to switch to UPDATE mode;
    ERR_READID    :read id fail;
    ERR_ERASE    :erase chip fail;
    ERR_STATUS    :status error;
    ERR_ECC        :ecc error.
*/
#define FTS_PACKET_LENGTH 128

static unsigned char CTPM_5X16_FW[]= {
	#include "ft5x16_firmware.i"    
};

static unsigned char CTPM_5X36_FW[]= {
	#include "ft5x36_firmware.i"
};

E_UPGRADE_ERR_TYPE  fts_ctpm_fw_upgrade(FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
{
    FTS_BYTE reg_val[2] = {0};
    FTS_DWRD i = 0, loop = 0;

    FTS_DWRD  packet_number;
    FTS_DWRD  j;
    FTS_DWRD  temp;
    FTS_DWRD  lenght;
    FTS_BYTE  packet_buf[FTS_PACKET_LENGTH + 6];
    FTS_BYTE  auc_i2c_write_buf[10];
    FTS_BYTE bt_ecc, bl_version = 0;
    int      i_ret;
    
    ft5x16_reset();

    for (loop = 0; loop < FTS_UPGRADE_LOOP; loop++) {
        /*********Step 1:Reset  CTPM *****/
        /*write 0xaa to register 0xfc*/
        ft5x16_write_reg(0xfc, 0xaa);

        msleep_interruptible(50); // 50    
        
         /*write 0x55 to register 0xfc*/
        ft5x16_write_reg(0xfc, 0x55);
        printk("[FTS] Step 1: Reset CTPM test\n");   

        msleep_interruptible(50); // 50    

        /*********Step 2:Enter upgrade mode *****/
        auc_i2c_write_buf[0] = 0x55;
        auc_i2c_write_buf[1] = 0xaa;
        do {
            i++;
            i_ret = ft5x16_i2c_txdata(auc_i2c_write_buf, 2);
        } while (i_ret <= 0 && i < 5 );

        /*********Step 3:check READ-ID***********************/        
        cmd_write(0x90, 0x00, 0x00, 0x00, 4);
        byte_read(reg_val, 2);

        if (g_ts_id == FT5316) {
            if (reg_val[0] == 0x79 && reg_val[1] == 0x7) {
                printk("[FT5316] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n", reg_val[0], reg_val[1]);
                break;
            }
            else 
                printk("[FT5316] ERR_READID: byte_read [0]: 0x%x [1]: 0x%x\n", reg_val[0], reg_val[1]);
        }
        else { // FT5336
            if (reg_val[0] == 0x79 && reg_val[1] == 0x11) {
                printk("[FT5336] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n", reg_val[0], reg_val[1]);
                break;
            }
            else 
                printk("[FT5336] ERR_READID: byte_read [0]: 0x%x [1]: 0x%x\n", reg_val[0], reg_val[1]);
        }

    }

    if (loop >= FTS_UPGRADE_LOOP)
        return ERR_READID;

    /* [downer] A141119 check BL compression type */
    cmd_write(0xcd, 0x00, 0x00, 0x00, 1);
    byte_read(reg_val, 1);
    printk("[FTS] bootloader version = %d\n", reg_val[0]);
    if (reg_val[0] <= 4)
        bl_version = BL_VERSION_LZ4;
    else if (reg_val[0] == 7)
        bl_version = BL_VERSION_Z7;
    else if (reg_val[0] >= 15)
        bl_version = BL_VERSION_GZF;
    
     /*********Step 4:erase app and panel paramenter area ********************/
    cmd_write(0x61, 0x00, 0x00, 0x00, 1);  //erase app area
    delay_qt_ms(1500); 
    cmd_write(0x63, 0x00, 0x00, 0x00, 1);  //erase panel parameter area
    delay_qt_ms(100);
    printk("[FTS] Step 4: erase. \n");

    /*********Step 5:write firmware(FW) to ctpm flash*********/
    bt_ecc = 0;
    printk("[FTS] Step 5: start upgrade. \n");
//    if (bl_version == BL_VERSION_GZF)
    if (g_ts_id == FT5336)
        dw_lenth = dw_lenth - 14;
    else 
        dw_lenth = dw_lenth - 8;
    
    packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
    packet_buf[0] = 0xbf;
    packet_buf[1] = 0x00;
    for (j = 0; j < packet_number; j++) {
        temp = j * FTS_PACKET_LENGTH;
        
        packet_buf[2] = (FTS_BYTE)(temp >> 8);
        packet_buf[3] = (FTS_BYTE)temp;
        lenght = FTS_PACKET_LENGTH;
        packet_buf[4] = (FTS_BYTE)(lenght >> 8);
        packet_buf[5] = (FTS_BYTE)lenght;

        for (i=0;i<FTS_PACKET_LENGTH;i++) {
            packet_buf[6+i] = pbt_buf[j*FTS_PACKET_LENGTH + i]; 
            bt_ecc ^= packet_buf[6+i];
        }
        
        byte_write(&packet_buf[0],FTS_PACKET_LENGTH + 6);
        delay_qt_ms(FTS_PACKET_LENGTH / 6 + 1);
        if ((j * FTS_PACKET_LENGTH % 1024) == 0) {
              printk("[FTS] upgrade the 0x%x th byte.\n", ((unsigned int)j) * FTS_PACKET_LENGTH);
        }
    }

    if ((dw_lenth) % FTS_PACKET_LENGTH > 0) {
        temp = packet_number * FTS_PACKET_LENGTH;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;

        temp = (dw_lenth) % FTS_PACKET_LENGTH;
        packet_buf[4] = (FTS_BYTE)(temp>>8);
        packet_buf[5] = (FTS_BYTE)temp;

        for (i = 0;i < temp; i++) {
            packet_buf[6+i] = pbt_buf[ packet_number*FTS_PACKET_LENGTH + i]; 
            bt_ecc ^= packet_buf[6+i];
        }

        byte_write(&packet_buf[0],temp+6);    
        delay_qt_ms(20);
    }

//    if (bl_version == BL_VERSION_GZF)  {   
    if (g_ts_id == FT5336) {
        //send the last 12 byte        
        for (i = 0; i < 12; i++) {        
            temp = 0x7ff4 + i;        
            packet_buf[2] = (FTS_BYTE)(temp>>8);
            packet_buf[3] = (FTS_BYTE)temp;
            temp =1;
            packet_buf[4] = (FTS_BYTE)(temp>>8);
            packet_buf[5] = (FTS_BYTE)temp;
            packet_buf[6] = pbt_buf[ dw_lenth + i]; 
            bt_ecc ^= packet_buf[6];

            byte_write(&packet_buf[0],7);  
            delay_qt_ms(10);
        }
    }
    else {
        //send the last six byte
        for (i = 0; i < 6; i++) {
            temp = 0x6ffa + i;
            packet_buf[2] = (FTS_BYTE)(temp>>8);
            packet_buf[3] = (FTS_BYTE)temp;
            temp =1;
            packet_buf[4] = (FTS_BYTE)(temp>>8);
            packet_buf[5] = (FTS_BYTE)temp;
            packet_buf[6] = pbt_buf[ dw_lenth + i]; 
            bt_ecc ^= packet_buf[6];

            byte_write(&packet_buf[0],7);  
            delay_qt_ms(30);
        }
    }
    
    /*********Step 6: read out checksum***********************/
    /*send the opration head*/
    cmd_write(0xcc,0x00,0x00,0x00,1);
    byte_read(reg_val,1);
    printk("[FTS] Step 6:  ecc read 0x%x, new firmware 0x%x. \n", reg_val[0], bt_ecc);
    if(reg_val[0] != bt_ecc) {
        return ERR_ECC;
    }

    /*********Step 7: reset the new FW***********************/
    cmd_write(0x07,0x00,0x00,0x00,1);
    printk("[FTS] Step 7:  reset new FW\n");
    
    msleep(300);  //make sure CTP startup normally
    
    return ERR_OK;
}

static int fts_Get_RawData(u16 RawData[][FT5x16_RX_NUM])
{
	int retval  = 0;
	int i       = 0;
//	u16 dataval = 0x0000;
	u8  devmode = 0x00;
	u8  rownum  = 0x00;

	u8 read_buffer[FT5x16_RX_NUM * 2];
	u8 read_len = 0;
	//u8 write_buffer[2];
	struct ft5x16_ts_data * ft5x16_ts =  i2c_get_clientdata(this_client);
    struct i2c_msg msgs[1];
    
	if (ft5x16_enter_factory(ft5x16_ts) < 0) {
		pr_err("%s ERROR: could not enter factory mode", __FUNCTION__);
		retval = -1;
		goto error_return;
	}
	//scan
	if (ft5x16_read_reg(0x00, &devmode) < 0) {
		pr_err("%s %d ERROR: could not read register 0x00", __FUNCTION__, __LINE__);
		retval = -1;
		goto error_return;
	}

	devmode |= 0x80;
	if (ft5x16_write_reg(0x00, devmode) < 0) {
		pr_err("%s %d ERROR: could not read register 0x00", __FUNCTION__, __LINE__);
		retval = -1;
		goto error_return;
	}
	msleep(20);
	if (ft5x16_read_reg(0x00, &devmode) < 0) {
		pr_err("%s %d ERROR: could not read register 0x00", __FUNCTION__, __LINE__);
		retval = -1;
		goto error_return;
	}
    
	if (0x00 != (devmode&0x80)) {
		pr_err("%s %d ERROR: could not scan", __FUNCTION__, __LINE__);
		retval = -1;
		goto error_return;
	}
    
	pr_info("Read rawdata .......\n");
	for (rownum = 0; rownum < FT5x16_TX_NUM; rownum++) {
		memset(read_buffer, 0x00, (FT5x16_RX_NUM * 2));

		if (ft5x16_write_reg(0x01, rownum) < 0) {
			pr_err("%s ERROR:could not write rownum", __FUNCTION__);
			retval = -1;
			goto error_return;
		}
		msleep(1);
		read_len = FT5x16_RX_NUM * 2;
        
		if (ft5x16_write_reg(0x10, read_len) < 0) {
			pr_err("%s ERROR:could not write rownum", __FUNCTION__);
			retval = -1;
			goto error_return;
		}
              
		msgs[0].addr = this_client->addr,
		msgs[0].flags = 1,
		msgs[0].len	= FT5x16_RX_NUM * 2,
		msgs[0].buf	= read_buffer,

		retval = i2c_transfer(this_client->adapter, msgs, 1);
		if (retval < 0) {
			pr_err("%s ERROR:Could not read row %u raw data", __FUNCTION__, rownum);
			retval = -1;
			goto error_return;
		}
        
		for (i = 0; i < FT5x16_RX_NUM; i++)	{
			RawData[rownum][i] = (read_buffer[i << 1] << 8) + read_buffer[(i << 1) + 1];
		}
	}
error_return:
	if (ft5x16_enter_work(ft5x16_ts) < 0) {
		pr_err("%s ERROR:could not enter work mode ", __FUNCTION__);
		retval = -1;
	}
	return retval;
}

int fts_ctpm_auto_clb(void)
{
    unsigned char uc_temp;
    unsigned char i ;

    printk("[FTS] start auto CLB.\n");
    
    msleep(200);
    ft5x16_write_reg(0, 0x40);  
    delay_qt_ms(100);   //make sure already enter factory mode
    ft5x16_write_reg(2, 0x4);  //write command to start calibration
    delay_qt_ms(300);
    
    for (i = 0;i < 100;i++) {
        ft5x16_read_reg(0,&uc_temp);
        
        if (((uc_temp & 0x70) >> 4) == 0x0) //return to normal mode, calibration finish
            break;

        delay_qt_ms(200);
        printk("[FTS] waiting calibration %d\n",i);
        
    }
    printk("[FTS] calibration OK.\n");
    
    msleep(300);
    ft5x16_write_reg(0, 0x40);  //goto factory mode
    delay_qt_ms(100);   //make sure already enter factory mode
    ft5x16_write_reg(2, 0x5);  //store CLB result
    delay_qt_ms(300);
    ft5x16_write_reg(0, 0x0); //return to normal mode 
    msleep(300);
    printk("[FTS] store CLB result OK.\n");
    
    return 0;
}

int fts_ctpm_fw_upgrade_with_i_file(void)
{
   FTS_BYTE*     pbt_buf = FTS_NULL;
   int i_ret;
    
    //=========FW upgrade========================*/
   if (g_ts_id == FT5316)
       pbt_buf = CTPM_5X16_FW;       
   else if (g_ts_id == FT5336) 
       pbt_buf = CTPM_5X36_FW;             
   else
       pbt_buf = CTPM_5X16_FW;
   
   /*call the upgrade function*/
   if (g_ts_id == FT5316)
       i_ret =  fts_ctpm_fw_upgrade(pbt_buf, sizeof(CTPM_5X16_FW));       
   else if (g_ts_id == FT5336)
       i_ret =  fts_ctpm_fw_upgrade(pbt_buf, sizeof(CTPM_5X36_FW));              
   else
       i_ret =  fts_ctpm_fw_upgrade(pbt_buf, sizeof(CTPM_5X16_FW));              
   
   if (i_ret != 0) {
       printk("[FTS] upgrade failed i_ret = %d.\n", i_ret);
       //error handling ...
       //TBD
   }
   else {
       printk("[FTS] upgrade successfully.\n");
       fts_ctpm_auto_clb();  //start auto CLB
   }

   return i_ret;
}

unsigned char fts_ctpm_get_i_file_ver(void)
{
    unsigned int ui_sz;
    if (g_ts_id == FT5316)
        ui_sz = sizeof(CTPM_5X16_FW);       
    else if (g_ts_id == FT5336) 
        ui_sz = sizeof(CTPM_5X36_FW);
    else
        ui_sz = sizeof(CTPM_5X16_FW);
    
    if (ui_sz > 2) {
        if (g_ts_id == FT5316)
            return CTPM_5X16_FW[ui_sz - 2];
        else if (g_ts_id == FT5336)
            return CTPM_5X36_FW[ui_sz - 2];           
        else
            return CTPM_5X16_FW[ui_sz - 2];           
    }
    
    else
        //TBD, error handling?
        return 0x00; //default value
}

#define    FTS_SETTING_BUF_LEN        128

//update project setting
//only update these settings for COB project, or for some special case
int fts_ctpm_update_project_setting(void)
{
    unsigned char uc_i2c_addr;             //I2C slave address (8 bit address)
    unsigned char uc_io_voltage;           //IO Voltage 0---3.3v;	1----1.8v
    unsigned char uc_panel_factory_id;     //TP panel factory ID

    unsigned char buf[FTS_SETTING_BUF_LEN];
    FTS_BYTE reg_val[2] = {0};
    FTS_BYTE  auc_i2c_write_buf[10];
    FTS_BYTE  packet_buf[FTS_SETTING_BUF_LEN + 6];
    FTS_DWRD i = 0;
    int      i_ret;

    uc_i2c_addr = 0x70;
    uc_io_voltage = 0x0;
    uc_panel_factory_id = 0x5a;

    /*********Step 1:Reset  CTPM *****/
    /*write 0xaa to register 0xfc*/
    ft5x16_write_reg(0xfc,0xaa);
    delay_qt_ms(50);
     /*write 0x55 to register 0xfc*/
    ft5x16_write_reg(0xfc,0x55);
    printk("[FTS] Step 1: Reset CTPM test\n");
   
    delay_qt_ms(30);   

    /*********Step 2:Enter upgrade mode *****/
    auc_i2c_write_buf[0] = 0x55;
    auc_i2c_write_buf[1] = 0xaa;
    do {
        i ++;
        i_ret = ft5x16_i2c_txdata(auc_i2c_write_buf, 2);
        delay_qt_ms(5);
    } while(i_ret <= 0 && i < 5 );

    /*********Step 3:check READ-ID***********************/        
    cmd_write(0x90,0x00,0x00,0x00,4);
    byte_read(reg_val,2);

    if (reg_val[0] == 0x79 && reg_val[1] == 0x3)
        printk("[FT5316] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
    else
        return ERR_READID;

    cmd_write(0xcd,0x0,0x00,0x00,1);
    byte_read(reg_val,1);
    printk("bootloader version = 0x%x\n", reg_val[0]);

    /* --------- read current project setting  ---------- */
    //set read start address
    buf[0] = 0x3;
    buf[1] = 0x0;
    buf[2] = 0x78;
    buf[3] = 0x0;
    byte_write(buf, 4);
    byte_read(buf, FTS_SETTING_BUF_LEN);
    
    printk("[FTS] old setting: uc_i2c_addr = 0x%x, uc_io_voltage = %d, uc_panel_factory_id = 0x%x\n",
        buf[0],  buf[2], buf[4]);
    for (i = 0; i < FTS_SETTING_BUF_LEN; i++) {
        if (i % 16 == 0)     printk("\n");
        printk("0x%x, ", buf[i]);
        
    }
    printk("\n");

     /*--------- Step 4:erase project setting --------------*/
    cmd_write(0x62,0x00,0x00,0x00,1);
    delay_qt_ms(100);
   
    /*----------  Set new settings ---------------*/
    buf[0] = uc_i2c_addr;
    buf[1] = ~uc_i2c_addr;
    buf[2] = uc_io_voltage;
    buf[3] = ~uc_io_voltage;
    buf[4] = uc_panel_factory_id;
    buf[5] = ~uc_panel_factory_id;
    packet_buf[0] = 0xbf;
    packet_buf[1] = 0x00;
    packet_buf[2] = 0x78;
    packet_buf[3] = 0x0;
    packet_buf[4] = 0;
    packet_buf[5] = FTS_SETTING_BUF_LEN;
    for (i = 0; i < FTS_SETTING_BUF_LEN; i++) {
        packet_buf[6 + i] = buf[i];
        if (i % 16 == 0)     printk("\n");
        printk("0x%x, ", buf[i]);
    }
    printk("\n");
    byte_write(&packet_buf[0],FTS_SETTING_BUF_LEN + 6);
    delay_qt_ms(100);

    /********* reset the new FW***********************/
    cmd_write(0x07,0x00,0x00,0x00,1);

    msleep(200);

    return 0;
}

#ifdef CFG_SUPPORT_AUTO_UPG
int fts_ctpm_auto_upg(void)
{
    unsigned char uc_host_fm_ver;
    unsigned char uc_tp_fm_ver;
    int           i_ret;

    uc_tp_fm_ver = ft5x16_read_fw_ver();
    uc_host_fm_ver = fts_ctpm_get_i_file_ver();

    if (uc_tp_fm_ver == 0xa6 ||   //the firmware in touch panel maybe corrupted
         uc_tp_fm_ver < uc_host_fm_ver) { //the firmware in host flash is new, need upgrade
        printk("[FTS] TSP firmware is old. now TSP firmware upgrading...\n");
        printk("[FTS] Current TSP ver = 0x%x, New firmware ver = 0x%x\n", uc_tp_fm_ver, uc_host_fm_ver);
        i_ret = fts_ctpm_fw_upgrade_with_i_file();    
        if (i_ret == 0) 
            uc_host_fm_ver = fts_ctpm_get_i_file_ver();
        else {
            printk("[FTS] upgrade failed ret=%d.\n", i_ret);
            ft5x16_reset();
        }
    }
    else
        printk("[FTS] TSP firmware is newest.x(Current: v0x%x/Request: v0x%x)\n", uc_tp_fm_ver, uc_host_fm_ver);

    return 0;
}
#endif
#endif

static void ft5x16_ts_release(void)
{
	struct ft5x16_ts_data *data = i2c_get_clientdata(this_client);
    int i = tsp_keystatus - 1;

	if (g_release_not_yet) {
        if (tsp_ignore_cnt > 2) {
            input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
            input_report_key(data->input_dev, BTN_TOUCH, 0);
        }

        if (tsp_keystatus) {
            input_report_key(data->input_dev, tsp_keycodes[i], KEY_RELEASE);
            tsp_keystatus = KEY_RELEASE;
        }

        tsp_ignore_cnt = 0;
        input_sync(data->input_dev);
        g_release_not_yet = 0;
	}
}

//read touch point information
static int ft5x16_read_data(void)
{
	struct ft5x16_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
	u8 buf[CFG_POINT_READ_BUF] = {0};
	int ret = -1;
	int i;
	
	ret = ft5x16_i2c_rxdata(buf, CFG_POINT_READ_BUF);
    if (ret < 0) {
		printk("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}
	memset(event, 0, sizeof(struct ts_event));
	event->touch_point = buf[2] & 0x07; 
    
    if (event->touch_point > CFG_MAX_TOUCH_POINTS)
        event->touch_point = CFG_MAX_TOUCH_POINTS;

    for (i = 0; i < event->touch_point; i++)  {
        event->au16_x[i] = (s16)(buf[3 + 6*i] & 0x0F) << 8 | (s16)buf[4 + 6*i];
        event->au16_y[i] = (s16)(buf[5 + 6*i] & 0x0F) << 8 | (s16)buf[6 + 6*i];
        event->au8_touch_event[i] = buf[0x3 + 6*i] >> 6;
        event->au8_finger_id[i] = (buf[5 + 6*i]) >> 4;
    }

    event->pressure = 200;

    return 0;
}

static void ft5x16_touch_release_event(unsigned long data)
{
    ft5x16_ts_release();
}

#ifdef CFG_SUPPORT_TOUCH_KEY
#if 0
int ft5x16_touch_key_process(struct input_dev *dev, int x, int y, int touch_event)
{
	struct ft5x16_ts_data *data = i2c_get_clientdata(this_client);    
    int key_id;
    int ret;
    
    if (x == 240 && y == 850)
        key_id = 1;
    else
        key_id = 0;

    if (key_id) {
        if (touch_event == 0) { // detect
            mod_timer(&data->touchkey_timer, TOUCH_KEY_CHECK_TIMEOUT);
            
            tsp_keystatus++;
            if (tsp_keystatus > KEY_DOUBLE)
                tsp_keystatus = KEY_DOUBLE;
        }
    }
    
    return 0;
}
#else
int ft5x16_touch_key_process(struct input_dev *dev, int x, int y, int touch_event)
{
    int key_id;
	struct ft5x16_ts_data *data = i2c_get_clientdata(this_client);
        
    if (x == 240 && y == 850)
        key_id = 0;
    else
        key_id = 0xf;
    
    if (key_id == 0) {
        if (touch_event == 0) { // detect
            input_report_key(dev, tsp_keycodes[0], KEY_PRESS);
//            printk( "[FTS] %s key is pressed. Keycode : %d\n", tsp_keyname[0], tsp_keycodes[0]);
            tsp_keystatus = KEY_PRESS;

        }
    }
	g_release_not_yet = 1;
    mod_timer(&data->release_timer, TOUCH_RELEASE_TIMEOUT);
    
    return 0;
}
#endif
#endif

static void ft5x16_report_value(void)
{
	struct ft5x16_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
	int i;

    tsp_ignore_cnt++;
    
	for (i  = 0; i < event->touch_point; i++) {
	    if (event->au16_x[i] < SCREEN_MAX_X && event->au16_y[i] < SCREEN_MAX_Y) {
    	    // LCD view area
	        input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->au16_x[i]);
    		input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->au16_y[i]);
    		input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
    		input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->au8_finger_id[i]);

    		if (event->au8_touch_event[i] == 0 || event->au8_touch_event[i] == 2) {
//                printk("X: %d, Y: %d\n", event->au16_x[i], event->au16_y[i]);
                if (tsp_ignore_cnt > 2) { /* [downer] A131107 ESD ???°ì¹˜ ??ë°©ì?ë¥??„í•´ ìµœì´ˆ 2ì¢Œí‘œ ë¬´ì‹œ. */
                    input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
                    input_report_key(data->input_dev, BTN_TOUCH, 1);
                    g_release_not_yet = 1;
                    mod_timer(&data->release_timer, TOUCH_RELEASE_TIMEOUT);                    
                }
                
    		}
    		else {
    		    input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
                input_report_key(data->input_dev, BTN_TOUCH, 0);                
    		}
	    }
	    else { //maybe the touch key area
#ifdef CFG_SUPPORT_TOUCH_KEY
            if (event->au16_y[i] >= SCREEN_MAX_Y) 
                ft5x16_touch_key_process(data->input_dev, event->au16_x[i], event->au16_y[i], event->au8_touch_event[i]);                        
#endif
	    }
			
		input_mt_sync(data->input_dev);
	}
    
	input_sync(data->input_dev);

#if 1                           /* [downer] A131108 release ??timerë¡??™ìž‘. */
    if (event->touch_point == 0) 
        ft5x16_ts_release();
#endif
    
}	/*end ft5x16_report_value*/

static void ft5x16_ts_pen_irq_work(struct work_struct *work)
{
	int ret = -1;

	ret = ft5x16_read_data();	
	if (ret == 0) 	
		ft5x16_report_value();

	enable_irq(this_client->irq);
}

static irqreturn_t ft5x16_ts_interrupt(int irq, void *dev_id)
{
	struct ft5x16_ts_data *ft5x16_ts = dev_id;

	disable_irq_nosync(this_client->irq);
	
	if (!work_pending(&ft5x16_ts->pen_event_work)) 
		queue_work(ft5x16_ts->ts_workqueue, &ft5x16_ts->pen_event_work);

	return IRQ_HANDLED;
}

/* sysfs */
static u8 ft5x16_enter_factory(struct ft5x16_ts_data *ft5x16_ts)
{
	u8 regval;
    
	flush_workqueue(ft5x16_ts->ts_workqueue);
	disable_irq_nosync(this_client->irq);
	ft5x16_write_reg(0x00, 0x40);  //goto factory mode
	delay_qt_ms(100);   //make sure already enter factory mode
    	
	if (ft5x16_read_reg(0x00, &regval) < 0)
		pr_err("%s ERROR: could not read register\n", __FUNCTION__);
	else {
		if((regval & 0x70) != 0x40)	{
			pr_err("%s() - ERROR: The Touch Panel was not put in Factory Mode. The Device Mode register contains 0x%02X\n", __FUNCTION__, regval);
			return -1;
		}
	}
    
	return 0;
}
static u8 ft5x16_enter_work(struct ft5x16_ts_data *ft5x16_ts)
{
    u8 regval;
    
   	ft5x16_write_reg(0x00, 0x00); //return to normal mode 
   	msleep(100);
	
	if (ft5x16_read_reg(0x00, &regval) < 0)
		pr_err("%s ERROR: could not read register\n", __FUNCTION__);
	else {
		if ((regval & 0x70) != 0x00) {
			pr_err("%s() - ERROR: The Touch Panel was not put in Work Mode. The Device Mode register contains 0x%02X\n", __FUNCTION__, regval);
			enable_irq(this_client->irq);
			return -1;
		}
	}
	enable_irq(this_client->irq);
    
	return 0;
}

static int ft5x16_GetFirmwareSize(char * firmware_name)
{
	struct file* pfile = NULL;
	struct inode *inode;
	unsigned long magic; 
	off_t fsize = 0; 
	char filepath[128];

    memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "/sdcard/%s", firmware_name);
	pr_info("filepath=%s\n", filepath);
    
	if (pfile == NULL)
		pfile = filp_open(filepath, O_RDONLY, 0);
	
	if (IS_ERR(pfile)){
		pr_err("error occured while opening file %s.\n", filepath);
		return -1;
	}
    
	inode = pfile->f_dentry->d_inode; 
	magic = inode->i_sb->s_magic;
	fsize = inode->i_size; 
    
	filp_close(pfile, NULL);
    
	return fsize;
}

static int ft5x16_ReadFirmware(char * firmware_name, unsigned char * firmware_buf)
{
	struct file* pfile = NULL;
	struct inode *inode;
	unsigned long magic; 
	off_t fsize; 
	char filepath[128];
	loff_t pos;

	mm_segment_t old_fs;

    memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "/sdcard/%s", firmware_name);
	pr_info("filepath=%s\n", filepath);
    
	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);
	
	if (IS_ERR(pfile)) {
		pr_err("error occured while opening file %s.\n", filepath);
		return -1;
	}
    
	inode=pfile->f_dentry->d_inode; 
	magic=inode->i_sb->s_magic;
	fsize=inode->i_size; 

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;

	vfs_read(pfile, firmware_buf, fsize, &pos);

	filp_close(pfile, NULL);
	set_fs(old_fs);
    
	return 0;
}

int fts_ctpm_fw_upgrade_with_app_file(char *firmware_name)
{
  	FTS_BYTE *pbt_buf = FTS_NULL;
   	int i_ret = 0; 
    u8 fwver = 0;
   	int fwsize = ft5x16_GetFirmwareSize(firmware_name);
    
   	if (fwsize <= 0) { 	
   		pr_err("%s ERROR:Get firmware size failed\n", __FUNCTION__);
		return -1;
   	}
    
    //=========FW upgrade========================*/
  	pbt_buf = (unsigned char *) kmalloc(fwsize+1,GFP_ATOMIC);
	if (ft5x16_ReadFirmware(firmware_name, pbt_buf)) {
       	pr_err("%s() - ERROR: request_firmware failed\n", __FUNCTION__);
    	kfree(pbt_buf);
		return -1;
   	}
    
   	/*call the upgrade function*/
   	i_ret =  fts_ctpm_fw_upgrade(pbt_buf, fwsize);
   	if (i_ret != 0)	{
       	pr_err("%s() - ERROR:[FTS] upgrade failed i_ret = %d.\n",__FUNCTION__,  i_ret);
       //error handling ...
       //TBD
   	}
    else {
       	pr_info("[FTS] upgrade successfully.\n");
		if (ft5x16_read_reg(FT5x16_REG_FW_VER, &fwver) >= 0)
			pr_info("the new fw ver is 0x%02x\n", fwver);
      		fts_ctpm_auto_clb();  //start auto CLB
   	}
    
	kfree(pbt_buf);
    
   	return i_ret;
}

#if 1
static ssize_t ftstpfwver_show(struct device *dev, struct device_attribute *attr, char *buf)
{									\
	struct ft5x16_ts_data *data = NULL;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	u8	   fwver = 0;
    
	data = (struct ft5x16_ts_data *)i2c_get_clientdata(client);
	
	mutex_lock(&data->device_mode_mutex);
    
	if (ft5x16_read_reg(FT5x16_REG_FW_VER, &fwver) < 0)
		num_read_chars = snprintf(buf, PAGE_SIZE, "get tp fw version fail!\n");
	else
		num_read_chars = snprintf(buf, PAGE_SIZE, "%02X\n", fwver);

	mutex_unlock(&data->device_mode_mutex);
    
	return num_read_chars;
}

static ssize_t ftstpfwver_store(struct device *dev,
					struct device_attribute *attr,
						const char *buf, size_t count)
{
	/* place holder for future use */
	return -EPERM;
}

static ssize_t ftstprwreg_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	/* place holder for future use */
	return -EPERM;
}

static ssize_t ftstprwreg_store(struct device *dev,
					struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct ft5x16_ts_data *data = NULL;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int retval;
	long wmreg = 0;
    u8 regaddr = 0xff,regvalue = 0xff;
	u8 valbuf[5];

    memset(valbuf, 0, sizeof(valbuf));  
	data = (struct ft5x16_ts_data *) i2c_get_clientdata(client);
	mutex_lock(&data->device_mode_mutex);
	num_read_chars = count - 1;

	if (num_read_chars != 2) {
		if (num_read_chars != 4) {
			pr_info("please input 2 or 4 character\n");
			goto error_return;
		}
	}
	
	memcpy(valbuf, buf, num_read_chars);
	retval = strict_strtoul(valbuf, 16, &wmreg);
	//pr_info("valbuf=%s wmreg=%x\n", valbuf, wmreg);
	if (retval != 0) {
       	pr_err("%s() - ERROR: Could not convert the given input to a number. The given input was: \"%s\"\n", __FUNCTION__, buf);
       	goto error_return;
    }

	if (num_read_chars == 2) {
		//read register
		regaddr = wmreg;
		if (ft5x16_read_reg(regaddr, &regvalue) < 0)
			pr_err("Could not read the register(0x%02x)\n", regaddr);
		else
			pr_info("the register(0x%02x) is 0x%02x\n", regaddr, regvalue);
	}
	else {
		regaddr = wmreg >> 8;
		regvalue = wmreg;
		if (ft5x16_write_reg(regaddr, regvalue) < 0)
			pr_err("Could not write the register(0x%02x)\n", regaddr);
		else
			pr_err("Write 0x%02x into register(0x%02x) successful\n", regvalue, regaddr);
	}
error_return:
	mutex_unlock(&data->device_mode_mutex);

	return count;
}


static ssize_t ftsfwupdate_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	/* place holder for future use */
    return -EPERM;
}
//upgrade from *.i
static ssize_t ftsfwupdate_store(struct device *dev,
					struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct ft5x16_ts_data *data = NULL;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	u8 uc_host_fm_ver;    
	int i_ret = 0;
    
	data = (struct ft5x16_ts_data *) i2c_get_clientdata(client);

	mutex_lock(&data->device_mode_mutex);

	disable_irq(this_client->irq);

    i_ret = fts_ctpm_fw_upgrade_with_i_file();        
    if (i_ret == 0) {
        msleep(300);
        uc_host_fm_ver = fts_ctpm_get_i_file_ver();
        pr_info("%s [FTS] upgrade to new version 0x%x\n", __FUNCTION__, uc_host_fm_ver);
    }
    else
        pr_err("%s ERROR:[FTS] upgrade failed ret=%d.\n", __FUNCTION__, i_ret);

	enable_irq(this_client->irq);
	
	mutex_unlock(&data->device_mode_mutex);

	return count;
}

static ssize_t ftsfwupgradeapp_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	/* place holder for future use */
    return -EPERM;
}
//upgrade from app.bin
static ssize_t ftsfwupgradeapp_store(struct device *dev,
					struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct ft5x16_ts_data *data = NULL;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
    char fwname[128];
    
	data = (struct ft5x16_ts_data *) i2c_get_clientdata(client);
	
    memset(fwname, 0, sizeof(fwname));
	sprintf(fwname, "%s", buf);
	fwname[count-1] = '\0';

	mutex_lock(&data->device_mode_mutex);
	disable_irq(this_client->irq);
	
	fts_ctpm_fw_upgrade_with_app_file(fwname);
	
	enable_irq(this_client->irq);

	mutex_unlock(&data->device_mode_mutex);

	return count;
}

static ssize_t ftsrawdata_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct ft5x16_ts_data *data = NULL;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int i=0, j=0;
	u16	RawData[FT5x16_TX_NUM][FT5x16_RX_NUM];    
    
	data = (struct ft5x16_ts_data *) i2c_get_clientdata(client);
    
	mutex_lock(&data->device_mode_mutex);
    
	if (fts_Get_RawData(RawData) < 0)
		sprintf(buf, "%s", "could not get rawdata\n");
	else {
		for(i=0; i<FT5x16_TX_NUM; i++) {
			for(j=0; j<FT5x16_RX_NUM; j++) 
				num_read_chars += sprintf(&(buf[num_read_chars]), "%u ", RawData[i][j]);

			buf[num_read_chars-1] = '\n';
		}
	}
    
	mutex_unlock(&data->device_mode_mutex);	
    
	return num_read_chars;
}
//upgrade from app.bin
static ssize_t ftsrawdata_store(struct device *dev,
					struct device_attribute *attr,
						const char *buf, size_t count)
{

	return -EPERM;
}

/* sysfs */
//static DEVICE_ATTR(rawbase, S_IRUGO|S_IWUSR, ft5x16_rawbase_show, ft5x16_rawbase_store);
static DEVICE_ATTR(ftstpfwver, S_IRUGO|S_IWUSR, ftstpfwver_show, ftstpfwver_store);
//upgrade from *.i
static DEVICE_ATTR(ftsfwupdate, S_IRUGO|S_IWUSR, ftsfwupdate_show, ftsfwupdate_store);
static DEVICE_ATTR(ftstprwreg, S_IRUGO|S_IWUSR, ftstprwreg_show, ftstprwreg_store);
//upgrade from app.bin 
static DEVICE_ATTR(ftsfwupgradeapp, S_IRUGO|S_IWUSR, ftsfwupgradeapp_show, ftsfwupgradeapp_store);
static DEVICE_ATTR(ftsrawdata, S_IRUGO|S_IWUSR, ftsrawdata_show, ftsrawdata_store);


static struct attribute *ft5x16_attributes[] = {
	&dev_attr_ftstpfwver.attr,
	&dev_attr_ftsfwupdate.attr,
	&dev_attr_ftstprwreg.attr,
	&dev_attr_ftsfwupgradeapp.attr,
	&dev_attr_ftsrawdata.attr,
	NULL
};

static struct attribute_group ft5x16_attribute_group = {
	.attrs = ft5x16_attributes
};
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
#define ft5x16_suspend NULL
#define ft5x16_resume  NULL

static void ft5x16_early_suspend(struct early_suspend *h)
{
	struct ft5x16_ts_data *data = container_of(h, struct ft5x16_ts_data, early_suspend);

	mutex_lock(&data->device_mode_mutex);    
    
	disable_irq(this_client->irq);
   	ft5x16_write_reg(FT5X16_REG_PMODE, PMODE_HIBERNATE);    
    ft5x16_power_off();
    
    mutex_unlock(&data->device_mode_mutex);    
}

static void ft5x16_late_resume(struct early_suspend *h)
{
	struct ft5x16_ts_data *data = container_of(h, struct ft5x16_ts_data, early_suspend);    
    
	printk("%s\n", __func__);

	// wake the mode
	mutex_lock(&data->device_mode_mutex);    	

    ft5x16_power_on();
    ft5x16_reset();
    
   	ft5x16_write_reg(FT5X16_REG_PMODE, PMODE_ACTIVE);        
    
	enable_irq(this_client->irq);
    
    mutex_unlock(&data->device_mode_mutex);    
}
#else
#define ft5x16_suspend NULL
#define ft5x16_resume  NULL
#define ft5x16_early_suspend NULL
#define ft5x16_late_resume NULL
#endif  //CONFIG_HAS_EARLYSUSPEND


#if(CUR_AK_REV >= AK70_HW_WS)
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>

void enable_tsp_pwen(int enable)
{
#ifdef CONFIG_REGULATOR
	static struct regulator *ldo_tsp3v3;
	ldo_tsp3v3 = regulator_get(NULL, "vdd_ldo11_sd30_3v3");
	if (IS_ERR(ldo_tsp3v3)) {
		printk("failed to obtain vdd_ldo11_sd30_3v3\n");
		ldo_tsp3v3 = NULL;
	}

	if(enable) {
		regulator_enable(ldo_tsp3v3);
	} else {
		regulator_disable(ldo_tsp3v3);
	}
#endif	
}
#endif

static int __devinit ft5x16_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ft5x16_ts_data *data;
	struct input_dev *input_dev;
	int err = 0;
	unsigned char uc_reg_value;
    
#if CFG_SUPPORT_TOUCH_KEY
    int i;
#endif
	
	printk("[FTS] %s driver version is %s.\n", __func__, CFG_FTS_CTP_DRIVER_VERSION);
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		dev_err(&client->dev, "i2c_check_functionality failed\n");        
		goto exit_check_functionality_failed;
	}

	data = (struct ft5x16_ts_data *)kzalloc(sizeof(struct ft5x16_ts_data), GFP_KERNEL);
	if (!data)	{
		err = -ENOMEM;
		dev_err(&client->dev, "kzalloc failed\n");                
		goto exit_alloc_data_failed;
	}

	this_client = client;
	i2c_set_clientdata(client, data);

	mutex_init(&data->device_mode_mutex);
	INIT_WORK(&data->pen_event_work, ft5x16_ts_pen_irq_work);

	data->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!data->ts_workqueue) {
		err = -ESRCH;
		dev_err(&client->dev, "ft5x16_probe: create_singlethread_workqueue failed\n");                        
		goto exit_create_singlethread;
	}

    tcc_gpio_config(GPIO_TOUCH_RST, GPIO_OUTPUT | GPIO_FN0); // TOUCH_RST

	#if(CUR_AK_REV == AK70_HW_EVM)
    tcc_gpio_config(GPIO_TSP_PWEN, GPIO_OUTPUT | GPIO_FN0); // TSP_PWEN
	#endif

	#if(CUR_AK_REV >= AK70_HW_WS)
	enable_tsp_pwen(1);
	#endif
	
	tcc_gpio_config(GPIO_TOUCH_INT, GPIO_INPUT | GPIO_FN0); // TSP_INT

    tcc_gpio_config_ext_intr(client->irq, EXTINT_GPIOE_14);
    gpio_request(GPIO_TOUCH_INT, "TS_INT");
  
    ft5x16_power_on();
    ft5x16_reset();
    
	err = request_irq(client->irq, ft5x16_ts_interrupt, IRQF_TRIGGER_FALLING, "ft5x16_ts", data);
	if (err < 0) {
		dev_err(&client->dev, "request irq failed\n");
		goto exit_irq_request_failed;
	}

	disable_irq(client->irq);

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	
	data->input_dev = input_dev;
    input_set_drvdata(input_dev, data);
	input_dev->name	= FT5X16_NAME;
    input_dev->id.bustype = BUS_I2C;
    input_dev->id.vendor = 0x0001;
    input_dev->id.product = 0x0001;
    input_dev->id.version = 0x0001;

	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);

    set_bit(EV_ABS, input_dev->evbit);
    set_bit(EV_KEY, input_dev->evbit);
    
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 5, 0, 0);

#ifdef CFG_SUPPORT_TOUCH_KEY
    //setup key code area
    set_bit(EV_SYN, input_dev->evbit);
    set_bit(BTN_TOUCH, input_dev->keybit);
    
    input_dev->keycode = tsp_keycodes;
    for(i = 0; i < CFG_NUMOFKEYS; i++) {
        input_set_capability(input_dev, EV_KEY, ((int*)input_dev->keycode)[i]);
        tsp_keystatus = KEY_RELEASE;            
    }

#endif

	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev, "failed to register input device: %s\n", dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = ft5x16_early_suspend;
	data->early_suspend.resume	= ft5x16_late_resume;
	register_early_suspend(&data->early_suspend);
#endif

    msleep(150);  //make sure CTP already finish startup process

    //get some register information
    uc_reg_value = ft5x16_read_fw_ver();
    printk("[FTS] Firmware version = 0x%x\n", uc_reg_value);

#if 0
    if (uc_reg_value <= 0xc) {
        printk("[FTS] TS TYPE FT5316\n");
        g_ts_id = FT5316;
    }
    else {
        printk("[FTS] TS TYPE FT5336\n");        
        g_ts_id = FT5336;
    }
#else	
	g_ts_id = FT5336;
#endif
	
    ft5x16_read_reg(FT5X16_REG_PERIODACTIVE, &uc_reg_value);
    printk("[FTS] report rate is %dHz.\n", uc_reg_value * 10);
    ft5x16_read_reg(FT5X16_REG_THGROUP, &uc_reg_value);
    printk("[FTS] touch threshold is %d.\n", uc_reg_value * 4);

#if (CFG_SUPPORT_AUTO_UPG == 1)
    fts_ctpm_auto_upg();
#endif    

#if (CFG_SUPPORT_UPDATE_PROJECT_SETTING == 1)
    fts_ctpm_update_project_setting();
#endif

    enable_irq(client->irq);

    /*
     * [downer] A131108
     * touch release timer
     */
    setup_timer(&data->release_timer, ft5x16_touch_release_event, (unsigned long)data);
    
#if 1
    //create sysfs
    err = sysfs_create_group(&client->dev.kobj, &ft5x16_attribute_group);
    if (0 != err) {
        dev_err(&client->dev, "%s() - ERROR: sysfs_create_group() failed: %d\n", __FUNCTION__, err);
        sysfs_remove_group(&client->dev.kobj, &ft5x16_attribute_group);
    }
    else {
        printk("ft5x16:%s() - sysfs_create_group() succeeded.\n", __FUNCTION__);
    }
#endif

    return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
	free_irq(client->irq, data);
exit_irq_request_failed:
	cancel_work_sync(&data->pen_event_work);
	destroy_workqueue(data->ts_workqueue);
exit_create_singlethread:
	i2c_set_clientdata(client, NULL);
	kfree(data);
exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}

static int __devexit ft5x16_ts_remove(struct i2c_client *client)
{
	struct ft5x16_ts_data *ft5x16_ts = i2c_get_clientdata(client);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ft5x16_ts->early_suspend);
#endif

	mutex_destroy(&ft5x16_ts->device_mode_mutex);
	free_irq(client->irq, ft5x16_ts);

	cancel_work_sync(&ft5x16_ts->pen_event_work);
	destroy_workqueue(ft5x16_ts->ts_workqueue);
	i2c_set_clientdata(client, NULL); 
	del_timer(&ft5x16_ts->release_timer);

	input_unregister_device(ft5x16_ts->input_dev);
	kfree(ft5x16_ts);
    
	return 0;
}

static const struct i2c_device_id ft5x16_ts_id[] = {
	{ FT5X16_NAME, 0 },
    {},
};

MODULE_DEVICE_TABLE(i2c, ft5x16_ts_id);

static const struct dev_pm_ops ft5x16_pm_ops = {
    .suspend    = ft5x16_suspend,
    .resume     = ft5x16_resume,
};

static struct i2c_driver ft5x16_ts_driver = {
	.id_table	= ft5x16_ts_id,    
	.probe		= ft5x16_ts_probe,
	.remove		= __devexit_p(ft5x16_ts_remove),
	.driver	    = {
		.owner	= THIS_MODULE,	
		.name	= FT5X16_NAME,
        .pm     = &ft5x16_pm_ops,
	},
};

static int __init ft5x16_ts_init(void)
{
	return i2c_add_driver(&ft5x16_ts_driver);
}

static void __exit ft5x16_ts_exit(void)
{
	i2c_del_driver(&ft5x16_ts_driver);
}

module_init(ft5x16_ts_init);
module_exit(ft5x16_ts_exit);

MODULE_AUTHOR("<wenfs@Focaltech-systems.com>");
MODULE_DESCRIPTION("FocalTech ft5x16 TouchScreen driver");
MODULE_LICENSE("GPL");

