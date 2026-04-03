#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/mmc/host.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>

#include <mach/mmc.h>

#include <mach/gpio.h>
#include <mach/irqs.h>
#include <mach/bsp.h>

#include <asm/mach-types.h>

#include "devices.h"
#include <mach/board-tcc8930.h>

#if defined(CONFIG_MMC_TCC_SDHC)
extern void tcc_init_sdhc_devices(void);

#if defined(CONFIG_WIFI_PWR_CTL)
extern int wifi_stat;
#endif

#define MMC_VOLTAGE_330     3300000
#define MMC_VOLTAGE_180     1800000

struct tcc_mmc_platform_data tcc8930_mmc_platform_data[];

/*
 * [downer] A130910 
 */ 
#ifdef CONFIG_REGULATOR
static struct regulator *ldo_sd30;
static unsigned int cur_voltage = 0;
#endif
  
typedef enum {
	TCC_MMC_BUS_WIDTH_4 = 4,
	TCC_MMC_BUS_WIDTH_8 = 8
} tcc_mmc_bus_width_type;

#define TCC_MMC_PORT_NULL	0x0FFFFFFF

// PIC 0
#define HwINT0_EI4					Hw7						// R/W, External Interrupt 4 enable
#define HwINT0_EI5					Hw8						// R/W, External Interrupt 5 enable

// PIC 1
#define HwINT1_SD0					Hw12					// R/W, SD/MMC 0 interrupt enable
#define HwINT1_SD1					Hw13					// R/W, SD/MMC 1 interrupt enable
#define HwINT1_SD2	 				Hw1 					// R/W, SD/MMC 2 Interrupt enable
#define HwINT1_SD3		 			Hw0 					// R/W, SD/MMC 3 Interrupt enable

static struct mmc_port_config mmc_ports[] = {
#if defined(CONFIG_MMC_TCC_SUPPORT_EMMC)
	[TCC_MMC_TYPE_EMMC0] = {
		.data0	= TCC_GPD(29),
		.data1	= TCC_GPD(28),
		.data2	= TCC_GPD(27),
		.data3	= TCC_GPD(26),
		.data4	= TCC_GPD(25),
		.data5	= TCC_GPD(24),
		.data6	= TCC_GPD(23),
		.data7	= TCC_GPD(22),
		.cmd	= TCC_GPD(30),
		.clk	= TCC_GPD(31),
		.func	= GPIO_FN(2),
		.width	= TCC_MMC_BUS_WIDTH_8,
		.strength = GPIO_CD(1),

		.cd	= TCC_MMC_PORT_NULL,	
		.pwr	= TCC_MMC_PORT_NULL,	
	},
	[TCC_MMC_TYPE_EMMC1] = {
		.data0	= TCC_GPD(16),
		.data1	= TCC_GPD(15),
		.data2	= TCC_GPD(14),
		.data3	= TCC_GPD(13),
		.data4	= TCC_GPD(12),
		.data5	= TCC_GPD(11),
		.data6	= TCC_GPD(10),
		.data7	= TCC_GPD(9),
		.cmd	= TCC_GPD(18),
		.clk	= TCC_GPD(17),
		.func	= GPIO_FN(2),
		.width	= TCC_MMC_BUS_WIDTH_8,
		.strength = GPIO_CD(1),

		.cd	= TCC_MMC_PORT_NULL,	
		.pwr	= TCC_MMC_PORT_NULL,	
	},
#endif
	[TCC_MMC_TYPE_SD] = {
		.data0	= TCC_GPA(9),
		.data1	= TCC_GPA(10),
		.data2	= TCC_GPA(11),
		.data3	= TCC_GPA(12),
		.data4	= TCC_MMC_PORT_NULL,
		.data5	= TCC_MMC_PORT_NULL,
		.data6	= TCC_MMC_PORT_NULL,
		.data7	= TCC_MMC_PORT_NULL,
		.cmd	= TCC_GPA(8),
		.clk	= TCC_GPA(7),
		.func	= GPIO_FN(2),
		.width	= TCC_MMC_BUS_WIDTH_4,
		.strength = GPIO_CD(3),

		.cd	= GPIO_SD30_DET,
		.pwr	= TCC_MMC_PORT_NULL,
	},
	[TCC_MMC_TYPE_WIFI] = {
		.data0	= TCC_GPF(26),
		.data1	= TCC_GPF(25),
		.data2	= TCC_GPF(24),
		.data3	= TCC_GPF(23),
		.data4	= TCC_MMC_PORT_NULL,
		.data5	= TCC_MMC_PORT_NULL,
		.data6	= TCC_MMC_PORT_NULL,
		.data7	= TCC_MMC_PORT_NULL,
		.cmd	= TCC_GPF(27),
		.clk	= TCC_GPF(28),
		.func	= GPIO_FN(3),
		.width	= TCC_MMC_BUS_WIDTH_4,
		.strength = GPIO_CD(1),

		.cd	= TCC_MMC_PORT_NULL,	
		.pwr	= GPIO_WL_REG_ON,
	},
};

