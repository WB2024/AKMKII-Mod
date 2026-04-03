#ifndef __JAP_TO_HANJA_H__
#define __JAP_TO_HANJA_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned short wchar;

int japan_to_hanja(wchar *japan_key_buffer, wchar *hanzi);

#ifdef __cplusplus
}
#endif

#endif