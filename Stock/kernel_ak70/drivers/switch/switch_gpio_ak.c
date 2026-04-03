
/*
*  drivers/switch/switch_gpio.c
*
* Copyright (C) 2008 Google, Inc.
* Author: Mike Lockwood <lockwood@android.com>
* modifyed iriver 2013.09.31
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>

#include <asm/io.h>
#include <mach/bsp.h>
#include <mach/irqs.h>
#include <asm/mach-types.h>

#include <linux/kthread.h> 
#include <linux/delay.h>    
#include <mach/board_astell_kern.h>

extern int ak_dock_power_state(void); //refer to board_ak_dock.c
extern int ak_dock_check_device(void);
extern int is_amp_on(void);
extern int is_cradle_on(void);
extern int detect_amp_unbal(void);
extern int detect_amp_bal(void);
extern void ak_route_amp_hp_balanced_out(int aot);
extern inline int get_xmos_is_playing(void);
extern int get_pcm_is_playing(void);

#if OPTICAL_RX_FUNC
#define ENABLE_OPTICAL_RX
#endif


#if (CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
#define ENABLE_BAL_HP
#endif

#ifdef ENABLE_OPTICAL_TX
#define ENABLE_OPTICAL_TX_SWITCH_DEV
#endif

#ifdef BALANCED_OUT_FUNC
#define ENABLE_BAL_HP
#endif


#define AK_BIT_HEADSET             (1 << 0)
#define AK_BIT_HEADSET_NO_MIC      (1 << 1)
#define AK_BIT_USB_HEADSET_ANLG    (1 << 2)
#define AK_BIT_USB_HEADSET_DGTL    (1 << 3)
#define AK_BIT_HDMI_AUDIO          (1 << 4)

#define AK_BIT_FM_HEADSET          (1 << 5)
#define AK_BIT_FM_SPEAKER          (1 << 6)

#define AK_OPTICAL_RX             (1 << 0)
#define AK_OPTICAL_TX             (1 << 0)

#define AK_BAL_HEADSET		  (1 << 0)

#undef ak_dbg(fmt,arg...)  
#if 0
#define ak_dbg(fmt,arg...)  
#else
#define ak_dbg(fmt,arg...)      printk(fmt,##arg)
#endif

struct ak_gpio_switch_data {
	struct switch_dev sdev;
	unsigned gpio;
	unsigned gpio_val;
	int connect_count;
	int disconnect_count;
	int debounce_count;
	int connected_check;
	int connected;


	char gpio_name[32];
	unsigned int state;
	unsigned int invert;
	unsigned int state_old;

	const char *name_on;
	const char *name_off;
	const char *state_on;
	const char *state_off;
	unsigned int state_val;
	unsigned int switch_on_val;


	int irq;
	struct work_struct work;
	struct task_struct *poll_task;
};

struct task_struct *poll_task;

static struct ak_gpio_switch_data *switch_data_h2w;

#if (CUR_AK==MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300  || CUR_AK == MODEL_AK70)
extern CONN_HP_TYPE is_hp_connected(void); 
#endif
#if (CUR_AK==MODEL_AK500N  )
extern void set_hp_connected_type(CONN_HP_TYPE type);
extern CONN_HP_TYPE is_hp_connected(void); 
extern CONN_HP_TYPE g_curr_hp_type;

#if (CUR_AK_REV >= AK500N_HW_ES1)
#define ENABLE_BAL_HP
#endif
#define ENABLE_65PHY_HP
#ifdef ENABLE_65PHY_HP
static struct ak_gpio_switch_data *switch_data_65hp;
#endif
#endif

#ifdef ENABLE_BAL_HP
static struct ak_gpio_switch_data *switch_data_bal_hp;
#endif
#ifdef ENABLE_OPTICAL_RX
static struct ak_gpio_switch_data *switch_data_optical_rx;
#endif

#ifdef ENABLE_OPTICAL_TX
static struct ak_gpio_switch_data *switch_data_optical_tx;
#endif

static struct ak_gpio_switch_data *switch_amp_bal_hp;
static struct ak_gpio_switch_data *switch_amp_unbal_hp;

//static int g_hp_connected = 0;
static int g_hp_connected_jiffies = 0;
static int g_hp_connected = 0;
static int g_65hp_connected = 0;
static int g_bal_hp_connected = 0;
static int g_optical_connected_jiffies = 0;

static int g_hp_disconnected_jiffies = 0;
static int g_optical_disconnected_jiffies = 0;

static DEFINE_MUTEX(ak_gpio_thread_mutex);

//jimi.140724 move to board_astell_kern.c
#if 0 //(CUR_AK==MODEL_AK500N)
static CONN_HP_TYPE g_curr_hp_type = CONN_HP_TYPE_NONE;

void set_hp_connected_type(CONN_HP_TYPE type)
{
	g_curr_hp_type = type;
}
EXPORT_SYMBOL(set_hp_connected_type);

CONN_HP_TYPE is_hp_connected(void)
{
	if(is_selected_balanced())
		g_curr_hp_type = CONN_HP_TYPE_BAL;

	return (g_curr_hp_type);
}
EXPORT_SYMBOL(is_hp_connected);
#endif

#ifdef SUPPORT_EXT_AMP
//ysyim
//AMP의 input 단자 초기화
void stop_amp_switch(void)
{
	switch_set_state(&switch_amp_bal_hp->sdev, 0);
	switch_set_state(&switch_amp_unbal_hp->sdev, 0);
	
	switch_amp_bal_hp->connected_check = 0;
	switch_amp_unbal_hp->connected_check = 0;

    g_hp_connected = 0;         /* [downer] A150714 reset variable */
}
EXPORT_SYMBOL(stop_amp_switch);

//본체의 input 단자 초기화 
void start_amp_switch(void)
{
	switch_data_h2w->state_val =0;
	switch_set_state(&switch_data_h2w->sdev, switch_data_h2w->state_val );	
	switch_data_h2w->connected_check = 0;

	switch_data_bal_hp->state_val =0;
	switch_set_state(&switch_data_bal_hp->sdev, 0);
	switch_data_bal_hp->connected_check = 0;

	switch_data_optical_tx->state_val =0;
	switch_set_state(&switch_data_optical_tx->sdev, 0);
	switch_data_optical_tx->connected_check = 0;
	
}
EXPORT_SYMBOL(start_amp_switch);