static int tccUsedSDportNum = TCC_MMC_TYPE_MAX;

#ifdef CONFIG_REGULATOR
static int g_ref_regulator = 0;

void mmc_regulator_enable(void)
{
    if (g_ref_regulator == 0) {
        g_ref_regulator = 1;        
        printk("\x1b[1;35m [mmc] %s, Enable SD30 voltage... \x1b[0m\n", __func__);
        regulator_enable(ldo_sd30);
    }
}

void mmc_regulator_disable(void)
{
    if (g_ref_regulator == 1) {
        g_ref_regulator = 0;        
        printk("\x1b[1;35m [mmc] %s, Disable SD30 voltage... \x1b[0m\n", __func__);        
        regulator_disable(ldo_sd30);        
    }
}
#endif

int tcc893x_sd_card_detect(void)
{
	// AK_FIXME_LATER
	return gpio_get_value(mmc_ports[TCC_MMC_TYPE_SD].cd) ? 0 : 1;
}

int tcc8930_mmc_init(struct device *dev, int id)
{
	struct mmc_port_config *mmc_port;
	int mmc_gpio_config;

//    printk("%s(%d) id = %d\n", __func__, __LINE__, id);

	BUG_ON(id >= tccUsedSDportNum);

	mmc_port = &mmc_ports[id];

	// 1. Power Up
	if (id == TCC_MMC_TYPE_WIFI) {
		/* init gpio ports for wifi */
		tcc_gpio_config(GPIO_WL_REG_ON, GPIO_OUTPUT | GPIO_FN0);         // TCC_GPB(16) W
		tcc_gpio_config(GPIO_WL_DEV_WAKE, GPIO_OUTPUT | GPIO_FN0);           // WL_DEV_WAKE
	}

	// 2. GPIO Function
	mmc_gpio_config = mmc_port->func | mmc_port->strength;
	tcc_gpio_config(mmc_port->data0, mmc_gpio_config);
	tcc_gpio_config(mmc_port->data1, mmc_gpio_config);
	tcc_gpio_config(mmc_port->data2, mmc_gpio_config);
	tcc_gpio_config(mmc_port->data3, mmc_gpio_config);

	if (mmc_port->width == TCC_MMC_BUS_WIDTH_8) {
		tcc_gpio_config(mmc_port->data4, mmc_gpio_config);
		tcc_gpio_config(mmc_port->data5, mmc_gpio_config);
		tcc_gpio_config(mmc_port->data6, mmc_gpio_config);
		tcc_gpio_config(mmc_port->data7, mmc_gpio_config);
	}

	tcc_gpio_config(mmc_port->cmd, mmc_gpio_config | GPIO_CD(1));
	tcc_gpio_config(mmc_port->clk, mmc_port->func | GPIO_CD(3));

	// 3. Card Detect
	if (mmc_port->cd != TCC_MMC_PORT_NULL) {
//		tcc_gpio_config(mmc_port->cd, GPIO_FN(0) | GPIO_PULL_DISABLE);
		tcc_gpio_config(mmc_port->cd, GPIO_FN(0) | GPIO_PULLUP); /* [downer] M140916 default PULLUP */
		gpio_request(mmc_port->cd, "sd_cd");

		gpio_direction_input(mmc_port->cd);
	}

	return 0;
}

static unsigned int wifi_on = 0;

void tcc8930_set_card_detected(int on)
{
	wifi_on = on;
}

int tcc8930_mmc_card_detect(struct device *dev, int id)
{
	BUG_ON(id >= tccUsedSDportNum);

	if(id == TCC_MMC_TYPE_WIFI)
		return wifi_on;

#if defined(CONFIG_BROADCOM_WIFI) && defined(CONFIG_WIFI_PWR_CTL)
	if(id == TCC_MMC_TYPE_WIFI && !wifi_stat) {
		return 0;
	}else if(id == TCC_MMC_TYPE_WIFI && wifi_stat){
		return 1;
	}
#endif

	if(mmc_ports[id].cd == TCC_MMC_PORT_NULL)
		return 1;

	return gpio_get_value(mmc_ports[id].cd) ? 0 : 1;	
}

