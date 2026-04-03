/*
 * wm8804.c  --  WM8804 S/PDIF transceiver driver
 *
 * Copyright 2010 Wolfson Microelectronics plc
 *
 * Author: Dimitris Papastamos <dp@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "wm8804.h"
#include <mach/board_astell_kern.h>



#define WM8804_NUM_SUPPLIES 2
static const char *wm8804_supply_names[WM8804_NUM_SUPPLIES] = {
	"PVDD",
	"DVDD"
};
#define TEST_WM8804_INFO  0

static int cache_sync = 1;
static struct workqueue_struct *wm8804_workq = NULL;

/*
 * wm8804 register cache
 * We can't read the WM8804 register space when we are
 * using 2 wire for device control, so we cache them instead.
 * There is no point in caching the reset register
 */

/*
0(0) 0x5   00000101
1(1) 0x88 10001000
2(2) 0x4 00000100
3(3) 0x89 10001001
4(4) 0xb0 10110000
5(5) 0x21 00100001
6(6) 0x7 00000111
7(7) 0x16 00010110
8(8) 0x10 00010000
9(9) 0xff 11111111
10(a) 0x0 00000000
11(b) 0x0 00000000
12(c) 0x0 00000000
13(d) 0x0 00000000
14(e) 0x0 00000000
15(f) 0x0 00000000
16(10) 0x0 00000000
17(11) 0x0 00000000
18(12) 0x0 00000000
19(13) 0x0 00000000
20(14) 0x0 00000000
21(15) 0x71 01110001  //chk
22(16) 0xb 00001011
23(17) 0x70 01110000
24(18) 0x57 01010111
25(19) 0x0 00000000
26(1a) 0x42 01000010


27(1b) 0x2 00000010
28(1c) 0x42 01000010
29(1d) 0x80 10000000
30(1e) 0x6 00000110

===========================================================
SPDIF TX
0(0) 0x5 00000101
1(1) 0x88 10001000
2(2) 0x4 00000100
3(3) 0xba 10111010
4(4) 0x49 01001001
5(5) 0xc 00001100
6(6) 0x8 00001000
7(7) 0xd 00001101
8(8) 0x10 00010000
9(9) 0xff 11111111
10(a) 0x0 00000000
11(b) 0x0 00000000
12(c) 0x0 00000000
13(d) 0x0 00000000
14(e) 0x0 00000000
15(f) 0x0 00000000
16(10) 0x0 00000000
17(11) 0x0 00000000
18(12) 0x0 00000000
19(13) 0x0 00000000
20(14) 0x0 00000000
21(15) 0x71 01110001
22(16) 0xb 00001011
23(17) 0x70 01110000
24(18) 0x57 01010111
25(19) 0x0 00000000
26(1a) 0x42 01000010
27(1b) 0xa 00001010
28(1c) 0x4a 01001010
29(1d) 0x80 10000000
30(1e) 0x2 00000010


*/

#if 0
/*===========================================================
SPIDF in
0(0) 0x5 00000101
1(1) 0x88 10001000
2(2) 0x4 00000100
3(3) 0x89 10001001
4(4) 0xb0 10110000
5(5) 0x21 00100001
6(6) 0x7 00000111
7(7) 0x16 00010110
8(8) 0x10 00010000
9(9) 0xff 11111111
10(a) 0x0 00000000
11(b) 0x0 00000000
12(c) 0x0 00000000
13(d) 0x0 00000000
14(e) 0x0 00000000
15(f) 0x0 00000000
16(10) 0x0 00000000
17(11) 0x0 00000000
18(12) 0x0 00000000
19(13) 0x0 00000000
20(14) 0x0 00000000
21(15) 0x71 01110001
22(16) 0xb 00001011
23(17) 0x70 01110000
24(18) 0x57 01010111
25(19) 0x0 00000000
26(1a) 0x42 01000010
27(1b) 0xa 00001010
28(1c) 0x4a 01001010
29(1d) 0x80 10000000
30(1e) 0x4 00000100

*/

static const u8 wm8804_reg_spdif_in[WM8804_REGISTER_COUNT+1] = {

0x5 ,
0x88 , 
0x4  ,
0x89  ,
0xb0  ,
0x21  ,
0x7  ,
0x16 ,
0x10,
0xff  ,
0x0  ,
0x0  ,
0x0  ,
0x0  ,
0x0  ,
0x0  ,
 0x0  ,
 0x0  ,
 0x0  ,
 0x0  ,
 0x0  ,
 0x71  ,
 0xb  ,
 0x70  ,
 0x57  ,
 0x0  ,
 0x42  ,
 0xa  ,
 0x4a  ,
 0x80  ,
 0x4 
};

#endif

#if 1 // DAC Play
static const u8 wm8804_reg[WM8804_REGISTER_COUNT+1] = {
	0x05,     /* R0  - RST/DEVID1 */
	0x88,     /* R1  - DEVID2 */
	0x04,     /* R2  - DEVREV */

#if 0 //MCLK : 12.288MHz, LRCLK : 48KHz
	0xBA,     /* R3  - PLL1 */
	0x49,     /* R4  - PLL2 */
	0x0C,     /* R5  - PLL3 */
	0x08,     /* R6  - PLL4 */
	0x16,     /* R7  - PLL5 */

#elif 1//MCLK : 11.2896MHz, LRCLK : 44.1KHz
	0x89,     /* R3  - PLL1 */
	0xB0,     /* R4  - PLL2 */
	0x21,     /* R5  - PLL3 */

	0x07,     /* R6  - PLL4 */
//	0x08,     /* R6  - PLL4 */
	
	0x16,     /* R7  - PLL5 */
//	0x1D,     /* R7  - PLL5 */ // MCLKDIV = 0
#elif 0//MCLK : 24.576MHz, LRCLK : 96KHz
	0xBA,     /* R3  - PLL1 */
	0x49,     /* R4  - PLL2 */
	0x0C,     /* R5  - PLL3 */
	0x08,     /* R6  - PLL4 */
	
	0x0D,     /* R7  - PLL5 */
#elif 0//MCLK : 24.576MHz, LRCLK : 192KHz
	0xBA,     /* R3  - PLL1 */
	0x49,     /* R4  - PLL2 */
	0x0C,     /* R5  - PLL3 */
	0x08,     /* R6  - PLL4 */
	
	0x0C,     /* R7  - PLL5 */

#else
#endif
//	0x18,     /* R8  - PLL6 *///--> 7:1 =>MCLK Source:CLK2, CLKOUT Pin:Enable, CLKOUT source: OSC
	0x10,     /* R8  - PLL6 */ //--> 7:1 =>Select OSCCLK, CLKOUT source: CLK1, MCLK Source : CLK2
//	0x98,     /* R8  - PLL6 */ //--> 7:1 =>Select OSCCLK
//	0x88,     /* R8  - PLL6 */ //--> 7:1 =>Select OSCCLK, CLKOUT Disable
//	0x10,     /* R8  - PLL6 */ //--> 7:1 =>OSCCLK Source: CLK1 , CLKOUT Pin :Enable


	0xFF,     /* R9  - SPDMODE */
	0x00,     /* R10 - INTMASK */
	0x00,     /* R11 - INTSTAT */
	0x00,     /* R12 - SPDSTAT */
	0x00,     /* R13 - RXCHAN1 */
	0x00,     /* R14 - RXCHAN2 */
	0x00,     /* R15 - RXCHAN3 */
	0x00,     /* R16 - RXCHAN4 */
	0x00,     /* R17 - RXCHAN5 */
	0x00,     /* R18 - SPDTX1 */
	0x00,     /* R19 - SPDTX2 */
	0x00,     /* R20 - SPDTX3 */
	0x71,     /* R21 - SPDTX4 */
	0x0B,     /* R22 - SPDTX5 */
	0x70,     /* R23 - GPO0 */
	0x57,     /* R24 - GPO1 */
	0x00,     /* R25 */
	0x42,     /* R26 - GPO2 */

//	0x06,     /* R27 - AIFTX */
//	0x02,
	0x0A,

//	0x06,     /* R28 - AIFRX */	//-->slave mode(MCLK, LRCK, BCLK are inputs), 20bit, I2S
//	0x02,     /* R28 - AIFRX */	//-->slave mode(MCLK, LRCK, BCLK are inputs), 16bit, I2S
//	0x42,     /* R28 - AIFRX */	//-->master mode(MCLK, LRCK, BCLK are outputs), 16bit, I2S
	0x4A,     /* R28 - AIFRX */	//-->master mode(MCLK, LRCK, BCLK are outputs), 24bit, I2S

	0x80,     /* R29 - SPDRX1 */

	//0x07,     /* R30 - PWRDN */
	//0x0f,     /* R30 - PWRDN */// Oscillator Power down
	0x06
};
#endif

