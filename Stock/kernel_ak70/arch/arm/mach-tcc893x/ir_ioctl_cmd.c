/*
* ir_ioctl-- helper tools for system tuning.
*
* Copyright 2010 iriver
*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include <linux/io.h>
#include <mach/gpio.h>

#include <linux/miscdevice.h>

#include <mach/ir_ioctl.h>
#include <mach/board_astell_kern.h>

#if (CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300 || CUR_AK == MODEL_AK70)
extern CONN_HP_TYPE is_hp_connected(void);
extern void ak_ext_dock_control(int pwr_ctl);
extern int ak_dock_power_state(void); //refer to board_ak_dock.c
extern int ak_dock_check_device(void);
extern int is_amp_on(void);
extern void ak_route_amp_hp_balanced_out(int aot);
extern void amp_unbal_mute(int val);
extern void amp_bal_mute(int val);
extern int detect_amp_bal(void);
extern int get_xmos_is_playing(void);
extern int get_pcm_is_playing(void);
extern void ak_otg_control(int pwr, int mode); /* [downer] A150820 */
extern void ak_set_hw_mute(AUDIO_HW_MUTE_TYPE e_type,int onoff);
extern void dock_mute(int val);
extern int g_stradeum_flag; //스트라디움용 펌웨어 flag 

extern int g_hpjack_connected_ak70mkii;  // for AK70MKII 20170823
extern int g_bal_hp_jack_connected_ak70mkii;   // for AK70MKII 20170823

int g_main_lvol = 0, g_main_rvol = 0, g_amp_gain = 0; /* [downer] A150520 */
#endif
#if (CUR_AK == MODEL_AK500N)
extern CONN_HP_TYPE is_hp_connected(void); //jimi.140723
extern void set_ak500n_volume(int lvol, int rvol); //jimi.140912
#endif

extern void ak_set_internal_play_mode(INTERNAL_PLAY_MODE mode);
//extern void ak_xmos_reboot(void);
extern void set_boot_complete(void); /* [downer] A140109 */
extern void set_dock_event_ready(void);
extern void set_cpu_to_dac(int dir);
static DEFINE_MUTEX(unbalanced_balanced_mutex);
static volatile int g_unbalanced_balanced = 0;  // 0 : unbalanced   1: balanced
static int g_volume_mute = 0;

#if (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII)
static int g_bt_streaming = 0;

int get_bt_streaming_state(void)
{
	return g_bt_streaming;
}
EXPORT_SYMBOL(get_bt_streaming_state);
#endif

// [downer] A150806 
int get_ak_volume_mute(void)
{
    return g_volume_mute;
}
EXPORT_SYMBOL(get_ak_volume_mute);

#if(CUR_AK == MODEL_AK500N)
extern int g_play_path;
#endif

extern int get_current_usb_device(void);

int is_selected_balanced(void)
{
	int ret;
	//	mutex_lock(&unbalanced_balanced_mutex);
	ret  = g_unbalanced_balanced;
	//	mutex_unlock(&unbalanced_balanced_mutex);

	return ret;
}

void ak_set_hw_mute_test(int hpmute,int balmute,int audio_sw_mute)
{
#if (CUR_AK == MODEL_AK100_II ||CUR_AK == MODEL_AK120_II )

#ifdef PRINT_ALSA_MSG
	DBG_ERR("GPIO","HP :%d BAL:%d SW:%d\n",hpmute,balmute,audio_sw_mute);
#endif
	gpio_request(GPIO_HP_MUTE, "GPIO_HP_MUTE");
	tcc_gpio_config(GPIO_HP_MUTE,GPIO_FN(0));

	gpio_request(GPIO_AUDIO_SW_MUTE, "GPIO_AUDIO_SW_MUTE");
	tcc_gpio_config(GPIO_AUDIO_SW_MUTE,GPIO_FN(0));

	gpio_request(GPIO_BAL_MUTE, "GPIO_BAL_MUTE");
	tcc_gpio_config(GPIO_BAL_MUTE,GPIO_FN(0));


	gpio_direction_output(GPIO_HP_MUTE,hpmute);
	gpio_direction_output(GPIO_BAL_MUTE,balmute);
	gpio_direction_output(GPIO_AUDIO_SW_MUTE,audio_sw_mute);
#endif
}


