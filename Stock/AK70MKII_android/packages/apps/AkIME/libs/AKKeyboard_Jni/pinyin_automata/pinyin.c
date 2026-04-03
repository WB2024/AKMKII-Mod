#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pinyin.h"

/****************************************************************************************
*
*	function name	: JavaToWSZ
*	description		: Convert the jstring to unsigned short
*	parameters		: 
*	return			: 
*	author			: JangJunHyeok
*	date			: 2010-09-14
*
*****************************************************************************************/
void JavaToWSZ(JNIEnv* env, jstring string, unsigned short* wsz)
{
	if (string == NULL)
		return;

	int len = (*env)->GetStringLength(env, string);
	const jchar* raw = (*env)->GetStringChars(env, string, NULL);
	if (raw == NULL)
		return;

	memcpy(wsz, raw, len*2);
	wsz[len] = 0;

	(*env)->ReleaseStringChars(env, string, raw);
}

/****************************************************************************************
*
*	function name	: jniPinyinAutomata
*	description		: get Pinyin Word
*	parameters		: 
*	return			: 
*	author			: jang
*	date			: 2012-07-22
*
*****************************************************************************************/
jobjectArray Java_com_iriver_ak_automata_jniPinAutomata_jniPinyinAutomata(JNIEnv* env, jobject thiz, int inputType, jstring jBuf1)
{	
	jclass stringClass = (*env)->FindClass(env, "java/lang/String");
	jobjectArray resultArray;
	
	unsigned short buf1[10] = {0};
	unsigned short buf_gan[MAX_PINYIN] = {0};
	unsigned short buf_bun[MAX_PINYIN] = {0};

	int isResult = 0;

	JavaToWSZ(env, jBuf1, buf1);

	// make
	isResult = _pinyin_to_hanzi_gan(buf1, buf_gan);
	isResult = _pinyin_to_hanzi_bun(buf1, buf_bun);

	if(isResult)
		memset(buf1, 0, DDE_WCSLEN(buf1));
//	else
//		printf("makeChina Fail");

	// return the result
	resultArray = (*env)->NewObjectArray(env, 3, stringClass, 0);	
	(*env)->SetObjectArrayElement(env, resultArray, 0, (*env)->NewString(env, buf1, DDE_WCSLEN(buf1)));
	(*env)->SetObjectArrayElement(env, resultArray, 1, (*env)->NewString(env, buf_gan, DDE_WCSLEN(buf_gan)));
	(*env)->SetObjectArrayElement(env, resultArray, 2, (*env)->NewString(env, buf_bun, DDE_WCSLEN(buf_bun)));

//	printf("watch Input = 0x%x, bShift = %d, buf1 = 0x%x, buf2 = 0x%x buf2 len = %d", Input, bShift, *buf1, *buf2, DDE_WCSLEN(buf2));

	return resultArray;
}

