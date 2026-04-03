/*
 * arch/arm/mach-tcc893x/cpufreq.c
 *
 * TCC893x cpufreq driver
 *
 * Copyright (C) 2012 Telechips, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/suspend.h>
#include <linux/debugfs.h>
#include <linux/cpu.h>

#include <asm/system.h>
#include <asm/mach-types.h>

#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>

#include <mach/bsp.h>
#include <mach/gpio.h>
#include <mach/board_astell_kern.h>

#if defined(CONFIG_AUTO_HOTPLUG)
#include "tcc_cpufreq.h"
#endif

static int g_fix_clock=0;
extern int get_boot_complete(void);
extern int ak_is_connected_optical_tx(void);

//#define RESUME_CLOCK_LIMIT
#ifdef USE_DYNAMIC_CHARGING_CURRENT
extern INTERNAL_PLAY_MODE ak_get_internal_play_mode(void);
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_MEM_BUS_SCALING)
#include <linux/earlysuspend.h>
struct early_suspend tcc_cpufreq_early_suspend;
static struct clk *mem_clk;
#endif

//iriver-ysyim: enable dvfs clock after 40 sec(default booting clock 850Mhz)
#define DVFS_CLOCK_DELAY		msecs_to_jiffies(1000*50) //90 sec  
struct delayed_work dvfs_clock_work;
static int disable_dvfs_clock=0;

struct tcc_cpufreq_table_t {
	unsigned int cpu_freq;
	unsigned int voltage;
};

#if defined(CONFIG_TCC_CORE_VOLTAGE_OFFSET) && defined(CONFIG_REGULATOR)
	#define CORE_VOLTAGE_OFFSET	((CONFIG_TCC_CORE_VOLTAGE_OFFSET * 1000) + 25000) // ak500n:1.245v, ak240:1.225v   [VDD_CPU (PWRCPU) - MIN :1.22v, TYP :1.28, MAX:1.35]
#else
	#define CORE_VOLTAGE_OFFSET	(0 + 25000)
#endif

#define CORE_VOLTAGE_WIFI_ON_OFFSET	75000

extern int get_xmos_is_playing(void);
extern int get_xmos_cur_sample_rate(void);

static struct tcc_cpufreq_table_t tcc_cpufreq_table[] = {
	/*  CPU    Voltage */
	//{   70000,  925000+CORE_VOLTAGE_OFFSET },
	//{ 101000,  925000+CORE_VOLTAGE_OFFSET },
	//{ 146000,  925000+CORE_VOLTAGE_OFFSET },
        //{ 210000,  925000+CORE_VOLTAGE_OFFSET },
#if (CUR_AK >= MODEL_AK240)
    //	{ 101000,  975000+CORE_VOLTAGE_OFFSET },
//	{ 146000,  975000+CORE_VOLTAGE_OFFSET },
//	{ 210000,  975000+CORE_VOLTAGE_OFFSET },
//	{ 342000,  975000+CORE_VOLTAGE_OFFSET },
	{ 424000,  975000+CORE_VOLTAGE_OFFSET },
#else
	{ 342000,  925000+CORE_VOLTAGE_OFFSET },
	{ 424000,  975000+CORE_VOLTAGE_OFFSET },
#endif
	{ 524000, 1000000+CORE_VOLTAGE_OFFSET },
	{ 626000, 1050000+CORE_VOLTAGE_OFFSET },
//	{ 728000, 1100000+CORE_VOLTAGE_OFFSET },
#if (CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK >= 850)
	{ 850000, 1150000+CORE_VOLTAGE_OFFSET },
#endif
#if (CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK >= 910)
	{ 910000, 1200000+CORE_VOLTAGE_OFFSET },
#endif
#if (CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK >= 1000)
	{1000000, 1200000+CORE_VOLTAGE_OFFSET },
#endif
    //////////////////////////////////////////////
#if (0)
#if (CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK >= 1060)
	{1060000, 1300000+CORE_VOLTAGE_OFFSET },
#endif
#if (CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK >= 1112)
	{1112000, 1350000+CORE_VOLTAGE_OFFSET },
#endif
#if (CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK >= 1152)
	{1152000, 1400000+CORE_VOLTAGE_OFFSET },
#endif
#endif
    //////////////////////////////////////////////
#if (CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK >= 1200)
	{1200000, 1450000+(CORE_VOLTAGE_OFFSET-25000) },
#endif

};

#define NUM_FREQS	ARRAY_SIZE(tcc_cpufreq_table)

static struct cpufreq_frequency_table freq_table_opp[NUM_FREQS + 1];
static struct cpufreq_frequency_table *freq_table;
static struct clk *cpu_clk;

#if defined(CONFIG_IOBUS_DFS)
static struct clk *max_io_clk;
static int max_io_enabled = 0;
#endif

static unsigned long policy_max_speed[CONFIG_NR_CPUS];
static unsigned long target_cpu_speed[CONFIG_NR_CPUS];
static DEFINE_MUTEX(tcc_cpu_lock);

static bool is_suspended;
static int suspend_index;
static bool force_policy_max;

