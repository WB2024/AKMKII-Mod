/*
 *   2013-5-20
 */
#ifndef __AK_GPIO_DEF_H__
#define __AK_GPIO_DEF_H__

#if (CUR_AK_REV == AK380_HW_EVM)
#include "ak/ak380_gpio_def.h"
#elif (CUR_AK_REV == AK380_HW_ES)
#include "ak/ak380_es_gpio_def.h"
#elif (CUR_AK_REV == AK380_HW_TP)
#include "ak/ak380_tp_gpio_def.h"
#elif (CUR_AK_REV == AK380_HW_MP)
#include "ak/ak380_mp_gpio_def.h"
#elif (CUR_AK_REV == AK320_HW_ES)
#include "ak/ak320_es_gpio_def.h"
#elif (CUR_AK_REV == AK300_HW_ES)
#include "ak/ak300_es_gpio_def.h"
#elif (CUR_AK_REV == AK300_HW_TP)
#include "ak/ak300_tp_gpio_def.h"


#endif

#if (CUR_AK == MODEL_AK70)

#if (CUR_AK_REV == AK70_HW_ES) && (CUR_AK_SUB == MKII)
#include "ak/ak70_mkii_es_gpio_def.h"
#elif (CUR_AK_REV == AK70_HW_TP) && (CUR_AK_SUB == MKII)
#include "ak/ak70_mkii_tp_gpio_def.h"
#elif (CUR_AK_REV == AK70_HW_EVM)
#include "ak/akjr2_gpio_def.h"
#elif (CUR_AK_REV == AK70_HW_WS)
#include "ak/akjr2_ws_gpio_def.h"
#elif (CUR_AK_REV == AK70_HW_ES)
#include "ak/ak70_es_gpio_def.h"
#elif (CUR_AK_REV == AK70_HW_TP)
#include "ak/ak70_tp_gpio_def.h"
#endif
#endif

#if (CUR_AK == MODEL_AK240)
//AK240 PORTMAP DEFINE.
#define GPIO_HP_DET				TCC_GPA(2)
#define	GPIO_OPTIC_TX_DET		TCC_GPA(3)

#define	GPIO_KEY_VOL_UP			TCC_GPA(5) // TCC_GPB(24)
#define	GPIO_KEY_VOL_DOWN		TCC_GPA(6) //TCC_GPB(25)

#define GPIO_CPU_I2S_EN         TCC_GPA(8)
#define GPIO_XMOS_I2S_EN        TCC_GPA(9)

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
#define GPIO_PLL25_VCTL			TCC_GPB(4)

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

#define	GPIO_KEY_NEXT		    TCC_GPB(26)
#define	GPIO_KEY_PLAY_STOP  	TCC_GPB(27)

#define	GPIO_WIFI_CLK			TCC_GPB(28)	
#define GPIO_KEY_BACK 			TCC_GPB(29)

#define GPIO_USB_PWR_EN         TCC_GPB(31)

#define	GPIO_TOUCH_RST			TCC_GPF(0)
#define	GPIO_TSP_PWEN 			TCC_GPF(1)	

#define	GPIO_XMOS_PLAY_EN		TCC_GPF(2)	
#define	GPIO_AUDIO_N5V_EN		TCC_GPF(3)	
#define	GPIO_OPAMP_4V_EN		TCC_GPF(4)	

#define	GPIO_OPTIC_RST 			TCC_GPF(5)
#define	GPIO_PLAY_EN			TCC_GPF(6)
#define	GPIO_OPTIC_TX_EN		TCC_GPF(7)
#define	GPIO_OPTIC_PLAY_EN		TCC_GPF(8)
#define	GPIO_OPAMP_PWR_EN		TCC_GPF(9)
#define	GPIO_HP_MUTE			TCC_GPF(10)
#define	GPIO_5V5_EN				TCC_GPF(11)
#define	GPIO_N2V8_EN			TCC_GPF(12)
#define	GPIO_OP_TX_PWEN			TCC_GPF(13)
    
#define	GPIO_CHG_CTL			TCC_GPF(14)

#define	GPIO_DAC_EN				TCC_GPF(15)
#define	GPIO_OTG_5V5_EN			TCC_GPF(16)

#define	GPIO_BT_TXD				TCC_GPF(17)
#define	GPIO_BT_RXD				TCC_GPF(18)
#define	GPIO_BT_RTS				TCC_GPF(19)
#define	GPIO_BT_CTS				TCC_GPF(20)

#define	GPIO_XMOS_USB_EN		TCC_GPF(21)	    
#define	GPIO_USB_DAC_SEL		TCC_GPF(22)	
#define GPIO_CPU_XMOS_EN		GPIO_USB_DAC_SEL //rename

#define	GPIO_PMIC_HOLD			TCC_GPF(23)

#define GPIO_USB20_HOST_EN      TCC_GPF(26)

//NOTE : TOUCH	
#define	GPIO_I2C_SDA21			TCC_GPF(27)
#define	GPIO_I2C_SCL21			TCC_GPF(28)

// for USB3.0 detect test
#define GPIO_OTG_DET            TCC_GPG(13)
    
#define GPIO_DSD_ON             TCC_GPG(1)
#define GPIO_LOW_BATT           TCC_GPG(5)

#define GPIO_XMOS_CPU3             TCC_GPG(2)

#define GPIO_USB_ON             TCC_GPG(12)
#define GPIO_ADAPTER_DET        TCC_GPG(14)

#define GPIO_CH_FLT				TCC_GPG(15)
#define GPIO_OTG_FAULT			TCC_GPG(16)
#define GPIO_BT_HOST_WAKE		TCC_GPG(17)

#define	GPIO_CHG_DONE  			TCC_GPG(18) 
#define	GPIO_CHG_DET			TCC_GPG(19)

////////////////////////////////////////////////////////////////////////////
#elif (CUR_AK == MODEL_AK100_II || CUR_AK == MODEL_AK120_II)
#define GPIO_OPTIC_RX_DET			0

