/*
 * ALAC (Apple Lossless Audio Codec) decoder
 * Copyright (c) 2005 David Hammerton
 * All rights reserved.
 *
 * Basic stream reading
 *
 * http://crazney.net/programs/itunes/alac.html
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifdef _WIN32
	#include "stdint_win.h"
#else
	#include <stdint.h>
#endif

// 2014.02.28 iriver-eddy
// #define LOG_NDEBUG 0
#define LOG_TAG "MPEG4Extractor"
#include <utils/Log.h>

#include "stream.h"

#define _Swap32(v) do { \
                   v = (((v) & 0x000000FF) << 0x18) | \
                       (((v) & 0x0000FF00) << 0x08) | \
                       (((v) & 0x00FF0000) >> 0x08) | \
                       (((v) & 0xFF000000) >> 0x18); } while(0)

#define _Swap16(v) do { \
                   v = (((v) & 0x00FF) << 0x08) | \
                       (((v) & 0xFF00) >> 0x08); } while (0)

extern int host_bigendian;

struct stream_tTAG {
	////////////////////////////////
	// 2014.02.28 iriver-eddy
	android::sp<android::DataSource> dataSource;
	off64_t curOffset;
	////////////////////////////////    
    int bigendian;
    int eof;
};

//////////////////////////////////////////////////////////////////////////////////
// 2014.02.28 iriver-eddy
int alac_fseek(stream_t *stream, long offset, int whence) {
	if(stream->dataSource == NULL) {
		ALOGE("### alac_fseek() / dataSource is NULL ###");
		return -1;
	}
	
	off64_t size;
	stream->dataSource->getSize(&size);
	
	if(whence == SEEK_SET) {
		if(size < offset) {
			ALOGE("### alac_fseek() Error / SEEK_SET ###");
			return -1;
		}
		stream->curOffset = offset;
		return 0;
	} else if(whence == SEEK_CUR) {
		if(size < stream->curOffset + offset) {
			ALOGE("### alac_fseek() Error / SEEK_SET ###");
			return -1;
		}
		stream->curOffset += offset; 
		return 0;
	} else if(whence == SEEK_END) {
		if(offset > size) {
			ALOGE("### alac_fseek() Error / SEEK_END ###");
			return -1;
		}
		stream->curOffset = size - offset;
		return 0;
	}
	
	return -1;
}

ssize_t alac_fread(void *ptr, size_t size, size_t nmemb, stream_t *stream) {
	if(stream->dataSource == NULL) {
		ALOGE("### alac_fread() / dataSource is NULL ###");
		return 0;
	}
	
	ssize_t read_size = stream->dataSource->readAt(stream->curOffset, ptr, (size * nmemb));
	if(read_size < 0) {
		ALOGE("### alac_fread() / Error readAt() ###");
		return read_size;
	}

	// 2014.03.10 iriver-eddy	
/*	
	if((size * nmemb) != read_size) {
		ALOGE("### alac_fread() / Error / read size %d / request size %d ###", read_size, (size * nmemb));
		stream->curOffset += read_size; 
		return (ssize_t)(read_size / size);
	}

	stream->curOffset += read_size; 
	return nmemb;
*/

	stream->curOffset += read_size; 
	return (ssize_t)(read_size / size);
}

int alac_fgetc(stream_t *stream) {
	uint8_t data;
	if(alac_fread(&data, 1, 1, stream) == 1) {
		stream->curOffset += 1;
		return (int)data;
	}
	
	return EOF;
}

long alac_ftell(stream_t *stream) {
	return stream->curOffset;
}

///////////////////////////////////////////////////////////////////////////////////

size_t stream_read(stream_t *stream, size_t size, void *buf)
{
    size_t ret;

	// 2014.02.28 iriver-eddy
//    ret = fread(buf, 4, size >> 2, stream->f) * 4;
//    ret += fread((char*)buf + ret, 1, size - ret, stream->f);
    ret = alac_fread(buf, 4, size >> 2, stream) * 4;
    ret += alac_fread((char*)buf + ret, 1, size - ret, stream);

    if (ret == 0 && size != 0) 
		stream->eof = 1;

	return ret;
}

int32_t stream_read_int32(stream_t *stream)
{
    int32_t v;
    stream_read(stream, 4, &v);
    if ((stream->bigendian && !host_bigendian) ||
            (!stream->bigendian && host_bigendian))
    {
        _Swap32(v);
    }
    return v;
}

uint32_t stream_read_uint32(stream_t *stream)
{
    uint32_t v;
    stream_read(stream, 4, &v);
    if ((stream->bigendian && !host_bigendian) ||
            (!stream->bigendian && host_bigendian))
    {
        _Swap32(v);
    }
    return v;
}

int16_t stream_read_int16(stream_t *stream)
{
    int16_t v;
    stream_read(stream, 2, &v);
    if ((stream->bigendian && !host_bigendian) ||
            (!stream->bigendian && host_bigendian))
    {
        _Swap16(v);
    }
    return v;
}

uint16_t stream_read_uint16(stream_t *stream)
{
    uint16_t v;
    stream_read(stream, 2, &v);
    if ((stream->bigendian && !host_bigendian) ||
            (!stream->bigendian && host_bigendian))
    {
        _Swap16(v);
    }
    return v;
}

int8_t stream_read_int8(stream_t *stream)
{
    int8_t v;
    stream_read(stream, 1, &v);
    return v;
}

uint8_t stream_read_uint8(stream_t *stream)
{
    uint8_t v;
    stream_read(stream, 1, &v);
    return v;
}

void stream_skip(stream_t *stream, size_t skip)
{
	// 2014.02.28 iriver-eddy
//    if (fseek(stream->f, (long)skip, SEEK_CUR) == 0) return;
	if (alac_fseek(stream, (long)skip, SEEK_CUR) == 0) return;
    if (errno == ESPIPE)
    {
        char *buffer = (char *)malloc(skip);
        stream_read(stream, skip, buffer);
        free(buffer);
    }
}

int stream_eof(stream_t *stream)
{
    return stream->eof;
}

long stream_tell(stream_t *stream)
{
	// 2014.02.28 iriver-eddy
//    return ftell(stream->f); /* returns -1 on error */
    return alac_ftell(stream); /* returns -1 on error */
}

long stream_getpos(stream_t *stream)
{
	return stream->curOffset;
}

int stream_setpos(stream_t *stream, long pos)
{
	// 2014.02.28 iriver-eddy
//    return fseek(stream->f, pos, SEEK_SET);
	return alac_fseek(stream, pos, SEEK_SET);
}

stream_t *stream_create_file(const android::sp<android::DataSource> &dataSource,
                             int bigendian)
{
    stream_t *new_stream;

    new_stream = (stream_t*)malloc(sizeof(stream_t));
    memset(new_stream, 0, sizeof(stream_t));
    
	// 2014.02.28 iriver-eddy
	new_stream->dataSource = dataSource;
    new_stream->curOffset = 0;
    new_stream->bigendian = bigendian;
    new_stream->eof = 0;

    return new_stream;
}

void stream_destroy(stream_t *stream)
{
	// 2015.05.14 iriver-eddy
	stream->dataSource = NULL;
    free(stream);
}