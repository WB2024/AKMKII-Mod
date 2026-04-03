#ifndef __PINYIN_H__
#define __PINYIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <jni.h>
#include <wchar.h>
#include <android/log.h> 

//#include "pinyin_util.h"
#include "pinyin_gan.h"
#include "pinyin_bun.h"

#define printf(fmt,args...)  __android_log_print(ANDROID_LOG_DEBUG  ,"jdebug", fmt, ##args) 

#define MAX_PINYIN	200
//	int DDE_WCSLEN(const unsigned short *s);

void JavaToWSZ(JNIEnv* env, jstring string, unsigned short* wsz);

	// º¯È¯±â 
//int _pinyin_to_hanzi(unsigned short *_pinyin_key_buffer, unsigned short *hanzi);

#ifdef __cplusplus
}
#endif

#endif

