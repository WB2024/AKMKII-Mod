#ifndef __BOARD_ASTELL_KERN__
#define __BOARD_ASTELL_KERN__

//#define LCD_SET_BRIGHTNESS		/* [downer] A160520 LCD 밝기 강제 조정 */

enum {
	AK_PWR_ON = 1,
	AK_PWR_OFF = 0
};

enum {
	DOCK_PWR_ON = 1,
	DOCK_PWR_OFF = 0
};

enum {
	DEVICE_NO = 0,
	DEVICE_AMP = 1,
	DEVICE_RECORDER = 2,
	DEVICE_CRADLE = 6,
	DEVICE_RIPPER = 10,
	DEVICE_USBAUDIO = 11
};

enum {
    OTG_NC_DEVICE = -200, // not connect
    OTG_NS_DEVICE = -201, // not support
    OTG_UNLOAD  = -202,
    OTG_NO_POWER = -120,
    OTG_DISCONNECT = 0,
	
    OTG_RIPPER_CONNECT = 1,
	OTG_RECORDER_CONNECT = 2,
	OTG_USBAUDIO_CONNECT = 300
};

/* [downer] A160125 */
enum {
	OTG_SELFPOWER_DEVICE = 0,	/* OTG 장치에 전원이 연결되어 있는 장치 */
	OTG_NOPOWER_DEVICE = 1		/* AK에서 5V 전원을 밀어줘야 하는 장치 */
};
	
/* [downer] A150624 */
enum {
    AMP_OUT_INIT = -1,
    AMP_NO_OUT = 0,
    AMP_UNBAL = 1,
    AMP_BAL = 2
};
    
typedef enum {
	ADP_NONE = -1,
	ADP_PLAYBACK,
	ADP_OPTICAL_IN,
	ADP_OPTICAL_OUT,
	ADP_USB_DAC
} AUDIO_DATA_PATH_TYPE;

void ak_set_audio_path(AUDIO_DATA_PATH_TYPE e_type);
int ak_get_audio_path(void);


typedef enum {
	AMT_HP_MUTE = 0,
	AMT_DAC_MUTE
} AUDIO_HW_MUTE_TYPE;


//arrange me later...
void ak_set_hw_mute(AUDIO_HW_MUTE_TYPE e_type,int onoff);
void ak_set_hw_mute_for_usbdac(AUDIO_HW_MUTE_TYPE e_type,int onoff);

void ak_set_hw_mute_for_ak100_ii(AUDIO_HW_MUTE_TYPE e_type,int onoff);
void ak_set_hp_mute(int onoff);
void ak_set_audio_sw_mute(int onoff);


typedef enum snd_pwr_ctrl{
	AK_AUDIO_POWER_ALL_OFF =0,
	AK_AUDIO_POWER = 1,
	AK_OPTICAL_RX_POWER=2,
	AK_OPTICAL_TX_POWER=4,
	AK_AUDIO_POWER_SUSPEND=8  /* [downer] A130827 */
}snd_pwr_ctrl_t;

int ak_get_snd_pwr_ctl(int power_ctl_status);
void ak_set_snd_pwr_ctl(snd_pwr_ctrl_t snd_ctl,int pwr_ctl);

typedef enum snd_wm8804_mode{
	SND_WM8804_PLAY = 0,
	SND_WM8804_SPDIF_BYPASS,
	SND_WM8804_SPDIF_TX,
}snd_wm8804_mode_t;


typedef enum {
    UCM_INIT = -3,
	UCM_NONE = -2,
	UCM_BOOTING = -1,
	UCM_MTP = 0,
	UCM_PC_FI,
	UCM_INTERNAL_DSD,
	UCM_DOCKING,
    UCM_ADAPTER,
    UCM_CHECKING,
    UCM_OTG_HOST,                    /* [downer] A150817 */
} USB_CONN_MODE;


typedef enum {
	INTERNAL_NONE_PLAY,
	INTERNAL_PCM_PLAY,
	INTERNAL_DSD_PLAY
	
} INTERNAL_PLAY_MODE;

INTERNAL_PLAY_MODE ak_get_internal_play_mode(void);
void ak_internal_play_change(INTERNAL_PLAY_MODE mode);



#ifdef _MTP_SOUND_BREAK_FIX
typedef enum {
    MPS_STOP = 0,
	MPS_UNDER_96,
	MPS_OVER_96
} MTP_PLAY_STATUS;

void mtp_set_play_status(int status);
int mtp_get_play_status(void);
#endif	