#ifdef CONFIG_REGULATOR
static struct regulator *vdd_cpu;
#endif
static unsigned int curr_voltage;

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_MEM_BUS_SCALING)
static int g_down_flag = 0;
static int g_down_memclk = 0;
#endif

int tcc_update_cpu_speed(unsigned long rate);
unsigned long tcc_cpu_highest_speed(void);

#if defined(RESUME_CLOCK_LIMIT)
static struct timer_list timer_resume_clock;
static int resume_clock_limit_flag = 0;
#endif

#ifdef USE_DYNAMIC_CHARGING_CURRENT 
//jimi.test.1220
int g_charge_current_sw = -1; //-1:init 0:Low, 1:High
extern int tcc_dwc3_vbus_ctrl(int on);
extern void tcc_usb30_phy_off(void);
extern void tcc_usb30_phy_on(void);
extern int s6e63m0x01_get_power_state(void);
#endif
#if (CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
extern int get_adapter_state(void);
#endif

#if defined(CONFIG_HIGHSPEED_TIME_LIMIT)

#if (CONFIG_HIGHSPEED_LIMITED_CLOCK >= CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK)
	#error "Time Limited Clock is higher than HighSpeed Clock"
#endif

#define DEBUG_HIGHSPEED 0
#define dbg_highspeed(msg...)	if (DEBUG_HIGHSPEED) { printk( "TCC_HIGHSPEED: " msg); }

#define TCC_CPU_FREQ_HIGH_SPEED         (CONFIG_TCC_CPUFREQ_HIGHSPEED_CLOCK*1000)
#define TCC_CPU_FREQ_LIMITED_SPEED      (CONFIG_HIGHSPEED_LIMITED_CLOCK*1000)

enum cpu_highspeed_status_t {
	CPU_FREQ_PROCESSING_NORMAL = 0,
	CPU_FREQ_PROCESSING_LIMIT_HIGHSPEED,
	CPU_FREQ_PROCESSING_HIGHSPEED,
	CPU_FREQ_PROCESSING_MAX
};

#define HIGHSPEED_TIMER_TICK             100                                // 100 ms.
#define HIGHSPEED_LIMIT_CLOCK_PERIOD     ((CONFIG_HIGHSPEED_LIMITED_TIME*1000)/HIGHSPEED_TIMER_TICK)
#define HIGHSPEED_HIGER_CLOCK_PERIOD     ((CONFIG_HIGHSPEED_HIGHSPEED_TIME*1000)/HIGHSPEED_TIMER_TICK)

static int highspeed_highspeed_mode = 0;
static int highspeed_highclock_counter = 0;
static int highspeed_lowclock_counter = 0;
static int highspeed_limit_highspeed_counter = 0;

static struct workqueue_struct *cpufreq_wq;
static struct work_struct cpufreq_work;
static enum cpu_highspeed_status_t cpu_highspeed_status = CPU_FREQ_PROCESSING_NORMAL;
static struct timer_list timer_highspeed;


static int tcc_cpufreq_is_limit_highspeed_status(void)
{
	if ( 0
	  || cpu_highspeed_status == CPU_FREQ_PROCESSING_LIMIT_HIGHSPEED
	) {
		return 1;
	}
	
	return 0;
}

static void cpufreq_work_func(struct work_struct *work)
{
	dbg_highspeed("######### %s\n", __func__);

	mutex_lock(&tcc_cpu_lock);

	if (cpu_highspeed_status == CPU_FREQ_PROCESSING_LIMIT_HIGHSPEED) {
		tcc_update_cpu_speed(TCC_CPU_FREQ_LIMITED_SPEED);
	}
	else if(cpu_highspeed_status == CPU_FREQ_PROCESSING_HIGHSPEED) {
	 	if (tcc_cpufreq_is_limit_highspeed_status())
			tcc_update_cpu_speed(TCC_CPU_FREQ_LIMITED_SPEED);
		else
			tcc_update_cpu_speed(TCC_CPU_FREQ_HIGH_SPEED);
	}

	mutex_unlock(&tcc_cpu_lock);
}

static void highspeed_reset_setting_values(void)
{
 	highspeed_highspeed_mode = 0;
	highspeed_highclock_counter = 0;
	highspeed_lowclock_counter = 0;
	highspeed_limit_highspeed_counter = 0;
	cpu_highspeed_status = CPU_FREQ_PROCESSING_NORMAL;
}