#if 0 //
SPDIF REC
static const u8 wm8804_reg[WM8804_REGISTER_COUNT] = {
	0x05,     /* R0  - RST/DEVID1 */
	0x88,     /* R1  - DEVID2 */
	0x04,     /* R2  - DEVREV */
	0x21,     /* R3  - PLL1 */
	0xFD,     /* R4  - PLL2 */
	0x36,     /* R5  - PLL3 */
	0x07,     /* R6  - PLL4 */	
	0x0C,     /* R7  - PLL5 */
	0x10,     /* R8  - PLL6 */ //--> 7:1 =>Select OSCCLK, CLKOUT source: CLK1, MCLK Source : CLK2
	0xFF,     /* R9  - SPDMODE */
	0x00,     /* R10 - INTMASK */

	0x00,     /* R11 - INTSTAT */
	0x00,     /* R12 - SPDSTAT */
	0x00,     /* R13 - RXCHAN1 */
	0x00,     /* R14 - RXCHAN2 */
	0x00,     /* R15 - RXCHAN3 */
	0x00,     /* R16 - RXCHAN4 */
	0x00,     /* R17 - RXCHAN5 */
	0x00,     /* R18 - SPDTX1 */
	0x00,     /* R19 - SPDTX2 */
	0x00,     /* R20 - SPDTX3 */

	0x71,     /* R21 - SPDTX4 */	// DAI Receive Data	
	0x0B,     /* R22 - SPDTX5 */
	0x70,     /* R23 - GPO0 */
	0x57,     /* R24 - GPO1 */
	0x00,     /* R25 */
	0x42,     /* R26 - GPO2 */

	0x02,     /* R27 - AIFTX */


//	0x06,     /* R28 - AIFRX */	//-->slave mode(MCLK, LRCK, BCLK are inputs), 20bit, I2S
	0x42,     /* R28 - AIFRX */	//-->master mode(MCLK, LRCK, BCLK are outputs), 16bit, I2S
//	0x02,     /* R28 - AIFRX */	//-->slave mode(MCLK, LRCK, BCLK are inputs), 16bit, I2S

	0x80,     /* R29 - SPDRX1 */

	//0x07,     /* R30 - PWRDN */
	//0x0f,     /* R30 - PWRDN */// Oscillator Power down
	//0x06
	//0x00	  /* R30 - PWRDN */// SPDIF rec, tr enable
	//0x02	/* R30 - PWRDN */// SPDIF rec disable, tr enable
	0x04	/* R30 - PWRDN */// SPDIF rec enable, tr disable
};
#endif
#if 0 //optical bypass
static const u8 wm8804_reg[WM8804_REGISTER_COUNT] = {
	0x05,     /* R0  - RST/DEVID1 */
	0x88,     /* R1  - DEVID2 */
	0x04,     /* R2  - DEVREV */
#if 1 //
	0x21,     /* R3  - PLL1 */
	0xFD,     /* R4  - PLL2 */
	0x36,     /* R5  - PLL3 */
	0x07,     /* R6  - PLL4 */	
	0x34,     /* R7  - PLL5 */ //--> CLKOUTDIV
#else
	0xBA,     /* R3  - PLL1 */
	0x49,     /* R4  - PLL2 */
	0x0C,     /* R5  - PLL3 */
	0x08,     /* R6  - PLL4 */
	0x0C,     /* R7  - PLL5 */
#endif

	0x10,     /* R8  - PLL6 */ //--> 7:1 =>Select OSCCLK, CLKOUT source: CLK1, MCLK Source : CLK2
	0xFF,     /* R9  - SPDMODE */
	0x00,     /* R10 - INTMASK */

	0x00,     /* R11 - INTSTAT */
	0x00,     /* R12 - SPDSTAT */
	0x00,     /* R13 - RXCHAN1 */
	0x00,     /* R14 - RXCHAN2 */
	0x00,     /* R15 - RXCHAN3 */
	0x00,     /* R16 - RXCHAN4 */
	0x00,     /* R17 - RXCHAN5 */
	0x00,     /* R18 - SPDTX1 */
	0x00,     /* R19 - SPDTX2 */
	0x00,     /* R20 - SPDTX3 */

	0x71,     /* R21 - SPDTX4 */	// DAI Receive Data	
	0x0B,     /* R22 - SPDTX5 */
	0x70,     /* R23 - GPO0 */
	0x57,     /* R24 - GPO1 */
	0x00,     /* R25 */
	0x42,     /* R26 - GPO2 */

	0x02,     /* R27 - AIFTX */


//	0x06,     /* R28 - AIFRX */	//-->slave mode(MCLK, LRCK, BCLK are inputs), 20bit, I2S
	0x42,     /* R28 - AIFRX */	//-->master mode(MCLK, LRCK, BCLK are outputs), 16bit, I2S
//	0x02,     /* R28 - AIFRX */	//-->slave mode(MCLK, LRCK, BCLK are inputs), 16bit, I2S

	0x80,     /* R29 - SPDRX1 */

	//0x07,     /* R30 - PWRDN */
	//0x0f,     /* R30 - PWRDN */// Oscillator Power down
	//0x06
	//0x00	  /* R30 - PWRDN */// SPDIF rec, tr enable
	//0x02	/* R30 - PWRDN */// SPDIF rec disable, tr enable
	0x04	/* R30 - PWRDN */// SPDIF rec enable, tr disable
};
#endif

#if 0 // Optical Play
static const u8 wm8804_reg[WM8804_REGISTER_COUNT] = {
	0x05,     /* R0  - RST/DEVID1 */
	0x88,     /* R1  - DEVID2 */
	0x04,     /* R2  - DEVREV */

#if 0 //MCLK : 12.288MHz, LRCLK : 48KHz
	0xBA,     /* R3  - PLL1 */
	0x49,     /* R4  - PLL2 */
	0x0C,     /* R5  - PLL3 */
	0x08,     /* R6  - PLL4 */
	0x16,     /* R7  - PLL5 */

#elif 1//MCLK : 11.2896MHz, LRCLK : 44.1KHz
	0x89,     /* R3  - PLL1 */
	0xB0,     /* R4  - PLL2 */
	0x21,     /* R5  - PLL3 */

	0x07,     /* R6  - PLL4 */
//	0x08,     /* R6  - PLL4 */
	
	0x16,     /* R7  - PLL5 */
//	0x1D,     /* R7  - PLL5 */ // MCLKDIV = 0
#elif 0//MCLK : 24.576MHz, LRCLK : 96KHz
	0xBA,     /* R3  - PLL1 */
	0x49,     /* R4  - PLL2 */
	0x0C,     /* R5  - PLL3 */
	0x08,     /* R6  - PLL4 */
	
	0x0D,     /* R7  - PLL5 */
#elif 1//MCLK : 24.576MHz, LRCLK : 192KHz
	0xBA,     /* R3  - PLL1 */
	0x49,     /* R4  - PLL2 */
	0x0C,     /* R5  - PLL3 */
	0x08,     /* R6  - PLL4 */
	
	0x0C,     /* R7  - PLL5 */

#else
#endif
//	0x18,     /* R8  - PLL6 *///--> 7:1 =>MCLK Source:CLK2, CLKOUT Pin:Enable, CLKOUT source: OSC
	0x10,     /* R8  - PLL6 */ //--> 7:1 =>Select OSCCLK, CLKOUT source: CLK1, MCLK Source : CLK2
//	0x98,     /* R8  - PLL6 */ //--> 7:1 =>Select OSCCLK
//	0x88,     /* R8  - PLL6 */ //--> 7:1 =>Select OSCCLK, CLKOUT Disable
//	0x10,     /* R8  - PLL6 */ //--> 7:1 =>OSCCLK Source: CLK1 , CLKOUT Pin :Enable


	0xFF,     /* R9  - SPDMODE */
	0x00,     /* R10 - INTMASK */
	0x00,     /* R11 - INTSTAT */
	0x00,     /* R12 - SPDSTAT */
	0x00,     /* R13 - RXCHAN1 */
	0x00,     /* R14 - RXCHAN2 */
	0x00,     /* R15 - RXCHAN3 */
	0x00,     /* R16 - RXCHAN4 */
	0x00,     /* R17 - RXCHAN5 */
	0x00,     /* R18 - SPDTX1 */
	0x00,     /* R19 - SPDTX2 */
	0x00,     /* R20 - SPDTX3 */
	0x71,     /* R21 - SPDTX4 */
	0x0B,     /* R22 - SPDTX5 */
	0x70,     /* R23 - GPO0 */
	0x57,     /* R24 - GPO1 */
	0x00,     /* R25 */
	0x42,     /* R26 - GPO2 */

//	0x06,     /* R27 - AIFTX */
	0x02,

//	0x06,     /* R28 - AIFRX */	//-->slave mode(MCLK, LRCK, BCLK are inputs), 20bit, I2S
//	0x02,     /* R28 - AIFRX */	//-->slave mode(MCLK, LRCK, BCLK are inputs), 16bit, I2S
	0x42,     /* R28 - AIFRX */	//-->master mode(MCLK, LRCK, BCLK are outputs), 16bit, I2S
//	0x4A,     /* R28 - AIFRX */	//-->master mode(MCLK, LRCK, BCLK are outputs), 24bit, I2S

	0x80,     /* R29 - SPDRX1 */

	//0x06
	//0x00	  /* R30 - PWRDN */// SPDIF rec, tr enable
	0x02	/* R30 - PWRDN */// SPDIF rec disable, tr enable
	//0x04	/* R30 - PWRDN */// SPDIF rec enable, tr disable
};
#endif


