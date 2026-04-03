#include "EngToJap.h"


/*
 * sj.. 04.03.11
 *
 *Trans_EngToJap.c 
 * 
 *To Jap.
 *:LatinToJapan()
 *Result[output]		: Hiragana only.
 *KeyIn[input]			: Alphabets
 *
 */

#define JK_KATA_A 0x30A1 
#define JK_HIRA_A 0x3041
#define JK_KH_GAP 0x60
#define _KEY_SHIFT	0x10


#define KATA_SYLLLEN		90
#define HIRA_SYLLLEN		86

#define SINGLE_VOWELN	5

#define _islower(x)			((x) >= 'a' && (x) <= 'z')

#define HANJA_MAX	256
#define DEFAULT_CNT	10

wchar KeyBuffer;


/*converting tables: Latin -> hiragana*/
/*converting tables: hira -> kata / (hira+0x60) -> kata */
static const wchar SingleVowel[][SINGLE_VOWELN + 1] = {
	{(wchar)'a', (wchar)'i', (wchar)'u', (wchar)'e', (wchar)'o', 0},
	{0x3042, 0x3044, 0x3046, 0x3048, 0x304A, 0}
};
static const wchar SmallSingleVowel[][SINGLE_VOWELN] = {
	{0x3041, 0x3043, 0x3045, 0x3047, 0x3049},	//small A
	{0x3083, 0x3043, 0x3085, 0x3047, 0x3087}	//small YA
};
static const wchar DoubleAlphaBaseChar1[][SINGLE_VOWELN + 1] = {
	{(wchar)'l', 0x3041, 0x3043, 0x3045, 0x3047, 0x3049},	//small A	
	{(wchar)'x', 0x3041, 0x3043, 0x3045, 0x3047, 0x3049},	//small A
	{(wchar)'y', 0x3084, 0x3044, 0x3086, 0x0000, 0x3088},	//YA
	{(wchar)'k', 0x304B, 0x304D, 0x304F, 0x3051, 0x3053},	//KA 
	{(wchar)'c', 0x304B, 0x0000, 0x304F, 0x0000, 0x3053},	//CA 
	{(wchar)'g', 0x304C, 0x304E, 0x3050, 0x3052, 0x3054},	//GA 
	{(wchar)'s', 0x3055, 0x3057, 0x3059, 0x305B, 0x305D},	//SA 
	{(wchar)'z', 0x3056, 0x3058, 0x305A, 0x305C, 0x305E},	//ZA 
	{(wchar)'t', 0x305F, 0x3061, 0x3064, 0x3066, 0x3068},	//TA 
	{(wchar)'d', 0x3060, 0x3062, 0x3065, 0x3067, 0x3069},	//DA 
	{(wchar)'n', 0x306A, 0x306B, 0x306C, 0x306D, 0x306E},	//NA 
	{(wchar)'h', 0x306F, 0x3072, 0x3075, 0x3078, 0x307B},	//HA 
	{(wchar)'b', 0x3070, 0x3073, 0x3076, 0x3079, 0x307C},	//BA 
	{(wchar)'p', 0x3071, 0x3074, 0x3077, 0x307A, 0x307D},	//PA 
	{(wchar)'m', 0x307E, 0x307F, 0x3080, 0x3081, 0x3082},	//MA 
	{(wchar)'r', 0x3089, 0x308A, 0x308B, 0x308C, 0x308D},	//RA 
	{0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}
};
static const wchar DoubleAlphaBaseChar2[][3] = {
	{(wchar)'j', 0x3058, 1},
	{(wchar)'q', 0x304F, 0},
	{(wchar)'f', 0x3075, 0},
	{(wchar)'v', 0x30F4, 0},
	{(wchar)'w', 0x3046, 0},
	{0, 0x0000, 0}
};
static const wchar TripleAlphaBaseChar1[][4] = {
	{(wchar)'s', (wchar)'h', 0x3057, 1},
	{(wchar)'t', (wchar)'h', 0x3066, 1},
	{(wchar)'c', (wchar)'h', 0x3061, 1},
	{(wchar)'d', (wchar)'h', 0x3067, 1},
	{(wchar)'w', (wchar)'h', 0x3046, 0},
	{(wchar)'t', (wchar)'s', 0x3064, 0},
	{0, 0, 0x0000, 0}	
};
static const wchar TripleAlphaBaseChar2[][2] = {//-ya
	{(wchar)'r', 0x308A},
	{(wchar)'m', 0x307F},
	{(wchar)'k', 0x304D},
	{(wchar)'q', 0x304F},
	{(wchar)'g', 0x304E},
	{(wchar)'s', 0x3057},
	{(wchar)'z', 0x3058},
	{(wchar)'j', 0x3058},
	{(wchar)'t', 0x3061},
	{(wchar)'c', 0x3061},
	{(wchar)'d', 0x3062},
	{(wchar)'n', 0x306B},
	{(wchar)'h', 0x3072},
	{(wchar)'f', 0x3075},
	{(wchar)'b', 0x3073},
	{(wchar)'v', 0x30F4},
	{(wchar)'p', 0x3074},
	{0, 0x0000}	
};
static const wchar TripleAlphaBaseChar3[][2] = {//-wa
	{(wchar)'q', 0x304F},
	{(wchar)'g', 0x3050},
	{(wchar)'s', 0x3059},
	{(wchar)'t', 0x3068},
	{(wchar)'d', 0x3069},
	{(wchar)'f', 0x3075},
	{0, 0x0000}
};