static void highspeed_timer_func(unsigned long data)
{
	int status_changed = 0;
	unsigned int noraml_speed_clock;

	noraml_speed_clock = TCC_CPU_FREQ_LIMITED_SPEED;

	// increase counters
	if (cpu_highspeed_status == CPU_FREQ_PROCESSING_LIMIT_HIGHSPEED)
		highspeed_limit_highspeed_counter++;
 	else if (highspeed_highspeed_mode) {
		if (tcc_cpu_highest_speed() > noraml_speed_clock)
			highspeed_highclock_counter++;
		else
			highspeed_lowclock_counter++;
 	}

	// start highspeed process
	if (cpu_highspeed_status == CPU_FREQ_PROCESSING_NORMAL && tcc_cpu_highest_speed() > noraml_speed_clock) {
		cpu_highspeed_status = CPU_FREQ_PROCESSING_HIGHSPEED;
		highspeed_highspeed_mode = 1;
		highspeed_highclock_counter = 0;
		highspeed_lowclock_counter = 0;
		dbg_highspeed("######### Change to highspeed status\n");
		status_changed = 1;
	}

	// during over clock status
	else if (cpu_highspeed_status == CPU_FREQ_PROCESSING_HIGHSPEED) {
		if (highspeed_highclock_counter >= HIGHSPEED_HIGER_CLOCK_PERIOD && highspeed_lowclock_counter < HIGHSPEED_HIGER_CLOCK_PERIOD) {
			cpu_highspeed_status = CPU_FREQ_PROCESSING_LIMIT_HIGHSPEED;
			highspeed_limit_highspeed_counter = highspeed_lowclock_counter;	// optional.  add lowclock_counter or not.
			highspeed_highclock_counter = 0;
			highspeed_lowclock_counter = 0;
			dbg_highspeed("######### Change to limit highspeed status\n");
			status_changed = 1;
		}
		else if (highspeed_lowclock_counter >= HIGHSPEED_HIGER_CLOCK_PERIOD) {
			highspeed_reset_setting_values();
			dbg_highspeed("######### Change to normal status\n");
			status_changed = 1;
 		}
	}

	// during limit over clock status
	else if (cpu_highspeed_status == CPU_FREQ_PROCESSING_LIMIT_HIGHSPEED) {
		if (highspeed_limit_highspeed_counter >= HIGHSPEED_LIMIT_CLOCK_PERIOD) {
			highspeed_reset_setting_values();
			dbg_highspeed("######### Change to normal status\n");
			status_changed = 1;
		}
 	}

	// unkown status
	else if (cpu_highspeed_status != CPU_FREQ_PROCESSING_NORMAL) {
		highspeed_reset_setting_values();
		dbg_highspeed("######### Change to normal status (dummy)\n");
		status_changed = 1;
	}

	if (status_changed)
		queue_work(cpufreq_wq, &cpufreq_work);

    timer_highspeed.expires = jiffies + msecs_to_jiffies(HIGHSPEED_TIMER_TICK);	// 100 milisec.
    add_timer(&timer_highspeed);
}
#endif /* CONFIG_HIGHSPEED_TIME_LIMIT */

static unsigned int tcc_get_voltage(unsigned int freq)
{
	int i;
	for (i=0 ; i<NUM_FREQS ; i++) {
		if (freq <= tcc_cpufreq_table[i].cpu_freq)
			return tcc_cpufreq_table[i].voltage;			
	}
	return tcc_cpufreq_table[NUM_FREQS-1].voltage;
}

#if defined(CONFIG_GPIO_CORE_VOLTAGE_CONTROL)
static unsigned int tcc_cpufreq_set_voltage_by_gpio(int uV)
{
	if(machine_is_tcc8930st()) {
		#if defined(CONFIG_STB_BOARD_EV_TCC893X) || defined(CONFIG_STB_BOARD_YJ8930T)
			if( uV > 1100000 ) {
				//CORE1_ON == 1, CORE2_ON == 1 ==> 1.30V
				gpio_set_value(TCC_GPB(19), 1);
				gpio_set_value(TCC_GPB(21), 1);
			}
			else {
				//CORE1_ON == 0, CORE2_ON == 1 ==> 1.10V
				gpio_set_value(TCC_GPB(19), 0);
				gpio_set_value(TCC_GPB(21), 1);
			}
        #elif defined(CONFIG_STB_BOARD_YJ8933T)			
            if( uV > 1100000 ) {
				//CORE1_ON == 1, CORE2_ON == 1 ==> 1.30V
				gpio_set_value(TCC_GPG(15), 1);
				gpio_set_value(TCC_GPG(16), 1);
			}
			else {
				//CORE1_ON == 0, CORE2_ON == 1 ==> 1.10V
				gpio_set_value(TCC_GPG(15), 0);
				gpio_set_value(TCC_GPG(16), 1);
			}
		#elif defined(CONFIG_STB_BOARD_UPC_TCC893X)
			if( uV > 1100000 ) {
				//CORE0_ON == 1 ==> 1.30V
				gpio_set_value(TCC_GPG(15), 1);
			}
			else {
				//CORE0_ON == 0 ==> 1.10V
				gpio_set_value(TCC_GPG(15), 0);
			}
		#elif defined(CONFIG_STB_BOARD_DONGLE_TCC893X)
			if( uV > 1100000 ) {
				// V0.1 : CORE0_ON == 1 ==> 1.10V
				// V0.2 : CORE1_ON == 1 ==> 1.25V
				gpio_set_value(TCC_GPG(14), 1);
				gpio_set_value(TCC_GPG(15), 1);
			}
			else {
				// V0.1 : CORE0_ON == 0 ==> 1.10V
				// V0.2 : CORE1_ON == 0 ==> 1.05V
				gpio_set_value(TCC_GPG(14), 0);
				gpio_set_value(TCC_GPG(15), 0);
			}
		#elif defined(CONFIG_STB_BOARD_YJ8935T)
			//CORE0_ON == 1 ==> 1.30V
			gpio_set_value(TCC_GPG(14), 1);
		#endif
		udelay(100);
	}

	return 0;
}
#endif