static void ak_gpio_switch_work_amp_bal_hp(struct work_struct *work)
{
	struct ak_gpio_switch_data	*switch_data =
		container_of(work, struct ak_gpio_switch_data, work);

	printk("ak_gpio_switch_work_amp_bal_hp\n");

	if(switch_data->connected_check) {
		switch_data->state_val = switch_data->switch_on_val;
	} else {
		switch_data->state_val = 0;
	}	

	CPRINT("AMP BAL ->switch %s : %d",
		switch_data->state_val ? switch_data->name_on : switch_data->name_off,
		switch_data->state_val);

	if(get_xmos_is_playing() || get_pcm_is_playing()){
		//printk("[ysyim]%d\n",__LINE__);
		ak_set_hw_mute(AMT_HP_MUTE,0); 				
	}else{
		//printk("[ysyim]%d\n",__LINE__);
		ak_set_hw_mute(AMT_HP_MUTE,1); 				
	}
		
	switch_set_state(&switch_data->sdev, switch_data->state_val);
}
static void ak_gpio_switch_work_amp_unbal_hp(struct work_struct *work)
{
	struct ak_gpio_switch_data	*switch_data =
		container_of(work, struct ak_gpio_switch_data, work);

	printk("ak_gpio_switch_work_amp_unbal_hp\n");

	if(switch_data->connected_check) {
		switch_data->state_val = switch_data->switch_on_val;
	} else {
		switch_data->state_val = 0;
	}	

	CPRINT("AMP UNBAL ->switch %s : %d",
		switch_data->state_val ? switch_data->name_on : switch_data->name_off,
		switch_data->state_val);

	if(get_xmos_is_playing() || get_pcm_is_playing()){
		//printk("[ysyim]%d\n",__LINE__);
		ak_set_hw_mute(AMT_HP_MUTE,0); 				
	}else{
		//printk("[ysyim]%d\n",__LINE__);
		ak_set_hw_mute(AMT_HP_MUTE,1); 				
	}
		
	switch_set_state(&switch_data->sdev, switch_data->state_val);	
}
#endif

#ifdef ENABLE_BAL_HP
static void ak_gpio_switch_work_bal_hp(struct work_struct *work)
{
	struct ak_gpio_switch_data	*switch_data =
		container_of(work, struct ak_gpio_switch_data, work);

	if(switch_data->connected_check) {
		switch_data->state_val = switch_data->switch_on_val;
	} else {
		switch_data->state_val = 0;
	}	
				
	CPRINT("BAL HP->switch %s : %d",
		switch_data->state_val ? switch_data->name_on : switch_data->name_off,
		switch_data->state_val);

	if(switch_data->state_val==0 && is_hp_connected()==CONN_HP_TYPE_NONE) {
		ak_set_hw_mute(AMT_HP_MUTE,1); 				//2014 8 4 dean 
	} else {
		ak_set_hw_mute(AMT_HP_MUTE,0); 				//2014 8 4 dean 
	}	



	switch_set_state(&switch_data->sdev, switch_data->state_val);
}
#endif

#ifdef ENABLE_65PHY_HP
static void ak_gpio_switch_work_65hp(struct work_struct *work)
{
	struct ak_gpio_switch_data	*switch_data =
		container_of(work, struct ak_gpio_switch_data, work);

	if(switch_data->connected_check) {
		switch_data->state_val = switch_data->switch_on_val;
	} else {
		switch_data->state_val = 0;
	}	
				
	CPRINT("65HP->switch %s : %d",
		switch_data->state_val ? switch_data->name_on : switch_data->name_off,
		switch_data->state_val);

	if(switch_data->state_val==0 && is_hp_connected()==CONN_HP_TYPE_NONE) {
		ak_set_hw_mute(AMT_HP_MUTE,1); 				//2014 8 4 dean 
	} else {
		ak_set_hw_mute(AMT_HP_MUTE,0); 				//2014 8 4 dean 
	}	


	#if BALANCED_OUT_FUNC

	#ifdef MENU_BALANCED_OUT
	if(switch_data->state_val==0) {
		ak_set_snd_pwr_ctl(AK_OPTICAL_TX_POWER,AK_PWR_OFF); 	
		ak_set_audio_path(ADP_NONE);		
		ak_set_optical_tx_status(0);
	}	
	#else
	if(switch_data->state_val==0) {
		ak_route_hp_balanced_out(AOT_BALANCED);
	} else {
		ak_route_hp_balanced_out(AOT_HP);
	}	
	#endif
	
	#endif
	
	switch_set_state(&switch_data->sdev, switch_data->state_val);
}
#endif

static void ak_gpio_switch_work_h2w(struct work_struct *work)
{
	struct ak_gpio_switch_data	*switch_data =
		container_of(work, struct ak_gpio_switch_data, work);

	if(switch_data->connected_check) {
		switch_data->state_val = switch_data->switch_on_val;
	} else {
		switch_data->state_val = 0;
	}	
				
//	CPRINT("UNBAL switch %s : %d",
//		switch_data->state_val ? switch_data->name_on : switch_data->name_off,
//		switch_data->state_val);


#if (CUR_AK==MODEL_AK500N)
	if(switch_data->state_val==0 && is_hp_connected()==CONN_HP_TYPE_NONE) {
		ak_set_hw_mute(AMT_HP_MUTE,1); 				//2014 8 4 dean 
	} else {
		ak_set_hw_mute(AMT_HP_MUTE,0); 				//2014 8 4 dean 
	}	
#endif

	#if BALANCED_OUT_FUNC

	#ifdef MENU_BALANCED_OUT
	if(switch_data->state_val==0) {
		ak_set_snd_pwr_ctl(AK_OPTICAL_TX_POWER,AK_PWR_OFF); 	
		ak_set_audio_path(ADP_NONE);		
		ak_set_optical_tx_status(0);
	}	
	#else
	if(switch_data->state_val==0) {
		ak_route_hp_balanced_out(AOT_BALANCED);
	} else {
		ak_route_hp_balanced_out(AOT_HP);
	}	
	#endif
	
	#endif
	
	switch_set_state(&switch_data->sdev, switch_data->state_val);
}

