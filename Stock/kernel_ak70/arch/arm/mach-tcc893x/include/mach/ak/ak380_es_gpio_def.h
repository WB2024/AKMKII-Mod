/*
 *   2015-03-12
 */
#ifndef __AK380_ES_EVM_GPIO_DEF_H__
#define __AK380_ES_EVM_GPIO_DEF_H__

//AK380 PORTMAP DEFINE.
#define GPIO_HP_DET				TCC_GPA(2)
#define	GPIO_OPTIC_TX_DET		TCC_GPA(3)

#define	GPIO_KEY_VOL_UP			TCC_GPA(5) // TCC_GPB(24)
#define	GPIO_KEY_VOL_DOWN		TCC_GPA(6) //TCC_GPB(25)

#define GPIO_CPU_I2S_EN         TCC_GPA(8)
#define GPIO_M_TOUCH_MCLR        TCC_GPA(9)

#define	GPIO_DAC_RST			TCC_GPA(11)

// [downer] A130909 for PMIC/BatteryI2C
#define GPIO_I2C_SDA2           TCC_GPA(15)
#define GPIO_I2C_SCL2           TCC_GPA(16)

#define GPIO_LCD_SCLK			TCC_GPA(17)
#define GPIO_LCD_SFRM			TCC_GPA(18)
#define GPIO_LCD_SDI 			TCC_GPA(19)
#define GPIO_LCD_SDO 			TCC_GPA(20)

#define GPIO_XMOS_SPI_SEL       TCC_GPA(23)    
    
#define GPIO_I2C_SEL 			TCC_GPA(24)
#define GPIO_XMOS_PWR_EN		TCC_GPA(25)
#define GPIO_XMOS_TXD 			TCC_GPA(26)
#define GPIO_XMOS_RXD	 		TCC_GPA(27)
#define GPIO_XMOS_SCLK 			TCC_GPA(28)
#define GPIO_XMOS_SS 			TCC_GPA(29)
#define GPIO_XMOS_SDI 			TCC_GPA(30)
#define GPIO_XMOS_SDO 			TCC_GPA(31)
#define GPIO_XMOS_RST_N			TCC_GPG(0)	// defined but not used. use XMOS_PWR_EN instead.

#define	GPIO_BT_HOST_WAKE		TCC_GPG(17) 
#define	GPIO_BT_DEV_WAKE		TCC_GPB(1)
#define	GPIO_BT_REG_ON			TCC_GPB(2)
#define	GPIO_CHG_ADP_MODE		TCC_GPB(3)	//Added in AK380
#define GPIO_PLL25_VCTL			TCC_GPB(4)
#define	GPIO_CHG_USB_MODE		TCC_GPB(5)	//Added in AK380
#define GPIO_AUDIO_SW_ON        TCC_GPB(6)
#define GPIO_BAL_UNBAL_SEL      TCC_GPB(7)
#define GPIO_AUDIO_SW_MUTE      TCC_GPB(8)

// [downer] M130909 for CS4389/WM8804/CS2000 I2C
#define	GPIO_I2C_SDA8           TCC_GPB(9)
#define	GPIO_I2C_SCL8           TCC_GPB(10)

#define	GPIO_TOUCH_INT			TCC_GPB(11)
#define	GPIO_SD30_DET			TCC_GPB(12)
#define	GPIO_PWR_ON_OFF			TCC_GPB(13)
    
#define	GPIO_WL_HOST_WAKE		TCC_GPB(15)
#define	GPIO_WL_REG_ON			TCC_GPB(16)
#define	GPIO_WL_DEV_WAKE		TCC_GPB(18)
#define	GPIO_WIFI_CMD			TCC_GPB(19)
#define	GPIO_WIFI_D0			TCC_GPB(20)
#define	GPIO_WIFI_D1			TCC_GPB(21)
#define	GPIO_WIFI_D2			TCC_GPB(22)
#define	GPIO_WIFI_D3			TCC_GPB(23)
#define	GPIO_WIFI_CLK			TCC_GPB(28)	

 /*
refer to Board-tcc8930.h, Board-tcc8930-panel.c
.power_on	= GPIO_LCD_ON,				//TCC_GPE(30)
.bl_on		= GPIO_LCD_BL,				//TCC_GPB(29)
.reset		= GPIO_LCD_RESET,			//TCC_GPE(29)
*/	
	