static int tcc_verify_speed(struct cpufreq_policy *policy)
{
	if (!freq_table)
		return -EINVAL;

	return cpufreq_frequency_table_verify(policy, freq_table);
}

unsigned int tcc_getspeed(unsigned int cpu)
{
	if (cpu >= NR_CPUS)
		return 0;

	return clk_get_rate(cpu_clk) / 1000;
}

void fix_cpu_clock(bool val)
{
	if(val == true)
		g_fix_clock = 1;
	else
		g_fix_clock = 0;		
}
EXPORT_SYMBOL(fix_cpu_clock);

/* [downer] A140107 */
extern int dvfs_lock_highspeed(void);
extern int get_lcd_state(void);

extern int get_wifi_state(void);
extern int get_bluetooth_state(void);

#if  (CUR_AK == MODEL_AK500N)
extern int get_cdrom_state(void); //iriver-ysyim
extern int get_cpu_boost(void);   /* [downer] A141106 */
#endif

#if (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII)
extern int get_bt_streaming_state(void);
#endif

int tcc_update_cpu_speed(unsigned long rate)
{
	int ret = 0;
	struct cpufreq_freqs freqs;
	unsigned int new_voltage;

	//rate=524000;
	//rate=342000;
	//rate=210000;	
	//rate=146000;

	freqs.old = tcc_getspeed(0);	

#if defined(CONFIG_HIGHSPEED_TIME_LIMIT)
	freqs.old = tcc_getspeed(0);

	if (rate > TCC_CPU_FREQ_LIMITED_SPEED) {
		if (tcc_cpufreq_is_limit_highspeed_status())
			rate = TCC_CPU_FREQ_LIMITED_SPEED;
	}

#if (DEBUG_HIGHSPEED)
	if (freqs.old > TCC_CPU_FREQ_LIMITED_SPEED && rate <= TCC_CPU_FREQ_LIMITED_SPEED ) {
		dbg_highspeed("change to %dMHz\n", TCC_CPU_FREQ_LIMITED_SPEED/1000);
	}
	else if (freqs.old <= TCC_CPU_FREQ_LIMITED_SPEED && rate > TCC_CPU_FREQ_LIMITED_SPEED ) {
		dbg_highspeed("change to %dMHz\n", TCC_CPU_FREQ_HIGH_SPEED/1000);
	}
#endif
#endif

	//iriver-ysyim
	if (!(get_boot_complete() ^ disable_dvfs_clock)){ //   0^0: 부팅시false  , 1^0: 부팅 완로후 true,  1^1: 파워off시 false
		//printk("no dvfs(%d): %d, %d\n", rate, get_boot_complete(), disable_dvfs_clock);
		rate = 1000000; //default 850Mhz
	}
		
#if 0
	if (get_wifi_state() || get_bluetooth_state())
		new_voltage += CORE_VOLTAGE_WIFI_ON_OFFSET;
#endif

	/* [downer/A170913] for AK70_MKII BT streaming */
// dean 20170920 v1.02	
//#if (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII)
//	if (get_bt_streaming_state())
//		rate = 1000000;
//#endif

	new_voltage = tcc_get_voltage(rate);	
	freqs.new = rate;

	if (curr_voltage < new_voltage) {
#if defined(CONFIG_REGULATOR)
		if (vdd_cpu) {			/* [downer] M160627 */
			regulator_set_voltage(vdd_cpu, new_voltage - CORE_VOLTAGE_OFFSET, new_voltage - CORE_VOLTAGE_OFFSET);
			mdelay(1);
			regulator_set_voltage(vdd_cpu, new_voltage, new_voltage);
		}
		
#elif defined(CONFIG_GPIO_CORE_VOLTAGE_CONTROL)
		tcc_cpufreq_set_voltage_by_gpio(new_voltage);
#endif
	}

#if 0
	if (freqs.old != freqs.new) {
		printk("Freq: %d Voltage: %d\n", rate, new_voltage);
	}
#endif
	
	for_each_online_cpu(freqs.cpu)
		cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	ret = clk_set_rate(cpu_clk, freqs.new * 1000);
	if (ret) {
		pr_err("%s: Failed to set cpu frequency to %d kHz\n", __func__, freqs.new);
		goto failed_cpu_clk_set_rate;
	}

	for_each_online_cpu(freqs.cpu)
		cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

#if defined(CONFIG_IOBUS_DFS)
	if (max_io_clk && !get_xmos_is_playing()) {
		if ((freqs.new > 425000) && (max_io_enabled==0)) {
			clk_enable(max_io_clk);
			max_io_enabled = 1;
		}
		else if ((freqs.new <= 425000) && max_io_enabled) {
			clk_disable(max_io_clk);
			max_io_enabled = 0;
		}
	}
#endif

	if (curr_voltage > new_voltage) {
#if defined(CONFIG_REGULATOR)
		if (vdd_cpu) {			/* [downer] M160627 */
			regulator_set_voltage(vdd_cpu, new_voltage + CORE_VOLTAGE_OFFSET, new_voltage + CORE_VOLTAGE_OFFSET);
			mdelay(1);			
			regulator_set_voltage(vdd_cpu, new_voltage, new_voltage);
		}
		
#elif defined(CONFIG_GPIO_CORE_VOLTAGE_CONTROL)
		tcc_cpufreq_set_voltage_by_gpio(new_voltage);
#endif
	}

	curr_voltage = new_voltage;


#ifdef USE_DYNAMIC_CHARGING_CURRENT
#if (CUR_AK >= MODEL_AK240)	
	//jimi.test.charging_current.1220
	/*
	printk("freqs.old = %d\n\
		freqs.new = %d\n\ 
		get_adapter_state()=%d\n\ 
		g_charge_current_sw = %d\n\ 
		s6e63m0x01_get_power_state() = %d\n",
		freqs.old, freqs.new, get_adapter_state(), g_charge_current_sw, s6e63m0x01_get_power_state());
	*/
	if( get_adapter_state()==1 && ak_get_internal_play_mode() != INTERNAL_DSD_PLAY) { 
#else  
	if(freqs.old != freqs.new ) {
#endif
		if ((freqs.new > 425000 && g_charge_current_sw == 1 && s6e63m0x01_get_power_state() == 1) || g_charge_current_sw == -1) {
		//	printk("+USB ADAPTER STATE ( HIGH CPU CLOCK ) ===========> LOW CHARGING CURRENT\n");
#if 0  // ARRANGE_ME_LATER
#if (CUR_AK_REV <= AK240_HW_TP1 ) 	
			tcc_usb30_phy_off();
			tcc_dwc3_vbus_ctrl(0);

			msleep(100);
#endif
#endif

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
			gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
			tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_DAC_SEL, 1);

		//	msleep(100);

			gpio_request(GPIO_XMOS_USB_EN, "GPIO_XMOS_USB_EN");
			tcc_gpio_config(GPIO_XMOS_USB_EN,GPIO_FN(0));
			gpio_direction_output(GPIO_XMOS_USB_EN, 1);
#elif (CUR_AK > MODEL_AK240)
			gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
			tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_DAC_SEL, 1);

			gpio_request(GPIO_USB_ON, "GPIO_USB_ON");
			tcc_gpio_config(GPIO_USB_ON,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_ON, 0);
#endif

#if 0	 // ARRANGE_ME_LATER
#if (CUR_AK_REV <= AK240_HW_TP1 ) 
			tcc_usb30_phy_off();
			tcc_dwc3_vbus_ctrl(0);
#endif
#endif

		//	printk("-USB ADAPTER STATE ( HIGH CPU CLOCK ) ===========> LOW CHARGING CURRENT\n");

			g_charge_current_sw = 0;

			freqs.old = freqs.new;
		}

		else if ((freqs.new <= 425000 && g_charge_current_sw == 0) || g_charge_current_sw == -1 || 
				(s6e63m0x01_get_power_state() == 0 && g_charge_current_sw != 1)) { 
		//	printk("+USB ADAPTER STATE ( LOW CPU CLOCK ) ===========> HIGH CHARGING CURRENT\n");

#if 0  // ARRANGE_ME_LATER
#if (CUR_AK_REV <= AK240_HW_TP1 )
			tcc_usb30_phy_off();
			tcc_dwc3_vbus_ctrl(0);
#endif
#endif


#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
			gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
			tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_DAC_SEL, 0);

		//	msleep(100);

			gpio_request(GPIO_XMOS_USB_EN, "GPIO_XMOS_USB_EN");
			tcc_gpio_config(GPIO_XMOS_USB_EN,GPIO_FN(0));
			gpio_direction_output(GPIO_XMOS_USB_EN, 1);
#elif (CUR_AK > MODEL_AK240)
			gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
			tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_DAC_SEL, 0);

			gpio_request(GPIO_USB_ON, "GPIO_USB_ON");
			tcc_gpio_config(GPIO_USB_ON,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_ON, 1);
