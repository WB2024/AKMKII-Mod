/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2009-2010 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for a default platform
 */
#include "mali_kernel_common.h"
#include "mali_osk.h"
#include "mali_platform.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/irq.h>

#include <linux/regulator/driver.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/pm.h>
#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif
#include <linux/mali/mali_utgard.h>
#include "mali_kernel_common.h"
#include "arch/config.h"

static struct clk *mali_clk = NULL;

#if defined(CONFIG_TCC_CLOCK_TABLE)
extern struct tcc_freq_table_t gtMaliClockLimitTable[];
#endif
typedef enum	{
	MALI_CLK_NONE=0,
	MALI_CLK_ENABLED,
	MALI_CLK_DISABLED
} t_mali_clk_type;
static t_mali_clk_type mali_clk_enable = MALI_CLK_NONE;

#if defined(CONFIG_GPU_BUS_SCALING)
extern mali_bool init_mali_dvfs_staus(int step);
extern void deinit_mali_dvfs_staus(void);
extern mali_bool mali_dvfs_handler(u32 utilization);
#endif

static void mali_platform_device_release(struct device *device);
static int mali_os_suspend(struct device *device);
static int mali_os_resume(struct device *device);
static int mali_os_freeze(struct device *device);
static int mali_os_thaw(struct device *device);
#ifdef CONFIG_PM_RUNTIME
static int mali_runtime_suspend(struct device *device);
static int mali_runtime_resume(struct device *device);
static int mali_runtime_idle(struct device *device);
#endif

/*
 * [downer] A140108 core volage control
 */
#define CORE_VOLTAGE1	 1175000
#define CORE_VOLTAGE2	 1000000
#define MEM_VOLTAGE1    1350000
#define MEM_VOLTAGE2    1300000
#define VOLTAGE_CHANGE_DELAY		msecs_to_jiffies(2000*1)  // 2sec

#ifdef CONFIG_REGULATOR
static struct regulator *vdd_core, *vdd_mem; 
#endif
static int core_voltage, mem_voltage;

extern int is_emmc_hs_mode(void); /* [downer] A150930 */

static struct resource mali_gpu_resources[] =
{
#if defined(CONFIG_ARCH_TCC893X)
	MALI_GPU_RESOURCES_MALI400_MP2(0x70000000, _IRQ_3DGP_, _IRQ_3DGPMMU_, _IRQ_3DPP0_, _IRQ_3DPP0MMU_, _IRQ_3DPP1_, _IRQ_3DPP1MMU_)
#else
	MALI_GPU_RESOURCES_MALI400_MP1(0x70000000, _IRQ_3DGP_, _IRQ_3DGPMMU_, _IRQ_3DPP0_, _IRQ_3DPP0MMU_)

#endif
};

static struct dev_pm_ops mali_gpu_device_type_pm_ops =
{
	.suspend = mali_os_suspend,
	.resume = mali_os_resume,
	.freeze = mali_os_freeze,
	.thaw = mali_os_thaw,
        .restore = mali_os_thaw,
#ifdef CONFIG_PM_RUNTIME
	.runtime_suspend = mali_runtime_suspend,
	.runtime_resume = mali_runtime_resume,
	.runtime_idle = mali_runtime_idle,
#endif
};

static struct device_type mali_gpu_device_device_type =
{
	.pm = &mali_gpu_device_type_pm_ops,
};

static struct platform_device mali_gpu_device =
{
	.name = MALI_GPU_NAME_UTGARD,
	.id = 0,
	.dev.release = mali_platform_device_release,
	/*
	 * We temporarily make use of a device type so that we can control the Mali power
	 * from within the mali.ko (since the default platform bus implementation will not do that).
	 * Ideally .dev.pm_domain should be used instead, as this is the new framework designed
	 * to control the power of devices.
	 */
	.dev.type = &mali_gpu_device_device_type, /* We should probably use the pm_domain instead of type on newer kernels */
};

static struct mali_gpu_device_data mali_gpu_data =
{
#if 0
	/* Dedicated memory setup (not sure if this is actually reserved on the platforms any more) */
	.dedicated_mem_start = 0x48A00000, /* Physical start address */
	.dedicated_mem_size = 0x07800000, /* 120MB */
#endif
	.shared_mem_size = CONFIG_MALI_MEMORY_SIZE * 1024UL * 1024UL,
/*
	.fb_start = 0x48200000,
	.fb_size = 0x00800000,
*/

#if USING_GPU_UTILIZATION
	.utilization_interval = 300,
	.utilization_handler = mali_gpu_utilization_handler,
#endif
};

