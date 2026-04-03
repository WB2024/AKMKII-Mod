#ifndef DEMUX_H
#define DEMUX_H

#ifdef _WIN32
	#include "stdint_win.h"
#else
	#include <stdint.h>
#endif

#include "stream.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t fourcc_t;

typedef struct
{
    int format_read;

    uint16_t num_channels;
    uint16_t sample_size;
    uint32_t sample_rate;
    fourcc_t format;
    void *buf;

    struct time_to_sample_t {
        uint32_t sample_count;
        uint32_t sample_duration;
    } *time_to_sample;
    uint32_t num_time_to_samples;

    uint32_t *sample_byte_size;
	// 2013.07.15 iriver-eddy
	uint64_t *sample_byte_pos;
    uint32_t num_sample_byte_sizes;

    uint32_t codecdata_len;
    void *codecdata;

    uint32_t mdat_len;
    uint32_t mdat_start_pos;

#if 0
    void *mdat;
#endif
} demux_res_t;

extern ALAC_API int qtmovie_read(stream_t *stream, demux_res_t *demux_res);

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ( \
    ( (int32_t)(char)(ch0) << 24 ) | \
    ( (int32_t)(char)(ch1) << 16 ) | \
    ( (int32_t)(char)(ch2) << 8 ) | \
    ( (int32_t)(char)(ch3) ) )
#endif

#ifndef SLPITFOURCC
/* splits it into ch0, ch1, ch2, ch3 - use for printf's */
#define SPLITFOURCC(code) \
    (char)((int32_t)code >> 24), \
    (char)((int32_t)code >> 16), \
    (char)((int32_t)code >> 8), \
    (char)code
#endif

#ifdef __cplusplus
}
#endif

#endif /* DEMUX_H */

