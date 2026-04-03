/*
 * drd_otg.c: common usb otg control API
 *
 *  Copyright (C) 2013, Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <asm/io.h>
#include <asm/mach-types.h>

#include <mach/bsp.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>
#include <linux/earlysuspend.h>

#include <mach/reg_physical.h>
#include <mach/structures_hsio.h>
#include <mach/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>

// [downer] for test
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include <mach/board_astell_kern.h>
#include <mach/ak_gpio_def.h>

//#define DAC_WAKELOCK

#define OTG_HOST       1
#define OTG_DEVICE     0

#define DRIVER_AUTHOR "Taejin"
#define DRIVER_DESC "'eXtensible' DRD otg Driver"
//#define DBG_FORCE_DRD_DEVICE_MODE

/* [downer] A140107 */
#define USB_DETECT_TIMER     msecs_to_jiffies(5000)
#define OTG_DETECT_TIMER     msecs_to_jiffies(5000)

#if(AK_HAVE_XMOS==1)
/* 2014.02.04 tonny */
struct semaphore dsd_usb_sem;
#endif

/*
 * [downer] A130911
 */ 
#ifdef CONFIG_REGULATOR
static struct regulator *ldo_usb1v2, *ldo_usb3v3, *vdd_io;
#endif

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR (DRIVER_AUTHOR);
MODULE_LICENSE ("GPL");

struct platform_device *pdev;
int cnt;
int cmode = OTG_DEVICE;

int incomplete_mtp_event = 0;
EXPORT_SYMBOL(incomplete_mtp_event);

struct usb3_data {
    struct work_struct     wq;
    unsigned long           flag;
};
struct usb3_data *drd_otg_mod_wq;

/* [downer] A160418 for CPU #1 active */
static void cpu_control(struct work_struct *work);
static DECLARE_WORK(cpu_control_wq, cpu_control);

static int g_usb_mode = OTG_DEVICE;

/* [downer] A160610 for USB process */
struct delayed_work usb_process_wq;

/*
 * [downer] M130806
 */
struct ak_usb_plug {
	struct work_struct wq;
    struct delayed_work usb_detect_work;   /* [downer] USB/Adapter detect */
    struct delayed_work otg_device_work; /* [downer] A150820 for OTG device check */
    
	int usb_plug_state;
    int adapter_plug_state;
    int enable_suspend;
    int mode;
    int usb_checking;
    int mode_change;            /* [downer] A150902 */
    
	int irq;
	int device; //tonny : mtp or usb_dac
};
struct ak_usb_plug *ak_usb_plug_wq;

static int g_set_plug_irq_flag = 0;

struct drd_dev {
	struct device *dev;

	bool enabled;
	struct mutex mutex;
	bool connected;
	bool sw_connected;

	struct work_struct work;
	struct delayed_work init_work;
};

static struct wake_lock usb_wakelock_delay;

struct early_suspend drd_early_suspend; /* [downer] A131203 */

static struct class *drd_class;
static struct drd_dev *_drd_dev;

static char mode_string[256];
static char def_mode_string[256];
static char plug_mode_string[256]; /* [downer] A130812 USB plug state */
static char usb_mode_string[256]; /* [downer] A140117 USB state */
static char xmos_state_string[256]; /*tonny : update xmos state*/

static int g_after_sleep = 0;


extern INTERNAL_PLAY_MODE ak_get_internal_play_mode(void);
extern void ak_set_internal_play_mode(INTERNAL_PLAY_MODE mode);
extern void ak_set_internal_play_mode_only(INTERNAL_PLAY_MODE mode);

static int g_otg_nopower_dev_check = 0; /* [downer] A160125 */
static int g_otg_disconnect_process = 0; /* [downer] A160516 */

void ak_usb_process(int mode);

int ak_check_usb_plug(void)
{
	if (gpio_get_value(GPIO_CHG_DET))
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(ak_check_usb_plug);

int ak_check_usb_unplug(void)
{
	if (!gpio_get_value(GPIO_CHG_DET))
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(ak_check_usb_unplug);

void ak_otg_disconnect_start(void)
{	
	g_otg_disconnect_process = 1;
}

int ak_otg_disconnect_state(void)
{
	return g_otg_disconnect_process;
}

void ak_otg_disconnect_finish(void)
{
	g_otg_disconnect_process = 0;
}
EXPORT_SYMBOL(ak_otg_disconnect_finish);

int ak_get_usb_mode(void)
{
    return g_usb_mode;
}
EXPORT_SYMBOL(ak_get_usb_mode);

void write_xmos_ptr_str(int val)
{
	if(val == 0)
	{
		strncpy(xmos_state_string, "xmos_off", sizeof(xmos_state_string) - 1);
	}
	else if(val == 1)
	{
		strncpy(xmos_state_string, "xmos_on", sizeof(xmos_state_string) - 1);
	}
	else if(val == 2)
	{
		strncpy(xmos_state_string, "xmos_ready", sizeof(xmos_state_string) - 1);
	}

}

static ssize_t enable_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct drd_dev *dev = dev_get_drvdata(pdev);
	return sprintf(buf, "%d\n", dev->enabled);
}

static ssize_t enable_store(struct device *pdev, struct device_attribute *attr, const char *buff, size_t size)
{
	//struct drd_dev *dev = dev_get_drvdata(pdev);
    return size;
}

static ssize_t state_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
	//struct drd_dev *dev = dev_get_drvdata(pdev);
	return 0;
}

#define DESCRIPTOR_ATTR(field, format_string)                           \
    static ssize_t                                                      \
    field ## _show(struct device *dev, struct device_attribute *attr,	\
                   char *buf)                                           \
    {                                                                   \
        return sprintf(buf, format_string, device_desc.field);          \
    }                                                                   \
    static ssize_t                                                      \
    field ## _store(struct device *dev, struct device_attribute *attr,	\
                    const char *buf, size_t size)                       \
    {                                                                   \
        int value;                                                      \
        if (sscanf(buf, format_string, &value) == 1) {                  \
            device_desc.field = value;                                  \
            return size;                                                \
        }                                                               \
        return -1;                                                      \
    }                                                                   \
    static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field ## _show, field ## _store);

#define DESCRIPTOR_STRING_ATTR(field, buffer)                           \
    static ssize_t                                                      \
    field ## _show(struct device *dev, struct device_attribute *attr,	\
                       char *buf)                                       \
    {                                                                   \
     return sprintf(buf, "%s", buffer);                                 \
     }                                                                  \
    static ssize_t                                                      \
    field ## _store(struct device *dev, struct device_attribute *attr,	\
                        const char *buf, size_t size)                   \
    {                                                                   \
     if (size >= sizeof(buffer))                                        \
         return -EINVAL;                                                \
     return strlcpy(buffer, buf, sizeof(buffer));                       \
     }                                                                  \
    static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field ## _show, field ## _store);

DESCRIPTOR_STRING_ATTR(drdmode, mode_string)
DESCRIPTOR_STRING_ATTR(defmode, def_mode_string)
DESCRIPTOR_STRING_ATTR(plugmode, plug_mode_string)
DESCRIPTOR_STRING_ATTR(usbmode, usb_mode_string)
DESCRIPTOR_STRING_ATTR(xmos, xmos_state_string);

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, enable_show, enable_store);
static DEVICE_ATTR(state, S_IRUGO, state_show, NULL);

static struct device_attribute *drd_usb_attributes[] = {
	&dev_attr_drdmode,
	&dev_attr_defmode,
	&dev_attr_plugmode,
	&dev_attr_usbmode,    
	&dev_attr_xmos,
	NULL
};

extern int tcc_usb30_vbus_init(void);
extern int tcc_usb30_vbus_exit(void);
extern int tcc_dwc3_vbus_ctrl(int on);
extern void tcc_usb30_phy_off(void);
extern void tcc_usb30_phy_on(void);
extern int tcc_usb30_clkset(int on);

static int set_otg_irq_flag = 0;

static void tcc_stop_dwc3(void)
{
    tcc_usb30_clkset(0);
}

static void tcc_start_dwc3(void)
{
    tcc_usb30_clkset(1);
}

#ifdef CONFIG_REGULATOR
#define IO_VOLTAGE_330  3300000
#define IO_VOLTAGE_280  2800000