#ifdef ENABLE_OPTICAL_RX
static void ak_gpio_switch_work_optical_rx(struct work_struct *work)
{
//	int state;
	struct ak_gpio_switch_data	*switch_data =
		container_of(work, struct ak_gpio_switch_data, work);

	if(switch_data->connected_check) {
		switch_data->state_val = switch_data->switch_on_val;
	} else {
		switch_data->state_val = 0;
	}	
				
	CPRINT("OPT_RX->switch %s : %d",
		switch_data->state_val ? switch_data->name_on : switch_data->name_off,
		switch_data->state_val);
	#if 1		
//	CPRINT("%d",data->state_val);
	if(switch_data->gpio==GPIO_OPTIC_RX_DET) {  // fix me later /* 2013-10-31 jhlim*/
		if(switch_data->state_val==0) {
			ak_set_wm8804_spdif_mode(SND_WM8804_PLAY);			
		} else {
			ak_set_wm8804_spdif_mode(SND_WM8804_SPDIF_BYPASS);			
		}
		
	}
	#endif
	switch_set_state(&switch_data->sdev, switch_data->state_val);
	
}
#endif

#if 0 //move to board_astell_kern
int g_optical_tx_connected = 0;

int ak_set_optical_tx_status(int connected)
{
	g_optical_tx_connected = connected;
}
int ak_is_connected_optical_tx(void)
{
	return g_optical_tx_connected;
}
#else
extern int g_optical_tx_connected;
extern int ak_set_optical_tx_status(int connected);
extern int ak_is_connected_optical_tx(void);
#endif

#if (CUR_AK!=MODEL_AK500N)
static void ak_gpio_switch_work_optical_tx(struct work_struct *work)
{
//	int state;
	struct ak_gpio_switch_data	*switch_data =
		container_of(work, struct ak_gpio_switch_data, work);

	if(switch_data->connected_check) {
		switch_data->state_val = switch_data->switch_on_val;
	} else {
		switch_data->state_val = 0;
	}	

	#ifdef ENABLE_OPTICAL_TX

	extern int get_current_usb_device(void);
	
	int usb_con_mode = get_current_usb_device();

	int play_mode = ak_get_internal_play_mode();

	if(switch_data->state_val==0) {
		g_optical_tx_connected = 0;
	} else {
		g_optical_tx_connected = 1;
	}

	CPRINT("OPT_TX->switch %s : %d",
		switch_data->state_val ? switch_data->name_on : switch_data->name_off,
		switch_data->state_val);

	#if 0		
	if(get_current_usb_device()==UCM_PC_FI  || play_mode ==INTERNAL_DSD_PLAY )  {	// PC_FI 일겨우에는 처리 하지  않는다.
		CPRINT("Not operate Optical TX under PC-FI or Int DSD play : %d",switch_data->state_val);
	} else {

		if(switch_data->state_val==0) {
			ak_set_wm8804_spdif_mode(SND_WM8804_PLAY);				
		} else {
			ak_set_wm8804_spdif_mode(SND_WM8804_SPDIF_TX);				
		}	
	}
	#endif
	if(switch_data->state_val==0) {
		ak_set_snd_pwr_ctl(AK_OPTICAL_TX_POWER,AK_PWR_OFF); 	
		ak_set_audio_path(ADP_NONE);		
	} else {
		if(play_mode ==INTERNAL_DSD_PLAY || usb_con_mode==UCM_PC_FI) {

		} else {
            //printk("%s -- ADP_OPTICAL_OUT SET!!!\n", __func__);
			ak_set_audio_path(ADP_OPTICAL_OUT);
			msleep_interruptible(10);
			ak_set_snd_pwr_ctl(AK_OPTICAL_TX_POWER,AK_PWR_ON);		
		}
	}	
	
	
	#endif

	#ifdef ENABLE_OPTICAL_TX_SWITCH_DEV
	switch_set_state(&switch_data->sdev, switch_data->state_val);
	#endif	
}
#endif

static ssize_t switch_ak_gpio_print_state(struct switch_dev *sdev, char *buf)
{
	struct ak_gpio_switch_data	*switch_data =
		container_of(sdev, struct ak_gpio_switch_data, sdev);

//	CPRINT("switch print %s : %d",
//		switch_data->state_val ? switch_data->name_on : switch_data->name_off,
//		switch_data->state_val);
	
    return sprintf(buf, "%d\n", switch_data->state_val);
}




#ifdef FORCE_USE_OPTICAL_RX_FUNC


static DEFINE_MUTEX(ak_gpio_rx_mutex);
static int g_optical_rx_status  = 1;

void toggle_optical_rx_status(void)
{
	mutex_lock(&ak_gpio_rx_mutex);

	if(g_optical_rx_status==0) g_optical_rx_status=1;
	else g_optical_rx_status=0;
	mutex_unlock(&ak_gpio_rx_mutex);
}

int is_jack_connected(struct ak_gpio_switch_data *switch_data)
{
	int status;

	if(switch_data->gpio==GPIO_OPTIC_RX_DET) {
		mutex_lock(&ak_gpio_rx_mutex);
		status = g_optical_rx_status;
		mutex_unlock(&ak_gpio_rx_mutex);
	} else {
		status = gpio_get_value(switch_data->gpio);
	}
	
	if(switch_data->invert) status = (status==1 ? 0 : 1);
	return status;
}
#else
int is_jack_connected(struct ak_gpio_switch_data *switch_data)
{
	int status;
	status = gpio_get_value(switch_data->gpio);

	if(switch_data->invert) status = (status==1 ? 0 : 1);
	return status;
}
//ysyim
int is_amp_jack_connected(struct ak_gpio_switch_data *switch_data)
{
	int status=0;

	if(switch_data->sdev.name == "25hp_amp"){	
		#ifdef SUPPORT_EXT_AMP
		if(detect_amp_bal())
			status = 1;
		else
		#endif	
			status = 0;
	}

	if(switch_data->sdev.name == "h2w_amp"){	
		
		#ifdef SUPPORT_EXT_AMP
		if(detect_amp_unbal())
			status = 1;
		else
		#endif	
			status = 0;
	}

	
	if(switch_data->invert) status = (status==1 ? 0 : 1);
	return status;
}

