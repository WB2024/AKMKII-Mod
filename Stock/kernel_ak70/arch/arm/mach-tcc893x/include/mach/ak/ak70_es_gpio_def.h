/*
 *   2016-01
 */
#ifndef __AK70_GPIO_DEF_H__
#define __AK70_GPIO_DEF_H__


//AK70 PORTMAP DEFINE.  // evm / ws /es


#if(CUR_AK_REV == AK70_HW_ES)


#define	ADP_CHG_ICTL				(0)	// FIX_ME_LATER
#define GPIO_OTG_5V5_EN 			(0) // FIX_ME_LATER
#define	GPIO_OTG_DET				(0) // FIX_ME_LATER


#define GPIO_CHG_CTL1		TCC_GPA(1) //OK
#define GPIO_CHG_CTL2			TCC_GPA(2) //OK


#define	GPIO_MICRO_SD_CLK		TCC_GPA(7) 
#define	GPIO_MICRO_SD_CMD		TCC_GPA(8) 
#define	GPIO_MICRO_SD_D0		TCC_GPA(9) 
#define	GPIO_MICRO_SD_D1		TCC_GPA(10) 
#define	GPIO_MICRO_SD_D2		TCC_GPA(11) 
#define	GPIO_MICRO_SD_D3		TCC_GPA(12) 



// [downer] A130909 for PMIC/BatteryI2C
#define GPIO_I2C_SDA2           TCC_GPA(15) //OK
#define GPIO_I2C_SCL2           TCC_GPA(16) //OK


//********************************************************
//					GPIO Group B
//********************************************************
#define	GPIO_PLL25_VCTL			TCC_GPB(29)	


 /*
refer to Board-tcc8930.h, Board-tcc8930-panel.c
.power_on	= GPIO_LCD_ON,				//TCC_GPE(30)
.bl_on		= GPIO_LCD_BL,				//TCC_GPB(29)
.reset		= GPIO_LCD_RESET,			//TCC_GPE(29)
*/	
	
//********************************************************
//					GPIO Group C
//********************************************************
//#define GPIO_TOUCH_RST				TCC_GPC(18)


//********************************************************
//					GPIO Group D
//********************************************************
//#define GPIO_PMIC_HOLD            	TCC_GPD(0) //OK
#define GPIO_PMIC_HOLD            	TCC_GPG(2) 
#define GPIO_DAC_RST           	TCC_GPD(1) //OK DSP_DAC_RST CS4398 DAC reset.
#define GPIO_OTG_EN			       	TCC_GPD(2) //OK  
#define GPIO_DAC_EN          		TCC_GPD(3) //OK  

#define GPIO_DEBUG_TX          		TCC_GPD(4) //OK  
#define GPIO_DEBUG_RX          		TCC_GPD(5) //OK  
#define GPIO_TOUCH_RST				TCC_GPD(6)
		
#define	GPIO_WL_REG_ON				TCC_GPD(7) //OK
#define	GPIO_WL_DEV_WAKE			TCC_GPD(8) //OK
#define GPIO_USB_PWR_EN				TCC_GPD(10)

// GPIO D11 ~ D20  emmc block


//********************************************************
//					GPIO Group E
//********************************************************

#define GPIO_HP_DET         	TCC_GPE(20) //OK     
#define GPIO_BAL_DET      		TCC_GPE(18) // WS
#define	GPIO_KEY_VOL_UP			TCC_GPE(17) //OK
#define	GPIO_KEY_VOL_DOWN		TCC_GPE(19) //OK
#define	GPIO_PMIC_INT			TCC_GPE(13) //OK PMU IC,ACT8804

#define	GPIO_TOUCH_INT			TCC_GPE(14)

#define	GPIO_LCD_BL_PWM			TCC_GPE(20) // DC/DC MAX1554ETA
#define GPIO_USB_VBUS			TCC_GPE(23)
#define	GPIO_BT_HOST_WAKE		TCC_GPE(24) //OK
#define	GPIO_WL_HOST_WAKE		TCC_GPE(26) //OK

#define	GPIO_CHG_DONE  			TCC_GPE(27) //OK   Charge IC, MAX8677A
#define	GPIO_CHG_DET			TCC_GPE(28) //OK   Charge IC, MAX8677A


//********************************************************
//					GPIO Group F
//********************************************************
#define	GPIO_BT_REG_ON			TCC_GPF(0) //OK  bluetooth IC, F1C81
#define GPIO_AUDIO_SW_MUTE      TCC_GPF(1)