typedef enum {
	EXT_DOCK_OFF = 0,	
	EXT_DOCK_ON
} AK_AUDIO_OUT_TYPE;

typedef enum {
	AOT_HP = 0,
	AOT_BALANCED
} AK_EXT_AUDIO_TYPE;


typedef enum {
	AUD_PATH_HP_35 = 0,
	AUD_PATH_HP_65,
	AUD_PATH_HP_BAL,
	AUD_PATH_FIXED_UNBAL,
	AUD_PATH_FIXED_BAL,
	AUD_PATH_VAR_BAL,
	AUD_PATH_VAR_UNBAL,
	AUD_PATH_DIG_IN,
	AUD_PATH_DIG_OUT
	
} AK_SND_OUT_PATH;

void ak_route_hp_balanced_out(int aot);


void ak_pcm_open_pre(void);
void ak_pcm_open_post(void);
void ak_pcm_start_pre(void);
void ak_pcm_start_post(void);
void ak_pcm_stop_pre(void);

void ak_pcm_stop_post(void);

void ak_pcm_close_pre(void);
void ak_pcm_close_post(void);


#ifdef PCM_DSD_CALLBACK

void snd_usb_pcm_open_pre();
void snd_usb_pcm_open_post();
void snd_usb_pcm_close_pre();
void snd_usb_pcm_close_post();
void snd_usb_playback_start_pre();
void snd_usb_playback_start_post();
void snd_usb_playback_stop_pre();
void snd_usb_playback_stop_post();
#endif

void ak_pcm_schedule_ctl(int onoff);
void pcm_open_elapsed_start(void);
void pcm_open_elapsed_end(void);
void set_audio_start_delay(int delay);


void audio_power_off_wq(void);
void set_audio_power_ctl_status(int onoff);



void ak_set_usb_conn_mode(USB_CONN_MODE ucm);

//wm8804.c
void wm8804_reinit(void);
void wm8804_spdif_rx_pwr(int onoff);
void wm8804_spdif_tx_pwr(int onoff);
void wm8804_mute2(int onoff);
int WM8804_SETPLL(unsigned int freq_in);

#ifdef FIX_MUTE_NOISE
void request_wm8804_set_pll(void);
int is_request_unmute(void);
int request_unmute(void);
int reset_unmute(void);

#endif


int tcc_pcm_fade_out();
int tcc_pcm_fade_in();

void ak_set_wm8804_spdif_mode(snd_wm8804_mode_t snd_ctl);
snd_wm8804_mode_t ak_get_wm8804_spdif_mode(void);

int ak_is_connected_optical_tx(void);
int ak_set_optical_tx_status(int connected);

void ak_set_snd_pwr_ctl(snd_pwr_ctrl_t snd_ctl,int pwr_ctl);
void ak_set_snd_pwr_ctl_request(snd_pwr_ctrl_t snd_ctl,int pwr_ctl);

// wm8740_spi.c
void init_wm8740(void);
void wm8740_set_volume(int lvol,int rvol);
void wm8740_set_mute(int mute);

// cs4398.c
void cs4398_reg_init(void);
void cs4398_schedule_power_ctl(int onoff);

void cs4398_set_volume(int mode, int lvol,int rvol);
void cs4398_volume_ramp_reset(void);
void cs4398_ramp_mode(int mode);


typedef enum {
	CS4398_RAMP_SR = 2,
	CS4398_RAMP_SRZC
} RAMP_TYPE;




void cs4398_volume_mute(void);
void cs4398_get_volume(int *lvol,int *rvol);
void cs4398_reg_freeze(int freeze);

void cs4398_resume_volume(void);
void cs4398_mute(int onoff);
void cs4398_pwr(int onoff);
void cs4398_FM(int freq);
void cs4398_FM_reinit(void);

void ak4490_reg_init(void);
void ak4490_schedule_power_ctl(int onoff);

void ak4490_set_volume(int mode, int lvol,int rvol);

void ak4490_volume_mute(void);
void ak4490_get_volume(int *lvol,int *rvol);
void ak4490_reg_freeze(int freeze);

void ak4490_resume_volume(void);
void ak4490_mute(int onoff);
void ak4490_pwr(int onoff);
void ak4490_FM(int freq);
void ak4490_FM_reinit(void);
void ak4490_save_cur_volume(void);

void ak8157a_set_samplerate(int rate);

#ifdef FORCE_USE_OPTICAL_RX_FUNC
void toggle_optical_rx_status(void);
#endif