#endif



#define JACK_OBSERVE_INTERVAL (100)

#define DEBUG_JACK_DETECT

#if 0 //OLD

#define ABS(x)		((x) < 0 ? (-x) : (x))


void process_jack_state(struct ak_gpio_switch_data *switch_data)
{
	#ifdef SUPPORT_EXT_AMP
	if(is_amp_on()){ //ysyim
		if(is_amp_jack_connected(switch_data)) {
			switch_data->disconnect_count = 0;
			if((switch_data->connect_count++ > switch_data->debounce_count) && (switch_data->connected_check==0))  {
				//printk("1.switch_data->sdev.name:%s, %d\n",switch_data->sdev.name,switch_data->connect_count);
				switch_data->connected_check = 1;

				if(detect_amp_unbal()){
					int diff_time;
					mutex_lock(&ak_gpio_thread_mutex);
					g_hp_connected_jiffies = jiffies;
					g_hp_connected = 1;
					mutex_unlock(&ak_gpio_thread_mutex);
					switch_data->connected = 1;
					schedule_work(&switch_data->work); // ak_gpio_switch_work_amp_unbal_hp				
				}	else {
					switch_data->connected = 1;
					schedule_work(&switch_data->work); // ak_gpio_switch_work_amp_bal_hp
				}
			}
		} else {	
		
			if((switch_data->disconnect_count++ > switch_data->debounce_count) && (switch_data->connected_check==1))  {
				//printk("2.switch_data->sdev.name:%s, %d\n",switch_data->sdev.name,switch_data->connect_count);
				switch_data->connected_check = 0;

				if(detect_amp_unbal()) {
					mutex_lock(&ak_gpio_thread_mutex);
					g_hp_disconnected_jiffies =jiffies;
					g_hp_connected = 0;
					mutex_unlock(&ak_gpio_thread_mutex);				

					switch_data->connected = 0;

					schedule_work(&switch_data->work);		
				} 	else {
					switch_data->connected = 0;				
					schedule_work(&switch_data->work);
				}
				
			}
		}
	} else 
	#endif
	
	{
	
		if(is_jack_connected(switch_data)) {
			switch_data->disconnect_count = 0;

				if(switch_data->connect_count++ > switch_data->debounce_count && switch_data->connected_check==0)  {
					switch_data->connected_check = 1;

						if(switch_data->gpio==GPIO_HP_DET) {
							int diff_time;
							mutex_lock(&ak_gpio_thread_mutex);
							g_hp_connected_jiffies = jiffies;
							g_hp_connected = 1;
							mutex_unlock(&ak_gpio_thread_mutex);
						//		CPRINT("switch %s : %d",
							//		switch_data->state_val ? switch_data->name_on : switch_data->name_off,
							//	switch_data->state_val);
							switch_data->connected = 1;
							//printk("ysyim1: %s ",switch_data->sdev.name);
							schedule_work(&switch_data->work);				
						} 
						#ifdef ENABLE_OPTICAL_TX

						else if(switch_data->gpio==GPIO_OPTIC_TX_DET) {
							int diff1;

							g_optical_connected_jiffies = jiffies;
							
							diff1= ABS(g_optical_connected_jiffies-g_hp_connected_jiffies);
							
							//printk("%d\n",diff1);
							//printk("diff1:%d H2W:%d HGPIO:%s TX:%s \n",diff1,g_hp_connected,is_jack_connected(switch_data_h2w)? "O" : "X",is_jack_connected(switch_data_optical_tx) ? "O" : "X");

                            if(!g_hp_connected) 
                            {
#if 0
                                msleep_interruptible(100);
                                printk("diff1:%d H2W:%d HGPIO:%s TX:%s \n",diff1,g_hp_connected,is_jack_connected(switch_data_h2w)? "O" : "X",is_jack_connected(switch_data_optical_tx) ? "O" : "X");
                                msleep_interruptible(100);				
                                printk("diff1:%d H2W:%d HGPIO:%s TX:%s \n",diff1,g_hp_connected,is_jack_connected(switch_data_h2w)? "O" : "X",is_jack_connected(switch_data_optical_tx) ? "O" : "X");
#endif
                                msleep_interruptible(100);
						
                                if(!is_jack_connected(switch_data_h2w)) {
                                    CPRINT("switch %s : %d", switch_data->state_val ? switch_data->name_on : switch_data->name_off, switch_data->state_val);
                                    switch_data->connected = 1;
                                    schedule_work(&switch_data->work);
                                }
                            }
                            
                        } 
#endif
                        else {
                            switch_data->connected = 1;
                            schedule_work(&switch_data->work);
                        }
                }
        } else {
            switch_data->connect_count = 0;
            
            if(switch_data->disconnect_count++ > switch_data->debounce_count && switch_data->connected_check==1)  {
                switch_data->connected_check = 0;

                if(switch_data->gpio==GPIO_HP_DET) {
                    mutex_lock(&ak_gpio_thread_mutex);
                    g_hp_disconnected_jiffies =jiffies;
                    g_hp_connected = 0;
                    mutex_unlock(&ak_gpio_thread_mutex);				

                    switch_data->connected = 0;

                    schedule_work(&switch_data->work);		
                }  
#ifdef ENABLE_OPTICAL_TX

                else if(switch_data->gpio==GPIO_OPTIC_TX_DET) {
                    g_optical_disconnected_jiffies = jiffies;

                    if(switch_data_h2w->connected_check) {

                    } else {
                        int diff1;
					
                        diff1= ABS(g_optical_disconnected_jiffies-g_hp_disconnected_jiffies);
                        //printk("%s %s\n",gpio_get_value(switch_data_h2w->gpio)? "X" : "O",gpio_get_value(switch_data_optical_tx->gpio) ? "X" : "O");

                        //printk("%d\n",diff1);
                        if(diff1 > 70) {
                            switch_data->connected = 0;						
                            schedule_work(&switch_data->work);
                        }
                        //mutex_lock(&ak_gpio_thread_mutex);
                        //mutex_unlock(&ak_gpio_thread_mutex);
                    }
                } 
#endif
			
                else {
                    switch_data->connected = 0;				
                    schedule_work(&switch_data->work);
                }
			
            }
		}
	}
}