int tcc8930_mmc_suspend(struct device *dev, int id)
{
    struct mmc_port_config *mmc_port;

	mmc_port = &mmc_ports[id];

	if (id == TCC_MMC_TYPE_SD) {    
#ifdef CONFIG_REGULATOR        
        if (tcc893x_sd_card_detect()) {
            printk("SD I/O regulator disable\n");            
            mmc_regulator_disable();
        }
#endif
	}    

	tcc_gpio_config(mmc_port->clk, GPIO_FN(0) | GPIO_PULL_DISABLE);
	tcc_gpio_config(mmc_port->cmd, GPIO_FN(0) | GPIO_PULL_DISABLE);
	tcc_gpio_config(mmc_port->data0, GPIO_FN(0) | GPIO_PULL_DISABLE);
	tcc_gpio_config(mmc_port->data1, GPIO_FN(0) | GPIO_PULL_DISABLE);
	tcc_gpio_config(mmc_port->data2, GPIO_FN(0) | GPIO_PULL_DISABLE);
	tcc_gpio_config(mmc_port->data3, GPIO_FN(0) | GPIO_PULL_DISABLE);

	if (mmc_port->width == TCC_MMC_BUS_WIDTH_8) {
        tcc_gpio_config(mmc_port->data4, GPIO_FN(0) | GPIO_PULL_DISABLE);
        tcc_gpio_config(mmc_port->data5, GPIO_FN(0) | GPIO_PULL_DISABLE);
        tcc_gpio_config(mmc_port->data6, GPIO_FN(0) | GPIO_PULL_DISABLE);
        tcc_gpio_config(mmc_port->data7, GPIO_FN(0) | GPIO_PULL_DISABLE);
	}

	return 0;
}

int tcc8930_mmc_resume(struct device *dev, int id)
{
    struct mmc_port_config *mmc_port;
    
    mmc_port = &mmc_ports[id];

	if (id == TCC_MMC_TYPE_SD) {    
#ifdef CONFIG_REGULATOR           
        if (tcc893x_sd_card_detect()) {
            printk("SD I/O regulator on\n");            
            mmc_regulator_enable();
        }
#endif       
	}
    
	tcc_gpio_config(mmc_port->clk, mmc_ports[id].func | mmc_ports[id].strength);
	tcc_gpio_config(mmc_port->cmd, mmc_ports[id].func | mmc_ports[id].strength);
	tcc_gpio_config(mmc_port->data0, mmc_ports[id].func | mmc_ports[id].strength);
	tcc_gpio_config(mmc_port->data1, mmc_ports[id].func | mmc_ports[id].strength);
	tcc_gpio_config(mmc_port->data2, mmc_ports[id].func | mmc_ports[id].strength);
	tcc_gpio_config(mmc_port->data3, mmc_ports[id].func | mmc_ports[id].strength);

    if (mmc_port->width == TCC_MMC_BUS_WIDTH_8) {
        tcc_gpio_config(mmc_port->data4, mmc_ports[id].func | mmc_ports[id].strength);
        tcc_gpio_config(mmc_port->data5, mmc_ports[id].func | mmc_ports[id].strength);
        tcc_gpio_config(mmc_port->data6, mmc_ports[id].func | mmc_ports[id].strength);
        tcc_gpio_config(mmc_port->data7, mmc_ports[id].func | mmc_ports[id].strength);
    }

	return 0;
}

int tcc8930_mmc_set_power(struct device *dev, int id, int on)
{
	struct mmc_port_config *mmc_port;

	mmc_port = &mmc_ports[id];
    
	if (on) {
		/* power */
		if(mmc_ports[id].pwr != TCC_MMC_PORT_NULL) {
			if(id == TCC_MMC_TYPE_WIFI) {
				printk("tcc_mmc : WIFI power ON\n");
				gpio_set_value(GPIO_WL_REG_ON, 0);
				msleep(500);
				gpio_set_value(GPIO_WL_REG_ON, 1);
			}
		}  
 	} 
    else {
		/* power */
		if(mmc_ports[id].pwr != TCC_MMC_PORT_NULL) {
			if(id == TCC_MMC_TYPE_WIFI) {
				printk("tcc_mmc : WIFI power OFF\n");
				gpio_set_value(GPIO_WL_REG_ON, 0);
			}
		}
 	}

	return 0;
}