#endif

#if 0   // ARRANGE_ME_LATER
#if (CUR_AK_REV <= AK240_HW_TP1 ) 
			tcc_usb30_phy_off();
			tcc_dwc3_vbus_ctrl(0);
#endif
#endif

		//	printk("-USB ADAPTER STATE ( LOW CPU CLOCK ) ===========> HIGH CHARGING CURRENT\n");

			g_charge_current_sw = 1;

			freqs.old = freqs.new;
		}


	}
	else {
#if (CUR_AK > MODEL_AK240)
		gpio_request(GPIO_USB_ON, "GPIO_USB_ON");
		tcc_gpio_config(GPIO_USB_ON,GPIO_FN(0));
		gpio_direction_output(GPIO_USB_ON, 0);
#endif
		g_charge_current_sw = -1;
	}
#endif //jimi.test.dvfs.1223

	return 0;

failed_cpu_clk_set_rate:
	curr_voltage = new_voltage;
	return ret;
}

unsigned int tcc_count_slow_cpus(unsigned long speed_limit)
{
	unsigned int cnt = 0;
	int i;

	for_each_online_cpu(i)
		if (target_cpu_speed[i] <= speed_limit)
			cnt++;
	return cnt;
}

unsigned int tcc_get_slowest_cpu_n(void) {
	unsigned int cpu = nr_cpu_ids;
	unsigned long rate = ULONG_MAX;
	int i;

	for_each_online_cpu(i)
		if ((i > 0) && (rate > target_cpu_speed[i])) {
			cpu = i;
			rate = target_cpu_speed[i];
		}
	return cpu;
}