#define GPIO_HP_DET				TCC_GPA(2)
#define	GPIO_OPTIC_TX_DET		TCC_GPA(3)

#define	GPIO_KEY_VOL_UP			TCC_GPA(5) // TCC_GPB(24)
#define	GPIO_KEY_VOL_DOWN		TCC_GPA(6) //TCC_GPB(25)

#define	GPIO_DAC_RST			TCC_GPA(11)

// [downer] A130909 for PMIC/BatteryI2C
#define GPIO_I2C_SDA2           TCC_GPA(15)
#define GPIO_I2C_SCL2           TCC_GPA(16)

#define GPIO_LCD_SCLK			TCC_GPA(17)
#define GPIO_LCD_SFRM			TCC_GPA(18)
#define GPIO_LCD_SDI 			TCC_GPA(19)
#define GPIO_LCD_SDO 			TCC_GPA(20)

#define	GPIO_BT_HOST_WAKE		TCC_GPG(17) 
#define	GPIO_BT_DEV_WAKE		TCC_GPB(1)
#define	GPIO_BT_REG_ON			TCC_GPB(2)
#define GPIO_PLL25_VCTL			TCC_GPB(4)

#define GPIO_AUDIO_SW_ON        TCC_GPB(6)
#define GPIO_BAL_UNBAL_SEL      TCC_GPB(7)

#define GPIO_AUDIO_SW_MUTE      TCC_GPB(8)
//#define GPIO_AUDIO_SW_MUTE      TCC_GPD(0)  // for test.

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

#if (CUR_AK == MODEL_AK100_II || CUR_AK_REV == AK120_II_HW_ES2 || CUR_AK_REV == AK120_II_HW_TP1)
#define	GPIO_KEY_BACK		    TCC_GPG(11)
#define	GPIO_KEY_PLAY_STOP      TCC_GPG(10)
#define GPIO_KEY_NEXT 			TCC_GPG(13)
#else
#define	GPIO_KEY_NEXT		    TCC_GPB(26)
#define	GPIO_KEY_PLAY_STOP      TCC_GPB(27)
#define GPIO_KEY_BACK 			TCC_GPB(29)
#endif

#define	GPIO_WIFI_CLK			TCC_GPB(28)	


#define GPIO_USB_PWR_EN         TCC_GPB(31)

#define	GPIO_TOUCH_RST			TCC_GPF(0)
#define	GPIO_TSP_PWEN 			TCC_GPF(1)	
#if (CUR_AK == MODEL_AK100_II || CUR_AK_REV == AK120_II_HW_ES1 || CUR_AK_REV == AK120_II_HW_ES2 || CUR_AK_REV == AK120_II_HW_TP1)
#define	GPIO_BAL_MUTE 			TCC_GPF(2)	
#endif

#define	GPIO_AUDIO_N5V_EN		TCC_GPF(3)	
#define	GPIO_OPAMP_4V_EN		TCC_GPF(4)	

#define	GPIO_OPTIC_RST 			TCC_GPF(5)
#define	GPIO_PLAY_EN			TCC_GPF(6)
#define	GPIO_OPTIC_TX_EN		TCC_GPF(7)
#define	GPIO_OPTIC_PLAY_EN		TCC_GPF(8)
#define	GPIO_OPAMP_PWR_EN		TCC_GPF(9)
#define	GPIO_HP_MUTE			TCC_GPF(10)
#define	GPIO_5V5_EN				TCC_GPF(11)
#define	GPIO_N2V8_EN			TCC_GPF(12)
#define	GPIO_OP_TX_PWEN			TCC_GPF(13)
    
#define	GPIO_CHG_CTL			TCC_GPF(14)
#define	GPIO_OP_RX_EN			0

#define	GPIO_DAC_EN				TCC_GPF(15)
#define	GPIO_OTG_5V5_EN			TCC_GPF(16)

#define	GPIO_BT_TXD				TCC_GPF(17)
#define	GPIO_BT_RXD				TCC_GPF(18)
#define	GPIO_BT_RTS				TCC_GPF(19)
#define	GPIO_BT_CTS				TCC_GPF(20)

#define	GPIO_PMIC_HOLD			TCC_GPF(23)

//NOTE : TOUCH	
#define	GPIO_I2C_SDA21			TCC_GPF(27)
#define	GPIO_I2C_SCL21			TCC_GPF(28)

#define GPIO_LOW_BATT           TCC_GPG(5)

#define GPIO_CHG_SEL            TCC_GPG(12)
#define GPIO_ADAPTER_DET        TCC_GPG(14)

#define GPIO_CH_FLT				TCC_GPG(15)
#define GPIO_BT_HOST_WAKE		TCC_GPG(17)
    
#define	GPIO_CHG_DONE  			TCC_GPG(18) 
#define	GPIO_CHG_DET			TCC_GPG(19)


//
//   AK1000 GPIO define
//
#elif (CUR_AK == MODEL_AK1000N)
#define GPIO_OPTIC_RX_DET		TCC_GPA(0)
//#define GPIO_OPTIC_RX_DET2		TCC_GPA(1)

#define GPIO_BAL_MUTE	TCC_GPA(1)
#define VAR_AMP_MUTE	TCC_GPA(4)
#define VARMP_PWR_EN	TCC_GPA(7)
#define VAR_OPAMP_EN	TCC_GPA(10)
#define VAR_OPAMP_RST	TCC_GPA(12)
#define VAR_READY_PWR_EN	TCC_GPD(4)

#define GPIO_HP_DET			TCC_GPA(2)
#define	GPIO_OPTIC_TX_DET		TCC_GPA(3)

#define	GPIO_KEY_VOL_UP			TCC_GPA(5) // TCC_GPB(24)
#define	GPIO_KEY_VOL_DOWN		TCC_GPA(6) //TCC_GPB(25)


