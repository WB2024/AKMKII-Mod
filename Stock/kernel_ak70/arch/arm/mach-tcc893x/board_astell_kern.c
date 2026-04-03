/*

  astell & kern port config function define.

*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/wlan_plat.h>

#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <asm/io.h>

#include "devices.h"

#include <mach/bsp.h>
#include <mach/gpio.h>
#include <mach/ak_gpio_def.h>
#include <mach/board_astell_kern.h>
#include <mach/board-tcc8930.h>
#include <mach/tca_tco.h>

//iriver-ysyim
#define DOCK_MUTE_TIMER		msecs_to_jiffies(1000*1) // 1 sec  
struct delayed_work dock_mute_work;
static void dock_mute_control_work(struct work_struct *work);
	
extern int ak_dock_power_state(void); //refer to board_ak_dock.c
extern int ak_dock_check_device(void);;
extern void start_cradle(void);
extern void dock_mute(int val); //iriver-ysyim
extern void amp_bal_unbal_select(int val);
void set_pcm_is_playing(int playing); //iriver-ysyim
extern void cradle_mute(int val);
extern void amp_unbal_mute(int val);
extern void amp_bal_mute(int val);
extern void CS3318_volume_mute(int val);
extern void ak_dock_led_control(int val);

// [downer] A160523
extern int ak_check_usb_plug(void);
extern int ak_check_usb_unplug(void);

static int g_amp_led_status=1;
int g_force_charge_det_ctl=0; // 공정툴에서 만 설정 ..
int g_stradeum_flag=0; // stradeum용 펌웨어 선택

EXPORT_SYMBOL(g_force_charge_det_ctl);
EXPORT_SYMBOL(g_stradeum_flag);

/* [downer] A150714 */
extern int is_amp_on(void);
extern int is_cradle_on(void);


//2015.4.25-iriver.ysyim
void gpio_direction_output2(unsigned gpio_pin, int val)
{

	//printk("GPIO_%d=%d ,dock:%d\n",gpio_pin,val,ak_dock_power_state() );
	//iriver-ysyim
	//AMP, REC의 Power가  OFF 되어있는 경우에 만  본체의 BAL/UNBAL/OPTICAL Mute를 동작시킴
	#ifdef SUPPORT_AK_DOCK
	//if((ak_dock_power_state() == DOCK_PWR_OFF)  && (ak_dock_check_device()!=DEVICE_AMP)){
	if((ak_dock_power_state() == DOCK_PWR_OFF) ){
		
		gpio_direction_output(gpio_pin, val);
	}

	#else
		gpio_direction_output(gpio_pin, val);
	#endif
}

#if(CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300 || CUR_AK == MODEL_AK70)
CONN_HP_TYPE g_curr_hp_type = CONN_HP_TYPE_NONE;
CONN_HP_TYPE is_hp_connected(void)
{
	if(is_selected_balanced())
		g_curr_hp_type = CONN_HP_TYPE_BAL;

	return (g_curr_hp_type);
}
EXPORT_SYMBOL(is_hp_connected);

extern void fix_cpu_clock(bool val);
#endif

//jimi.140806
#if(CUR_AK == MODEL_AK500N)
extern void wheel_led_control(int volume); //iriver-ysyim 2014.09.12
extern int get_battery_level(void); //iriver-ysyim 2014.09.16

int g_cur_lvol = 0;
int g_cur_rvol = 0;
int g_optical_rx_status = 0; //add dean
int g_play_path;
unsigned int g_barled_brightness = 0;

CONN_HP_TYPE g_curr_hp_type = CONN_HP_TYPE_NONE;
EXPORT_SYMBOL(g_curr_hp_type);

AUDIO_OUT_TYPE g_curr_audio_out_type = AUDIO_OUT_TYPE_NONE;
AUDIO_IN_TYPE g_curr_audio_in_type = AUDIO_IN_TYPE_NONE;

void set_audio_out_type(AUDIO_OUT_TYPE type)
{
	if(type >= AUDIO_OUT_TYPE_NONE  && type < AUDIO_OUT_TYPE_END)
		g_curr_audio_out_type = type;
	else {
		printk("Error! unknown  audio output type!!!(%d)\n", type);
		g_curr_audio_out_type = AUDIO_OUT_TYPE_NONE;
	}
}

AUDIO_OUT_TYPE get_cur_audio_out_type(void)
{
	return g_curr_audio_out_type;
}
EXPORT_SYMBOL(get_cur_audio_out_type);

void set_audio_in_type(AUDIO_IN_TYPE type)
{
	if(type >= AUDIO_IN_TYPE_NONE  && type < AUDIO_IN_TYPE_END)
		g_curr_audio_in_type = type;
	else {
		printk("Error! unknown  audio input type!!!(%d)\n", type);
		g_curr_audio_in_type = AUDIO_IN_TYPE_NONE;
	}
}

AUDIO_IN_TYPE get_cur_audio_in_type(void)
{
	return g_curr_audio_in_type;
}
EXPORT_SYMBOL(get_cur_audio_in_type);



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

int get_current_volume(void)
{
    return g_cur_lvol;
}
EXPORT_SYMBOL(get_current_volume);

void set_ak500n_volume(int lvol, int rvol) //jimi.140912
{
}
EXPORT_SYMBOL(set_ak500n_volume);

void resume_ak500n_volume()
{
}

/* [downer] A140919 for AC connect wq */
struct delayed_work ac_work;
#define AC_CONNECT_TIMER        msecs_to_jiffies(7000)
#ifdef REDUCE_EXT_AMP_POPNOISE
static int g_relay_on_flag=0;
void set_all_relay_on(void)
{
	gpio_request(BAL_RELAY_PWR_EN, "BAL_RELAY_PWR_EN");
	tcc_gpio_config(BAL_RELAY_PWR_EN,GPIO_FN(0));
	gpio_direction_output(BAL_RELAY_PWR_EN,1);		//D5 low relay power on

	gpio_request(GPIO_XLR_RELAY_PWR_EN, "GPIO_XLR_RELAY_PWR_EN");
	tcc_gpio_config(GPIO_XLR_RELAY_PWR_EN,GPIO_FN(0));
	gpio_direction_output(GPIO_XLR_RELAY_PWR_EN,1);		//F2 high relay power on

	gpio_request(VAR_XLR_RELAY_PWR_EN, "VAR_XLR_RELAY_PWR_EN");
	tcc_gpio_config(VAR_XLR_RELAY_PWR_EN,GPIO_FN(0));
	gpio_direction_output(VAR_XLR_RELAY_PWR_EN,1);		//D3 high	audio out 2

	gpio_request(VAR_BAL_RELAY_PWR_EN, "VAR_BAL_RELAY_PWR_EN");
	tcc_gpio_config(VAR_BAL_RELAY_PWR_EN,GPIO_FN(0));
	gpio_direction_output(VAR_BAL_RELAY_PWR_EN,1);		//D4 low power off

	gpio_request(GPIO_BAL_MUTE, "GPIO_BAL_MUTE");
	tcc_gpio_config(GPIO_BAL_MUTE,GPIO_FN(0));
	gpio_direction_output(GPIO_BAL_MUTE,1);		//A1 high relay on

	gpio_request(GPIO_XLR_MUTE, "GPIO_XLR_MUTE");
	tcc_gpio_config(GPIO_XLR_MUTE,GPIO_FN(0));
	gpio_direction_output(GPIO_XLR_MUTE,1);		//A0 high relay on

	gpio_request(GPIO_VAR_XLR_MUTE, "GPIO_VAR_XLR_MUTE");
	tcc_gpio_config(GPIO_VAR_XLR_MUTE,GPIO_FN(0));
	gpio_direction_output(GPIO_VAR_XLR_MUTE,1);		//A2 high relay on

	gpio_request(GPIO_VAR_BAL_MUTE, "GPIO_VAR_BAL_MUTE");
	tcc_gpio_config(GPIO_VAR_BAL_MUTE,GPIO_FN(0));
	gpio_direction_output(GPIO_VAR_BAL_MUTE,1);		//A3 high relay on

	g_relay_on_flag=1;
}
#endif


#endif

//jimi.0912.move from switch_gpio_ak.
int g_optical_tx_connected = 0;

int ak_set_optical_tx_status(int connected)
{
	g_optical_tx_connected = connected;
}
int ak_is_connected_optical_tx(void)
{
	return g_optical_tx_connected;
}
EXPORT_SYMBOL(ak_is_connected_optical_tx);

/* current usb connection state */
int usb_conn_mode = UCM_MTP;//UCM_NONE;

/* current internal-play state */
int internal_play_mode = INTERNAL_NONE_PLAY;

#if(AK_HAVE_XMOS==1)
extern struct semaphore dsd_usb_sem;
#endif

// 2015.06.29
struct semaphore pwr_ctl_sem;


USB_CONN_MODE ak_get_current_usb_mode(void);
INTERNAL_PLAY_MODE ak_get_internal_play_mode(void);
void ak_set_internal_play_mode(INTERNAL_PLAY_MODE mode);
extern void set_xmos_pwr_str(int val);
extern int get_current_usb_device(void);

int get_boot_complete(void);

static int g_audio_path = ADP_NONE;

int ak_get_audio_path(void)
{
	return	g_audio_path;
}

void ak_set_audio_path(AUDIO_DATA_PATH_TYPE e_type)
{
	if(g_audio_path == e_type) {
		//CPRINT("Already set_autio_path : %d ",e_type);
		return ;
	}
	DBG_PRINT("GPIO","%d\n",e_type);

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)    
	gpio_request(GPIO_PLAY_EN, "GPIO_PLAY_EN");
	gpio_request(GPIO_OPTIC_TX_EN, "GPIO_OPTIC_TX_EN");
	gpio_request(GPIO_OPTIC_PLAY_EN, "GPIO_OPTIC_PLAY_EN");

	tcc_gpio_config(GPIO_PLAY_EN,GPIO_FN(0));
	tcc_gpio_config(GPIO_OPTIC_TX_EN,GPIO_FN(0));
	tcc_gpio_config(GPIO_OPTIC_PLAY_EN,GPIO_FN(0));
#endif
    
#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)
#if( CUR_AK == MODEL_AK240)  // NO AK380
	gpio_request(GPIO_XMOS_PLAY_EN, "GPIO_XMOS_PLAY_EN");
	tcc_gpio_config(GPIO_XMOS_PLAY_EN,GPIO_FN(0));
	gpio_direction_output(GPIO_XMOS_PLAY_EN,0);
#endif

//pio_request(GPIO_XMOS_I2S_EN, "GPIO_XMOS_I2S_EN");
	gpio_request(GPIO_CPU_I2S_EN, "GPIO_CPU_I2S_EN");
	
//cc_gpio_config(GPIO_XMOS_I2S_EN,GPIO_FN(0));
	tcc_gpio_config(GPIO_CPU_I2S_EN,GPIO_FN(0));

//pio_direction_output(GPIO_XMOS_I2S_EN,0);

#endif
	g_audio_path = e_type;

	// LOW ACTIVE
	switch(e_type)  {
	case ADP_PLAYBACK:
#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)
		gpio_direction_output(GPIO_CPU_I2S_EN,0);

		gpio_direction_output(GPIO_PLAY_EN,0);
		gpio_direction_output(GPIO_OPTIC_PLAY_EN,1);
		gpio_direction_output(GPIO_OPTIC_TX_EN,1);
#endif
        
		break;
#if OPTICAL_RX_FUNC
	case ADP_OPTICAL_IN:
#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)
		gpio_direction_output(GPIO_CPU_I2S_EN,0);
		gpio_direction_output(GPIO_PLAY_EN,1);
		gpio_direction_output(GPIO_OPTIC_PLAY_EN,0);
		gpio_direction_output(GPIO_OPTIC_TX_EN,1);
#endif       
		break;
#endif

	case ADP_OPTICAL_OUT:
#if (CUR_AK == MODEL_AK240 ||  CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)
		gpio_direction_output(GPIO_CPU_I2S_EN,0);
		gpio_direction_output(GPIO_PLAY_EN,1);
		gpio_direction_output(GPIO_OPTIC_PLAY_EN,1);
		gpio_direction_output(GPIO_OPTIC_TX_EN,0);
#endif        
		break;

#if (CUR_AK == MODEL_AK240|| CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)
	case ADP_USB_DAC:
		gpio_direction_output(GPIO_CPU_I2S_EN,1);
		gpio_direction_output(GPIO_PLAY_EN,1);
		gpio_direction_output(GPIO_OPTIC_PLAY_EN,1);
		gpio_direction_output(GPIO_OPTIC_TX_EN,1);
		break;
#endif
	}
}
EXPORT_SYMBOL(ak_set_audio_path);

int g_mute_status = -1;

void ak_route_hp_balanced_out2(int aot)
{
	gpio_request(GPIO_BAL_UNBAL_SEL, "GPIO_BAL_UNBAL_SEL");
	tcc_gpio_config(GPIO_BAL_UNBAL_SEL, GPIO_FN(0));

	if(aot==AOT_HP) {
		gpio_direction_output(GPIO_BAL_UNBAL_SEL, 0);
		//CPRINT("Hp Connected\n");
	} else {
		gpio_direction_output(GPIO_BAL_UNBAL_SEL,1);
		//CPRINT("Balanced Out Connected\n");
	}

}



#if (CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300 || CUR_AK == MODEL_AK70)

#if 1 // OLD

#ifdef REDUCE_GAP
static int g_hw_mute_status = -1;
#endif

void ak_set_hw_mute(AUDIO_HW_MUTE_TYPE e_type,int onoff)
{
	switch(e_type)  {
	case AMT_HP_MUTE:
       #ifdef REDUCE_GAP

	   //DBG_CALLER("onoff:%d g_hw_mute_staus:%d\n",onoff,g_hw_mute_status);

	   if(g_hw_mute_status ==-1 || g_hw_mute_status!= onoff) {  

	   } else {  // 중복 호출이면 무시함.
	   
	  		//DBG_ERR("AK380","HP MUTE :%d ignored\n",onoff);
			return;
	   }

       #endif

#ifdef PRINT_ALSA_MSG
		DBG_ERR("AK380","HP MUTE :%d\n",onoff);
#endif
		gpio_request(GPIO_AUDIO_SW_MUTE, "GPIO_AUDIO_SW_MUTE");
		tcc_gpio_config(GPIO_AUDIO_SW_MUTE,GPIO_FN(0));
		
		gpio_request(GPIO_HP_BAL_MUTE, "GPIO_HP_BAL_MUTE");
		tcc_gpio_config(GPIO_HP_BAL_MUTE,GPIO_FN(0));

		gpio_request(GPIO_HP_MUTE, "GPIO_HP_MUTE");
		tcc_gpio_config(GPIO_HP_MUTE,GPIO_FN(0) | GPIO_CD(0));		
		

		if(onoff)  {
#if BALANCED_OUT_FUNC
			if(is_selected_balanced())  {   // balanced

				#if (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII)		////dean 20170719 	
					gpio_direction_output2(GPIO_AUDIO_SW_MUTE,1); 

					
					#ifdef SUPPORT_AK_DOCK
					amp_bal_mute(1); //ysyim
					#endif
					msleep_interruptible(150);
					gpio_direction_output(GPIO_HP_BAL_MUTE,1);	//always low	        	

				#else
					gpio_direction_output(GPIO_HP_BAL_MUTE,1);
					
					#ifdef SUPPORT_AK_DOCK
					amp_bal_mute(1); //ysyim
					#endif
					msleep_interruptible(150);
			        	gpio_direction_output2(GPIO_AUDIO_SW_MUTE,1); 

				#endif
			}
			else
#endif
			{

				gpio_direction_output(GPIO_HP_MUTE,1);

				msleep_interruptible(150);

				gpio_direction_output(GPIO_AUDIO_SW_MUTE,1);
	
				
				#ifdef SUPPORT_EXT_AMP
				amp_unbal_mute(1); //ysyim				
				#endif
			}

			#ifdef SUPPORT_AK_DOCK
			cradle_mute(1); //ysyim
			#endif


			#ifdef SUPPORT_AK_DOCK
			if(ak_dock_check_device()==DEVICE_AMP)
				msleep_interruptible(50);
			else
			#endif	

			#ifdef SUPPORT_AK_DOCK
			dock_mute(1); //ysyim: GPIO_AMP_SW_MUTE_COM high
			#endif

			request_unmute();

		} else {			
			#ifdef SUPPORT_AK_DOCK
			// 2015.05.12 tonny
			dock_mute(0);
			#endif

#if BALANCED_OUT_FUNC
			if(is_selected_balanced())  {   // balanced

				#if (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII)		////dean 20170719 	
					gpio_direction_output2(GPIO_AUDIO_SW_MUTE,0);
				    	msleep_interruptible(100);

					//gpio_direction_output2(GPIO_AUDIO_SW_MUTE,0);
					msleep_interruptible(50);
					
					gpio_direction_output(GPIO_HP_BAL_MUTE,0);	

					
					#ifdef SUPPORT_AK_DOCK
					amp_bal_mute(0); //ysyim
					#endif

				#else
					gpio_direction_output2(GPIO_AUDIO_SW_MUTE,0);
				    	msleep_interruptible(100);

			
					gpio_direction_output(GPIO_HP_BAL_MUTE,0);	
					msleep_interruptible(50);
					
					#ifdef SUPPORT_AK_DOCK
					amp_bal_mute(0); //ysyim
					#endif
				#endif
			}
			else
#endif   
			{

			
				gpio_direction_output(GPIO_HP_MUTE,0);
				
				msleep_interruptible(120);

				gpio_direction_output(GPIO_AUDIO_SW_MUTE,0);
				
				#ifdef SUPPORT_EXT_AMP
				amp_unbal_mute(0); //ysyim
				#endif
			}

			#ifdef SUPPORT_AK_DOCK
			cradle_mute(0); //ysyim
			#endif

			msleep_interruptible(5);
			
			reset_unmute();
		}
                #ifdef REDUCE_GAP
		g_hw_mute_status = onoff;
                #endif
		break;

	default:
		break;

		
	}
}

