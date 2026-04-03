/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _flac_encoder_h_
#define _flac_encoder_h_

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FLAC_ENCODER_STATUS_ERROR_GENERAL = 0,
    FLAC_ENCODER_STATUS_OK = 1,
    FLAC_ENCODER_STATUS_ERROR_FILE = 2,
    FLAC_ENCODER_STATUS_ERROR_OPEN = 3,
    FLAC_ENCODER_STATUS_ERROR_HEADER = 4,
    FLAC_ENCODER_STATUS_ERROR_META = 5,
    FLAC_ENCODER_STATUS_ERROR_ENCODE = 6,
} FLAC_ENCODER_STATUS;

typedef enum {
    FLAC_METADATA_ALBUM = 0,
    FLAC_METADATA_ARTIST = 1,
    FLAC_METADATA_ALBUMARTIST = 2,
    FLAC_METADATA_COMPOSER = 3,
    FLAC_METADATA_GENRE = 4,
    FLAC_METADATA_TITLE = 5,
    FLAC_METADATA_YEAR = 6,
    FLAC_METADATA_TRACKNUM = 7,
    FLAC_METADATA_ALBUMART_PATH = 8,
    FLAC_METADATA_ALBUMART_DATA = 9,
    FLAC_METADATA_GREACENOTE_UIDOWNER = 10,
    FLAC_METADATA_GREACENOTE_TAGID = 11,
    FLAC_METADATA_COPYRIGHT = 12,
	// /////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 2018.07.18 iriver-eddy #MQA_DECODE
#if defined(_SUPPORT_MQA_DECODE_)
    FLAC_METADATA_ENCODER = 13,
	FLAC_METADATA_ORIGINAL_SAMPLERATE = 14,
#endif // #if defined(_SUPPORT_MQA_DECODE_)
	// /////////////////////////////////////////////////////////////////////////////////////////////////////////
} FLAC_METADATA_INFO;

extern FLAC_ENCODER_STATUS wav_encodeTo_flac_Open();
extern FLAC_ENCODER_STATUS wav_encodeTo_flac_Create_Header(unsigned long total_samples,
        unsigned int channels,
        unsigned int sample_rate,
        unsigned int bit_depth,
        unsigned int compress_level,
        unsigned int uncompressed);
extern FLAC_ENCODER_STATUS wav_encodeTo_flac_add_AlbumArt(FLAC_METADATA_INFO info, 
        const unsigned char *imageFile, 
        const unsigned char *buffer,
        const unsigned long length, 
        const unsigned int type);
extern FLAC_ENCODER_STATUS wav_encodeTo_flac_add_meta(FLAC_METADATA_INFO info, const char *infoData);
extern FLAC_ENCODER_STATUS wav_encodeTo_flac_finish_meta();
extern FLAC_ENCODER_STATUS wav_encodeTo_flac_init_meta();    
extern FLAC_ENCODER_STATUS wav_encodeTo_flac_Create_meta();
extern FLAC_ENCODER_STATUS wav_encodeTo_flac_EncodeBufferInit(const char *outFlacMakeFile);
extern FLAC_ENCODER_STATUS wav_encodeTo_flac_EncodeBuffer(unsigned char *buffer, unsigned long length);
extern FLAC_ENCODER_STATUS wav_encodeTo_flac_Close();
    
#ifdef __cplusplus
}
#endif
#endif