int tcc8930_mmc_set_bus_width(struct device *dev, int id, int width)
{
	return 0;
}

int tcc8930_mmc_cd_int_config(struct device *dev, int id, unsigned int cd_irq)
{
	if(tcc8930_mmc_platform_data[id].cd_int_num > 0) 
		tcc_gpio_config_ext_intr(tcc8930_mmc_platform_data[id].cd_irq_num, tcc8930_mmc_platform_data[id].cd_ext_irq);
	else
		return -1;

	return 0;
}

int tcc8930_mmc_switch_voltage(struct device *dev, int id, int level)
{
    int rc;

    switch (level) {
    case SD30_ON:
        /*
         * [downer] A130822
         * ToDo: PMIC SD I/O voltage 3.3v->1.8v
         */
        if (id == TCC_MMC_TYPE_SD) { // only SD  
            if (cur_voltage != MMC_VOLTAGE_180) {        
                printk("[mmc] %s, Down SD30 voltage from 3.3V to 1.8V...\n", __func__);     
#ifdef CONFIG_REGULATOR                    
                rc = regulator_set_voltage(ldo_sd30, MMC_VOLTAGE_180, MMC_VOLTAGE_180);
                if (rc) {
                    printk("switching to 1.8V failed. Switching back to 3.3V\n");
                    regulator_set_voltage(ldo_sd30, MMC_VOLTAGE_330, MMC_VOLTAGE_330);

                    cur_voltage = MMC_VOLTAGE_330;

                    return -1;
                }           
                cur_voltage = MMC_VOLTAGE_180;
#endif                    
            }
        }

        break;
    case SD30_OFF:
        /*
         * [downer] A130822
         * ToDo: PMIC SD I/O voltage 1.8v->3.3v
         */
        if (id == TCC_MMC_TYPE_SD) { // only SD  
            if (cur_voltage != MMC_VOLTAGE_330) {
                printk("[mmc] %s, Up SD30 voltage from 1.8V to 3.3V...\n", __func__);                
#ifdef CONFIG_REGULATOR                    
                rc = regulator_set_voltage(ldo_sd30, MMC_VOLTAGE_330, MMC_VOLTAGE_330);
                if (rc) {
                    printk("switching to 3.3V failed\n");
                    return -1;
                }
                cur_voltage = MMC_VOLTAGE_330;                
#endif                    
            }
        }
            
        break;
    case SD30_ENABLE:      
#ifdef CONFIG_REGULATOR
        mmc_regulator_enable();
#endif
        break;
    case SD30_DISABLE:
#ifdef CONFIG_REGULATOR
        mmc_regulator_disable();            
#endif
        break;
    default:
        break;
    }
	return 0;
}

/*
 * 0x1000 : TCC8930_D3_08X4_SV_0.1 - DDR3 1024(32Bit)
 */
