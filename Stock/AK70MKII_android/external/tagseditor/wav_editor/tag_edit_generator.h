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

#ifndef TAG__GENERATOR_H
#define TAG__GENERATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#define _WAV_GENERATOR_TEST_SAMPLE_CODE_

typedef enum {
    TAG_GENERATOR_STATUS_GENERAL_ERROR = 0,
    TAG_GENERATOR_STATUS_OK = 1,
    TAG_GENERATOR_STATUS_INIT_ERROR = 2,
    TAG_GENERATOR_STATUS_ADD_META_ERROR = 3,
    TAG_GENERATOR_STATUS_WRITE_META_ERROR = 4,
    TAG_GENERATOR_STATUS_ALBUMART_MAKE_ERROR = 5,
    TAG_GENERATOR_STATUS_FOURCE_MAKE_ERROR = 6,
    TAG_GENERATOR_STATUS_ID3_MAKE_ERROR = 7,
    TAG_GENERATOR_STATUS_ADD_ALBUMART_ERROR = 8,
    //End of Error
    TAG_GENERATOR_STATUS_FINISH_ERROR = 9,
} TAG_GENERATOR_STATUS;

typedef enum {
    WAV_METADATA_ALBUM = 0,
    WAV_METADATA_ARTIST = 1,
    WAV_METADATA_ALBUMARTIST = 2,
    WAV_METADATA_COMPOSER = 3,
    WAV_METADATA_GENRE = 4,
    WAV_METADATA_TITLE = 5,
    WAV_METADATA_YEAR = 6,
    WAV_METADATA_TRACKNUM = 7,
    WAV_METADATA_ALBUMART_DATA = 8,
    WAV_METADATA_GRACENOTE_UIDOWNER = 9,
    WAV_METADATA_GRACENOTE_TAGID = 10,
	WAV_METADATA_COMMENTS = 11,
    //Add here
    //Must be located in the end of WAV_METADATA_INFO
    WAV_METADATA_COPYRIGHT = 12,
} WAV_METADATA_INFO;

typedef enum {
    WAV_METADATA_FILEPATH  = 1,
    WAV_METADATA_FILEPOINT = 2,
    WAV_METADATA_FILEDESC  = 3,
} WAV_SOURCE_FROM;

extern TAG_GENERATOR_STATUS wav_editor_tag_generator_gen_init(
        const int fileFd,
        const unsigned char *inWavfilename, 
        const void *file, 
        const unsigned long long offset,
        const WAV_SOURCE_FROM mode);
extern TAG_GENERATOR_STATUS wav_editor_tag_generator_gen_add_meta(
        const WAV_METADATA_INFO info, 
        const unsigned char *field_value);
extern TAG_GENERATOR_STATUS wav_editor_tag_generator_gen_meta_write();
extern TAG_GENERATOR_STATUS wav_editor_tag_generator_gen_add_albumart(
        const WAV_METADATA_INFO info, 
        const unsigned char *buffer, 
        const unsigned long length);
extern TAG_GENERATOR_STATUS wav_editor_tag_generator_gen_finish();

#ifdef _WAV_GENERATOR_TEST_SAMPLE_CODE_
extern void wav_editor_tag_tagGenerator_TEST();
#endif

#ifdef __cplusplus
}
#endif

#endif

