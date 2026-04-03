/*

  astell & kern ak380 dock board

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
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <linux/earlysuspend.h>
#include <linux/switch.h>
#include <mach/tcc_adc.h>
#include <linux/mutex.h>
#include <mach/bsp.h>
#include <mach/gpio.h>
#include <mach/ak_gpio_def.h>
#include <mach/board_astell_kern.h>
#include <mach/board-tcc8930.h>
#include <mach/tca_tco.h>

#ifdef SUPPORT_AK_DOCK

/*refer to frameworks/base/core/java/android/hardware/input/InputManager.java  
private static final int DOCK_STATE_UNDOCKED = 0;
private static final int DOCK_STATE_AMP_DOCKED = 1;					//  AMP 연결
private static final int DOCK_STATE_RECORDER_DOCKED = 2;		//  REC 연결
private static final int DOCK_STATE_AMP_POWERED = 3;
private static final int DOCK_STATE_RECORDER_POWERED = 4;
private static final int DOCK_STATE_POWER_REQUEST = 5;
private static final int DOCK_STATE_CRADLE_DOCKED = 6;			// CRADDLE 연결 
-----------------------------------------------------------------------------------------
refer to frameworks/base/services/jni/com_android_server_input_InputManagerService.cpp

#define DOCK_STATE_UNDOCKED                     0
#define DOCK_STATE_AMP_DOCKED                   1
#define DOCK_STATE_RECORDER_DOCKED          2
#define DOCK_STATE_AMP_POWERED                  3
#define DOCK_STATE_RECORDER_POWERED     4
#define DOCK_STATE_CRADLE_DOCKED                6
#define DOCK_STATE_CRADLE_POWERED           7

static jint nativeGetExternalDockState(JNIEnv* env, jclass clazz) {
   int dock_state, doc_fd;                      
                                                
    dock_state = DOCK_STATE_UNDOCKED;           
                                                
    doc_fd = open("/sys/class/switch/external_dock/state", O_RDONLY);
    if (doc_fd) {                               
        char val;                               
        if (read(doc_fd, &val, 1) == 1 ) {      
            if(val == '0')                      
                dock_state = DOCK_STATE_UNDOCKED;
.......
} 
*/

#define AK380_AMP_TP  // from AMP TP
//#define AMP_SLEEP_WAKUP_ENABLE
#define AK_DOCK_CONNECT_TIMER		msecs_to_jiffies(500)  // 500msc
#define AMP_OUT_DETECT_TIMER     msecs_to_jiffies(500)  // 500msc
#define CRADLE_ERROR_TIMER		msecs_to_jiffies(1000) // 1 sec

/*device on*/
#define DEVICE_AMP_ON			   3
#define DEVICE_AMP_OFF			   1

#define DEVICE_RECORDER_ON    4
#define DEVICE_RECORDER_OFF   2

#define DEVICE_CRADLE_ON		  7  // == DEVICE_AMP_ON
#define DEVICE_CRADLE_OFF	  6



#define AMP_BAT_INIT_ZERO						 (-100)
#define AMP_BAT_WORKING_ZERO			 (-101)

#define REC_BAT_INIT_ZERO						 (-110)
#define REC_BAT_WORKING_ZERO			 (-111)

#define CRADLE_POWER_ERROR				 (-120)


#define LED_OFF					0
#define LED_ON						1
#define LED_LOW_BAT			2
#define LED_BLUE_TEST		3
#define LED_RED_TEST		4
#define LED_OFF_TEST		5


#define POWER_OFF   0
#define POWER_ON    1
#define POWER_SLEEP 2
#define POWER_WAKEUP 3

#define AK_DOCK_LOW_BAT 10

void ak_ext_dock_control(int pwr_ctl);
//J151022+
extern int init_MAX14662(void);
extern int init_reg_PGA2505(void);
extern int init_CS3318_rec(void);
int init_preamplifier(){return 0;}//sc18is602 
//int init_AK5572(){return 0;}
extern int init_AK5572(void);
extern int init_SA9227(void);
//int init_SA9227(){return 0;}
//J151022-
extern int get_boot_complete(void);
extern int start_ext_battery(void);
extern int stop_ext_battery(void);
extern int get_ext_battery_level(void);
extern int ak_is_connected_optical_tx(void);
extern int init_CS3318(void);
extern void ak4490_resume_volume(void);
extern int get_xmos_is_playing(void);
extern int get_pcm_is_playing(void);
extern int check_dock_battery_id(void);
extern void stop_amp_switch(void);
extern void start_amp_switch(void);
extern int is_amp_led_enable(void);
extern void CS3318_volume_mute(int val);
extern void ak_route_amp_hp_balanced_out(int aot);
extern int get_dock_event_ready(void);

extern int fake_ext_audio; //2012.02.10-ysyim: refer to switch_class.c
extern int fake_ext_bat_enable;
extern int g_force_charge_det_ctl; //ysyim-reger to board_asterll_kern.c
extern int g_stradeum_flag; //스트라디움용 펌웨 flag

static struct early_suspend dcok_early_suspend;
static struct delayed_work ak_dock_work;
static struct tcc_adc_client *ad_client;
static struct switch_dev dock_dev;
static struct switch_dev dock_bal_dev;
static struct switch_dev dock_unbal_dev;

static struct delayed_work cradle_error_work;

static struct switch_dev cdripper_dev; /* [downer] A150820 for cdripper event */

/* [downer] A150624 */
struct delayed_work amp_out_detect_wq;
static int g_amp_current_out_state = AMP_NO_OUT;
static int g_amp_before_out_state = AMP_OUT_INIT;

static DEFINE_MUTEX(dock_lock);

static int now_device_id, old_device_id=0;
static int adc_channel=5;
static int dock_power_state=POWER_OFF;
static int glow_battery_led = 0;

static int g_ak_otg_device = 0;	/* [downer] A160125 */

void sys_external_dock_state(int state)
{
	printk("--%s:%d:state(%d)--\n", __func__, __LINE__, state);
	switch_set_state(&dock_dev, state);
}

/* [downer] A150820 for otg device event */
void sys_external_otg_state(int state)
{
    switch_set_state(&cdripper_dev, state);
}
EXPORT_SYMBOL(sys_external_otg_state);

/* [downer] A160125 set external OTG device */
int ak_get_otg_device(void)
{
	return g_ak_otg_device;
}
EXPORT_SYMBOL(ak_get_otg_device);