/*exceptions..*/
static const wchar except0[] = {
	(wchar)'y', (wchar)'e', 0x3044, 0x3047
};
static const wchar except1[][3] = {
	{(wchar)'c', (wchar)'i', 0x3057},  
	{(wchar)'c', (wchar)'e', 0x305B},  
	{(wchar)'j', (wchar)'i', 0x3058},
	{(wchar)'n', (wchar)'n', 0x3093},
	{(wchar)'x', (wchar)'n', 0x3093},
	{(wchar)'q', (wchar)'u', 0x304F},  
	{(wchar)'f', (wchar)'u', 0x3075},  
	{(wchar)'v', (wchar)'u', 0x30F4},  
	{(wchar)'w', (wchar)'a', 0x308F}, 
	{(wchar)'w', (wchar)'u', 0x3046}, 
	{(wchar)'w', (wchar)'o', 0x3092},
	{0, 0, 0x0000} 
};
static const wchar except2[][4] = {
	{(wchar)'w', (wchar)'h', (wchar)'u', 0x3046},   //whu
	{(wchar)'s', (wchar)'h', (wchar)'i', 0x3057},   //shi
	{(wchar)'c', (wchar)'h', (wchar)'i', 0x3061},   //chi
	{(wchar)'t', (wchar)'s', (wchar)'u', 0x3064},    //tsu
	{(wchar)'l', (wchar)'t', (wchar)'u', 0x3063},	//ltu
	{(wchar)'x', (wchar)'t', (wchar)'u', 0x3063},	//xtu
	{0, 0, 0, 0x0000}
};

static int InsertSmallTuNn(wchar FirstConsonant)	
{
	if(FirstConsonant == KeyBuffer)
	{
		if(FirstConsonant == (wchar)'n') KeyBuffer = 0x3093;
		else KeyBuffer = 0x3063;
		return 1;
	}
	else if(KeyBuffer == (wchar)'n')
	{
		KeyBuffer = 0x3093;
		return 1;
	}
	return 0;
}

#define INSERTSMALLTUNN(FirstConsonant) \
			 if(KeyBuffer && InsertSmallTuNn(FirstConsonant))\
			{	Result[ResultLen-1] = KeyBuffer;\
				KeyBuffer = 0;	}\

/****************************************************************************************
*
*	function name	: getSingleCompareIndex
*	description		: SingleVowel maching KeyIn
*	parameters		: 
*	return			: unsigned short* Index
*	author			: JangYunSuk
*	date			: 2013-07-12
*
*	//Index = wcschr(SingleVowel[0], KeyIn);
*****************************************************************************************/
static int getSingleCompareIndex(wchar* KeyIn)
{
	int i=0;

	for(i=0;SingleVowel[0][i];i++)
		if(SingleVowel[0][i] == KeyIn) return i;

	return -1;
}

/****************************************************************************************
*
*	function name	: wcharncmp
*	description		: wcsncmp ��Ż��
*	parameters		: 
*	return			: true & false
*	author			: JangYunSuk
*	date			: 2013-07-24
*
*****************************************************************************************/
static int wcharncmp(wchar* a, wchar* b, int len)
{
	int i=0;
	for(i=0;i<len;i++)
		if(a[i] != b[i]) return 0;

	return 1;
}

