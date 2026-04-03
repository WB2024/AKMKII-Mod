/*
 * linux/arch/arm/mach-tccxxxx/include/mach/astell_kern.h
 *
 * Author:  <@iriver>
 * Created: May, 2013
 * Description: Definitions for configurations.
 *
 * Copyright (C) Iriver, Inc.
 *
 */
#ifndef __ASTELL_KERN__H__
#define __ASTELL_KERN__H__

#include "ak_debug.h"


#define MODEL_AK240  	(10)   // AK240_TP2
#define AK240_HW_TP2	(11)

#define MODEL_AK120_II  (20)    
#define AK120_II_HW_ES1	(21)
#define AK120_II_HW_ES2	(22)
#define AK120_II_HW_TP1	(23)

#define MODEL_AK100_II  (30)
#define AK100_II_HW_ES2	(31)
#define AK100_II_HW_TP1 (32)

#define MODEL_AK1000N  	(40)
#define AK1000N_HW_WS	(41)

#define MODEL_AK500N    (50)
#define AK500N_HW_WS1   (51)
#define AK500N_HW_WS2   (52)
#define AK500N_HW_ES1    (53)
#define AK500N_HW_TP    (54)

#define MODEL_AK380    (60)
#define AK380_HW_EVM   (61)
#define AK380_HW_WS   (62)
#define AK380_HW_ES    (63)
#define AK380_HW_TP    (64)
#define AK380_HW_MP    (65)

#define MODEL_AK320    (70)
#define AK320_HW_ES    (71)

#define MODEL_AK300    (80)
#define AK300_HW_ES    (81)
#define AK300_HW_TP    (82)

#define MODEL_AK70  (90)
#define AK70_HW_EVM	(91)
#define AK70_HW_WS	(92)
#define AK70_HW_ES	(93)
#define AK70_HW_TP	(94)
#define MKII	(98)
#define ORG	(99)

#define MODEL_AK380S   (100)
#define AK380S_HW_ES    (103)
#define AK380S_HW_TP    (104)
#define AK380S_HW_MP    (105)

#include "ak_target.h"

#if (CUR_AK >= MODEL_AK240)
#define USE_ADAPTER_DET
#endif

#define AK_HAVE_XMOS (0)	   


//Astell & Kern Function Enable/Disable Define.

#if(CUR_AK == MODEL_AK70)

#if( CUR_AK_REV == AK70_HW_EVM)
#define ENABLE_OPTICAL_TX
#endif

#if( CUR_AK_REV >= AK70_HW_WS)
#define BALANCED_OUT_FUNC (1)
#define MENU_BALANCED_OUT
#endif



#endif




#define AK_WM8804_MASTER

//#define DAC_VOL_CTL_IN_KERNEL

#define AK_GPIO_JACK_SWITCH /* 2013-7-11 */

//#define TEST_SPDIF_RX
//#define TEST_SPDIF_TX

#define CODEC_WM8741 (0)
#define CODEC_CS4398 (1)
#define CODEC_ES9018 (2)
#define CODEC_AK4490 (3)

#define CLK_WN8804 	(10)
#define CLK_AK8157A (11)

#if (CUR_AK == MODEL_AK240   || CUR_AK == MODEL_AK120_II || CUR_AK == MODEL_AK100_II || CUR_AK == MODEL_AK500N)
#define CUR_DAC CODEC_CS4398

#if (CUR_AK == MODEL_AK240   || CUR_AK == MODEL_AK120_II | CUR_AK == MODEL_AK500N)
#define CODEC_CS4398_DUAL
#endif

#elif (CUR_AK == MODEL_AK1000N)
#define CUR_DAC CODEC_ES9018

#elif (CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
#define CUR_DAC CODEC_AK4490

#elif (CUR_AK == MODEL_AK70)
	#if(CUR_AK_SUB == MKII) 			
		
		#if defined(CONFIG_DAC_AK4490) //AK70_MKII Moudle(ak70_defconfig)
		#define AK_AUDIO_MODULE_USED		
		#define AUDIO_CLOCK CLK_AK8157A
		#define CUR_DAC CODEC_AK4490
		#else													//AK70_MKII
		#define AUDIO_CLOCK CLK_AK8157A
		#define CUR_DAC CODEC_CS4398		
		#define CODEC_CS4398_DUAL
		#endif
		
	#else  // CUR_AK_SUB == ORG =>AK70
	#define AUDIO_CLOCK CLK_WN8804
	#define CUR_DAC CODEC_CS4398
	#endif