#define GPIO_CPU_I2S_EN         TCC_GPA(8)
#define GPIO_XMOS_I2S_EN        TCC_GPA(9)

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
#define GPIO_PLL25_VCTL			TCC_GPB(4)
//#if (CUR_AK_REV >= AK240_HW_TP)  /* [downer] A131125 */
#define GPIO_AUDIO_SW_ON        TCC_GPB(6)
#define GPIO_BAL_UNBAL_SEL      TCC_GPB(7)
#define GPIO_AUDIO_SW_MUTE      TCC_GPB(8)
//#endif



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

#define	GPIO_KEY_NEXT		    TCC_GPB(26)
#define	GPIO_KEY_PLAY_STOP  	TCC_GPB(27)

#define	GPIO_WIFI_CLK			TCC_GPB(28)	
#define GPIO_KEY_BACK 			TCC_GPB(29)

#define GPIO_OPTIC_RX_CS8416_RST	TCC_GPB(31)			//#define GPIO_USB_PWR_EN         TCC_GPB(31)

#define	GPIO_TOUCH_RST			TCC_GPF(0)
#define	GPIO_TSP_PWEN 			TCC_GPF(1)	
#define	GPIO_XMOS_PLAY_EN		TCC_GPF(2)	
#define	GPIO_AUDIO_N5V_EN		TCC_GPF(3)	
#define	GPIO_OPAMP_4V_EN		TCC_GPF(4)	


#define	GPIO_OPTIC_RST 			TCC_GPF(5)
#define	GPIO_PLAY_EN			TCC_GPF(6)
#define	GPIO_OPTIC_TX_EN		TCC_GPF(7)
#define	GPIO_OPTIC_PLAY_EN		TCC_GPF(8)
#define	GPIO_OPAMP_PWR_EN		TCC_GPF(9)
#define	GPIO_HP_MUTE			TCC_GPF(10)
#define	GPIO_5V5_EN				TCC_GPF(11)
#define	GPIO_N2V8_EN			TCC_GPF(12)
#define	GPIO_OP_TX_PWEN			TCC_GPF(13)
    
#define	GPIO_OP_RX_EN			TCC_GPF(14)		
//#define	GPIO_CHG_CTL			TCC_GPF(14)


#define	GPIO_DAC_EN				TCC_GPF(15)
#define	GPIO_OTG_5V5_EN			TCC_GPF(16)

#define	GPIO_BT_TXD				TCC_GPF(17)
#define	GPIO_BT_RXD				TCC_GPF(18)
#define	GPIO_BT_RTS				TCC_GPF(19)
#define	GPIO_BT_CTS				TCC_GPF(20)


#define	GPIO_XMOS_USB_EN		TCC_GPF(21)	    
#define	GPIO_USB_DAC_SEL		TCC_GPF(22)	
#define GPIO_CPU_XMOS_EN		GPIO_USB_DAC_SEL //rename

#define	GPIO_PMIC_HOLD			TCC_GPF(23)

#define	GPIO_TXD_RESET			TCC_GPF(24)		//cs8406


#define GPIO_USB20_HOST_EN      TCC_GPF(26)


//NOTE : TOUCH	
#define	GPIO_I2C_SDA21			TCC_GPF(27)
#define	GPIO_I2C_SCL21			TCC_GPF(28)


#if 0 // WM8740
#define GPIO_MC_SCL				TCC_GPG(11)
#define GPIO_ML_CSB				TCC_GPG(12)
#define GPIO_MD_SDA				TCC_GPG(14)
#endif

// for USB3.0 detect test
#define GPIO_OTG_DET            TCC_GPG(13)

    

#define GPIO_DSD_ON             TCC_GPG(1)
#define GPIO_LOW_BATT           TCC_GPG(5)
#define GPIO_DSD_SEL            TCC_GPG(12)
#define GPIO_ADAPTER_DET        TCC_GPG(14)


#define GPIO_CH_FLT				TCC_GPG(15)
#define GPIO_OTG_FAULT			TCC_GPG(16)
#define GPIO_BT_HOST_WAKE		TCC_GPG(17)

#define	GPIO_CHG_DONE  			TCC_GPG(18) 
#define	GPIO_CHG_DET			0//TCC_GPG(19)

/////////////////////////////////////////////////////////////////////////////////    
#elif (CUR_AK == MODEL_AK500N)
#if (CUR_AK_REV == AK500N_HW_WS1)
#define GPIO_HP_DET				TCC_GPA(2)
#define	GPIO_OPTIC_TX_DET		TCC_GPA(3)
#define VAR_AMP_MUTE	TCC_GPA(4)
#define	GPIO_KEY_VOL_UP			TCC_GPA(5) // TCC_GPB(24)
#define	GPIO_KEY_VOL_DOWN		TCC_GPA(6) //TCC_GPB(25)
#define GPIO_VARAMP_PWR_EN	TCC_GPA(7)
#define GPIO_CPU_I2S_EN         TCC_GPA(8)
#define GPIO_XMOS_I2S_EN        TCC_GPA(9)
#define VAR_OPAMP_EN	TCC_GPA(10)
#define	GPIO_DAC_RST			TCC_GPA(11)
#define VAR_OPAMP_RST	TCC_GPA(12)
#define GPIO_WHEEL_LED     TCC_GPA(13)
#define GPIO_WHEEL_LED_PWM		GPIO_WHEEL_LED // to fix build error

// [downer] A130909 for PMIC/BatteryI2C
#define GPIO_I2C_SDA2           TCC_GPA(15)
#define GPIO_I2C_SCL2           TCC_GPA(16)

//#define GPIO_LCD_SCLK			TCC_GPA(17)  //NC
//#define GPIO_LCD_SFRM			TCC_GPA(18)  //NC
//#define GPIO_LCD_SDI 			TCC_GPA(19)  //NC
//#define GPIO_LCD_SDO 			TCC_GPA(20)

#define GPIO_I2C_SDA3           TCC_GPA(20)  //MIPI_SDA
#define GPIO_I2C_SCL3           TCC_GPA(21)  //MIPI_SCL
#define TC358768XBG_RESX 		TCC_GPA(22)  //MIPI_RESET

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
#define GPIO_PLL25_VCTL			TCC_GPB(4)

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