void ak_set_hw_mute_for_usbdac(AUDIO_HW_MUTE_TYPE e_type,int onoff)
{
	switch(e_type)  {
	case AMT_HP_MUTE:
	   //DBG_CALLER("\n");
		printk("%s -- onoff: %d\n", __func__, onoff);
		
#ifdef PRINT_ALSA_MSG
		DBG_ERR("AK380","HP MUTE :%d\n",onoff);
#endif
		gpio_request(GPIO_AUDIO_SW_MUTE, "GPIO_AUDIO_SW_MUTE");
		tcc_gpio_config(GPIO_AUDIO_SW_MUTE,GPIO_FN(0));
		
		gpio_request(GPIO_HP_BAL_MUTE, "GPIO_HP_BAL_MUTE");
		tcc_gpio_config(GPIO_HP_BAL_MUTE,GPIO_FN(0));

		gpio_request(GPIO_HP_MUTE, "GPIO_HP_MUTE");
		tcc_gpio_config(GPIO_HP_MUTE,GPIO_FN(0) | GPIO_CD(0));		
		

		if(onoff)  {
			gpio_direction_output(GPIO_HP_MUTE,1);
			mdelay(10);
			gpio_direction_output(GPIO_AUDIO_SW_MUTE,1);

			request_unmute();			
		}
		else {			
			gpio_direction_output(GPIO_HP_MUTE,0);
			mdelay(10);
			gpio_direction_output(GPIO_AUDIO_SW_MUTE,0);

			reset_unmute();
		}

		break;

	default:
		break;
	}
}


EXPORT_SYMBOL(ak_set_hw_mute_for_usbdac);

#else


void ak_set_hw_mute(AUDIO_HW_MUTE_TYPE e_type,int onoff)
{
	switch(e_type)  {
	case AMT_HP_MUTE:
//	   DBG_CALLER("\n");

#ifdef PRINT_ALSA_MSG
		DBG_ERR("AK380","HP MUTE :%d\n",onoff);
#endif
		gpio_request(GPIO_AUDIO_SW_MUTE, "GPIO_AUDIO_SW_MUTE");
		tcc_gpio_config(GPIO_AUDIO_SW_MUTE,GPIO_FN(0));
		
		gpio_request(GPIO_HP_BAL_MUTE, "GPIO_HP_BAL_MUTE");
		tcc_gpio_config(GPIO_HP_BAL_MUTE,GPIO_FN(0));

		gpio_request(GPIO_HP_MUTE, "GPIO_HP_MUTE");
		tcc_gpio_config(GPIO_HP_MUTE,GPIO_FN(0) | GPIO_CD(0));		
		

		if(onoff)  {

#if BALANCED_OUT_FUNC
			if(is_selected_balanced())  {   // balanced
			

				gpio_direction_output(GPIO_HP_BAL_MUTE,1);
				
			}
			else
#endif
			{

				gpio_direction_output(GPIO_HP_MUTE,0);			

				
				//msleep_interruptible_precise(50);

				//ak_route_hp_balanced_out2(AOT_BALANCED);
				
				//msleep_interruptible_precise(50);

				gpio_direction_output(GPIO_AUDIO_SW_MUTE,1);

				msleep_interruptible_precise(150);

				//ak_route_hp_balanced_out2(AOT_HP);
				
				
			}




		} else {			

#if BALANCED_OUT_FUNC
			if(is_selected_balanced())  {   // balanced
		
				gpio_direction_output(GPIO_HP_BAL_MUTE,0);	
				
			}
			else
#endif
			{
//				ak_route_hp_balanced_out2(AOT_BALANCED);
						
				gpio_direction_output(GPIO_HP_MUTE,0);
				
				//msleep_interruptible_precise(150);
				gpio_direction_output(GPIO_AUDIO_SW_MUTE,0);
//				ak_route_hp_balanced_out2(AOT_HP);
				//cs4398_mute(0);

				
			}
			
		}

		break;

	default:
		break;
	}
}
#endif

EXPORT_SYMBOL(ak_set_hw_mute);
#endif


void ak_set_hp_mute(int onoff)
{
#ifdef PRINT_ALSA_MSG
	DBG_PRINT("GPIO","HP MUTE :%d\n",onoff);
#endif

#if (CUR_AK != MODEL_AK500N)
	gpio_request(GPIO_HP_MUTE, "GPIO_HP_MUTE");
	tcc_gpio_config(GPIO_HP_MUTE,GPIO_FN(0));
	gpio_direction_output(GPIO_HP_MUTE,onoff);
	
	#ifdef SUPPORT_EXT_AMP
	amp_unbal_mute(onoff); //ysyim
	#endif

#endif
}


void ak_set_audio_sw_mute(int onoff)
{

#ifdef PRINT_ALSA_MSG
	DBG_PRINT("GPIO","Audio MUTE :%d\n",onoff);
#endif

	gpio_request(GPIO_AUDIO_SW_MUTE, "GPIO_AUDIO_SW_MUTE");

#ifndef SLEEP_CURRENT_CHECK
	tcc_gpio_config(GPIO_AUDIO_SW_MUTE,GPIO_FN(0));
#else
	tcc_gpio_config(GPIO_AUDIO_SW_MUTE,GPIO_FN(0) | GPIO_INPUT);
#endif

#if (CUR_AK == MODEL_AK100_II || CUR_AK==MODEL_AK120_II)
#ifndef SLEEP_CURRENT_CHECK
	gpio_direction_output(GPIO_AUDIO_SW_MUTE,onoff==1 ? 1 : 0);
#endif
#elif (CUR_AK == MODEL_AK240||  CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300 || CUR_AK==MODEL_AK500N || CUR_AK==MODEL_AK70)
	gpio_direction_output2(GPIO_AUDIO_SW_MUTE,onoff==1 ? 1 : 0);
#endif

}


void  reset_wm8804(void)
{
	gpio_request(GPIO_OPTIC_RST, "GPIO_OPTIC_RST");
	tcc_gpio_config(GPIO_OPTIC_RST,GPIO_FN(0));
	gpio_direction_output(GPIO_OPTIC_RST,1);
	msleep(1);
	gpio_free(GPIO_OPTIC_RST);

	wm8804_reinit();
}

#ifdef _MTP_SOUND_BREAK_FIX
static DEFINE_MUTEX(mtp_transfer_delay_mutex);
static int g_play_status = MPS_STOP;

void mtp_set_play_status(int status)
{
	mutex_lock(&mtp_transfer_delay_mutex);
	g_play_status = status;
	CPRINT("mtp:%d\n",status);
	mutex_unlock(&mtp_transfer_delay_mutex);
}

int mtp_get_play_status(void)
{
	int status;
	mutex_lock(&mtp_transfer_delay_mutex);
	status = g_play_status;
	mutex_unlock(&mtp_transfer_delay_mutex);

	return status;
}
EXPORT_SYMBOL(mtp_get_play_status);
#endif

/* [downer] A140806 for Battery playing */
#if (CUR_AK == MODEL_AK500N)
#if (CUR_AK_REV == AK500N_HW_ES1) || (CUR_AK_REV == AK500N_HW_TP)
static int g_adapter_playing = 0;
static int g_current_playing = 0;
static int g_mtp_mode = 0;

void set_mtp_mode(int mode)
{
    g_mtp_mode = mode;
}
EXPORT_SYMBOL(set_mtp_mode);

void disconnect_AC(void)
{
	if (get_boot_complete() && (get_battery_level() != 0)) {
        if (!g_adapter_playing && g_current_playing && !g_mtp_mode) {
            gpio_request(GPIO_ADAPTER_EN, "GPIO_ADAPTER_EN");
            tcc_gpio_config(GPIO_ADAPTER_EN, GPIO_FN(0));
            gpio_direction_output(GPIO_ADAPTER_EN, 1);
        }
	}
}
EXPORT_SYMBOL(disconnect_AC);

void connect_AC(void)
{
	if (get_boot_complete()) {
		gpio_request(GPIO_ADAPTER_EN, "GPIO_ADAPTER_EN");
		tcc_gpio_config(GPIO_ADAPTER_EN, GPIO_FN(0));
		gpio_direction_output(GPIO_ADAPTER_EN, 0);
	}
}
EXPORT_SYMBOL(connect_AC);

static void ac_connect_work(struct work_struct *work)
{
	connect_AC();
}

void connect_adapter(void)
{
	schedule_delayed_work(&ac_work, AC_CONNECT_TIMER);
}
EXPORT_SYMBOL(connect_adapter);

void disconnect_adapter(void)
{
	cancel_delayed_work(&ac_work);
    disconnect_AC();
}
EXPORT_SYMBOL(disconnect_adapter);

void init_bar_led_init(void)
{
	printk("%s\n",__FUNCTION__);
	
	gpio_request(GPIO_WHITE_LED_EN, "GPIO_WHITE_LED_EN");
	tcc_gpio_config(GPIO_WHITE_LED_EN,GPIO_FN(0));
	gpio_direction_output(GPIO_WHITE_LED_EN, 1); // output
	gpio_set_value(GPIO_WHITE_LED_EN, 1);	
	
	gpio_request(GPIO_YELLOW_LED_EN, "GPIO_YELLOW_LED_EN");
	tcc_gpio_config(GPIO_YELLOW_LED_EN,GPIO_FN(0));
	gpio_direction_output(GPIO_YELLOW_LED_EN, 1); // output
	gpio_set_value(GPIO_YELLOW_LED_EN, 1);	

	//gpio_request(GPIO_YELLOW_LED_PWM, "GPIO_YELLOW_LED_PWM");
	//tcc_gpio_config(GPIO_YELLOW_LED_PWM,GPIO_FN(0));
	//gpio_direction_output(GPIO_YELLOW_LED_PWM, 1); // output
	//gpio_set_value(GPIO_YELLOW_LED_PWM, 1);

	gpio_request(GPIO_WHITE_LED_PWM, "GPIO_WHITE_LED_PWM");
	tcc_gpio_config(GPIO_WHITE_LED_PWM,GPIO_FN(0));
	gpio_direction_output(GPIO_WHITE_LED_PWM, 1); // output
	//gpio_set_value(GPIO_WHITE_LED_PWM, 0);	

}

static bar_led_fade_in(struct work_struct *ignored)	
{
 	#define MAX_BACKLIGTH 1000
	unsigned int count, level;
	
	printk("+ %s\n ",__FUNCTION__);

	for(count = 60; count <=MAX_BACKLIGTH; count++){
		level = count;	
 		tca_tco_pwm_ctrl(2,GPIO_WHITE_LED_PWM, MAX_BACKLIGTH, level);

		if(count <100)
			msleep(70);
		else if(count < 200)
			msleep(50);
		else
			msleep(5);
		
		//if(count == 400)
		//	printk("%s level:%d\n",__FUNCTION__,level);
	}

	printk("- %s\n ",__FUNCTION__);
}
static DECLARE_WORK(bar_led_fade_in_type_work, bar_led_fade_in);

static bar_led_fade_out(struct work_struct *ignored)	
{
 	#define MAX_BACKLIGTH 300
	unsigned int count, level;
	
	printk("+ %s\n ",__FUNCTION__);
	
	for(count = MAX_BACKLIGTH; count >=40; count--){
		level = count;	
 		tca_tco_pwm_ctrl(2,GPIO_WHITE_LED_PWM, MAX_BACKLIGTH, level);

		msleep(5);
		
		//if(count == 400)
		//	printk("%s level:%d\n",__FUNCTION__,level);
	}

	printk("- %s\n ",__FUNCTION__);
}
static DECLARE_WORK(bar_led_fade_out_type_work, bar_led_fade_out);

void set_bar_led_brightness(int level)
{
	 #define MAX_BACKLIGTH 255
	g_barled_brightness = level;

	if(level >= 11)
		level = MAX_BACKLIGTH;		
	else if(level <= 0)
		level = 0;			
	else{
		level = level*20+30;  // (255-80)/11 = 15
	}

	printk("set_bar_led_brightness:%d(%d)\n",level,g_barled_brightness);	
 	tca_tco_pwm_ctrl(2,GPIO_WHITE_LED_PWM, MAX_BACKLIGTH, level);
}
EXPORT_SYMBOL(set_bar_led_brightness);

void bar_led_off(void)
{
	tcc_gpio_config(GPIO_WHITE_LED_PWM,GPIO_FN(0));
	gpio_direction_output(GPIO_WHITE_LED_PWM, 1); // output
	gpio_set_value(GPIO_WHITE_LED_PWM, 0);	
	
	gpio_set_value(GPIO_WHITE_LED_EN, 0);	
	gpio_set_value(GPIO_YELLOW_LED_EN, 0);	
}

void bar_white_led_on(void)
{
	gpio_set_value(GPIO_WHITE_LED_EN, 1);	
	gpio_set_value(GPIO_YELLOW_LED_EN, 0);	
}
void bar_yellow_led_on(void)
{
	gpio_set_value(GPIO_WHITE_LED_EN, 0);	
	gpio_set_value(GPIO_YELLOW_LED_EN, 1);	
}

void bar_white_yellow_led_on(void)
{
	gpio_set_value(GPIO_WHITE_LED_EN, 1);	
	gpio_set_value(GPIO_YELLOW_LED_EN, 1);	
}

void bar_led_control(int state)
{
	printk("bar_led_control:%d\n",state);
	
	if(state == 0)
		bar_led_off();
	else if(state ==1)
		bar_white_led_on();
	else if(state == 2)
		bar_yellow_led_on();
	else if(state == 3)
		bar_white_yellow_led_on();
	
	if(state != 0)
		set_bar_led_brightness(g_barled_brightness);
		
}
EXPORT_SYMBOL(bar_led_control);

#endif
#endif

/*
  for ak220 reducing pop noise.comtp_set_play_statusnsole_set_on_cmdline
*/

//#define DEBUG_PCM_MSG

int g_pcm_stoped = 1;

static int g_pcm_opened = 0;