#else
#define CUR_DAC CODEC_CS4398
#endif

#if (CUR_DAC==CODEC_WM8741)
#define AK_USE_WM8741
#define CONFIG_IRIVER_AK100
#endif


//CS4398 setting .

#if( CUR_AK_REV == AK100_II_HW_ES2)
#define MAX_VOLUME_OFFSET (6) // max volume 72
//#define MAX_VOLUME_OFFSET (24) // max volume 72
#elif ( CUR_AK == MODEL_AK500N)
#define MAX_VOLUME_OFFSET (6)  
//#define MAX_VOLUME_OFFSET (29) //EU
#elif(CUR_AK == MODEL_AK70)
#if(CUR_AK_SUB == MKII)                                
#define MAX_VOLUME_OFFSET (3)  //AK70_MKII volume 150(MAX) 2.232 Vrms ==> 148 1.999 Vrms 
//#define MAX_VOLUME_OFFSET (0x3d)  //dean 20170720 음압 인증용 92 140.8 mV  140.9 mV
#else
#define MAX_VOLUME_OFFSET (1)  
#endif
#else
#define MAX_VOLUME_OFFSET (0)  
#endif

#define MAX_VOLUME_IDX (150)

//#define AK_FIXME_LATER

//#define PRINT_ALSA_MSG  /* 2013-10-25 */  
//#define PRINT_VOLUME_VALUE  /* 2013-11-1 */

#ifndef CONFIG_INTERNAL_DSD_DEMO 
#define AUDIO_POWER_CTL 
#endif

#define REDUCE_POP_1th   /* 2013-9-16 */
//#define REDUCE_POP_2th   /* 2013-10-25 */
#define REDUCE_POP_3th   /* 2013-11-10 */
//#define REDUCE_POP_4th   /*  2014-1-3  */

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
#define REDUCE_BALANCED_POPNOISE
#endif

#if (CUR_AK == MODEL_AK100_II)
//#define REDUCE_AUDIOSW_POPNOISE
#endif

//jimi.141111
#if (CUR_AK == MODEL_AK500N) 
//#define REDUCE_EXT_AMP_POPNOISE
#endif

//#define PCM_DSD_CALLBACK
//#define ENABLE_DEBUG_PORT   /* 2013-10-25 */

#if (CUR_AK >= MODEL_AK240) 
//#undef CONFIG_WAKEUP_ASSIST
//#undef CORE_VOLTAGE_CHANGE
#define CORE_VOLTAGE_CHANGE	/*2013.11.19 iriver-ysyim: refer to act8840.c: LCD on: 1.175v, LCD off: 1.05v*/
#else
#undef CONFIG_WAKEUP_ASSIST
#define CORE_VOLTAGE_CHANGE	/*2013.11.19 iriver-ysyim: refer to act8840.c: LCD on: 1.175v, LCD off: 1.05v*/
#endif

/*
 * [downer] A140218
 */
#define USE_LCD_OFF_WAKELOCK
//#define USE_FORCE_POWERKEY_PUSH

/*
 * [downer] A140425
 * for sleep current check AK100_II ES2 event only
 */
#if (CUR_AK_REV == AK100_II_HW_ES2)
//#define SLEEP_CURRENT_CHECK
#endif


#if(CUR_AK == MODEL_AK500N)
#define ENABLE_AK_RAID_SYSTEM
#endif

#if(CUR_AK == MODEL_AK70)
#define FIX_MUTE_NOISE
//#define _AK_REMOVE_ 

//#define SUPPORT_AK_DOCK 
//#define SUPPORT_CD_RIPPER 
//#define SUPPORT_EXT_AMP 
//#define SUPPORT_DSP_CS48L1X

//#define USE_VOLUME_WHEEL_RAMP
#ifdef USE_VOLUME_WHEEL_RAMP
//#define USE_VWR_TYPE1
#define USE_VWR_TYPE2
#endif

//#define NO_MEMCLK_CHG_PCFI  // PCFI 일경우에 MEM CLK 변경시 음 끊기는 문제 수정. 216.06.17 

#define ENABLE_DDR_600MHz	//iriver.ysyim 2016.04.25 : DDR600MHz 
#define SCREEN_HEIGHT_FORCE_800	//iriver.ysyim 2016.04.25 : LCD  org height is 854 , but only display 800, refer to Tcc_vioc_fb.c


//#define FORCE_UAC2_0_FORMAT  // jhlim 2016.08.04

#define REDUCE_GAP   // 곡 시작시 음 짤림 수정.

#endif

#endif	