#define	GPIO_FRONT_LED_ON  	TCC_GPB(27)

#define	GPIO_WIFI_CLK			TCC_GPB(28)	
#define GPIO_LED_PWM			TCC_GPB(29)
#define GPIO_BLU_EN				TCC_GPB(30)

#define GPIO_USB_PWR_EN         TCC_GPB(31)

#define GPIO_HUB_PWR_EN         TCC_GPD(2) /* [downer] A140509 */
#define GPIO_BAL_DET						TCC_GPD(13) //ES

#define	GPIO_MIPI_PWEN			TCC_GPE(31)

#define	GPIO_TOUCH_RST			TCC_GPF(0)
#define	GPIO_TSP_PWEN 			TCC_GPF(1)	

#define	GPIO_XMOS_PLAY_EN		TCC_GPF(2)	
#define	GPIO_AUDIO_N5V_EN		TCC_GPF(3)	
#define	GPIO_OPAMP_4V_EN		TCC_GPF(4)	

#define	GPIO_OPTIC_RST 			TCC_GPF(5)
#define	GPIO_PLAY_EN			TCC_GPF(6)
#define	GPIO_OPTIC_TX_EN		TCC_GPF(7)
#define	GPIO_OPTIC_PLAY_EN		TCC_GPF(8)
#define	GPIO_OPAMP_PWR_EN		TCC_GPF(9)
#define 	GPIO_VAR_BAL_READY_PWREN	TCC_GPF(10)
//#define	GPIO_HP_MUTE			TCC_GPF(10)

#define	GPIO_5V5_EN				TCC_GPF(11)
#define	GPIO_N2V8_EN			TCC_GPF(12)
#define	GPIO_OP_TX_PWEN			TCC_GPF(13)
    
#define	GPIO_CHG_CTL			TCC_GPF(14)

#define	GPIO_DAC_EN				TCC_GPF(15)
#define	GPIO_OTG_5V5_EN			TCC_GPF(16)

#define	GPIO_BT_TXD				TCC_GPF(17)
#define	GPIO_BT_RXD				TCC_GPF(18)
#define	GPIO_BT_RTS				TCC_GPF(19)
#define	GPIO_BT_CTS				TCC_GPF(20)

#define	GPIO_XMOS_USB_EN		TCC_GPF(21)	    
#define	GPIO_CPU_XMOS_EN		TCC_GPF(22)	

#define	GPIO_PMIC_HOLD			TCC_GPF(23)
#define	GPIO_TXD_RESET			TCC_GPF(24)		//cs8406
#define GPIO_USB20_HOST_EN      TCC_GPF(26)

//NOTE : TOUCH	
#define	GPIO_I2C_SDA21			TCC_GPF(27)
#define	GPIO_I2C_SCL21			TCC_GPF(28)

// for USB3.0 detect test
#define GPIO_OTG_DET            TCC_GPG(13)
    
//#define GPIO_DSD_ON             TCC_GPG(1)
//#define GPIO_LOW_BATT           TCC_GPG(5)
//#define GPIO_XMOS_CPU3          TCC_GPG(2)
#define GPIO_TXD_OPTIC_EN             TCC_GPG(1)
#define GPIO_TXD_COXIAL_EN             TCC_GPG(2)
#define GPIO_TXD_BNC_EN             TCC_GPG(3)
#define GPIO_TXD_XLR_EN            TCC_GPG(4)

#define GPIO_USB_ON             TCC_GPG(12)
#define GPIO_ADAPTER_DET        TCC_GPG(14)

#define GPIO_HOST_5V_EN			TCC_GPG(15) /* [downer] M140509 */
#define GPIO_OTG_FAULT			TCC_GPG(16)
#define GPIO_BT_HOST_WAKE		TCC_GPG(17)

#define	GPIO_CHG_DONE  			TCC_GPG(18) 
#define	GPIO_CHG_DET			0//#define	GPIO_CHG_DET			TCC_GPG(19)
#define	GPIO_SPDIF_TX_ON 	    TCC_GPG(19)

// for GMAC
#define GPIO_GMAC_INTB			TCC_GPG(11)

//avoid build error.
#define GPIO_SPDIF_TX_I2S_EN (0)
#define GPIO_XLR_MUTE 			(0)
#define GPIO_HP_XLR_SEL         (0)
#define GPIO_XLR_SEL            (0)
#define GPIO_XLR_RELAY_PWR_EN   (0)
#define GPIO_BAL_XLR_SEL        (0)
#define GPIO_VAR_AMP_MUTE       (0)
#define GPIO_TOSLINK_TX_PWEN    (0)
#define GPIO_TOSLINK_RX_PWEN    (0)
#define GPIO_SPDIF_TX_RST       (0)
#define GPIO_AUDIO_XLR_MUTE     (0)