static int TrippleAlphaChar(wchar* KeyIn, wchar* Result)
{
	int i, ResultLen = wcslen(Result);
	int Index = -1;

	for(i = 0; except2[i][0]; i++)
		if(wcharncmp(except2[i], KeyIn, 3))
		{
			INSERTSMALLTUNN(*KeyIn);
			Result[ResultLen++] = except2[i][3];
			Result[ResultLen] = 0;
			return 3;
		}
		Index = getSingleCompareIndex(*(KeyIn+2)) + 1;

		if( Index > 0 )
		{
			//general
			for(i = 0; TripleAlphaBaseChar1[i][0]; i++)
				if(wcharncmp(TripleAlphaBaseChar1[i], KeyIn, 2))
				{
					INSERTSMALLTUNN(*KeyIn);
					Result[ResultLen++] = TripleAlphaBaseChar1[i][2];
					Result[ResultLen++] = SmallSingleVowel[TripleAlphaBaseChar1[i][3]][Index - 1]; //index 0���ͽ���
					Result[ResultLen] = 0;
					return 3;
				}
				//ya
				if( *(KeyIn + 1) == (wchar)'y' )
				{	
					for(i = 0; TripleAlphaBaseChar2[i][0]; i++)
						if( *KeyIn ==  TripleAlphaBaseChar2[i][0])
						{
							INSERTSMALLTUNN( *KeyIn);
							Result[ResultLen++] = TripleAlphaBaseChar2[i][1];
							Result[ResultLen++] = SmallSingleVowel[1][Index - 1]; //index 0���ͽ���
							Result[ResultLen] = 0;
							return 3;
						}
						if ( *(KeyIn) == (wchar)'l' || *(KeyIn) == (wchar)'x' )
						{
							INSERTSMALLTUNN( *KeyIn );
							Result[ResultLen++] = SmallSingleVowel[1][Index - 1]; //index 0���ͽ���
							Result[ResultLen] = 0;
							return 3;
						}			
				}
				//wa		
				if( *(KeyIn + 1) == (wchar)'w' )
				{
					for(i = 0; TripleAlphaBaseChar3[i][0]; i++)
						if( *KeyIn ==  TripleAlphaBaseChar3[i][0])
						{
							INSERTSMALLTUNN( *KeyIn );
							Result[ResultLen++] = TripleAlphaBaseChar3[i][1];
							Result[ResultLen++] = SmallSingleVowel[0][Index - 1]; //index 0���ͽ���
							Result[ResultLen] = 0;
							return 3;
						}
				}
		}
		return 0;		
}


static int DoubleAlphaChar(wchar* KeyIn, wchar* Result)
{
	int i, ResultLen = wcslen(Result);
	int Index;

	// normal
	if(wcharncmp(except0, KeyIn, 2))
	{
		INSERTSMALLTUNN(*KeyIn);
		Result[ResultLen++] = except0[2];
		Result[ResultLen++] = except0[3];
		Result[ResultLen] = 0;
		return 2;
	}	

	// except
	for(i = 0; except1[i][0]; i++){
		if(wcharncmp(except1[i], KeyIn, 2))
		{
			INSERTSMALLTUNN(*KeyIn);
			Result[ResultLen++] = except1[i][2];
			Result[ResultLen] = 0;
			return 2;
		}
	}

	// except1 !
	if(KeyIn[0] == KeyIn[1]){ // ex) 'cc' = 0x3063
		Result[0] = 0x3063;
		Result[1] = 0;
	}

	Index = getSingleCompareIndex(*(KeyIn+1)) + 1;

	if(Index > 0)
	{
		for(i = 0; DoubleAlphaBaseChar1[i][0]; i++)
		{
			if( *KeyIn ==  DoubleAlphaBaseChar1[i][0])
			{
				INSERTSMALLTUNN(*KeyIn);
				Result[ResultLen++] = DoubleAlphaBaseChar1[i][Index];
				Result[ResultLen] = 0;
				return 2;
			}
		}
		for(i = 0; DoubleAlphaBaseChar2[i][0]; i++)
		{
			if( *KeyIn ==  DoubleAlphaBaseChar2[i][0])
			{
				INSERTSMALLTUNN(*KeyIn);
				Result[ResultLen++] = DoubleAlphaBaseChar2[i][1];
				Result[ResultLen++] = SmallSingleVowel[DoubleAlphaBaseChar2[i][2]][Index-1]; //single�� 0���ͽ���
				Result[ResultLen] = 0;
				return 2;
			}
		}
	}	 
		return 0;		
}

static int SingleAlphaChar(wchar* KeyIn, wchar* Result)
{
	int ResultLen = wcslen(Result);
	int Index = -1;

	Index = getSingleCompareIndex(*KeyIn);

	if(Index > -1)
	{
		Result[ResultLen++] = SingleVowel[1][Index];
		Result[ResultLen] = 0;

		return 1;
	}

	return 0;
}  