int ak_switch_gpio_scan_thread(void *data)
{
 	while(1)
	{
		#ifdef SUPPORT_AK_DOCK
		if(is_cradle_on()){
			//printk("is_cradle_on:%d\n",is_cradle_on());			
			//do not call process_jack_state()
		}
		else 
		#endif

		#ifdef SUPPORT_EXT_AMP
		if(is_amp_on())
		{
			process_jack_state(switch_amp_bal_hp);

			if(!switch_amp_bal_hp->connected) //ysyim-2015.11.06: bal연결상태에서는 unbal 출력 dsiable 시큼 (bal 우선순위)
				process_jack_state(switch_amp_unbal_hp);
		}else 
		#endif

		{
		
		#ifdef ENABLE_BAL_HP
		if(!switch_data_bal_hp->connected) //ysyim-2015.11.06: bal연결상태에서는 unbal 출력 dsiable 시큼 (bal 우선순위)
			process_jack_state(switch_data_h2w);
		#endif
		

#ifdef ENABLE_65PHY_HP
		process_jack_state(switch_data_65hp);
#endif
#ifdef ENABLE_BAL_HP
		process_jack_state(switch_data_bal_hp);
#endif

		#ifdef ENABLE_OPTICAL_TX
		process_jack_state(switch_data_optical_tx);
		#endif

		#ifdef ENABLE_OPTICAL_RX
		process_jack_state(switch_data_optical_rx);
		#endif
		
		#ifdef DEBUG_JACK_DETECT
		printk("%s %s\r",gpio_get_value(switch_data_h2w->gpio)? "X" : "O",gpio_get_value(switch_data_optical_tx->gpio) ? "X" : "O");
		#endif
		
		}
		msleep_interruptible(JACK_OBSERVE_INTERVAL);
		
		if (kthread_should_stop())  {
			printk(KERN_ERR "jack_observe_thread exited !! \n");
			break;
		}
//}		
	}

	return 0;
}
#else 

//
//2016.01  NEW jack detect algorithm for AKJR II
//
#define JACK_CONNECT_BOUNCE (3)  //jack bounce check time: 300ms 

static int g_hpjack_connected = -1;
int g_hpjack_connected_ak70mkii = 0;  // for AK70MKII 20170823
static int g_hpjack_connected_count = 0;
static int g_temp_hp_connected = -1;

#ifdef ENABLE_OPTICAL_TX
static int g_tx_connected = -1;
static int g_tx_connected_count = 0;
static int g_temp_tx_connected = -1;
#endif

#ifdef ENABLE_BAL_HP
static int g_bal_hp_jack_connected = -1;
int g_bal_hp_jack_connected_ak70mkii = 0;  // for AK70MKII 20170823
static int g_bal_hpjack_connected_count = 0;
static int g_temp_bal_hp_connected = -1;
#endif