#elif (CUR_AK_REV == AK500N_HW_WS2)
#define GPIO_XLR_MUTE           TCC_GPA(0)
#define GPIO_BAL_MUTE           TCC_GPA(1)
#define GPIO_VAR_XLR_MUTE       TCC_GPA(2)
#define GPIO_VAR_BAL_MUTE       TCC_GPA(3)
#define GPIO_VAR_AMP_MUTE       TCC_GPA(4)
#define GPIO_KEY_VOL_UP         TCC_GPA(5)
#define GPIO_KEY_VOL_DOWN       TCC_GPA(6)
#define GPIO_VARAMP_PWR_EN      TCC_GPA(7)
#define GPIO_CPU_I2S_EN         TCC_GPA(8)
#define GPIO_BAL_XLR_SEL        TCC_GPA(9)
#define VAR_OPAMP_EN            TCC_GPA(10)
#define GPIO_DAC_RST            TCC_GPA(11)
#define VAR_OPAMP_RST           TCC_GPA(12)
#define GPIO_WHEEL_LED          TCC_GPA(13)
#define GPIO_WHEEL_LED_PWM			GPIO_WHEEL_LED // to fix build error
#define GPIO_35HP_DET           TCC_GPA(14)
#define GPIO_I2C_SDA2           TCC_GPA(15)
#define GPIO_I2C_SCL2           TCC_GPA(16)
#define GPIO_HP_BAL_MUTE        TCC_GPA(17)
#define GPIO_HP_XLR_SEL         TCC_GPA(18)
#define GPIO_65HP_DET           TCC_GPA(19)
#define GPIO_I2C_SDA3           TCC_GPA(20) // MIPI_SDA
#define GPIO_I2C_SCL3           TCC_GPA(21) // MIPI_SCL
#define TC358768XBG_RESX        TCC_GPA(22) // MIPI_RST
#define GPIO_XMOS_SPI_SEL       TCC_GPA(23)
#define GPIO_I2C_SEL 			TCC_GPA(24)
#define GPIO_XMOS_PWR_EN		TCC_GPA(25)
#define GPIO_XMOS_TXD 			TCC_GPA(26)
#define GPIO_XMOS_RXD	 		TCC_GPA(27)
#define GPIO_XMOS_SCLK 			TCC_GPA(28)
#define GPIO_XMOS_SS 			TCC_GPA(29)
#define GPIO_XMOS_SDI 			TCC_GPA(30)
#define GPIO_XMOS_SDO 			TCC_GPA(31)


#define	GPIO_BT_DEV_WAKE		TCC_GPB(1)
#define	GPIO_BT_REG_ON			TCC_GPB(2)
#define GPIO_PLL25_VCTL			TCC_GPB(4)
#define GPIO_AUDIO_SW_ON        TCC_GPB(6)
#define GPIO_AUDIO_XLR_MUTE     TCC_GPB(7)
#define GPIO_AUDIO_SW_MUTE      TCC_GPB(8)//#define GPIO_AUDIO_BAL_MUTE     TCC_GPB(8)
#define	GPIO_I2C_SDA8           TCC_GPB(9)
#define	GPIO_I2C_SCL8           TCC_GPB(10)
#define	GPIO_TOUCH_INT			TCC_GPB(11)
#define	GPIO_SD30_DET			TCC_GPB(12)
#define	GPIO_PWR_ON_OFF			TCC_GPB(13)
#define GPIO_PD                 TCC_GPB(14)
#define	GPIO_WL_HOST_WAKE		TCC_GPB(15)
#define	GPIO_WL_REG_ON			TCC_GPB(16)
#define	GPIO_WL_DEV_WAKE		TCC_GPB(18)
#define	GPIO_WIFI_CMD			TCC_GPB(19)
#define	GPIO_WIFI_D0			TCC_GPB(20)
#define	GPIO_WIFI_D1			TCC_GPB(21)
#define	GPIO_WIFI_D2			TCC_GPB(22)
#define	GPIO_WIFI_D3			TCC_GPB(23)
#define GPIO_SATA_HOST_5V_EN    TCC_GPB(25)  // jhlim
#define GPIO_EXT_HOST_5V_EN     TCC_GPB(26)
#define	GPIO_FRONT_LED_ON       TCC_GPB(27)
#define	GPIO_WIFI_CLK			TCC_GPB(28)	
#define GPIO_LED_PWM			TCC_GPB(29)
#define GPIO_BLU_EN				TCC_GPB(30)
#define GPIO_SPDIF_RX_RST       TCC_GPB(31)

#define GPIO_MAC_TXCLK          TCC_GPC(0)
#define GPIO_MAC_MDC            TCC_GPC(1)
#define GPIO_MAC_MDIO           TCC_GPC(2)
#define GPIO_MAC_RXD0           TCC_GPC(3)
#define GPIO_MAC_RXD1           TCC_GPC(4)
#define GPIO_MAC_TXD0           TCC_GPC(5)
#define GPIO_MAC_TXD1           TCC_GPC(6)
#define GPIO_MAC_TXEN           TCC_GPC(7)
#define GPIO_MAC_RXDV           TCC_GPC(8)
#define GPIO_MAC_PHY_RSTN       TCC_GPC(9)
#define GPIO_MAC_RXD2           TCC_GPC(10)    
#define GPIO_MAC_RXD3           TCC_GPC(11)
#define GPIO_MAC_TXD2           TCC_GPC(12)
#define GPIO_MAC_TXD3           TCC_GPC(13)
#define GPIO_MAC_RXER           TCC_GPC(14)
#define GPIO_MAC_TXER           TCC_GPC(15)
#define GPIO_MAC_COL            TCC_GPC(16)
#define GPIO_MAC_CRS            TCC_GPC(17)
#define GPIO_MAC_RXD4           TCC_GPC(18)
#define GPIO_MAC_RXD5           TCC_GPC(19)
#define GPIO_MAC_RXD6           TCC_GPC(20)
#define GPIO_MAC_RXD7           TCC_GPC(21)
#define GPIO_MAC_TXD4           TCC_GPC(22)
#define GPIO_MAC_TXD5           TCC_GPC(23)
#define GPIO_MAC_TXD6           TCC_GPC(24)
#define GPIO_MAC_TXD7           TCC_GPC(25)
#define GPIO_MAC_RXCLK          TCC_GPC(26)
#define GPIO_ETHERNET_ON        TCC_GPC(31)

#define GPIO_ADAPTER_EN         TCC_GPD(0)
#define GPIO_HUB_PWR_EN         TCC_GPD(2)
#define VAR_XLR_RELAY_PWR_EN    TCC_GPD(3)
#define VAR_BAL_RELAY_PWR_EN    TCC_GPD(4)
#define BAL_RELAY_PWR_EN        TCC_GPD(5)
#define GPIO_YELLOW_LED_EN      TCC_GPD(6)
#define GPIO_WHITE_LED_EN       TCC_GPD(7)
#define GPIO_USB_DET            TCC_GPD(8)
#define GPIO_YELLOW_LED_PWM     TCC_GPD(9)
#define GPIO_WHITE_LED_PWM      TCC_GPD(11)
#define GPIO_ADAPTER_DET        TCC_GPD(12)
#define GPIO_BAL_DET						TCC_GPD(13) //ES