void ak_pcm_open_pre(void)
{
#ifdef DEBUG_PCM_MSG
	DBG_ERR("d","PRE pcm open\n");
#endif

	set_pcm_is_playing(1); //iriver-ysyim		

	//if(ak_dock_check_device() == DEVICE_CRADLE) //iriver-ysyim
	//	start_cradle();
		
#if(CUR_AK == MODEL_AK500N) //jimi.1030.change audio path to avoid crack noise
	if(g_curr_audio_in_type != AUDIO_IN_TYPE_LOCAL &&
			g_curr_audio_in_type != AUDIO_IN_TYPE_NONE ) {
		switch(g_curr_audio_in_type) {
			case AUDIO_IN_TYPE_OPTICAL:
				Optical_In(0);
				break;
			case AUDIO_IN_TYPE_COXIAL:
				Coxial_In(0);
				break;
			case AUDIO_IN_TYPE_BNC:
				Bnc_In(0);
				break;
			case AUDIO_IN_TYPE_SPDIF:
				Spdif_In(0);
				break;
			default:
				break;
		}
		Local_in(1);
	} 
#else

#ifdef FIX_MUTE_NOISE

#else
#if (CUR_AK != MODEL_AK380 && CUR_AK != MODEL_AK320 && CUR_AK != MODEL_AK300)
	ak_set_hw_mute(AMT_HP_MUTE,1); //jimi.1103
#endif
#endif

#endif

	/* [downer] A140806 */
#if (CUR_AK == MODEL_AK500N)
#if (CUR_AK_REV == AK500N_HW_ES1) || (CUR_AK_REV == AK500N_HW_TP)
	set_playing_mode(1);
	disconnect_adapter();
#endif
#endif


#ifdef AUDIO_POWER_CTL
	if(g_pcm_opened) {
		set_audio_power_ctl_status(FALSE);
		
		#ifdef SUPPORT_AK_DOCK
		if(ak_is_connected_optical_tx() && (!is_amp_on() && !is_cradle_on())) { /* [downer] M150714 */
		#else
		if(ak_is_connected_optical_tx()) { /* [downer] M150714 */
		#endif
            printk("%s -- ADP_OPTICAL_OUT SET!!\n", __func__);
			ak_set_audio_path(ADP_OPTICAL_OUT);
			msleep_interruptible(10);
			ak_set_snd_pwr_ctl(AK_OPTICAL_TX_POWER,AK_PWR_ON);
		} else {
            //printk("%s -- ADP_PLAYBACK SET!!\n", __func__);
			ak_set_audio_path(ADP_PLAYBACK);
		}

		if(ak_get_snd_pwr_ctl(AK_AUDIO_POWER) == 0) {
			ak_set_snd_pwr_ctl(AK_AUDIO_POWER,AK_PWR_ON);
			msleep_interruptible(50);
#if (CUR_DAC==CODEC_CS4398)
			//cs4398_reg_init();
#elif(CUR_DAC == CODEC_AK4490)
			//ak4490_reg_init();
			//msleep_interruptible(500);
#endif
		}

	}
	else {
		ak_set_snd_pwr_ctl(AK_AUDIO_POWER,AK_PWR_ON);  // avoid 1th pop noise.
	}

	g_pcm_opened = 1;
	g_pcm_stoped = 0;

#endif
}


void ak_pcm_open_post(void)
{
#ifdef DEBUG_PCM_MSG
	DBG_ERR("d","pcm_open post\n");
#endif

}
extern int tcc_pcm_davc(int volume);

int g_audio_delay1=300,g_audio_delay2=100;


void ak_pcm_stop_pre(void)
{	
	int usb_mode = get_current_usb_device();
	//	int usb_mode = ak_get_current_usb_mode();
	int play_mode = ak_get_internal_play_mode();

	#ifdef USE_VWR_TYPE2
	#if (CUR_DAC==CODEC_CS4398)
	cs4398_ramp_mode(CS4398_RAMP_SR);
	#endif
	#endif


#ifdef DEBUG_PCM_MSG
	DBG_ERR("d","ak_pcm_stop_pre %d\n",usb_mode);
#endif

#if (CUR_AK == MODEL_AK500N)
	if (g_optical_rx_status == 1) {
	} else
#endif
		if(play_mode==INTERNAL_DSD_PLAY || usb_mode ==UCM_PC_FI)  {

		} else {
			int i,j,ramp_down_delay=10;
			int lvol,rvol,vol ;
			extern void tcc_pcm_mute(int onoff);

			#if (CUR_DAC==CODEC_CS4398)
			cs4398_mute(1);
			#endif
		}

	g_pcm_stoped = 1;
}

void ak_pcm_stop_post(void)
{
	//ak_set_internal_play_mode(INTERNAL_NONE_PLAY);
	//DBG_ERR("d","ak_pcm_stop_post \n");
#ifdef DEBUG_PCM_MSG
	DBG_ERR("d","ak_pcm_stop_post \n");
#endif
}


void ak_pcm_close_pre(void)
{
#ifdef DEBUG_PCM_MSG
	DBG_ERR("d","pcm_close prev\n");
#endif

	set_pcm_is_playing(0); //iriver-ysyim		

#if (CUR_AK == MODEL_AK500N)
	if (g_optical_rx_status == 1)
		return;
#endif
	if(get_current_usb_device() == UCM_PC_FI) {
		//DBG_ERR("d","PC_FI");
		return;
	}

	if(ak_get_internal_play_mode()==INTERNAL_DSD_PLAY)	{
		//DBG_ERR("d","DSD");
		return;
	}
#if (CUR_AK != MODEL_AK380 && CUR_AK != MODEL_AK320 && CUR_AK != MODEL_AK300)
#ifdef FIX_MUTE_NOISE

#else
	ak_set_hw_mute(AMT_HP_MUTE,1);
#endif
#endif
}


void ak_pcm_close_post(void)
{
#ifdef DEBUG_PCM_MSG
	DBG_ERR("d","pcm_close post\n");
#endif
	//  ak_set_snd_pwr_ctl(AK_AUDIO_POWER, AK_PWR_OFF);

#if (CUR_AK == MODEL_AK500N)
	if (g_optical_rx_status == 0)
#endif
#ifdef AUDIO_POWER_CTL
		if(ak_get_internal_play_mode() != INTERNAL_DSD_PLAY)
			set_audio_power_ctl_status(TRUE);
#endif

#ifdef _MTP_SOUND_BREAK_FIX
	mtp_set_play_status(MPS_STOP);
#endif

	if(ak_is_connected_optical_tx()) { /* 2014-3-17 */
        printk("%s -- ADP_NONE\n", __func__);
		ak_set_audio_path(ADP_NONE);
	}

	/* [downer] M140919 AC connect */
#if (CUR_AK == MODEL_AK500N)
#if (CUR_AK_REV == AK500N_HW_ES1) || (CUR_AK_REV == AK500N_HW_TP)
	set_playing_mode(0);
	connect_adapter();
#endif
#endif

#ifdef FIX_MUTE_NOISE
#else
ak_set_hw_mute(AMT_HP_MUTE,1);
#endif

}

void ak_pcm_start_pre(void)
{
#ifdef DEBUG_PCM_MSG
	DBG_ERR("d","ak_pcm_start_pre \n");
#endif
}

void ak_pcm_start_post(void)
{
#ifdef DEBUG_PCM_MSG
	DBG_ERR("d","ak_pcm_start_post \n");
#endif
#if(CUR_DAC == CODEC_ES9018)
	ES9018_schedule_power_ctl(1);
#endif

	//#if 0//ndef CONFIG_INTERNAL_DSD_DEMO
	//-jimi.test.demo.1213
#if (CUR_AK == MODEL_AK100_II || CUR_AK == MODEL_AK120_II)
	//	if(get_current_usb_device() != UCM_PC_FI) //0515
#endif
	ak_pcm_schedule_ctl(1);
	//#endif
}

#ifdef PCM_DSD_CALLBACK
#define PRINT_ALSA_DSD_MSG

void snd_usb_pcm_open_pre()
{
#ifdef	PRINT_ALSA_DSD_MSG
	CPRINT("%s",__FUNCTION__);
#endif
}

void snd_usb_pcm_open_post()
{
#ifdef	PRINT_ALSA_DSD_MSG
	CPRINT("%s",__FUNCTION__);
#endif
}

void snd_usb_pcm_close_pre()
{
#ifdef	PRINT_ALSA_DSD_MSG
	CPRINT("%s",__FUNCTION__);
#endif
}

void snd_usb_pcm_close_post()
{
#ifdef	PRINT_ALSA_DSD_MSG
	CPRINT("%s",__FUNCTION__);
#endif
}

void snd_usb_playback_start_pre()
{
#ifdef	PRINT_ALSA_DSD_MSG
	CPRINT("%s",__FUNCTION__);
#endif
}

void snd_usb_playback_start_post()
{
#ifdef	PRINT_ALSA_DSD_MSG
	CPRINT("%s",__FUNCTION__);
#endif
}

void snd_usb_playback_stop_pre()
{
#ifdef	PRINT_ALSA_DSD_MSG
	CPRINT("%s",__FUNCTION__);
#endif
}

void snd_usb_playback_stop_post()
{
#ifdef	PRINT_ALSA_DSD_MSG
	CPRINT("%s",__FUNCTION__);
#endif
}
#endif

static struct delayed_work g_pcm_open_work;
static struct delayed_work g_pcm_off_work;

extern void tcc_pcm_mute(int onoff);

#if (CUR_AK == MODEL_AK240 ||  CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)
int g_audio_start_delay = 50;
#elif (CUR_AK == MODEL_AK320)
int g_audio_start_delay = 50;
#elif (CUR_AK == MODEL_AK100_II)
int g_audio_start_delay = 130;
#elif (CUR_AK == MODEL_AK70)

#ifdef FIX_MUTE_NOISE
int g_audio_start_delay = 10;
#else
int g_audio_start_delay = 80;
#endif

#else
int g_audio_start_delay = 80;
#endif

void set_audio_start_delay(int delay)
{
	g_audio_start_delay = delay;
}

static struct delayed_work g_power_on_work;
static struct delayed_work g_power_off_work;

extern void tcc_pcm_mute(int onoff);

static ktime_t g_pcm_open_post_start, g_pcm_open_post_end;

//jimi.0508
void mute_for_usbdac(int on)
{
	ak_set_hw_mute(AMT_HP_MUTE,on);
}
EXPORT_SYMBOL(mute_for_usbdac);

void pcm_open_elapsed_start(void)
{
	g_pcm_open_post_start = ktime_get();
}

void pcm_open_elapsed_end(void)
{
	g_pcm_open_post_end = ktime_get();
}

void msleep_interruptible_precise(int msec)
{
	ktime_t _start_time;

	s64 _elapsed_time,_rest_time;

	if(msec <= 10) {
		msleep_interruptible(msec);
	} else {
		_start_time= ktime_get();

		while(1)  {
			msleep_interruptible(10);
			_elapsed_time = ktime_to_ms(ktime_sub(ktime_get(), _start_time));

			if( _elapsed_time >= msec - 5) {
				break;
			}
		}
		_rest_time = msec - _elapsed_time;

		if(_rest_time > 0)  {
			msleep_interruptible(_rest_time);
			//CPRINT("%d\n",(int)_rest_time);

		}
		if(_elapsed_time>50) {
			//CPRINT("%d\n",(int)_elapsed_time);
		}
	}
}

static void pcm_open_work(struct work_struct *work)
{
	ktime_t g_pcm_open_post_period;
	struct timespec period;
	unsigned long elapsed_ms,elapsed2_ms,delay_adjust_ms;
	ktime_t time;
	#if 0
	pcm_open_elapsed_end();
	g_pcm_open_post_period = ktime_sub(g_pcm_open_post_end, g_pcm_open_post_start);

	period = ktime_to_timespec(g_pcm_open_post_period);
	elapsed_ms = period.tv_nsec / 1000000;

	delay_adjust_ms = g_audio_start_delay - elapsed_ms;

	if(delay_adjust_ms > 0 && delay_adjust_ms <50)  {
		msleep_interruptible(delay_adjust_ms);
		//mdelay(delay_adjust_ms);
	}
	#endif

	//pcm_open_elapsed_end();
	//g_pcm_open_post_period = ktime_sub(g_pcm_open_post_end, g_pcm_open_post_start);
	//period = ktime_to_timespec(g_pcm_open_post_period);
	//elapsed2_ms = period.tv_nsec / 1000000;

#ifdef FIX_MUTE_NOISE
    #ifdef REDUCE_GAP
		ak_set_hw_mute(AMT_HP_MUTE, 0);

		#if(CUR_DAC==CODEC_CS4398)
		cs4398_mute(0);
		cs4398_resume_volume();		
		#endif

    #else
		if(is_request_unmute()) {
			reset_unmute();
			if(is_selected_balanced())	{
			} else {
				msleep_interruptible(50);
			}
			ak_set_hw_mute(AMT_HP_MUTE,0);
		}

		#if(CUR_DAC==CODEC_CS4398)
		cs4398_mute(0);
		cs4398_resume_volume();		
		#endif

	#endif

	#ifdef USE_VWR_TYPE2
	msleep_interruptible(200);
	#if (CUR_DAC==CODEC_CS4398)
	cs4398_ramp_mode(CS4398_RAMP_SRZC);
	#endif
	#endif


#else
	ak_set_hw_mute(AMT_HP_MUTE,0);

	msleep_interruptible(30);
#endif


	//CPRINT("%d %d",elapsed_ms,delay_adjust_ms);

	//cs4398_mute(0);
}

static void pcm_off_work(struct work_struct *work)
{
}

void ak_pcm_schedule_ctl(int onoff)
{
	if(onoff==1) {
		pcm_open_elapsed_start();
		schedule_delayed_work(&g_pcm_open_work, msecs_to_jiffies(g_audio_start_delay));
	} else {
		schedule_delayed_work(&g_pcm_off_work, 0);
	}
}

static int g_ak_audio_power_status = AK_AUDIO_POWER_ALL_OFF;

int ak_get_snd_pwr_ctl(int power_ctl_status)
{
	return g_ak_audio_power_status & power_ctl_status;
}

void ak_set_snd_pwr_ctl_request(snd_pwr_ctrl_t snd_ctl,int pwr_ctl)
{
	if(pwr_ctl == AK_PWR_ON) {
		g_ak_audio_power_status &= ~snd_ctl;
		//CPRINT("ak_set_snd_pwr_ctl_request :%d ",snd_ctl);

	} else {
		g_ak_audio_power_status |= snd_ctl;
	}
}

void ak_set_snd_pwr_ctl(snd_pwr_ctrl_t snd_ctl,int pwr_ctl)
{
	gpio_request(GPIO_5V5_EN, "GPIO_5V5_EN");

	#ifdef _AK_REMOVE_
	gpio_request(GPIO_N2V8_EN, "GPIO_N2V8_EN");
	#endif
	
	gpio_request(GPIO_DAC_EN, "GPIO_DAC_EN");
	gpio_request(GPIO_DAC_RST, "GPIO_DAC_RST");
	gpio_request(GPIO_OPAMP_PWR_EN, "GPIO_OPAMP_PWR_EN");
        #if (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII)  ////dean 20170804   	
	gpio_request(GPIO_OPAMP_UNBAL_PWR_EN, "GPIO_OPAMP_UNBAL_PWR_EN");
	#endif
	

#if OPTICAL_RX_FUNC
	gpio_request(GPIO_OP_RX_EN, "GPIO_OP_RX_EN");
#endif

#if 0
	gpio_request(GPIO_AUDIO_SW_ON, "GPIO_AUDIO_SW_ON");
#endif

	#ifdef ENABLE_OPTICAL_TX
	gpio_request(GPIO_OP_TX_PWEN, "GPIO_OP_TX_PWEN");
	#endif

#if (CUR_AK >= MODEL_AK240  || CUR_AK==MODEL_AK500N)
	#if (CUR_AK_REV != AK380_HW_EVM)
	
	#ifdef _AK_REMOVE_
	gpio_request(GPIO_AUDIO_N5V_EN, "GPIO_AUDIO_N5V_EN");
	#endif
	
	#endif

	
	#ifdef _AK_REMOVE_
	gpio_request(GPIO_OPAMP_4V_EN, "GPIO_OPAMP_4V_EN");
	#endif

#endif

	// 2015.06.29 tonny - obtain semaphore
	down(&pwr_ctl_sem);

#ifdef AK_AUDIO_MODULE_USED
	pwr_ctl = AK_PWR_ON; // for moduel demo
#endif

	switch(snd_ctl) {
	case AK_AUDIO_POWER:
		if(pwr_ctl == AK_PWR_ON) {
			if(g_ak_audio_power_status & snd_ctl) {

#ifdef PRINT_ALSA_MSG
				DBG_ERR("d","Already AK_AUDIO_POWER ON\n");
#endif

				//return;
			}
#if 0
			tcc_gpio_config(GPIO_AUDIO_SW_ON, GPIO_FN(0));
			gpio_direction_output(GPIO_AUDIO_SW_ON, 1);
#endif


			ak_set_hw_mute(AMT_HP_MUTE,1);
			msleep_interruptible(100);
			
			CPRINT("AK_AUDIO_POWER => AK_PWR_ON\n");
			/* [downer] M130827 change audio PWR on sequence */

			#ifdef _AK_REMOVE_
			tcc_gpio_config(GPIO_OPAMP_4V_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_OPAMP_4V_EN, 1);
			#endif

			#if (CUR_AK == MODEL_AK70) ////dean 20170719  && (CUR_AK_SUB != MKII) 	
				 #if (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII)  //dean 20170830 unbal noise fixed
				if(is_selected_balanced())
				{
					
					tcc_gpio_config(GPIO_OPAMP_PWR_EN, GPIO_FN(0));
					gpio_direction_output(GPIO_OPAMP_PWR_EN, 1);		

				}
				else
				{
						tcc_gpio_config(GPIO_OPAMP_UNBAL_PWR_EN, GPIO_FN(0));
						gpio_direction_output(GPIO_OPAMP_UNBAL_PWR_EN, 1);	
				}
					msleep_interruptible(100);
				#endif	
			
			//** GPIO_5V5_EN--> High
			tcc_gpio_config(GPIO_5V5_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_5V5_EN, 1);
			#endif	
			
			//** GPIOD[13](DAC_EN) --> High
			tcc_gpio_config(GPIO_DAC_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_DAC_EN, 1);

			mdelay(10);

			//WM8740 Reset(High Active)
			tcc_gpio_config(GPIO_DAC_RST, GPIO_FN(0));
			gpio_direction_output(GPIO_DAC_RST, 1);

			mdelay(10);

#if (CUR_DAC==CODEC_AK4490)
			mdelay(10);
			ak4490_reg_init();
			msleep_interruptible(500);
#endif


#if (CUR_DAC==CODEC_CS4398)
			cs4398_reg_init();
			msleep_interruptible(200);

#endif

		      #if (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII)  ////dean 20170804   	
			#else
			//** GPIOF[0](OPAMP_EN) --> High
			tcc_gpio_config(GPIO_OPAMP_PWR_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_OPAMP_PWR_EN, 1);
                        #endif

			#if 0	//dean 20170719
			#if (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII) 		
			//170424-yys: to remove pop-noise, enable GPIO_5V5_EN after GPIO_OPAMP_PWR_EN			
			mdelay(50);
			tcc_gpio_config(GPIO_5V5_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_5V5_EN, 1);			
			#endif
			#endif
			
			#ifdef _AK_REMOVE_
			//** (N2V8_EN) --> High
			tcc_gpio_config(GPIO_N2V8_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_N2V8_EN, 1);
			#endif

#if 0//BALANCED_OUT_FUNC
			tcc_gpio_config(GPIO_AUDIO_SW_ON, GPIO_FN(0));
			gpio_direction_output(GPIO_AUDIO_SW_ON, 1);
#endif

#if (CUR_AK >= MODEL_AK240)
#ifndef SLEEP_CURRENT_CHECK
			#if (CUR_AK_REV != AK380_HW_EVM)

			
			#ifdef _AK_REMOVE_
			tcc_gpio_config(GPIO_AUDIO_N5V_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_AUDIO_N5V_EN, 1);
			#endif
			
			#endif
#else
			#if (CUR_AK_REV != AK380_HW_EVM)
			tcc_gpio_config(GPIO_AUDIO_N5V_EN, GPIO_FN(0) | GPIO_INPUT);
			#endif
#endif
#endif

#ifdef FIX_MUTE_NOISE
			//ak_set_hw_mute(AMT_HP_MUTE,0);
#endif

		} else {
			CPRINT("AK_AUDIO_POWER => AK_PWR_OFF\n");

			ak_set_hw_mute(AMT_HP_MUTE,1);

#if (CUR_AK >= MODEL_AK240)
#ifndef SLEEP_CURRENT_CHECK
			#if (CUR_AK_REV != AK380_HW_EVM)
			
			#ifdef _AK_REMOVE_
			tcc_gpio_config(GPIO_AUDIO_N5V_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_AUDIO_N5V_EN, 0);
			#endif
			
			#endif
#else
			#if (CUR_AK_REV != AK380_HW_EVM)
			tcc_gpio_config(GPIO_AUDIO_N5V_EN, GPIO_FN(0) | GPIO_INPUT);
			#endif
#endif
#endif

#if BALANCED_OUT_FUNC
//			tcc_gpio_config(GPIO_AUDIO_SW_ON, GPIO_FN(0));
//			gpio_direction_output(GPIO_AUDIO_SW_ON, 0);
#endif

			mdelay(30);

			//WM8740 Reset(High Active)
#if 1
			tcc_gpio_config(GPIO_DAC_RST, GPIO_FN(0));
			gpio_direction_output(GPIO_DAC_RST, 0);
#endif

			mdelay(10);

			//** GPIOD[13](DAC_EN) --> High
#if 1
			tcc_gpio_config(GPIO_DAC_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_DAC_EN, 0);
#endif


	#if (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII) 	
			tcc_gpio_config(GPIO_5V5_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_5V5_EN, 0);
			mdelay(10);
	#endif

#if 1
			//** GPIOF[0](OPAMP_EN) --> LOW
			tcc_gpio_config(GPIO_OPAMP_PWR_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_OPAMP_PWR_EN, 0);

			//dean 20170830 unbal noise fixed 
			#if (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII)
                     tcc_gpio_config(GPIO_OPAMP_UNBAL_PWR_EN, GPIO_FN(0));
		       gpio_direction_output(GPIO_OPAMP_UNBAL_PWR_EN, 0);                        
                      #endif
                    ////////////////////   
#endif

#ifdef _AK_REMOVE_
			//** GPIOF[11](N2V8_EN) --> LOW
			tcc_gpio_config(GPIO_N2V8_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_N2V8_EN, 0);
#endif

#if( CUR_AK != MODEL_AK380 && CUR_AK != MODEL_AK320 && CUR_AK != MODEL_AK300)
			tcc_gpio_config(GPIO_5V5_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_5V5_EN, 0);
#endif
#if 1  // tick sound. 
#if (CUR_AK >= MODEL_AK240)

			
			#ifdef _AK_REMOVE_
			tcc_gpio_config(GPIO_OPAMP_4V_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_OPAMP_4V_EN, 0);
			#endif
			
			//				msleep_interruptible(30);
#endif
#endif

		}
		break;

#if OPTICAL_RX_FUNC
	case AK_OPTICAL_RX_POWER:
		tcc_gpio_config(GPIO_OP_RX_EN,GPIO_FN(0));
		if(pwr_ctl==AK_PWR_ON) {
			gpio_direction_output(GPIO_OP_RX_EN,1);
			wm8804_spdif_rx_pwr(1);
		} else {
			gpio_direction_output(GPIO_OP_RX_EN,0);
			wm8804_spdif_rx_pwr(0);
		}
		//   DBG_PRINT("TEST","AK_OPTICAL_RX_POWER %s\n",pwr_ctl==AK_PWR_ON ? "ON":"OFF");

		break;
#endif
	case AK_OPTICAL_TX_POWER:
		#ifdef ENABLE_OPTICAL_TX
		tcc_gpio_config(GPIO_OP_TX_PWEN,GPIO_FN(0));

		if(pwr_ctl==AK_PWR_ON) {
			gpio_direction_output(GPIO_OP_TX_PWEN,1);
			wm8804_spdif_tx_pwr(1);
		} else {
			gpio_direction_output(GPIO_OP_TX_PWEN,0);
			wm8804_spdif_tx_pwr(0);
		}
		#endif
		//            DBG_PRINT("TEST","AK_OPTICAL_TX_POWER %s\n",pwr_ctl==AK_PWR_ON ? "ON":"OFF");

		break;
	case AK_AUDIO_POWER_SUSPEND:
		printk("************ AK_AUDIO_POWER_SUSPEND\n\n\n");

#if (CUR_AK >= MODEL_AK240)
#ifndef SLEEP_CURRENT_CHECK
		#if (CUR_AK_REV != AK380_HW_EVM)
		#ifdef _AK_REMOVE_
		tcc_gpio_config(GPIO_AUDIO_N5V_EN, GPIO_FN(0));
		gpio_direction_output(GPIO_AUDIO_N5V_EN, 0);
		#endif
		
		#endif
#else
		#if (CUR_AK_REV != AK380_HW_EVM)
		tcc_gpio_config(GPIO_AUDIO_N5V_EN, GPIO_FN(0) | GPIO_INPUT);
		#endif
#endif

		#ifdef _AK_REMOVE_
		tcc_gpio_config(GPIO_OPAMP_4V_EN, GPIO_FN(0));
		gpio_direction_output(GPIO_OPAMP_4V_EN, 0);
		#endif
		
#endif

		//** GPIO_5V5_EN--> Low
		tcc_gpio_config(GPIO_5V5_EN, GPIO_FN(0));
		gpio_direction_output(GPIO_5V5_EN, 0);

		//** GPIOD[13](DAC_EN) --> Low
		tcc_gpio_config(GPIO_DAC_EN, GPIO_FN(0));
		gpio_direction_output(GPIO_DAC_EN, 0);

		//WM8740 Reset(High Active)
		//  tcc_gpio_config(GPIO_DAC_RST, GPIO_FN(0));
		// gpio_direction_output(GPIO_DAC_RST, 0);

		//  mdelay(100);
		//** GPIOF[0](OPAMP_EN) --> Low
		tcc_gpio_config(GPIO_OPAMP_PWR_EN, GPIO_FN(0));
		gpio_direction_output(GPIO_OPAMP_PWR_EN, 0);

		#ifdef _AK_REMOVE_
		//** (N2V8_EN) --> Low
		tcc_gpio_config(GPIO_N2V8_EN, GPIO_FN(0));
		gpio_direction_output(GPIO_N2V8_EN, 0);
		#endif
		
#if OPTICAL_RX_FUNC
		tcc_gpio_config(GPIO_OP_RX_EN,GPIO_FN(0));
		gpio_direction_output(GPIO_OP_RX_EN, 0);
#endif

		#ifdef ENABLE_OPTICAL_TX
		tcc_gpio_config(GPIO_OP_TX_PWEN,GPIO_FN(0));
		gpio_direction_output(GPIO_OP_TX_PWEN, 0);
		#endif
		
		break;
	default:
		break;

	}

	if(pwr_ctl == AK_PWR_ON) {
		g_ak_audio_power_status |= snd_ctl;
	} else {
		g_ak_audio_power_status &= ~snd_ctl;
	}

	// 2015.06.29 tonny - release semaphore
	up(&pwr_ctl_sem);
}
EXPORT_SYMBOL(ak_set_snd_pwr_ctl);

static DEFINE_MUTEX(g_spdif_mode);

static int g_wm8804_spdif_mode = SND_WM8804_PLAY;

snd_wm8804_mode_t ak_get_wm8804_spdif_mode(void)
{
	int mode;


	mutex_lock(&g_spdif_mode);
	mode = g_wm8804_spdif_mode;
	mutex_unlock(&g_spdif_mode);
	return mode;
}
void ak_set_wm8804_spdif_mode(snd_wm8804_mode_t snd_ctl)
{
	int play_mode ;

	mutex_lock(&g_spdif_mode);

	g_wm8804_spdif_mode = snd_ctl;

	switch(snd_ctl) {
	case SND_WM8804_PLAY:
		play_mode = ak_get_internal_play_mode();
		if(play_mode == INTERNAL_PCM_PLAY) {

			ak_set_hw_mute(AMT_HP_MUTE,1);
			msleep_interruptible(100);
#if (CUR_DAC==CODEC_CS4398)
			cs4398_reg_init();
#elif(CUR_DAC == CODEC_AK4490)
			ak4490_reg_init();
#endif

            printk("%s -- ADP_PLAYBACK\n", __func__);
			ak_set_audio_path(ADP_PLAYBACK);

#ifndef FORCE_USE_OPTICAL_RX_FUNC
			ak_set_snd_pwr_ctl(AK_OPTICAL_RX_POWER,AK_PWR_OFF);
#endif

			ak_set_snd_pwr_ctl(AK_OPTICAL_TX_POWER,AK_PWR_OFF);
			ak_set_snd_pwr_ctl(AK_AUDIO_POWER,AK_PWR_ON);

			msleep_interruptible(100);
			ak_set_hw_mute(AMT_HP_MUTE,0);
		} else if (play_mode == INTERNAL_DSD_PLAY) {
			ak_set_internal_play_mode(INTERNAL_PCM_PLAY);
		}

		break;
#if OPTICAL_RX_FUNC
	case SND_WM8804_SPDIF_BYPASS: {
		ak_set_hw_mute(AMT_HP_MUTE,1);
#if(CUR_DAC == CODEC_CS4398)
		cs4398_mute(1);
#endif

		msleep_interruptible(10);
		ak_set_snd_pwr_ctl(AK_AUDIO_POWER,AK_PWR_ON);

		ak_set_audio_path(ADP_OPTICAL_IN);
		ak_set_snd_pwr_ctl(AK_OPTICAL_RX_POWER,AK_PWR_ON);

		//	set_spdif_pll(2);
		msleep_interruptible(500);
		ak_set_hw_mute(AMT_HP_MUTE,0);
#if(CUR_DAC == CODEC_CS4398)
		cs4398_mute(0);
#endif
	}
	break;
#endif

	case SND_WM8804_SPDIF_TX:
		ak_set_hw_mute(AMT_HP_MUTE,1);
#if(CUR_DAC == CODEC_CS4398)
		cs4398_mute(1);
#endif

        printk("%s -- ADP_OPTICAL_OUT\n", __func__);
		ak_set_audio_path(ADP_OPTICAL_OUT);
		ak_set_snd_pwr_ctl(AK_OPTICAL_TX_POWER,AK_PWR_ON);

		ak_set_hw_mute(AMT_HP_MUTE,0);
#if(CUR_DAC == CODEC_CS4398)
		cs4398_mute(0);
#endif

		break;
	}

	mutex_unlock(&g_spdif_mode);
}

#ifdef SUPPORT_EXT_AMP
void ak_route_amp_hp_balanced_out(int aot)
{
	//ak_set_hw_mute(AMT_HP_MUTE,1);

	if(aot==AOT_HP) {
		printk("ak_route_amp_hp_balanced_out: AOT_HP\n");
		amp_bal_unbal_select(AOT_HP);
		amp_bal_mute(1);
		
		#ifdef SUPPORT_EXT_AMP
		amp_unbal_mute(0);
		#endif
		
	}else{
		printk("ak_route_amp_hp_balanced_out: AOT_BALANCED\n");
		amp_bal_unbal_select(AOT_BALANCED);
		amp_bal_mute(0);
		
		#ifdef SUPPORT_EXT_AMP
		amp_unbal_mute(1);
		#endif
	}
	//ak_set_hw_mute(AMT_HP_MUTE,0);
}
EXPORT_SYMBOL(ak_route_amp_hp_balanced_out);
#endif

#if BALANCED_OUT_FUNC
void ak_route_hp_balanced_out(int aot)
{
#if (CUR_AK == MODEL_AK500N)
	gpio_request(GPIO_HP_XLR_SEL, "GPIO_HP_XLR_SEL");
	tcc_gpio_config(GPIO_HP_XLR_SEL, GPIO_FN(0));

	if(aot==AOT_HP) {
		gpio_direction_output(GPIO_HP_XLR_SEL,1);
		//CPRINT("Hp Connected\n");
	} else {
		gpio_direction_output(GPIO_HP_XLR_SEL,0);
		//CPRINT("Balanced Out Connected\n");
	}
	#ifndef REDUCE_BALANCED_POPNOISE
	ak_set_hw_mute(AMT_HP_MUTE,0);
	#endif
#elif (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII)   //dean 20170803 power on 3.5이어폰 꽂힌상태에서 밸런스아웃 활성화 -> 노래재생 -> 밸런스아웃 비활성화시 3.5에서  mute 걸리는 문제 
	gpio_request(GPIO_BAL_UNBAL_SEL, "GPIO_BAL_UNBAL_SEL");
	gpio_request(GPIO_OPAMP_PWR_EN, "GPIO_OPAMP_PWR_EN");
	gpio_request(GPIO_HP_MUTE, "GPIO_HP_MUTE");
	gpio_request(GPIO_OPAMP_UNBAL_PWR_EN, "GPIO_OPAMP_UNBAL_PWR_EN");

	tcc_gpio_config(GPIO_BAL_UNBAL_SEL, GPIO_FN(0));
	tcc_gpio_config(GPIO_HP_MUTE,GPIO_FN(0) | GPIO_CD(0));	

	if(aot==AOT_HP) {

		ak_set_hw_mute(AMT_HP_MUTE,1);
		//msleep_interruptible(100);
			
		gpio_direction_output(GPIO_BAL_UNBAL_SEL, 0);
		gpio_direction_output(GPIO_HP_BAL_MUTE,1);  //2015.4.17.ysyim: ak380 음악 재생->BAL 연결 -> 간헐적 무음 발생 문제 수
		if (g_ak_audio_power_status != AK_AUDIO_POWER_ALL_OFF)
		{
			tcc_gpio_config(GPIO_OPAMP_UNBAL_PWR_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_OPAMP_UNBAL_PWR_EN, 1);	

			tcc_gpio_config(GPIO_OPAMP_PWR_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_OPAMP_PWR_EN, 0);	
		}
		gpio_direction_output(GPIO_HP_MUTE,0);

		ak_set_hw_mute(AMT_HP_MUTE,0);

		CPRINT("Hp Connected\n");
	} else {

		ak_set_hw_mute(AMT_HP_MUTE,1);
		//msleep_interruptible(100);
		
		gpio_direction_output(GPIO_BAL_UNBAL_SEL,1);
		gpio_direction_output(GPIO_HP_MUTE,1);
		if (g_ak_audio_power_status != AK_AUDIO_POWER_ALL_OFF)
		{
			tcc_gpio_config(GPIO_OPAMP_PWR_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_OPAMP_PWR_EN, 1);		

			//dean 20170830 unbal noise fixed
			tcc_gpio_config(GPIO_OPAMP_UNBAL_PWR_EN, GPIO_FN(0));
			gpio_direction_output(GPIO_OPAMP_UNBAL_PWR_EN, 0);	
			//////////////
		}
		gpio_direction_output(GPIO_HP_BAL_MUTE, 0); //2015.4.17.ysyim: ak380 음악 재생->BAL 연결 -> 간헐적 무음 발생 문제 수

		ak_set_hw_mute(AMT_HP_MUTE,0);

		CPRINT("Balanced Out Connected\n");
	}
	#ifndef REDUCE_BALANCED_POPNOISE
	ak_set_hw_mute(AMT_HP_MUTE,0);
	#endif
#elif (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == ORG)  //dean 20170803 power on 3.5이어폰 꽂힌상태에서 밸런스아웃 활성화 -> 노래재생 -> 밸런스아웃 비활성화시 3.5에서  mute 걸리는 문제 
	gpio_request(GPIO_BAL_UNBAL_SEL, "GPIO_BAL_UNBAL_SEL");
	tcc_gpio_config(GPIO_BAL_UNBAL_SEL, GPIO_FN(0));

	gpio_request(GPIO_HP_MUTE, "GPIO_HP_MUTE");
	tcc_gpio_config(GPIO_HP_MUTE,GPIO_FN(0) | GPIO_CD(0));	

	if(aot==AOT_HP) {
		gpio_direction_output(GPIO_BAL_UNBAL_SEL, 0);
		gpio_direction_output(GPIO_HP_BAL_MUTE,1);  //2015.4.17.ysyim: ak380 음악 재생->BAL 연결 -> 간헐적 무음 발생 문제 수
		gpio_direction_output(GPIO_HP_MUTE,0);
		CPRINT("Hp Connected\n");
	} else {
		gpio_direction_output(GPIO_BAL_UNBAL_SEL,1);
		gpio_direction_output(GPIO_HP_BAL_MUTE, 0); //2015.4.17.ysyim: ak380 음악 재생->BAL 연결 -> 간헐적 무음 발생 문제 수
		gpio_direction_output(GPIO_HP_MUTE,1);
		CPRINT("Balanced Out Connected\n");
	}
	#ifndef REDUCE_BALANCED_POPNOISE
	ak_set_hw_mute(AMT_HP_MUTE,0);
	#endif
#else
	gpio_request(GPIO_BAL_UNBAL_SEL, "GPIO_BAL_UNBAL_SEL");
	tcc_gpio_config(GPIO_BAL_UNBAL_SEL, GPIO_FN(0));

	if(aot==AOT_HP) {
		gpio_direction_output(GPIO_BAL_UNBAL_SEL, 0);
		gpio_direction_output(GPIO_HP_BAL_MUTE,1);  //2015.4.17.ysyim: ak380 음악 재생->BAL 연결 -> 간헐적 무음 발생 문제 수
		//CPRINT("Hp Connected\n");
	} else {
		gpio_direction_output(GPIO_BAL_UNBAL_SEL,1);
		gpio_direction_output(GPIO_HP_BAL_MUTE, 0); //2015.4.17.ysyim: ak380 음악 재생->BAL 연결 -> 간헐적 무음 발생 문제 수
		//CPRINT("Balanced Out Connected\n");
	}
	#ifndef REDUCE_BALANCED_POPNOISE
	ak_set_hw_mute(AMT_HP_MUTE,0);
	#endif
#endif
}
#endif

extern void ak_usb_plug_change(int connect_type);
extern int get_adapter_state(void);
extern void usb_check_refresh(void);
extern void usb_check_refresh_now(void);
extern void usb_check_refresh_mode_change(void);
extern void set_usb_connect_state(int state);
extern int get_current_pcfi_state(void);
extern int get_usb_checking_state(void);

void ak_set_sw_mute_zero(void)
{
	//printk("============= sw mute low0 ==============\n"); 
	gpio_request(GPIO_AUDIO_SW_MUTE, "GPIO_AUDIO_SW_MUTE");
	tcc_gpio_config(GPIO_AUDIO_SW_MUTE,GPIO_FN(0));
	gpio_direction_output2(GPIO_AUDIO_SW_MUTE, 1);
}
//
// USB Route setting.
//
void ak_set_usb_conn_mode(USB_CONN_MODE ucm)
{
	switch(ucm) {
	case UCM_MTP:
		DBG_PRINT("USB","MTP Mode \n");

		if(get_current_pcfi_state()) {
			ak_set_hw_mute(AMT_HP_MUTE, 1);
			msleep_interruptible(200);
		}

#if (CUR_AK == MODEL_AK500N)        
		if(usb_conn_mode == UCM_PC_FI)
			ak_set_snd_out_path();
#endif
        
		usb_conn_mode = UCM_MTP;

		if (!get_boot_complete()) {
			printk("\x1b[1;36m %s -- Now booting....(MTP mode) \x1b[0m\n", __func__);
			return;
		}
#if (CUR_AK == MODEL_AK500N)
#if (CUR_AK_REV <= AK500N_HW_WS2)
		if (!gpio_get_value(GPIO_USB_DET)) {
#else
		if (gpio_get_value(GPIO_USB_DET)) {
#endif
			printk("\x1b[1;36m %s -- USB unplugged(MTP mode) \x1b[0m\n", __func__);
			return;
		}
#else
		if (ak_check_usb_unplug()) {
			printk("\x1b[1;36m %s -- USB unplugged(MTP mode) \x1b[0m\n", __func__);
			return;
		}
#endif

#if (CUR_AK != MODEL_AK500N)
		/* [downer] A140115 */
		if (get_adapter_state()) {
			printk("\x1b[1;36m %s -- Adapter state \x1b[0m\n", __func__);
			return;
		}

		if (get_usb_checking_state()) {
			printk("\x1b[1;36m %s -- USB checking state \x1b[0m\n", __func__);
			return;
		}
#endif
		usb_check_refresh_mode_change(); /* [downer] A140115 for USB/Adapter check */

		//        ak_usb_plug_change(UCM_MTP);
		break;

	case UCM_PC_FI:
		DBG_PRINT("USB","USB DAC Mode \n");
		usb_conn_mode = UCM_PC_FI;
        
		if (!get_boot_complete()) {
			printk("\x1b[1;36m %s -- Now booting....(USB DAC mode) \x1b[0m\n", __func__);
			return;
		}

#if (CUR_AK == MODEL_AK500N)
#if (CUR_AK_REV <= AK500N_HW_WS2)
		if (!gpio_get_value(GPIO_USB_DET)) {
#else
		if (gpio_get_value(GPIO_USB_DET)) {
#endif
			printk("\x1b[1;36m %s -- USB unplugged(USB DAC mode) \x1b[0m\n", __func__);
			return;
		}
#else
		if (ak_check_usb_unplug()) {
			printk("\x1b[1;36m %s -- USB unplugged(USB DAC mode) \x1b[0m\n", __func__);
			return;
		}
#endif

#if (CUR_AK != MODEL_AK500N)
		/* [downer] A140115 */
		if (get_adapter_state()) {
			printk("\x1b[1;36m %s -- Adapter state \x1b[0m\n", __func__);
			return;
		}

		if (get_usb_checking_state()) {
			printk("\x1b[1;36m %s -- USB checking state \x1b[0m\n", __func__);
			return;
		}
#endif
		//ak_set_hw_mute(AMT_HP_MUTE, 1);

		usb_check_refresh_mode_change(); /* [downer] A140115 for USB/Adapter check */

		//        ak_usb_plug_change(UCM_PC_FI);
		//        ak_set_audio_path(ADP_USB_DAC);
		//        ak_set_hw_mute(AMT_HP_MUTE, 0);

		ak_set_sw_mute_zero();

		break;
	default:
		break;

	}
}
EXPORT_SYMBOL(ak_set_usb_conn_mode);

extern void write_xmos_ptr_str(int val);

void ak_internal_play_change(INTERNAL_PLAY_MODE mode)
{
#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300 || CUR_AK == MODEL_AK500N)
	switch(mode) {
	case INTERNAL_DSD_PLAY: 
#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)
		write_xmos_ptr_str(2);//"xmos_ready"

		//printk("============= USB20 Host Enable  ==============\n");
		gpio_request(GPIO_USB20_HOST_EN, "usb20_host_en");
		tcc_gpio_config(GPIO_USB20_HOST_EN, GPIO_FN(0));
		gpio_direction_output(GPIO_USB20_HOST_EN, 1);
#elif (CUR_AK == MODEL_AK500N)
		//printk("============= USB20 Host Enable  ==============\n");
		gpio_request(GPIO_USB_ONOFF, "usb20_host_en");
		tcc_gpio_config(GPIO_USB_ONOFF, GPIO_FN(0));
		gpio_direction_output(GPIO_USB_ONOFF, 1);
#endif

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)        
		//printk("============= xmos Power Off ==============\n");
		gpio_request(GPIO_XMOS_PWR_EN, "xmos_pwr_en");
		tcc_gpio_config(GPIO_XMOS_PWR_EN, GPIO_FN(0));
		gpio_direction_output(GPIO_XMOS_PWR_EN, 0);
		//mdelay(500);
#endif
        
#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300 || CUR_AK == MODEL_AK500N)
		//printk("============= Connect XMOS to flash ==========\n");
		gpio_request(GPIO_XMOS_SPI_SEL, "GPIO_XMOS_SPI_SEL");
		tcc_gpio_config(GPIO_XMOS_SPI_SEL, GPIO_FN(0));
		gpio_direction_output(GPIO_XMOS_SPI_SEL, 1);
		//mdelay(1000);
#endif

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK500N)        
		//printk("============= cpu xmos Path Enable ==========\n");
		gpio_request(GPIO_XMOS_USB_EN, "GPIO_XMOS_USB_EN");
		tcc_gpio_config(GPIO_XMOS_USB_EN,GPIO_FN(0));
		gpio_direction_output(GPIO_XMOS_USB_EN, 0);
#endif

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)
		//printk("============= Host Mode Enable =============\n");
		gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
		tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
		gpio_direction_output(GPIO_USB_DAC_SEL, 1);
		mdelay(200);
        
#elif (CUR_AK == MODEL_AK500N)
		gpio_request(GPIO_CPU_XMOS_EN, "GPIO_CPU_XMOS_EN");
		tcc_gpio_config(GPIO_CPU_XMOS_EN,GPIO_FN(0));
		gpio_direction_output(GPIO_CPU_XMOS_EN, 0);
		mdelay(200);
#endif

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300) //jimi.0123
		if(get_adapter_state()) {
			gpio_request(GPIO_USB_ON, "GPIO_USB_ON");
			tcc_gpio_config(GPIO_USB_ON,GPIO_FN(0));
			gpio_direction_output(GPIO_USB_ON, 1);
		}
#endif

#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)                
		//printk("============= xmos to dac connection =========\n");
		gpio_request(GPIO_I2C_SEL, "GPIO_I2C_SEL");
		tcc_gpio_config(GPIO_I2C_SEL,GPIO_FN(0));
		gpio_direction_output(GPIO_I2C_SEL,1);

		//printk("============= CPU I2S Disable =============\n");
		gpio_request(GPIO_CPU_I2S_EN, "GPIO_I2C_SEL");
		tcc_gpio_config(GPIO_CPU_I2S_EN,GPIO_FN(0));
		gpio_direction_output(GPIO_CPU_I2S_EN, 1);

		mdelay(800);
		printk("============= xmos Power On ==============\n");
		gpio_request(GPIO_XMOS_PWR_EN, "xmos_pwr_en");
		tcc_gpio_config(GPIO_XMOS_PWR_EN, GPIO_FN(0));
		gpio_direction_output(GPIO_XMOS_PWR_EN, 1);

		printk("\x1b[1;36m %s --xmos_on! \x1b[0m\n", __func__);

		write_xmos_ptr_str(1);//"xmos_on"
#endif
		break;

	case INTERNAL_PCM_PLAY: 
#if (CUR_AK == MODEL_AK240 ||  CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)                        
		write_xmos_ptr_str(0);//"xmos_off"

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
        
#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK500N)                
		//printk("============= XMOS Path Disable \n");
		gpio_request(GPIO_XMOS_USB_EN, "GPIO_XMOS_USB_EN");
		tcc_gpio_config(GPIO_XMOS_USB_EN,GPIO_FN(0));
		gpio_direction_output(GPIO_XMOS_USB_EN, 1);
#endif
        
		break;
	case INTERNAL_NONE_PLAY: 
#if (CUR_AK == MODEL_AK240 ||  CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)                        
		write_xmos_ptr_str(0);//"xmos_off"

		//printk("============= xmos Power Off ==============\n");
		gpio_request(GPIO_XMOS_PWR_EN, "xmos_pwr_en");
		tcc_gpio_config(GPIO_XMOS_PWR_EN, GPIO_FN(0));
		gpio_direction_output(GPIO_XMOS_PWR_EN, 0);
#endif
		break;
    }