#if 0// Optical loop(spdif receve -> spdif transter)
static const u8 wm8804_reg[WM8804_REGISTER_COUNT] = {
	0x05,     /* R0  - RST/DEVID1 */
	0x88,     /* R1  - DEVID2 */
	0x04,     /* R2  - DEVREV */

#if 0 //MCLK : 12.288MHz, LRCLK : 48KHz
	0xBA,     /* R3  - PLL1 */
	0x49,     /* R4  - PLL2 */
	0x0C,     /* R5  - PLL3 */
	0x08,     /* R6  - PLL4 */
	0x16,     /* R7  - PLL5 */

#elif 0//MCLK : 11.2896MHz, LRCLK : 44.1KHz
	0x89,     /* R3  - PLL1 */
	0xB0,     /* R4  - PLL2 */
	0x21,     /* R5  - PLL3 */

	0x07,     /* R6  - PLL4 */
//	0x08,     /* R6  - PLL4 */
	
	0x16,     /* R7  - PLL5 */
//	0x1D,     /* R7  - PLL5 */ // MCLKDIV = 0
#elif 0//MCLK : 24.576MHz, LRCLK : 96KHz
	0xBA,     /* R3  - PLL1 */
	0x49,     /* R4  - PLL2 */
	0x0C,     /* R5  - PLL3 */
	0x08,     /* R6  - PLL4 */
	
	0x0D,     /* R7  - PLL5 */
#elif 1//MCLK : 24.576MHz, LRCLK : 192KHz
	0xBA,     /* R3  - PLL1 */
	0x49,     /* R4  - PLL2 */
	0x0C,     /* R5  - PLL3 */
	0x08,     /* R6  - PLL4 */
	
	0x0C,     /* R7  - PLL5 */

#else
#endif
//	0x18,     /* R8  - PLL6 *///--> 7:1 =>MCLK Source:CLK2, CLKOUT Pin:Enable, CLKOUT source: OSC
	0x10,     /* R8  - PLL6 */ //--> 7:1 =>Select OSCCLK, CLKOUT source: CLK1, MCLK Source : CLK2
//	0x98,     /* R8  - PLL6 */ //--> 7:1 =>Select OSCCLK
//	0x88,     /* R8  - PLL6 */ //--> 7:1 =>Select OSCCLK, CLKOUT Disable
//	0x10,     /* R8  - PLL6 */ //--> 7:1 =>OSCCLK Source: CLK1 , CLKOUT Pin :Enable


	0xFF,     /* R9  - SPDMODE */
	0x00,     /* R10 - INTMASK */
	0x00,     /* R11 - INTSTAT */
	0x00,     /* R12 - SPDSTAT */
	0x00,     /* R13 - RXCHAN1 */
	0x00,     /* R14 - RXCHAN2 */
	0x00,     /* R15 - RXCHAN3 */
	0x00,     /* R16 - RXCHAN4 */
	0x00,     /* R17 - RXCHAN5 */
	0x00,     /* R18 - SPDTX1 */
	0x00,     /* R19 - SPDTX2 */
	0x00,     /* R20 - SPDTX3 */

//	0x71,     /* R21 - SPDTX4 */
	0x31,     /* R21 - SPDTX4 */ // spdif receive -> spdif transfer

	0x0B,     /* R22 - SPDTX5 */
	0x70,     /* R23 - GPO0 */
	0x57,     /* R24 - GPO1 */
	0x00,     /* R25 */
	0x42,     /* R26 - GPO2 */

//	0x06,     /* R27 - AIFTX */
//	0x02,
	0x0A,
	
//	0x06,     /* R28 - AIFRX */	//-->slave mode(MCLK, LRCK, BCLK are inputs), 20bit, I2S
//	0x02,     /* R28 - AIFRX */	//-->slave mode(MCLK, LRCK, BCLK are inputs), 16bit, I2S
//	0x42,     /* R28 - AIFRX */	//-->master mode(MCLK, LRCK, BCLK are outputs), 16bit, I2S
	0x4A,     /* R28 - AIFRX */	//-->master mode(MCLK, LRCK, BCLK are outputs), 24bit, I2S

	0x80,     /* R29 - SPDRX1 */

	//0x06
	0x00	  /* R30 - PWRDN */// SPDIF rec, tr enable
	//0x02	/* R30 - PWRDN */// SPDIF rec disable, tr enable
	//0x04	/* R30 - PWRDN */// SPDIF rec enable, tr disable
};
#endif

struct wm8804_priv {
	enum snd_soc_control_type control_type;
//	struct regulator_bulk_data supplies[WM8804_NUM_SUPPLIES];
	struct notifier_block disable_nb[WM8804_NUM_SUPPLIES];
	struct snd_soc_codec *codec;
};

static struct snd_soc_codec *wm8804_codec;
#if 0
static int txsrc_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *ucontrol);

static int txsrc_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *ucontrol);

/*
 * We can't use the same notifier block for more than one supply and
 * there's no way I can see to get from a callback to the caller
 * except container_of().
 */
#define WM8804_REGULATOR_EVENT(n) \
static int wm8804_regulator_event_##n(struct notifier_block *nb, \
				      unsigned long event, void *data)    \
{ \
	struct wm8804_priv *wm8804 = container_of(nb, struct wm8804_priv, \
						  disable_nb[n]); \
	if (event & REGULATOR_EVENT_DISABLE) { \
		wm8804->codec->cache_sync = 1; \
	} \
	return 0; \
}

WM8804_REGULATOR_EVENT(0)
WM8804_REGULATOR_EVENT(1)

static const char *txsrc_text[] = { "S/PDIF RX", "AIF" };
static const SOC_ENUM_SINGLE_EXT_DECL(txsrc, txsrc_text);

static const struct snd_kcontrol_new wm8804_snd_controls[] = {
	SOC_ENUM_EXT("Input Source", txsrc, txsrc_get, txsrc_put),
	SOC_SINGLE("TX Playback Switch", WM8804_PWRDN, 2, 1, 1),
	SOC_SINGLE("AIF Playback Switch", WM8804_PWRDN, 4, 1, 1)
};

static int txsrc_get(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec;
	unsigned int src;

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	codec = snd_kcontrol_chip(kcontrol);
	src = snd_soc_read(codec, WM8804_SPDTX4);
	if (src & 0x40)
		ucontrol->value.integer.value[0] = 1;
	else
		ucontrol->value.integer.value[0] = 0;

	return 0;
}