#define	GPIO_MIPI_PWEN			TCC_GPE(31)

#define	GPIO_TOUCH_RST			TCC_GPF(0)
#define	GPIO_TSP_PWEN 			TCC_GPF(1)	
#define GPIO_XLR_RELAY_PWR_EN   TCC_GPF(2)
#define	GPIO_AUDIO_N5V_EN		TCC_GPF(3)	
#define	GPIO_OPAMP_4V_EN		TCC_GPF(4)	
#define	GPIO_OPTIC_RST 			TCC_GPF(5)//#define GPIO_PLL_RST            TCC_GPF(5)
#define GPIO_SPDIF_RX_I2S_EN    TCC_GPF(6)		//#define	GPIO_PLAY_EN				TCC_GPF(6)//
#define GPIO_SPDIF_TX_I2S_EN    TCC_GPF(7)
#define GPIO_XLR_SEL            TCC_GPF(8)
#define	GPIO_OPAMP_PWR_EN		TCC_GPF(9)
#define GPIO_VAR_BAL_READY_PWREN TCC_GPF(10)
#define	GPIO_5V5_EN				TCC_GPF(11)
#define	GPIO_N2V8_EN			TCC_GPF(12)
#define GPIO_TOSLINK_TX_PWEN    TCC_GPF(13)
#define GPIO_TOSLINK_RX_PWEN    TCC_GPF(14)
#define	GPIO_DAC_EN				TCC_GPF(15)
#define	GPIO_BT_TXD				TCC_GPF(17)
#define	GPIO_BT_RXD				TCC_GPF(18)
#define	GPIO_BT_RTS				TCC_GPF(19)
#define	GPIO_BT_CTS				TCC_GPF(20)
#define	GPIO_XMOS_USB_EN		TCC_GPF(21)	    
#define	GPIO_CPU_XMOS_EN		TCC_GPF(22)
#define	GPIO_PMIC_HOLD			TCC_GPF(23)
#define GPIO_SPDIF_TX_RST       TCC_GPF(24)
#define GPIO_BM_S0              TCC_GPF(25)
//#define GPIO_USB20_HOST_EN      TCC_GPF(26) // USB ON/OFF
#define GPIO_USB_ONOFF      TCC_GPF(26) // USB ON/OFF

//NOTE : TOUCH	
#define	GPIO_I2C_SDA21			TCC_GPF(27)
#define	GPIO_I2C_SCL21			TCC_GPF(28)
#define GPIO_PWR_ON_INDICATOR   TCC_GPF(29)

#define GPIO_XMOS_RST_N         TCC_GPG(0)
#define GPIO_TXD_OPTIC_EN       TCC_GPG(1)
#define GPIO_TXD_COXIAL_EN      TCC_GPG(2)
#define GPIO_TXD_BNC_EN         TCC_GPG(3)
#define GPIO_TXD_XLR_EN         TCC_GPG(4)
#define GPIO_DSP_MCLK           TCC_GPG(6)
#define GPIO_DSP_BCLK           TCC_GPG(7)    
#define GPIO_DSP_LRCK           TCC_GPG(8)
#define GPIO_DSP_DO             TCC_GPG(9)    
#define GPIO_MAC_PMEB           TCC_GPG(10)
#define GPIO_MAC_INTB           TCC_GPG(11)
#define GPIO_HP_SEL             TCC_GPG(13)
#define GPIO_HUB_RST            TCC_GPG(14)
#define GPIO_CD_HOST_5V_EN      TCC_GPG(15)   // jhlim
#define GPIO_CHARGING           TCC_GPG(16)
#define GPIO_BT_HOST_WAKE		TCC_GPG(17)
#define	GPIO_SPDIF_RX_ON 	    TCC_GPG(18)
#define	GPIO_SPDIF_TX_ON 	    TCC_GPG(19)
 
// for GMAC
#define GPIO_GMAC_INTB			TCC_GPG(11)

//AK500N rename gpio
#define GPIO_CHG_DET 			GPIO_ADAPTER_DET
#define GPIO_CHG_DONE 			GPIO_CHARGING
#define GPIO_USB_ON             GPIO_USB_DET
#define	GPIO_HOST_5V_EN			GPIO_CD_HOST_5V_EN

//temporary gpio
//#define	GPIO_PLAY_EN				TCC_GPG(5)
//#define	GPIO_OPTIC_RST 			TCC_GPF(5)
//#define	GPIO_OPTIC_TX_EN		TCC_GPG(5)
//#define	GPIO_OPTIC_PLAY_EN	TCC_GPG(5)
//#define	GPIO_OPTIC_TX_DET		TCC_GPG(5)
//#define	GPIO_AUDIO_SW_MUTE	TCC_GPG(5)
//#define	VAR_AMP_MUTE				TCC_GPG(5)
//#define	GPIO_OP_TX_PWEN			TCC_GPG(5)
//#define	GPIO_BAL_UNBAL_SEL	TCC_GPG(5)
//#define	GPIO_TXD_RESET			TCC_GPG(5)
//#define	GPIO_HP_DET					TCC_GPG(5)
//#define	GPIO_XMOS_PLAY_EN		TCC_GPG(5)
//#define	GPIO_XMOS_I2S_EN		TCC_GPG(5)
#define GPIO_HP35_MUTE	TCC_GPG(5)
#define GPIO_HP25_MUTE  TCC_GPG(5)