void ak_set_otg_device(int device)
{
	g_ak_otg_device = device;

	if(device == DEVICE_RECORDER) {
#if (CUR_AK == MODEL_AK320 || CUR_AK == MODEL_AK380)
		ak4490_save_cur_volume();
#elif (CUR_AK == MODEL_AK380S)
		ak4497_save_cur_volume();
#endif
	}
}
EXPORT_SYMBOL(ak_set_otg_device);

//여기서 dock의 ad값을 읽어 AMP/REC/CRADDLE 판단 
int ak_dock_check_device(void)
{
	int id1,id2;
	int ret=0;	
	
	id1= gpio_get_value(GPIO_AMP_DET);
	id2= gpio_get_value(GPIO_AMP_ADP_DET );

	if((id1 == 1) &&(id2 == 1))
		ret = DEVICE_NO;
	else if((id1 == 0) &&(id2 == 1))
		ret = DEVICE_AMP;		
	else if((id1 == 1) &&(id2 == 0))
		ret = DEVICE_RECORDER;	
	else if((id1 == 0) &&(id2 == 0))
		ret = DEVICE_CRADLE;		 
	
	//printk("ak_dock_check_device:%d\n",ret);
	return ret;	
}
EXPORT_SYMBOL(ak_dock_check_device);

int ak_dock_power_state(void)
{
	//printk("ak_dock_power_state:%d\n",dock_power_state);
	return dock_power_state;
}
EXPORT_SYMBOL(ak_dock_power_state);

static void set_ak_dock_power_state(int sta)
{
	printk("set_ak_dock_power_state:%d\n",sta);
	dock_power_state = sta;
}

