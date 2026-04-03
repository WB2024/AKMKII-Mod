#ifndef __ENG_TO_JAP_H__
#define __ENG_TO_JAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <jni.h>
#include <wchar.h>
#include <android/log.h> 
#include "JapToHanja.h"	// hanja table

#define printf(fmt,args...)  __android_log_print(ANDROID_LOG_DEBUG  ,"jdebug", fmt, ##args) 



#define DRAW_STRING_MAX_BYTE	1024		// flow_draw_string 할 수 있는 최대크기(byte단위)

/* 설명 romanji 스트링을 HIRAGANA 로 변환한다.
 * 인자 :
 * [in] romanjis : 변환할 영어 문자열
 * [out] hira : 변환된 히라가나
 * 반환값:
 * 해당 히라가나가 있을 때 : FLOW_TURE, hira에 히라가나 Unicode 값 (1자 이상일경우도 있음)
 * 해당 히라가나가 없을 때 : FLOW_FALSE
 *
 */
int romanji_to_hiragana(wchar *romanjis, wchar *hira);
void JavaToWSZ(JNIEnv* env, jstring string, wchar* wsz);
int DDE_WCSLEN(const unsigned short *s);
int convert_utf8_to_unicode(wchar *unicode_string, void *utf, unsigned int i);
int com_wcslen (const wchar *s);

#ifdef __cplusplus
}
#endif

#endif