int ak_switch_gpio_scan_thread(void *data)
{
 	while(1)
	{

		
		g_temp_hp_connected = gpio_get_value(switch_data_h2w->gpio);  // ES Falling

		#ifdef ENABLE_OPTICAL_TX
		g_temp_tx_connected = !gpio_get_value(switch_data_optical_tx->gpio);
		#endif

		#ifdef ENABLE_BAL_HP
		g_temp_bal_hp_connected = gpio_get_value(switch_data_bal_hp->gpio);
		
		#ifdef AK_AUDIO_MODULE_USED
		g_temp_bal_hp_connected = 0;
		#endif		

		#endif
		
		if(g_temp_hp_connected >0) {
			if(g_hpjack_connected_count < 0) g_hpjack_connected_count=0;

			g_hpjack_connected_count++;
		}
		else {
			if(g_hpjack_connected_count > 0) g_hpjack_connected_count=0;

			g_hpjack_connected_count--;
		}

		#ifdef ENABLE_BAL_HP
		if(g_temp_bal_hp_connected >0) {
			if(g_bal_hpjack_connected_count < 0) g_bal_hpjack_connected_count=0;

			g_bal_hpjack_connected_count++;
		}
		else {
			if(g_bal_hpjack_connected_count > 0) g_bal_hpjack_connected_count=0;

			g_bal_hpjack_connected_count--;
		}
		#endif
		

		#ifdef ENABLE_OPTICAL_TX
		if(g_temp_tx_connected >0) {
			
			if(g_tx_connected_count < 0) g_tx_connected_count=0;

			g_tx_connected_count++;
		}
		else {
			if(g_tx_connected_count > 0) g_tx_connected_count=0;

			g_tx_connected_count--;
		}
		#endif


		#ifdef DEBUG_JACK_DETECT
		//printk("HP:%s TX:%s\r",g_temp_hp_connected ? "O" : "X",g_temp_tx_connected ? "O" : "X");
		#endif

		if(g_hpjack_connected_count > JACK_CONNECT_BOUNCE			
			#ifdef ENABLE_OPTICAL_TX
			&& g_tx_connected_count > JACK_CONNECT_BOUNCE
			#endif
		 ) {
			if(g_hpjack_connected != 1) {
				g_hpjack_connected = 1;

				#if (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII)
				if((g_bal_hp_jack_connected_ak70mkii == 0)&&(g_hpjack_connected_ak70mkii == 0))
					ak_route_hp_balanced_out(AOT_HP);
				#endif
				g_hpjack_connected_ak70mkii = 1; 
				CPRINT("HP CONNECTED\n"); 
				//ak_route_hp_balanced_out(AOT_HP);

				switch_data_h2w->connected_check = 1;
				ak_gpio_switch_work_h2w(&switch_data_h2w->work);			
			}
		} 
		else if(g_hpjack_connected_count < -JACK_CONNECT_BOUNCE
			#ifdef ENABLE_OPTICAL_TX
			&& g_tx_connected_count < -JACK_CONNECT_BOUNCE
			#endif
		) {

			if(g_hpjack_connected == 1) {
				g_hpjack_connected = 0;
				g_hpjack_connected_ak70mkii = 0;
				CPRINT("HP DISCONNECTED\n"); 

				switch_data_h2w->connected_check = 0;
				ak_gpio_switch_work_h2w(&switch_data_h2w->work);			
			}
			
			#ifdef ENABLE_OPTICAL_TX
			if(g_tx_connected == 1) {
				g_tx_connected = 0;

				CPRINT("TX DISCONNECTED\n"); 
				switch_data_optical_tx->connected_check = 0;
				ak_gpio_switch_work_optical_tx(&switch_data_optical_tx->work);			
			}
			#endif
			
		}
		#ifdef ENABLE_OPTICAL_TX	
		else if(g_hpjack_connected_count < -JACK_CONNECT_BOUNCE && g_tx_connected_count > -JACK_CONNECT_BOUNCE) 
		{
			if(g_tx_connected != 1) {
				g_tx_connected = 1;
				CPRINT("TX CONNECTED\n"); 
				switch_data_optical_tx->connected_check = 1;
				ak_gpio_switch_work_optical_tx(&switch_data_optical_tx->work);			
			}

		}
		#endif

		#ifdef ENABLE_BAL_HP
		if(g_bal_hpjack_connected_count > JACK_CONNECT_BOUNCE) {
			if(g_bal_hp_jack_connected != 1) {
				g_bal_hp_jack_connected = 1;
				g_bal_hp_jack_connected_ak70mkii = 1;
				CPRINT("BAL HP CONNECTED\n"); 
				//ak_route_hp_balanced_out(AOT_BALANCED);

				switch_data_bal_hp->connected_check = 1;
				ak_gpio_switch_work_h2w(&switch_data_bal_hp->work);			
			}
		} 
		else if(g_bal_hpjack_connected_count < -JACK_CONNECT_BOUNCE	) {

			if(g_bal_hp_jack_connected == 1) {
				g_bal_hp_jack_connected = 0;
				g_bal_hp_jack_connected_ak70mkii = 0;

				CPRINT("BAL HP DISCONNECTED\n"); 

				switch_data_bal_hp->connected_check = 0;
				ak_gpio_switch_work_h2w(&switch_data_bal_hp->work);			
			}		
		}
		#endif

		msleep_interruptible(JACK_OBSERVE_INTERVAL);
		
		if (kthread_should_stop())  {
			printk(KERN_ERR "jack_observe_thread exited !! \n");
			break;
		}
	}

	return 0;
}