int is_amp_on(void)
{
	if((ak_dock_power_state()==DOCK_PWR_ON) && (ak_dock_check_device()==DEVICE_AMP))
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(is_amp_on);

int is_cradle_on(void)
{
	if((ak_dock_power_state()==DOCK_PWR_ON) && (ak_dock_check_device()==DEVICE_CRADLE))
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(is_cradle_on);

int is_optical_tx_connect(void)
{
	if(gpio_get_value(GPIO_HP_DET)==1 && (gpio_get_value(GPIO_OPTIC_TX_DET)==0) )
		return 1;
	else 
		return 0;
}

void ak_dock_led_control(int val)
{
	
	if(val == LED_BLUE_TEST){  // 공젙울 에서 RED LED 테스트시.. refer to amp_led_control_store
		gpio_set_value(EXT_GPIO_P13,1); //LED_R
		gpio_set_value(EXT_GPIO_P14,0); //LED_B
	}
	else if(val == LED_RED_TEST){
		gpio_set_value(EXT_GPIO_P13,0); //LED_R
		gpio_set_value(EXT_GPIO_P14,1); //LED_B
	}
	else if(val == LED_OFF_TEST){
		gpio_set_value(EXT_GPIO_P13,1); //LED_R
		gpio_set_value(EXT_GPIO_P14,1); //LED_B
	}
	else {
	
		if(val==LED_OFF){ //off
			//printk("DOCK  LED_OFF\n");
			gpio_set_value(EXT_GPIO_P13,1); //LED_R
			gpio_set_value(EXT_GPIO_P14,1); //LED_B
			
		}else if (val == LED_ON){ //on		
			//echo "0" > /sys/devices/platform/astell_kern_drv/amp_led_control    //amp led off
			//echo "1" > /sys/devices/platform/astell_kern_drv/amp_led_control   //amp led on
			if(is_amp_led_enable()){
				//printk("DOCK  LED_ENABLE\n");
				gpio_set_value(EXT_GPIO_P13,1); //LED_R
				gpio_set_value(EXT_GPIO_P14,0); //LED_B
			}else{
				printk("DOCK  LED_DISABLE\n");
				gpio_set_value(EXT_GPIO_P13,1); //LED_R
				gpio_set_value(EXT_GPIO_P14,1); //LED_B
			}
		}else if (val == LED_LOW_BAT){ //충전 중
			//printk("DOCK  LED_LOW_BAT\n");
			gpio_set_value(EXT_GPIO_P13,0); //LED_R
			gpio_set_value(EXT_GPIO_P14,1); //LED_B			
		}
	}
}
EXPORT_SYMBOL(ak_dock_led_control);

static int check_init_low_battery(void)
{
	if((now_device_id == DEVICE_AMP) || (now_device_id==DEVICE_RECORDER)){
		if((get_ext_battery_level() == 0) && (ak_dock_power_state() == POWER_OFF) ){
			if(now_device_id==DEVICE_AMP){
				sys_external_dock_state(AMP_BAT_INIT_ZERO);  //  (-100)
			}
			else if(now_device_id==DEVICE_RECORDER){
				sys_external_dock_state(REC_BAT_INIT_ZERO); //  (-110)
			}				
			msleep_interruptible(300);
			ak_ext_dock_control(EXT_DOCK_OFF);				
			return 1;	
		}
	}
	return 0;
}

static int check_working_low_battery(void)
{
	if((now_device_id == DEVICE_AMP) || (now_device_id==DEVICE_RECORDER)){
		if((get_ext_battery_level() == 0) && (ak_dock_power_state() == POWER_ON) ){
			if(now_device_id==DEVICE_AMP){
				sys_external_dock_state(AMP_BAT_WORKING_ZERO); //(-101)
			}
			else if(now_device_id==DEVICE_RECORDER){
				sys_external_dock_state(REC_BAT_WORKING_ZERO); // (-111)
			}				
			msleep_interruptible(300);
			ak_ext_dock_control(EXT_DOCK_OFF);
			return 1;
		}
	}
	return 0;
}

static int is_charging(void)
{  
	if(g_force_charge_det_ctl)  //공정툴 진입시 무조건 charge detet신호를  ON해서 USB케이블 연결되어 있지 않아도 Cradle On되도록함 
		return 1;
	else
    	return !gpio_get_value(GPIO_CHG_DET); 
}

static int detect_optical(void)
{
	return !gpio_get_value(GPIO_OPTIC_TX_DET);
}

int detect_amp_unbal(void)
{
	return !(gpio_get_value(EXT_GPIO_P06) >> 6);
}
EXPORT_SYMBOL(detect_amp_unbal);

int detect_amp_bal(void)
{
#ifdef AK380_AMP_TP
	return !(gpio_get_value(EXT_GPIO_P05) >> 5);
#else
	return (gpio_get_value(EXT_GPIO_P05) >> 5);
#endif
}
EXPORT_SYMBOL(detect_amp_bal);

void cradle_mute(int val)
{
	int dock_device = ak_dock_check_device();
	if(is_cradle_on()){
		printk("cradle_mute:%d\n",val);		
		gpio_set_value(GPIO_AMP_SW_MUTE_COM ,val); // 볼륨 MAX + Cradle off + 재생시 Cradle로 잠깐 출력 나오는것 방지
     	gpio_direction_output(GPIO_AMP_ACC_EN, val);  // GPIO_AMP_ACC_EN == CRADLE_XLR_MUTE     
	}
}
EXPORT_SYMBOL(cradle_mute);

void amp_unbal_mute(int val)
{
	int dock_device = ak_dock_check_device();
	if(dock_device == DEVICE_AMP){
		//printk("amp_unbal_mute:%d\n",val);
		#ifdef AK380_AMP_TP
     	gpio_direction_output(EXT_GPIO_P04, val); /* UNBAL_MUTE Active High*/
		#else
		 gpio_direction_output(EXT_GPIO_P04, !val); /* UNBAL_MUTE Active Low*/
		#endif		
	}
}
EXPORT_SYMBOL(amp_unbal_mute);

void amp_bal_mute(int val)
{
	int dock_device = ak_dock_check_device();
	if(dock_device == DEVICE_AMP){
		//printk("amp_bal_mute:%d\n",val);
		#ifdef AK380_AMP_TP
     	gpio_direction_output(EXT_GPIO_P03, val); /* BAL_MUTE Active High*/
     	//gpio_direction_output(EXT_GPIO_P03, 0); /* BAL_MUTE Active High*/
		#else
		gpio_direction_output(EXT_GPIO_P03, !val); /* BAL_MUTE Active Low*/
		#endif		
	}
}
EXPORT_SYMBOL(amp_bal_mute);

void amp_unbal_enable(int val)
{
	printk("amp_unbal_enable:%d\n",val);
	gpio_direction_output(EXT_GPIO_P11, val); /* UNBAL_AMP_EN */
	#ifdef AK380_AMP_TP
	gpio_direction_output(EXT_GPIO_P01, val); /* INPUT_SEL - H: UNB, Low:BAL*/	
	#endif

}
EXPORT_SYMBOL(amp_unbal_enable);

void amp_bal_enable(int val)
{
	printk("amp_bal_enable:%d\n",val);
	gpio_direction_output(EXT_GPIO_P12, val); /* BAL_AMP_EN */
	#ifdef AK380_AMP_TP
	gpio_direction_output(EXT_GPIO_P01, !val); /* INPUT_SEL - H: UNB, Low:BAL*/	
	#endif

}
EXPORT_SYMBOL(amp_bal_enable);

static void display_led(void)
{

	if(is_amp_on() && (is_amp_led_enable() < LED_BLUE_TEST)){
		if(get_ext_battery_level() <= AK_DOCK_LOW_BAT){
			glow_battery_led = 1;
			//printk("%d\n",__LINE__);
			ak_dock_led_control(LED_LOW_BAT);
		}else{
			if((get_ext_battery_level() < (AK_DOCK_LOW_BAT+20)) && is_charging() && (glow_battery_led == 1)){
				//printk("%d\n",__LINE__);
				ak_dock_led_control(LED_LOW_BAT);
			}else if(ak_dock_power_state()){
				glow_battery_led = 0;
				//printk("%d\n",__LINE__);
				ak_dock_led_control(LED_ON);	
			}else{
				glow_battery_led = 0;
				//printk("%d\n",__LINE__);
				ak_dock_led_control(LED_OFF);	
			}
		}
	}
}

void ak_dock_power(int ctl)
{
	printk("+ak_dock_power:%d\n",ctl);
	
	//gpio_request(GPIO_AMP_ACC_EN	,"GPIO_AMP_ACC_EN");
	//gpio_request(GPIO_DOCK_I2C_EN	,"GPIO_DOCK_I2C_EN");
	
	msleep_interruptible(300);
	
	if(ctl){
		gpio_set_value(GPIO_AMP_ACC_EN,1);
		gpio_set_value(GPIO_DOCK_I2C_EN,0); //Low active
	}
	else{
		gpio_set_value(GPIO_AMP_ACC_EN,0);
		gpio_set_value(GPIO_DOCK_I2C_EN,1);
	}

	msleep_interruptible(300);

	printk("-ak_dock_power:%d\n",ctl);

}
static int  is_bal(void)
{
	return gpio_get_value(GPIO_BAL_DET);
}
static int is_unbal(void)
{
	return !gpio_get_value(GPIO_HP_DET);

}	

void dock_mute(int val)
{
	int dock_device = ak_dock_check_device();
	if(dock_device == DEVICE_AMP){
		//printk("dock_mute:%d\n",val);		
		//CS3318_volume_mute(val); // CS3318 S/W MUTE	
		//gpio_direction_output(EXT_GPIO_P10, val); // CS3318 H/W MUTE
		gpio_set_value(GPIO_AMP_SW_MUTE_COM ,val);
	}	
}
EXPORT_SYMBOL(dock_mute);

void amp_bal_unbal_select(int val)
{

 	//ak_set_hw_mute(AMT_HP_MUTE,1);

	if(val == AOT_HP){
		//printk("amp_bal_unbal_select: AOT_HP\n");
		amp_bal_enable(0);
    	amp_unbal_enable(1);
	}else {
		//printk("amp_bal_unbal_select: AOT_BALANCED\n");
		amp_bal_enable(1);
		if(g_stradeum_flag)
			amp_unbal_enable(1); //스트라디움용 펌웨어서 동작 
		else
			amp_unbal_enable(0);
	}

	//ak_set_hw_mute(AMT_HP_MUTE,0);
}
EXPORT_SYMBOL(amp_bal_unbal_select);

/* [downer] A150624 for AMP output detect wq */
static void amp_output_detect_work(struct work_struct *wq)
{
    g_amp_current_out_state = (detect_amp_unbal() << 1) | detect_amp_bal();
    
    if (g_amp_before_out_state != g_amp_current_out_state) {
        switch(g_amp_current_out_state) {
        case 3:
            printk("DUAL OUTPUT DETECT!\n");
            gpio_direction_output(EXT_GPIO_P12, 0); /* BAL_AMP_EN */
            gpio_direction_output(EXT_GPIO_P03, 0); /* BAL_MUTE */            
        case 2:
            printk("UNBAL AMP on!\n");            
            amp_bal_enable(0);
			amp_unbal_enable(1);

            break;
        case 0:
            printk("AMP NO OUT!\n");
            amp_bal_enable(0);			
			amp_unbal_enable(0);
			
            break;
        case 1:
            printk("BAL AMP on!\n");
            amp_bal_enable(1);
			amp_unbal_enable(0);

            break;
        }
        
        g_amp_before_out_state = g_amp_current_out_state;
    }
    
   // schedule_delayed_work(&amp_out_detect_wq, AMP_OUT_DETECT_TIMER);    
}

int recorder_set_adc_reset(int no_reset)
{
	gpio_request(EXT_GPIO_P00,"EXT_GPIO0_ADC_RST");
	gpio_direction_output(EXT_GPIO_P00, no_reset);	/* ADC_RST-LOW ACTIVE */
	return 0;
}

int recorder_set_sa_reset(int no_reset)
{
	gpio_request(EXT_GPIO_P01,"EXT_GPIO1_SA_RST");
	gpio_direction_output(EXT_GPIO_P01, no_reset);
	return 0;
}

int recorder_set_vol_ctrl(int enable)
{
	gpio_request(EXT_GPIO_P02,"EXT_GPIO2_VOL_CTRL_EN");
	gpio_direction_output(EXT_GPIO_P02, enable);
	return 0;
}

int recorder_set_vol_reset(int no_reset)
{
	gpio_request(EXT_GPIO_P03,"EXT_GPIO3_VOL_CTRL_RST");
	gpio_direction_output(EXT_GPIO_P03, no_reset);
	return 0;
}

int recorder_set_pcmdsd_clock(int off)
{
	gpio_request(EXT_GPIO_P04,"EXT_GPIO4_PCM_DSD_EN");
	gpio_direction_output(EXT_GPIO_P04, off);
	return 0;
}

int recorder_set_dsddata_enable(int off)
{
	gpio_request(EXT_GPIO_P05,"EXT_GPIO5_DSD_DATA_ON");
	gpio_direction_output(EXT_GPIO_P05, off);
	return 0;
}

int recorder_set_phantom_pwr(int on)
{
	gpio_request(EXT_GPIO_P12,"EXT_GPIO12_PHANTOM_EN");
	gpio_direction_output(EXT_GPIO_P12, on);
	return 0;
}

int recorder_set_mic_preamp_pwr(int on)
{
	gpio_request(EXT_GPIO_P13,"EXT_GPIO13_PRE_PWR_EN");
	gpio_direction_output(EXT_GPIO_P13, on);
	return 0;
}

int recorder_set_adc_pwr(int on)
{
	gpio_request(EXT_GPIO_P14,"EXT_GPIO14_ADC_PWR_EN");
	gpio_direction_output(EXT_GPIO_P14, on);
	return 0;
}

int recorder_set_usb_switch(int connect_rec)
{
	gpio_request(EXT_GPIO_P15,"EXT_GPIO15_USB_CTL");
	gpio_direction_output(EXT_GPIO_P15, connect_rec);
	return 0;
}

int recorder_set_main_pwr(int on)
{
	gpio_request(EXT_GPIO_P16,"EXT_GPIO16_MAIN_PWR_EN");
	gpio_direction_output(EXT_GPIO_P16, on);
	return 0;
}

int recorder_set_usb_speed(int full_speed)
{
	gpio_request(EXT_GPIO_P17,"EXT_GPIO17_USB_SPEED");
	gpio_direction_output(EXT_GPIO_P17, full_speed);
	return 0;
}

int recorder_get_35_det(void)
{
	int ret = 0;
	gpio_request(EXT_GPIO_P06, "EXT_GPIO_35_DET");
	gpio_direction_input(EXT_GPIO_P06);
	ret = (gpio_get_value(EXT_GPIO_P06) >> 6);
	return ret;
}

int recorder_get_adc_ovf(void)
{
	int ret = 0;
	gpio_request(EXT_GPIO_P07, "EXT_GPIO_ADC_OVF");
	gpio_direction_input(EXT_GPIO_P07);
	ret = (gpio_get_value(EXT_GPIO_P07) >> 7);

	return ret;
}

int recorder_get_mic_l_ovr(void)
{
	int ret = 0;
	gpio_request(EXT_GPIO_P10, "EXT_GPIO_MIC_L_OVR");
	gpio_direction_input(EXT_GPIO_P10);
	ret = (gpio_get_value(EXT_GPIO_P10) >> 10);

	return ret;
}

int recorder_get_mic_r_ovr(void)
{
	int ret = 0;
	gpio_request(EXT_GPIO_P11, "EXT_GPIO_MIC_R_OVR");
	gpio_direction_input(EXT_GPIO_P11);
	ret = (gpio_get_value(EXT_GPIO_P11) >> 11);

	return ret;
}

static void recorder_detect_work(struct work_struct *wq)
{
	

}

void recorder_start(int wakeup)
{
	recorder_set_main_pwr(1);

	recorder_set_adc_pwr(1);
	recorder_set_usb_switch(1);
	recorder_set_mic_preamp_pwr(1);

#if 1
	msleep(10);
	recorder_set_sa_reset(0);
	msleep(10);
	recorder_set_sa_reset(1);
	msleep(10);
#endif

}

void recorder_stop(int sleep)
{
	recorder_set_mic_preamp_pwr(0);
	recorder_set_adc_pwr(0);
	recorder_set_main_pwr(0);
}

void start_cradle(void)
{
	printk("start_cradle\n");

	//본체 BAL/UNBAL mute
	ak_set_hw_mute(AMT_HP_MUTE,1);		
	
	//본체에서 4GO핀 출력 MUTE시킴
	dock_mute(1);

	//본체 출력 MUTE시킴
	gpio_set_value(GPIO_AUDIO_SW_MUTE ,1);	
	// Low(L1,R1): 본체에서 4GOPIN 출력 enable , Hihg(L2,R21): 본체에서 4GOPIN 출력 disable
	gpio_set_value(GPIO_AMP_BAL_SEL_COM,0); 

    if (ak_is_connected_optical_tx()){ /* [downer] A140715 AMP on시 광 off */
        ak_set_snd_pwr_ctl(AK_OPTICAL_TX_POWER,AK_PWR_OFF);
        ak_set_optical_tx_status(0);
    }
			

	//본체에서 출력 MUTE 푼다 
	dock_mute(0);

	start_amp_switch();
	set_ak_dock_power_state(POWER_ON);			
		
}
EXPORT_SYMBOL(start_cradle);
	
void stop_cradle(void)
{
	printk("stop_cradle\n");

	set_ak_dock_power_state(POWER_OFF); // cradle off하지 않고 cradle임의로 제거시 gpio_direction_output2 에서 gpio_direction_output 정상적으로 호출하기 위해..

	//본체 BAL/UNBAL mute
	gpio_set_value(GPIO_HP_BAL_MUTE,1);
	gpio_set_value(GPIO_HP_MUTE,1);

	// Low(L1,R1): 본체에서 출력 enable , High(L2,R21): 본체에서 출력 disable
	gpio_set_value(GPIO_AMP_BAL_SEL_COM,1); 	
	gpio_set_value(GPIO_AMP_SW_MUTE_COM ,1);
	gpio_set_value(GPIO_AUDIO_SW_MUTE ,1); // 본체 내부 출력 mute: AMP OFF시 Pause->Play시점에서 GPIO_AUDIO_SW_MUTE플 풀게 한다 
    gpio_set_value(GPIO_AMP_ACC_EN, 1);  // Cradle일때는 MUTE 핀이므로 Hihg     	
    
    if(is_optical_tx_connect())
        ak_set_optical_tx_status(1);

}

void init_amp_gpio(void)
{
	ak_dock_led_control(LED_OFF);	
	
	//amp BAL/UNBAL mute
	amp_bal_mute(1);
	amp_unbal_mute(1);
	
	//본체 BAL/UNBAL mute
	gpio_set_value(GPIO_HP_BAL_MUTE,1);
	gpio_set_value(GPIO_HP_MUTE,1);

	//본체출력 MUTE시킴
	gpio_set_value(GPIO_AUDIO_SW_MUTE ,1);	
	// Low(L1,R1): 본체에서 4GOPIN 출력 enable , Hihg(L2,R21): 본체에서 4GOPIN 출력 disable
	gpio_set_value(GPIO_AMP_BAL_SEL_COM,0); 

	//PCF8575RGER 초기화
	gpio_direction_output(EXT_GPIO_P00, 1);	/* CHG_DET */
	gpio_direction_output(EXT_GPIO_P02, 1); /* VOL_CTL_EN */
	gpio_direction_input(EXT_GPIO_P05);    /* BAL_DET */
	gpio_direction_input(EXT_GPIO_P06);    /* UNBAL_DET */
	gpio_direction_output(EXT_GPIO_P07, 1); /* VOL_CTL_RST */
	gpio_direction_output(EXT_GPIO_P10, 0); /* VOL_CTL_MUTE */
	gpio_direction_output(EXT_GPIO_P11, 0); /* UNBAL_AMP_EN */
	gpio_direction_output(EXT_GPIO_P12, 0); /* BAL_AMP_EN */
	gpio_direction_output(EXT_GPIO_P15, 0); /* AMP_PWR_EN not used on AMP ES event*/
}


void deinit_amp_gpio(void)
{
	ak_dock_led_control(LED_OFF);	

	//amp BAL/UNBAL mute
	amp_bal_mute(1);
	amp_unbal_mute(1);

	//본체 BAL/UNBAL mute
	gpio_set_value(GPIO_HP_BAL_MUTE,1);
	gpio_set_value(GPIO_HP_MUTE,1);

	// Low(L1,R1): 본체에서 출력 enable , High(L2,R21): 본체에서 출력 disable
	gpio_set_value(GPIO_AMP_BAL_SEL_COM,1); 	
	dock_mute(1); // 본체에서 AMP쪽으로 출력 mute 		
	gpio_set_value(GPIO_AUDIO_SW_MUTE ,1); // 본체 내부 출력 mute: AMP OFF시 Pause->Play시점에서 GPIO_AUDIO_SW_MUTE플 풀게 한다 

    /* [downer] A150624 */
	gpio_direction_output(EXT_GPIO_P02, 0); /* VOL_CTL_EN */
	gpio_direction_output(EXT_GPIO_P07, 0); /* VOL_CTL_RST */
	gpio_direction_output(EXT_GPIO_P10, 0); /* VOL_CTL_MUTE */            
	gpio_direction_output(EXT_GPIO_P11, 0); /* UNBAL_AMP_EN */
	gpio_direction_output(EXT_GPIO_P12, 0); /* BAL_AMP_EN */
	gpio_direction_output(EXT_GPIO_P15, 0); /* AMP_PWR_EN not used on AMP ES event*/
}

void start_amp(int wakeup)
{
	printk("+start_amp\n");
	//printk("xmox:%d, pcm:%d\n",get_xmos_is_playing(),get_pcm_is_playing());

	//본체 출력 MUTE
	dock_mute(1);	

    if (ak_is_connected_optical_tx()){ /* [downer] A140715 AMP on시 광 off */
        ak_set_snd_pwr_ctl(AK_OPTICAL_TX_POWER,AK_PWR_OFF);
        ak_set_optical_tx_status(0);
    }

	init_amp_gpio();

	if(detect_amp_bal()) {	  // detect_amp_bal() 를 사용해야 BAL/UNBAL 제거상태에서 Sleep->wakeup시 UNBAL 출력 나옴 
		//amp_bal_unbal_select(AOT_BALANCED);
		ak_route_amp_hp_balanced_out(AOT_BALANCED);
	}else{
	
		if(g_stradeum_flag)
			ak_route_amp_hp_balanced_out(AOT_BALANCED); //스트라디움용 펌웨어서 동작 
		else
			ak_route_amp_hp_balanced_out(AOT_HP);
	}

	 if(!wakeup){  // wakeup한후에는 Pause되는것을 막기위해..
			//printk("start_amp_switch-%d\n");
			start_amp_switch();
			set_ak_dock_power_state(POWER_ON);
	 }	
 	
    //schedule_delayed_work(&amp_out_detect_wq, AMP_OUT_DETECT_TIMER); /* [downer] A150624 for AMP out detect */
}

void stop_amp(int sleep)
{
	printk("stop_amp\n");

	set_ak_dock_power_state(POWER_OFF);

	ak_set_hw_mute(AMT_HP_MUTE,1);		
	
    //cancel_delayed_work(&amp_out_detect_wq); /* [downer] A150624 */
    g_amp_before_out_state = AMP_NO_OUT;

    /* [downer] M150715 sleep 조건이 아닐때만 실행 */
    if (!sleep){
        stop_amp_switch();

		if (is_optical_tx_connect()) //optical tx가 연결되어 있으면 
	    	ak_set_optical_tx_status(1);
    }
	
	ak4490_resume_volume();	

	dock_mute(1);
	
	deinit_amp_gpio();

	ak_dock_power(POWER_OFF);

}

static void ak_cradle_error_work(struct work_struct *work)
{
		printk("ak_cradle_error_work\n");			
		sys_external_dock_state(DEVICE_CRADLE_OFF);
		cancel_delayed_work(&cradle_error_work);
}

//ir_ioctl_cmd.c에서 호출
void ak_ext_dock_control(int pwr_ctl)
{
	//mutex_lock(&dock_lock);
	if ((pwr_ctl == POWER_ON) || (pwr_ctl == POWER_WAKEUP)) { 
		
		// sys/class/switch/external_dock/state 
		if(now_device_id==DEVICE_AMP){ //AMP
			printk("DEVICE_AMP ON\n");

			ak_dock_power(POWER_ON);

			start_ext_battery(); //Dock 뱃터리 최기화 및 1초 단위 체크 시작

			if(check_dock_battery_id()){
				if(!check_init_low_battery()){ // AMP, REC인경우 battery 0%인지 체크 
					 if (pwr_ctl == POWER_ON)
						start_amp(0);							
					else
						start_amp(1); //wakeup 							
					
						init_CS3318();

						//start amp 이후에  POWER_ON상태로 변경 필요 그렇지 않으면 본체 이어폰 연결 상태에서 AMP ON시 본체 이어폰에서 간헐 MAX 볼륨 나옴				 
						set_ak_dock_power_state(POWER_ON); 
						
		              if (pwr_ctl == POWER_ON)
		                    sys_external_dock_state(DEVICE_AMP_ON);	//AMP ON							
					}
			}else{
				ak_dock_power(POWER_OFF); // ysyim-2015.11.19: AMP ON 실패시 AMP_ACC_EN / DOCK_I2C_EN  OFF시킴 
			}
		}
		else if(now_device_id==DEVICE_CRADLE){ //Cradle
			if(is_charging()){
				printk("DEVICE_CRADLE ON\n");
				start_cradle();	
				sys_external_dock_state(DEVICE_CRADLE_ON);	//Cradle ON		
			}else{  // USB케이블이 연결되어 있지 않을경우 -120 error return후 0 return
				printk("DEVICE_CRADLE ON: USB cable is not detected\n");			
				sys_external_dock_state(CRADLE_POWER_ERROR); //state 값에 -120 error  
				schedule_delayed_work(&cradle_error_work, CRADLE_ERROR_TIMER);  // state 값에 0 
			}			
		}
		else if(now_device_id==DEVICE_RECORDER){ //REC
			printk("DEVICE_RECORDER ON\n");
			ak_dock_power(POWER_ON);

			start_ext_battery(); //Dock 뱃터리 최기화 및 1초 단위 체크 시작

			if(check_dock_battery_id()){
				if(!check_init_low_battery()){ // AMP, REC인경우 battery 0%인지 체크 
					 if (pwr_ctl == POWER_ON)
						recorder_start(0);							
					else
						recorder_start(1); //wakeup 							
						init_MAX14662();	
#if 1
						init_CS3318_rec();
						init_AK5572();
						init_reg_PGA2505();
						init_SA9227();
#endif
						set_ak_dock_power_state(POWER_ON); 
						
		              if (pwr_ctl == POWER_ON)
		                    sys_external_dock_state(DEVICE_RECORDER_ON);	//AMP ON							
					}
			}
		}			
	}
	else if ((pwr_ctl == POWER_OFF) || (pwr_ctl == POWER_SLEEP)) {
		
		if(now_device_id==DEVICE_AMP){ //AMP
			printk("DEVICE_AMP OFF\n");
			
			stop_ext_battery(); //Dock 뱃터리 체크 중단

            if (pwr_ctl == POWER_OFF) {
                stop_amp(0);                
                sys_external_dock_state(DEVICE_AMP_OFF); //AMP OFF
            }
            else /* [downer] M150715 */
                stop_amp(1);
		}
		else if(now_device_id==DEVICE_CRADLE){ //Cradle
			stop_cradle();			
			if(is_charging()){
				printk("DEVICE_CRADLE OFF\n");
				sys_external_dock_state(DEVICE_CRADLE_OFF);
			}else{
				printk("DEVICE_CRADLE OFF: USB cable is not detected\n");	
				sys_external_dock_state(CRADLE_POWER_ERROR); //state 값에 -120 error  
				schedule_delayed_work(&cradle_error_work, CRADLE_ERROR_TIMER);  // state 값에 0 
			}
		}
		else if(now_device_id==DEVICE_RECORDER){ //REC
			printk("DEVICE_RECORDER OFF\n");
			stop_ext_battery(); //Dock 뱃터리 체크 중단

			if (pwr_ctl == POWER_OFF) {
				recorder_stop(0);                
				sys_external_dock_state(DEVICE_RECORDER_OFF); //REC OFF
			}
			else /* [downer] M150715 */
				recorder_stop(1);
		}

        /* [downer] M150715 stop_amp 이후에 상태값 변경. */
        if (pwr_ctl == POWER_OFF)
            set_ak_dock_power_state(POWER_OFF);
        else 
            set_ak_dock_power_state(POWER_SLEEP);        
		
	}
 
	//printk("ak_ext_dock_control(%d) now_device_id(%d)\n",pwr_ctl, now_device_id);	
	//mutex_unlock(&dock_lock);
	
}
EXPORT_SYMBOL(ak_ext_dock_control);

//500mse마다 호출
spinlock_t ak_dock_lock;
static void ak_dock_ctl_work(struct work_struct *work)
{
	int detect_device;

	//spin_lock(&ak_dock_lock);
	//printk("%s\n",__FUNCTION__);		
	//if(get_boot_complete()){
	if(get_dock_event_ready()){		
		detect_device = ak_dock_check_device();
		now_device_id = detect_device;

		if(fake_ext_audio) // fake_ext_audio 값 이 설정 될경우 detect_device 무시하고 fake_ext_audio 값으로 설정 
			now_device_id = fake_ext_audio;

		if(now_device_id == DEVICE_RECORDER)
			//AMP, REC인경우 battery 0%이지 체크 		
			check_working_low_battery();

			display_led();
		}
		
		if(now_device_id == DEVICE_CRADLE){
			if((ak_dock_power_state()==POWER_ON) && !is_charging()){			
					printk("CRADLE OFF: USB cable is removed\n");		
					ak_ext_dock_control(POWER_OFF);
			}
		}
		//printk("%s: now_device_id:%d, old_device_id:%d, fake_ext_audio:%d\n",__FUNCTION__,now_device_id, old_device_id,fake_ext_audio);		
		
		if(now_device_id != old_device_id){
			printk("\nak_dock_id:%d\n",now_device_id);		
			if(now_device_id){
				if(ak_dock_check_device()==DEVICE_CRADLE)
	    				gpio_set_value(GPIO_AMP_ACC_EN, 1);  // Cradle일때는 MUTE 핀이므로 Hihg   
	    
				if(now_device_id != 0 && old_device_id != 0){
					now_device_id = 0;
					sys_external_dock_state(0);   // ysyim-2015.11.19: Device가 연결된 상태에서 해지 절차없이 간헐적 다른 디바이스 인신시 강제로 한번 해지 시키게 함
				}else{
					sys_external_dock_state(now_device_id); // 1s마다 어떤 dock이 연결되어 있는지 확인후 /sys/class/switch/external_dock/state 에 write한다 				
				}
			}else{ //No device
				
				switch(old_device_id){
					case DEVICE_AMP:
						stop_ext_battery();	// dock이 연결되어 있는지 않으면 뱃터리체크 중단
						stop_amp(0);
						break;
						
					case DEVICE_CRADLE:
						stop_cradle();
						gpio_set_value(GPIO_AMP_ACC_EN, 0);  // Cradle 제거시 AMP_ACC_EN 0로 하여 AMP_ACC_EN high 상태로 AMP에 연결되지 않도록 
						break;
						
					case DEVICE_RECORDER:
						break;	
				}
				
				now_device_id = 0;
				sys_external_dock_state(now_device_id); // dock이 연결되어 있는지 않으면 /sys/class/switch/external_dock/state 0 write
			}			
			old_device_id = now_device_id;
		}
	}	

	//spin_unlock(&ak_dock_lock);
	
	schedule_delayed_work(&ak_dock_work, AK_DOCK_CONNECT_TIMER);

}

static void ak_dock_early_suspend(struct early_suspend *h)
{	
	printk("%s\n",__FUNCTION__);		
}

static void ak_dock_late_resume(struct early_suspend *h)
{	
	printk("%s\n",__FUNCTION__);		

}
static int ak_dock_suspend(struct platform_device *pdev, pm_message_t state)
{                   
   	printk("%s\n",__FUNCTION__);     
	cancel_delayed_work(&ak_dock_work);
	if(is_amp_on()){
	    /* [downer] A150625 */
	   // cancel_delayed_work(&amp_out_detect_wq); 
	    g_amp_before_out_state = AMP_NO_OUT;

		//Sleep진입시 노이즈 제거
		ak_set_hw_mute(AMT_HP_MUTE,1);

		//BAL/UNBAL AMP Power disable
		amp_bal_enable(0);
		amp_unbal_enable(0);
	    
		dock_mute(1); //본체 출력 MUTE	
		//인자로 POWER_OFF를 쓰면  dock_power_state 의 값이 POER_OFF로 되어 resuem시 bat초기화 하지 못함 
		//POWER_SLEEP인자를 두어 dock_power_state값은 변하지 않고 stop_ext_battery(), ak_dock_power() 함수만 실행한다.

	if(g_stradeum_flag)  // AMP_SLEEP_WAKUP_ENABLE
		ak_ext_dock_control(POWER_SLEEP);  //스트라디움 펌웨어는 AMP연결상태에서 Slepp->Wakeup시 자동으로 AMP ON되도록 함
	else
		ak_ext_dock_control(POWER_OFF); //일반 사용자 펌웨어는 AMP연결상태에서 Slepp->Wakeup시 자동으로 AMP OFF되도록 함 
		
	}
    return 0;       
}

static int ak_dock_resume(struct platform_device *pdev)
{  
	printk("%s %d\n",__FUNCTION__,ak_dock_power_state());

	if (ak_dock_power_state() == POWER_SLEEP) {
        ak_ext_dock_control(POWER_WAKEUP); //sleep진입전 dock의 Power가 ON되어있었으면 resume시 다시 ON시킨다
    }

	schedule_delayed_work(&ak_dock_work, AK_DOCK_CONNECT_TIMER);
  //  schedule_delayed_work(&amp_out_detect_wq, AMP_OUT_DETECT_TIMER); /* [downer] A150625 for AMP out detect */
    return 0;  
}

static void ak_dock_selec(unsigned int selected)
{

}
static void ak_dock_convert(unsigned int d0, unsigned int d1)
{

}
static __devinit int ak_dock_probe(struct platform_device *pdev)
{

	printk("%s\n",__FUNCTION__);

	dcok_early_suspend.suspend = ak_dock_early_suspend;	
	dcok_early_suspend.resume = ak_dock_late_resume;	
	dcok_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;	
	register_early_suspend(&dcok_early_suspend);

	dock_dev.name = "external_dock" ;
	switch_dev_register(&dock_dev);

    /* [downer] A150820 for cdripper event */
    cdripper_dev.name = "cd_rom";
    switch_dev_register(&cdripper_dev);
    
	//ad_client = tcc_adc_register(pdev, ak_dock_selec, ak_dock_convert, 0);

#if defined(CONFIG_GPIO_PCF857X) // refer to drivers/gpio/gpio-pcf857x.c
	//gpio_request(EXT_GPIO_P01,"EXT_GPIO_P01");	
	//gpio_direction_output(EXT_GPIO_P01,0);	
	//gpio_set_value(EXT_GPIO_P01,1);	// EXT_GPIO_P01? OPAMP_EN High
#endif

	gpio_request(GPIO_AMP_ACC_EN	,"GPIO_AMP_ACC_EN");
	tcc_gpio_config(GPIO_AMP_ACC_EN, GPIO_FN(0));
	if(ak_dock_check_device()==DEVICE_CRADLE){
		printk("ak_dock_probe: DEVICE_CRADLE\n");
		gpio_direction_output(GPIO_AMP_ACC_EN, 1);  // GPIO_AMP_ACC_EN == CRADLE_XLR_MUTE    		
		msleep(100);  // Cradle연경상태에서 부팅중 팝노이즈 제
	}else{
		printk("ak_dock_probe\n");
		gpio_direction_output(GPIO_AMP_ACC_EN, 0);
	}

	gpio_request(GPIO_AMP_SW_MUTE_COM	,"GPIO_AMP_SW_MUTE_COM");
	tcc_gpio_config(GPIO_AMP_SW_MUTE_COM, GPIO_FN(0));
	gpio_direction_output(GPIO_AMP_SW_MUTE_COM, 1);

	gpio_request(GPIO_AMP_BAL_SEL_COM	,"GPIO_AMP_BAL_SEL_COM");
	tcc_gpio_config(GPIO_AMP_BAL_SEL_COM, GPIO_FN(0));
	gpio_direction_output(GPIO_AMP_BAL_SEL_COM, 1);

	gpio_request(GPIO_DOCK_I2C_EN	,"GPIO_DOCK_I2C_EN");
	tcc_gpio_config(GPIO_DOCK_I2C_EN, GPIO_FN(0));
	gpio_direction_output(GPIO_DOCK_I2C_EN, 1); //Low active
	
	gpio_request(GPIO_AMP_DET,"GPIO_AMP_DET");	
	tcc_gpio_config(GPIO_AMP_DET, GPIO_FN(0));
	gpio_direction_input(GPIO_AMP_DET);	

	gpio_request(GPIO_AMP_ADP_DET	,"GPIO_AMP_ADP_DET");
	tcc_gpio_config(GPIO_AMP_ADP_DET, GPIO_FN(0) | GPIO_PULLUP);
	gpio_direction_input(GPIO_AMP_ADP_DET);	

    /* [downer] A150624 for AMP */
	gpio_request(EXT_GPIO_P00,"EXT_GPIO0_CHG_DET");
	gpio_request(EXT_GPIO_P01,"EXT_GPIO_INPUT_SEL");			
	gpio_request(EXT_GPIO_P02,"EXT_GPIO2_VOL_CTL_EN");
	gpio_request(EXT_GPIO_P03,"EXT_GPIO3_BAL_MUTE");
	gpio_request(EXT_GPIO_P04,"EXT_GPIO4_UNBAL_MUTE");
	gpio_request(EXT_GPIO_P05,"EXT_GPIO5_BAL_DET#");
	gpio_request(EXT_GPIO_P06,"EXT_GPIO6_UNBAL_DET#");
	gpio_request(EXT_GPIO_P07,"EXT_GPIO7_VOL_CTL_RST");
	gpio_request(EXT_GPIO_P10,"EXT_GPIO10_VOL_CTL_MUTE");
	gpio_request(EXT_GPIO_P11,"EXT_GPIO11_UNBAL_AMP_EN");
	gpio_request(EXT_GPIO_P12,"EXT_GPIO12_BAL_AMP_EN");
	gpio_request(EXT_GPIO_P13,"EXT_GPIO13_LED_R");
	gpio_request(EXT_GPIO_P14,"EXT_GPIO14_LED_B");
	gpio_request(EXT_GPIO_P15,"EXT_GPIO15_AMP_PWR_EN"); /* not used on AMP ES event*/
    
	INIT_DELAYED_WORK(&ak_dock_work, ak_dock_ctl_work);
	INIT_DELAYED_WORK(&cradle_error_work, ak_cradle_error_work);
  // INIT_DELAYED_WORK(&amp_out_detect_wq, amp_output_detect_work); /* [downer] A150624 */
    
	schedule_delayed_work(&ak_dock_work, AK_DOCK_CONNECT_TIMER);
	return 0;
}

static __devexit int ak_dock_remove(struct platform_device *pdev)
{
	printk("%s\n",__FUNCTION__);
	return 0;
}

static __devexit int ak_dock_shutdown(struct platform_device *pdev)
{
	printk("%s\n",__FUNCTION__);
	ak_set_hw_mute(AMT_HP_MUTE,1);

	if(is_amp_on())
		stop_amp(0);
	if(is_cradle_on())
		stop_cradle();
	
	return 0;
}

static struct platform_driver ak_dock_driver = {
	.driver = {
		.name = "ak_dock_drv",
		.owner = THIS_MODULE,
	},
	.probe = ak_dock_probe,
	.remove = __devexit_p(ak_dock_remove),
	.shutdown = ak_dock_shutdown,
	.suspend	= ak_dock_suspend,
	.resume 	= ak_dock_resume,
};



static struct platform_device *ak_dock_dev;

int __init ak_dock_board_init(void)
{
	int ret;

	ak_dock_dev = platform_device_alloc("ak_dock_drv", -1);
	if (!ak_dock_dev)
		return -ENOMEM;

	ret = platform_device_add(ak_dock_dev);
	if (ret != 0) {
		platform_device_put(ak_dock_dev);
		return ret;
	}

	ret = platform_driver_register(&ak_dock_driver);
	if (ret != 0)
		platform_device_unregister(ak_dock_dev);

	return ret;
}

void __exit ak_dock_board_exit(void)
{
	printk("ak_dock_board_exit\n");
	platform_device_unregister(ak_dock_dev);
	platform_driver_unregister(&ak_dock_driver);
}

device_initcall(ak_dock_board_init);
#else
static int g_ak_otg_device = 0;	/* [downer] A160125 */
static struct switch_dev cdripper_dev; /* [downer] A150820 for cdripper event */
static struct platform_device *ak_dock_dev;

/* [downer] A150820 for otg device event */
void sys_external_otg_state(int state)
{
    switch_set_state(&cdripper_dev, state);
}
EXPORT_SYMBOL(sys_external_otg_state);

/* [downer] A160125 set external OTG device */
int ak_get_otg_device(void)
{
	return g_ak_otg_device;
}
EXPORT_SYMBOL(ak_get_otg_device);

void ak_set_otg_device(int device)
{
	g_ak_otg_device = device;
}
EXPORT_SYMBOL(ak_set_otg_device);

static __devexit int ak_dock_remove(struct platform_device *pdev)
{
	return 0;
}

static __devexit int ak_dock_shutdown(struct platform_device *pdev)
{
	return 0;
}

static __devinit int ak_dock_probe(struct platform_device *pdev)
{
	printk("%s\n",__FUNCTION__);

    /* [downer] A150820 for cdripper event */
    cdripper_dev.name = "cd_rom";
    switch_dev_register(&cdripper_dev);
    
	return 0;
}

static struct platform_driver ak_dock_driver = {
	.driver = {
		.name = "ak_dock_drv",
		.owner = THIS_MODULE,
	},
	.probe = ak_dock_probe,
	.remove = __devexit_p(ak_dock_remove),
	.shutdown = ak_dock_shutdown,
//	.suspend	= ak_dock_suspend,
//	.resume 	= ak_dock_resume,
};

int __init ak_dock_board_init(void)
{
	int ret;

	ak_dock_dev = platform_device_alloc("ak_dock_drv", -1);
	if (!ak_dock_dev)
		return -ENOMEM;

	ret = platform_device_add(ak_dock_dev);
	if (ret != 0) {
		platform_device_put(ak_dock_dev);
		return ret;
	}

	ret = platform_driver_register(&ak_dock_driver);
	if (ret != 0)
		platform_device_unregister(ak_dock_dev);

	return ret;
}

void __exit ak_dock_board_exit(void)
{
	printk("ak_dock_board_exit\n");
	
	platform_device_unregister(ak_dock_dev);
	platform_driver_unregister(&ak_dock_driver);
}

device_initcall(ak_dock_board_init);

#endif