//********************************************************
//					GPIO Group C
//********************************************************
#define GPIO_OTG_DET            TCC_GPC(0) //USB_ID - Added in AK380 
#define GPIO_M_TOUCH_DAT    TCC_GPC(1) 
#define GPIO_HOME_KEY    			TCC_GPC(4) //HOME_KEY
#define GPIO_M_TOUCH_CLK			TCC_GPC(5) 
#define GPIO_METAL_PWR_EN		TCC_GPC(6) 
#define GPIO_AMP_DET					TCC_GPC(7) 
#define GPIO_AMP_ADP_DET			TCC_GPC(8) 
#define GPIO_AMP_ACC_EN			TCC_GPC(9) 
#define GPIO_AMP_SW_MUTE_COM		TCC_GPC(10) 
#define GPIO_AMP_BAL_SEL_COM			TCC_GPC(11) 
#define GPIO_DOCK_I2C_EN						TCC_GPC(18) 

// SPI pins to control Audio DSP
#define GPIO_DSP_SCK			TCC_GPC(28)
#define GPIO_DSP_CS				TCC_GPC(29)
#define GPIO_DSP_MISO			TCC_GPC(30)
#define GPIO_DSP_MOSI			TCC_GPC(31)

#define GPIO_DSP_INT_HS1		TCC_GPC(14)
#define GPIO_DSP_RESET			TCC_GPC(15)
#define GPIO_DSP_BUSY_HS0		TCC_GPC(16)
#define GPIO_DSP_POWER			TCC_GPC(17)

//********************************************************
//					GPIO Group F
//********************************************************
//#define	GPIO_TOUCH_RST			TCC_GPF(0)
#define GPIO_OTG_5V5_EN         TCC_GPF(0)        
#define	GPIO_TSP_PWEN 			TCC_GPF(1)	

#define	GPIO_DAC_DZFL			TCC_GPF(2)	//Added in AK380
#define	GPIO_AUDIO_N5V_EN			TCC_GPF(3)

#define	GPIO_OPAMP_4V_EN		TCC_GPF(4)	

#define	GPIO_OPTIC_RST 			TCC_GPF(5)
#define	GPIO_PLAY_EN			TCC_GPF(6)
#define	GPIO_OPTIC_TX_EN		TCC_GPF(7)
#define	GPIO_OPTIC_PLAY_EN		TCC_GPF(8)
#define	GPIO_OPAMP_PWR_EN		TCC_GPF(9)
#define	GPIO_HP_MUTE			TCC_GPF(10)
#define	GPIO_5V5_EN				TCC_GPF(11)
#define	GPIO_N2V8_EN			TCC_GPF(12)  // used in AK380 ES(not used in EVM)

#define	GPIO_OP_TX_PWEN			TCC_GPF(13)
    
#define	ADP_CHG_ICTL			TCC_GPF(14)	// Hihg: 1~1.5A, Low:500~100mA

#define	GPIO_DAC_EN				TCC_GPF(15)
#define GPIO_HP_BAL_MUTE		TCC_GPF(16)	//Changed in AK380

#define	GPIO_BT_TXD				TCC_GPF(17)
#define	GPIO_BT_RXD				TCC_GPF(18)
#define	GPIO_BT_RTS				TCC_GPF(19)
#define	GPIO_BT_CTS				TCC_GPF(20)

//#define	GPIO_XMOS_USB_EN		TCC_GPF(21)	//AK380 ES NC  (keep it to avoid build error)
#define GPIO_USB_PWR_EN         TCC_GPF(21)    
#define	GPIO_USB_DAC_SEL		TCC_GPF(22)	
#define GPIO_CPU_XMOS_EN		GPIO_USB_DAC_SEL //rename

#define	GPIO_PMIC_HOLD			TCC_GPF(23)

#define GPIO_USB20_HOST_EN      TCC_GPF(26)

//NOTE : TOUCH	
#define	GPIO_I2C_SDA21			TCC_GPF(27)
#define	GPIO_I2C_SCL21			TCC_GPF(28)

#define	GPIO_DAC_DZFR			TCC_GPF(29)	//Added in AK380

    
#define GPIO_DSD_ON             TCC_GPG(1)

#define GPIO_XMOS_CPU3             TCC_GPG(2)
#define GPIO_LOW_BATT           TCC_GPG(5)
#define GPIO_KEY_PLAY_STOP		TCC_GPG(10)	//changed in AK380
#define GPIO_KEY_BACK		TCC_GPG(11)	//changed in AK380

#define GPIO_USB_ON             TCC_GPG(12)  // AK380 ES NC  (keep it to avoid build error)
#define GPIO_KEY_NEXT		TCC_GPG(13)	//changed in AK380
//#define GPIO_SD30_VCTL	        TCC_GPG(14)	//Changed in AK380

#define GPIO_CH_FLT				TCC_GPG(15)
#define GPIO_BAL_DET			TCC_GPG(16)	//Changd in AK380
#define GPIO_BT_HOST_WAKE		TCC_GPG(17)

#define	GPIO_CHG_DONE  			TCC_GPG(18) 
#define	GPIO_CHG_DET			TCC_GPG(19)

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