extern int get_boot_complete(void);
static int g_reg_ref = 0; // default on
void dwc_usb_regulator_enable(void)
{
    printk("\x1b[1;33m%s(%d) \x1b[0m\n", __func__, g_reg_ref);
    
    if (g_reg_ref == 0) {
        if (get_boot_complete()) 
            regulator_set_voltage(vdd_io, IO_VOLTAGE_330, IO_VOLTAGE_330);

        regulator_enable(ldo_usb1v2);
        regulator_enable(ldo_usb3v3);

        g_reg_ref = 1;        
    }
}
EXPORT_SYMBOL(dwc_usb_regulator_enable);

void dwc_usb_regulator_disable(void)
{
    printk("\x1b[1;33m%s(%d) \x1b[0m\n", __func__, g_reg_ref);    
    
    if (g_reg_ref == 1) {
        if (get_boot_complete())        
           regulator_set_voltage(vdd_io, IO_VOLTAGE_280, IO_VOLTAGE_280);     // 2016.04.26-ysyim: AK70 LCD           
          //  regulator_set_voltage(vdd_io, IO_VOLTAGE_330, IO_VOLTAGE_330);    //2015.02.10-ysyim: io voltage 3.3v?? ??d ?׽?Ʈ 

        regulator_disable(ldo_usb1v2);
        regulator_disable(ldo_usb3v3);
        
        g_reg_ref = 0;
    }
}
EXPORT_SYMBOL(dwc_usb_regulator_disable);
#endif                                  

/* [downer] A131127 USB 연결상태에서는 sleep에 들어가지 않지만 예외로 들어가기 위한 플래그.*/
int get_pcfi_suspend_enable(void)
{
    return ak_usb_plug_wq->enable_suspend;
}
EXPORT_SYMBOL(get_pcfi_suspend_enable);

/* [downer] A140106 USB-DAC 상태 플래그 */
int get_current_pcfi_state(void)
{
    if ((ak_get_current_usb_mode() == UCM_PC_FI) && (g_usb_mode == OTG_DEVICE))
        return ak_usb_plug_wq->usb_plug_state;

    return 0;
}
EXPORT_SYMBOL(get_current_pcfi_state);

int get_current_usb_device(void)
{
    if(ak_usb_plug_wq) 
	   	return ak_usb_plug_wq->device;

    return 0;
}
EXPORT_SYMBOL(get_current_usb_device);

/* [downer] A131205 */
int get_adapter_state(void)
{
    if (get_boot_complete())     
        return ak_usb_plug_wq->adapter_plug_state;
    else
        return 0;
}
EXPORT_SYMBOL(get_adapter_state);

/* [downer] A140119 for wakeup source */
int get_usb_plug_state(void)
{
    return ak_usb_plug_wq->usb_plug_state || ak_usb_plug_wq->adapter_plug_state;
}
EXPORT_SYMBOL(get_usb_plug_state);

/* [downer] A140107 */
static int g_set_usb_connect = 0, g_set_usb_configure = 0;
void set_usb_connect_state(int state)
{
    g_set_usb_connect = state;
}
EXPORT_SYMBOL(set_usb_connect_state);

int get_usb_connect_state(void)
{
    return g_set_usb_connect;
}

void set_usb_configure_state(int state)
{
    g_set_usb_configure = state;
}
EXPORT_SYMBOL(set_usb_configure_state);

int get_usb_configure_state(void)
{
    return g_set_usb_configure;
}
EXPORT_SYMBOL(get_usb_configure_state);

int get_usb_checking_state(void)
{
    return ak_usb_plug_wq->usb_checking;
}
EXPORT_SYMBOL(get_usb_checking_state);

static void  ak_mtp_path_enable(int on)
{
#if 0
	if (on) {
		printk("============= MTP Path Enable =============\n");
		gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
		tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
		gpio_direction_output(GPIO_USB_DAC_SEL, 1);
	}
	else {
		int play_mode = ak_get_internal_play_mode();
		if (play_mode != INTERNAL_DSD_PLAY)
		{
			printk("============= MTP Path Disable =============\n");
			gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
			tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_DAC_SEL, 0);
		}
	}
#endif
}

int tcc_drd_vbus_ctrl(int on)
{
    if (on) {
        printk("========== USB POWER ENABLE =========\n");            
        gpio_direction_output(GPIO_USB_PWR_EN, 1);
        mdelay(10);
		
        printk("========== OTG ENABLE =========\n");            
        gpio_direction_output(GPIO_OTG_EN, 1);
        mdelay(10);

        printk("========== OTG POWER ENABLE =========\n");            
        gpio_direction_output(GPIO_OTG_5V_EN, 1);
    }
    else {
        printk("========== OTG POWER DISABLE =========\n");            
        gpio_direction_output(GPIO_OTG_5V_EN, 0);

        printk("========== OTG DISABLE =========\n");            
        gpio_direction_output(GPIO_OTG_EN, 0);
        mdelay(10);

        printk("========== USB POWER DISABLE =========\n");            
        gpio_direction_output(GPIO_USB_PWR_EN, 0);
    }
    
    return 0;
}

spinlock_t lock;
static irqreturn_t dwc_usb3_otg_host_dwc_irq(int irq, struct platform_device *pdev )
{
    printk("\x1b[1;36m Device mode -> Host mode(line : %d)\x1b[0m\n",__LINE__);
    spin_lock(&lock);

    g_usb_mode = OTG_HOST;
    drd_otg_mod_wq->flag = 0;
    schedule_work(&drd_otg_mod_wq->wq);
    
    spin_unlock(&lock);
    
    return IRQ_HANDLED;
}

static irqreturn_t dwc_usb3_otg_device_dwc_irq(int irq, struct platform_device *pdev )
{
    printk("\x1b[1;36m Host mode -> Device mode(line: %d)  \x1b[0m\n",__LINE__);

    spin_lock(&lock);

    g_usb_mode = OTG_DEVICE;    
    drd_otg_mod_wq->flag = 1;
    schedule_work(&drd_otg_mod_wq->wq);

    spin_unlock(&lock);

    return IRQ_HANDLED;
}

/* [downer] A140115 */
void usb_check_refresh(void)
{
//    printk("\x1b[1;36m %s  \x1b[0m\n", __func__);
    ak_usb_plug_wq->mode_change = 0;    
    cancel_delayed_work(&ak_usb_plug_wq->usb_detect_work);
    schedule_delayed_work(&ak_usb_plug_wq->usb_detect_work, USB_DETECT_TIMER);
}
EXPORT_SYMBOL(usb_check_refresh);

void usb_check_refresh_now(void)
{
//    printk("\x1b[1;36m %s  \x1b[0m\n", __func__);
    ak_usb_plug_wq->mode_change = 0;
    
    cancel_delayed_work(&ak_usb_plug_wq->usb_detect_work);
    schedule_delayed_work(&ak_usb_plug_wq->usb_detect_work, 0);
}
EXPORT_SYMBOL(usb_check_refresh_now);

void usb_check_refresh_mode_change(void)
{
//    printk("\x1b[1;36m %s  \x1b[0m\n", __func__);
    ak_usb_plug_wq->mode_change = 1;
    
    cancel_delayed_work(&ak_usb_plug_wq->usb_detect_work);
    schedule_delayed_work(&ak_usb_plug_wq->usb_detect_work, 0);
}
EXPORT_SYMBOL(usb_check_refresh_mode_change);

void ak_otg_unplug_refresh(void)
{
    g_usb_mode = OTG_DEVICE;

	#if 0 // NOT USE
    gpio_direction_output(GPIO_OTG_DET, 1);                    
	#endif
	
    schedule_delayed_work(&ak_usb_plug_wq->usb_detect_work, 0);    
}
EXPORT_SYMBOL(ak_otg_unplug_refresh);

void otg_host_check_refresh(void)
{
//    printk("\x1b[1;36m %s  \x1b[0m\n", __func__);
	if (ak_get_internal_play_mode() != INTERNAL_DSD_PLAY) {
		cancel_delayed_work(&ak_usb_plug_wq->otg_device_work);
		schedule_delayed_work(&ak_usb_plug_wq->otg_device_work, OTG_DETECT_TIMER);
	}
}
EXPORT_SYMBOL(otg_host_check_refresh);