#elif (CUR_AK_REV == AK500N_HW_ES1) || (CUR_AK_REV == AK500N_HW_TP)
#define GPIO_XLR_MUTE           TCC_GPA(0)
#define GPIO_BAL_MUTE           TCC_GPA(1)
#define GPIO_VAR_XLR_MUTE       TCC_GPA(2)
#define GPIO_VAR_BAL_MUTE       TCC_GPA(3)
#define GPIO_VAR_AMP_MUTE       TCC_GPA(4)
#define GPIO_KEY_VOL_UP         TCC_GPA(5)
#define GPIO_KEY_VOL_DOWN       TCC_GPA(6)
#define GPIO_VARAMP_PWR_EN      TCC_GPA(7)
#define GPIO_CPU_I2S_EN         TCC_GPA(8)
#define GPIO_BAL_XLR_SEL        TCC_GPA(9)
#define VAR_OPAMP_EN            TCC_GPA(10)
#define GPIO_DAC_RST            TCC_GPA(11)
#define VAR_OPAMP_RST           TCC_GPA(12)
#define GPIO_WHEEL_LED          TCC_GPA(13)
#define GPIO_35HP_DET           TCC_GPA(14)
#define GPIO_I2C_SDA2           TCC_GPA(15)
#define GPIO_I2C_SCL2           TCC_GPA(16)
#define GPIO_HP_BAL_MUTE        TCC_GPA(17)
#define GPIO_HP_XLR_SEL         TCC_GPA(18)
#define GPIO_65HP_DET           TCC_GPA(19)
#define GPIO_I2C_SDA3           TCC_GPA(20) // MIPI_SDA
#define GPIO_I2C_SCL3           TCC_GPA(21) // MIPI_SCL
#define TC358768XBG_RESX        TCC_GPA(22) // MIPI_RST
#define GPIO_XMOS_SPI_SEL       TCC_GPA(23)
#define GPIO_I2C_SEL 			TCC_GPA(24)
#define GPIO_XMOS_PWR_EN		TCC_GPA(25)
#define GPIO_XMOS_TXD 			TCC_GPA(26)
#define GPIO_XMOS_RXD	 		TCC_GPA(27)
#define GPIO_XMOS_SCLK 			TCC_GPA(28)
#define GPIO_XMOS_SS 			TCC_GPA(29)
#define GPIO_XMOS_SDI 			TCC_GPA(30)
#define GPIO_XMOS_SDO 			TCC_GPA(31)

#if (CUR_AK_REV == AK500N_HW_TP)
#define GPIO_FUEL_I2C_EN        TCC_GPB(0)    
#endif   
#define	GPIO_BT_DEV_WAKE		TCC_GPB(1)
#define	GPIO_BT_REG_ON			TCC_GPB(2)
#define GPIO_PLL25_VCTL			TCC_GPB(4)
#define GPIO_AUDIO_SW_ON        TCC_GPB(6)
#define GPIO_AUDIO_XLR_MUTE     TCC_GPB(7)
#define GPIO_AUDIO_SW_MUTE      TCC_GPB(8)//#define GPIO_AUDIO_BAL_MUTE     TCC_GPB(8)
#define	GPIO_I2C_SDA8           TCC_GPB(9)
#define	GPIO_I2C_SCL8           TCC_GPB(10)
#define	GPIO_TOUCH_INT			TCC_GPB(11)
#define	GPIO_SD30_DET			TCC_GPB(12)
#define	GPIO_PWR_ON_OFF			TCC_GPB(13)
#define GPIO_PD                 TCC_GPB(14)
#define	GPIO_WL_HOST_WAKE		TCC_GPB(15)
#define	GPIO_WL_REG_ON			TCC_GPB(16)
#define	GPIO_WL_DEV_WAKE		TCC_GPB(18)
#define	GPIO_WIFI_CMD			TCC_GPB(19)
#define	GPIO_WIFI_D0			TCC_GPB(20)
#define	GPIO_WIFI_D1			TCC_GPB(21)
#define	GPIO_WIFI_D2			TCC_GPB(22)
#define	GPIO_WIFI_D3			TCC_GPB(23)
#define GPIO_REAR_HOST_5V_EN		TCC_GPB(24) //ES
#define GPIO_SATA_HOST_5V_EN    TCC_GPB(25)  // jhlim
#define GPIO_FRONT_HOST_5V_EN     TCC_GPB(26)
#define	GPIO_FRONT_LED_ON       TCC_GPB(27)
#define	GPIO_WIFI_CLK			TCC_GPB(28)	
#define GPIO_LED_PWM			TCC_GPB(29)
#define GPIO_BLU_EN				TCC_GPB(30)
#define GPIO_SPDIF_RX_RST       TCC_GPB(31)

#define GPIO_MAC_TXCLK          TCC_GPC(0)
#define GPIO_MAC_MDC            TCC_GPC(1)
#define GPIO_MAC_MDIO           TCC_GPC(2)
#define GPIO_MAC_RXD0           TCC_GPC(3)
#define GPIO_MAC_RXD1           TCC_GPC(4)
#define GPIO_MAC_TXD0           TCC_GPC(5)
#define GPIO_MAC_TXD1           TCC_GPC(6)
#define GPIO_MAC_TXEN           TCC_GPC(7)
#define GPIO_MAC_RXDV           TCC_GPC(8)
#define GPIO_MAC_PHY_RSTN       TCC_GPC(9)
#define GPIO_MAC_RXD2           TCC_GPC(10)    
#define GPIO_MAC_RXD3           TCC_GPC(11)
#define GPIO_MAC_TXD2           TCC_GPC(12)
#define GPIO_MAC_TXD3           TCC_GPC(13)
#define GPIO_MAC_RXER           TCC_GPC(14)
#define GPIO_MAC_COL            TCC_GPC(16)
#define GPIO_MAC_CRS            TCC_GPC(17)
#define GPIO_MAC_INTB		TCC_GPC(21) //ES
#define GPIO_MAC_RXCLK          TCC_GPC(26)
#define GPIO_ETHERNET_ON        TCC_GPC(31)

