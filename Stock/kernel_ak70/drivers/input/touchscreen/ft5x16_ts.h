#ifndef __LINUX_FT5X16_TS_H__
#define __LINUX_FT5X16_TS_H__


/* -- dirver configure -- */
#define CFG_SUPPORT_AUTO_UPG 1 
#define CFG_SUPPORT_UPDATE_PROJECT_SETTING  0
#define CFG_SUPPORT_TOUCH_KEY  1    //touch key, HOME, SEARCH, RETURN etc

#define CFG_MAX_TOUCH_POINTS  5
#define CFG_NUMOFKEYS 1
#define CFG_FTS_CTP_DRIVER_VERSION "3.0"

#ifdef CFG_SUPPORT_AUTO_UPG
#define FTS_UPGRADE_LOOP    30
#endif

#define SCREEN_MAX_X    480//1024
#define SCREEN_MAX_Y    800//600
#define PRESS_MAX       255

#define CFG_POINT_READ_BUF  (3 + 6 * (CFG_MAX_TOUCH_POINTS))

#define FT5X16_NAME	"ft5x16_ts"

#define KEY_DOUBLE      2
#define KEY_PRESS       1
#define KEY_RELEASE     0

#define BL_VERSION_LZ4  0
#define BL_VERSION_Z7   1
#define BL_VERSION_GZF  2

enum ft5x16_ts_regs {
	FT5X16_REG_THGROUP					= 0x80,     /* touch threshold, related to sensitivity */
	FT5X16_REG_THPEAK						= 0x81,
	FT5X16_REG_THCAL						= 0x82,
	FT5X16_REG_THWATER					= 0x83,
	FT5X16_REG_THTEMP					= 0x84,
	FT5X16_REG_THDIFF						= 0x85,				
	FT5X16_REG_CTRL						= 0x86,
	FT5X16_REG_TIMEENTERMONITOR			= 0x87,
	FT5X16_REG_PERIODACTIVE				= 0x88,      /* report rate */
	FT5X16_REG_PERIODMONITOR			= 0x89,
	FT5X16_REG_HEIGHT_B					= 0x8a,
	FT5X16_REG_MAX_FRAME					= 0x8b,
	FT5X16_REG_DIST_MOVE					= 0x8c,
	FT5X16_REG_DIST_POINT				= 0x8d,
	FT5X16_REG_FEG_FRAME					= 0x8e,
	FT5X16_REG_SINGLE_CLICK_OFFSET		= 0x8f,
	FT5X16_REG_DOUBLE_CLICK_TIME_MIN	= 0x90,
	FT5X16_REG_SINGLE_CLICK_TIME			= 0x91,
	FT5X16_REG_LEFT_RIGHT_OFFSET		= 0x92,
	FT5X16_REG_UP_DOWN_OFFSET			= 0x93,
	FT5X16_REG_DISTANCE_LEFT_RIGHT		= 0x94,
	FT5X16_REG_DISTANCE_UP_DOWN		= 0x95,
	FT5X16_REG_ZOOM_DIS_SQR				= 0x96,
	FT5X16_REG_RADIAN_VALUE				=0x97,
	FT5X16_REG_MAX_X_HIGH                       	= 0x98,
	FT5X16_REG_MAX_X_LOW             			= 0x99,
	FT5X16_REG_MAX_Y_HIGH            			= 0x9a,
	FT5X16_REG_MAX_Y_LOW             			= 0x9b,
	FT5X16_REG_K_X_HIGH            			= 0x9c,
	FT5X16_REG_K_X_LOW             			= 0x9d,
	FT5X16_REG_K_Y_HIGH            			= 0x9e,
	FT5X16_REG_K_Y_LOW             			= 0x9f,
	FT5X16_REG_AUTO_CLB_MODE			= 0xa0,
	FT5X16_REG_LIB_VERSION_H 				= 0xa1,
	FT5X16_REG_LIB_VERSION_L 				= 0xa2,		
	FT5X16_REG_CIPHER						= 0xa3,
	FT5X16_REG_MODE						= 0xa4,
	FT5X16_REG_PMODE						= 0xa5,	  /* Power Consume Mode		*/	
	FT5X16_REG_FIRMID						= 0xa6,   /* Firmware version */
	FT5X16_REG_STATE						= 0xa7,
	FT5X16_REG_FT5201ID					= 0xa8,
	FT5X16_REG_ERR						= 0xa9,
	FT5X16_REG_CLB						= 0xaa,
};

//FT5X16_REG_PMODE
#define PMODE_ACTIVE        0x00
#define PMODE_MONITOR       0x01
#define PMODE_STANDBY       0x02
#define PMODE_HIBERNATE     0x03


#ifndef ABS_MT_TOUCH_MAJOR
#define ABS_MT_TOUCH_MAJOR	0x30	/* touching ellipse */
#define ABS_MT_TOUCH_MINOR	0x31	/* (omit if circular) */
#define ABS_MT_WIDTH_MAJOR	0x32	/* approaching ellipse */
#define ABS_MT_WIDTH_MINOR	0x33	/* (omit if circular) */
#define ABS_MT_ORIENTATION	0x34	/* Ellipse orientation */
#define ABS_MT_POSITION_X	0x35	/* Center X ellipse position */
#define ABS_MT_POSITION_Y	0x36	/* Center Y ellipse position */
#define ABS_MT_TOOL_TYPE	0x37	/* Type of touching device */
#define ABS_MT_BLOB_ID		0x38	/* Group set of pkts as blob */
#endif /* ABS_MT_TOUCH_MAJOR */

#ifndef ABS_MT_TRACKING_ID
    #define ABS_MT_TRACKING_ID 0x39 /* Unique ID of initiated contact */
#endif

#endif