void otg_host_check_refresh_now(void)
{
//    printk("\x1b[1;36m %s  \x1b[0m\n", __func__);
	if (ak_get_internal_play_mode() != INTERNAL_DSD_PLAY) {
		cancel_delayed_work(&ak_usb_plug_wq->otg_device_work);
		schedule_delayed_work(&ak_usb_plug_wq->otg_device_work, 0);
	}
}
EXPORT_SYMBOL(otg_host_check_refresh_now);

void otg_host_check_cancel(void)
{
//    printk("\x1b[1;36m %s  \x1b[0m\n", __func__);
	if (ak_get_internal_play_mode() != INTERNAL_DSD_PLAY) {	
		cancel_delayed_work(&ak_usb_plug_wq->otg_device_work);
	}
}
EXPORT_SYMBOL(otg_host_check_cancel);

static int g_confirm_mode = 0;
int get_usb_mode_confirm(void)
{
    // 1: checking, 2: MTP, 3: DAC
    return g_confirm_mode;
}
EXPORT_SYMBOL(get_usb_mode_confirm);

static void send_usb_uevent(int mode)
{
    char *mtp[2] = { "USB_STATE=CONFIGURED", NULL };
    char *dac[2] = { "USB_PLUG_STATE=DAC", NULL };
    char *plug[2] = { "USB_PLUG_STATE=PLUG", NULL };    
    char *unplug[2] = { "USB_PLUG_STATE=UNPLUG", NULL };
    char *adapter[2] = { "USB_PLUG_STATE=ADAPTER", NULL };
    char **uevent_envp = NULL;

    if (mode == UCM_MTP)
        uevent_envp = mtp;
    else if (mode == UCM_PC_FI)
        uevent_envp = dac;
    else if (mode == UCM_ADAPTER)
        uevent_envp = adapter;    
    else if (mode == UCM_CHECKING)
        uevent_envp = plug;
    else
        uevent_envp = unplug;

	if (uevent_envp) {
        kobject_uevent_env(&_drd_dev->dev->kobj, KOBJ_CHANGE, uevent_envp);
		printk("\x1b[1;31m %s: sent uevent %s \x1b[0m\n", __func__, uevent_envp[0]) ;               
    }
    else {
		pr_info("%s: did not send uevent\n", __func__);
    }
}

/* [downer] A160610 USB checking workqueue */
static void usb_process_work(struct work_struct *work)
{
	printk("\x1b[1;31m %s\x1b[0m\n", __func__);
	ak_usb_process(0);
}

/* [downer] A150820 OTG CD_ROM ì¸ì‹ ì—¬ë¶€ í™•ì¸ */
extern void sys_external_otg_state(int state);
extern int get_otg_dev_num(void);
extern int ak_get_otg_device(void);
extern int get_usbaudio_chip_num(void);
extern void set_cpu_mem_down(void);
extern void set_cpu_mem_up(void);

void ak_otg_control(int pwr, int mode);
void ak_recorder_control(int pwr);

static void otg_device_work(struct work_struct *work)
{
	if (ak_usb_plug_wq->usb_plug_state) {
		if (g_usb_mode == OTG_HOST) {
			if (get_otg_dev_num() == OTG_NC_DEVICE) {
				printk("\x1b[1;36m OTG HOST not connect DEVICE!  \x1b[0m\n");
				sys_external_otg_state(OTG_NC_DEVICE);
				sys_external_otg_state(OTG_DISCONNECT);

				ak_otg_control(EXT_DOCK_OFF, OTG_SELFPOWER_DEVICE);
			}
			else if (get_otg_dev_num() == OTG_NS_DEVICE) {
				printk("\x1b[1;36m OTG HOST not support DEVICE! \x1b[0m\n");
				sys_external_otg_state(OTG_NS_DEVICE);
				sys_external_otg_state(OTG_DISCONNECT);

				ak_otg_control(EXT_DOCK_OFF, OTG_SELFPOWER_DEVICE);
			}
            else {
				if (ak_get_otg_device() == DEVICE_RIPPER) {
					printk("\x1b[1;36m OTG AK-Ripper found! \x1b[0m\n");        
					sys_external_otg_state(OTG_RIPPER_CONNECT);
				}
				else if (ak_get_otg_device() == DEVICE_RECORDER) {
					printk("\x1b[1;36m OTG AK-Recorder found! \x1b[0m\n");        
					sys_external_otg_state(OTG_RECORDER_CONNECT);
#if(CUR_DAC==CODEC_AK4490)
					ak4490_save_cur_volume();
#elif(CUR_DAC==CODEC_AK4497)
					ak4497_save_cur_volume();
#endif
					//recorder_module_change(1);

				}
				else if (ak_get_otg_device() == DEVICE_USBAUDIO) {
					//set_cpu_mem_down(); /* [[downer] A160418 for USB-Audio noise */
					printk("\x1b[1;36m OTG USB-AUDIO found(%d)! \x1b[0m\n", get_usbaudio_chip_num());        
					sys_external_otg_state(OTG_USBAUDIO_CONNECT + get_usbaudio_chip_num());
				}
				else {
					printk("\x1b[1;36m OTG ?????? found! \x1b[0m\n");        
				}
            }
        }
		else {
			printk("Not HOST mode\n");
		}
	}
	else {
		if (g_usb_mode == OTG_HOST) {        
			if (!g_otg_nopower_dev_check) {
				printk("\x1b[1;36m OTG No power device checking....\x1b[0m\n");
				g_otg_nopower_dev_check = 1;
				
				ak_otg_control(EXT_DOCK_ON, OTG_NOPOWER_DEVICE);
			}
			else {
				printk("\x1b[1;36m Not connect or No Adaptor!! \x1b[0m\n");
				sys_external_otg_state(OTG_NC_DEVICE);			
				sys_external_otg_state(OTG_DISCONNECT);
				g_otg_nopower_dev_check = 0;
				
				ak_otg_control(EXT_DOCK_OFF, OTG_SELFPOWER_DEVICE);
			} 
		}
		else {					/* [downer/A170811] */
			printk("\x1b[1;36m Unexpected USB event. \x1b[0m\n");			
			sys_external_otg_state(OTG_NC_DEVICE);			
			sys_external_otg_state(OTG_DISCONNECT);
			g_otg_nopower_dev_check = 0;
			ak_otg_control(EXT_DOCK_OFF, OTG_SELFPOWER_DEVICE);			
		}
	}
}