unsigned long tcc_cpu_lowest_speed(void) {
	unsigned long rate = ULONG_MAX;
	int i;

	for_each_online_cpu(i)
		rate = min(rate, target_cpu_speed[i]);
	return rate;
}

unsigned long tcc_cpu_highest_speed(void)
{
	unsigned long policy_max = ULONG_MAX;
	unsigned long rate = 0;
	int i;

	for_each_online_cpu(i) {
		if (force_policy_max)
			policy_max = min(policy_max, policy_max_speed[i]);
		rate = max(rate, target_cpu_speed[i]);
	}
	rate = min(rate, policy_max);
	return rate;
}

int tcc_cpu_set_speed_cap(unsigned int *speed_cap)
{
	int ret = 0;
	unsigned int new_speed = tcc_cpu_highest_speed();

	if (is_suspended)
		return -EBUSY;

#if defined(RESUME_CLOCK_LIMIT)
	if (resume_clock_limit_flag && (new_speed > 850000))
		new_speed = 850000;
#endif

	if (speed_cap)
		*speed_cap = new_speed;

	ret = tcc_update_cpu_speed(new_speed);
#if defined(CONFIG_AUTO_HOTPLUG)
	if (ret == 0)
		auto_hotplug_governor(new_speed, false);
#endif
	return ret;
}
EXPORT_SYMBOL(tcc_cpu_set_speed_cap);

static int tcc_target(struct cpufreq_policy *policy,
			      unsigned int target_freq,
			      unsigned int relation)
{
	int idx;
	unsigned int freq;
	unsigned int new_speed;
	int ret = 0;

	mutex_lock(&tcc_cpu_lock);

#if defined(RESUME_CLOCK_LIMIT)
	if (resume_clock_limit_flag && (target_freq > 850000))
		target_freq = 850000;
#endif

	ret = cpufreq_frequency_table_target(policy, freq_table, target_freq,
		relation, &idx);
	if (ret)
		goto _out;

	freq = freq_table[idx].frequency;

	target_cpu_speed[policy->cpu] = freq;
	ret = tcc_cpu_set_speed_cap(&new_speed);
_out:
	mutex_unlock(&tcc_cpu_lock);

	return ret;
}

#if defined(RESUME_CLOCK_LIMIT)
static void resume_clock_timer_func(unsigned long data)
{
	resume_clock_limit_flag = 0;
}
#endif

static int tcc_pm_notify(struct notifier_block *nb, unsigned long event,
	void *dummy)
{
	mutex_lock(&tcc_cpu_lock);
	if (event == PM_SUSPEND_PREPARE) {
#if defined(CONFIG_HIGHSPEED_TIME_LIMIT)
		highspeed_reset_setting_values();
#endif

#if (CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
		//jimi.150507
		if ( get_adapter_state()==1 ) {
			printk("\x1b[1;39m In charging 1.5A \x1b[0m\n");
			tcc_gpio_config(GPIO_CHG_ADP_MODE, GPIO_FN(0));
			gpio_request(GPIO_CHG_ADP_MODE, "GPIO_CHG_ADP_MODE");
			gpio_direction_output(GPIO_CHG_ADP_MODE, 1); //Hihg:AC, Low:USB
		}
#endif


		is_suspended = true;
		pr_info("TCC cpufreq suspend: setting frequency to %d kHz\n",
			freq_table[suspend_index].frequency);
		tcc_update_cpu_speed(freq_table[suspend_index].frequency);
#if defined(CONFIG_AUTO_HOTPLUG)
		auto_hotplug_governor(freq_table[suspend_index].frequency, true);
#endif
#if defined(CONFIG_HIGHSPEED_TIME_LIMIT)
		del_timer(&timer_highspeed);
#endif
#if defined(RESUME_CLOCK_LIMIT)
		del_timer(&timer_resume_clock);
		resume_clock_limit_flag = 0;
#endif
	} else if (event == PM_POST_SUSPEND) {
		unsigned int freq;
		is_suspended = false;
#if (1)
		tcc_cpu_set_speed_cap(&freq);
#else
		/* forced set the resume clock to 850MHz. It has problem on performance mode */
		freq = 850000;
		tcc_update_cpu_speed(freq);
#endif
		pr_info("TCC cpufreq resume: restoring frequency to %d kHz\n",
			freq);

#if defined(CONFIG_HIGHSPEED_TIME_LIMIT)
		timer_highspeed.expires = jiffies + msecs_to_jiffies(HIGHSPEED_TIMER_TICK);	// 100 milisec.
		add_timer(&timer_highspeed);
#endif

#if defined(RESUME_CLOCK_LIMIT)
		resume_clock_limit_flag = 1;
		timer_resume_clock.expires = jiffies + msecs_to_jiffies(2000);	// 2 secs.
		add_timer(&timer_resume_clock);
#endif
	}
	mutex_unlock(&tcc_cpu_lock);

	return NOTIFY_OK;
}