/*
 * [downer] A140318
 */
#if defined(CONFIG_MEM_BUS_SCALING)
extern void set_cpu_mem_down(void);
extern void set_cpu_mem_up(void);
extern int get_xmos_is_playing(void);
#endif

extern int get_wifi_state(void);
extern int get_current_pcfi_state(void);

static int core_voltage_work_func(struct work_struct *ignored)
{
	int ret = 0;

	if (get_wifi_state()) {
		printk("%s return(wifi on)!\n", __func__);
		return 0;		
	}

#if defined(CONFIG_MEM_BUS_SCALING)
    if (is_emmc_hs_mode())      /* [downer] A150930 for eMMC HS */
        return 0;

	#ifdef NO_MEMCLK_CHG_PCFI
	if(get_current_pcfi_state()) {
		CPRINT("PCFI bypass 3\n");
		return 0;
	}	
	#endif
	
	set_cpu_mem_down();

    ret = regulator_set_voltage(vdd_core, CORE_VOLTAGE2, CORE_VOLTAGE2);
	core_voltage = regulator_get_voltage(vdd_core);

    if (!get_xmos_is_playing()) 
        ret = regulator_set_voltage(vdd_mem, MEM_VOLTAGE2, MEM_VOLTAGE2);
    
	mem_voltage = regulator_get_voltage(vdd_mem);

    printk("<<< core(%dmV) / ddr(%dmV)\n", core_voltage / 1000, mem_voltage / 1000);
#endif
    
    return ret;
}
static DECLARE_DELAYED_WORK(core_voltage_work, core_voltage_work_func); 

int mali_platform_device_register(void)
{
	int err;

	MALI_DEBUG_PRINT(4, ("mali_platform_device_register() called\n"));

	/* Connect resources to the device */
	err = platform_device_add_resources(&mali_gpu_device, mali_gpu_resources, sizeof(mali_gpu_resources) / sizeof(mali_gpu_resources[0]));
	if (0 == err)
	{
		err = platform_device_add_data(&mali_gpu_device, &mali_gpu_data, sizeof(mali_gpu_data));
		if (0 == err)
		{
			/* Register the platform device */
			err = platform_device_register(&mali_gpu_device);
			if (0 == err)
			{
				mali_platform_init();

#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
				pm_runtime_set_autosuspend_delay(&(mali_gpu_device.dev), 1000);
				pm_runtime_use_autosuspend(&(mali_gpu_device.dev));
#endif
				pm_runtime_enable(&(mali_gpu_device.dev));
#endif

				return 0;
			}
		}

		platform_device_unregister(&mali_gpu_device);
	}

	return err;
}

void mali_platform_device_unregister(void)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_unregister() called\n"));

	mali_platform_deinit();

	platform_device_unregister(&mali_gpu_device);
}

static void mali_platform_device_release(struct device *device)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_release() called\n"));
}