void ak_usb_plug_change(int connect_type);
static void usb_detect_work(struct work_struct *work)
{
    int usb_mode;

    if (!get_boot_complete())
        return;

    printk("%s\n", __func__);
    
    if (ak_check_usb_plug() || g_otg_nopower_dev_check) {
        if (g_usb_mode == OTG_HOST) {
            printk("DRD USB HOST mode\n");
            strncpy(mode_string, "usb_host", sizeof(mode_string) - 1);

            strncpy(usb_mode_string, "otg", sizeof(usb_mode_string) - 1);                
            ak_usb_plug_wq->usb_plug_state = 1;

			if (g_otg_nopower_dev_check)
				ak_usb_plug_wq->adapter_plug_state = 0;
			else
				ak_usb_plug_wq->adapter_plug_state = 1;
			
            ak_usb_plug_wq->mode = UCM_OTG_HOST;                        
            ak_usb_plug_wq->usb_checking = 0;
        }
        else {
            usb_mode = ak_get_current_usb_mode();

            if ((get_usb_connect_state() && ak_usb_plug_wq->usb_plug_state) ||
                (ak_usb_plug_wq->mode_change && ak_usb_plug_wq->usb_plug_state)) { /* real USB */
                printk("\x1b[1;36m PC <-> USB connection success \x1b[0m\n");
                strncpy(plug_mode_string, "usb_plugged", sizeof(plug_mode_string) - 1);

                if (ak_usb_plug_wq->mode_change)
                    ak_usb_plug_wq->mode_change = 0;
                
                if (usb_mode == UCM_MTP) {
                    printk("\x1b[1;36m Current mode is USB_MTP \x1b[0m\n");

                    g_confirm_mode = 2;
                    strncpy(usb_mode_string, "mtp", sizeof(usb_mode_string) - 1);                
                    ak_usb_plug_wq->usb_plug_state = 1;
                    ak_usb_plug_wq->adapter_plug_state = 0;
                
                    if (ak_usb_plug_wq->usb_plug_state) {
                        printk("\x1b[1;36m USB plugged(MTP) \x1b[0m\n");

                        ak_usb_plug_wq->mode = usb_mode;                        
                        ak_usb_plug_change(usb_mode);   /* [downer] A131205 */
                        ak_usb_plug_wq->usb_checking = 0;

                        /* [downer] A151116 force push for mtp event */
                        if (incomplete_mtp_event) {
                            printk("force push incomplete mtp event\n");
                            send_usb_uevent(usb_mode);
							incomplete_mtp_event = 0;
                        }
                    }
                }
#if 1 //(CUR_AK == MODEL_AK240)                        
                else if (usb_mode == UCM_PC_FI) {
                    printk("\x1b[1;36m Current mode is USB_DAC \x1b[0m\n");

                    g_confirm_mode = 3;                
                    ak_usb_plug_wq->mode = usb_mode;
                    ak_usb_plug_wq->usb_plug_state = 1;
                    ak_usb_plug_wq->adapter_plug_state = 0; 
                    ak_usb_plug_wq->usb_checking = 0;
                
                    ak_usb_plug_change(usb_mode);   /* [downer] A131205 */
                
                    strncpy(usb_mode_string, "dac", sizeof(usb_mode_string) - 1);
                    send_usb_uevent(usb_mode);                
                }
#endif
            }
            else { /* maybe adapter or OTG HOST */
				/* [downer] A160125 OTG HOST ì²´í¬ */
				if (g_otg_nopower_dev_check) {
					ak_otg_control(EXT_DOCK_OFF, OTG_NOPOWER_DEVICE);
					//ak_usb_process(0);
				}
				else {
					printk("\x1b[1;36m Adapter plugged \x1b[0m\n");

	                /* [downer] A140225 */            
	#if (CUR_AK == MODEL_AK120_II || CUR_AK == MODEL_AK100_II)
	                gpio_request(GPIO_CHG_SEL, "GPIO_CHG_SEL");
	                tcc_gpio_config(GPIO_CHG_SEL, GPIO_FN(0));
	                gpio_direction_output(GPIO_CHG_SEL, 1);
	#endif

	                g_confirm_mode = 0;

	                strncpy(plug_mode_string, "adapter_plugged", sizeof(plug_mode_string) - 1);
	                strncpy(usb_mode_string, "adapter", sizeof(usb_mode_string) - 1);

	                send_usb_uevent(UCM_ADAPTER);
	                ak_usb_plug_wq->mode = UCM_ADAPTER;

	                ak_usb_plug_change(UCM_ADAPTER);

					ak_usb_plug_wq->adapter_plug_state = 1;
					ak_usb_plug_wq->usb_plug_state = 0;
					ak_usb_plug_wq->usb_checking = 0;
				}
			}
        }
    }
}

/*
 * [downer] A140119
 * mode: 0(normal), 1(after wakeup), 2(boot_complete)
 */
void ak_usb_process(int mode)
{
    /* [downer] M140117 USB/Adapter detect procedure */
    if (ak_check_usb_plug()) { // plug
        if (mode == 1) {
            if (get_pcfi_suspend_enable() || get_adapter_state()) 
                return;
        }
		else if (mode == 2) {	/* [downer]A160331 ë¶€íŒ… ì™„ë£Œ ì´ë²¤íŠ¸ ì´ì „ì— MTP ì—°ê²° ì‹œ */
			if (incomplete_mtp_event) {
				ak_usb_plug_wq->mode = UCM_MTP;
				ak_usb_plug_wq->usb_plug_state = 1;
				g_confirm_mode = 1;
				
				schedule_delayed_work(&ak_usb_plug_wq->usb_detect_work, 0);
				return;
			}
		}
        /* [downer] A140225 */
#if (CUR_AK == MODEL_AK120_II || CUR_AK == MODEL_AK100_II)
        gpio_request(GPIO_CHG_SEL, "GPIO_CHG_SEL");
        tcc_gpio_config(GPIO_CHG_SEL, GPIO_FN(0));
        gpio_direction_output(GPIO_CHG_SEL, 0);
#endif
        
        printk("\x1b[1;36m %s -- USB plugged! Checking USB connection.... \x1b[0m\n", __func__);

		ak_mtp_path_enable(1);					
		ak_usb_plug_wq->mode = UCM_MTP;
		ak_usb_plug_wq->usb_checking = 1;
        ak_usb_plug_wq->usb_plug_state = 1;
        g_confirm_mode = 1;
        
        strncpy(plug_mode_string, "usb_plugged", sizeof(plug_mode_string) - 1);        

        wake_lock_timeout(&usb_wakelock_delay, USB_DETECT_TIMER);
		schedule_delayed_work(&ak_usb_plug_wq->usb_detect_work, USB_DETECT_TIMER);
    }
    else {	// unplug
        if (ak_usb_plug_wq->device == UCM_PC_FI) { 		  //if  pcfi unpluged.	
#if (CUR_AK != MODEL_AK500N)
            gpio_request(GPIO_HP_MUTE, "GPIO_HP_MUTE");
            tcc_gpio_config(GPIO_HP_MUTE,GPIO_FN(0));
            gpio_direction_output(GPIO_HP_MUTE, 1);
#endif
            gpio_request(GPIO_AUDIO_SW_MUTE, "GPIO_AUDIO_SW_MUTE");

#ifdef DAC_WAKELOCK
            wake_lock_destroy(&usb_wakelock_delay);            
#endif
            
#ifndef SLEEP_CURRENT_CHECK
            tcc_gpio_config(GPIO_AUDIO_SW_MUTE,GPIO_FN(0));
            gpio_direction_output(GPIO_AUDIO_SW_MUTE, 0);
#else
            tcc_gpio_config(GPIO_AUDIO_SW_MUTE, GPIO_FN(0) | GPIO_INPUT);
#endif
        }

        printk("\x1b[1;36m %s -- USB or Adapter unplugged! \x1b[0m\n", __func__);

        strncpy(plug_mode_string, "usb_unplugged", sizeof(plug_mode_string) - 1);
        
        ak_usb_plug_wq->usb_plug_state = 0;
        ak_usb_plug_wq->adapter_plug_state = 0;            
        ak_usb_plug_wq->usb_checking = 0;
        g_confirm_mode = 0;
        g_set_usb_connect = 0;
		g_otg_nopower_dev_check = 0;
        g_usb_mode = OTG_DEVICE; /* [downer] A150819 */
        incomplete_mtp_event = 0;

		ak_mtp_path_enable(0);		
        ak_usb_plug_wq->mode = UCM_NONE;
    }
    
    schedule_work(&ak_usb_plug_wq->wq);    
}
EXPORT_SYMBOL(ak_usb_process);

static irqreturn_t ak_usb_plug_handler(int irq, void *data)
{
//	ak_usb_process(0);
	schedule_delayed_work(&usb_process_wq, 50);
	
    return IRQ_HANDLED;
}

