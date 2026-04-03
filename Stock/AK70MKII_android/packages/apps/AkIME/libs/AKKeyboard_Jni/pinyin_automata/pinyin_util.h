#ifndef __PINYIN_UTIL_H__
#define __PINYIN_UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <wchar.h>

int DDE_WCSLEN(const unsigned short *s);
void usTochar(char* a, unsigned short* b);
void uscpy(unsigned short* a, unsigned short* b);

	
#ifdef __cplusplus
}
#endif

#endif