static struct notifier_block tcc_cpu_pm_notifier = {
	.notifier_call = tcc_pm_notify,
};

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_MEM_BUS_SCALING)
extern int get_lcd_state(void);
extern int get_current_pcfi_state(void);

void set_cpu_mem_down(void)
{
	int ret;
	#ifdef NO_MEMCLK_CHG_PCFI
	if(get_current_pcfi_state()) {
		CPRINT("PCFI bypass 1\n");
		return;
	}
	#endif
	
    if (!g_down_flag) {
    	ret = cpu_down(1);
#ifdef ENABLE_DDR_600MHz                            /* [downer] A140609 */
        if (!ret && mem_clk && !ak_is_connected_optical_tx() && !get_xmos_is_playing() && !get_lcd_state()) {
            clk_set_rate(mem_clk, 300*1000*1000);
            g_down_memclk = 1;
        }
#endif
        g_down_flag = 1;
    }
}
EXPORT_SYMBOL(set_cpu_mem_down);

extern int ak_get_otg_device(void);

void set_cpu_mem_up(void)
{
	#ifdef NO_MEMCLK_CHG_PCFI
	if(get_current_pcfi_state()) {
		CPRINT("PCFI bypass 2\n");
		return;
	}
	#endif
	
    if (g_down_flag && (ak_get_otg_device() != DEVICE_USBAUDIO)) {    
#ifdef ENABLE_DDR_600MHz                         /* [downer] A140609 */
    	if (mem_clk && g_down_memclk) {
            clk_set_rate(mem_clk, 600*1000*1000);
            g_down_memclk = 0;
        }
        
#endif
        cpu_up(1);

        g_down_flag = 0;
    }
}
EXPORT_SYMBOL(set_cpu_mem_up);

void tcc_cpu_early_suspend(struct early_suspend *h)
{
//    set_cpu_mem_down();
}

void tcc_cpu_late_resume(struct early_suspend *h)
{
//    set_cpu_mem_up();
}


#ifdef TEST_DDR_CLOCK
void set_ddr_clock(int freq)
{
	if(freq < 100000000 || freq > 600000000) {
		DBG_PRINT("DDR","Invalid DDR CLK : %d \n",freq);
	} else {
		int ret;
		DBG_ERR("DDR","DDR CLK changed as : %d \n",freq);
		ret = cpu_down(1);

		if (mem_clk)
			clk_set_rate(mem_clk, freq);

		cpu_up(1);

	}
}
#endif


#endif

static int tcc_cpu_init(struct cpufreq_policy *policy)
{
	if (policy->cpu >= CONFIG_NR_CPUS)
		return -EINVAL;

	mutex_lock(&tcc_cpu_lock);

#if defined(CONFIG_HIGHSPEED_TIME_LIMIT)
	if (policy->cpu == 0) {
		INIT_WORK(&cpufreq_work, cpufreq_work_func);
	}
#endif

#if defined(CONFIG_REGULATOR)
	if (policy->cpu == 0) {
		int i;
		if (vdd_cpu == NULL) {
			vdd_cpu = regulator_get(NULL, "vdd_cpu");
			if (IS_ERR(vdd_cpu)) {
				pr_warning("cpufreq: failed to obtain vdd_cpu\n");
				vdd_cpu = NULL;
			}
			else
				curr_voltage = regulator_get_voltage(vdd_cpu);
		}
		if (!vdd_cpu) {
			for (i=0 ; i< NUM_FREQS ; i++) {
				if (tcc_cpufreq_table[i].voltage > 1200000)
					break;
			}
			freq_table_opp[i].frequency = CPUFREQ_TABLE_END;
		}
	}
#endif

	cpu_clk = clk_get(NULL, "cpu");
	if (IS_ERR(cpu_clk))
		return PTR_ERR(cpu_clk);

	clk_enable(cpu_clk);

#if defined(CONFIG_IOBUS_DFS)
	max_io_clk = clk_get(NULL, "max_io_clk");
	if (IS_ERR(max_io_clk))
		max_io_clk = NULL;

	if (max_io_clk && (max_io_enabled==0)) {
		clk_enable(max_io_clk);
		max_io_enabled = 1;
	}
#endif

	cpufreq_frequency_table_cpuinfo(policy, freq_table);
	cpufreq_frequency_table_get_attr(freq_table, policy->cpu);
	policy->cur = tcc_getspeed(policy->cpu);
	target_cpu_speed[policy->cpu] = policy->cur;

	/* FIXME: what's the actual transition time? */
	policy->cpuinfo.transition_latency = 300 * 1000;

	policy->shared_type = CPUFREQ_SHARED_TYPE_ALL;
	cpumask_copy(policy->related_cpus, cpu_possible_mask);

	if (policy->cpu == 0) {
		register_pm_notifier(&tcc_cpu_pm_notifier);
	}

#if defined(CONFIG_HIGHSPEED_TIME_LIMIT)
	if (policy->cpu == 0) {
	    init_timer(&timer_highspeed);
	    timer_highspeed.data = 0;
	    timer_highspeed.function = highspeed_timer_func;
	    timer_highspeed.expires = jiffies + msecs_to_jiffies(HIGHSPEED_TIMER_TICK);	// 100 milisec.
	    add_timer(&timer_highspeed);
	}
#endif
#if defined(RESUME_CLOCK_LIMIT)
	if (policy->cpu == 0) {
		init_timer(&timer_resume_clock);
		timer_resume_clock.data = 0;
		timer_resume_clock.function = resume_clock_timer_func;
		timer_resume_clock.expires = jiffies + msecs_to_jiffies(2000);	// 2 secs.
		add_timer(&timer_resume_clock);
	}
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_MEM_BUS_SCALING)
	if (policy->cpu == 0) {
		tcc_cpufreq_early_suspend.suspend = tcc_cpu_early_suspend;
		tcc_cpufreq_early_suspend.resume = tcc_cpu_late_resume;
		tcc_cpufreq_early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 2;
		register_early_suspend(&tcc_cpufreq_early_suspend);	

		mem_clk = clk_get(NULL, "membus");
		if (IS_ERR(mem_clk))
			mem_clk = NULL;
	}