//mtp <--> usb dac
void ak_usb_plug_change(int connect_type)
{           
	int play_mode = ak_get_internal_play_mode();

	printk("\x1b[1;36m %s -- Changing USB Mode! %d \x1b[0m\n", __func__,connect_type);
#if(AK_HAVE_XMOS==1)
	down(&dsd_usb_sem);
	printk("------------------------>ak_usb_plug_change0\n");
#endif

	/* [downer] A131127 set default value */
	ak_usb_plug_wq->enable_suspend = 0;

	if (get_boot_complete())
		ak_set_sidekey_lock_mode(0);

	if (connect_type == UCM_MTP) {
		if (ak_usb_plug_wq->device != UCM_MTP) {
			printk("\x1b[1;36m -- UCM_MTP ! \x1b[0m\n");

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK380S || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)                        
			if (play_mode != INTERNAL_DSD_PLAY) {
				if (ak_usb_plug_wq->device == UCM_PC_FI) 	{	  // pcfi  ===============>  	mtp.	
					ak_set_hw_mute(AMT_HP_MUTE, 1);				
					ak_set_snd_pwr_ctl_request(AK_AUDIO_POWER,AK_PWR_ON);
				}

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK380S || CUR_AK == MODEL_AK300)                
				printk("============= USB20 Host Disable  ==============\n");
				gpio_request(GPIO_USB20_HOST_EN, "usb20_host_en");
				tcc_gpio_config(GPIO_USB20_HOST_EN, GPIO_OUTPUT | GPIO_FN(0));
				gpio_direction_output(GPIO_USB20_HOST_EN, 0);

				printk("============ xMos Power Off ===========\n");
				gpio_request(GPIO_XMOS_PWR_EN, "xmos_pwr_en");
				tcc_gpio_config(GPIO_XMOS_PWR_EN, GPIO_FN(0)); 
				gpio_direction_output(GPIO_XMOS_PWR_EN, 0);

				//printk("============= cpu to dac connection =========\n");
				gpio_request(GPIO_I2C_SEL, "GPIO_I2C_SEL");
				tcc_gpio_config(GPIO_I2C_SEL,GPIO_FN(0));
				gpio_direction_output(GPIO_I2C_SEL, 0);

				//printk("============= CPU I2S Enable =============\n");
				gpio_request(GPIO_CPU_I2S_EN, "GPIO_CPU_I2S_EN");
				tcc_gpio_config(GPIO_CPU_I2S_EN,GPIO_FN(0));
				gpio_direction_output(GPIO_CPU_I2S_EN, 0);
#endif
                
				printk("============= MTP Path Enable =============\n");
				gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
				tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
				gpio_direction_output(GPIO_USB_DAC_SEL, 1);

			}
			gpio_request(GPIO_USB_ON, "GPIO_USB_ON");
			tcc_gpio_config(GPIO_USB_ON,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_ON, 0);

#ifdef ARRANGE_AUDIO_PATH
			if(ak_usb_plug_wq->device == UCM_PC_FI)  {		  //if  pcfi  ===============>  	mtp.	
				if(ak_is_connected_optical_tx()) 
					ak_set_audio_path(ADP_OPTICAL_OUT);
				else 	
					ak_set_audio_path(ADP_PLAYBACK);		
			}
#endif
			if(play_mode != INTERNAL_DSD_PLAY)
				strncpy(xmos_state_string, "xmos_off", sizeof(xmos_state_string) - 1);
#endif
			ak_usb_plug_wq->device = UCM_MTP;
		}
	}            
	else if(connect_type == UCM_PC_FI) {
//jimi.0422
#if (CUR_AK == MODEL_AK120_II ||  CUR_AK == MODEL_AK100_II || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300 || CUR_AK == MODEL_AK70) //J150924
		if (ak_usb_plug_wq->device != UCM_PC_FI) {
			printk("\x1b[1;36m --- UCM_PC_FI \x1b[0m\n");
#if (CUR_AK == MODEL_AK320) || (CUR_AK == MODEL_AK300)
			ak4490_save_cur_volume();
#endif
			ak_set_sidekey_lock_mode(1);
			ak_usb_plug_wq->device = UCM_PC_FI;
		}

#elif (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK380S)                        
		if (ak_usb_plug_wq->device != UCM_PC_FI) {
			printk("\x1b[1;36m -- UCM_PC_FI \x1b[0m\n");

			printk("USB_DAC suspend_enable!\n");

			ak_set_sidekey_lock_mode(1);

			//volume down
			#if (CUR_DAC==CODEC_CS4398) 			
			cs4398_volume_mute();
			#endif
			
			msleep_interruptible(250);

			if(is_selected_balanced())
				ak_set_audio_sw_mute(1);
			else
				ak_set_hp_mute(1);

			//ak_set_hw_mute(AMT_HP_MUTE, 1);

			msleep_interruptible(200);

			if(play_mode == INTERNAL_DSD_PLAY)
				ak_set_internal_play_mode_only(INTERNAL_NONE_PLAY);

#ifdef DAC_WAKELOCK
            wake_lock(&usb_wakelock_delay);            
#else
			ak_usb_plug_wq->enable_suspend = 1; /* [downer] A131127 PC-FI 상태는 suspend 진입한다. */
#endif
			set_audio_power_ctl_status(0);

			if(ak_get_snd_pwr_ctl(AK_AUDIO_POWER) == 0)
				ak_set_snd_pwr_ctl(AK_AUDIO_POWER, AK_PWR_ON);

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK380S)                                        
			//printk("============= xmos Power Off ==============\n");
			gpio_request(GPIO_XMOS_PWR_EN, "xmos_pwr_en");
			tcc_gpio_config(GPIO_XMOS_PWR_EN, GPIO_FN(0)); 
			gpio_direction_output(GPIO_XMOS_PWR_EN, 0);

			mdelay(200);

			//printk("============= Connect XMOS to flash ==========\n");
			gpio_request(GPIO_XMOS_SPI_SEL, "GPIO_XMOS_SPI_SEL");
			tcc_gpio_config(GPIO_XMOS_SPI_SEL, GPIO_FN(0)); 
			gpio_direction_output(GPIO_XMOS_SPI_SEL, 1);
#endif
			//printk("============= USB DAC Path Enable =============\n");
			gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
			tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_DAC_SEL, 0);

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK380S)            
			//printk("============= xmos to dac connection =========\n");
			gpio_request(GPIO_I2C_SEL, "GPIO_I2C_SEL");
			tcc_gpio_config(GPIO_I2C_SEL,GPIO_FN(0));
			gpio_direction_output(GPIO_I2C_SEL,1);

			//printk("============= CPU I2S Disable =============\n");
			gpio_request(GPIO_CPU_I2S_EN, "GPIO_CPU_I2S_EN");
			tcc_gpio_config(GPIO_CPU_I2S_EN,GPIO_FN(0));
			gpio_direction_output(GPIO_CPU_I2S_EN,1);

			//printk("============= xmos I2S Disable =============\n");
			//io_request(GPIO_XMOS_I2S_EN, "GPIO_I2C_SEL");
			//c_gpio_config(GPIO_XMOS_I2S_EN,GPIO_FN(0));
			//io_direction_output(GPIO_XMOS_I2S_EN, 1);

			mdelay(500);
			//printk("============= xmos Power On ==============\n");
			gpio_request(GPIO_XMOS_PWR_EN, "xmos_pwr_en");
			tcc_gpio_config(GPIO_XMOS_PWR_EN, GPIO_FN(0)); 
			gpio_direction_output(GPIO_XMOS_PWR_EN, 1);
#endif
			gpio_request(GPIO_USB_ON, "GPIO_USB_ON");
			tcc_gpio_config(GPIO_USB_ON,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_ON, 0);

			ak_set_audio_path(ADP_USB_DAC);

			//msleep_interruptible(300);
			strncpy(xmos_state_string, "xmos_off", sizeof(xmos_state_string) - 1);
			msleep_interruptible(300);

			ak_set_hw_mute(AMT_DAC_MUTE, 0);
			ak_set_hw_mute(AMT_HP_MUTE, 0);

#if (CUR_AK != MODEL_AK380 && CUR_AK != MODEL_AK380S && CUR_AK != MODEL_AK320 && CUR_AK != MODEL_AK300)
			printk("============= sw mute low ==============\n");
			gpio_request(GPIO_AUDIO_SW_MUTE, "GPIO_AUDIO_SW_MUTE");

#ifndef SLEEP_CURRENT_CHECK            
			tcc_gpio_config(GPIO_AUDIO_SW_MUTE,GPIO_FN(0));
			gpio_direction_output(GPIO_AUDIO_SW_MUTE, 0);
#else
			tcc_gpio_config(GPIO_AUDIO_SW_MUTE,GPIO_FN(0) | GPIO_INPUT);
#endif
#endif

			ak_usb_plug_wq->device = UCM_PC_FI;
		}
#endif
	}
	else if (connect_type == UCM_ADAPTER) {
		printk("\x1b[1;36m -- UCM_ADAPTER \x1b[0m\n");
#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK380S) //J150924  || CUR_AK == MODEL_AK320)                        
		if (play_mode != INTERNAL_DSD_PLAY) {
			gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
			tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_DAC_SEL, 0);

#if 0 //(CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK380S || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
			gpio_request(GPIO_USB_ON, "GPIO_USB_ON");
			tcc_gpio_config(GPIO_USB_ON,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_ON, 1);
#endif
		}
		else {
			gpio_request(GPIO_USB_ON, "GPIO_USB_ON");
			tcc_gpio_config(GPIO_USB_ON,GPIO_FN(0));
#if (CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK380S) //J150924 || CUR_AK == MODEL_AK320)
			gpio_direction_output(GPIO_USB_ON, 0);
#else
			gpio_direction_output(GPIO_USB_ON, 1);
#endif
		}