static int txsrc_put(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec;
	unsigned int src, txpwr;

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	codec = snd_kcontrol_chip(kcontrol);

	if (ucontrol->value.integer.value[0] != 0
			&& ucontrol->value.integer.value[0] != 1)
		return -EINVAL;

	src = snd_soc_read(codec, WM8804_SPDTX4);
	switch ((src & 0x40) >> 6) {
	case 0:
		if (!ucontrol->value.integer.value[0])
			return 0;
		break;
	case 1:
		if (ucontrol->value.integer.value[1])
			return 0;
		break;
	}

	/* save the current power state of the transmitter */
	txpwr = snd_soc_read(codec, WM8804_PWRDN) & 0x4;
	/* power down the transmitter */
	snd_soc_update_bits(codec, WM8804_PWRDN, 0x4, 0x4);
	/* set the tx source */
	snd_soc_update_bits(codec, WM8804_SPDTX4, 0x40,
			    ucontrol->value.integer.value[0] << 6);

	if (ucontrol->value.integer.value[0]) {
		/* power down the receiver */
		snd_soc_update_bits(codec, WM8804_PWRDN, 0x2, 0x2);
		/* power up the AIF */
		snd_soc_update_bits(codec, WM8804_PWRDN, 0x10, 0);
	} else {
		/* don't power down the AIF -- may be used as an output */
		/* power up the receiver */
		snd_soc_update_bits(codec, WM8804_PWRDN, 0x2, 0);
	}

	/* restore the transmitter's configuration */
	snd_soc_update_bits(codec, WM8804_PWRDN, 0x4, txpwr);

	return 0;
}

#endif

static int wm8804_volatile(struct snd_soc_codec *codec, unsigned int reg)
{
	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	switch (reg) {
	case WM8804_RST_DEVID1:
	case WM8804_DEVID2:
	case WM8804_DEVREV:
	case WM8804_INTSTAT:
	case WM8804_SPDSTAT:
	case WM8804_RXCHAN1:
	case WM8804_RXCHAN2:
	case WM8804_RXCHAN3:
	case WM8804_RXCHAN4:
	case WM8804_RXCHAN5:
		return 1;
	default:
		break;
	}

	return 0;
}

static int wm8804_reset(struct snd_soc_codec *codec)
{
	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 1;
	#endif

	return snd_soc_write(codec, WM8804_RST_DEVID1, 0x0);
}

static int wm8804_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_codec *codec;
	u16 format, master, bcp, lrp;

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	codec = dai->codec;

	#ifdef PRINT_ALSA_MSG
	DBG_PRINT("WM8804","fmt %d\n",fmt);
	#endif

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		format = 0x2;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		format = 0x0;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		format = 0x1;
		break;
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		format = 0x3;
		break;
	default:
		dev_err(dai->dev, "Unknown dai format\n");
		return -EINVAL;
	}

	/* set data format */
	snd_soc_update_bits(codec, WM8804_AIFTX, 0x3, format);
	snd_soc_update_bits(codec, WM8804_AIFRX, 0x3, format);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		master = 1;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		master = 0;
		break;
	default:
		dev_err(dai->dev, "Unknown master/slave configuration\n");
		return -EINVAL;
	}

	/* set master/slave mode */
	snd_soc_update_bits(codec, WM8804_AIFRX, 0x40, master << 6);

	bcp = lrp = 0;
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_IB_IF:
		bcp = lrp = 1;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		bcp = 1;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		lrp = 1;
		break;
	default:
		dev_err(dai->dev, "Unknown polarity configuration\n");
		return -EINVAL;
	}

	/* set frame inversion */
	snd_soc_update_bits(codec, WM8804_AIFTX, 0x10 | 0x20,
			    (bcp << 4) | (lrp << 5));
	snd_soc_update_bits(codec, WM8804_AIFRX, 0x10 | 0x20,
			    (bcp << 4) | (lrp << 5));
	return 0;
}

static int wm8804_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec;
	u16 blen;

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	codec = dai->codec;
	#ifdef PRINT_ALSA_MSG	
	DBG_PRINT("WM8804","%d\n",params_format(params));
	#endif

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		blen = 0x0;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		blen = 0x1;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		blen = 0x2;
		break;
	default:
		dev_err(dai->dev, "Unsupported word length: %u\n",
			params_format(params));
		return -EINVAL;
	}

	/* set word length */
//	blen = 0x2;

	snd_soc_update_bits(codec, WM8804_AIFTX, 0xc, blen << 2);
	snd_soc_update_bits(codec, WM8804_AIFRX, 0xc, blen << 2);

	return 0;
}

struct pll_div {
	u32 prescale:1;
	u32 mclkdiv:1;
	u32 freqmode:2;
	u32 n:4;
	u32 k:22;
};

/* PLL rate to output rate divisions */
static struct {
	unsigned int div;
	unsigned int freqmode;
	unsigned int mclkdiv;
} post_table[] = {
	{  2,  0, 0 },
	{  4,  0, 1 },
	{  4,  1, 0 },
	{  8,  1, 1 },
	{  8,  2, 0 },
	{ 16,  2, 1 },
	{ 12,  3, 0 },
	{ 24,  3, 1 }
};


#define FIXED_PLL_SIZE ((1ULL << 22) * 10)
static int pll_factors(struct pll_div *pll_div, unsigned int target,
		       unsigned int source)
{
	u64 Kpart;
	unsigned long int K, Ndiv, Nmod, tmp;
	int i;

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	/*
	 * Scale the output frequency up; the PLL should run in the
	 * region of 90-100MHz.
	 */
	for (i = 0; i < ARRAY_SIZE(post_table); i++) {
		tmp = target * post_table[i].div;
		if (tmp >= 90000000 && tmp <= 100000000) {
			pll_div->freqmode = post_table[i].freqmode;
			pll_div->mclkdiv = post_table[i].mclkdiv;
			target *= post_table[i].div;
			break;
		}
	}

	if (i == ARRAY_SIZE(post_table)) {
		pr_err("%s: Unable to scale output frequency: %uHz\n",
		       __func__, target);
		return -EINVAL;
	}

	pll_div->prescale = 0;
	Ndiv = target / source;
	if (Ndiv < 5) {
		source >>= 1;
		pll_div->prescale = 1;
		Ndiv = target / source;
	}

	if (Ndiv < 5 || Ndiv > 13) {
		pr_err("%s: WM8804 N value is not within the recommended range: %lu\n",
		       __func__, Ndiv);
		return -EINVAL;
	}
	pll_div->n = Ndiv;

	Nmod = target % source;
	Kpart = FIXED_PLL_SIZE * (u64)Nmod;

	do_div(Kpart, source);

	K = Kpart & 0xffffffff;
	if ((K % 10) >= 5)
		K += 5;
	K /= 10;
	pll_div->k = K;

	return 0;
}

#ifdef FIX_MUTE_NOISE
static DEFINE_MUTEX(g_wm8804_audio_pll);

static int g_prev_pll_freq = -1;
static int g_request_unmute = 0;

void request_wm8804_set_pll(void)
{
	g_prev_pll_freq = -1;
}
int is_request_unmute(void)
{
	return g_request_unmute;
}
int request_unmute(void)
{
	g_request_unmute = 1;
}
int reset_unmute(void)
{
	g_request_unmute = 0;

}
#endif

#if(AUDIO_CLOCK == CLK_AK8157A)
int WM8804_SETPLL(unsigned int freq_in)
{
	struct snd_soc_codec *codec;


	if(g_prev_pll_freq == -1 || g_prev_pll_freq !=freq_in)  
	{

		//CPRINT("WM8804 PLL\n");
		
		ak_set_hw_mute(AMT_HP_MUTE, 1);

		#ifdef FIX_MUTE_NOISE		
		#if(CUR_DAC == CODEC_CS4398) 
		msleep_interruptible(50);	
		cs4398_FM(freq_in);
		#elif(CUR_DAC == CODEC_AK4490)
		ak4490_FM(params_rate(freq_in));
		#endif

		g_prev_pll_freq = freq_in;
 		g_request_unmute = 1;
		//CPRINT("wm8804 pll:%d\n",freq_in);	
        #ifdef REDUCE_GAP
 		ak_set_hw_mute(AMT_HP_MUTE, 0);
		set_audio_start_delay(0);
		#else
		set_audio_start_delay(130);		
        #endif 
		#endif
	}

	else {
		//CPRINT("wm8804 pll same:%d\n",freq_in);
		set_audio_start_delay(10);
	}

	return 0;
}