#endif

	mutex_unlock(&tcc_cpu_lock);

	return 0;
}

static int tcc_cpu_exit(struct cpufreq_policy *policy)
{
#if defined(CONFIG_IOBUS_DFS)
	if (max_io_clk)
		clk_put(max_io_clk);
#endif

	clk_put(cpu_clk);
	return 0;
}

static int tcc_cpufreq_policy_notifier(struct notifier_block *nb, unsigned long event, void *data)
{
	int i, ret;
	struct cpufreq_policy *policy = data;

	if (event == CPUFREQ_NOTIFY) {
		ret = cpufreq_frequency_table_target(policy, freq_table, policy->max, CPUFREQ_RELATION_H, &i);
		policy_max_speed[policy->cpu] =	ret ? policy->max : freq_table[i].frequency;
	}
	return NOTIFY_OK;
}

static struct notifier_block tcc_cpufreq_policy_nb = {
	.notifier_call = tcc_cpufreq_policy_notifier,
};

static struct freq_attr *tcc_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver tcc_driver = {
	.verify	= tcc_verify_speed,
	.target	= tcc_target,
	.get	= tcc_getspeed,
	.init	= tcc_cpu_init,
	.exit	= tcc_cpu_exit,
	.name	= "tcc",
	.attr	= tcc_cpufreq_attr,
};

//iriver-ysyim
static void start_dvfs_work(struct work_struct *work)
{
	printk("start_dvfs_work\n");
	disable_dvfs_clock=0;	
	cancel_delayed_work(&dvfs_clock_work);
}

/* [downer] A140416 for power-off dvfs lock */
void stop_dvfs_work(void)
{
    printk("stop_dvfs_work\n");

    disable_dvfs_clock = 1;
    cancel_delayed_work(&dvfs_clock_work);
}
EXPORT_SYMBOL(stop_dvfs_work);

static int __init tcc_cpufreq_init(void)
{
	int ret, i;

	for (i = 0; i < NUM_FREQS; i++) {
		freq_table_opp[i].index = i;
		freq_table_opp[i].frequency = tcc_cpufreq_table[i].cpu_freq;
	}
	freq_table_opp[i].frequency = CPUFREQ_TABLE_END;
	freq_table = freq_table_opp;

	for (i = 0; i < NUM_FREQS; i++) {
		if (tcc_cpufreq_table[i].voltage >= 1000000) {
			suspend_index = i;
			break;
		}
	}

#if defined(CONFIG_AUTO_HOTPLUG)
	ret = auto_hotplug_init(&tcc_cpu_lock);
	if (ret)
		return ret;
#endif

	ret = cpufreq_register_notifier(&tcc_cpufreq_policy_nb, CPUFREQ_POLICY_NOTIFIER);
	if (ret)
		return ret;

#if defined(CONFIG_HIGHSPEED_TIME_LIMIT)
	cpufreq_wq = create_singlethread_workqueue("cpufreq_wq");
	if (!cpufreq_wq)
		return -ENOMEM;
#endif

	//iriver-ysyim
	INIT_DELAYED_WORK_DEFERRABLE(&dvfs_clock_work, start_dvfs_work);
	//schedule_delayed_work(&dvfs_clock_work, DVFS_CLOCK_DELAY);

	return cpufreq_register_driver(&tcc_driver);
}

static void __exit tcc_cpufreq_exit(void)
{
#if defined(CONFIG_AUTO_HOTPLUG)
	auto_hotplug_exit();
#endif
	cpufreq_unregister_driver(&tcc_driver);
	cpufreq_unregister_notifier(&tcc_cpufreq_policy_nb, CPUFREQ_POLICY_NOTIFIER);
}

MODULE_DESCRIPTION("cpufreq driver for TCC893X SOCs");
MODULE_LICENSE("GPL");
module_init(tcc_cpufreq_init);
module_exit(tcc_cpufreq_exit);