_mali_osk_errcode_t mali_platform_init(void)
{
#if defined(CONFIG_GPU_BUS_SCALING)
	if(!init_mali_dvfs_staus(0))
		MALI_DEBUG_PRINT(1, ("mali_platform_init failed\n"));        
#endif

	if(mali_clk_enable != MALI_CLK_ENABLED)
	{
		MALI_DEBUG_PRINT(4, ("mali_platform_init() clk_enable\n"));
		if (mali_clk == NULL)
			mali_clk = clk_get(NULL, "mali_clk");	
		clk_enable(mali_clk);
#if defined(CONFIG_TCC_CLOCK_TABLE)
		tcc_cpufreq_set_limit_table(&gtMaliClockLimitTable[0], TCC_FREQ_LIMIT_MALI, 1);
#endif
		mali_clk_enable = MALI_CLK_ENABLED;
	}

    /*
     * core voltage
     */
#ifdef CORE_VOLTAGE_CHANGE	
	if (vdd_core == NULL) {
		vdd_core = regulator_get(NULL, "vdd_core");
		if (IS_ERR(vdd_core)) {
			pr_warning("failed to obtain vdd_core\n");
			vdd_core = NULL;
		}
	}

	if (vdd_mem == NULL) {
		vdd_mem = regulator_get(NULL, "vdd_mem");
		if (IS_ERR(vdd_mem)) {
			pr_warning("failed to obtain vdd_mem\n");
			vdd_mem = NULL;
		}
	}
    
#else
	vdd_core = NULL;
    vdd_mem = NULL;
#endif
    
    MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_deinit(void)
{
	if(mali_clk_enable == MALI_CLK_ENABLED)
	{
		MALI_DEBUG_PRINT(4, ("mali_platform_deinit() clk_disable\n"));
		if (mali_clk == NULL)
			mali_clk = clk_get(NULL, "mali_clk");	
#if defined(CONFIG_TCC_CLOCK_TABLE)
		tcc_cpufreq_set_limit_table(&gtMaliClockLimitTable[0], TCC_FREQ_LIMIT_MALI, 0);
#endif
		clk_disable(mali_clk);
		mali_clk_enable = MALI_CLK_DISABLED;

	}

#if defined(CONFIG_GPU_BUS_SCALING)
	deinit_mali_dvfs_staus();
#endif
    MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_powerdown(u32 cores)
{
	if(mali_clk_enable == MALI_CLK_ENABLED)
	{
		MALI_DEBUG_PRINT(4, ("mali_platform_powerdown() clk_disable\n"));
		if (mali_clk == NULL)
			mali_clk = clk_get(NULL, "mali_clk");	
#if defined(CONFIG_TCC_CLOCK_TABLE)
		tcc_cpufreq_set_limit_table(&gtMaliClockLimitTable[0], TCC_FREQ_LIMIT_MALI, 0);
#endif
		clk_disable(mali_clk);
		mali_clk_enable = MALI_CLK_DISABLED;

	}
    MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_powerup(u32 cores)
{
	if(mali_clk_enable != MALI_CLK_ENABLED)
	{
		MALI_DEBUG_PRINT(4, ("mali_platform_powerup() clk_enable\n"));
		if (mali_clk == NULL)
			mali_clk = clk_get(NULL, "mali_clk");	
		clk_enable(mali_clk);
#if defined(CONFIG_TCC_CLOCK_TABLE)
		tcc_cpufreq_set_limit_table(&gtMaliClockLimitTable[0], TCC_FREQ_LIMIT_MALI, 1);
#endif
		mali_clk_enable = MALI_CLK_ENABLED;

	}
    MALI_SUCCESS;
}

extern int get_lcd_state(void);
extern int get_current_pcfi_state(void);

_mali_osk_errcode_t mali_platform_power_mode_change(mali_power_mode power_mode)
{
    int ret = 0;
    
	if(power_mode == MALI_POWER_MODE_ON)
	{
		if(mali_clk_enable != MALI_CLK_ENABLED)
		{
#if defined(CONFIG_MEM_BUS_SCALING)            
            if (vdd_core && vdd_mem) {
				
			#ifdef NO_MEMCLK_CHG_PCFI
			if(get_current_pcfi_state()) {			
				CPRINT("PCFI bypass 5\n");
			} else 
			#endif
				{
	                cancel_delayed_work_sync(&core_voltage_work);

	                core_voltage = regulator_get_voltage(vdd_core);
	                if (core_voltage != CORE_VOLTAGE1) {
	                    ret = regulator_set_voltage(vdd_core, CORE_VOLTAGE1, CORE_VOLTAGE1); 
	                    core_voltage = regulator_get_voltage(vdd_core);
	                    printk(">>> core(%dmV) ", core_voltage / 1000);
	                }

	                mem_voltage = regulator_get_voltage(vdd_mem);
	                if (mem_voltage != MEM_VOLTAGE1) {
	                    ret = regulator_set_voltage(vdd_mem, MEM_VOLTAGE1, MEM_VOLTAGE1); 
	                    mem_voltage = regulator_get_voltage(vdd_mem);
	                    printk("/ ddr(%dmV)\n", mem_voltage / 1000);
	                }

	                set_cpu_mem_up();
				}
            }
#endif            
			MALI_DEBUG_PRINT(4, ("mali_platform_power_mode_change() clk_enable\n"));
			if (mali_clk == NULL)
				mali_clk = clk_get(NULL, "mali_clk");	
			clk_enable(mali_clk);
#if defined(CONFIG_TCC_CLOCK_TABLE)
			tcc_cpufreq_set_limit_table(&gtMaliClockLimitTable[0], TCC_FREQ_LIMIT_MALI, 1);
#endif
			mali_clk_enable = MALI_CLK_ENABLED;
		}
	}
	else
	{
		if( mali_clk_enable == MALI_CLK_ENABLED)
		{
			MALI_DEBUG_PRINT(4, ("mali_platform_power_mode_change() clk_disable\n"));
			if (mali_clk == NULL)
				mali_clk = clk_get(NULL, "mali_clk");	
#if defined(CONFIG_TCC_CLOCK_TABLE)
			tcc_cpufreq_set_limit_table(&gtMaliClockLimitTable[0], TCC_FREQ_LIMIT_MALI, 0);
#endif
			clk_disable(mali_clk);
			mali_clk_enable = MALI_CLK_DISABLED;

            if (vdd_core && vdd_mem && !get_lcd_state()) {
                schedule_delayed_work(&core_voltage_work, VOLTAGE_CHANGE_DELAY);
            }
		}
	}
    MALI_SUCCESS;
}

void mali_gpu_utilization_handler(u32 utilization)
{
#if defined(CONFIG_GPU_BUS_SCALING)
	if(mali_clk_enable == MALI_CLK_ENABLED)
	{
		//printk("%s: utilization:%d\n", __func__,  utilization);
		if(!mali_dvfs_handler(utilization))
			MALI_DEBUG_PRINT(1,( "error on mali dvfs status in utilization\n"));
	}
#endif
}

void set_mali_parent_power_domain(void* dev)
{
}
static int mali_os_suspend(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_os_suspend() called\n"));

#ifdef ENABLE_DDR_600MHz  
	printk("mali_os_suspend\n");

	ret = regulator_set_voltage(vdd_core, CORE_VOLTAGE1, CORE_VOLTAGE1); 
    core_voltage = regulator_get_voltage(vdd_core);
	printk("+++ core(%dmV) ", core_voltage / 1000);

  	ret = regulator_set_voltage(vdd_mem, MEM_VOLTAGE1, MEM_VOLTAGE1); 
    mem_voltage = regulator_get_voltage(vdd_mem);
	printk("/ ddr(%dmV)\n", mem_voltage / 1000);
#endif


	if (NULL != device->driver &&
	    NULL != device->driver->pm &&
	    NULL != device->driver->pm->suspend)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->suspend(device);
	}

	mali_platform_power_mode_change(MALI_POWER_MODE_DEEP_SLEEP);

	return ret;
}

static int mali_os_resume(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_os_resume() called\n"));

	mali_platform_power_mode_change(MALI_POWER_MODE_ON);

	if (NULL != device->driver &&
	    NULL != device->driver->pm &&
	    NULL != device->driver->pm->resume)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->resume(device);
	}

	return ret;
}

static int mali_os_freeze(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_os_freeze() called\n"));

	if (NULL != device->driver &&
	    NULL != device->driver->pm &&
	    NULL != device->driver->pm->freeze)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->freeze(device);
	}

	return ret;
}