#endif
}


void ak_set_internal_play_mode_only(INTERNAL_PLAY_MODE mode)
{
	internal_play_mode = mode;
}

void ak_set_internal_play_mode(INTERNAL_PLAY_MODE mode)
{

#if(AK_HAVE_XMOS==1)
	// tonny - obtain semaphore
	// printk("============= ak_set_internal_play_mode:%d\n",mode);
	down(&dsd_usb_sem);
#endif

	switch(mode) {
	case INTERNAL_DSD_PLAY: {
		ak_set_hw_mute(AMT_HP_MUTE, 1);
		msleep_interruptible(150);

		DBG_PRINT("PLAY","Internal DSD Mode \n");
		
		fix_cpu_clock(1);

		internal_play_mode = INTERNAL_DSD_PLAY;
		ak_internal_play_change(internal_play_mode);

		ak_set_audio_path(ADP_USB_DAC);

		ak_set_snd_pwr_ctl(AK_OPTICAL_TX_POWER,AK_PWR_OFF); //DSD 일경우에  OPTICAL TX Power OFF

		if(ak_get_snd_pwr_ctl(AK_AUDIO_POWER) == 0) {
			ak_set_snd_pwr_ctl(AK_AUDIO_POWER, AK_PWR_ON);
		}

		msleep(200);

#if (CUR_AK == MODEL_AK240) //2014.11.26 tonny : 500N에서 dsd 재생 시 팝 노이즈 개선.
		ak_set_hw_mute(AMT_HP_MUTE, 0);
#endif
		ak_set_sw_mute_zero();

		set_audio_power_ctl_status(FALSE); //Don't move because this makes dac power off problem of timing issue on playing DSD
		break;
	}

	case INTERNAL_PCM_PLAY: {
#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)            
			DBG_PRINT("PLAY","Internal PCM Mode \n");
			
			fix_cpu_clock(0);

#if (CUR_AK != MODEL_AK380 && CUR_AK != MODEL_AK320 && CUR_AK != MODEL_AK300)
			ak_set_snd_pwr_ctl(AK_AUDIO_POWER, AK_PWR_OFF);
#endif
			{
#if (CUR_AK == MODEL_AK240 || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)                
				//printk("============= cpu to dac connection \n");
				gpio_request(GPIO_I2C_SEL, "GPIO_I2C_SEL");
				tcc_gpio_config(GPIO_I2C_SEL,GPIO_FN(0));
				gpio_direction_output(GPIO_I2C_SEL, 0);
#endif
			}
	
#if (CUR_DAC==CODEC_CS4398)
			cs4398_volume_mute();
#endif
			msleep_interruptible(300);
	
			ak_set_hw_mute(AMT_HP_MUTE, 1);
			msleep_interruptible(100);
			ak_set_sw_mute_zero();
		#if 0  // audio power off : for fix popnoise . 2014.08.25
			if(ak_get_snd_pwr_ctl(AK_AUDIO_POWER) == 0) {
				msleep_interruptible(100);
	
				ak_set_snd_pwr_ctl(AK_AUDIO_POWER, AK_PWR_ON);
				msleep_interruptible(100);
				//cs4398_reg_init();
			}
		#endif
			if(ak_is_connected_optical_tx())  {  // optical tx
				ak_set_snd_pwr_ctl(AK_OPTICAL_TX_POWER,AK_PWR_ON);
			}
			internal_play_mode = INTERNAL_PCM_PLAY;
			ak_internal_play_change(internal_play_mode);
	
			msleep_interruptible(20);
	
#if (CUR_DAC==CODEC_CS4398)
			cs4398_reg_init();// 차후 9번 레지터터만 초기화 고려.
#elif(CUR_DAC == CODEC_AK4490)
			ak4490_reg_init();			
#endif
	
			set_audio_power_ctl_status(FALSE);
			break;

#endif

		break;
	}

	case INTERNAL_NONE_PLAY: {
		DBG_PRINT("PLAY","Internal NONE Mode \n");

		ak_set_hw_mute(AMT_HP_MUTE, 1);

		internal_play_mode = INTERNAL_NONE_PLAY;
		ak_internal_play_change(internal_play_mode);

		break;
	}
	}

#if(AK_HAVE_XMOS==1)
	//tonny - release semaphore
	up(&dsd_usb_sem);
	// printk("============= ak_set_internal_play_mode1\n");

#endif

}

