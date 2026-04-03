#ifndef __ALAC__DECOMP_H
#define __ALAC__DECOMP_H

#include "alac_export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alac_file alac_file;

extern ALAC_API alac_file *create_alac(int samplesize, int numchannels);
extern ALAC_API void alac_destroy(alac_file *alac);
extern ALAC_API void decode_frame(alac_file *alac,
                  unsigned char *inbuffer,
                  void *outbuffer, int *outputsize);
extern ALAC_API void alac_set_info(alac_file *alac, char *inputbuffer);

#ifdef __cplusplus
}
#endif

#endif /* __ALAC__DECOMP_H */