static int mali_os_thaw(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_os_thaw() called\n"));

	if (NULL != device->driver &&
	    NULL != device->driver->pm &&
	    NULL != device->driver->pm->thaw)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->thaw(device);
	}

	return ret;
}

#ifdef CONFIG_PM_RUNTIME
static int mali_runtime_suspend(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_runtime_suspend() called\n"));    
    
	if (NULL != device->driver &&
	    NULL != device->driver->pm &&
	    NULL != device->driver->pm->runtime_suspend)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->runtime_suspend(device);
	}

	mali_platform_power_mode_change(MALI_POWER_MODE_LIGHT_SLEEP);

	return ret;
}

static int mali_runtime_resume(struct device *device)
{
	int ret = 0;

	MALI_DEBUG_PRINT(4, ("mali_runtime_resume() called\n"));        

	mali_platform_power_mode_change(MALI_POWER_MODE_ON);

	if (NULL != device->driver &&
	    NULL != device->driver->pm &&
	    NULL != device->driver->pm->runtime_resume)
	{
		/* Need to notify Mali driver about this event */
		ret = device->driver->pm->runtime_resume(device);
	}

	return ret;
}

static int mali_runtime_idle(struct device *device)
{
	MALI_DEBUG_PRINT(4, ("mali_runtime_idle() called\n"));

	if (NULL != device->driver &&
	    NULL != device->driver->pm &&
	    NULL != device->driver->pm->runtime_idle)
	{
		/* Need to notify Mali driver about this event */
		int ret = device->driver->pm->runtime_idle(device);
		if (0 != ret)
		{
			return ret;
		}
	}

	pm_runtime_suspend(device);

	return 0;
}
#endif