#endif
		ak_usb_plug_wq->device = UCM_ADAPTER;
	}
	else if (connect_type == UCM_NONE) {
		printk("\x1b[1;36m -- UCM_NONE \x1b[0m\n");
#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK380S || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)                        
		if (ak_usb_plug_wq->device == UCM_PC_FI) { 		  //if  pcfi unpluged.	
			ak_set_snd_pwr_ctl_request(AK_AUDIO_POWER,AK_PWR_ON);
			ak_set_hw_mute(AMT_HP_MUTE, 1);
		}

		if (play_mode != INTERNAL_DSD_PLAY) {
#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK380S)            
			//printk("============ xMos Power Off ===========\n");
			gpio_request(GPIO_XMOS_PWR_EN, "xmos_pwr_en");
			tcc_gpio_config(GPIO_XMOS_PWR_EN, GPIO_FN(0)); 
			gpio_direction_output(GPIO_XMOS_PWR_EN, 0);

			//printk("============= cpu to dac connection \n");
			gpio_request(GPIO_I2C_SEL, "GPIO_I2C_SEL");
			tcc_gpio_config(GPIO_I2C_SEL,GPIO_FN(0));
			gpio_direction_output(GPIO_I2C_SEL, 0);

			//printk("============= CPU I2S Enable \n");
			gpio_request(GPIO_CPU_I2S_EN, "GPIO_I2C_SEL");
			tcc_gpio_config(GPIO_CPU_I2S_EN,GPIO_FN(0));
			gpio_direction_output(GPIO_CPU_I2S_EN, 0);
#endif
            
			//printk("============= MTP Path Enable \n");
			gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
			tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_DAC_SEL, 0);	//interal 재생 중 adapter detactor 문제로 low 고정.

#ifdef ARRANGE_AUDIO_PATH
			if (ak_is_connected_optical_tx()) 
				ak_set_audio_path(ADP_OPTICAL_OUT);
			else 
				ak_set_audio_path(ADP_PLAYBACK);
#endif
		}

		if (ak_usb_plug_wq->device == UCM_PC_FI) //if  pcfi unpluged.	
			ak_set_hw_mute(AMT_HP_MUTE, 1);

		if (play_mode != INTERNAL_DSD_PLAY) {
			printk("\x1b[1;36m %s --xmos_off! \x1b[0m\n", __func__);        
			strncpy(xmos_state_string, "xmos_off", sizeof(xmos_state_string) - 1);
		}
#endif        
		ak_usb_plug_wq->device = UCM_NONE; //jimi.0127
	}
	else if(connect_type == UCM_INIT) {
		printk("\x1b[1;36m -- UCM_INIT \x1b[0m\n");
#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK380S) //J150924 || CUR_AK == MODEL_AK320)                        
		gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
		tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
		gpio_direction_output(GPIO_USB_DAC_SEL, 0);
#endif
		ak_usb_plug_wq->device = UCM_INIT;
	}      
#if(AK_HAVE_XMOS==1)
	printk("------------------------>ak_usb_plug_change1\n");
	up(&dsd_usb_sem);
#endif
}

static void usb_irq_change_work(struct work_struct *work)
{
	struct ak_usb_plug *p = container_of(work, struct ak_usb_plug, wq);
	int usb_check = p->usb_checking;
    int mode = p->mode;
	int irq = p->irq;

	if (g_set_plug_irq_flag)
		free_irq(irq, pdev);

	g_set_plug_irq_flag = 0;

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK380S) //J150924 || CUR_AK == MODEL_AK320)
    gpio_request(GPIO_USB_ON, "GPIO_USB_ON");
    tcc_gpio_config(GPIO_USB_ON,GPIO_FN(0));
    gpio_direction_output(GPIO_USB_ON, 1);
#endif
    
    ak_usb_plug_change(mode);

    if (usb_check) {
        send_usb_uevent(UCM_CHECKING);        
        strncpy(usb_mode_string, "checking", sizeof(usb_mode_string) - 1);        
		if (request_irq(irq, ak_usb_plug_handler, IRQ_TYPE_EDGE_FALLING, "usb_unplug_detect", pdev))
			printk("request rising edge of irq#%d failed!\n", irq);
		else 
			g_set_plug_irq_flag = 1;
    }
    else {
        /* [downer] A140115 */
        strncpy(usb_mode_string, "unplug", sizeof(usb_mode_string) - 1);        
        cancel_delayed_work_sync(&ak_usb_plug_wq->usb_detect_work);
		if (!ak_otg_disconnect_state())
			send_usb_uevent(UCM_NONE);

		if (request_irq(irq, ak_usb_plug_handler, IRQ_TYPE_EDGE_RISING, "usb_plug_detect", pdev))
			printk("request falling edge of irq#%d failed!\n", irq);
		else 
			g_set_plug_irq_flag = 1;
    }
}

static void tcc_usb30_module_change(struct work_struct *work)
{
    int retval = 0;
    struct usb3_data *p =  container_of(work, struct usb3_data, wq);
    unsigned long flag = p->flag;

    if (set_otg_irq_flag)
        free_irq(INT_EINT9, pdev);
    
    set_otg_irq_flag = 0;

    if (flag) {
        //host -> device
        tcc_drd_vbus_ctrl(0);
        printk("DRD USB DEVICE mode\n");
        strncpy(mode_string, "usb_device", sizeof(mode_string) - 1);

        strncpy(usb_mode_string, "unplug", sizeof(usb_mode_string) - 1);                
        ak_usb_plug_wq->usb_plug_state = 0;
        ak_usb_plug_wq->adapter_plug_state = 0;
        
    } else {
        //device ->host
        tcc_drd_vbus_ctrl(1);
        printk("DRD USB HOST mode\n");
        strncpy(mode_string, "usb_host", sizeof(mode_string) - 1);

        strncpy(usb_mode_string, "otg", sizeof(usb_mode_string) - 1);                
        ak_usb_plug_wq->usb_plug_state = 1;
        ak_usb_plug_wq->adapter_plug_state = 0;
    }

    if(!flag){
        printk("\x1b[1;38mChange Falling -> Rising (Catch Host to Device)\x1b[0m\n");
        retval = request_irq(INT_EINT9, &dwc_usb3_otg_device_dwc_irq, IRQF_SHARED | IRQ_TYPE_EDGE_RISING, "USB30_IRQ11", pdev);
        if (retval) {
            dev_err(&pdev->dev, "request rising edge of irq%d failed!\n", INT_EINT9);
            retval = -EBUSY;
        } else {
            cmode = OTG_HOST;
            set_otg_irq_flag = 1;
        }
    } else {
        printk("\x1b[1;38mChange Rising -> Falling (Catch Device to Host) \x1b[0m\n");
        retval = request_irq(INT_EINT9, &dwc_usb3_otg_host_dwc_irq, IRQF_SHARED | IRQ_TYPE_EDGE_FALLING, "USB30_IRQ5", pdev);
        if (retval) {
            dev_err(&pdev->dev, "request falling edge of irq%d failed!\n", INT_EINT9);
            retval = -EBUSY;
        } else {
            cmode = OTG_DEVICE;
            set_otg_irq_flag = 1;
        }
    }

    return;

End:
    printk("usb3.0 otg mode change fail.\n");
}

static void drd_tcc_set_default_mode(void)
{
	strncpy(def_mode_string, "usb_device", sizeof(def_mode_string) - 1);

    /* [downer] A130812 initial state for USB plug */
    if (ak_check_usb_unplug()) 
		strncpy(plug_mode_string, "usb_unplugged", sizeof(plug_mode_string) - 1);        
    else 
        strncpy(plug_mode_string, "usb_plugged", sizeof(plug_mode_string) - 1);        
}

static struct proc_dir_entry *proc_entry_usb_phy;

static ssize_t usb30_phy_ctrl(struct file *file, const char *buffer, unsigned long count, void *data)
{
    char buf[10], *pbuf, *value;
    unsigned long val;

    printk("%s\n", __func__);
    
    memset(buf, 0, count + 1);

    if (copy_from_user(&buf[0], buffer, count))
        return -EFAULT;

    buf[count - 1] = '\0';
    pbuf = &buf[0];

    value = strsep(&pbuf, " ");
    if (value != NULL) {
        strict_strtoul(value, 0, &val);

        switch(val) {
        case 1:
            printk("PHY OFF\n");
//            dwc_usb3_phy_off();                
            break;
        case 2:
            printk("PHY ON\n");
//          dwc_usb3_phy_on();                
            break;
        default:
            break;
        }
    }

    return count;
}