// tonny
// get current usb connection mode
extern USB_CONN_MODE ak_get_current_usb_mode(void);



void ak_set_sidekey_lock_mode(int mode);

// ioctl_cmd.c 
int is_selected_balanced(void);

void CS3318_set_volume(int gain,int lvol,int rvol);
//ES9018
#if (CUR_AK == MODEL_AK1000N)
void ES9018_reg_init(void);
void ES9018_schedule_power_ctl(int onoff);
void CS3318_set_volume(int lvol,int rvol);
void init_reg_CS8416(void);
void init_reg_CS8406(void);
#elif (CUR_AK == MODEL_AK500N)
void CS3318_set_volume(int lvol,int rvol);
void Coxial_Out(int On_OFF);
void Optical_Out(int On_OFF);
void Spdif_Out(int On_OFF);
void Bnc_Out(int On_OFF);
void Fix_Unbal_Out(int On_OFF);
void Audio_Out1(int On_OFF);
void Audio_Out2(int On_OFF);
void Var_Unbal(int On_OFF);
void Local_in(int On_OFF);
void Coxial_In(int On_OFF);
void Optical_In(int On_OFF);
void Spdif_In(int On_OFF);
void Bnc_In(int On_OFF);
void headphone_35(int on);
void headphone_65(int on);
void headphone_bal(int on);

void init_reg_CS8416(unsigned char rxSel);
void init_reg_CS8406(void);
void init_reg_CS8406_192(void);
void init_reg_CS8406_176(void);
void init_reg_CS8406_48(void);

void ak_set_snd_out_path(void);


void set_xmos_is_playing(int playing);
int get_xmos_is_playing(void);


typedef enum {
	AUDIO_OUT_TYPE_NONE = -1,
	AUDIO_OUT_TYPE_AUDIO_OUT1,
	AUDIO_OUT_TYPE_AUDIO_OUT2,
	AUDIO_OUT_TYPE_VAR_UNBAL,
	AUDIO_OUT_TYPE_FIX_UNBAL,
	AUDIO_OUT_TYPE_HP_65,

	AUDIO_OUT_TYPE_HP_35,
	AUDIO_OUT_TYPE_HP_BAL,
	AUDIO_OUT_TYPE_COXIAL,
	AUDIO_OUT_TYPE_OPTICAL,
	AUDIO_OUT_TYPE_SPDIF,

	AUDIO_OUT_TYPE_BNC,
	AUDIO_OUT_TYPE_END
} AUDIO_OUT_TYPE;

typedef enum {
	AUDIO_IN_TYPE_NONE = -1,
	AUDIO_IN_TYPE_LOCAL,
	AUDIO_IN_TYPE_COXIAL,
	AUDIO_IN_TYPE_OPTICAL,
	AUDIO_IN_TYPE_SPDIF,
	AUDIO_IN_TYPE_BNC,

	AUDIO_IN_TYPE_END
} AUDIO_IN_TYPE;

typedef enum {
	AUDIO_PATH_NONE = -1,
	AUDIO_PATH_AUDIO_OUT1,
	AUDIO_PATH_AUDIO_OUT2,
	AUDIO_PATH_VAR_UNBAL,
	AUDIO_PATH_FIX_UNBAL,
	AUDIO_PATH_HP_65,
	AUDIO_PATH_HP_35,
	AUDIO_PATH_HP_BAL,
	AUDIO_PATH_COXIAL_OUT,
	AUDIO_PATH_OPTICAL_OUT,
	AUDIO_PATH_SPDIF_OUT,
	AUDIO_PATH_BNC_OUT,
	AUDIO_PATH_COXIAL_IN,
	AUDIO_PATH_OPTICAL_IN,
	AUDIO_PATH_SPDIF_IN,
	AUDIO_PATH_BNC_IN,
} AUDIO_PATH_STATE;
#endif

typedef enum {
	CONN_HP_TYPE_NONE = -1,
	CONN_HP_TYPE_35PHY,
	CONN_HP_TYPE_65PHY,
	CONN_HP_TYPE_BAL,
} CONN_HP_TYPE;


void msleep_interruptible_precise(int msec);


// RAID dip switch.

typedef enum {
	RT_0 = 0,
	RT_5
} RAID_TYPE;


#if (CUR_AK_REV == AK500N_HW_ES1) || (CUR_AK_REV == AK500N_HW_TP)
int get_ssd_attach(void);
int get_raid_array_type(void);
#endif



#endif

