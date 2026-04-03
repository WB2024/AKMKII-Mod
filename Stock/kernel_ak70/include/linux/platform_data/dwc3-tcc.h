/*
 * Copyright (C) 2013 Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _DWC3_TCC_H_
#define _DWC3_TCC_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/platform_data/dwc3-tcc.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <mach/tcc_board_power.h>
#include <linux/delay.h>

#include <mach/bsp.h>
#include <mach/gpio.h>

struct dwc3_tcc_data {
	int phy_type;
	int (*phy_init)(struct platform_device *pdev, int type);
	int (*phy_exit)(struct platform_device *pdev, int type);
};

extern int tcc_usb30_vbus_init(void);
extern int tcc_usb30_vbus_exit(void);
extern int tcc_dwc3_vbus_ctrl(int on);

extern void tcc_usb30_phy_off(void);
extern void tcc_usb30_phy_on(void);

extern int tcc_usb30_clkset(int on);

#define TXVRT_SHIFT 6
#define TXVRT_MASK (0xF << TXVRT_SHIFT)
#define TXRISE_SHIFT 10
#define TXRISE_MASK (0x3 << TXRISE_SHIFT)

static void tcc_stop_dwc3(void)
{
	tcc_usb30_clkset(0);
}

static void tcc_start_dwc3(void)
{
	tcc_usb30_clkset(1);
}

#include <asm/mach-types.h>

#define TX_DEEMPH_SHIFT 		21
#define TX_DEEMPH_MASK  		0x7E00000

#define TX_DEEMPH_6DB_SHIFT 	15
#define TX_DEEMPH_6DB_MASK  	0x1F8000

#define TX_SWING_SHIFT		8
#define TX_SWING_MASK		0x7F00

#define TX_MARGIN_SHIFT		3
#define TX_MARGIN_MASK		0x7

#define TX_DEEMPH_SET_SHIFT	1
#define TX_DEEMPH_SET_MASK	0x3

#define RX_EQ_SHIFT	8
#define RX_EQ_MASK	0x700

#define RX_BOOST_SHIFT	2
#define RX_BOOST_MASK	0x1C

#define TX_BOOST_SHIFT	13
#define TX_BOOST_MASK	0xE000


static int dm = 0x15; // default value
//static int dm_device = 0x23;	// device mode
//static int dm_host = 0x10;	// host mode
static int dm_6db= 0x20;
static int sw = 0x5b;
static int sel_dm = 0x1;
static int rx_eq = 0xFF;
static int rx_boost = 0xFF;
static int tx_boost = 0xFF;

module_param(dm, int, S_IRUGO);
MODULE_PARM_DESC (dm, "Tx De-Emphasis at 3.5 dB"); // max: 0x3F

module_param(dm_6db, int, S_IRUGO);
MODULE_PARM_DESC (dm_6db, "Tx De-Emphasis at 6 dB"); // max: 0x3F

module_param(sw, int, S_IRUGO);
MODULE_PARM_DESC (sw, "Tx Amplitude (Full Swing Mode)");

module_param(sel_dm, int, S_IRUGO);
MODULE_PARM_DESC (sel_dm, "Tx De-Emphasis set");

module_param(rx_eq, int, S_IRUGO);
MODULE_PARM_DESC (rx_eq, "Rx Equalization");

module_param(rx_boost, int, S_IRUGO);
MODULE_PARM_DESC (rx_boost, "Rx Boost");

module_param(tx_boost, int, S_IRUGO);
MODULE_PARM_DESC (tx_boost, "Tx Boost");

void Write32(unsigned int addr, unsigned int val)
{
    writel(val,(void __iomem*)tcc_p2v(addr));
}
void Read32(unsigned int addr, unsigned int *val)
{
	*val = readl((void __iomem*)tcc_p2v(addr));
}

unsigned int dwc3_tcc_read_u30phy_reg(unsigned int address)
{
//    PUSBPHYCFG USBPHYCFG = (PUSBPHYCFG)tcc_p2v(HwUSBPHYCFG_BASE);
	PUSBPHYCFG USBPHYCFG = (PUSBPHYCFG)tcc_p2v(0x74600800);
    unsigned int tmp;
    unsigned int read_data;

    // address capture phase
    USBPHYCFG->UPCR0 = address;// write addr
    USBPHYCFG->UPCR2 |= 0x1; // capture addr

    while (1) {
        tmp = USBPHYCFG->UPCR2;
        tmp = (tmp>>16)&0x1;

        if (tmp==1) break; // check ack == 1
    }

    USBPHYCFG->UPCR2 &= ~0x1; // clear capture addr

    while (1) {
        tmp = USBPHYCFG->UPCR2;
        tmp = (tmp>>16)&0x1;
        if (tmp==0) break; // check ack == 0
    }

    // read phase
    USBPHYCFG->UPCR2 |= 0x1<<8; // set read
    while (1) {
        tmp = USBPHYCFG->UPCR2;
        tmp = (tmp>>16)&0x1;
        if (tmp==1) break; // check ack == 1
    }

    read_data = USBPHYCFG->UPCR1;    // read data

    USBPHYCFG->UPCR2 &= ~(0x1<<8); // clear read
    while (1) {
        tmp = USBPHYCFG->UPCR2;
        tmp = (tmp>>16)&0x1;
        if (tmp==0) break; // check ack == 0
    }

    return(read_data);
}

void dwc3_tcc_read_u30phy_reg_all (void) {
	unsigned int read_data;
	unsigned int i;

	for (i=0;i<0x37;i++) {
		read_data = dwc3_tcc_read_u30phy_reg(i);
		printk("addr:0x%08X value:0x%08X\n",i,read_data);
	}
	
	for (i=0x1000;i<0x1030;i++) {
		read_data = dwc3_tcc_read_u30phy_reg(i);
		printk("addr:0x%08X value:0x%08X\n",i,read_data);
	}
}

unsigned int dwc3_tcc_write_u30phy_reg(unsigned int address, unsigned int write_data)
{
	//PUSBPHYCFG USBPHYCFG = (PUSBPHYCFG)tcc_p2v(HwUSBPHYCFG_BASE);
	PUSBPHYCFG USBPHYCFG = (PUSBPHYCFG)tcc_p2v(0x74600800);
	unsigned int tmp;
	unsigned int read_data;

	// address capture phase
	USBPHYCFG->UPCR0 = address; // write addr
	USBPHYCFG->UPCR2 |= 0x1; // capture addr
	while (1) {
	  tmp = USBPHYCFG->UPCR2;
	  tmp = (tmp>>16)&0x1;
	  if (tmp==1) break; // check ack == 1
	}
	USBPHYCFG->UPCR2 &= ~(0x1); // clear capture addr
	while (1) {
	  tmp = USBPHYCFG->UPCR2;
	  tmp = (tmp>>16)&0x1;
	  if (tmp==0) break; // check ack == 0
	}

	// write phase
	USBPHYCFG->UPCR0 = write_data; // write data
	USBPHYCFG->UPCR2 |= 0x1<<4; // set capture data
	while (1) {
	  tmp = USBPHYCFG->UPCR2;
	  tmp = (tmp>>16)&0x1;
	  if (tmp==1) break; // check ack == 1
	}
	USBPHYCFG->UPCR2 &= ~(0x1<<4); // clear capture data
	while (1) {
	  tmp = USBPHYCFG->UPCR2;
	  tmp = (tmp>>16)&0x1;
	  if (tmp==0) break; // check ack == 0
	}
	USBPHYCFG->UPCR2 |= 0x1<<12; // set write
	while (1) {
	  tmp = USBPHYCFG->UPCR2;
	  tmp = (tmp>>16)&0x1;
	  if (tmp==1) break; // check ack == 1
	}
	USBPHYCFG->UPCR2 &= ~(0x1<<12); // clear write
	while (1) {
	  tmp = USBPHYCFG->UPCR2;
	  tmp = (tmp>>16)&0x1;
	  if (tmp==0) break; // check ack == 0
	}

	// read phase
	USBPHYCFG->UPCR2 |= 0x1<<8; // set read
	while (1) {
	  tmp = USBPHYCFG->UPCR2;
	  tmp = (tmp>>16)&0x1;
	  if (tmp==1) break; // check ack == 1
	}
	read_data = USBPHYCFG->UPCR1; // read data
	USBPHYCFG->UPCR2 &= ~(0x1<<8); // clear read
	while (1) {
	  tmp = USBPHYCFG->UPCR2;
	  tmp = (tmp>>16)&0x1;
	  if (tmp==0) break; // check ack == 0
	}
	if (read_data==write_data) return(1); // success
	else return(0); // fail
}

static int tcc_usb30_phy_init(struct platform_device *pdev, int type)
{
	PUSBPHYCFG USBPHYCFG = (PUSBPHYCFG)tcc_p2v(HwUSBPHYCFG_BASE);
	PUSB3_GLB	gUSB3_GLB = (PUSB3_GLB)tcc_p2v(HwUSBGLOBAL_BASE);

	unsigned int uTmp, time_delay = 0;

	while (1){
#if 1//!defined(CONFIG_TCC_USB_DRD)
		tcc_usb30_vbus_init();
#endif

		tcc_dwc3_vbus_ctrl(1);
		tcc_start_dwc3();
		tcc_usb30_phy_on();

		msleep(1);
		//msleep(time_delay);

		// Initialize All PHY & LINK CFG Registers
		USBPHYCFG->UPCR0 = 0xB5484068;
		USBPHYCFG->UPCR1 = 0x0000041C;
		USBPHYCFG->UPCR2 = 0x919E14C8;
		USBPHYCFG->UPCR3 = 0x4AB05D00;
#if 1//!defined(CONFIG_TCC_USB_DRD)
		USBPHYCFG->UPCR4 = 0x00000000;
#endif
		BITCSET(USBPHYCFG->UPCR3, TX_DEEMPH_MASK, dm << TX_DEEMPH_SHIFT);
		//BITCSET(USBPHYCFG->UPCR3, TX_DEEMPH_MASK, dm_device<< TX_DEEMPH_SHIFT);
		//BITCSET(USBPHYCFG->UPCR3, TX_DEEMPH_MASK, dm_host<< TX_DEEMPH_SHIFT);

		//BITCSET(USBPHYCFG->UPCR3, TX_DEEMPH_6DB_MASK, dm_6db<< TX_DEEMPH_6DB_SHIFT);
		BITCSET(USBPHYCFG->UPCR3, TX_SWING_MASK, sw << TX_SWING_SHIFT);
		printk("dwc3 tcc: PHY cfg - TX_DEEMPH 3.5dB: 0x%02x, TX_DEEMPH 6dB: 0x%02x, TX_SWING: 0x%02x\n", dm, dm_6db, sw);

		USBPHYCFG->UPCR5 = 0x00108001;
		USBPHYCFG->LCFG = 0x80420013;
#if 0//defined(CONFIG_TCC_USB_DRD)
		BITSET(USBPHYCFG->UPCR4, Hw20|Hw21); //ID pin interrupt enable
#endif
		msleep(1);

		USBPHYCFG->UPCR1 = 0x00000412;
		USBPHYCFG->LCFG	 = 0x86420013;

		// PHY Clock Setting (all are reset values)
		// USB3PHY_CFG0[26:21] : FSEL = 6'b101010 
		// USB3PHY_CFG0[28:27] : REFCLKSEL = 2'b10
		// USB3PHY_CFG0[29:29] : REF_SSP_EN = 1'b1
		// USB3PHY_CFG0[30:30] : REF_USE_PAD = 1'b0
		// USB3PHY_CFG0[31:31] : SSC_EN = 1'b1

		// Setting LINK Suspend clk dividor (all are reset values)
		//HwHSB_CFG->uUSB30LINK_CFG.bReg.SUSCLK_DIV_EN = 0x1;
		//HwHSB_CFG->uUSB30LINK_CFG.bReg.SUSCLK_DIV = 0x3; // busclk / (3+1) = 250 / 4 = 15.625MHz

		//HC DC controll
		BITCSET(USBPHYCFG->UPCR2, TXVRT_MASK, 10 << TXVRT_SHIFT);
		BITCSET(gUSB3_GLB->GCTL.nREG,(Hw12|Hw13), Hw13);
#if 0
		// USB20 Only Mode
		USBPHYCFG->LCFG	 |= Hw30;	//USB2.0 HS only mode
		USBPHYCFG->UPCR1 |= Hw9;	//test_powerdown_ssp
#else
		// USB 3.0
		BITCLR(USBPHYCFG->LCFG, Hw30);
		BITCLR(USBPHYCFG->UPCR1,Hw9);

		//USBPHYCFG->UPCR1 |= Hw8; 	//test_powerdown_hsp
		//=====================================================================
		// Tx Deemphasis setting
		// Global USB3 PIPE Control Register (GUSB3PIPECTL0): 0x1100C2C0
		//=====================================================================
		Read32(0x7100C2C0,&uTmp);
		BITCSET(uTmp, TX_DEEMPH_SET_MASK, sel_dm << TX_DEEMPH_SET_SHIFT);

		if(sel_dm == 0)
			printk("dwc3 tcc: PHY cfg - 6dB de-emphasis\n");
		else if (sel_dm == 1)
			printk("dwc3 tcc: PHY cfg - 3.5dB de-emphasis (default)\n");
		else if (sel_dm == 2)
			printk("dwc3 tcc: PHY cfg - de-emphasis\n");
		Write32(0x7100C2C0,uTmp);

		//=====================================================================
		// Rx EQ setting
		//=====================================================================
		if ( rx_eq != 0xFF)
		{
			printk("dwc3 tcc: phy cfg - EQ Setting\n");
			//========================================
			//	Read RX_OVER_IN_HI
			//========================================
			uTmp = dwc3_tcc_read_u30phy_reg(0x1006);
			printk("Reset value - RX-OVRD: 0x%08X\n", uTmp);
			//printk("Reset value - RX_EQ: 0x%X\n", ((uTmp & 0x00000700 ) >> 8));
			//
			//printk("\x1b[1;33m[%s:%d]rx_eq: %d\x1b[0m\n", __func__, __LINE__, rx_eq);

			BITCSET(uTmp, RX_EQ_MASK, rx_eq << RX_EQ_SHIFT);	
			uTmp |= Hw11;

			//printk("Set value - RX-OVRD: 0x%08X\n", uTmp);
			//printk("Set value - RX_EQ: 0x%X\n", ((uTmp & 0x00000700 ) >> 8));


			dwc3_tcc_write_u30phy_reg(0x1006, uTmp);

			uTmp = dwc3_tcc_read_u30phy_reg(0x1006);
			//printk("	  Reload - RX-OVRD: 0x%08X\n", uTmp);
			//printk("	  Reload - RX_EQ: 0x%X\n", ((uTmp & 0x00000700 ) >> 8));
			printk("dwc3 tcc: PHY cfg - RX EQ: 0x%x\n", ((uTmp & 0x00000700 ) >> 8));
		}

		//=====================================================================
		// Rx Boost setting
		//=====================================================================
		if ( rx_boost != 0xFF)
		{
			//printk("dwc3 tcc: phy cfg - Rx Boost Setting\n");
			//========================================
			//	Read RX_OVER_IN_HI
			//========================================
			uTmp = dwc3_tcc_read_u30phy_reg(0x1024);
			//printk("Reset value - RX-ENPWR1: 0x%08X\n", uTmp);
			//printk("Reset value - RX_BOOST: 0x%X\n", ((uTmp & RX_BOOST_MASK) >> RX_BOOST_SHIFT));
			//
			//printk("\x1b[1;33m[%s:%d]rx_boost: %d\x1b[0m\n", __func__, __LINE__, rx_boost);

			BITCSET(uTmp, RX_BOOST_MASK, rx_boost << RX_BOOST_SHIFT);	 
			uTmp |= Hw5;

			//printk("Reset value - RX-ENPWR1: 0x%08X\n", uTmp);
			//printk("Reset value - RX_BOOST: 0x%X\n", ((uTmp & RX_BOOST_MASK) >> RX_BOOST_SHIFT));

			dwc3_tcc_write_u30phy_reg(0x1024, uTmp);

			uTmp = dwc3_tcc_read_u30phy_reg(0x1024);
			//printk("	  Reload value - RX-ENPWR1: 0x%08X\n", uTmp);
			//printk("	  Reload value - RX_BOOST: 0x%X\n", ((uTmp & RX_BOOST_MASK) >> RX_BOOST_SHIFT));
			printk("dwc3 tcc: PHY cfg - RX BOOST: 0x%x\n", ((uTmp & RX_BOOST_MASK) >> RX_BOOST_SHIFT));
		}

		//=====================================================================
		// Tx Boost setting
		//=====================================================================
		if ( tx_boost != 0xFF)
		{
			printk("dwc3 tcc: phy cfg - Tx Boost Setting\n");
			//========================================
			//	Read RX_OVER_IN_HI
			//========================================
			uTmp = USBPHYCFG->UPCR2;
			BITCSET(uTmp, 0x7, tx_boost);

			USBPHYCFG->UPCR2 = uTmp;
		}
#endif /* 0 */

		// Wait USB30 PHY initialization finish. PHY FREECLK should be stable.
		msleep(time_delay++);
		// GDBGLTSSM : Check LTDB Sub State is non-zero
		// When PHY FREECLK is valid, LINK mac_resetn is released and LTDB Sub State change to non-zero
		uTmp = readl(tcc_p2v(HwUSBDEVICE_BASE+0x164));
		uTmp = (uTmp>>18)&0xF;

		// Break when LTDB Sub state is non-zero and CNR is zero
		if (uTmp != 0 || time_delay > 500) {
			break;
		}
	}

	if(uTmp)
		printk("\x1b[1;33mtime delay = %d  \x1b[0m\n",time_delay);
	else
		printk("\x1b[1;31mPHY stable is not guaranteed\x1b[0m\n");

	return 0;
}

int usb30_phy_exit(void)
{
	tcc_usb30_phy_off();
	tcc_usb30_clkset(0);
	tcc_dwc3_vbus_ctrl(0);
	tcc_usb30_vbus_exit();

	return 0;
}
#endif /* _DWC3_TCC_H_ */