static void cpu_control(struct work_struct *work)
{
#ifdef CONFIG_MEM_BUS_SCALING
set_cpu_mem_up();
#endif
}

void ak_otg_control(int pwr, int mode)
{
	int irq = gpio_to_irq(GPIO_CHG_DET);
	unsigned long flags;
	spin_lock_irqsave(&lock, flags);
	
    
    if (pwr) {                  /* USB HOST enable */
	    printk("USB HOST Enable!(mode:%d)\n", mode);

//		disable_irq(irq);
		
	    g_usb_mode = OTG_HOST;
	    ak_mtp_path_enable(1);

	    if (mode)
		    tcc_drd_vbus_ctrl(1);

		#if 0 // NOT USE
		gpio_direction_output(GPIO_OTG_DET, 1);
		#endif
		
	    if (!ak_usb_plug_wq->usb_checking) {
			schedule_delayed_work(&ak_usb_plug_wq->otg_device_work, OTG_DETECT_TIMER);
		}
		
	    schedule_delayed_work(&ak_usb_plug_wq->usb_detect_work, 0);
    }
    else {                      /* USB HOST disable */
	    printk("USB HOST Disable!%d\n", g_confirm_mode);

		ak_otg_disconnect_start();
	    g_usb_mode = OTG_DEVICE;

	    tcc_drd_vbus_ctrl(0);

		#if 0 // NOT USE
		gpio_direction_output(GPIO_OTG_DET, 1);	  
		#endif
		
		/* [downer] M160401 */
		if (!g_otg_nopower_dev_check)
			schedule_delayed_work(&ak_usb_plug_wq->usb_detect_work, 0);
		else {
			ak_mtp_path_enable(0);			
			ak_usb_process(0);

			g_otg_nopower_dev_check = 0;
		}
//		enable_irq(irq);
		schedule_work(&cpu_control_wq); /* [downer] A160418 for CPU #1 active */
		
    }

    spin_unlock_irqrestore(&lock, flags);
}
EXPORT_SYMBOL(ak_otg_control);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void drd_otg_early_suspend(struct early_suspend *h)
{
    printk("%s\n", __func__);    
#if 0 //(CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
	//jimi.150507
    if (ak_usb_plug_wq->adapter_plug_state == 1) {
        printk("\x1b[1;39m In charging 1.5A \x1b[0m\n");
    	gpio_set_value(GPIO_CHG_ADP_MODE,1); //High: AC 
    }
#endif


}

static void drd_otg_late_resume(struct early_suspend *h)
{
    printk("%s\n", __func__);
#if 0 //(CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK380S || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
     if (ak_usb_plug_wq->adapter_plug_state == 1) {
        printk("\x1b[1;39m In charging 0.5A \x1b[0m\n");
    	gpio_set_value(GPIO_CHG_ADP_MODE,0); //High: AC 
    }
#endif

    if (g_after_sleep) {
        g_after_sleep = 0;

        msleep_interruptible(10);
        ak_usb_process(1);
    }
}
#endif

static int __devinit drd_tcc_drv_probe(struct platform_device *pdev2)
{
	int retval = 0, irq = -1;
    struct proc_dir_entry *proc_dir;
    
	printk("%s: start\n", __FUNCTION__);

	spin_lock_init(&lock);
	pdev = pdev2;
	cnt = 0;

	printk("\x1b[1;33mfunc: %s, line: %d  \x1b[0m\n",__func__,__LINE__);

    /*
     * [downer] A140119
     */
    wake_lock_init(&usb_wakelock_delay, WAKE_LOCK_SUSPEND, "usb_wakelock");
    
    /*
     * [downer] A131203
     */
#ifdef CONFIG_HAS_EARLYSUSPEND
    drd_early_suspend.suspend = drd_otg_early_suspend;
    drd_early_suspend.resume = drd_otg_late_resume;
    drd_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    register_early_suspend(&drd_early_suspend);
#endif

#if (CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300) //ysyim
	tcc_gpio_config(GPIO_CHG_ADP_MODE, GPIO_FN(0));
	gpio_request(GPIO_CHG_ADP_MODE, "GPIO_CHG_ADP_MODE");
	gpio_direction_output(GPIO_CHG_ADP_MODE, 0); //Hihg:AC, Low:USB

	tcc_gpio_config(ADP_CHG_ICTL, GPIO_FN(0));
	gpio_request(ADP_CHG_ICTL, "ADP_CHG_ICTL");
	gpio_direction_output(ADP_CHG_ICTL, 0); // Hihg:1.5A, Low:1A <-

	tcc_gpio_config(GPIO_CHG_USB_MODE, GPIO_FN(0));
	gpio_request(GPIO_CHG_USB_MODE, "GPIO_CHG_USB_MODE");
	gpio_direction_output(GPIO_CHG_USB_MODE, 1); // Hihg:500mA <- , Low:100mA
#endif

	/* [downer] A160526 for OTG GPIO reset(AK70) */
	gpio_request(GPIO_OTG_5V_EN, "OTG_PWR_EN");
	tcc_gpio_config(GPIO_OTG_5V_EN, GPIO_OUTPUT | GPIO_FN(0));
	gpio_direction_output(GPIO_OTG_5V_EN, 0);
	
	gpio_request(GPIO_OTG_EN, "OTG_EN");
	tcc_gpio_config(GPIO_OTG_EN, GPIO_OUTPUT | GPIO_FN(0));
	gpio_direction_output(GPIO_OTG_EN, 0);
	mdelay(10);
	
	gpio_request(GPIO_USB_PWR_EN, "USB_PWR_EN");
	tcc_gpio_config(GPIO_USB_PWR_EN, GPIO_OUTPUT | GPIO_FN(0));
	gpio_direction_output(GPIO_USB_PWR_EN, 0);
	
#if 0 /* [downer] M150818 USB OTG DETECT 사용안함 */
#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
	drd_otg_mod_wq = (struct usb3_data *)kzalloc(sizeof(struct usb3_data), GFP_KERNEL);
	if (drd_otg_mod_wq == NULL) {
        retval = -ENOMEM;
	}

	INIT_WORK(&drd_otg_mod_wq->wq, tcc_usb30_module_change);

    /*
     * [downer] A131127
     * Register USB OTG detect IRQ
     */
#if(CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK380S || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
	tcc_gpio_config_ext_intr(INT_EINT9, EXTINT_GPIOC_00);
#else
	tcc_gpio_config_ext_intr(INT_EINT9, EXTINT_GPIOG_13);
#endif 
	gpio_request(GPIO_OTG_DET, "OTG DETECT");
	tcc_gpio_config(GPIO_OTG_DET, GPIO_FN(0)|GPIO_PULLUP); //iriver-ysyim
	gpio_direction_input(GPIO_OTG_DET);

    if (gpio_get_value(GPIO_OTG_DET)) {
    	printk("\x1b[1;39mfunc: %s, line: %d Falling \x1b[0m\n",__func__,__LINE__);
        retval = request_irq(INT_EINT9, &dwc_usb3_otg_host_dwc_irq, IRQF_SHARED | IRQ_TYPE_EDGE_FALLING, "USB30_IRQ", pdev);
        if (retval) {
            dev_err(&pdev->dev, "request falling edge of irq%d failed!\n", INT_EINT9);
            retval = -EBUSY;
            goto fail_drd_change;
        } 
        else {
            cmode = g_usb_mode = OTG_DEVICE;
            set_otg_irq_flag = 1;
        }
        strncpy(mode_string, "usb_device", sizeof(mode_string) - 1);        
    }
    else {
    	printk("\x1b[1;39mfunc: %s, line: %d Rising \x1b[0m\n",__func__,__LINE__);
        retval = request_irq(INT_EINT9, &dwc_usb3_otg_device_dwc_irq, IRQF_SHARED | IRQ_TYPE_EDGE_RISING, "USB30_IRQ", pdev);
        if (retval) {
            dev_err(&pdev->dev, "request rising edge of irq%d failed!\n", INT_EINT9);
            retval = -EBUSY;
            goto fail_drd_change;
        } 
        else {
            cmode = g_usb_mode = OTG_HOST;
            set_otg_irq_flag = 1;
        }
        strncpy(mode_string, "usb_host", sizeof(mode_string) - 1);
    }
#endif
#else
    /* [downer] A150818 default mode */

	#if 0 // NOT USE
	gpio_request(GPIO_OTG_DET, "OTG DETECT");
	tcc_gpio_config(GPIO_OTG_DET, GPIO_FN(0)|GPIO_PULLUP); //iriver-ysyim
	gpio_direction_output(GPIO_OTG_DET, 1);
    #endif
	
    cmode = g_usb_mode = OTG_DEVICE;
    strncpy(mode_string, "usb_device", sizeof(mode_string) - 1);    
#endif

#if defined(CONFIG_REGULATOR)
    ldo_usb1v2 = regulator_get(NULL, "vdd_ldo9_usb_1v2");
    if (IS_ERR(ldo_usb1v2)) {
        dev_err(&pdev->dev, "failed to obtain ldo_usb1v2\n");
        ldo_usb1v2 = NULL;
    }

    ldo_usb3v3 = regulator_get(NULL, "vdd_ldo10_usb_3v3");
    if (IS_ERR(ldo_usb3v3)) {
        dev_err(&pdev->dev, "failed to obtain ldo_usb3v3\n");
        ldo_usb3v3 = NULL;
    }

    vdd_io = regulator_get(NULL, "vdd_io");
    if (IS_ERR(vdd_io)) {
        dev_err(&pdev->dev, "failed to obtain vdd_io\n");
        vdd_io = NULL;
    }

    /* [downer] A131206 default enable */
    dwc_usb_regulator_enable();
#endif

	/* [downer] M130806 */
	ak_usb_plug_wq = (struct ak_usb_plug *)kzalloc(sizeof(struct ak_usb_plug), GFP_KERNEL);
	if (ak_usb_plug_wq == NULL) {
		dev_err(&pdev->dev, "failed to allocate ak_usb_plug_wq\n");		
		retval = -ENOMEM;
		goto fail_drd_change;		
	}
	INIT_WORK(&ak_usb_plug_wq->wq, usb_irq_change_work);
    /*
     * [downer] A140107 for adapter
     */
    INIT_DELAYED_WORK(&ak_usb_plug_wq->usb_detect_work, usb_detect_work);

    /*
     * [downer] A150820 for CD ripper check
     */
    INIT_DELAYED_WORK(&ak_usb_plug_wq->otg_device_work, otg_device_work);

	/*
	 * [downer] A160610 for USB process 
	 */
    INIT_DELAYED_WORK(&usb_process_wq, usb_process_work);
	
    /* init setting */
    ak_usb_plug_change(UCM_INIT);
	drd_tcc_set_default_mode();

	/*
     * [downer] A130806 
     * Register USB detect IRQ
     */
	tcc_gpio_config_ext_intr(INT_EINT10, EXTINT_GPIOE_15);
	gpio_request(GPIO_CHG_DET, "USB DETECT");
	gpio_direction_input(GPIO_CHG_DET);

	irq = gpio_to_irq(GPIO_CHG_DET);
	if (ak_check_usb_unplug()) {
		if (request_irq(irq, ak_usb_plug_handler, IRQ_TYPE_EDGE_RISING, "usb_plug_detect", pdev)) {
			dev_err(&pdev->dev, "request falling edge of irq#%d failed!\n", irq);
			goto fail_drd_change;
		}
		else {
			printk("Register falling edge of irq#%d success!\n", irq);
			g_set_plug_irq_flag = 1;
			ak_usb_plug_wq->irq = irq;
		}
	}
	else {
		if (request_irq(irq, ak_usb_plug_handler, IRQ_TYPE_EDGE_FALLING, "usb_unplug_detect", pdev)) {
			dev_err(&pdev->dev, "request rising edge of irq#%d failed!\n", irq);
			goto fail_drd_change;			
		}
		else {
			printk("Register rising edge of irq#%d success!\n", irq);
			g_set_plug_irq_flag	= 1;
			ak_usb_plug_wq->irq = irq;
		}
	}

    proc_dir = proc_mkdir("usb", NULL);
    if (!proc_dir) {
        printk("usb_phy_ctrl proc fail!\n");
    }

    proc_entry_usb_phy = create_proc_entry("phy_ctrl", 0644, proc_dir);
    if (proc_entry_usb_phy) {
        proc_entry_usb_phy->write_proc = usb30_phy_ctrl;
    }
    else {
        printk("create proc phy_ctrl fail!\n");
        remove_proc_entry("phy_ctrl", proc_dir);
    }

    return retval;

fail_drd_change:
    
    printk("fail_drd_change\n");
    
    return retval;
}