EXPORT_SYMBOL(ak_set_internal_play_mode);

#if 0
void ak_xmos_reboot(void)
{
	set_xmos_pwr_str(0);

	printk("============= USB20 Host Disable  ==============\n");
	gpio_request(GPIO_USB20_HOST_EN, "usb20_host_en");
	tcc_gpio_config(GPIO_USB20_HOST_EN, GPIO_OUTPUT | GPIO_FN(0));
	gpio_direction_output(GPIO_USB20_HOST_EN, 0);

	printk("============ xMos Power Off ===========\n");
	gpio_request(GPIO_XMOS_PWR_EN, "xmos_pwr_en");
	tcc_gpio_config(GPIO_XMOS_PWR_EN, GPIO_FN(0));
	gpio_direction_output(GPIO_XMOS_PWR_EN, 0);

	//printk("============= XMOS Path Disable \n");
	gpio_request(GPIO_XMOS_USB_EN, "GPIO_XMOS_USB_EN");
	tcc_gpio_config(GPIO_XMOS_USB_EN,GPIO_FN(0));
	gpio_direction_output(GPIO_XMOS_USB_EN, 1);

#if (CUR_AK_REV >= AK240_HW_ES)
	//printk("============= MTP Path Enable \n");
	gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
	tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
	gpio_direction_output(GPIO_USB_DAC_SEL, 0);	//interal 재생 중 adapter detactor 문제로 low 고정.
#endif
	mdelay(5);


	//printk("============= USB20 Host Enable  ==============\n");
	gpio_request(GPIO_USB20_HOST_EN, "usb20_host_en");
	tcc_gpio_config(GPIO_USB20_HOST_EN, GPIO_OUTPUT | GPIO_FN(0));
	gpio_direction_output(GPIO_USB20_HOST_EN, 1);

#if(CUR_AK_REV >= AK240_HW_ES)
	//printk("============= Connect XMOS to flash ==========\n");
	gpio_request(GPIO_XMOS_SPI_SEL, "GPIO_XMOS_SPI_SEL");
	tcc_gpio_config(GPIO_XMOS_SPI_SEL, GPIO_FN(0));
	gpio_direction_output(GPIO_XMOS_SPI_SEL, 1);
	mdelay(1000);
#endif
	//printk("============= cpu xmos Path Enable ==========\n");
	gpio_request(GPIO_XMOS_USB_EN, "GPIO_XMOS_USB_EN");
	tcc_gpio_config(GPIO_XMOS_USB_EN,GPIO_FN(0));
	gpio_direction_output(GPIO_XMOS_USB_EN, 0);

#if(CUR_AK_REV >= AK240_HW_ES)
	//printk("============= Host Mode Enable =============\n");
	gpio_request(GPIO_USB_DAC_SEL, "GPIO_USB_DAC_SEL");
	tcc_gpio_config(GPIO_USB_DAC_SEL,GPIO_FN(0));
	gpio_direction_output(GPIO_USB_DAC_SEL, 1);
	mdelay(200);
#endif

	//printk("============= CPU I2S Disable =============\n");
	gpio_request(GPIO_CPU_I2S_EN, "GPIO_I2C_SEL");
	tcc_gpio_config(GPIO_CPU_I2S_EN,GPIO_FN(0));
	gpio_direction_output(GPIO_CPU_I2S_EN, 1);

	mdelay(20);
	printk("============= xmos Power On ==============\n");
	gpio_request(GPIO_XMOS_PWR_EN, "xmos_pwr_en");
	tcc_gpio_config(GPIO_XMOS_PWR_EN, GPIO_FN(0));
	gpio_direction_output(GPIO_XMOS_PWR_EN, 1);

	//gpio_direction_output(GPIO_HP_MUTE, 0);
	set_xmos_pwr_str(1);

}
EXPORT_SYMBOL(ak_xmos_reboot);
#endif

