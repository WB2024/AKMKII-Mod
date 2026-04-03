#ifndef STREAM_H
#define STREAM_H

/* stream.h */

#ifdef _WIN32
	#include "stdint_win.h"
#else
	#include <stdint.h>
#endif

#include "alac_export.h"
#include <media/stagefright/DataSource.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stream_tTAG stream_t;

extern ALAC_API size_t stream_read(stream_t *stream, size_t len, void *buf);

int32_t stream_read_int32(stream_t *stream);
uint32_t stream_read_uint32(stream_t *stream);

int16_t stream_read_int16(stream_t *stream);
uint16_t stream_read_uint16(stream_t *stream);

int8_t stream_read_int8(stream_t *stream);
uint8_t stream_read_uint8(stream_t *stream);

void stream_skip(stream_t *stream, size_t skip);

extern ALAC_API int stream_eof(stream_t *stream);

long stream_tell(stream_t *stream);

extern ALAC_API long stream_getpos(stream_t *stream);

extern ALAC_API int stream_setpos(stream_t *stream, long pos);

extern ALAC_API stream_t *stream_create_file(const android::sp<android::DataSource> &dataSource,
                             int bigendian);
extern ALAC_API void stream_destroy(stream_t *stream);

#ifdef __cplusplus
}
#endif

#endif /* STREAM_H */