static void drd_tcc_remove(struct platform_device *pdev)
{
    tcc_usb30_vbus_exit();

    /* [downer] A140117 */
    cancel_delayed_work_sync(&ak_usb_plug_wq->usb_detect_work);
    cancel_work_sync(&ak_usb_plug_wq->wq);    
    cancel_work_sync(&drd_otg_mod_wq->wq);
    
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&drd_early_suspend);
#endif
}

static int drd_otg_suspend(struct platform_device *dev, pm_message_t state)
{
   PUSBPHYCFG USBPHYCFG = (PUSBPHYCFG)tcc_p2v(HwUSBPHYCFG_BASE);
 
   printk("%d\n", __func__);
   
    BITCLR(USBPHYCFG->UPCR4, Hw20|Hw21);

    /* [downer] A131204 charging 1.5A in adapter mode */
    if (ak_usb_plug_wq->adapter_plug_state == 1) {
        printk("\x1b[1;39m In charging 700mA \x1b[0m\n");

		// ToDo: AK70 CHG_CTL 
    }
   
#ifdef CONFIG_REGULATOR
    if (!get_pcfi_suspend_enable())
        dwc_usb_regulator_disable();
#endif                   
   
    return 0;
}

static int drd_otg_resume(struct platform_device *dev)
{
    /* [downer] A131204 default 1A charging */
    printk("\x1b[1;39m In charging 500mA \x1b[0m\n");

	// ToDo: AK70 CHG_CTL
	
    g_after_sleep = 1;
  
    return 0;
}

struct platform_device drd_otg_device = {
    .name                       = "tcc-drd",
    .id                         = -1,
    //.num_resources  = ARRAY_SIZE(tcc8930_xhci_hs_resources),
    //.resource       = tcc8930_xhci_hs_resources,
    .dev                        = {
        //.dma_mask                      = &tcc8930_device_xhci_dmamask,
        //.coherent_dma_mask     = 0xffffffff,
        //.platform_data = tcc8930_ehci_hs_data,
    },
};

static struct platform_driver drd_otg_driver = {
    .probe              = drd_tcc_drv_probe,
    .remove             = __exit_p(drd_tcc_remove),
    .suspend    = drd_otg_suspend,
    .resume             = drd_otg_resume,
    //.shutdown = NULL,
    .driver = {
        .name    = (char *) "tcc-drd",
        .owner   = THIS_MODULE,
        //.pm            = EHCI_TCC_PMOPS,
    }
};

static int drd_create_device(struct drd_dev *dev)
{
	struct device_attribute **attrs = drd_usb_attributes;
	struct device_attribute *attr;
	int err;

	dev->dev = device_create(drd_class, NULL,
                             MKDEV(0, 0), NULL, "drd0");
	if (IS_ERR(dev->dev))
		return PTR_ERR(dev->dev);

	dev_set_drvdata(dev->dev, dev);

	while ((attr = *attrs++)) {
		err = device_create_file(dev->dev, attr);
		if (err) {
			device_destroy(drd_class, dev->dev->devt);
			return err;
		}
	}
    
	return 0;
}

static int __init drd_otg_init(void)
{
    int retval = 0;
	struct drd_dev *dev;
	int err;

#if(AK_HAVE_XMOS==1)
	//tonny - initialize semaphore
	sema_init(&dsd_usb_sem, 1);
#endif

	drd_class = class_create(THIS_MODULE, "drd_otg");
	if (IS_ERR(drd_class))
		return PTR_ERR(drd_class);

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	err = drd_create_device(dev);
	if (err) {
		class_destroy(drd_class);
		kfree(dev);
		return err;
	}

	_drd_dev = dev;

    retval = platform_device_register(&drd_otg_device);
    if (retval < 0){
        printk("drd device register fail!\n");
        return retval;
    }

    retval = platform_driver_register(&drd_otg_driver);
    if (retval < 0){
        printk("drd drvier register fail!\n");
        return retval;
    }
    return retval;
}
module_init(drd_otg_init);