#define	GPIO_BT_DEV_WAKE 			TCC_GPF(2) //OK 	BT_WAKEUP
#define	GPIO_WIFI_BT_PWR_EN		(0)   // FIX_ME_LATER	
#define GPIO_BAL_UNBAL_SEL      TCC_GPF(4)

#define	GPIO_OPTIC_RST 			TCC_GPF(5)


#define GPIO_LCD_BL_EN 			TCC_GPF(6) // FIX_ME_LATER

#define	GPIO_OTG_5V_EN			TCC_GPF(7)	
#define	GPIO_CHG_USB_MODE		(0)   // FIX_ME_LATER	

#define	GPIO_HP_BAL_MUTE		TCC_GPF(8)	

#define	GPIO_OPAMP_PWR_EN		TCC_GPF(9) //OK Voltage Regulator
#define	GPIO_HP_MUTE			TCC_GPF(10) // OK
#define	GPIO_5V5_EN 			TCC_GPF(11) //OK AUDIO_5V5_EN DC/DC, MAX8614B
#define	GPIO_ADP_CHG_ICTL		TCC_GPF(12)	//OK  Hihg: 1~1.5A, Low:500~100mA


#define	GPIO_I2C_SDA8		TCC_GPF(13)	//OK CPU_I2C_SDA DAC,WM8804
#define	GPIO_I2C_SCL8		TCC_GPF(14)	//OK CPU_I2C_SCL DAC,WM8804



#define	GPIO_CHG_ON				TCC_GPF(15)	//OK Charger IC,MAX8677A

//GPIO_F23 ~ GPIOF28 : wifi io.



//********************************************************
//					GPIO Group G
//********************************************************
#define GPIO_LCD_SCLK             	TCC_GPG(0) //OK
#define GPIO_LCD_SFRM             	TCC_GPG(1) //OK
#define GPIO_LCD_SDI             	TCC_GPG(2) //OK
#define GPIO_LCD_SDO             	TCC_GPG(3) //OK

#define GPIO_LCD_PWEN             	TCC_GPG(4) //OK FIX_ME_LATER
#define GPIO_LOW_BATT           	TCC_GPG(5)
#define GPIO_KEY_PLAY_STOP			TCC_GPG(10)	// OK
#define GPIO_KEY_BACK				TCC_GPG(13)	//  OK 	REV
#define GPIO_KEY_NEXT				TCC_GPG(11)	// OK	FF

#define	GPIO_SD30_DET 				TCC_GPG(12) // OK MICRO_SD_DET

#define	GPIO_PWR_ON_OFF				TCC_GPG(14) // OK

#define	GPIO_CHG_FLT				TCC_GPG(15) // 


#define	GPIO_I2C_SDA28				TCC_GPADC(2)
#define	GPIO_I2C_SCL28				TCC_GPADC(3)


#if defined(CONFIG_GPIO_PCF857X) 

//PCF8575 Remote16-BIT I2C AND SMBus I/O Expander
#define	EXT_GPIO_P00				NR_BUILTIN_GPIO+0
#define	EXT_GPIO_P01				NR_BUILTIN_GPIO+1
#define	EXT_GPIO_P02				NR_BUILTIN_GPIO+2
#define	EXT_GPIO_P03				NR_BUILTIN_GPIO+3
#define	EXT_GPIO_P04				NR_BUILTIN_GPIO+4
#define	EXT_GPIO_P05				NR_BUILTIN_GPIO+5
#define	EXT_GPIO_P06				NR_BUILTIN_GPIO+6
#define	EXT_GPIO_P07				NR_BUILTIN_GPIO+7
#define	EXT_GPIO_P10				NR_BUILTIN_GPIO+8
#define	EXT_GPIO_P11				NR_BUILTIN_GPIO+9

#define	EXT_GPIO_P12				NR_BUILTIN_GPIO+10
#define	EXT_GPIO_P13				NR_BUILTIN_GPIO+11
#define	EXT_GPIO_P14				NR_BUILTIN_GPIO+12
#define	EXT_GPIO_P15				NR_BUILTIN_GPIO+13
#define	EXT_GPIO_P16				NR_BUILTIN_GPIO+14
#define	EXT_GPIO_P17				NR_BUILTIN_GPIO+15

#endif


#endif

#endif	