struct tcc_mmc_platform_data tcc8930_mmc_platform_data[] = {
#if defined(CONFIG_MMC_TCC_SUPPORT_EMMC)		
	// [0]:eMMC0,  [1]:SD,  [2]:WiFi, [3]:eMMC1
	[TCC_MMC_TYPE_EMMC0] = {
		.slot	= 3,
		.caps	= MMC_CAP_SDIO_IRQ | MMC_CAP_8_BIT_DATA | MMC_CAP_NONREMOVABLE
#if defined(CONFIG_MMC_TCC_HIGHSPEED_MODE)        
        | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED
#endif
#ifdef CONFIG_MMC_TCC_DDR_MODE
        | MMC_CAP_UHS_DDR50 | MMC_CAP_1_8V_DDR
        | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED
		| MMC_CAP_BKOPS | MMC_CAP_BUS_WIDTH_TEST | MMC_CAP_CMD23
#endif
        ,
		.f_min	= 100000,
		.f_max	= MMC_MAX_CLOCK_SPEED,//24000000,	/* support highspeed mode */
		.ocr_mask = MMC_VDD_32_33 | MMC_VDD_33_34,
		.init	= tcc8930_mmc_init,
		.card_detect = tcc8930_mmc_card_detect,
		.cd_int_config = tcc8930_mmc_cd_int_config,
		.suspend = tcc8930_mmc_suspend,
		.resume	= tcc8930_mmc_resume,
		.set_power = tcc8930_mmc_set_power,
		.set_bus_width = tcc8930_mmc_set_bus_width,

		.cd_int_num = -1,
		.cd_ext_irq = -1,
		.peri_name = PERI_SDMMC3,
		.io_name = RB_SDMMC3CONTROLLER,
		.pic = HwINT1_SD3,
	}, 
	[TCC_MMC_TYPE_EMMC1] = {
		.slot	= 0,
		.caps	= MMC_CAP_SDIO_IRQ | MMC_CAP_8_BIT_DATA | MMC_CAP_NONREMOVABLE
#if defined(CONFIG_MMC_TCC_HIGHSPEED_MODE)        
        | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED
#endif
#ifdef CONFIG_MMC_TCC_DDR_MODE
        | MMC_CAP_UHS_DDR50 | MMC_CAP_1_8V_DDR
        | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED
		| MMC_CAP_BKOPS | MMC_CAP_BUS_WIDTH_TEST | MMC_CAP_CMD23		
#endif
        ,
		.f_min	= 100000,
		.f_max	= MMC_MAX_CLOCK_SPEED,//24000000,	/* support highspeed mode */
		.ocr_mask = MMC_VDD_32_33 | MMC_VDD_33_34,
		.init	= tcc8930_mmc_init,
		.card_detect = tcc8930_mmc_card_detect,
		.cd_int_config = tcc8930_mmc_cd_int_config,
		.suspend = tcc8930_mmc_suspend,
		.resume	= tcc8930_mmc_resume,
		.set_power = tcc8930_mmc_set_power,
		.set_bus_width = tcc8930_mmc_set_bus_width,

		.cd_int_num = -1,
		.cd_ext_irq = -1,
		.peri_name = PERI_SDMMC1,
		.io_name = RB_SDMMC1CONTROLLER,
		.pic = HwINT1_SD0,
	},
#endif
	[TCC_MMC_TYPE_SD] = {
		.slot	= 1,
		.caps	= MMC_CAP_SDIO_IRQ | MMC_CAP_4_BIT_DATA
//        | MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED
		,
		.f_min = 100000,
		.f_max = 48000000,        
		.ocr_mask = 
#if defined(CONFIG_MMC_TCC_SD30_SDR12) || defined(CONFIG_MMC_TCC_SD30_SDR25) || defined(CONFIG_MMC_TCC_SD30_SDR50)
        MMC_VDD_165_195 |
#endif
        MMC_VDD_32_33 | MMC_VDD_33_34,
		.init	= tcc8930_mmc_init,
		.card_detect = tcc8930_mmc_card_detect,
		.cd_int_config = tcc8930_mmc_cd_int_config,
		.suspend = tcc8930_mmc_suspend,
		.resume	= tcc8930_mmc_resume,
		.set_power = tcc8930_mmc_set_power,
		.set_bus_width = tcc8930_mmc_set_bus_width,
		.switch_voltage = tcc8930_mmc_switch_voltage,
		.cd_int_num = HwINT0_EI5,
		.cd_irq_num = INT_EINT5,
		.cd_ext_irq = EXTINT_GPIOG_12,
		.peri_name = PERI_SDMMC0,
		.io_name = RB_SDMMC0CONTROLLER,
		.pic = HwINT1_SD1,
	},
	[TCC_MMC_TYPE_WIFI] = {
		.slot	= 2,
		.caps	= MMC_CAP_SDIO_IRQ | MMC_CAP_4_BIT_DATA
		| MMC_CAP_BUS_WIDTH_TEST
/*        | MMC_CAP_NONREMOVABLE */ /* WIF do not use this option */ 
        | MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED,
		.f_min	= 100000,
		.f_max	= 40000000,	/* support highspeed mode */
		.ocr_mask = MMC_VDD_32_33 | MMC_VDD_33_34,
		.init	= tcc8930_mmc_init,
		.card_detect = tcc8930_mmc_card_detect,
		.cd_int_config = tcc8930_mmc_cd_int_config,
		.suspend = tcc8930_mmc_suspend,
		.resume	= tcc8930_mmc_resume,
		.set_power = tcc8930_mmc_set_power,
		.set_bus_width = tcc8930_mmc_set_bus_width,
		.cd_int_num = -1,
		.cd_irq_num = -1,
		.cd_ext_irq = -1,
		.peri_name = PERI_SDMMC2,
		.io_name = RB_SDMMC2CONTROLLER,
		.pic = HwINT1_SD2,
	},
};