#else
 int WM8804_SETPLL(unsigned int freq_in)
{
	struct snd_soc_codec *codec;


	if(!wm8804_codec) return -1;
	
	codec = wm8804_codec;

//	dump_stack();
	#ifdef PRINT_ALSA_MSG	
	DBG_PRINT("WM8804","freq_in:%d\n",freq_in);
	#endif
	#if 0
	if (!freq_in || !freq_out) {
		/* disable the PLL */
		snd_soc_update_bits(codec, WM8804_PWRDN, 0x1, 0x1);
		return 0;
	} else {
		int ret;
		struct pll_div pll_div;

		ret = pll_factors(&pll_div, freq_out, freq_in);
		if (ret)
			return ret;

		/* power down the PLL before reprogramming it */
		snd_soc_update_bits(codec, WM8804_PWRDN, 0x1, 0x1);

		if (!freq_in || !freq_out)
			return 0;

		/* set PLLN and PRESCALE */
		snd_soc_update_bits(codec, WM8804_PLL4, 0xf | 0x10,
				    pll_div.n | (pll_div.prescale << 4));
		/* set mclkdiv and freqmode */
		snd_soc_update_bits(codec, WM8804_PLL5, 0x3 | 0x8,
				    pll_div.freqmode | (pll_div.mclkdiv << 3));
		/* set PLLK */
		snd_soc_write(codec, WM8804_PLL1, pll_div.k & 0xff);
		snd_soc_write(codec, WM8804_PLL2, (pll_div.k >> 8) & 0xff);
		snd_soc_write(codec, WM8804_PLL3, pll_div.k >> 16);

		/* power up the PLL */
		snd_soc_update_bits(codec, WM8804_PWRDN, 0x1, 0);
	}
	#else

#ifdef FIX_MUTE_NOISE

	if(g_prev_pll_freq == -1 || g_prev_pll_freq !=freq_in)  {

		//CPRINT("WM8804 PLL\n");
		
		ak_set_hw_mute(AMT_HP_MUTE, 1);
#endif
		snd_soc_update_bits(codec, WM8804_PWRDN, 0x1, 0x1);

		if(freq_in == 44100)
		{
			snd_soc_write(codec, WM8804_PLL1, 0x89);
			snd_soc_write(codec, WM8804_PLL2, 0xB0);
			snd_soc_write(codec, WM8804_PLL3, 0x21);
			snd_soc_write(codec, WM8804_PLL4, 0x07);
			snd_soc_write(codec, WM8804_PLL5, 0x16);	//upsampling
			//snd_soc_write(codec, WM8804_PLL5, 0x1E);
		}
		else if(freq_in == 48000)
		{
			snd_soc_write(codec, WM8804_PLL1, 0xBA);
			snd_soc_write(codec, WM8804_PLL2, 0x49);
			snd_soc_write(codec, WM8804_PLL3, 0x0C);
			snd_soc_write(codec, WM8804_PLL4, 0x08);
			snd_soc_write(codec, WM8804_PLL5, 0x16);	//upsampling
			//snd_soc_write(codec, WM8804_PLL5, 0x1E);
		}
		else if(freq_in == 96000)
		{
			snd_soc_write(codec, WM8804_PLL1, 0xBA);
			snd_soc_write(codec, WM8804_PLL2, 0x49);
			snd_soc_write(codec, WM8804_PLL3, 0x0C);
			snd_soc_write(codec, WM8804_PLL4, 0x08);
			snd_soc_write(codec, WM8804_PLL5, 0x0D);
		}
		else if(freq_in == 192000)
		{
			snd_soc_write(codec, WM8804_PLL1, 0xBA);
			snd_soc_write(codec, WM8804_PLL2, 0x49);
			snd_soc_write(codec, WM8804_PLL3, 0x0C);
			snd_soc_write(codec, WM8804_PLL4, 0x08);
			snd_soc_write(codec, WM8804_PLL5, 0x0C);
		}
		else if(freq_in == 88200)
		{
			snd_soc_write(codec, WM8804_PLL1, 0x89);
			snd_soc_write(codec, WM8804_PLL2, 0xB0);
			snd_soc_write(codec, WM8804_PLL3, 0x21);
			snd_soc_write(codec, WM8804_PLL4, 0x07);
			//snd_soc_write(codec, WM8804_PLL5, 0x16);
			//snd_soc_write(codec, WM8804_PLL5, 0x15);
			snd_soc_write(codec, WM8804_PLL5, 0x1D);
		}
		else if(freq_in == 176400)
		{
			snd_soc_write(codec, WM8804_PLL1, 0x89);
			snd_soc_write(codec, WM8804_PLL2, 0xB0);
			snd_soc_write(codec, WM8804_PLL3, 0x21);
			snd_soc_write(codec, WM8804_PLL4, 0x07);
	//			snd_soc_write(codec, WM8804_PLL5, 0x14);
			snd_soc_write(codec, WM8804_PLL5, 0x1c);
		}
		snd_soc_update_bits(codec, WM8804_PWRDN, 0x1, 0);

#ifdef FIX_MUTE_NOISE		
		#if(CUR_DAC == CODEC_CS4398) 
		msleep_interruptible(50);
		cs4398_FM(freq_in);
		#elif(CUR_DAC == CODEC_AK4490)
		ak4490_FM(params_rate(freq_in));
		#endif

		g_prev_pll_freq = freq_in;
 		g_request_unmute = 1;
		//CPRINT("wm8804 pll:%d\n",freq_in);	
        #ifdef REDUCE_GAP
 		ak_set_hw_mute(AMT_HP_MUTE, 0);
		set_audio_start_delay(0);
		#else
		set_audio_start_delay(130);		
        #endif 
		
	}

	else {
		//CPRINT("wm8804 pll same:%d\n",freq_in);
		set_audio_start_delay(10);
	}
#endif

	#endif
	return 0;
}
#endif

static int wm8804_set_pll(struct snd_soc_dai *dai, int pll_id,
			  int source, unsigned int freq_in,
			  unsigned int freq_out)
{
 
	#ifdef AK_AUDIO_MODULE_USED 
 	return 0;
	#endif

	WM8804_SETPLL(freq_in);

	return 0;
}

static int wm8804_set_sysclk(struct snd_soc_dai *dai,
			     int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec;

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	codec = dai->codec;

	DBG_PRINT("WM8804","clk_id:%d freq:%d dir:%d\n",clk_id,freq,dir);
	

	switch (clk_id) {
	case WM8804_TX_CLKSRC_MCLK:
		#if 0
		if ((freq >= 10000000 && freq <= 14400000)
				|| (freq >= 16280000 && freq <= 27000000))
			snd_soc_update_bits(codec, WM8804_PLL6, 0x80, 0x80);
		else {
			dev_err(dai->dev, "OSCCLOCK is not within the "
				"recommended range: %uHz\n", freq);
			return -EINVAL;
		}
		#else

		printk("WM8804_MCLK_SETTING\n");
		if(freq == 44100)
		{
			snd_soc_write(codec, WM8804_PLL1, 0x89);
			snd_soc_write(codec, WM8804_PLL2, 0xB0);
			snd_soc_write(codec, WM8804_PLL3, 0x21);
			snd_soc_write(codec, WM8804_PLL4, 0x07);
			snd_soc_write(codec, WM8804_PLL5, 0x16);
		}
		else if(freq == 48000)
		{
			snd_soc_write(codec, WM8804_PLL1, 0xBA);
			snd_soc_write(codec, WM8804_PLL2, 0x49);
			snd_soc_write(codec, WM8804_PLL3, 0x0C);
			snd_soc_write(codec, WM8804_PLL4, 0x08);
			snd_soc_write(codec, WM8804_PLL5, 0x16);
		}
		else if(freq == 96000)
		{
			snd_soc_write(codec, WM8804_PLL1, 0xBA);
			snd_soc_write(codec, WM8804_PLL2, 0x49);
			snd_soc_write(codec, WM8804_PLL3, 0x0C);
			snd_soc_write(codec, WM8804_PLL4, 0x08);
			snd_soc_write(codec, WM8804_PLL5, 0x0D);
		}
		else if(freq == 192000)
		{
			snd_soc_write(codec, WM8804_PLL1, 0xBA);
			snd_soc_write(codec, WM8804_PLL2, 0x49);
			snd_soc_write(codec, WM8804_PLL3, 0x0C);
			snd_soc_write(codec, WM8804_PLL4, 0x08);
			snd_soc_write(codec, WM8804_PLL5, 0x0C);
		}


		#endif
		break;
	case WM8804_TX_CLKSRC_PLL:
		snd_soc_update_bits(codec, WM8804_PLL6, 0x80, 0);
		break;
	case WM8804_CLKOUT_SRC_CLK1:
		snd_soc_update_bits(codec, WM8804_PLL6, 0x8, 0);
		break;
	case WM8804_CLKOUT_SRC_OSCCLK:
		snd_soc_update_bits(codec, WM8804_PLL6, 0x8, 0x8);
		break;
	default:
		dev_err(dai->dev, "Unknown clock source: %d\n", clk_id);
		return -EINVAL;
	}

	return 0;
}