int ir_ioctl_cmd(io_buffer_t *p_io_buffer)
{
	char *p_from_user_buffer = p_io_buffer->io_buffer;
	char *p_to_user_buffer = p_io_buffer->io_buffer;
	int cmd_data_len=0;

	char cmd_str[MAX_SIO_STR] = {0,};
	char cmd_str1[MAX_SIO_STR] = {0,};
	char cmd_str2[MAX_SIO_STR] = {0,};

	sio_read_string(&p_from_user_buffer,cmd_str);  //get 1th argv

	if(!strcmp(cmd_str,"AK_IO"))  {
		sio_read_string(&p_from_user_buffer,cmd_str1);  //get 2th argv

		// DBG_PRINT("IR_IOCTL","%s %s \n",cmd_str,cmd_str1);
		//jimi.140723
		//printk("--%s:%d(ir_cmd[%s])--\n", __func__, __LINE__, cmd_str1);

		if(!strcmp(cmd_str1,"AK_SET_AUDIO_PATH"))  {
			int path;
			path =  sio_read_int(&p_from_user_buffer,0);
            printk("%s -- ak_set_audio_path(%d)\n", __func__);
			ak_set_audio_path(path);
		} else if(!strcmp(cmd_str1,"AK_SET_SND_PWR_CTL"))  {
			int ctl;
			ctl =  sio_read_int(&p_from_user_buffer,0);
			ak_set_snd_pwr_ctl(AK_AUDIO_POWER,ctl);
		} else if(!strcmp(cmd_str1,"INIT_WM8740"))  {
#if(CUR_DAC == CODEC_WM8741)
			init_wm8740();
#endif
		} else if(!strcmp(cmd_str1,"WM8740_SET_VOLUME"))  {
			int lvol,rvol,amp_gain;
			lvol =  sio_read_int(&p_from_user_buffer,0);
			rvol =  sio_read_int(&p_from_user_buffer,0);
			amp_gain =  sio_read_int(&p_from_user_buffer,0);

#if (CUR_AK == MODEL_AK380 || CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK300)
            g_main_lvol = lvol;
            g_main_rvol = rvol;
            g_amp_gain = amp_gain;
#endif
			//printk("lvol:%d rvol:%d amp_gain:%d\n", lvol,rvol,amp_gain);
			//jimi.140723
			//printk("--%s:%d--\n", __func__, __LINE__);
#if(CUR_DAC == CODEC_CS4398)
#if (CUR_AK == MODEL_AK500N)
			set_ak500n_volume(lvol, rvol);
			//printk("--%s:%d(L[%d],R[%d])--\n", __func__, __LINE__, lvol, rvol);
#else
			cs4398_set_volume(1,lvol,rvol);
#endif
#elif(CUR_DAC == CODEC_ES9018)
			// AK1000N volume control 함수 cs3318 is volume IC
			CS3318_set_volume(lvol,rvol);
#elif(CUR_DAC == CODEC_AK4490)
#ifndef AK_AUDIO_MODULE_USED //tonny : To avoid build error
			if((ak_dock_power_state()==DOCK_PWR_ON) && (ak_dock_check_device()==DEVICE_AMP)){ //ysyim amp
				//printk("DOCK_PWR_ON: ak4490_set_volume: %d %d\n",lvol,rvol);
				if((lvol==0) && (rvol == 0)){ 
					g_volume_mute = 1;
					amp_bal_mute(1);  // PCM NEXT시 Volume 0로 되는데 이때 AMP Volume만 0로 하면 노이즈 발생하여 AMP mute걸어줌
					
					#ifdef SUPPORT_EXT_AMP
					amp_unbal_mute(1);		
					#endif	
					msleep_interruptible(50);					
					//ak_set_hw_mute(AMT_HP_MUTE, 1);
				}else{					
					if(g_volume_mute){ 
						ak4490_set_volume(0, MAX_VOLUME_IDX,MAX_VOLUME_IDX);  //DOCK ON이고 AMP인경우에만 DAC Max volume으로 설정 
						if(get_pcm_is_playing() || get_xmos_is_playing()){ // Volume 0후 Volume 높일때 PCM, DSD 재생중일때만 mute를 푼다 , 재생전 먼저 mute풀면 노이즈 발
							#ifdef SUPPORT_EXT_AMP
							amp_bal_mute(0);
							#endif
							
							amp_unbal_mute(0);
							//ak_set_hw_mute(AMT_HP_MUTE, 0);
						}
						g_volume_mute = 0;						
					}
				}	
				CS3318_set_volume(amp_gain, lvol, rvol);
				
			}else
#endif//#ifndef AK_AUDIO_MODULE_USED
			{
//				printk("DOCK_PWR_OFF: ak4490_set_volume: %d %d\n",lvol,rvol);
				g_volume_mute = 0;
				if (lvol == 0 && rvol == 0)
					ak4490_set_volume(0, lvol,rvol);
				else
					ak4490_set_volume(1, lvol,rvol);
			}
			
#else

#endif
		} else if(!strcmp(cmd_str1,"AK_MUTE_TEST"))  {
			int hpmute,balmute,swmute;
			hpmute =  sio_read_int(&p_from_user_buffer,0);
			balmute =  sio_read_int(&p_from_user_buffer,0);
			swmute =  sio_read_int(&p_from_user_buffer,0);

			ak_set_hw_mute_test(hpmute,balmute,swmute);
		}

#if(CUR_DAC == CODEC_WM8741)
		else if(!strcmp(cmd_str1,"WM8740_SET_MUTE"))  {
			int mute;
			mute =  sio_read_int(&p_from_user_buffer,0);
			wm8740_set_mute(mute);
		}
#else
		else if(!strcmp(cmd_str1,"AK_SET_MUTE"))  {
			int mute;
			
			mute =	sio_read_int(&p_from_user_buffer,0);
#if (CUR_AK != MODEL_AK500N) && (CUR_AK != MODEL_AK380) && (CUR_AK != MODEL_AK320) && (CUR_AK != MODEL_AK300)
			if(mute) {
				set_cpu_to_dac(1);
#if(CUR_DAC==CODEC_CS4398)
				cs4398_volume_mute();
#endif

#ifdef AK_AUDIO_MODULE_USED
#if(CUR_DAC==CODEC_AK4490)
				ak_set_hw_mute(AMT_HP_MUTE, mute);
#endif
#endif
				msleep_interruptible(5);
				set_cpu_to_dac(0);
			}
			if(is_selected_balanced()) {
				//ak_set_audio_sw_mute(mute);
			} else {
				//ak_set_hp_mute(mute);
			}
#else
			//printk("---->AK_SET_MUTE\n");
			// ysyim : g_volume_mute 는 AMP DSD NEXT시 volume 0 상태에서 간헐적 mute를 풀어 AMP쪽에서 틱 노이즈 발생 제거 하기 위함
			if((get_current_usb_device() != UCM_PC_FI) && !g_volume_mute){
				printk("AK_SET_MUTE:%d\n",mute);
				
				if(get_xmos_is_playing() == 0){
					if(mute == 0){
						printk("skip\n");
					}else{
						ak_set_hw_mute(AMT_HP_MUTE, mute);
					}					
				}else{
					ak_set_hw_mute(AMT_HP_MUTE, mute);
				}
			}
#endif
		}


#endif
		else if(!strcmp(cmd_str1,"WM8740_OPTICAL_RX"))  {
			int on_off;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if(on_off==1)  {
				ak_set_wm8804_spdif_mode(SND_WM8804_SPDIF_BYPASS);
			} else {
				ak_set_wm8804_spdif_mode(SND_WM8804_PLAY);
			}
		} else if(!strcmp(cmd_str1,"AK_OPTICAL_TX"))  {
			int on_off;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if(on_off==1)  {
				ak_set_wm8804_spdif_mode(SND_WM8804_SPDIF_TX);
			} else  {
				ak_set_wm8804_spdif_mode(SND_WM8804_PLAY);
			}
		} else if(!strcmp(cmd_str1,"AK_BALANCE_OUT"))  {
			int on_off;
			on_off =  sio_read_int(&p_from_user_buffer,0);
			
			#ifdef AK_AUDIO_MODULE_USED
			on_off = 0;//2017.04.14 tonny : forced to HP for AK70 with AK Module
			g_unbalanced_balanced = on_off;
			#endif
			
#ifdef MENU_BALANCED_OUT
			mutex_lock(&unbalanced_balanced_mutex);
			g_unbalanced_balanced = on_off;
			CPRINT("HP/BAL %d\n",on_off);

			#ifdef SUPPORT_EXT_AMP
			if(is_amp_on()){ //ysyim
				printk("ir_ioctl_cmd-is_amp_on:%d\n",on_off);	
				if(on_off==1)  {
					ak_route_amp_hp_balanced_out(AOT_BALANCED);
				} else  {
					if(g_stradeum_flag) //ysyim: cat /sys/devices/platform/astell_kern_drv/stradeum_flag
						ak_route_amp_hp_balanced_out(AOT_BALANCED); //스트라디움용 펌웨어서 동작 
					else
						ak_route_amp_hp_balanced_out(AOT_HP);
				}				
			}else
			#endif
			
			{
				#if (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII)
				CPRINT("g_hpjack_connected_ak70mkii %d\n",g_hpjack_connected_ak70mkii);
				CPRINT("g_bal_hp_jack_connected_ak70mkii %d\n",g_bal_hp_jack_connected_ak70mkii);
				//if ((g_hpjack_connected_ak70mkii != 0) || (g_bal_hp_jack_connected_ak70mkii !=0))
				{
					if(on_off==1)  {
						ak_route_hp_balanced_out(AOT_BALANCED);
					} else  {
						if((g_bal_hp_jack_connected_ak70mkii == 1)&&(g_hpjack_connected_ak70mkii == 0))
							ak_route_hp_balanced_out(AOT_BALANCED);	
						else	 if((g_bal_hp_jack_connected_ak70mkii == 0)&&(g_hpjack_connected_ak70mkii == 0))
							ak_route_hp_balanced_out(AOT_BALANCED);	
						else	
							ak_route_hp_balanced_out(AOT_HP);
					}
				}
				#else
				if(on_off==1)  {
					ak_route_hp_balanced_out(AOT_BALANCED);
				} else  {
					ak_route_hp_balanced_out(AOT_HP);
				}
				#endif
			}
			mutex_unlock(&unbalanced_balanced_mutex);
#endif

		}
		#ifdef _AK_REMOVE_
		else if(!strcmp(cmd_str1,"AK_EXT_DOCK"))  { //2015.02.16-ysyim : frameworks\base\services\jni\com_android_server_input_InputManagerService.cpp 파일 nativeSetExternalDockPower함수에서 호출
			int on_off;
			on_off =  sio_read_int(&p_from_user_buffer,0);
			if(on_off==1){
				printk("AK_EXT_DOCK_ON\n");
				ak_ext_dock_control(EXT_DOCK_ON);
				msleep_interruptible(300);
			}
			else {				
				printk("AK_EXT_DOCK_OFF \n");
				ak_ext_dock_control(EXT_DOCK_OFF);
				msleep_interruptible(300);
			}
		}
		else if(!strcmp(cmd_str1,"AK_DOCK_EVENT_READY"))  { //2015.08.6-ysyim
			printk("AK_DOCK_EVENT_READY\n");
			set_dock_event_ready();
		}
		#endif
		else if(!strcmp(cmd_str1,"AK_EXT_CDRIPPER"))  { /* [downer] A150817 */
			int on_off;
			on_off =  sio_read_int(&p_from_user_buffer,0);
            
			if (on_off) {
				printk("AK_EXT_OTG ON!\n");
				ak_otg_control(EXT_DOCK_ON, OTG_SELFPOWER_DEVICE);
			}
			else {				
				printk("AK_EXT_OTG OFF! \n");
				ak_otg_control(EXT_DOCK_OFF, OTG_SELFPOWER_DEVICE);
			}
		}


		
		else if(!strcmp(cmd_str1,"PCM_CLOSE_PRE"))  {
#ifdef REDUCE_POP_4th

#else
			ak_pcm_stop_pre();
#endif
		} else if(!strcmp(cmd_str1,"XMOS_PWR"))  {
			int on_off;
			on_off =  sio_read_int(&p_from_user_buffer,0);
#if 0
			gpio_request(GPIO_XMOS_PWR_EN, "GPIO_XMOS_PWR_EN");


			tcc_gpio_config(GPIO_XMOS_PWR_EN, GPIO_FN(0));

			if(on_off==1)
				gpio_direction_output(GPIO_XMOS_PWR_EN,1);
			else
				gpio_direction_output(GPIO_XMOS_PWR_EN,0);
#endif
			if(on_off==1) {
				printk("---> Xmos Power on to Play DSD\n");
				ak_set_internal_play_mode(INTERNAL_DSD_PLAY);

			} else if(on_off == 2) {
				printk("---> Xmos Reboot\n");
				//ak_xmos_reboot();
				//ak_set_internal_play_mode(INTERNAL_PCM_PLAY);
				//ak_set_internal_play_mode(INTERNAL_DSD_PLAY);
				ak_set_internal_play_mode(INTERNAL_NONE_PLAY);
			} else {
				printk("---> Xmos Power off to Play PCM\n");
				ak_set_internal_play_mode(INTERNAL_PCM_PLAY);
			}
		} else if(!strcmp(cmd_str1,"AK_AUDIO_POWER"))  {
			int on_off;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if(on_off==1) {
				ak_set_snd_pwr_ctl(AK_AUDIO_POWER, AK_PWR_ON);
			} else {
#if (CUR_DAC==CODEC_CS4398)
				cs4398_volume_mute();
#endif

				mdelay(10);

				ak_set_snd_pwr_ctl(AK_AUDIO_POWER, AK_PWR_OFF);
			}

		}

		else if(!strcmp(cmd_str1,"AK_PRE_POWER_OFF"))  {
			int on_off;

		}

		else if(!strcmp(cmd_str1,"USB_CONN_MODE"))  {
			int usb_conn_mode;
			usb_conn_mode =  sio_read_int(&p_from_user_buffer,0);
			ak_set_usb_conn_mode(usb_conn_mode);
		} else if (!strcmp(cmd_str1, "KEY_LOCK")) {
			extern void ak_set_keylock_mode(int mode);
			int mode;

			mode = sio_read_int(&p_from_user_buffer, 0);

			ak_set_keylock_mode(mode);
		} else if (!strcmp(cmd_str1, "POWERKEY_LOCK")) {
			extern void ak_set_powerkey_lock_mode(int mode);
			int mode;

			mode = sio_read_int(&p_from_user_buffer, 0);

			ak_set_powerkey_lock_mode(mode);
		} else if (!strcmp(cmd_str1, "WHEELKEY_LOCK")) {
			extern void ak_set_wheelkey_lock_mode(int mode);
			int mode;

			mode = sio_read_int(&p_from_user_buffer, 0);

			ak_set_wheelkey_lock_mode(mode);
		} else if(!strcmp(cmd_str1,"AK_MUTE_DELAY"))	{
			extern int g_audio_delay1,g_audio_delay2;
			g_audio_delay1 =	sio_read_int(&p_from_user_buffer,0);
			g_audio_delay2 =	sio_read_int(&p_from_user_buffer,0);
			CPRINT("delay1:%d delay2:%d\n",g_audio_delay1,g_audio_delay2);

		} else if (!strcmp(cmd_str1, "SYSTEM_BOOT_COMPLETE")) { /* [downer] A140109 */
			set_boot_complete();
		}

#if (CUR_AK == MODEL_AK500N)
#if 1 //jimi.0905
		else if(!strcmp(cmd_str1,"HP_35")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("Headphone 35 on \n");
			else
				printk("Headphone 35 off \n");
			
			headphone_35(on_off);
			g_play_path = AUD_PATH_HP_35;			
		} else if(!strcmp(cmd_str1,"HP_65")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("Headphone 65 on \n");
			else
				printk("Headphone 65 off \n");

			headphone_65(on_off);
			g_play_path = AUD_PATH_HP_65;			
		} else if(!strcmp(cmd_str1,"HP_BAL")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("Headphone Balanced on \n");
			else
				printk("Headphone Balanced off \n");

			headphone_bal(on_off);
			g_play_path = AUD_PATH_HP_BAL;
		}
#endif
		else if(!strcmp(cmd_str1,"LOCAL_IN")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("LOCAL_IN on \n");
			else
				printk("LOCAL_IN off \n");

			Local_in(on_off);

		} else if(!strcmp(cmd_str1,"COXIAL_IN")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("COXIAL_IN on \n");
			else
				printk("COXIAL_IN off \n");

			Coxial_In(on_off);
			//g_play_path = AUD_PATH_DIG_IN;
		} else if(!strcmp(cmd_str1,"OPTICAL_IN")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("OPTICAL_IN on \n");
			else
				printk("OPTICAL_IN off \n");

			Optical_In(on_off);
			//g_play_path = AUD_PATH_DIG_IN;
		} else if(!strcmp(cmd_str1,"SPDIF_IN")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("SPDIF_IN on \n");
			else
				printk("SPDIF_IN off \n");

			Spdif_In(on_off);
			//g_play_path = AUD_PATH_DIG_IN;
		} else if(!strcmp(cmd_str1,"SPDIF_OUT")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("SPDIF_OUT on \n");
			else
				printk("SPDIF_OUT off \n");

			Spdif_Out(on_off);
			g_play_path = AUD_PATH_DIG_OUT;
		} else if(!strcmp(cmd_str1,"COXIAL_OUT")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("COXIAL_OUT on \n");
			else
				printk("COXIAL_OUT off \n");

			Coxial_Out(on_off);
			g_play_path = AUD_PATH_DIG_OUT;
		} else if(!strcmp(cmd_str1,"OPTICAL_OUT")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("OPTICAL_OUT on \n");
			else
				printk("OPTICAL_OUT off \n");

			Optical_Out(on_off);
			g_play_path = AUD_PATH_DIG_OUT;
		} else if(!strcmp(cmd_str1,"BNC_IN")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("BNC_IN on \n");
			else
				printk("BNC_IN off \n");

			Bnc_In(on_off);
			//g_play_path = AUD_PATH_DIG_IN;
		} else if(!strcmp(cmd_str1,"BNC_OUT")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("BNC_OUT on \n");
			else
				printk("BNC_OUT off \n");

			Bnc_Out(on_off);
			g_play_path = AUD_PATH_DIG_OUT;
		} else if(!strcmp(cmd_str1,"FIX_UNBAL")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("FIX_UNBAL on \n");
			else
				printk("FIX_UNBAL off \n");

			Fix_Unbal_Out(on_off);
			g_play_path = AUD_PATH_FIXED_UNBAL;

		} else if(!strcmp(cmd_str1,"AUDIO_OUT1")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("AUDIO_OUT1 on \n");
			else
				printk("AUDIO_OUT1 off \n");

			Audio_Out1(on_off);
			g_play_path = AUD_PATH_FIXED_BAL;
		} else if(!strcmp(cmd_str1,"AUDIO_OUT2")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("AUDIO_OUT2 on \n");
			else
				printk("AUDIO_OUT2 off \n");

			Audio_Out2(on_off);
			g_play_path = AUD_PATH_VAR_BAL;
		} else if(!strcmp(cmd_str1,"VAR_UNBAL")) {
			int on_off=0;
			on_off =  sio_read_int(&p_from_user_buffer,0);

			if (on_off == 1)
				printk("VAR_UNBAL on \n");
			else
				printk("VAR_UNBAL off \n");

			Var_Unbal(on_off);
			g_play_path = AUD_PATH_VAR_UNBAL;
		}

#if (CUR_AK_REV == AK500N_HW_ES1) || (CUR_AK_REV == AK500N_HW_TP)
		else if (!strcmp(cmd_str1, "ADAPTER_PLAYING")) { /* [downer] A140826 for Adapter playing mode */
			extern void ak_set_adapter_playing_mode(int mode);
			int mode;

			mode = sio_read_int(&p_from_user_buffer, 0);

			ak_set_adapter_playing_mode(mode);
		}
		else if (!strcmp(cmd_str1, "CDRIPPING_PLAYING")) { //iriver-ysyim
			extern void ak_cdripping_playing_mode(int mode);
			int mode;

			mode = sio_read_int(&p_from_user_buffer, 0);

			ak_cdripping_playing_mode(mode);
		}
#endif
#endif
		#if(CUR_AK == MODEL_AK500N)
		else if(!strcmp(cmd_str1,"RAID_STAT"))  {
			extern int ak_get_mdstat(char **pp_from_user_buffer,char **pp_to_user_buffer);
			cmd_data_len = ak_get_mdstat(&p_from_user_buffer,&p_to_user_buffer);
		}
		#endif
		
		#ifdef SUPPORT_DSP_CS48L1X
		else if(!strcmp(cmd_str1, "AUDIO_DSP")) {
			int value;
			char cmd[MAX_SIO_STR] = {0,};
			extern void tcc8930_audio_dsp_gpio(char *cmd, int value); 
			
			sio_read_string(&p_from_user_buffer, cmd);  //get 3rd argv
			value =  sio_read_int(&p_from_user_buffer, 0);

			tcc8930_audio_dsp_gpio(cmd, value);
		}
		#endif
		else if (!strcmp(cmd_str1, "GPIO_VAL")) {
			extern void tcc8930_gpio_set_val(char*type, int num, int val);
			char type[32] = {0};
			int num;
			int val;

			sio_read_string(&p_from_user_buffer, type);
			num = sio_read_int(&p_from_user_buffer, 0);
			val = sio_read_int(&p_from_user_buffer, 0);

			tcc8930_gpio_set_val(type, num, val);
		}
		else if (!strcmp(cmd_str1, "GPIO_VAL_R")) {
			extern int tcc8930_gpio_get_val(char*group, int num);
			
			char type[32] = {0};
			int num;
			int val = 0;

			sio_read_string(&p_from_user_buffer, type);
			num = sio_read_int(&p_from_user_buffer, 0);
			val = tcc8930_gpio_get_val(type, num);

			cmd_data_len =sio_set_argv(&p_to_user_buffer,
									   SIO_ARGV_INT32,"reg_value",val,
									   SIO_ARGV_END);
			
		}
		
		else if (!strcmp(cmd_str1, "GPIO_CONF")) {
			extern void tcc8930_gpio_set_conf(char*type, int num, int io, int func);
			char type[32] = {0};
			int num, io, func;

			sio_read_string(&p_from_user_buffer, type);
			num = sio_read_int(&p_from_user_buffer, 0);
			io = sio_read_int(&p_from_user_buffer, 0);
			func = sio_read_int(&p_from_user_buffer, 0);

			tcc8930_gpio_set_conf(type, num, io, func);
		}
#if (CUR_AK == MODEL_AK70) && (CUR_AK_SUB == MKII)			
		else if (!strcmp(cmd_str1, "BT_STREAMING")) {
			g_bt_streaming = sio_read_int(&p_from_user_buffer, 0);
			printk("Bluetooth Streaming %s\n", g_bt_streaming ? "Started" : "Stopped");
		}
#endif
		else {
			DBG_PRINT("IR_IOCTL", "ir_ioctl io error !!!! %s \n",cmd_str1);
		}
	} else if(!strcmp(cmd_str,"ITOOLS_IO"))  {
		char cmd_str1[16] = {0,};
		sio_read_string(&p_from_user_buffer,cmd_str1); //get 2th argv

		if(!strcmp(cmd_str1,"CPU_REG_READ"))  {
			unsigned int physical_addr,virtual_addr,virtual_addr2;
			unsigned int reg_addr,reg_value;

			reg_addr =  sio_read_int(&p_from_user_buffer,0);
			physical_addr=reg_addr;

			physical_addr	>>=12;
			physical_addr	<<=12;

			virtual_addr = (unsigned long)ioremap(physical_addr,0x1000);  // 0x1000 로 떨어지게
			if(virtual_addr) {
				virtual_addr2 = virtual_addr + (reg_addr % 0x1000);

				reg_value =  readl(virtual_addr2);

				cmd_data_len =sio_set_argv(&p_to_user_buffer,
				                           SIO_ARGV_INT32,"reg_value",reg_value,
				                           SIO_ARGV_END);

				iounmap((void __iomem *)virtual_addr);
			} else {
				DBG_PRINT("IR_IOCTL", "ir_ioctl io error !!!! %s \n",cmd_str1);
			}
		} else if(!strcmp(cmd_str1,"CPU_REG_WRITE")) {
			unsigned int physical_addr,virtual_addr,virtual_addr2;
			unsigned int write_addr,reg_value;

			write_addr =  sio_read_int(&p_from_user_buffer,0);
			reg_value =  sio_read_int(&p_from_user_buffer,0);

			physical_addr=write_addr;

			physical_addr	>>=12;
			physical_addr	<<=12;

			virtual_addr = (unsigned long)ioremap(physical_addr,0x1000);  // 0x1000 로 떨어지게
			if(virtual_addr) {
				virtual_addr2 = virtual_addr + (write_addr % 0x1000);
				writel(reg_value,virtual_addr2);
				iounmap((void __iomem *)virtual_addr);
			} else {

			}

		} else if(!strcmp(cmd_str1,"WM8804_RESET"))  {

			int reset;
			reset =	sio_read_int(&p_from_user_buffer,0);

			gpio_request(GPIO_OPTIC_RST, "GPIO_OPTIC_RST");
			tcc_gpio_config(GPIO_OPTIC_RST,GPIO_FN(0) | GPIO_PULLUP);
			if(reset) {
				gpio_direction_output(GPIO_OPTIC_RST,1);
			} else {
				gpio_direction_output(GPIO_OPTIC_RST,0);
			}

			DBG_PRINT("IR_IOCTL","WM8804 Reset: %d\n",reset);
		}
		// WM8804 IO
		else if(!strcmp(cmd_str1,"WM8804_READ")) {
			extern int WM8804_READ(unsigned char reg_addr);
			int reg_addr,reg_value;

			reg_addr =  sio_read_int(&p_from_user_buffer,0);
			reg_value=WM8804_READ(reg_addr);
			//				  DBG_PRINT("IR_IOCTL","%x \n",reg_value);
			cmd_data_len =sio_set_argv(&p_to_user_buffer,
			                           SIO_ARGV_INT8,"reg_value",reg_value,
			                           SIO_ARGV_END);
		}

		else if(!strcmp(cmd_str1,"WM8804_WRITE"))  {
			extern int WM8804_WRITE(unsigned int reg, unsigned int val);

			int reg_addr,reg_value;
			reg_addr =	sio_read_int(&p_from_user_buffer,0);
			reg_value =  sio_read_int(&p_from_user_buffer,0);
			WM8804_WRITE(reg_addr,reg_value);

			DBG_PRINT("IR_IOCTL","WRITE:%x VALUE:%d(%x) \n",reg_addr,reg_value,reg_value);
		}

		else if(!strcmp(cmd_str1,"WM8804_RESET"))  {

			int reset;
			reset =	sio_read_int(&p_from_user_buffer,0);

			gpio_request(GPIO_OPTIC_RST, "GPIO_OPTIC_RST");
			tcc_gpio_config(GPIO_OPTIC_RST,GPIO_FN(0) | GPIO_PULLUP);
			if(reset) {
				gpio_direction_output(GPIO_OPTIC_RST,1);
			} else {
				gpio_direction_output(GPIO_OPTIC_RST,0);
			}

			DBG_PRINT("IR_IOCTL","WM8804 Reset: %d\n",reset);
		}
		// WM8741 IO

#if(CUR_DAC == CODEC_WM8741)
		else if(!strcmp(cmd_str1,"WM8741_READ"))	  {
			extern	 int WM8741_READ(unsigned short reg);

			int reg_addr,reg_value;
			reg_addr =  sio_read_int(&p_from_user_buffer,0);
			reg_value =	WM8741_READ(reg_addr);

			//				   DBG_PRINT("IR_IOCTL","WM8741 READ:%x VALUE:%d(%x) \n",reg_addr,reg_value,reg_value);
			cmd_data_len =sio_set_argv(&p_to_user_buffer,
			                           SIO_ARGV_INT32,"reg_value",reg_value,
			                           SIO_ARGV_END);
		} else if(!strcmp(cmd_str1,"WM8741_WRITE"))	 {
			extern int WM8741_WRITE(unsigned short reg, unsigned short value);

			int reg_addr,reg_value;
			reg_addr =	sio_read_int(&p_from_user_buffer,0);
			reg_value =  sio_read_int(&p_from_user_buffer,0);
			WM8741_WRITE(reg_addr,reg_value);

			//				DBG_PRINT("IR_IOCTL","WM8741 WRITE:%x VALUE:%d(%x) \n",reg_addr,reg_value,reg_value);
		}
#endif

#if(CUR_DAC == CODEC_CS4398)
		else if(!strcmp(cmd_str1,"CS4398_RESET"))	  {
			void cs4398_reg_init(void);
			DBG_PRINT("IR_IOCTL","CS4398 Reset \n");

			cs4398_reg_init();
		} else if(!strcmp(cmd_str1,"CS4398_READ"))	  {
			extern	 int CS4398_READ(int ch,unsigned short reg);

			int ch,reg_addr,reg_value;
			ch =  sio_read_int(&p_from_user_buffer,0);
			reg_addr =  sio_read_int(&p_from_user_buffer,0);
			reg_value =	CS4398_READ(ch,reg_addr);

			//DBG_PRINT("IR_IOCTL","CS4398 CH:%d READ:%x VALUE:%d(%x) \n",ch,reg_addr,reg_value,reg_value);
			cmd_data_len =sio_set_argv(&p_to_user_buffer,
			                           SIO_ARGV_INT32,"reg_value",reg_value,
			                           SIO_ARGV_END);
		} else if(!strcmp(cmd_str1,"CS4398_WRITE"))	 {
			extern int CS4398_WRITE(int ch,unsigned short reg, unsigned short value);

			int ch,reg_addr,reg_value;
			ch =  sio_read_int(&p_from_user_buffer,0);
			reg_addr =	sio_read_int(&p_from_user_buffer,0);
			reg_value =  sio_read_int(&p_from_user_buffer,0);
			CS4398_WRITE(ch,reg_addr,reg_value);

			//DBG_PRINT("IR_IOCTL","CS4398 CH:%d WRITE:%x VALUE:%d(%x) \n",ch,reg_addr,reg_value,reg_value);
		}
#endif

#ifdef TEST_DDR_CLOCK
		else if(!strcmp(cmd_str1,"DDR_CLK"))	{

			extern	void set_ddr_clock(int freq);
			int freq;
			freq =  sio_read_int(&p_from_user_buffer,0) * 1000000;

			set_ddr_clock(freq);

			DBG_PRINT("DDR","DDR CLK:%d \n",freq);
		}
#endif

		else {
			DBG_PRINT("IR_IOCTL", "ir_ioctl io error !!!! %s \n",cmd_str1);
		}
	} else {
		DBG_PRINT("IR_IOCTL", "ir_ioctl io error !!!! %s \n",cmd_str);
	}

	return cmd_data_len;
}