static void tcc8930_mmc_port_setup(void)
{
#if 0
	/*
	 * 0x1000 : TCC8930_D3_08X4_SV_0.1 - DDR3 1024(32Bit)
	 * 0x2000 : TCC8935_D3_08X4_2CS_SV0.2 - DDR3 1024MB(16Bit)
	 */
	if (system_rev == 0x2000 || system_rev == 0x3000)
	{
#if defined(CONFIG_MMC_TCC_SUPPORT_EMMC)		// [0]:eMMC,   [1]:SD,   [2]:WiFi
		mmc_ports[TCC_MMC_TYPE_EMMC].data0 = TCC_GPD(29);
		mmc_ports[TCC_MMC_TYPE_EMMC].data1 = TCC_GPD(28);
		mmc_ports[TCC_MMC_TYPE_EMMC].data2 = TCC_GPD(27);
		mmc_ports[TCC_MMC_TYPE_EMMC].data3 = TCC_GPD(26);
		mmc_ports[TCC_MMC_TYPE_EMMC].data4 = TCC_GPD(25);
		mmc_ports[TCC_MMC_TYPE_EMMC].data5 = TCC_GPD(24);
		mmc_ports[TCC_MMC_TYPE_EMMC].data6 = TCC_GPD(23);
		mmc_ports[TCC_MMC_TYPE_EMMC].data7 = TCC_GPD(22);
		mmc_ports[TCC_MMC_TYPE_EMMC].cmd = TCC_GPD(30);
		mmc_ports[TCC_MMC_TYPE_EMMC].clk = TCC_GPD(31);
		mmc_ports[TCC_MMC_TYPE_EMMC].func = GPIO_FN(2);
#endif

		mmc_ports[TCC_MMC_TYPE_SD].data0 = TCC_GPF(26);
		mmc_ports[TCC_MMC_TYPE_SD].data1 = TCC_GPF(25);
		mmc_ports[TCC_MMC_TYPE_SD].data2 = TCC_GPF(24);
		mmc_ports[TCC_MMC_TYPE_SD].data3 = TCC_GPF(23);
		mmc_ports[TCC_MMC_TYPE_SD].cmd = TCC_GPF(27);
		mmc_ports[TCC_MMC_TYPE_SD].clk = TCC_GPF(28);
		mmc_ports[TCC_MMC_TYPE_SD].cd = TCC_GPE(28);
		mmc_ports[TCC_MMC_TYPE_SD].func = GPIO_FN(3);
	}
#endif
}

static int __init tcc8930_init_mmc(void)
{
//	if (!machine_is_tcc893x())
//		return 0;

	tccUsedSDportNum = TCC_MMC_TYPE_MAX;

	tcc8930_mmc_port_setup();
	tcc_init_sdhc_devices();

#if defined(CONFIG_MMC_TCC_SDHC0)
	if (tccUsedSDportNum > 0)
	{
		tcc_sdhc0_device.dev.platform_data = &tcc8930_mmc_platform_data[0];
		platform_device_register(&tcc_sdhc0_device);
	}
#endif

#if defined(CONFIG_MMC_TCC_SDHC1)
	if (tccUsedSDportNum > 1)
	{
		tcc_sdhc1_device.dev.platform_data = &tcc8930_mmc_platform_data[1];
		platform_device_register(&tcc_sdhc1_device);
	}
#endif
#if defined(CONFIG_MMC_TCC_SDHC2)
	if (tccUsedSDportNum > 2)
	{
		tcc_sdhc2_device.dev.platform_data = &tcc8930_mmc_platform_data[2];
		platform_device_register(&tcc_sdhc2_device);
	}
#endif 
#if defined(CONFIG_MMC_TCC_SDHC3)
	if (tccUsedSDportNum > 3)
	{
		tcc_sdhc3_device.dev.platform_data = &tcc8930_mmc_platform_data[3];
		platform_device_register(&tcc_sdhc3_device);
	}
#endif

#if defined(CONFIG_REGULATOR)
    ldo_sd30 = regulator_get(NULL, "vdd_ldo11_sd30_3v3");
    if (IS_ERR(ldo_sd30)) {
        pr_warning("cpufreq: failed to obtain vdd_cpu\n");
        ldo_sd30 = NULL;
    }
#endif

	return 0;
}
device_initcall(tcc8930_init_mmc);
#endif