static int wm8804_set_clkdiv(struct snd_soc_dai *dai,
			     int div_id, int div)
{
	struct snd_soc_codec *codec;

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	codec = dai->codec;
	switch (div_id) {
	case WM8804_CLKOUT_DIV:
		snd_soc_update_bits(codec, WM8804_PLL5, 0x30,
				    (div & 0x3) << 4);
		break;
	default:
		dev_err(dai->dev, "Unknown clock divider: %d\n", div_id);
		return -EINVAL;
	}
	return 0;
}


void wm8804_reinit(void)
{
	int i;

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return;
	#endif

	printk("%s\n", __FUNCTION__);

	if(wm8804_codec != NULL)
	{
		wm8804_reset(wm8804_codec);

		for (i = 0; i < WM8804_REGISTER_COUNT; i++) {
			snd_soc_write(wm8804_codec, i, wm8804_reg[i]);
		}
	}
	else
	{
		printk("Error:%s\n", __FUNCTION__);
	}

}


static void wm8804_sync_cache(struct snd_soc_codec *codec)
{
	short i;
	u8 *cache;

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return;
	#endif

	if (!codec->cache_sync)
		return;

	codec->cache_only = 0;
	cache = codec->reg_cache;
	for (i = 0; i < codec->driver->reg_cache_size; i++) {
		if (i == WM8804_RST_DEVID1 || cache[i] == wm8804_reg[i])
			continue;
		snd_soc_write(codec, i, cache[i]);
	}
	codec->cache_sync = 0;
}

static int wm8804_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	int ret;
	struct wm8804_priv *wm8804;

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	wm8804 = snd_soc_codec_get_drvdata(codec);
	switch (level) {
	case SND_SOC_BIAS_ON:
		break;
	case SND_SOC_BIAS_PREPARE:
		/* power up the OSC and the PLL */
		snd_soc_update_bits(codec, WM8804_PWRDN, 0x9, 0);
		break;
	case SND_SOC_BIAS_STANDBY:
		if (codec->dapm.bias_level == SND_SOC_BIAS_OFF) {
			#if 0
			ret = regulator_bulk_enable(ARRAY_SIZE(wm8804->supplies),
						    wm8804->supplies);
			if (ret) {
				dev_err(codec->dev,
					"Failed to enable supplies: %d\n",
					ret);
				return ret;
			}
			#endif
			
			wm8804_sync_cache(codec);
		}
		/* power down the OSC and the PLL */
		//snd_soc_update_bits(codec, WM8804_PWRDN, 0x9, 0x9);
		break;
	case SND_SOC_BIAS_OFF:
		/* power down the OSC and the PLL */
		snd_soc_update_bits(codec, WM8804_PWRDN, 0x9, 0x9);
		#if 0
		regulator_bulk_disable(ARRAY_SIZE(wm8804->supplies),
				       wm8804->supplies);
		#endif
		break;
	}

	codec->dapm.bias_level = level;
	return 0;
}

#ifdef CONFIG_PM
extern int get_pcfi_suspend_enable(void);

static int wm8804_suspend(struct snd_soc_codec *codec, pm_message_t state)
{
	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	wm8804_set_bias_level(codec, SND_SOC_BIAS_OFF);
    if (!get_pcfi_suspend_enable()) {
        /* [downer] A138027 */
		#ifdef AUDIO_POWER_CTL

		#else
		ak_set_hw_mute(AMT_DAC_MUTE ,1);
        ak_set_hw_mute(AMT_HP_MUTE ,1);
	
         ak_set_snd_pwr_ctl(AK_AUDIO_POWER, AK_PWR_OFF);  
		 #endif
    }
    
	return 0;
}

static int wm8804_resume(struct snd_soc_codec *codec)
{
	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	wm8804_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	#ifdef AUDIO_POWER_CTL

	#else
		
    /* [downer] A138027 */    
    ak_set_snd_pwr_ctl(AK_AUDIO_POWER, AK_PWR_ON);    

    #if(CUR_DAC == CODEC_CS4398) 
    cs4398_reg_init();
    #elif (CUR_DAC == CODEC_ES9018)
    ES9018_reg_init();
    #endif   

    ak_set_hw_mute(AMT_DAC_MUTE ,0);
    ak_set_hw_mute(AMT_HP_MUTE ,0);
    #endif
	
	return 0;
}
#else
#define wm8804_suspend NULL
#define wm8804_resume NULL
#endif

static int wm8804_remove(struct snd_soc_codec *codec)
{
	struct wm8804_priv *wm8804;
	int i;

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	#ifdef REDUCE_POP_2th
	DBG_ERR("POP","power off mute\n");
	
    ak_set_hw_mute(AMT_HP_MUTE ,1);
	#endif

	wm8804 = snd_soc_codec_get_drvdata(codec);

	wm8804_set_bias_level(codec, SND_SOC_BIAS_OFF);
	#if 0
	for (i = 0; i < ARRAY_SIZE(wm8804->supplies); ++i)
		regulator_unregister_notifier(wm8804->supplies[i].consumer,
					      &wm8804->disable_nb[i]);
	regulator_bulk_free(ARRAY_SIZE(wm8804->supplies), wm8804->supplies);
	#endif
	
	return 0;
}
static char *intToBinary(int width,int i) 
{
  static char s1[64 + 1] = { '0', };
  int count = width;

  memset(s1,0,sizeof(s1));

  do { s1[--count] = '0' + (char) (i & 1);
       i = i >> 1;
  } while (count);

  return s1;
}	
#include <linux/gpio.h>

static int wm8804_read(struct snd_soc_codec *codec,unsigned char reg)
{
	int ret = 0;
	uint8_t buf[2];
	struct i2c_client *i2c =  codec->control_data;
	
	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

//	DBG_PRINT("WM8804","reg : %d\n",reg);
	buf[0] = reg;
	ret = i2c_master_send(i2c, buf, 1);
	if (ret < 0)  {
		printk(KERN_ERR "melfas_ts_work_func: i2c_master_send() failed\n");
		return -1;
	}

	i2c->flags = I2C_M_RD;
	ret = i2c_master_recv(i2c, buf, 1);
	if (ret < 0) {
		printk(KERN_ERR "melfas_ts_work_func: i2c_master_recv() failed\n");
		return -1;
	} 

	return buf[0];
}