static int Inner_LatinToJapan_CheckKey(wchar *Result, wchar *KeyIn)
{
	// first + 1, + 2, + 3 ...
	int KeyInPosBuf = 0;

	KeyInPosBuf = SingleAlphaChar(KeyIn, Result);

	if(KeyInPosBuf == 0 && KeyIn[1])
	{
		KeyInPosBuf = DoubleAlphaChar(KeyIn, Result);

		if(KeyInPosBuf == 0 && KeyIn[2])
		{
			KeyInPosBuf = TrippleAlphaChar(KeyIn, Result);
//			printf("result : %c%c%c", Result[0], Result[1], Result[2]);
		}
	}

//	if(KeyInPosBuf) memset(KeyIn, 0, DDE_WCSLEN(KeyIn));
	return KeyInPosBuf;
}
static void Inner_LatinToJapan_ReCheckKey(wchar *Result, wchar *KeyIn)
{
	// last -2, -3 ...
	// 2st Test
	int i=0;
	int InLen = com_wcslen(KeyIn);
	// exception
	if(InLen > 2)
	{
		if(TrippleAlphaChar((KeyIn + (InLen - 3)), Result) == 0){
			if(DoubleAlphaChar((KeyIn + (InLen - 2)), Result) != 0){
				for(i=InLen-2;i<InLen;i++){
					KeyIn[i - (InLen-2)] = KeyIn[i];
					KeyIn[i] = 0;
				}
			}
		}else{
			for(i=InLen-3;i<InLen;i++){
				KeyIn[i - (InLen-3)] = KeyIn[i];
				KeyIn[i] = 0;
			}
		}
	}
}

static void Inner_LatinToJapan(wchar *Result, wchar *KeyIn)
{
	int i=0;
	int InLen = wcslen(KeyIn), KeyInPos = 0, ResultLen ;

	if(!InLen)	return;

	*Result = 0;
	KeyBuffer = 0;

	if(KeyIn[0])
	{
		if(Inner_LatinToJapan_CheckKey(Result, KeyIn) == 0)
		{
			Inner_LatinToJapan_ReCheckKey(Result, KeyIn);
		}
	}

//	ResultLen = wcslen(Result);// : ��ȯ���� ���� ���ĺ� ���� �ֵ��� �ϱ� ����.
	ResultLen = com_wcslen(Result);
//	Result[ResultLen++] = KeyBuffer = KeyIn[KeyInPos++];
	KeyBuffer = KeyIn[KeyInPos++];
	Result[ResultLen] = 0;
}

static void LatinToJapan(wchar *Result, wchar *KeyIn)
{
	Inner_LatinToJapan(Result, KeyIn);
//	unsigned int SkipLen, KeyInLen, i;
	
/*
	if(_islower(Result[0])) // a~z
	{
		SkipLen = 0;
		KeyInLen = wcslen(KeyIn);
		if(KeyInLen < 2) {
			Result[0] = 0;
			return;
		}
		for(i = 0; i < 5; i++)
		{
			KeyIn[KeyInLen] = SingleVowel[0][i];
			KeyIn[KeyInLen + 1] = 0;
			Inner_LatinToJapan(Result, KeyIn);

			if(_islower(Result[0]))
			{
				SkipLen |= 1;
				if(_islower(Result[1])) SkipLen |= 2;
			}			
		}
		KeyIn[KeyInLen] = 0;
		if(SkipLen) wcscpy(KeyIn, &KeyIn[((SkipLen == 1)?1:2)]);
		Result[0] = 0;
	}
	else
		KeyIn[0] = 0;
*/

}

int hira_to_kata(wchar* hira, wchar* kata)
{
	int i=0;	
	int cnt=com_wcslen(hira);
	
	if(cnt)
	{
		for(i=0;i<cnt;i++)
			if(hira[i])	kata[i] = hira[i] + 0x60;

		return 1;
	}
	return 0;
}