USB_CONN_MODE ak_get_current_usb_mode(void)
{
	return usb_conn_mode;
}
EXPORT_SYMBOL(ak_get_current_usb_mode);

INTERNAL_PLAY_MODE ak_get_internal_play_mode(void)
{
	return internal_play_mode;
}


#ifdef ENABLE_DEBUG_PORT

void splash_debug_port()
{
	//	tcc_gpio_config(GPIO_DBG_PORT,GPIO_OUTPUT | GPIO_FN0);
}
#endif


/*
 * [downer] A131017
 */
#if defined(CONFIG_INPUT_GPIO)
extern void ak_gpio_key_lock_mode(int mode);
extern void ak_wheel_lock_mode(int mode);
static int g_keylock_state = 0;

int get_keylock_state(void)
{
	return g_keylock_state;
}
EXPORT_SYMBOL(get_keylock_state);

void ak_set_keylock_mode(int mode)
{
	if (mode)
		g_keylock_state = 1;
	else
		g_keylock_state = 0;

	//    ak_wheel_lock_mode(mode); // [downer] M140211 휠 키 잠금은 lcd 드라이버에서...
}
EXPORT_SYMBOL(ak_set_keylock_mode);

void ak_set_sidekey_lock_mode(int mode)
{
	ak_gpio_key_lock_mode(mode);
}
EXPORT_SYMBOL(ak_set_sidekey_lock_mode);

void ak_set_wheelkey_lock_mode(int mode)
{
	if (!mode)
		mode = g_keylock_state;

	ak_wheel_lock_mode(mode);
}
EXPORT_SYMBOL(ak_set_wheelkey_lock_mode);

#endif

#if (CUR_AK == MODEL_AK500N)    /* [downer] M140826 for Adapter mode */
#if (CUR_AK_REV == AK500N_HW_ES1) || (CUR_AK_REV == AK500N_HW_TP)
int set_playing_mode(int flag)
{
	if (g_optical_rx_status == 1)
		flag = 1;
		
	g_current_playing = flag;
}
EXPORT_SYMBOL(set_playing_mode);

void ak_set_adapter_playing_mode(int mode)
{
	g_adapter_playing = mode;

	if (mode)                   /* 옵션을 키면 바로 AC 전원을 킨다. */
		connect_AC();
	else 
        disconnect_AC();
}
EXPORT_SYMBOL(ak_set_adapter_playing_mode);

static int g_cdrom_playing = 0;
int ak_cdripping_playing_mode(int mode)
{
	g_cdrom_playing = mode;
	
}
EXPORT_SYMBOL(ak_cdripping_playing_mode);

int get_cdrom_state(void)
{
    return g_cdrom_playing;
}
EXPORT_SYMBOL(get_cdrom_state);
#endif
#endif

/*
 * [downer] A131217
 */
#if defined(CONFIG_KEYBOARD_GPIO)
extern void ak_power_key_lock_mode(int mode);

void ak_set_powerkey_lock_mode(int mode)
{
	ak_power_key_lock_mode(mode);
}
EXPORT_SYMBOL(ak_set_powerkey_lock_mode);
#endif

/*
 * [downer] A140109
 */
static int g_system_boot_complete = 0;
static int g_dock_event_ready = 0;

extern void ak_usb_process(int mode);
void set_boot_complete(void)
{
	printk("\x1b[1;31m %s \x1b[0m\n", __func__);
	g_system_boot_complete = 1;

#if (CUR_AK != MODEL_AK500N)    /* [downer] M140822 avoid duplicate USB event */
	ak_usb_process(2);
#endif
}
EXPORT_SYMBOL(set_boot_complete);

int get_boot_complete(void)
{
	return g_system_boot_complete;
}
EXPORT_SYMBOL(get_boot_complete);

void set_dock_event_ready(void)
{
	printk("\x1b[1;31m %s \x1b[0m\n", __func__);
	g_dock_event_ready = 1;

}
EXPORT_SYMBOL(set_dock_event_ready);

int get_dock_event_ready(void)
{
	return g_dock_event_ready;
}
EXPORT_SYMBOL(get_dock_event_ready);

//
// Audio Power Control.
//

struct audio_power_ctl_struct {
	struct delayed_work work;
	struct workqueue_struct *wq;
	int audio_power_off_enable;
};

static DEFINE_MUTEX(audio_power_ctl_mutex);

static struct audio_power_ctl_struct g_audio_power_ctl;

static int g_audio_power_off_count = 0xff;

void set_audio_power_ctl_status(int onoff)
{

	mutex_lock( &audio_power_ctl_mutex);
	g_audio_power_ctl.audio_power_off_enable = onoff;

	if(onoff==1) {
		g_audio_power_off_count = 0;
	} else {
		g_audio_power_off_count = 0xff;
	}
	mutex_unlock( &audio_power_ctl_mutex);
}

static void trigger_delayed_work(void)
{
	queue_delayed_work(g_audio_power_ctl.wq, &g_audio_power_ctl.work, msecs_to_jiffies(1000));
}
#if (CUR_AK == MODEL_AK240 || CUR_AK==MODEL_AK500N || CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
#define AUDIO_AUTO_POWER_OFF_TIME (30)
#else
#define AUDIO_AUTO_POWER_OFF_TIME (5)
#endif
static void audio_power_off_ctl_work(struct work_struct *work)
{
	mutex_lock( &audio_power_ctl_mutex);

	if(g_audio_power_ctl.audio_power_off_enable) {

		if(g_audio_power_off_count++==AUDIO_AUTO_POWER_OFF_TIME) {   // audio power off after 5sec.

			int usb_con_mode = get_current_usb_device();

#ifdef PRINT_ALSA_MSG
			CPRINT("Audio Power OFF %d\n",usb_con_mode);
#endif

			if((usb_con_mode==UCM_PC_FI)
				|| ak_get_internal_play_mode() == INTERNAL_DSD_PLAY)	{  // PC_FI 일겨우에는 끄지 않는다.

			} else {
				ak_set_snd_pwr_ctl(AK_AUDIO_POWER, AK_PWR_OFF);
				#ifdef FIX_MUTE_NOISE
				request_wm8804_set_pll();
				#endif
			}
		}
	}
	mutex_unlock( &audio_power_ctl_mutex);
	trigger_delayed_work();
}

static __devinit int astell_kern_drv_probe(struct platform_device *pdev)
{

	INIT_DELAYED_WORK(&g_audio_power_ctl.work, audio_power_off_ctl_work);
	g_audio_power_ctl.wq = create_workqueue("audio_power_ctl_wq");
	if (g_audio_power_ctl.wq == NULL) {

		return -ENOMEM;
	}
	ak_set_hw_mute(AMT_HP_MUTE, 1);

	trigger_delayed_work();

#if (CUR_AK_REV == AK500N_HW_ES1 || CUR_AK_REV == AK500N_HW_TP)
	init_bar_led_init();
	schedule_work(&bar_led_fade_in_type_work);	
#endif

	#ifdef  SUPPORT_AK_DOCK 
	INIT_DELAYED_WORK(&dock_mute_work, dock_mute_control_work);
	#endif
	#if(CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII)
	printk("MODEL: AK70_MKII\n");
	#else if(CUR_AK == MODEL_AK70) && (CUR_AK_SUB == ORG)
	printk("MODEL: AK70\n");
	#endif
	
	return 0;
}

static __devexit int astell_kern_drv_shutdown(struct platform_device *pdev)
{
	CPRINT("Audio PowerDown ");

	ak_set_hw_mute(AMT_HP_MUTE, 1);

	ak_set_snd_pwr_ctl(AK_AUDIO_POWER, AK_PWR_OFF);

	ak_set_internal_play_mode(INTERNAL_NONE_PLAY); //xmos power off

	return 0;
}


static __devexit int astell_kern_drv_remove(struct platform_device *pdev)
{
	destroy_workqueue(g_audio_power_ctl.wq);

	return 0;
}


#ifdef CONFIG_PM

extern int get_pcfi_suspend_enable(void);

static int astell_kern_suspend(struct i2c_client *client,
                               pm_message_t state)
{
	cancel_delayed_work(&g_audio_power_ctl.work);

	if (!get_pcfi_suspend_enable()) {
		ak_set_hw_mute(AMT_HP_MUTE, 1); //tonny

		set_audio_power_ctl_status(0);

#if (CUR_DAC==CODEC_CS4398)
		cs4398_volume_mute();
#endif

		mdelay(10);

		ak_set_snd_pwr_ctl(AK_AUDIO_POWER, AK_PWR_OFF);

		ak_set_audio_path(ADP_NONE);

		ak_set_internal_play_mode(INTERNAL_NONE_PLAY); //xmos power off

		ak_set_audio_sw_mute(1);  // sleep current issue v1.01 15mA  v.1.02 20mA 이슈 , 반드시 마지막에 여기에서 호출해야함.

#ifdef FIX_MUTE_NOISE
		request_wm8804_set_pll();
#endif

	}

	CPRINT("astell_kern_suspend\n");

	return 0;
}

static int astell_kern_resume(struct i2c_client *client)
{

	set_audio_power_ctl_status(0);
	trigger_delayed_work();

	return 0;
}
#endif


static ssize_t ak_audio_ctl_show(struct device *dev,struct device_attribute *attr, char *buf)
{

	int count = 0;
	u8 data;

	//	count += sprintf(buf,"%d",g_ak_audio_ctl);

	return count;
}



static ssize_t ak_audio_ctl_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t size)
{
	int count=0;
	int onoff;

	if(size < 2) return -EINVAL;

	count = size;

	//	DBG_PRINT("d","%s : buf:%s size:%d\n",attr->attr.name,buf,size);

#ifdef REDUCE_POP_4th
	ak_pcm_stop_pre();
#endif

	if(!strncmp(buf,"audio_stop_prev",size-1)) {

	} else {

	}

	return count ;
}

static DEVICE_ATTR(ak_audio_ctl, 0777, ak_audio_ctl_show, ak_audio_ctl_store);


#if (CUR_AK_REV == AK500N_HW_ES1)
int get_ssd_attach(void)
{
	int iattachd_ssd = 0;

	gpio_request(GPIO_SSD0, "GPIO_SSD0" );
	gpio_request(GPIO_SSD1, "GPIO_SSD1" );
	gpio_request(GPIO_SSD2, "GPIO_SSD2" );

	tcc_gpio_config(GPIO_SSD0, GPIO_FN(0));
	tcc_gpio_config(GPIO_SSD1, GPIO_FN(0));
	tcc_gpio_config(GPIO_SSD2, GPIO_FN(0));

	gpio_direction_input(GPIO_SSD0);
	gpio_direction_input(GPIO_SSD1);
	gpio_direction_input(GPIO_SSD2);

	iattachd_ssd = !gpio_get_value(GPIO_SSD0) + (!gpio_get_value(GPIO_SSD1)) * 2 + (!gpio_get_value(GPIO_SSD2)) * 4;

	CPRINT("attached SSD num %d\n",iattachd_ssd);

	return iattachd_ssd;
}
#elif(CUR_AK_REV == AK500N_HW_TP)
int get_ssd_attach(void)
{
	int iattachd_ssd = 0;

	gpio_request(GPIO_SSD0, "GPIO_SSD0" );
	gpio_request(GPIO_SSD1, "GPIO_SSD1" );
	gpio_request(GPIO_SSD2, "GPIO_SSD2" );
	gpio_request(GPIO_SSD3, "GPIO_SSD3" );

	tcc_gpio_config(GPIO_SSD0, GPIO_FN(0));
	tcc_gpio_config(GPIO_SSD1, GPIO_FN(0));
	tcc_gpio_config(GPIO_SSD2, GPIO_FN(0));
	tcc_gpio_config(GPIO_SSD3, GPIO_FN(0));

	gpio_direction_input(GPIO_SSD0);
	gpio_direction_input(GPIO_SSD1);
	gpio_direction_input(GPIO_SSD2);
	gpio_direction_input(GPIO_SSD3);

	iattachd_ssd = !gpio_get_value(GPIO_SSD0) + (!gpio_get_value(GPIO_SSD1)) * 2 + (!gpio_get_value(GPIO_SSD2)) * 4 + (!gpio_get_value(GPIO_SSD3)) * 8;

	//CPRINT("attached SSD devices #%d\n",iattachd_ssd);

	return iattachd_ssd;
}
#endif

#if (CUR_AK_REV == AK500N_HW_ES1 || CUR_AK_REV == AK500N_HW_TP)
void sata_host_5v_enable(int enable)
{

	gpio_request(GPIO_SATA_HOST_5V_EN, "GPIO_SATA_HOST_5V_EN");
	tcc_gpio_config(GPIO_SATA_HOST_5V_EN, GPIO_FN(0));

	gpio_direction_output(GPIO_SATA_HOST_5V_EN,enable);

	CPRINT("sata_host_5v %d\n",enable);
}


static ssize_t sata_host_5v_enable_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t size)
{
	int count=0;
	int onoff;

	if(size < 2) return -EINVAL;

	count = size;

	if(!strncmp(buf,"on",size-1)) {
		sata_host_5v_enable(1);
	} else {
		sata_host_5v_enable(0);
	}

	return count ;
}



static ssize_t ak_ssd_num_show(struct device *dev,struct device_attribute *attr, char *buf)
{

	int count = 0;
	u8 data;

	count += sprintf(buf,"%d",get_ssd_attach());

	return count;
}