static int wm8804_probe(struct snd_soc_codec *codec)
{
	struct wm8804_priv *wm8804;
	int i, id1, id2, ret;

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 1;
	#endif

	DBG_PRINT("WM8804","\n");
	wm8804 = snd_soc_codec_get_drvdata(codec);
	wm8804->codec = codec;

	wm8804_codec = codec;
	codec->dapm.idle_bias_off = 1;

	ret = snd_soc_codec_set_cache_io(codec, 8, 8, wm8804->control_type);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set cache i/o: %d\n", ret);
		return ret;
	}

	codec->hw_read = wm8804_read;	
	codec->hw_write = (hw_write_t)i2c_master_send;
        #if 0
	for (i = 0; i < ARRAY_SIZE(wm8804->supplies); i++)
		wm8804->supplies[i].supply = wm8804_supply_names[i];

	ret = regulator_bulk_get(codec->dev, ARRAY_SIZE(wm8804->supplies),
				 wm8804->supplies);
	if (ret) {
		dev_err(codec->dev, "Failed to request supplies: %d\n", ret);
		return ret;
	}

	wm8804->disable_nb[0].notifier_call = wm8804_regulator_event_0;
	wm8804->disable_nb[1].notifier_call = wm8804_regulator_event_1;

	/* This should really be moved into the regulator core */
	for (i = 0; i < ARRAY_SIZE(wm8804->supplies); i++) {
		ret = regulator_register_notifier(wm8804->supplies[i].consumer,
						  &wm8804->disable_nb[i]);
		if (ret != 0) {
			dev_err(codec->dev,
				"Failed to register regulator notifier: %d\n",
				ret);
		}
	}

	ret = regulator_bulk_enable(ARRAY_SIZE(wm8804->supplies),
				    wm8804->supplies);
	if (ret) {
		dev_err(codec->dev, "Failed to enable supplies: %d\n", ret);
		goto err_reg_get;
	}
	#endif
	

	id1 = snd_soc_read(codec, WM8804_RST_DEVID1);
	if (id1 < 0) {
		dev_err(codec->dev, "Failed to read device ID: %d\n", id1);
		ret = id1;
		goto err_reg_enable;
	}
	id2 = snd_soc_read(codec, WM8804_DEVID2);
	if (id2 < 0) {
		dev_err(codec->dev, "Failed to read device ID: %d\n", id2);
		ret = id2;
		goto err_reg_enable;
	}

	id2 = (id2 << 8) | id1;

	if (id2 != ((wm8804_reg[WM8804_DEVID2] << 8)
			| wm8804_reg[WM8804_RST_DEVID1])) {
		dev_err(codec->dev, "Invalid device ID: %#x\n", id2);
		//ret = -EINVAL;
		//goto err_reg_enable;
	}

	ret = snd_soc_read(codec, WM8804_DEVREV);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to read device revision: %d\n",
			ret);
		goto err_reg_enable;
	}
	dev_info(codec->dev, "revision %c\n", ret + 'A');

	ret = wm8804_reset(codec);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to issue reset: %d\n", ret);
		goto err_reg_enable;
	}

	wm8804_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	#ifdef TEST_SPDIF_RX
	ak_set_wm8804_spdif_mode(SND_WM8804_SPDIF_BYPASS);			
	msleep(1000);
	#endif

	#ifdef TEST_SPDIF_TX
	ak_set_wm8804_spdif_mode(SND_WM8804_SPDIF_TX);			
	msleep(1000);
	#endif
	
	ak_set_hw_mute(AMT_DAC_MUTE,1);
	ak_set_hw_mute(AMT_HP_MUTE,1);

	#if 0
	snd_soc_add_controls(codec, wm8804_snd_controls,
			     ARRAY_SIZE(wm8804_snd_controls));
	#endif
	
	return 0;

err_reg_enable:
//	regulator_bulk_disable(ARRAY_SIZE(wm8804->supplies), wm8804->supplies);
;
err_reg_get:
	//regulator_bulk_free(ARRAY_SIZE(wm8804->supplies), wm8804->supplies);
	;
	return ret;
}

#ifdef REDUCE_POP_1th
static int wm8804_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	if(ak_get_wm8804_spdif_mode()==SND_WM8804_SPDIF_BYPASS) return 0;
		
	if (mute) {
		#if(CUR_DAC == CODEC_CS4398)
		cs4398_mute(1);
		#endif
		ak_set_hw_mute(AMT_DAC_MUTE,1);
		ak_set_hw_mute(AMT_HP_MUTE,1);
		//mdelay(10);
	}
	else {
		mdelay(10);
		ak_set_hw_mute(AMT_DAC_MUTE,0);
		ak_set_hw_mute(AMT_HP_MUTE,0);
		#if(CUR_DAC == CODEC_CS4398)
		cs4398_mute(0);
		#endif
	}
	return 0;
}
#endif

static struct snd_soc_dai_ops wm8804_dai_ops = {
	.hw_params = wm8804_hw_params,
	.set_fmt = wm8804_set_fmt,
	.set_sysclk = wm8804_set_sysclk,
	.set_clkdiv = wm8804_set_clkdiv,
	.set_pll = wm8804_set_pll,
	#ifndef REDUCE_POP_3th
	.digital_mute = wm8804_mute
	#endif
};

#if 0
#define WM8804_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
			SNDRV_PCM_FMTBIT_S24_LE)
#endif

#define WM8804_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE |\
								SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_3LE)

#define WM8804_RATES (SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
		      SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_64000 | \
		      SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 | \
		      SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000)

static struct snd_soc_dai_driver wm8804_dai = {
	.name = "wm8804-spdif",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = WM8804_RATES,
		.formats = WM8804_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = WM8804_RATES,
		.formats = WM8804_FORMATS,
	},
	.ops = &wm8804_dai_ops,
	.symmetric_rates = 1
};

static struct snd_soc_codec_driver soc_codec_dev_wm8804 = {
	.probe = wm8804_probe,
	.remove = wm8804_remove,
	.suspend = wm8804_suspend,
	.resume = wm8804_resume,
	.set_bias_level = wm8804_set_bias_level,
	.reg_cache_size = ARRAY_SIZE(wm8804_reg),
	.reg_word_size = sizeof(u8),
	.reg_cache_default = wm8804_reg,
	.volatile_register = wm8804_volatile
};

#if defined(CONFIG_SPI_MASTER)
static int __devinit wm8804_spi_probe(struct spi_device *spi)
{
	struct wm8804_priv *wm8804;
	int ret;

	wm8804 = kzalloc(sizeof *wm8804, GFP_KERNEL);
	if (!wm8804)
		return -ENOMEM;

	wm8804->control_type = SND_SOC_SPI;
	spi_set_drvdata(spi, wm8804);

	ret = snd_soc_register_codec(&spi->dev,
				     &soc_codec_dev_wm8804, &wm8804_dai, 1);
	if (ret < 0)
		kfree(wm8804);
	return ret;
}

static int __devexit wm8804_spi_remove(struct spi_device *spi)
{
	snd_soc_unregister_codec(&spi->dev);
	kfree(spi_get_drvdata(spi));
	return 0;
}

static struct spi_driver wm8804_spi_driver = {
	.driver = {
		.name = "wm8804",
		.owner = THIS_MODULE,
	},
	.probe = wm8804_spi_probe,
	.remove = __devexit_p(wm8804_spi_remove)
};
#endif


/* 2013-5-21 jhlim*/

static struct i2c_client *g_codec_i2c;

int WM8804_WRITE(unsigned int reg, unsigned int val)
{
	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	if(!wm8804_codec) {
		DBG_PRINT("WM8804","ERROR \n");
		return -1;
	}
	return snd_soc_write(wm8804_codec,reg,val);
}


int WM8804_READ(unsigned char reg)
{
	#if(AUDIO_CLOCK == CLK_AK8157A)
	return 0;
	#endif

	if(!wm8804_codec) {
		DBG_PRINT("WM8804","ERROR \n");

		return -1;
	}
	return snd_soc_read(wm8804_codec, reg);
}



#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
static __devinit int wm8804_i2c_probe(struct i2c_client *i2c,
				      const struct i2c_device_id *id)
{
	struct wm8804_priv *wm8804;
	int ret;


	wm8804 = kzalloc(sizeof *wm8804, GFP_KERNEL);
	if (!wm8804)
		return -ENOMEM;

	wm8804->control_type = SND_SOC_I2C;
	i2c_set_clientdata(i2c, wm8804);

	ret = snd_soc_register_codec(&i2c->dev,
				     &soc_codec_dev_wm8804, &wm8804_dai, 1);
	if (ret < 0)
		kfree(wm8804);
	return ret;
}

static __devexit int wm8804_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	kfree(i2c_get_clientdata(client));
	return 0;
}