#endif
static int ak_gpio_switch_probe(struct platform_device *pdev)
{
	
	int ret = 0;
	
    ak_dbg("gpio_switch_probe()...in \n\n");
	
	
	/*
	
	  Headset Detect
	  
	*/
	switch_data_h2w = kzalloc(sizeof(struct ak_gpio_switch_data), GFP_KERNEL);
	if (!switch_data_h2w) {
		return -ENOMEM;
	}
	memset(switch_data_h2w,0x0,sizeof(struct ak_gpio_switch_data));
    switch_data_h2w->sdev.name = "h2w";
    switch_data_h2w->state_val = 0;
	switch_data_h2w->switch_on_val = AK_BIT_HEADSET;
    switch_data_h2w->name_on = "Headset connected";
    switch_data_h2w->name_off = "Headset disconnected";
    switch_data_h2w->state_on = "2";
    switch_data_h2w->state_off = "0";
	switch_data_h2w->sdev.print_state = switch_ak_gpio_print_state;
#if (CUR_AK==MODEL_AK500N)
	switch_data_h2w->gpio = GPIO_35HP_DET;
#else
	switch_data_h2w->gpio = GPIO_HP_DET;
#endif
	switch_data_h2w->invert = 1;
	switch_data_h2w->debounce_count = 5; 
		
	sprintf(switch_data_h2w->gpio_name,"GPIO_SWITCH%d",switch_data_h2w->gpio);
	gpio_request(switch_data_h2w->gpio, switch_data_h2w->gpio_name  );
	tcc_gpio_config(switch_data_h2w->gpio, GPIO_FN(0));
	gpio_direction_input(switch_data_h2w->gpio);


    ret = switch_dev_register(&switch_data_h2w->sdev);
	if (ret < 0) {
		goto err_switch_dev_register;
	}
	INIT_WORK(&switch_data_h2w->work, ak_gpio_switch_work_h2w);

#ifdef ENABLE_65PHY_HP
	switch_data_65hp = kzalloc(sizeof(struct ak_gpio_switch_data), GFP_KERNEL);
	if (!switch_data_65hp) {
		return -ENOMEM;
	}
	memset(switch_data_65hp,0x0,sizeof(struct ak_gpio_switch_data));
    switch_data_65hp->sdev.name = "65hp";
    switch_data_65hp->state_val = 0;
	switch_data_65hp->switch_on_val = AK_BIT_HEADSET;
    switch_data_65hp->name_on = "65 HP connected";
    switch_data_65hp->name_off = "65 HP disconnected";
    switch_data_65hp->state_on = "2";
    switch_data_65hp->state_off = "0";
    switch_data_65hp->sdev.print_state = switch_ak_gpio_print_state;
    switch_data_65hp->gpio = GPIO_65HP_DET;
    switch_data_65hp->invert = 0;
    switch_data_65hp->debounce_count = 5; 

    sprintf(switch_data_65hp->gpio_name,"GPIO_SWITCH%d",switch_data_65hp->gpio);
    gpio_request(switch_data_65hp->gpio, switch_data_65hp->gpio_name  );
    tcc_gpio_config(switch_data_65hp->gpio, GPIO_FN(0));
    gpio_direction_input(switch_data_65hp->gpio);

	
    ret = switch_dev_register(&switch_data_65hp->sdev);
	if (ret < 0) {
		goto err_switch_dev_register;
	}
	INIT_WORK(&switch_data_65hp->work, ak_gpio_switch_work_65hp);


#endif

#ifdef ENABLE_BAL_HP
	switch_data_bal_hp = kzalloc(sizeof(struct ak_gpio_switch_data), GFP_KERNEL);
	if (!switch_data_bal_hp) {
		return -ENOMEM;
	}
	memset(switch_data_bal_hp,0x0,sizeof(struct ak_gpio_switch_data));
    switch_data_bal_hp->sdev.name = "25hp";
    switch_data_bal_hp->state_val = 0;
	switch_data_bal_hp->switch_on_val = AK_BAL_HEADSET;
    switch_data_bal_hp->name_on = "Balanced HP connected";
    switch_data_bal_hp->name_off = "Balanced HP disconnected";
    switch_data_bal_hp->state_on = "1";
    switch_data_bal_hp->state_off = "0";
    switch_data_bal_hp->sdev.print_state = switch_ak_gpio_print_state;
    switch_data_bal_hp->gpio = GPIO_BAL_DET;
    switch_data_bal_hp->invert = 0;
    switch_data_bal_hp->debounce_count = 5; 

    sprintf(switch_data_bal_hp->gpio_name,"GPIO_SWITCH%d",switch_data_bal_hp->gpio);
    gpio_request(switch_data_bal_hp->gpio, switch_data_bal_hp->gpio_name  );
    tcc_gpio_config(switch_data_bal_hp->gpio, GPIO_FN(0));
    gpio_direction_input(switch_data_bal_hp->gpio);

	
    ret = switch_dev_register(&switch_data_bal_hp->sdev);
	if (ret < 0) {
		goto err_switch_dev_register;
	}
	INIT_WORK(&switch_data_bal_hp->work, ak_gpio_switch_work_bal_hp);
#endif

	//AMP BAL
	#ifdef SUPPORT_EXT_AMP

	switch_amp_bal_hp = kzalloc(sizeof(struct ak_gpio_switch_data), GFP_KERNEL);
	if (!switch_amp_bal_hp) {
		return -ENOMEM;
	}
	memset(switch_amp_bal_hp,0x0,sizeof(struct ak_gpio_switch_data));
    switch_amp_bal_hp->sdev.name = "25hp_amp";
    switch_amp_bal_hp->state_val = 0;
	switch_amp_bal_hp->switch_on_val = AK_BAL_HEADSET;
    switch_amp_bal_hp->name_on = "AMP BAL connected";
    switch_amp_bal_hp->name_off = "AMP BAL disconnected";
    switch_amp_bal_hp->state_on = "1";
    switch_amp_bal_hp->state_off = "0";
    //switch_amp_bal_hp->sdev.print_state = switch_ak_gpio_print_state;
   // switch_amp_bal_hp->gpio = GPIO_BAL_DET;
    switch_amp_bal_hp->invert = 0;
    switch_amp_bal_hp->debounce_count = 5; 
	
    ret = switch_dev_register(&switch_amp_bal_hp->sdev);
	if (ret < 0) {
		goto err_switch_dev_register;
	}

	INIT_WORK(&switch_amp_bal_hp->work, ak_gpio_switch_work_amp_bal_hp);

	//AMP UNBAL
	switch_amp_unbal_hp = kzalloc(sizeof(struct ak_gpio_switch_data), GFP_KERNEL);
	if (!switch_amp_unbal_hp) {
		return -ENOMEM;
	}
	memset(switch_amp_unbal_hp,0x0,sizeof(struct ak_gpio_switch_data));
    switch_amp_unbal_hp->sdev.name = "h2w_amp";
    switch_amp_unbal_hp->state_val = 0;
	switch_amp_unbal_hp->switch_on_val = AK_BIT_HEADSET;
    switch_amp_unbal_hp->name_on = "AMP UNBAL connected";
    switch_amp_unbal_hp->name_off = "AMP UNBAL disconnected";
    switch_amp_unbal_hp->state_on = "1";
    switch_amp_unbal_hp->state_off = "0";
    //switch_amp_unbal_hp->sdev.print_state = switch_ak_gpio_print_state;
    //switch_amp_unbal_hp->gpio = GPIO_BAL_DET;
    switch_amp_unbal_hp->invert = 0;
    switch_amp_unbal_hp->debounce_count = 5; 
	
    ret = switch_dev_register(&switch_amp_unbal_hp->sdev);
	if (ret < 0) {
		goto err_switch_dev_register;
	}
	INIT_WORK(&switch_amp_unbal_hp->work, ak_gpio_switch_work_amp_unbal_hp);
	#endif
	

	/*
	
	  Optical Rx Detect
	  
	*/
	
	#ifdef ENABLE_OPTICAL_RX	
	switch_data_optical_rx = kzalloc(sizeof(struct ak_gpio_switch_data), GFP_KERNEL);
	if (!switch_data_optical_rx) {
		return -ENOMEM;
	}
	
	memset(switch_data_optical_rx,0x0,sizeof(struct ak_gpio_switch_data));
    switch_data_optical_rx->sdev.name = "optical_rx";
    switch_data_optical_rx->state_val = 0;
	switch_data_optical_rx->switch_on_val = AK_OPTICAL_RX;
    switch_data_optical_rx->name_on = "OPTICAL RX connected";
    switch_data_optical_rx->name_off = "OPTICAL RX disconnected";
    switch_data_optical_rx->state_on = "1";
    switch_data_optical_rx->state_off = "0";
	switch_data_optical_rx->sdev.print_state = switch_ak_gpio_print_state;
	switch_data_optical_rx->gpio = GPIO_OPTIC_RX_DET;	
	switch_data_optical_rx->invert = 1;
	switch_data_optical_rx->debounce_count = 3; 
	
	sprintf(switch_data_optical_rx->gpio_name,"GPIO_SWITCH%d",switch_data_optical_rx->gpio);
	gpio_request(switch_data_optical_rx->gpio, switch_data_optical_rx->gpio_name  );
	tcc_gpio_config(switch_data_optical_rx->gpio, GPIO_FN(0));
	gpio_direction_input(switch_data_optical_rx->gpio);

	
	
    ret = switch_dev_register(&switch_data_optical_rx->sdev);
	if (ret < 0) {
		goto err_switch_dev_register;
	}
	INIT_WORK(&switch_data_optical_rx->work, ak_gpio_switch_work_optical_rx);
	#endif

	
	/*
	
	  Optical Tx Detect
	  
	*/
	
	
	
	#ifdef  ENABLE_OPTICAL_TX
	switch_data_optical_tx = kzalloc(sizeof(struct ak_gpio_switch_data), GFP_KERNEL);
	if (!switch_data_optical_tx) {
		return -ENOMEM;
	}
	
	memset(switch_data_optical_tx,0x0,sizeof(struct ak_gpio_switch_data));
    switch_data_optical_tx->sdev.name = "optical_tx";
    switch_data_optical_tx->state_val = 0;
	switch_data_optical_tx->switch_on_val = AK_OPTICAL_TX;
    switch_data_optical_tx->name_on = "OPTICAL TX connected";
    switch_data_optical_tx->name_off = "OPTICAL TX disconnected";
    switch_data_optical_tx->state_on = "1";
    switch_data_optical_tx->state_off = "0";
	#ifdef ENABLE_OPTICAL_TX_SWITCH_DEV
	switch_data_optical_tx->sdev.print_state = switch_ak_gpio_print_state;
	#endif
	
	switch_data_optical_tx->gpio = GPIO_OPTIC_TX_DET;	
	switch_data_optical_tx->invert = 1;
	switch_data_optical_tx->debounce_count = 7; 
	
	
	sprintf(switch_data_optical_tx->gpio_name,"GPIO_SWITCH%d",switch_data_optical_tx->gpio);
	gpio_request(switch_data_optical_tx->gpio, switch_data_optical_tx->gpio_name  );
	tcc_gpio_config(switch_data_optical_tx->gpio, GPIO_FN(0));
	gpio_direction_input(switch_data_optical_tx->gpio);

	
	#ifdef ENABLE_OPTICAL_TX_SWITCH_DEV
    ret = switch_dev_register(&switch_data_optical_tx->sdev);
	if (ret < 0) {
		goto err_switch_dev_register;
	}
	#endif
	
#if (CUR_AK!=MODEL_AK500N)
	INIT_WORK(&switch_data_optical_tx->work, ak_gpio_switch_work_optical_tx);
#endif
	#endif
	
	
	poll_task = kthread_create(ak_switch_gpio_scan_thread, switch_data_h2w, "headset-poll-thread");
    if (IS_ERR(poll_task)) {
        printk("\ntcc-gpio-poll-thread create error: %p\n", poll_task);
      //  kfree(switch_data_h2w);
        return (-EINVAL);
    }
    wake_up_process(poll_task);

	
    ak_dbg("gpio_switch_probe()...out \n\n");
	
	return 0;
	
err_request_irq:
err_detect_irq_num_failed:
err_set_gpio_input:
	gpio_free(switch_data_h2w->gpio);

#ifdef ENABLE_BAL_HP
	gpio_free(switch_data_bal_hp->gpio);
#endif

#ifdef ENABLE_65PHY_HP
	gpio_free(switch_data_65hp->gpio);	
#endif
	#ifdef ENABLE_OPTICAL_RX
	gpio_free(switch_data_optical_rx->gpio);
	#endif
	
	#ifdef ENABLE_OPTICAL_TX_SWITCH_DEV
	gpio_free(switch_data_optical_tx->gpio);
	#endif
	
err_request_gpio:
    switch_dev_unregister(&switch_data_h2w->sdev);

#ifdef ENABLE_BAL_HP
    switch_dev_unregister(&switch_data_bal_hp->sdev);	
#endif

#ifdef ENABLE_65PHY_HP
    switch_dev_unregister(&switch_data_65hp->sdev);
#endif

	#ifdef ENABLE_OPTICAL_RX
    switch_dev_unregister(&switch_data_optical_rx->sdev);
	#endif
	
	#ifdef ENABLE_OPTICAL_TX_SWITCH_DEV
    switch_dev_unregister(&switch_data_optical_tx->sdev);
	#endif
	
err_switch_dev_register:
	kfree(switch_data_h2w);
#ifdef ENABLE_BAL_HP
	kfree(switch_data_bal_hp);
#endif

#ifdef ENABLE_65PHY_HP
	kfree(switch_data_65hp);
#endif

	#ifdef ENABLE_OPTICAL_RX
	kfree(switch_data_optical_rx);
	#endif
	
	#ifdef ENABLE_OPTICAL_TX_SWITCH_DEV
	kfree(switch_data_optical_tx);
	#endif
	return ret;
}

static int __devexit ak_gpio_switch_remove(struct platform_device *pdev)
{
	struct ak_gpio_switch_data *switch_data = platform_get_drvdata(pdev);
	
	cancel_work_sync(&switch_data->work);
	gpio_free(switch_data->gpio);
    switch_dev_unregister(&switch_data->sdev);
	kfree(switch_data);
	
	return 0;
}

static struct platform_driver ak_gpio_switch_driver = {
	.probe		= ak_gpio_switch_probe,
		.remove		= __devexit_p(ak_gpio_switch_remove),
		.driver		= {
		.name	= "ak-switch-gpio",
			.owner	= THIS_MODULE,
	},
};

static int __init ak_gpio_switch_init(void)
{
	
	
    ak_dbg("\n%s()...\n\n", __func__);
	return platform_driver_register(&ak_gpio_switch_driver);
}

static void __exit ak_gpio_switch_exit(void)
{
	platform_driver_unregister(&ak_gpio_switch_driver);
}

module_init(ak_gpio_switch_init);
module_exit(ak_gpio_switch_exit);



MODULE_AUTHOR("<iriver>");
MODULE_DESCRIPTION("Astell & Kern GPIO Switch driver");
MODULE_LICENSE("GPL");