int romanji_to_hiragana(wchar *keys, wchar *res)
{
	LatinToJapan(res, keys);
	if (res[0] == 0)
		return 0;
	else
		return 1;
}
	

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
void JavaToWSZ(JNIEnv* env, jstring string, wchar* wsz)
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
*	function name	: getPageNum
*	description		: get the current page num
*	parameters		: 
*	return			: 
*	author			: jang
*	date			: 2012-05-13
*
*****************************************************************************************/
jobjectArray Java_com_iriver_ak_automata_jniJapAutomata_jniEngToJapanAutomata(JNIEnv* env, jobject thiz, int inputType, jstring jBuf1, jstring jBuf2)
{	
	jclass stringClass = (*env)->FindClass(env, "java/lang/String");
	jobjectArray resultArray;

	int isResult = 0;

	wchar input[DEFAULT_CNT] = {0}; // input
//	wchar input2[DEFAULT_CNT] = {0}; // result Input
	wchar hiraBuf[DEFAULT_CNT] = {0}; // output
	wchar kataBuf[DEFAULT_CNT] = {0}; // output
//	wchar kataTohanjaBuf[DEFAULT_CNT] = {0}; // output
//	wchar hanjaBuf[HANJA_MAX] = {0}; // hanja output [max=101]

	JavaToWSZ(env, jBuf1, input);	
//	JavaToWSZ(env, jBuf2, input2);

	// make japan	
	isResult = romanji_to_hiragana(input, hiraBuf);
	if(isResult)
		isResult = hira_to_kata(hiraBuf, kataBuf);
	else
		memset(hiraBuf, 0, DDE_WCSLEN(hiraBuf));

	if(isResult){
//		if(wcslen(input2) > 0)
//		{
//			wcscpy(kataTohanjaBuf, input2);
//			wcscat(kataTohanjaBuf, kataBuf);
//			isResult = japan_to_hanja(kataTohanjaBuf, hanjaBuf);
//		}
//		else
//		{
//			isResult = japan_to_hanja(kataBuf, hanjaBuf);
//		}
	}else{
		memset(kataBuf, 0, DDE_WCSLEN(kataBuf));
	}
//	isResult = japan_to_hanja(kataBuf, hanjaBuf);

//	if(isResult){
//		printf("makeJapan Success ");
//	}else{
//		memset(hanjaBuf, 0, DDE_WCSLEN(hanjaBuf));
//		printf("makeJapan Fail");
//	}
	// return the result
	resultArray = (*env)->NewObjectArray(env, 3, stringClass, 0);
	(*env)->SetObjectArrayElement(env, resultArray, 0, (*env)->NewString(env, input, DDE_WCSLEN(input)));
	(*env)->SetObjectArrayElement(env, resultArray, 1, (*env)->NewString(env, hiraBuf, DDE_WCSLEN(hiraBuf)));
	(*env)->SetObjectArrayElement(env, resultArray, 2, (*env)->NewString(env, kataBuf, DDE_WCSLEN(kataBuf)));
//	(*env)->SetObjectArrayElement(env, resultArray, 3, (*env)->NewString(env, hanjaBuf, DDE_WCSLEN(hanjaBuf)));

	return resultArray;
}


int DDE_WCSLEN(const unsigned short *s)
{
	const unsigned short *save;

	if (s == 0)
		return 0;
	for (save = s; *save; ++save);
	return save-s;
}


int convert_utf8_to_unicode(wchar *unicode_string, void *utf, unsigned int i)
{
	unsigned int t;

	unsigned char *p, *q;

	unsigned char k;

	unsigned char * uni = (unsigned char *)unicode_string;

	p = (unsigned char*)utf;
	q = uni;

	for( t = 0; t < i; ) {
		if( (*p & 0x80) == 0x00 ) {
			*q = *p;
			*(q + 1) = 0x00;
			p++;
			t += 1;
		}
		else if( ((*p & 0xC0) == 0xC0) && ((*p & 0xE0) != 0xE0)) {
			*q = (*p & 0x1C) >> 2;
			*(q + 1) = (*p & 0x03) << 6;
			*(q + 1) = *(q + 1) | (*(p + 1) & 0x3F);

			k = *(q + 1);
			*(q + 1) = *q;
			*q = k;
			//

			p += 2;
			t += 2;
		}
		else if( (*p & 0xE0) == 0xE0 ) {
			*q = *p << 4;
			*q = *q | ((*(p + 1) & 0x3C) >> 2);

			*(q + 1) = (*(p + 1) & 0x03) << 6;
			*(q + 1) = *(q + 1) | (*(p + 2) & 0x3F);

			k = *(q + 1);
			*(q + 1) = *q;
			*q = k;
			//

			p += 3;
			t += 3;
		}
		else {
		}
		q += 2;

		if((unsigned int)(q-uni) > DRAW_STRING_MAX_BYTE) break;
	}

	return q-uni;
}
	
int com_wcslen (const wchar *s)
{
	int i; 
	for(i=0;s[i];i++); 
	return i;
}