static const struct i2c_device_id wm8804_i2c_id[] = {
	{ "wm8804", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, wm8804_i2c_id);

static struct i2c_driver wm8804_i2c_driver = {
	.driver = {
		.name = "wm8804",
		.owner = THIS_MODULE,
	},
	.probe = wm8804_i2c_probe,
	.remove = __devexit_p(wm8804_i2c_remove),
	.id_table = wm8804_i2c_id
};
#endif


int spdif_mode = 0;
int spdif_connected = 0;

void set_spdif_pll(int mode)
{
	#if(AUDIO_CLOCK == CLK_AK8157A)
	return;
	#endif

	if(wm8804_codec == NULL)
		return;

		if(mode==1)  //192K
		{
			DBG_PRINT("WM8804","\nMode 1\n");
			DBG_PRINT("WM8804","192khz setting\n");

			snd_soc_write(wm8804_codec, WM8804_PLL1, 0xBA);
			snd_soc_write(wm8804_codec, WM8804_PLL2, 0x49);
			snd_soc_write(wm8804_codec, WM8804_PLL3, 0x0C);
			snd_soc_write(wm8804_codec, WM8804_PLL4, 0x08);
			snd_soc_write(wm8804_codec, WM8804_PLL5, 0x0C);	
			
		}
		else
		{
			DBG_PRINT("WM8804","\nMode 2,3,4\n");

			snd_soc_write(wm8804_codec, WM8804_PLL1, 0x21);
			snd_soc_write(wm8804_codec, WM8804_PLL2, 0xFD);
			snd_soc_write(wm8804_codec, WM8804_PLL3, 0x36);
			snd_soc_write(wm8804_codec, WM8804_PLL4, 0x07);
			snd_soc_write(wm8804_codec, WM8804_PLL5, 0x34);	
		}

	
	snd_soc_write(wm8804_codec, WM8804_AIFRX,0x46); 
	snd_soc_write(wm8804_codec, WM8804_SPDRX1,0x8); 
	snd_soc_write(wm8804_codec, WM8804_PWRDN, 0x4); 

}

void wm8804_spdif_rx_pwr(int onoff)
{
	int value;	

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return;
	#endif

	if(!wm8804_codec) return;
	
	value = snd_soc_read(wm8804_codec,WM8804_PWRDN);

	if(onoff) value &= ~0b10;
	else value |= 0b10;


	DBG_PRINT("wm8804","%X\n",value);
		
	snd_soc_write(wm8804_codec, WM8804_PWRDN,value); 

}

void wm8804_spdif_tx_pwr(int onoff)
{
	int value;	

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return;
	#endif

	if(!wm8804_codec) return;

	value = snd_soc_read(wm8804_codec,WM8804_PWRDN);

	if(onoff) value &= ~0b100;
	else value |= 0b100;

//	DBG_PRINT("wm8804","%X\n",value);
	
	snd_soc_write(wm8804_codec, WM8804_PWRDN,value); 
}

void wm8804_mute2(int onoff)
{
	int value;	

	#if(AUDIO_CLOCK == CLK_AK8157A)
	return;
	#endif

	if(!wm8804_codec) return;

	value = snd_soc_read(wm8804_codec,0x1C);

	if(onoff) value &= ~0b01000000;
	else value |= 0b01000000;

	DBG_PRINT("wm8804","%X\n",value);
	
	snd_soc_write(wm8804_codec, 0x1C,value); 

}


void wm8804_check_received_spdif(void)
{
	int value, value1;
	#if(AUDIO_CLOCK == CLK_AK8157A)
	return;
	#endif

	if(wm8804_codec == NULL)
		return;

#if 0
	//Jack Detection check.
	//DETECT : GPIOF[2] : Low,  GPIOF[3] : High
	if(!(pStrGPIOBaseReg->GPFDAT & Hw2) && (pStrGPIOBaseReg->GPFDAT & Hw3))
	{
		printk("SPDIF Connected\n");
		spdif_connected = 1;
	}
	else
	{
		spdif_connected = 0;
		return;
	}

	if(!(pStrGPIOBaseReg->GPFDAT & Hw3) && !(pStrGPIOBaseReg->GPFDAT & Hw2))
	{
		printk("LINEIN Connected\n");
	}
#endif
	
	//printk("wm8804_check_received_spdif");
	value = 0;
	value = snd_soc_read(wm8804_codec, WM8804_INTSTAT);
	if (value < 0) {
		DBG_PRINT("WM8804", "Failed to read device ID: %d\n", value);
	}
//	DBG_PRINT("WM8804","INTSTAT : 0x%x\t\n", value);
#if 0
	if(spdif_mode == 1)
	{
		//if((value & 0x08) && (value & 0x01))
		if((value == 0x2f))
		{
			printk( "\nMode 1\n");
			printk( "176khz setting\n");

			snd_soc_write(wm8804_codec, WM8804_PLL1, 0x21);
			snd_soc_write(wm8804_codec, WM8804_PLL2, 0xFD);
			snd_soc_write(wm8804_codec, WM8804_PLL3, 0x36);
			snd_soc_write(wm8804_codec, WM8804_PLL4, 0x08);
			//snd_soc_write(wm8804_codec, WM8804_PLL5, 0x34);	
		}
	}
	else 
#endif

	if(value & 0x80)
	{
		value1 =0;
		value1 = snd_soc_read(wm8804_codec, WM8804_SPDSTAT);

		printk( "[WM8804]Receive FREQ changed : 0x%x %x\n", value,value1);

		if(value1 & 0x30)
		{
			spdif_mode = 234;
			DBG_PRINT("WM8804","\nMode 2,3,4\n");
			
			snd_soc_write(wm8804_codec, WM8804_PLL1, 0x21);
			snd_soc_write(wm8804_codec, WM8804_PLL2, 0xFD);
			snd_soc_write(wm8804_codec, WM8804_PLL3, 0x36);
			snd_soc_write(wm8804_codec, WM8804_PLL4, 0x07);
			snd_soc_write(wm8804_codec, WM8804_PLL5, 0x34);	
			msleep_interruptible(10);
			
			#if (CUR_DAC==CODEC_CS4398)
			cs4398_FM(96000);
			#endif
		}
		else //192K
		{
			DBG_PRINT("WM8804","\nMode 1\n");
			DBG_PRINT("WM8804","192khz setting\n");
			spdif_mode = 1;

			snd_soc_write(wm8804_codec, WM8804_PLL1, 0xBA);
			snd_soc_write(wm8804_codec, WM8804_PLL2, 0x49);
			snd_soc_write(wm8804_codec, WM8804_PLL3, 0x0C);
			snd_soc_write(wm8804_codec, WM8804_PLL4, 0x08);
			snd_soc_write(wm8804_codec, WM8804_PLL5, 0x0C);	

			msleep_interruptible(10);
			
			#if (CUR_DAC==CODEC_CS4398)
			cs4398_FM(192000);
			#endif

		}
	}

	return;

	value = 0;
	value = snd_soc_read(wm8804_codec, WM8804_RXCHAN4);	
}
EXPORT_SYMBOL(wm8804_check_received_spdif);

static int __init wm8804_modinit(void)
{
	int ret = 0;
	DBG_PRINT("WM8804","\n");


#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	ret = i2c_add_driver(&wm8804_i2c_driver);
	if (ret) {
		printk(KERN_ERR "Failed to register wm8804 I2C driver: %d\n",
		       ret);
	}
#endif
#if defined(CONFIG_SPI_MASTER)
	ret = spi_register_driver(&wm8804_spi_driver);
	if (ret != 0) {
		printk(KERN_ERR "Failed to register wm8804 SPI driver: %d\n",
		       ret);
	}
#endif
	return ret;
}
module_init(wm8804_modinit);

static void __exit wm8804_exit(void)
{
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	i2c_del_driver(&wm8804_i2c_driver);
#endif
#if defined(CONFIG_SPI_MASTER)
	spi_unregister_driver(&wm8804_spi_driver);
#endif
}
module_exit(wm8804_exit);

MODULE_DESCRIPTION("ASoC WM8804 driver");
MODULE_AUTHOR("Dimitris Papastamos <dp@opensource.wolfsonmicro.com>");
MODULE_LICENSE("GPL");