#if (CUR_AK_REV == AK500N_HW_ES1)
int get_raid_array_type(void)
{
	int raid_type;

	gpio_request(GPIO_SSD3, "GPIO_SSD3" );

	tcc_gpio_config(GPIO_SSD3, GPIO_FN(0));

	gpio_direction_input(GPIO_SSD3);

	if(gpio_get_value(GPIO_SSD3)==0) {
		raid_type = 5;
	} else {
		raid_type = 0;
	}

	CPRINT("RAID type : raid%d\n",raid_type);
	return raid_type;
}
#elif(CUR_AK_REV == AK500N_HW_TP)
int get_raid_array_type(void)
{
	int raid_type;

	gpio_request(GPIO_RAID0, "GPIO_RAID0" );

	tcc_gpio_config(GPIO_RAID0, GPIO_FN(0));

	gpio_direction_input(GPIO_RAID0);

	if(gpio_get_value(GPIO_RAID0)==0) {
		raid_type = 5;
	} else {
		raid_type = 0;
	}

	//CPRINT("RAID type : raid%d\n",raid_type);
	return raid_type;
}
#endif


static ssize_t ak_raid_array_type_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	int count = 0;
	u8 data;

	count += sprintf(buf,"raid%d",get_raid_array_type());

	return count;
}

static ssize_t bar_led_control_show(struct device *dev,struct device_attribute *attr, char *buf)
{

	int count = 0;
	u8 data;

	printk("%s\n",__FUNCTION__);
	
	return count;
}

static ssize_t bar_led_control_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t size)
{
	int count=0;
	int control;
	
	//printk("%s\n",__FUNCTION__);
	
	if(size < 2) return -EINVAL;

	count = size;

	sscanf(buf,"%d\n",&control);	

	bar_led_control(control);
		
	//printk("bar led  control:%d\n",control);
	
	return count ;
}

static ssize_t bar_led_brightness_show(struct device *dev,struct device_attribute *attr, char *buf)
{

	int count = 0;
	u8 data;

	printk("%s\n",__FUNCTION__);

	return count;
}

static ssize_t bar_led_brightness_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t size)
{
	int count=0;
	int level;
	
	//printk("%s\n",__FUNCTION__);
	
	if(size < 2) return -EINVAL;

	count = size;

	sscanf(buf,"%d\n",&level);	

	
	set_bar_led_brightness(level);
	
	printk("bar led  brightness:%d\n",level);
	
	return count ;
}

static ssize_t bar_led_fade_inout_show(struct device *dev,struct device_attribute *attr, char *buf)
{

	int count = 0;
	u8 data;

	printk("%s\n",__FUNCTION__);

	return count;
}

static ssize_t bar_led_fade_inout_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t size)
{
	int count=0;
	int value;
	
	//printk("%s\n",__FUNCTION__);
	
	if(size < 2) return -EINVAL;

	count = size;

	sscanf(buf,"%d\n",&value);	

	//printk("bar_led_fade_inout_store:%d\n",value);

	bar_white_yellow_led_on();

	if(value == 0){ 
		schedule_work(&bar_led_fade_out_type_work);
		
	}

	 if(value == 1) {
		schedule_work(&bar_led_fade_in_type_work);
		
	 }


	
	return count ;
}


static ssize_t cdrom_power_enable_show(struct device *dev,struct device_attribute *attr, char *buf)
{

	int count = 0;
	u8 data;

	printk("%s\n",__FUNCTION__);

	return count;
}

static ssize_t cdrom_power_enable_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t size)
{
	int count=0;
	int value;
	
	//printk("%s\n",__FUNCTION__);
	
	if(size < 2) return -EINVAL;

	count = size;

	sscanf(buf,"%d\n",&value);	

	printk("%s:%d\n",__FUNCTION__,value);


	if(value == 0){ 
			gpio_set_value(GPIO_CD_HOST_5V_EN, 0);	
	}

	 if(value == 1) {
			gpio_set_value(GPIO_CD_HOST_5V_EN, 1);	
	 }


	
	return count ;
}


static DEVICE_ATTR(ak_raid_array_type, 0777, ak_raid_array_type_show, NULL);
/*
cat /sys/devices/platform/astell_kern_drv/ak_raid_array_type
*/

static DEVICE_ATTR(ak_ssd_num, 0777, ak_ssd_num_show, NULL);

/*
cat /sys/devices/platform/astell_kern_drv/ak_ssd_num
*/

static DEVICE_ATTR(sata_host_5v_enable, 0777, NULL, sata_host_5v_enable_store);

/*
echo "on" > /sys/devices/platform/astell_kern_drv/sata_host_5v_enable
*/

static DEVICE_ATTR(bar_led_control, 0777, bar_led_control_show, bar_led_control_store);  //iriver-ysyim
/*
echo "0" > /sys/devices/platform/astell_kern_drv/bar_led_control    //bar led off
echo "1" > /sys/devices/platform/astell_kern_drv/bar_led_control   //bar white led on
echo "2" > /sys/devices/platform/astell_kern_drv/bar_led_control   //bar yellow led on
echo "3" > /sys/devices/platform/astell_kern_drv/bar_led_control   //bar white+yellow led on
*/

static DEVICE_ATTR(bar_led_brightness, 0777, bar_led_brightness_show, bar_led_brightness_store); //iriver-ysyim
/*
echo "1" > /sys/devices/platform/astell_kern_drv/bar_led_brightness  //mix brightness
~
echo "11" > /sys/devices/platform/astell_kern_drv/bar_led_brightness //max brightness
*/

static DEVICE_ATTR(bar_led_fade_inout, 0777, bar_led_fade_inout_show, bar_led_fade_inout_store); //iriver-ysyim
/*
echo "0" > /sys/devices/platform/astell_kern_drv/bar_led_fade_inout  //bar led fade out
echo "1" > /sys/devices/platform/astell_kern_drv/bar_led_fade_inout //bar led fade in
*/

static DEVICE_ATTR(cdrom_power_enable, 0777, cdrom_power_enable_show, cdrom_power_enable_store); //iriver-ysyim
/*
echo "0" > /sys/devices/platform/astell_kern_drv/cdrom_power_enable  //CD_HOST_5V_EN Low
echo "1" > /sys/devices/platform/astell_kern_drv/cdrom_power_enable //CD_HOST_5V_EN High 
*/
#endif
int is_amp_led_enable(void)
{
	//printk("is_amp_led_enable:%d\n",g_amp_led_status);
	return g_amp_led_status;
}
EXPORT_SYMBOL(is_amp_led_enable);

//echo "0" > /sys/devices/platform/astell_kern_drv/amp_led_control    //amp led off
//echo "1" > /sys/devices/platform/astell_kern_drv/amp_led_control   //amp led blue
//echo "2" > /sys/devices/platform/astell_kern_drv/amp_led_control   //amp led red
//echo "3" > /sys/devices/platform/astell_kern_drv/amp_led_control   //amp led blue test
//echo "4" > /sys/devices/platform/astell_kern_drv/amp_led_control   //amp led red test
//echo "5" > /sys/devices/platform/astell_kern_drv/amp_led_control   //amp led off test
static ssize_t amp_led_control_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t size)
{
	int count=0;
	int control;
	
	if(size < 2) return -EINVAL;

	count = size;

	sscanf(buf,"%d\n",&control);	

	g_amp_led_status = control;

	printk("%s: %d\n",__FUNCTION__,g_amp_led_status);

	#ifdef SUPPORT_AK_DOCK
	ak_dock_led_control(g_amp_led_status);
	#endif

	if(g_amp_led_status==1){		
		gpio_direction_output(GPIO_HP_BAL_MUTE,1);	
printk("\033[01;33m[HP_BAL_MUTE(%d)] \033[00m\n",1);				
//mdelay(2000);

//		gpio_direction_output(GPIO_HP_MUTE,1);
//printk("\033[01;33m[HP_MUTE(%d)] \033[00m\n",1);						
//mdelay(2000);	
	}

	if(g_amp_led_status==0){
		gpio_direction_output(GPIO_HP_BAL_MUTE,0);
printk("\033[01;33m[HP_BAL_MUTE(%d)] \033[00m\n",0);				
//mdelay(2000);

//		gpio_direction_output(GPIO_HP_MUTE,0);
//printk("\033[01;33m[HP_MUTE(%d)] \033[00m\n",0);						
//mdelay(2000);
	}

#if 0
	if(g_amp_led_status==1){

		ak_set_hw_mute(AMT_HP_MUTE, 1);
printk("\033[01;33m[AUDIO_SW_MUTE,HP_MUTE(%d)] \033[00m\n", 1);
mdelay(1000);

		gpio_direction_output(GPIO_5V5_EN, 1);
printk("\033[01;33m[AUDIO_5V5_EN(%d)] \033[00m\n", 1);
mdelay(1000);

		gpio_direction_output(GPIO_DAC_EN, 1);
printk("\033[01;33m[DAC_EN(%d)] \033[00m\n", 1);
mdelay(1000);		

		gpio_direction_output(GPIO_DAC_RST, 1);
printk("\033[01;33m[DAC_RST(%d)] \033[00m\n", 1);
mdelay(1000);

		gpio_direction_output(GPIO_OPAMP_PWR_EN, 1);
printk("\033[01;33m[OPAMP_PWR_EN(%d)] \033[00m\n", 1);
mdelay(1000);
	}

	if(g_amp_led_status==0){

		ak_set_hw_mute(AMT_HP_MUTE, 1);
printk("\033[01;33m[AUDIO_SW_MUTE,HP_MUTE(%d)] \033[00m\n", 1);
mdelay(1000);

		gpio_direction_output(GPIO_DAC_RST, 0);
printk("\033[01;33m[DAC_RST(%d)] \033[00m\n", 0);
mdelay(1000);		

		gpio_direction_output(GPIO_DAC_EN, 0);
printk("\033[01;33m[DAC_EN(%d)] \033[00m\n", 0);
mdelay(1000);		

		gpio_direction_output(GPIO_5V5_EN, 0);
printk("\033[01;33m[AUDIO_5V5_EN(%d)] \033[00m\n", 0);
mdelay(1000);		

		gpio_direction_output(GPIO_OPAMP_PWR_EN, 0);
printk("\033[01;33m[OPAMP_PWR_EN(%d)] \033[00m\n", 0);
mdelay(1000);		

	}


	
#endif
	
	return count ;
}
static DEVICE_ATTR(amp_led_control, 0777, NULL, amp_led_control_store);  //iriver-ysyim
//echo "0" > /sys/devices/platform/astell_kern_drv/amp_led_control    //amp led off
//echo "1" > /sys/devices/platform/astell_kern_drv/amp_led_control   //amp led on


static ssize_t charge_det_control_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t size)
{
	int count=0;
	int control;
	
	if(size < 2) return -EINVAL;

	count = size;

	sscanf(buf,"%d\n",&control);	

	g_force_charge_det_ctl = control;

	printk("%s: %d\n",__FUNCTION__,g_force_charge_det_ctl);
	
	return count;
}
static DEVICE_ATTR(charge_det_control, 0777, NULL, charge_det_control_store);  //iriver-ysyim
//echo "0" > /sys/devices/platform/astell_kern_drv/charge_det_control    //Real charge detect 
//echo "1" > /sys/devices/platform/astell_kern_drv/charge_det_control   //Force charge detect


static ssize_t stradeum_flag_show(struct device *dev,struct device_attribute *attr, char *buf)
{

	int count = 0;
	u8 data;

	count = g_stradeum_flag;

	printk("%s:%d\n",__FUNCTION__,count);	

	return count;
}

static ssize_t stradeum_flag_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t size)
{
	int count=0;
	int control;
	
	if(size < 2) return -EINVAL;

	count = size;

	sscanf(buf,"%d\n",&control);	

	g_stradeum_flag = control;

	printk("%s: %d\n",__FUNCTION__,g_stradeum_flag);
	
	return count;
}

static DEVICE_ATTR(stradeum_flag, 0777, stradeum_flag_show, stradeum_flag_store);  //iriver-ysyim
//echo "0" > /sys/devices/platform/astell_kern_drv/stradeum_flag    // enable stradeum flag
//echo "1" > /sys/devices/platform/astell_kern_drv/stradeum_flag   //disabel stradeum flag


int g_sleep_time = 180; //default 180sec
int get_sleep_time(void)
{
	return g_sleep_time;
}
EXPORT_SYMBOL(get_sleep_time);

void set_sleep_time(int val)
{
	g_sleep_time = val;
}

static ssize_t sleep_time_show(struct device *dev,struct device_attribute *attr, char *buf)
{

	int count = 0;
	u8 data;

	printk("%d\n",get_sleep_time());

	return count;
}

static ssize_t sleep_time_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t size)
{
	int count=0;
	int time;
	
	printk("%s\n",__FUNCTION__);
	
	if(size < 2) return -EINVAL;

	count = size;

	sscanf(buf,"%d\n",&time);	

	
	set_sleep_time(time);
	
	printk("set_sleep_time:%d\n",time);
	
	return count ;
}

static DEVICE_ATTR(sleep_time, 0777, sleep_time_show, sleep_time_store);  //iriver-ysyim
/*
echo "10" > /sys/devices/platform/astell_kern_drv/sleep_time    //go to sleep after 30sec
*/

extern int get_lcd_state(void);
extern void ak_set_lcd_off_wakelock(void);

static ssize_t do_wake_lock(struct device *dev,struct device_attribute *attr,const char *buf, size_t size)
{
#ifdef USE_LCD_OFF_WAKELOCK
    if (!get_lcd_state()) 
        ak_set_lcd_off_wakelock();
#endif
}

static DEVICE_ATTR(wake_lock, 0222, NULL, do_wake_lock);

static struct platform_driver astell_kern_driver = {
	.driver = {
		.name = "astell_kern_drv",
		.owner = THIS_MODULE,
	},
	.probe = astell_kern_drv_probe,
	.remove = __devexit_p(astell_kern_drv_remove),
	.shutdown = astell_kern_drv_shutdown,
#ifdef CONFIG_PM
	.suspend	= astell_kern_suspend,
	.resume 	= astell_kern_resume,
#endif

};

static struct platform_device *astell_kern_dev;

int __init astell_kern_board_init(void)
{
	int ret;

	astell_kern_dev = platform_device_alloc("astell_kern_drv", -1);
	if (!astell_kern_dev)
		return -ENOMEM;

	ret = platform_device_add(astell_kern_dev);
	if (ret != 0) {
		platform_device_put(astell_kern_dev);
		return ret;
	}

	ret = platform_driver_register(&astell_kern_driver);
	if (ret != 0)
		platform_device_unregister(astell_kern_dev);

	ret = sysfs_create_file(&(astell_kern_dev->dev.kobj),&dev_attr_ak_audio_ctl.attr);

	if (ret) {
		printk(KERN_ERR
		       "Unable to register sysdev entry for ak_audio_ctl.");
	}

#if (CUR_AK_REV == AK500N_HW_ES1 || CUR_AK_REV == AK500N_HW_TP)
	ret = sysfs_create_file(&(astell_kern_dev->dev.kobj),&dev_attr_ak_raid_array_type.attr);
	if (ret) {
		printk(KERN_ERR
		       "Unable to register sysdev entry for ak_raid_array_type.");
	}

	ret = sysfs_create_file(&(astell_kern_dev->dev.kobj),&dev_attr_ak_ssd_num.attr);
	if (ret) {
		printk(KERN_ERR
		       "Unable to register sysdev entry for ak_ssd_num.");
	}

	ret = sysfs_create_file(&(astell_kern_dev->dev.kobj),&dev_attr_sata_host_5v_enable.attr);
	if (ret) {
		printk(KERN_ERR
		       "Unable to register sysdev entry for sata_host_5v_enable.");
	}
	ret = sysfs_create_file(&(astell_kern_dev->dev.kobj),&dev_attr_bar_led_control.attr); //iriver-ysyim
	if (ret) {
		printk(KERN_ERR
		       "Unable to register sysdev entry for bar_led_control..");
	}
	ret = sysfs_create_file(&(astell_kern_dev->dev.kobj),&dev_attr_bar_led_brightness.attr);//iriver-ysyim
	if (ret) {
		printk(KERN_ERR
		       "Unable to register sysdev entry for bar_led_brightness...");
	}

	ret = sysfs_create_file(&(astell_kern_dev->dev.kobj),&dev_attr_bar_led_fade_inout.attr);//iriver-ysyim
	if (ret) {
		printk(KERN_ERR
		       "Unable to register sysdev entry for bar_led_fade_inout...");
	}

	ret = sysfs_create_file(&(astell_kern_dev->dev.kobj),&dev_attr_cdrom_power_enable.attr);//iriver-ysyim
	if (ret) {
		printk(KERN_ERR
		       "Unable to register sysdev entry for cdrom_power_enable...");
	}
#endif

	ret = sysfs_create_file(&(astell_kern_dev->dev.kobj),&dev_attr_amp_led_control.attr); //iriver-ysyim
	if (ret) {
		printk(KERN_ERR
		       "Unable to register sysdev entry for amp led..");
	}

	ret = sysfs_create_file(&(astell_kern_dev->dev.kobj),&dev_attr_charge_det_control.attr); //iriver-ysyim
	if (ret) {
		printk(KERN_ERR
		       "Unable to register sysdev entry for charge det..");
	}

	ret = sysfs_create_file(&(astell_kern_dev->dev.kobj),&dev_attr_stradeum_flag.attr); //iriver-ysyim
	if (ret) {
		printk(KERN_ERR
		       "Unable to register sysdev entry for stradeum flag..");
	}

	ret = sysfs_create_file(&(astell_kern_dev->dev.kobj),&dev_attr_sleep_time.attr);//iriver-ysyim
	if (ret) {
		printk(KERN_ERR
		       "Unable to register sysdev entry for sleep_time...");
	}
	
	ret = sysfs_create_file(&(astell_kern_dev->dev.kobj),&dev_attr_wake_lock.attr);
	if (ret) {
		printk(KERN_ERR
		       "Unable to register sysdev entry for wake_lock...");
	}

	INIT_DELAYED_WORK(&g_pcm_open_work, pcm_open_work);
	INIT_DELAYED_WORK(&g_pcm_off_work, pcm_off_work);

#if (CUR_AK == MODEL_AK500N)
#if (CUR_AK_REV == AK500N_HW_ES1) || (CUR_AK_REV == AK500N_HW_TP)
	INIT_DELAYED_WORK(&ac_work, ac_connect_work);
#endif
#endif

#if (CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
	tcc_gpio_config(GPIO_DSP_RESET, GPIO_FN(0) | GPIO_OUTPUT);
	tcc_gpio_config(GPIO_DSP_POWER, GPIO_FN(0) | GPIO_OUTPUT);
#endif

	//2015.06.29 tonny
	sema_init(&pwr_ctl_sem, 1);

	return ret;
}