#define GPIO_ADAPTER_EN         TCC_GPD(0)
#define GPIO_SSD0		TCC_GPD(1) //ES
#define GPIO_HUB_PWR_EN         TCC_GPD(2)
#define VAR_XLR_RELAY_PWR_EN    TCC_GPD(3)
#define VAR_BAL_RELAY_PWR_EN    TCC_GPD(4)
#define BAL_RELAY_PWR_EN        TCC_GPD(5)
#define GPIO_YELLOW_LED_EN      TCC_GPD(6)
#define GPIO_WHITE_LED_EN       TCC_GPD(7)
#define GPIO_USB_DET            TCC_GPD(8)
#define GPIO_WHEEL_LED_PWM		TCC_GPD(9)
#define GPIO_WHITE_LED_PWM      TCC_GPD(11)
#define GPIO_ADAPTER_DET        TCC_GPD(12)
#define GPIO_BAL_DET		TCC_GPD(13) //ES
#if (CUR_AK_REV == AK500N_HW_TP)
#define GPIO_YELLOW_LED_PWM     TCC_GPD(14)    
#endif
#define GPIO_SSD1		TCC_GPD(15)
#define GPIO_SSD2		TCC_GPD(16)
#define GPIO_SSD3		TCC_GPD(17)
#if (CUR_AK_REV == AK500N_HW_TP)
#define GPIO_RAID0              TCC_GPD(18)
#define GPIO_RAID1              TCC_GPD(19)    
#endif
    
#define	GPIO_MIPI_PWEN			TCC_GPE(31)

#define	GPIO_TOUCH_RST			TCC_GPF(0)
#define	GPIO_TSP_PWEN 			TCC_GPF(1)	
#define GPIO_XLR_RELAY_PWR_EN   TCC_GPF(2)
#define	GPIO_AUDIO_N5V_EN		TCC_GPF(3)	
#define	GPIO_OPAMP_4V_EN		TCC_GPF(4)	
#define	GPIO_OPTIC_RST 			TCC_GPF(5)//#define GPIO_PLL_RST            TCC_GPF(5)
#define GPIO_SPDIF_RX_I2S_EN    TCC_GPF(6)		//#define	GPIO_PLAY_EN
#define GPIO_SPDIF_TX_I2S_EN    TCC_GPF(7)
#define GPIO_XLR_SEL            TCC_GPF(8)
#define	GPIO_OPAMP_PWR_EN		TCC_GPF(9)
#define GPIO_CHG_OFF		TCC_GPF(10) //ES
#define	GPIO_5V5_EN				TCC_GPF(11)
#define	GPIO_N2V8_EN			TCC_GPF(12)
#define GPIO_TOSLINK_TX_PWEN    TCC_GPF(13)
#define GPIO_TOSLINK_RX_PWEN    TCC_GPF(14)
#define	GPIO_DAC_EN				TCC_GPF(15)
#define	GPIO_BT_TXD				TCC_GPF(17)
#define	GPIO_BT_RXD				TCC_GPF(18)
#define	GPIO_BT_RTS				TCC_GPF(19)
#define	GPIO_BT_CTS				TCC_GPF(20)
#define	GPIO_XMOS_USB_EN		TCC_GPF(21)	    
#define	GPIO_CPU_XMOS_EN		TCC_GPF(22)	// CPU_XMOS_EN
#define	GPIO_PMIC_HOLD			TCC_GPF(23)
#define GPIO_SPDIF_TX_RST       TCC_GPF(24)
#define GPIO_BM_S0              TCC_GPF(25)
#define GPIO_USB20_HOST_EN      TCC_GPF(26) // USB ON/OFF
#define GPIO_USB_ONOFF          GPIO_USB20_HOST_EN
//NOTE : TOUCH	
#define	GPIO_I2C_SDA21			TCC_GPF(27)
#define	GPIO_I2C_SCL21			TCC_GPF(28)
#if (CUR_AK_REV <= AK500N_HW_ES1)    
#define GPIO_PWR_ON_INDICATOR   TCC_GPF(29)
#endif

#define GPIO_XMOS_RST_N         TCC_GPG(0)
#define GPIO_TXD_OPTIC_EN       TCC_GPG(1)
#define GPIO_TXD_COXIAL_EN      TCC_GPG(2)
#define GPIO_TXD_BNC_EN         TCC_GPG(3)
#define GPIO_TXD_XLR_EN         TCC_GPG(4)
#define GPIO_HP25_MUTE		TCC_GPG(5) //ES
#define GPIO_DSP_MCLK           TCC_GPG(6)
#define GPIO_DSP_BCLK           TCC_GPG(7)    
#define GPIO_DSP_LRCK           TCC_GPG(8)
#define GPIO_DSP_DO             TCC_GPG(9)    
#define GPIO_MAC_PMEB           TCC_GPG(10)
#define GPIO_HP35_MUTE		TCC_GPG(11) //ES
#if (CUR_AK_REV <= AK500N_HW_ES1)        
#define GPIO_RX_PLL_UNLOCK 	TCC_GPG(12) //TP  open port  no use
#else
#define GPIO_RX_PLL_UNLOCK 	TCC_GPG(12) //TP  open port  no use

#define GPIO_OPTICAL_INT        TCC_GPG(12)
#endif
#define GPIO_HP_SEL             TCC_GPG(13) //ES
#define GPIO_HUB_RST            TCC_GPG(14)
#define GPIO_CD_HOST_5V_EN      TCC_GPG(15)   // jhlim
#define GPIO_CHARGING           TCC_GPG(16)
#define GPIO_BT_HOST_WAKE		TCC_GPG(17)
#define	GPIO_SPDIF_RX_ON 	    TCC_GPG(18)
#define	GPIO_SPDIF_TX_ON 	    TCC_GPG(19)
 
// for GMAC
//#define GPIO_GMAC_INTB			TCC_GPG(11) //WS2

//AK500N rename gpio
#define GPIO_CHG_DET 			GPIO_ADAPTER_DET
#define GPIO_CHG_DONE 			GPIO_CHARGING
#define GPIO_USB_ON            TCC_GPC(15) // GPIO_USB_DET
#define	GPIO_HOST_5V_EN			GPIO_CD_HOST_5V_EN


#endif  // CUR_AK_REV == AK500N_HW_ES1
#endif

#endif	