#ifdef SUPPORT_DSP_CS48L1X
void tcc8930_audio_dsp_gpio(char *cmd, int value) {
	if (!strcmp(cmd, "power")) {
		printk("audio dsp power %d\n", value);

		if (value == 1) {
			gpio_set_value(GPIO_DSP_POWER, 0);
			mdelay(1);
			gpio_set_value(GPIO_DSP_POWER, 1);
		} else if (value == 0) {
			gpio_set_value(GPIO_DSP_POWER, 0);
		}
	} else if (!strcmp(cmd, "reset")) {
		printk("audio dsp reset\n");

		tcc_gpio_config(GPIO_DSP_INT_HS1, GPIO_FN(0) | GPIO_OUTPUT);
		tcc_gpio_config(GPIO_DSP_BUSY_HS0, GPIO_FN(0) | GPIO_OUTPUT);
	
		gpio_set_value(GPIO_DSP_RESET, 0);

		mdelay(1);
			
		gpio_set_value(GPIO_DSP_BUSY_HS0, 1);
		gpio_set_value(GPIO_DSP_INT_HS1, 1);

		mdelay(1);
		
		gpio_set_value(GPIO_DSP_RESET, 1);
		
		mdelay(1);

		tcc_gpio_config(GPIO_DSP_INT_HS1, GPIO_FN(0) | GPIO_INPUT);
		tcc_gpio_config(GPIO_DSP_BUSY_HS0, GPIO_FN(0) | GPIO_INPUT);
	} else if (!strcmp(cmd, "mode")) {
		printk("audio dsp mode %d\n", value);
		
	}
}
#endif

static int tcc8930_gpio_get_number(char *group, int number)
{
	char grp = group[0];
	int grp_val = ((int)grp) - 'a';

	/* this function handles only GPIO group from A to G */
	if (grp_val > 6)
		return -1;

	/* reference is a arch/arm/mach-tcc893x/include/mach/gpio.h */
	return ((grp_val << GPIO_REG_SHIFT) | number);
}

void tcc8930_gpio_set_val(char*group, int num, int on)
{
	int port = tcc8930_gpio_get_number(group, num);

	if (port < 0) {
		printk("cannot be handled: group %s\n", group);
		return;
	}

	printk("group: %s pin: %d on: %d\n", group, num, on);
	gpio_set_value(port, on);
}

int tcc8930_gpio_get_val(char*group, int num)
{
	int port = tcc8930_gpio_get_number(group, num);

	if (port < 0) {
		printk("cannot be handled: group %s\n", group);
		return;
	}

	printk("group: %s pin: %d\n", group, num);
	return gpio_get_value(port);
}

void tcc8930_gpio_set_conf(char*group, int num, int io, int func)
{
	int port = tcc8930_gpio_get_number(group, num);

	if (port < 0) {
		printk("cannot be handled: group %s\n", group);
		return;
	}

	printk("group: %s pin: %d io: %d func: %d\n", group, num, io, func);
	tcc_gpio_config(port, GPIO_FN(func) | (io ? GPIO_OUTPUT : GPIO_INPUT));
}
void __exit astell_kern_board_exit(void)
{
	platform_device_unregister(astell_kern_dev);
	platform_driver_unregister(&astell_kern_driver);
}


device_initcall(astell_kern_board_init);


//////////////////////////
// Wifi platform device //
//////////////////////////
void tcc8930_set_card_detected(int on);

#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM

#define WLAN_STATIC_SCAN_BUF0		5
#define WLAN_STATIC_SCAN_BUF1		6
#define PREALLOC_WLAN_SEC_NUM		4
#define PREALLOC_WLAN_BUF_NUM		160
#define PREALLOC_WLAN_SECTION_HEADER	24

#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_BUF_NUM * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_BUF_NUM * 1024)

#define DHD_SKB_HDRSIZE			336
#define DHD_SKB_1PAGE_BUFSIZE	((PAGE_SIZE*1)-DHD_SKB_HDRSIZE)
#define DHD_SKB_2PAGE_BUFSIZE	((PAGE_SIZE*2)-DHD_SKB_HDRSIZE)
#define DHD_SKB_4PAGE_BUFSIZE	((PAGE_SIZE*4)-DHD_SKB_HDRSIZE)

#define WLAN_SKB_BUF_NUM	17

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

struct wlan_mem_prealloc {
	void *mem_ptr;
	unsigned long size;
};

static struct wlan_mem_prealloc wlan_mem_array[PREALLOC_WLAN_SEC_NUM] = {
	{NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER)}
};

void *wlan_static_scan_buf0;
void *wlan_static_scan_buf1;
static void *brcm_wlan_mem_prealloc(int section, unsigned long size)
{
	if (section == PREALLOC_WLAN_SEC_NUM)
		return wlan_static_skb;
	if (section == WLAN_STATIC_SCAN_BUF0)
		return wlan_static_scan_buf0;
	if (section == WLAN_STATIC_SCAN_BUF1)
		return wlan_static_scan_buf1;
	if ((section < 0) || (section > PREALLOC_WLAN_SEC_NUM))
		return NULL;

	if (wlan_mem_array[section].size < size)
		return NULL;

	return wlan_mem_array[section].mem_ptr;
}

static int brcm_init_wlan_mem(void)
{
	int i;
	int j;

	for (i = 0; i < 8; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_1PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}

	for (; i < 16; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_2PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}

	wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_4PAGE_BUFSIZE);
	if (!wlan_static_skb[i])
		goto err_skb_alloc;

	for (i = 0 ; i < PREALLOC_WLAN_SEC_NUM ; i++) {
		wlan_mem_array[i].mem_ptr =
		   kmalloc(wlan_mem_array[i].size, GFP_KERNEL);

		if (!wlan_mem_array[i].mem_ptr)
			goto err_mem_alloc;
	}
	wlan_static_scan_buf0 = kmalloc (65536, GFP_KERNEL);
	if(!wlan_static_scan_buf0)
		goto err_mem_alloc;
	wlan_static_scan_buf1 = kmalloc (65536, GFP_KERNEL);
	if(!wlan_static_scan_buf1)
		goto err_mem_alloc;

	printk("%s: WIFI MEM Allocated\n", __FUNCTION__);
	return 0;

err_mem_alloc:
	pr_err("Failed to mem_alloc for WLAN\n");
	for (j = 0 ; j < i ; j++)
		kfree(wlan_mem_array[j].mem_ptr);

	i = WLAN_SKB_BUF_NUM;

err_skb_alloc:
	pr_err("Failed to skb_alloc for WLAN\n");
	for (j = 0 ; j < i ; j++)
		dev_kfree_skb(wlan_static_skb[j]);

	return -ENOMEM;
}
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */

static int brcm_wlan_power(int onoff)
{
	return 0;
}

static int brcm_wlan_reset(int onoff)
{
	return 0;
}

static int brcm_wlan_set_carddetect(int onoff)
{
	tcc8930_set_card_detected(onoff);
	return 0;
}

extern char *wifi_mac;

static int brcm_wlan_get_mac_addr(unsigned char *buff)
{
	int len = strlen(wifi_mac);
	int nok = 1;

	printk("wifi_mac : %s len : %d\n", wifi_mac, len);

	if(len != 0) {
		int cnt;
		int vals[6];

		cnt = sscanf(wifi_mac, "%02x:%02x:%02x:%02x:%02x:%02x",
		             &vals[0], &vals[1], &vals[2],
		             &vals[3], &vals[4], &vals[5]);

		if(cnt == 6) {
			buff[0] = (char)vals[0];
			buff[1] = (char)vals[1];
			buff[2] = (char)vals[2];
			buff[3] = (char)vals[3];
			buff[4] = (char)vals[4];
			buff[5] = (char)vals[5];

			nok = 0;
			printk("parsing wifi mac is ok.\n");
		} else {
			printk("parsing wifi mac is failed.\n");
		}
	}

	return nok;
}

/* Customized Locale table : OPTIONAL feature */
#define WLC_CNTRY_BUF_SZ        4
typedef struct cntry_locales_custom {
	char iso_abbrev[WLC_CNTRY_BUF_SZ];
	char custom_locale[WLC_CNTRY_BUF_SZ];
	int  custom_locale_rev;
} cntry_locales_custom_t;

static cntry_locales_custom_t brcm_wlan_translate_custom_table[] = {
	/* Table should be filled out based on custom platform regulatory requirement */
	{"",   "XZ", 11},  /* Universal if Country code is unknown or empty */
	{"AE", "AE", 1},
	{"AR", "AR", 1},
	{"AT", "AT", 1},
	{"AU", "AU", 2},
	{"BE", "BE", 1},
	{"BG", "BG", 1},
	{"BN", "BN", 1},
	{"CA", "CA", 2},
	{"CH", "CH", 1},
	{"CY", "CY", 1},
	{"CZ", "CZ", 1},
	{"DE", "DE", 3},
	{"DK", "DK", 1},
	{"EE", "EE", 1},
	{"ES", "ES", 1},
	{"FI", "FI", 1},
	{"FR", "FR", 1},
	{"GB", "GB", 1},
	{"GR", "GR", 1},
	{"HR", "HR", 1},
	{"HU", "HU", 1},
	{"IE", "IE", 1},
	{"IS", "IS", 1},
	{"IT", "IT", 1},
	{"JP", "JP", 3},
	{"KR", "KR", 24},
	{"KW", "KW", 1},
	{"LI", "LI", 1},
	{"LT", "LT", 1},
	{"LU", "LU", 1},
	{"LV", "LV", 1},
	{"MA", "MA", 1},
	{"MT", "MT", 1},
	{"MX", "MX", 1},
	{"NL", "NL", 1},
	{"NO", "NO", 1},
	{"PL", "PL", 1},
	{"PT", "PT", 1},
	{"PY", "PY", 1},
	{"RO", "RO", 1},
	{"RU", "RU", 5},
	{"SE", "SE", 1},
	{"SG", "SG", 4},
	{"SI", "SI", 1},
	{"SK", "SK", 1},
	{"TR", "TR", 7},
	{"TW", "TW", 2},
	{"US", "US", 46}
};

static void *brcm_wlan_get_country_code(char *ccode)
{
	int size = ARRAY_SIZE(brcm_wlan_translate_custom_table);
	int i;

	if (!ccode)
		return NULL;

	for (i = 0; i < size; i++)
		if (strcmp(ccode, brcm_wlan_translate_custom_table[i].iso_abbrev) == 0)
			return &brcm_wlan_translate_custom_table[i];
	return &brcm_wlan_translate_custom_table[0];
}

static struct resource brcm_wlan_resources[] = {
	[0] = {
		.name	= "bcmdhd_wlan_irq",
		.start	= INT_EINT2,
		.end	= INT_EINT2,
		.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE,
	},
};

static struct wifi_platform_data brcm_wlan_control = {
	.set_power	= brcm_wlan_power,
	.set_reset	= brcm_wlan_reset,
	.set_carddetect	= brcm_wlan_set_carddetect,
#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	.mem_prealloc	= brcm_wlan_mem_prealloc,
#endif
	.get_country_code = brcm_wlan_get_country_code,
	.get_mac_addr = brcm_wlan_get_mac_addr,
};

static struct platform_device brcm_device_wlan = {
	.name		= "bcmdhd_wlan",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(brcm_wlan_resources),
	.resource	= brcm_wlan_resources,
	.dev		= {
		.platform_data = &brcm_wlan_control,
	},
};

int __init brcm_wlan_init(void)
{
	int ret;

	printk("%s: start\n", __FUNCTION__);

	tcc_gpio_config_ext_intr(INT_EINT2, EXTINT_GPIOE_26);

	tcc_gpio_config(GPIO_WL_HOST_WAKE, GPIO_FN(0)|GPIO_PULL_DISABLE);
	gpio_request(GPIO_WL_HOST_WAKE, "bcmdhd");

	gpio_direction_input(GPIO_WL_HOST_WAKE);

#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	brcm_init_wlan_mem();
#endif

	ret =  platform_device_register(&brcm_device_wlan);
	return ret;
}

device_initcall(brcm_wlan_init);


void	set_cpu_to_dac(int dir)
{
#if(AK_HAVE_XMOS==1)
	gpio_request(GPIO_I2C_SEL, "GPIO_I2C_SEL");
	tcc_gpio_config(GPIO_I2C_SEL,GPIO_FN(0));
	if(dir) {
		//printk("============= cpu to dac connection \n");
		gpio_direction_output(GPIO_I2C_SEL, 0);
	} else {
		//printk("============= xmos to dac connection \n");
		gpio_direction_output(GPIO_I2C_SEL, 1);
	}
#endif
}


static int g_xmos_playing = 0;
void set_xmos_is_playing(int playing)
{
	//printk("set_xmos_is_playing:%d\n",playing);
	g_xmos_playing = playing;
#if 0	
	if(g_xmos_playing)
		schedule_delayed_work(&dock_mute_work, DOCK_MUTE_TIMER);
	else{
		//dock_mute(1); 	//iriver-ysyim
		cancel_delayed_work(&dock_mute_work);
	}
#endif	
}

inline int get_xmos_is_playing(void)
{
	return g_xmos_playing;
}
EXPORT_SYMBOL(get_xmos_is_playing);




static int g_xmos_sample_rate = 0;
void set_xmos_cur_sample_rate(int rate)
{
	//printk("set_xmos_is_playing:%d\n", rate);
	g_xmos_sample_rate = rate;
}

inline int get_xmos_cur_sample_rate(void)
{
	return g_xmos_sample_rate;
}
EXPORT_SYMBOL(get_xmos_cur_sample_rate);



static void dock_mute_control_work(struct work_struct *work)
{
	//printk("dock_mute_control_work:%d\n",g_xmos_playing);

	cancel_delayed_work(&dock_mute_work);

	//dock_mute(0); 	//iriver-ysyim
}

static int g_pcm_playing = 0;
void set_pcm_is_playing(int playing)
{
	//printk("set_pcm_is_playing:%d\n",playing);
	g_pcm_playing = playing;
#if 0	
	if(g_pcm_playing)
		schedule_delayed_work(&dock_mute_work, DOCK_MUTE_TIMER );
	else{

		//dock_mute(1); 	//iriver-ysyim
		cancel_delayed_work(&dock_mute_work);
	}
#endif	
}

int get_pcm_is_playing(void)
{
	return g_pcm_playing;
}
EXPORT_SYMBOL(get_pcm_is_playing);

#if(CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK300)
void set_xmos_pwr_ctrl(int pwr)
{	
	gpio_request(GPIO_XMOS_PWR_EN, "xmos_pwr_en");
	tcc_gpio_config(GPIO_XMOS_PWR_EN, GPIO_FN(0));

	if(pwr)
	{
		gpio_direction_output(GPIO_XMOS_PWR_EN, 1);
	}
	else
	{
		gpio_direction_output(GPIO_XMOS_PWR_EN, 0);
	}
}
#endif
