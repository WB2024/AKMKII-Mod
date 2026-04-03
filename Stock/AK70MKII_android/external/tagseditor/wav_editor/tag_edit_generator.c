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

// #define LOG_NDEBUG 0
#define TAG "WAVTagGenerator"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <math.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <utils/Log.h>

#include "tag_edit_generator.h"

typedef unsigned char TAG_byte;
typedef unsigned short TAG_uint16;
typedef unsigned int TAG_uint32;
typedef unsigned long TAG_ulong32;
typedef unsigned long long TAG_uint64;
typedef short TAG_int16;
typedef int TAG_int32;
typedef long TAG_long32;
typedef long long TAG_int64;
typedef int TAG_bool;

// 2016.02.23 iriver-eddy
// #define _FOURCC_NEED_ENCODING_TYPE_

#define TRUE		1
#define FALSE		0

// * 3 for Korea, Japan, China ... UNICODE 16 3bytes, 
// + 10 is padding to protect string overflow
#define METADATA_MAX_LENGTH				((128 * 3) + 10) 

#define METADATA_TAG_SIZE				4
#define METADATA_TAG_LENGTH_SIZE		4 
#define METADATA_TAG_UTF8_SIZE			3 
#define METADATA_TAG_HEADER_SIZE		12 
#define METADATA_TAG_ID3_VERSION		3
#define METADATA_TAG_ID3_IDENTFY		3
#define METADATA_TAG_ALBUMART_HEADER	100
#define METADATA_TAG_PADDING_SIZE		1

#define METADATA_TAG_LIST_BUFFER_SIZE (METADATA_MAX_LENGTH * WAV_METADATA_COPYRIGHT) + \
    ((METADATA_TAG_SIZE + METADATA_TAG_LENGTH_SIZE) * WAV_METADATA_COPYRIGHT) + \
    1 + METADATA_TAG_HEADER_SIZE
#define METADATA_TAG_ID3_BUFFER_SIZE (METADATA_MAX_LENGTH * WAV_METADATA_COPYRIGHT) + \
    ((METADATA_TAG_SIZE + METADATA_TAG_UTF8_SIZE + METADATA_TAG_LENGTH_SIZE) * \
    WAV_METADATA_COPYRIGHT) + 1 + METADATA_TAG_HEADER_SIZE + METADATA_TAG_ID3_VERSION

//$00 ? ISO-8859-1 (LATIN-1, Identical to ASCII for values smaller than 0x80).
//$01 ? UCS-2 (UTF-16 encoded Unicode with BOM), in ID3v2.2 and ID3v2.3.
//$02 ? UTF-16BE encoded Unicode without BOM, in ID3v2.4.
//$03 ? UTF-8 encoded Unicode, in ID3v2.4.    
typedef enum {
    TE_ISO8859_1 = 0x00,
    TE_UTF16 = 0x01,
    TE_UTF16BE = 0x02,
    TE_UTF8 = 0x03, //Support Now only for ID3v2.4.
} TextEncoding;

typedef enum {
    ALBUMART_FRONT_UNKNOWN = 0,
    ALBUMART_FRONT_JPEG = 1,
    ALBUMART_FRONT_PNG = 2,
} AlbumArtMimeType;

typedef struct {
    TAG_byte KeyAlbum[METADATA_MAX_LENGTH];
    TAG_byte KeyArtist[METADATA_MAX_LENGTH];
    TAG_byte KeyAlbumArtist[METADATA_MAX_LENGTH];
    TAG_byte KeyComposer[METADATA_MAX_LENGTH];
    TAG_byte KeyGenre[METADATA_MAX_LENGTH];
    TAG_byte KeyTitle[METADATA_MAX_LENGTH];
    TAG_byte KeyYear[METADATA_MAX_LENGTH];
    TAG_byte KeyCDTrackNumber[METADATA_MAX_LENGTH];
    TAG_byte KeyGraceNoteUIDOwner[METADATA_MAX_LENGTH];
    TAG_byte KeyGraceNoteTAGID[METADATA_MAX_LENGTH];
    TAG_byte KeyCopyright[METADATA_MAX_LENGTH];
	TAG_byte KeyComments[METADATA_MAX_LENGTH];
    TAG_uint16 metaNum;

    TAG_byte *AlbumArtData;
    TAG_uint32 AlbumArtLength;
    AlbumArtMimeType AlbumArtType;
    TAG_bool isHasAlbumArt;
} MetaData_Info;

typedef struct {
    TAG_uint64 offset_start;
    TAG_uint64 offset_current;
    TAG_uint32 meta_list_length;
    TAG_uint32 meta_id3_length;

    WAV_SOURCE_FROM fileOpenMode;
    
    FILE *stream;
    TAG_bool filePathOpen;

    TAG_int32 fileDescriptor;

    MetaData_Info *MetaData_info;
} TagData_Info;

static TagData_Info *gTagDataInfo = NULL;

static void wav_editor_tag_generator_gen_meta_free();
static void wav_editor_tag_uint32_little_endian_byte(TAG_uint32 val, TAG_byte *b, TAG_uint32 bytes);
static void wav_editor_tag_uint32_byte(TAG_uint32 val, TAG_byte *b, TAG_uint32 bytes);
static TAG_uint32 wav_editor_tag_utf8_length(const TAG_byte *utf8);
static TAG_bool wav_editor_tag_name_is_legal(const TAG_byte *name);
static TAG_bool wav_editor_tag_entry_value_is_legal(const TAG_byte *value, TAG_uint32 length);
static TAG_bool wav_editor_tag_entry_is_legal(const TAG_byte *entry, TAG_uint32 length);
static AlbumArtMimeType wav_editor_tag_extract_mime_type();
static TAG_uint32 wav_editor_tag_synchsafe_integer(TAG_uint32 in);
static TAG_uint32 wav_editor_tag_unsynchsafe_integer(TAG_uint32 in);
static TAG_uint32 wav_editor_tag_generator_FOURCC_add(const TAG_byte *field, 
        const TAG_byte *tag, 
        TAG_byte *buffer, 
        TAG_uint32 *offset);
static TAG_uint32 wav_editor_tag_generator_ID3_add(const TAG_byte *field, 
        const TAG_byte *tag, 
        TAG_byte *buffer, 
        TAG_uint32 *offset);
TAG_GENERATOR_STATUS wav_editor_tag_generator_FOURCC_making(TAG_byte *buffer, TAG_uint32 *readOffset);
TAG_GENERATOR_STATUS wav_editor_tag_generator_ID3_making(TAG_byte *buffer, TAG_uint32 *readOffset);
TAG_GENERATOR_STATUS wav_editor_tag_generator_ID3_AlbumArt_making(TAG_byte *buffer, TAG_uint32 *readOffset);
       
static AlbumArtMimeType wav_editor_tag_extract_mime_type() {
    ALOGV("### %s() ###", __FUNCTION__);
        
    if(gTagDataInfo->MetaData_info->AlbumArtLength >= 8 && 
        0 == memcmp(gTagDataInfo->MetaData_info->AlbumArtData, "\x89PNG\x0d\x0a\x1a\x0a", 8))
        return ALBUMART_FRONT_PNG;
    else if(gTagDataInfo->MetaData_info->AlbumArtLength >= 2 
        && 0 == memcmp(gTagDataInfo->MetaData_info->AlbumArtData, "\xff\xd8", 2))
        return ALBUMART_FRONT_JPEG;
    return ALBUMART_FRONT_UNKNOWN;
}

//Sample Code
static TAG_uint32 wav_editor_tag_unsynchsafe_integer(TAG_uint32 in) {
    TAG_uint32 a, b, c, d, x_final = 0x0; 

    /* MSB of each byte is inored */
    if (in & 0x808080)       
        in = in & 0x7f7f7f7f;    
    
    a = in & 0xff;       
    b = (in >> 8) & 0xff;       
    c = (in >> 16) & 0xff;       
    d = (in >> 24) & 0xff;   
    
    x_final = x_final | a;       
    x_final = x_final | (b << 7);       
    x_final = x_final | (c << 14);       
    x_final = x_final | (d << 21);  
    
    return x_final;
}
    
static TAG_uint32 wav_editor_tag_synchsafe_integer(TAG_uint32 in) {    
    TAG_uint32 a, b, c, d, x_final = 0x0; 

    /* only 28 bits used */
    if (in & 0xf0000000) {
        return 0;
    }
        
    a = in & 0x7f;       
    b = (in >> 7) & 0x7f;       
    c = (in >> 14) & 0x7f;       
    d = (in >> 21) & 0x7f;         

    x_final = x_final | a;       
    x_final = x_final | (b << 8);       
    x_final = x_final | (c << 16);       
    x_final = x_final | (d << 24); 
    
    return x_final;
} 


static void wav_editor_tag_uint32_byte(TAG_uint32 val, TAG_byte *b, TAG_uint32 bytes)
{
    TAG_uint32 i;

    b += bytes;

    for(i = 0; i < bytes; i++) {
        *(--b) = (TAG_byte)(val & 0xff);
        val >>= 8;
    }
}

static void wav_editor_tag_uint32_little_endian_byte(TAG_uint32 val, TAG_byte *b, TAG_uint32 bytes)
{
    TAG_uint32 i;

    for(i = 0; i < bytes; i++) {
        *(b++) = (TAG_byte)(val & 0xff);
        val >>= 8;
    }
}

static TAG_uint32 wav_editor_tag_utf8_length(const TAG_byte *utf8)
{
    //if(debug_log) ALOGV("### %s() / utf8 : %s ###", __FUNCTION__, utf8);
    
    if((utf8 == NULL) || (strlen((char*)utf8) <= 0)) {
        return FALSE;
    }    
    if ((utf8[0] & 0x80) == 0) {
        return 1;
    }
    else if ((utf8[0] & 0xE0) == 0xC0 && (utf8[1] & 0xC0) == 0x80) {
        if ((utf8[0] & 0xFE) == 0xC0) /* overlong sequence check */
            return FALSE;
        return 2;
    }
    else if ((utf8[0] & 0xF0) == 0xE0 && (utf8[1] & 0xC0) == 0x80 && 
        (utf8[2] & 0xC0) == 0x80) {
        if (utf8[0] == 0xE0 && (utf8[1] & 0xE0) == 0x80) /* overlong sequence check */
            return FALSE;
        /* illegal surrogates check (U+D800...U+DFFF and U+FFFE...U+FFFF) */
        if (utf8[0] == 0xED && (utf8[1] & 0xE0) == 0xA0) /* D800-DFFF */
            return FALSE;
        if (utf8[0] == 0xEF && utf8[1] == 0xBF && 
            (utf8[2] & 0xFE) == 0xBE) /* FFFE-FFFF */
            return FALSE;
        return 3;
    }
    else if ((utf8[0] & 0xF8) == 0xF0 && (utf8[1] & 0xC0) == 0x80 && 
        (utf8[2] & 0xC0) == 0x80 && (utf8[3] & 0xC0) == 0x80) {
        if (utf8[0] == 0xF0 && (utf8[1] & 0xF0) == 0x80) /* overlong sequence check */
            return FALSE;
        return 4;
    }
    else if ((utf8[0] & 0xFC) == 0xF8 && (utf8[1] & 0xC0) == 0x80 && 
        (utf8[2] & 0xC0) == 0x80 && (utf8[3] & 0xC0) == 0x80 && 
        (utf8[4] & 0xC0) == 0x80) {
        if (utf8[0] == 0xF8 && (utf8[1] & 0xF8) == 0x80) /* overlong sequence check */
            return FALSE;
        return 5;
    }
    else if ((utf8[0] & 0xFE) == 0xFC && (utf8[1] & 0xC0) == 0x80 && 
        (utf8[2] & 0xC0) == 0x80 && (utf8[3] & 0xC0) == 0x80 && 
        (utf8[4] & 0xC0) == 0x80 && (utf8[5] & 0xC0) == 0x80) {
        if (utf8[0] == 0xFC && (utf8[1] & 0xFC) == 0x80) /* overlong sequence check */
            return FALSE;
        return 6;
    }
    else {
        return FALSE;
    }
}

static TAG_bool wav_editor_tag_name_is_legal(const TAG_byte *name)
{
    TAG_byte c;
    for(c = *name; c; c = *(++name)) {
        if(c < 0x20 || c == 0x3d || c > 0x7d) {
            ALOGE("### %s() / FALSE ###", __FUNCTION__);
            return FALSE;
        }
    }
    return TRUE;
}

static TAG_bool wav_editor_tag_entry_value_is_legal(const TAG_byte *value, TAG_uint32 length)
{
//    ALOGV("### %s() / value : %s ###", __FUNCTION__, value);

    if(length == (TAG_uint32)(-1)) {
        while(*value) {
            TAG_uint32 n = wav_editor_tag_utf8_length(value);
            if(n == 0)
                return FALSE;
            value += n;
        }
    }
    else {
        const TAG_byte *end = value + length;
        while(value < end) {
            TAG_uint32 n = wav_editor_tag_utf8_length(value);
            if(n == 0)
                return FALSE;
            value += n;
        }
        if(value != end)
            return FALSE;
    }
    return TRUE;
}

//Not Used
static TAG_bool wav_editor_tag_entry_is_legal(const TAG_byte *entry, TAG_uint32 length)
{
    const TAG_byte *s, *end;
    
    ALOGV("### %s() / entry : %s, length : %lu ###", __FUNCTION__, entry, length);
    
    for(s = entry, end = s + length; s < end && *s != '='; s++) {
        if(*s < 0x20 || *s > 0x7D)
            return FALSE;
    }
    if(s == end)
        return FALSE;

    s++; /* skip '=' */

    while(s < end) {
        TAG_uint32 n = wav_editor_tag_utf8_length(s);
        if(n == 0)
            return FALSE;
        s += n;
    }
    if(s != end)
        return FALSE;   
    return TRUE;
}

static void wav_editor_tag_generator_gen_meta_free() {

    ALOGV("### %s() ###", __FUNCTION__);
    
    if(gTagDataInfo != NULL) {
        if(gTagDataInfo->filePathOpen && gTagDataInfo->stream != NULL) {
            fclose(gTagDataInfo->stream);
            gTagDataInfo->stream = NULL;
        }
        
        if(gTagDataInfo->MetaData_info != NULL) {
            if(gTagDataInfo->MetaData_info->AlbumArtData != NULL) {
                free(gTagDataInfo->MetaData_info->AlbumArtData);
                gTagDataInfo->MetaData_info->AlbumArtData = NULL;            
            }
            
            free(gTagDataInfo->MetaData_info);
            gTagDataInfo->MetaData_info = NULL;
        }
        
        free(gTagDataInfo);
        gTagDataInfo = NULL;
    }

    return;
}

static TAG_uint32 wav_editor_tag_generator_ID3_add(const TAG_byte *field, 
        const TAG_byte *tag, 
        TAG_byte *buffer, 
        TAG_uint32 *offset) {
    TAG_byte lBuffer[METADATA_TAG_SIZE];
    TAG_uint32 length = 0;
    TAG_byte *pBuffer = buffer;   

    length = strlen((char *)field);
    ALOGV("### %s() / field : %s / length : %d ###", __FUNCTION__, field, length);
    
    wav_editor_tag_uint32_byte(length + 1, lBuffer, METADATA_TAG_LENGTH_SIZE);
    wav_editor_tag_name_is_legal(tag);
    memcpy(pBuffer + *offset, tag, METADATA_TAG_SIZE);
    *offset += METADATA_TAG_SIZE;
    memcpy(pBuffer + *offset, lBuffer, METADATA_TAG_LENGTH_SIZE);
    *offset += METADATA_TAG_LENGTH_SIZE;
    memcpy(pBuffer + *offset, "\000\000\003", METADATA_TAG_UTF8_SIZE); //TE_UTF8 0x03
    *offset += METADATA_TAG_UTF8_SIZE;
    memcpy(pBuffer + *offset, (char *)field, length);
    *offset += length;
    
    return *offset;
}

//Important Rule (2016
//field length must set (length % 4(byte) == 0)
//Example : field length is 7bytes then Padding(0) must be added in end of field
//Fix under codes
static TAG_uint32 wav_editor_tag_generator_FOURCC_add(const TAG_byte *field, 
        const TAG_byte *tag, 
        TAG_byte *buffer, 
        TAG_uint32 *offset) {
    TAG_bool use4bytePadding = TRUE;   
    TAG_byte lBuffer[METADATA_TAG_SIZE];
    TAG_uint32 length = 0;
    TAG_byte *pBuffer = buffer;
    TAG_uint16 restBytePadding = 0;

    ALOGV("### %s() / field %s ###", __FUNCTION__, field);
    
    length = strlen((char *)field);
	
    if(length < 4) {
        restBytePadding = length;
    } else {
		restBytePadding = length % 4;
    }
    
    ALOGV("### %s() / length : %d / restBytePadding : %d ###", __FUNCTION__, length, restBytePadding);
    
    wav_editor_tag_uint32_little_endian_byte(length, lBuffer, METADATA_TAG_LENGTH_SIZE);
    wav_editor_tag_name_is_legal(tag);
    memcpy(pBuffer + *offset, tag, METADATA_TAG_SIZE);
    *offset += METADATA_TAG_SIZE;
    memcpy(pBuffer + *offset, lBuffer, METADATA_TAG_LENGTH_SIZE);
    *offset += METADATA_TAG_LENGTH_SIZE;
   	memcpy(pBuffer + *offset, (char *)field, length);
    *offset += length;
    if(restBytePadding > 0 && (use4bytePadding == TRUE)) {
        TAG_uint32 paddingLength = (4 - restBytePadding);
        switch(restBytePadding) {
			case 1: memcpy(pBuffer + *offset, (char *)"\000\000\000", paddingLength); break;
            case 2: memcpy(pBuffer + *offset, (char *)"\000\000", paddingLength); break;
            case 3: memcpy(pBuffer + *offset, (char *)"\000", paddingLength); break;
            default: 
                ALOGE("### %s() / Error! restBytePadding is overflow %d ###", __FUNCTION__, restBytePadding);
                break; 
        }
        *offset += paddingLength;
    }

    return *offset;
}

// ## Option 01
// fileFd : file descriptor for WAV_METADATA_FILEDESC 
// ## Option 02
// inWavfilename : file full path for WAV_METADATA_FILEPATH
// ## Option 03
// file : file pointer for WAV_METADATA_FILEPOINT
// offset : end offset of file pointer for WAV_METADATA_FILEPOINT
TAG_GENERATOR_STATUS wav_editor_tag_generator_gen_init(const int fileFd, 
        const unsigned char *inWavfilename, 
        const void *file, 
        const unsigned long long offset, 
        const WAV_SOURCE_FROM mode) {
    struct stat statbuf;
    FILE *stream = NULL;
    TAG_int32 type = 0;
    TAG_uint64 nFileSize = 0;
    TAG_int32 lFileFd = -1;
    
    ALOGV("### %s() / mode %d ###", __FUNCTION__, mode);

    if(inWavfilename == NULL && mode == WAV_METADATA_FILEPATH) {
        ALOGE("### %s() / ERROR! inWavfilename == NULL ###", __FUNCTION__);
        return TAG_GENERATOR_STATUS_INIT_ERROR;
    }
    
    if((file == NULL || offset <= 0) && mode == WAV_METADATA_FILEPOINT) {
        ALOGE("### %s() / ERROR! stream == NULL ###", __FUNCTION__);
        return TAG_GENERATOR_STATUS_INIT_ERROR;
    }

    if((fileFd == -1) && mode == WAV_METADATA_FILEDESC) {
        ALOGE("### %s() / ERROR! fileFd == -1 ###", __FUNCTION__);
        return TAG_GENERATOR_STATUS_INIT_ERROR;
    }

    if((fileFd != -1) && mode == WAV_METADATA_FILEDESC) {
        ALOGD("### %s() / fileFd : %d ###", __FUNCTION__, fileFd);
        lFileFd = fileFd;        
    } else if(inWavfilename && mode == WAV_METADATA_FILEPATH) {
        if (stat(inWavfilename, &statbuf) == 0) {
            if (S_ISREG(statbuf.st_mode)) {
                type = DT_REG;
            } else if (S_ISDIR(statbuf.st_mode)) {
                type = DT_DIR;
            }
        } else {
            ALOGE("### %s() / ERROR! statbuf == NULL ###", __FUNCTION__);
            return TAG_GENERATOR_STATUS_INIT_ERROR;
        }
    
        if (type == DT_DIR) {
            ALOGE("### %s() / ERROR! type == DT_DIR ###", __FUNCTION__);
            return TAG_GENERATOR_STATUS_INIT_ERROR;
        }
    
        if((stream = fopen(inWavfilename, "ab+")) == NULL) {  
            ALOGE("### %s() / ERROR! fopen is failed ###", __FUNCTION__);
            if(stream != NULL) {
                fclose(stream);
                stream = NULL;
            }
            return TAG_GENERATOR_STATUS_INIT_ERROR;
        } 
    
        nFileSize = statbuf.st_size;
    
        if(0 != (fseeko(stream, nFileSize, SEEK_SET))) {
            ALOGE("### %s() / ERROR! WAV_METADATA_FILEPATH seek is failed ###", __FUNCTION__);
            return TAG_GENERATOR_STATUS_INIT_ERROR;
        }
    } else if(mode == WAV_METADATA_FILEPOINT) {
        stream = (FILE *)file;
        nFileSize = offset;
        if(0 != (fseeko(stream, nFileSize, SEEK_SET))) {
            ALOGE("### %s() / ERROR! WAV_METADATA_FILEPOINT seek is failed ###", __FUNCTION__);
            return TAG_GENERATOR_STATUS_INIT_ERROR;
        }
    }

    wav_editor_tag_generator_gen_meta_free();
        
    if(NULL == (gTagDataInfo = (TagData_Info *) malloc(sizeof(TagData_Info) + 1))) {
        ALOGE("### %s() / ERROR! gTagDataInfo Out of Memory ###", __FUNCTION__);
        if(inWavfilename != NULL && mode == WAV_METADATA_FILEPATH) {
            if(stream != NULL) {
                fclose(stream);
                stream = NULL;
            }
        }
        return TAG_GENERATOR_STATUS_INIT_ERROR;
    }
    
    memset(gTagDataInfo, 0 , sizeof(TagData_Info) + 1);
    
    gTagDataInfo->offset_start = nFileSize;
    gTagDataInfo->offset_current = nFileSize;
    gTagDataInfo->meta_list_length = 0;
    gTagDataInfo->meta_id3_length = 0;
    gTagDataInfo->MetaData_info = NULL;  
    gTagDataInfo->filePathOpen = FALSE;
    if(inWavfilename != NULL && mode == WAV_METADATA_FILEPATH) {
        gTagDataInfo->filePathOpen = TRUE;
    }
    gTagDataInfo->stream = NULL;
    if(mode == WAV_METADATA_FILEPATH || mode == WAV_METADATA_FILEPOINT) {
        gTagDataInfo->stream = (FILE *)stream;
    }
    gTagDataInfo->fileDescriptor = -1;
    if(mode == WAV_METADATA_FILEDESC) {
        gTagDataInfo->fileDescriptor = lFileFd;
    }
    gTagDataInfo->fileOpenMode = mode;

    if(NULL == (gTagDataInfo->MetaData_info = 
            (MetaData_Info *) malloc(sizeof(MetaData_Info) + 1))) {
        ALOGE("### %s() / ERROR! MetaData_info Out of Memory ###", __FUNCTION__);
        if(inWavfilename != NULL && mode == WAV_METADATA_FILEPATH) {
            if(stream != NULL) {
                fclose(stream);
                stream = NULL;
            }
        }
        wav_editor_tag_generator_gen_meta_free();
        return TAG_GENERATOR_STATUS_INIT_ERROR;
    }   

    memset(gTagDataInfo->MetaData_info, 0 , sizeof(MetaData_Info) + 1);

    gTagDataInfo->MetaData_info->AlbumArtData = NULL;
    gTagDataInfo->MetaData_info->AlbumArtLength = 0; 
    gTagDataInfo->MetaData_info->AlbumArtType = ALBUMART_FRONT_UNKNOWN;
    gTagDataInfo->MetaData_info->metaNum = 0;
    gTagDataInfo->MetaData_info->isHasAlbumArt = FALSE;
    
    return TAG_GENERATOR_STATUS_OK;
}

TAG_GENERATOR_STATUS wav_editor_tag_generator_gen_add_meta(const WAV_METADATA_INFO info, 
        const unsigned char *field_value) {
        
    TAG_uint16 fieldLength = 0;
    TAG_bool success = TRUE;
    
    ALOGV("### %s() / info : %d ###", __FUNCTION__, info);

    if(gTagDataInfo == NULL) {        
        ALOGE("### %s() / ERROR! gTagDataInfo == NULL ###", __FUNCTION__);
        return TAG_GENERATOR_STATUS_ADD_META_ERROR;
    }
    
    if(gTagDataInfo->MetaData_info == NULL) {
        ALOGE("### %s() / ERROR! gTagDataInfo->MetaData_info == NULL ###", __FUNCTION__);
        return TAG_GENERATOR_STATUS_ADD_META_ERROR;
    }
    
    if(field_value == NULL || strlen((char *)field_value) <= 0) {
        ALOGE("### %s() / ERROR! field_value == NULL ###", __FUNCTION__);
        return TAG_GENERATOR_STATUS_ADD_META_ERROR;
    }

    ALOGV("### %s() / field_value : %s ###", __FUNCTION__, field_value);
        
    if(!wav_editor_tag_entry_value_is_legal((const TAG_byte *)field_value, (TAG_uint32)(-1))) {
        ALOGE("### %s() / ERROR! wav_editor_tag_entry_value_is_legal ###", __FUNCTION__);
        return TAG_GENERATOR_STATUS_ADD_META_ERROR;     
    }
    
    fieldLength = strlen((char *)field_value);

    if(fieldLength >= METADATA_MAX_LENGTH) {
        fieldLength = METADATA_MAX_LENGTH - 1;
    }

    switch(info) {
        case WAV_METADATA_ALBUM:
            memset(gTagDataInfo->MetaData_info->KeyAlbum, 0, METADATA_MAX_LENGTH);
            memcpy(gTagDataInfo->MetaData_info->KeyAlbum, field_value, fieldLength);
        break;
        case WAV_METADATA_ARTIST:
            memset(gTagDataInfo->MetaData_info->KeyArtist, 0, METADATA_MAX_LENGTH);
            memcpy(gTagDataInfo->MetaData_info->KeyArtist, field_value, fieldLength);
        break;
        case WAV_METADATA_ALBUMARTIST:
            memset(gTagDataInfo->MetaData_info->KeyAlbumArtist, 0, METADATA_MAX_LENGTH);
            memcpy(gTagDataInfo->MetaData_info->KeyAlbumArtist, field_value, fieldLength);
        break;
        case WAV_METADATA_COMPOSER:
            memset(gTagDataInfo->MetaData_info->KeyComposer, 0, METADATA_MAX_LENGTH);
            memcpy(gTagDataInfo->MetaData_info->KeyComposer, field_value, fieldLength); 
        break;
        case WAV_METADATA_GENRE:
            memset(gTagDataInfo->MetaData_info->KeyGenre, 0, METADATA_MAX_LENGTH);
            memcpy(gTagDataInfo->MetaData_info->KeyGenre, field_value, fieldLength); 
        break;
        case WAV_METADATA_TITLE:
            memset(gTagDataInfo->MetaData_info->KeyTitle, 0, METADATA_MAX_LENGTH);
            memcpy(gTagDataInfo->MetaData_info->KeyTitle, field_value, fieldLength); 
        break;
        case WAV_METADATA_YEAR:
            memset(gTagDataInfo->MetaData_info->KeyYear, 0, METADATA_MAX_LENGTH);
            memcpy(gTagDataInfo->MetaData_info->KeyYear, field_value, fieldLength); 
        break;
        case WAV_METADATA_TRACKNUM:
            memset(gTagDataInfo->MetaData_info->KeyCDTrackNumber, 0, METADATA_MAX_LENGTH);
            memcpy(gTagDataInfo->MetaData_info->KeyCDTrackNumber, field_value, fieldLength); 
        break;
        case WAV_METADATA_GRACENOTE_UIDOWNER:
            memset(gTagDataInfo->MetaData_info->KeyGraceNoteUIDOwner, 0, METADATA_MAX_LENGTH);
            memcpy(gTagDataInfo->MetaData_info->KeyGraceNoteUIDOwner, field_value, fieldLength); 
        break;
        case WAV_METADATA_GRACENOTE_TAGID:
            memset(gTagDataInfo->MetaData_info->KeyGraceNoteTAGID, 0, METADATA_MAX_LENGTH);
            memcpy(gTagDataInfo->MetaData_info->KeyGraceNoteTAGID, field_value, fieldLength); 
        break;
        case WAV_METADATA_COPYRIGHT:
            memset(gTagDataInfo->MetaData_info->KeyCopyright, 0, METADATA_MAX_LENGTH);
            memcpy(gTagDataInfo->MetaData_info->KeyCopyright, field_value, fieldLength);
        break;
		// 2016.02.23 iriver-eddy
        case WAV_METADATA_COMMENTS:
            memset(gTagDataInfo->MetaData_info->KeyComments, 0, METADATA_MAX_LENGTH);
            memcpy(gTagDataInfo->MetaData_info->KeyComments, field_value, fieldLength);
        break;
        default:
            success = FALSE;
        break;
    }

    if(success) {
        gTagDataInfo->MetaData_info->metaNum += 1;
        ALOGV("### %s() / info : %d , infoData : %s / field length : %u ###", __FUNCTION__, info, field_value, fieldLength);
    }
    
    return TAG_GENERATOR_STATUS_OK;
}

TAG_GENERATOR_STATUS wav_editor_tag_generator_gen_meta_write() {
    ALOGV("### %s() / METADATA_TAG_LIST_BUFFER_SIZE = %d ###", __FUNCTION__, METADATA_TAG_LIST_BUFFER_SIZE);

    TAG_byte bufferList[METADATA_TAG_LIST_BUFFER_SIZE];
    TAG_byte bufferID3[METADATA_TAG_ID3_BUFFER_SIZE];
    TAG_byte bufferAlbumArtHeader[METADATA_TAG_ALBUMART_HEADER];
    TAG_uint32 readOffset = 0;
    size_t writeCount = 0;
    TAG_long32 descCount = 0;

    if(gTagDataInfo == NULL) {        
        ALOGE("### %s() / ERROR! gTagDataInfo == NULL ###", __FUNCTION__);
        wav_editor_tag_generator_gen_meta_free();
        return TAG_GENERATOR_STATUS_WRITE_META_ERROR;
    }
    
    if(gTagDataInfo->MetaData_info == NULL) {
        ALOGE("### %s() / ERROR! gTagDataInfo->MetaData_info == NULL ###", __FUNCTION__);
        wav_editor_tag_generator_gen_meta_free();
        return TAG_GENERATOR_STATUS_WRITE_META_ERROR;
    }

    if((gTagDataInfo->MetaData_info->metaNum <= 0) && 
        (gTagDataInfo->MetaData_info->isHasAlbumArt == FALSE)) {
        ALOGE("### %s() / ERROR! meta and albumArt are NULL ###", __FUNCTION__);
        wav_editor_tag_generator_gen_meta_free();
        return TAG_GENERATOR_STATUS_WRITE_META_ERROR;
    }

    if(gTagDataInfo->fileOpenMode == WAV_METADATA_FILEPATH || 
        gTagDataInfo->fileOpenMode == WAV_METADATA_FILEPOINT) {
        if(gTagDataInfo->stream == NULL) {
            ALOGE("### %s() / ERROR! gTagDataInfo->stream = NULL ###", __FUNCTION__);
            wav_editor_tag_generator_gen_meta_free();
            return TAG_GENERATOR_STATUS_WRITE_META_ERROR;
        }
    }

    if((gTagDataInfo->MetaData_info->metaNum > 0) || 
        (gTagDataInfo->MetaData_info->isHasAlbumArt == TRUE)) {
		memset(bufferList, 0, METADATA_TAG_LIST_BUFFER_SIZE);     

#ifdef _FOURCC_NEED_ENCODING_TYPE_
        ///////////////////////////////////////////
        // LIST____INFO
        ///////////////////////////////////////////
        {
            wav_editor_tag_generator_FOURCC_making(bufferList, (TAG_uint32 *) &readOffset);
            
            if(readOffset <= 0) {
                ALOGE("### %s() / ERROR! FOURCC readOffset <= 0 ###", __FUNCTION__);
                wav_editor_tag_generator_gen_meta_free();
                return TAG_GENERATOR_STATUS_WRITE_META_ERROR;
            }
            
            //FOURCC LIST
            if(gTagDataInfo->fileOpenMode == WAV_METADATA_FILEDESC) {
                descCount = write(gTagDataInfo->fileDescriptor, bufferList, readOffset);
                if(descCount == -1) {
                    ALOGE("### %s() / ERROR! WAV_METADATA_FILEDESC FOURCC write is failed. FD : %d ###",
							__FUNCTION__,
                            gTagDataInfo->fileDescriptor);
                    wav_editor_tag_generator_gen_meta_free();
                    return TAG_GENERATOR_STATUS_WRITE_META_ERROR;
                }
				ALOGD("### %s() / descCount(%u) / readOffset(%u) ###", __FUNCTION__, descCount, readOffset);
            } else {
                writeCount = fwrite(bufferList, readOffset, 1, gTagDataInfo->stream);
                if(writeCount != 1) {
                    ALOGE("### %s() / ERROR! FOURCC write is failed ###", __FUNCTION__);
                    wav_editor_tag_generator_gen_meta_free();
                    return TAG_GENERATOR_STATUS_WRITE_META_ERROR;    
                }
            }
            gTagDataInfo->offset_current += readOffset;
        }
#endif // #ifdef _FOURCC_NEED_ENCODING_TYPE_

        ///////////////////////////////////////////
        // id3_____ID3, 
        // Encoding Version : ID3_V2_4 to support UTF-8
        ///////////////////////////////////////////
        {
            writeCount = 0;
            descCount = 0;
            readOffset = 0;
            memset(bufferID3, 0, METADATA_TAG_ID3_BUFFER_SIZE);
            wav_editor_tag_generator_ID3_making(bufferID3, (TAG_uint32 *) &readOffset);
            
            if(readOffset <= 0) {
                ALOGE("### %s() / ERROR! ID3 readOffset <= 0 ###", __FUNCTION__);
                wav_editor_tag_generator_gen_meta_free();
                return TAG_GENERATOR_STATUS_WRITE_META_ERROR;
            }
    
            //ID3 
            if(gTagDataInfo->fileOpenMode == WAV_METADATA_FILEDESC) {
                descCount = write(gTagDataInfo->fileDescriptor, bufferID3, readOffset);
                if(descCount == -1) {
                    ALOGE("### %s() / ERROR! WAV_METADATA_FILEDESC ID3 write is failed. FD : %d ###", __FUNCTION__, 
                            gTagDataInfo->fileDescriptor);
                    wav_editor_tag_generator_gen_meta_free();
                    return TAG_GENERATOR_STATUS_WRITE_META_ERROR;
                }
				ALOGD("### %s() / descCount(%u) / readOffset(%u) ###", __FUNCTION__, descCount, readOffset);
            } else {
                writeCount = fwrite(bufferID3, readOffset, 1, gTagDataInfo->stream);        
                if(writeCount != 1) {
                    ALOGE("### %s() / ERROR! ID3 write is failed ###", __FUNCTION__);
                    wav_editor_tag_generator_gen_meta_free();
                    return TAG_GENERATOR_STATUS_WRITE_META_ERROR;    
                }
            }

            gTagDataInfo->offset_current += readOffset;
  
            if(gTagDataInfo->MetaData_info->isHasAlbumArt) {
                ALOGV("### %s() / isHasAlbumArt ###", __FUNCTION__);
                
                readOffset = 0;
                writeCount = 0;
                descCount = 0;
                memset(bufferAlbumArtHeader, 0, METADATA_TAG_ALBUMART_HEADER);        
                wav_editor_tag_generator_ID3_AlbumArt_making(bufferAlbumArtHeader, &readOffset);
    
                if(readOffset <= 0) {
                    ALOGE("### %s() / ERROR! ALBUMART Header readOffset <= 0 ###", __FUNCTION__);
                    wav_editor_tag_generator_gen_meta_free();
                    return TAG_GENERATOR_STATUS_WRITE_META_ERROR;
                }
    
                //AlbumArt Header
                if(gTagDataInfo->fileOpenMode == WAV_METADATA_FILEDESC) {
                    descCount = write(gTagDataInfo->fileDescriptor, bufferAlbumArtHeader, readOffset);
                    if(descCount == -1) {
                        ALOGE("### %s() / ERROR! WAV_METADATA_FILEDESC ALBUMART Header write is failed. FD : %d", __FUNCTION__,
                                gTagDataInfo->fileDescriptor);
                        wav_editor_tag_generator_gen_meta_free();
                        return TAG_GENERATOR_STATUS_WRITE_META_ERROR;
                    }
                } else {
                    writeCount = fwrite(bufferAlbumArtHeader, readOffset, 1, gTagDataInfo->stream);        
                    if(writeCount != 1) {
                        ALOGE("### %s() / ERROR! ALBUMART Header write is failed ###", __FUNCTION__);
                        wav_editor_tag_generator_gen_meta_free();
                        return TAG_GENERATOR_STATUS_WRITE_META_ERROR;    
                    }   
                }

                gTagDataInfo->offset_current += readOffset;
    
                //AlbumArt Data
                writeCount = 0;
                descCount = 0;
                if(gTagDataInfo->fileOpenMode == WAV_METADATA_FILEDESC) {
                    descCount = write(gTagDataInfo->fileDescriptor, 
                            gTagDataInfo->MetaData_info->AlbumArtData, 
                            gTagDataInfo->MetaData_info->AlbumArtLength);
                    if(descCount == -1) {
                        ALOGE("### %s() / ERROR! WAV_METADATA_FILEDESC ALBUMART data write is failed. FD : %d", __FUNCTION__, 
                                gTagDataInfo->fileDescriptor);
                        wav_editor_tag_generator_gen_meta_free();
                        return TAG_GENERATOR_STATUS_WRITE_META_ERROR;
                    }
                } else {
                    writeCount = fwrite(gTagDataInfo->MetaData_info->AlbumArtData,
                        gTagDataInfo->MetaData_info->AlbumArtLength, 1, gTagDataInfo->stream);
                    if(writeCount != 1) {
                        ALOGE("### %s() / ERROR! ALBUMART data write is failed", __FUNCTION__);
                        wav_editor_tag_generator_gen_meta_free();
                        return TAG_GENERATOR_STATUS_WRITE_META_ERROR;    
                    }    
                }
                gTagDataInfo->offset_current += readOffset;
            }   
        }
		
		// //////////////////////////////////////////////////////////////////////////////////////////////////
		// 2016.02.24 iriver-eddy
		// Total file size change
		if(gTagDataInfo->fileOpenMode == WAV_METADATA_FILEDESC) {
			uint32_t total_length = lseek64(gTagDataInfo->fileDescriptor, 0, SEEK_END) - 8;
			lseek64(gTagDataInfo->fileDescriptor, 4, SEEK_SET);
			write(gTagDataInfo->fileDescriptor, &total_length, 4);
			ALOGD("### %s() / WAV total length = 0x%08x ###", __FUNCTION__, total_length);
		} else {
/*
			uint32_t total_length = fseeko(gTagDataInfo->stream, 0, SEEK_END) - 8;
			fseeko(gTagDataInfo->stream, 4, SEEK_SET);
			fwrite(&total_length, 4, 1, gTagDataInfo->stream); 
			ALOGD("### %s() / WAV total length = 0x%08x ###", __FUNCTION__, total_length);
*/
			// 2016.10.20 iriver-eddy [FIXME] 
			if(fseeko(gTagDataInfo->stream, 0, SEEK_END) == 0) {
				uint32_t total_length = ftello(gTagDataInfo->stream) - 8;
				fseeko(gTagDataInfo->stream, 4, SEEK_SET);
				fwrite(&total_length, 4, 1, gTagDataInfo->stream); 
				ALOGD("### %s() / WAV total length = 0x%08x ###", __FUNCTION__, total_length);
			}
		}
		// //////////////////////////////////////////////////////////////////////////////////////////////////

    } else {
        ALOGE("### %s() / ERROR! meta is NULL ###", __FUNCTION__);
        wav_editor_tag_generator_gen_meta_free();
        return TAG_GENERATOR_STATUS_WRITE_META_ERROR;
    }
    
    ALOGV("### %s() / Start Offset : %llu, End of Offset : %llu ###", __FUNCTION__, gTagDataInfo->offset_start, gTagDataInfo->offset_current);
    ALOGV("### %s() / LIST Length : %lu, ID3 Length : %lu", __FUNCTION__, gTagDataInfo->meta_list_length, gTagDataInfo->meta_id3_length);
    
    return TAG_GENERATOR_STATUS_OK;
}

TAG_GENERATOR_STATUS wav_editor_tag_generator_ID3_AlbumArt_making(TAG_byte *buffer, TAG_uint32 *readOffset) {
    TAG_uint32 tagOffset = 0;
    TAG_uint32 tagAlbumTotalSize = 0;
    TAG_byte charLengthbuffer[METADATA_TAG_SIZE];
    TAG_byte *pBuffer = buffer;

    ALOGV("### %s() ###", __FUNCTION__);
    
    if(gTagDataInfo->MetaData_info->AlbumArtType == ALBUMART_FRONT_UNKNOWN) {
        ALOGE("### %s() / ERROR! mimeType : %d", __FUNCTION__, gTagDataInfo->MetaData_info->AlbumArtType);
        wav_editor_tag_generator_gen_meta_free();
        return TAG_GENERATOR_STATUS_ALBUMART_MAKE_ERROR;
    }
    
    //<Header for 'Attached picture', ID: "APIC">
    //Text encoding      $xx
    //MIME type          <text string> $00
    //Strating Total Length
    //Picture type       $xx
    //Description        <text string according to encoding> $00 (00)
    //Picture data       <binary data>
     
    //Header
    wav_editor_tag_name_is_legal("APIC");
    memcpy(buffer, "APIC", METADATA_TAG_SIZE);
    tagOffset += (METADATA_TAG_LENGTH_SIZE * 2);
    memcpy(pBuffer + tagOffset, "\000\000", 2); //Text encoding
    tagOffset += 2;

    //Info
    if(gTagDataInfo->MetaData_info->AlbumArtType == ALBUMART_FRONT_JPEG) {
        memcpy(pBuffer + tagOffset, "\000image/jpeg\000\003\000", 14); //3//\003 Front Cover Art
        tagOffset += 14;
    } else {
        memcpy(pBuffer + tagOffset, "\000image/png\000\003\000", 13); //\003 Front Cover Art
        tagOffset += 13;
    }
    
    //Total Length for AlbumArt
    tagAlbumTotalSize = gTagDataInfo->MetaData_info->AlbumArtLength + tagOffset - 10;
       
    wav_editor_tag_uint32_byte(tagAlbumTotalSize, charLengthbuffer, METADATA_TAG_LENGTH_SIZE);
    memcpy(pBuffer + 4, charLengthbuffer, METADATA_TAG_LENGTH_SIZE);
    
    *readOffset = tagOffset;

    ALOGV("### %s() / tagOffset : %d ###", __FUNCTION__, tagOffset);
    
    return TAG_GENERATOR_STATUS_OK;
}

TAG_GENERATOR_STATUS wav_editor_tag_generator_FOURCC_making(TAG_byte *buffer, TAG_uint32 *readOffset) {
    TAG_byte charLengthbuffer[METADATA_TAG_SIZE];
    TAG_uint32 tagLength = 0;
    TAG_uint32 tagOffset = 0;
    TAG_byte *pBuffer = buffer;
    
    ALOGV("### %s() ###", __FUNCTION__);

    wav_editor_tag_name_is_legal("LIST");
    memcpy(pBuffer, "LIST", METADATA_TAG_SIZE);    
    tagOffset += (METADATA_TAG_LENGTH_SIZE * 2);
    wav_editor_tag_name_is_legal("INFO");
    memcpy(pBuffer + tagOffset, "INFO", METADATA_TAG_SIZE);  
    tagOffset += METADATA_TAG_LENGTH_SIZE;

    if((strlen((char *)gTagDataInfo->MetaData_info->KeyAlbum) > 0)) {
        wav_editor_tag_generator_FOURCC_add(gTagDataInfo->MetaData_info->KeyAlbum, 
            "TALB", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyArtist) > 0)) {       
        wav_editor_tag_generator_FOURCC_add(gTagDataInfo->MetaData_info->KeyArtist, 
            "IART", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyAlbumArtist) > 0)) {
        wav_editor_tag_generator_FOURCC_add(gTagDataInfo->MetaData_info->KeyAlbumArtist, 
            "TEP1", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyComposer) > 0)) {
        wav_editor_tag_generator_FOURCC_add(gTagDataInfo->MetaData_info->KeyComposer, 
            "TCOM", pBuffer, &tagOffset);
    }        
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyGenre) > 0)) {  
        wav_editor_tag_generator_FOURCC_add(gTagDataInfo->MetaData_info->KeyGenre, 
            "IGNR", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyTitle) > 0)) {          
        wav_editor_tag_generator_FOURCC_add(gTagDataInfo->MetaData_info->KeyTitle, 
            "INAM", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyYear) > 0)) {     
        wav_editor_tag_generator_FOURCC_add(gTagDataInfo->MetaData_info->KeyYear, 
            "ICRD", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyCDTrackNumber) > 0)) {         
        wav_editor_tag_generator_FOURCC_add(gTagDataInfo->MetaData_info->KeyCDTrackNumber, 
            "TRCK", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyGraceNoteUIDOwner) > 0)) {         
        wav_editor_tag_generator_FOURCC_add(gTagDataInfo->MetaData_info->KeyGraceNoteUIDOwner, 
            "UFID", pBuffer, &tagOffset);
    }      
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyGraceNoteTAGID) > 0)) {         
        wav_editor_tag_generator_FOURCC_add(gTagDataInfo->MetaData_info->KeyGraceNoteTAGID, 
            "GNID", pBuffer, &tagOffset);
    } 
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyCopyright) > 0)) {
        wav_editor_tag_generator_FOURCC_add(gTagDataInfo->MetaData_info->KeyCopyright, 
            "ICOP", pBuffer, &tagOffset);
    }
	// 2016.02.23 iriver-eddy
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyComments) > 0)) {         
        wav_editor_tag_generator_FOURCC_add(gTagDataInfo->MetaData_info->KeyComments, 
            "ICMT", pBuffer, &tagOffset);
    }  

    wav_editor_tag_uint32_little_endian_byte(tagOffset - (METADATA_TAG_LENGTH_SIZE * 2), 
            charLengthbuffer, METADATA_TAG_LENGTH_SIZE);
    memcpy(pBuffer + 4, charLengthbuffer, METADATA_TAG_LENGTH_SIZE);
    
    *readOffset = tagOffset; 
    gTagDataInfo->meta_list_length = tagOffset;
    
    ALOGV("### %s() / tagOffset : %d", __FUNCTION__, tagOffset);
    
    return TAG_GENERATOR_STATUS_OK;
}

TAG_GENERATOR_STATUS wav_editor_tag_generator_ID3_making(TAG_byte *buffer, TAG_uint32 *readOffset) {
    TAG_byte charLengthbuffer[METADATA_TAG_SIZE];
    TAG_uint32 tagLength = 0; //Tag Filed Length
    TAG_uint32 tagOffset = 0;
    TAG_uint32 tagTotalLength = 0;
    TAG_uint32 tagHeaderLength = 0;
    TAG_uint32 tagSafeLength = 0;
    TAG_uint32 tagSafeRealLength = 0;
    TAG_byte *pBuffer = buffer;
    
    ALOGV("### %s() ###", __FUNCTION__);

    //char id[3]; ID3
    //uint8_t version_major; 3 or 4
    //uint8_t version_minor; 0
    //uint8_t flags; 0
    //uint8_t enc_size[4]; 4bytes : synchsafe integer, http://en.wikipedia.org/wiki/Synchsafe
    
    memcpy(pBuffer, "id3\x20", METADATA_TAG_SIZE);    
    tagOffset += (METADATA_TAG_SIZE + METADATA_TAG_LENGTH_SIZE); //8bytes id30x20 = 4bytes, totalLength = 4bytes   
    memcpy(pBuffer + tagOffset, "ID3", METADATA_TAG_ID3_VERSION); //Identifier
    tagOffset += METADATA_TAG_ID3_VERSION;
    memcpy(pBuffer + tagOffset, "\004\000\000", METADATA_TAG_ID3_IDENTFY); //ID3_V2_4, First Byte = 0x04
    //memcpy(pBuffer + tagOffset, "\003\000\000", METADATA_TAG_ID3_IDENTFY); //ID3_V2_3, First Byte = 0x03
    tagOffset += METADATA_TAG_ID3_IDENTFY + 4 /* Synchsafe Length */;   

    tagHeaderLength = tagOffset;

    if((strlen((char *)gTagDataInfo->MetaData_info->KeyTitle) > 0)) {
        wav_editor_tag_generator_ID3_add(gTagDataInfo->MetaData_info->KeyTitle, 
            "TIT2", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyArtist) > 0)) {
        wav_editor_tag_generator_ID3_add(gTagDataInfo->MetaData_info->KeyArtist, 
            "TPE1", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyAlbum) > 0)) {
        wav_editor_tag_generator_ID3_add(gTagDataInfo->MetaData_info->KeyAlbum, 
            "TALB", pBuffer, &tagOffset);
    }
	if((strlen((char *)gTagDataInfo->MetaData_info->KeyAlbumArtist) > 0)) {
        wav_editor_tag_generator_ID3_add(gTagDataInfo->MetaData_info->KeyAlbumArtist, 
            "TPE2", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyCDTrackNumber) > 0)) {
        wav_editor_tag_generator_ID3_add(gTagDataInfo->MetaData_info->KeyCDTrackNumber, 
            "TRCK", pBuffer, &tagOffset);
    }
	// 2016.02.24 iriver-eddy
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyComments) > 0)) {
        wav_editor_tag_generator_ID3_add(gTagDataInfo->MetaData_info->KeyComments, 
            "COMM", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyYear) > 0)) {
        wav_editor_tag_generator_ID3_add(gTagDataInfo->MetaData_info->KeyYear, 
            "TYER", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyCopyright) > 0)) {
        wav_editor_tag_generator_ID3_add(gTagDataInfo->MetaData_info->KeyCopyright, 
            "TCOP", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyComposer) > 0)) {
        wav_editor_tag_generator_ID3_add(gTagDataInfo->MetaData_info->KeyComposer, 
            "TCOM", pBuffer, &tagOffset);
    }        
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyGenre) > 0)) {
        wav_editor_tag_generator_ID3_add(gTagDataInfo->MetaData_info->KeyGenre, 
            "TCON", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyGraceNoteUIDOwner) > 0)) {
        wav_editor_tag_generator_ID3_add(gTagDataInfo->MetaData_info->KeyGraceNoteUIDOwner, 
            "UFID", pBuffer, &tagOffset);
    }
    if((strlen((char *)gTagDataInfo->MetaData_info->KeyGraceNoteTAGID) > 0)) {
        wav_editor_tag_generator_ID3_add(gTagDataInfo->MetaData_info->KeyGraceNoteTAGID, 
            "GNID", pBuffer, &tagOffset);
    }

	tagTotalLength = tagOffset;

    //Checking Image Size (APIC Tag Header Size included)
    if(gTagDataInfo->MetaData_info->AlbumArtType == ALBUMART_FRONT_JPEG) {
        tagTotalLength += gTagDataInfo->MetaData_info->AlbumArtLength + 24 /*APIC Header Length*/;   
    } else if(gTagDataInfo->MetaData_info->AlbumArtType == ALBUMART_FRONT_PNG) {
        tagTotalLength += gTagDataInfo->MetaData_info->AlbumArtLength + 23 /*APIC Header Length*/;
    }

    if(tagTotalLength - (METADATA_TAG_SIZE + METADATA_TAG_LENGTH_SIZE) <= 0) {
        ALOGE("### %s() / ERROR! tagTotalLength = 0 ###", __FUNCTION__);
        return TAG_GENERATOR_STATUS_ID3_MAKE_ERROR;
    }

    //ID3 Total Tag Length (Except ID3 Header - 8bytes)    
    wav_editor_tag_uint32_little_endian_byte(tagTotalLength - (METADATA_TAG_SIZE + METADATA_TAG_LENGTH_SIZE), 
            charLengthbuffer, METADATA_TAG_LENGTH_SIZE);
    memcpy(pBuffer + 4, charLengthbuffer, METADATA_TAG_LENGTH_SIZE);

    //ID3 SafeSync Integer
    tagSafeLength = tagTotalLength - tagHeaderLength;
    tagSafeRealLength = wav_editor_tag_synchsafe_integer(tagSafeLength);
    if(tagSafeRealLength <= 0) {
        ALOGE("### %s() / ERROR! tagSafeRealLength = 0 ###", __FUNCTION__);
        return TAG_GENERATOR_STATUS_ID3_MAKE_ERROR;
    }
    wav_editor_tag_uint32_byte(tagSafeRealLength, charLengthbuffer, METADATA_TAG_LENGTH_SIZE);   
    memcpy(pBuffer + 14, charLengthbuffer, METADATA_TAG_LENGTH_SIZE);
    
    *readOffset = tagOffset;
    gTagDataInfo->meta_id3_length = tagOffset;

    ALOGV("### %s() / tagOffset : %d ###", __FUNCTION__, tagOffset);
    
    return TAG_GENERATOR_STATUS_OK;
}


TAG_GENERATOR_STATUS wav_editor_tag_generator_gen_add_albumart(const WAV_METADATA_INFO info, 
        const unsigned char *buffer, 
        const unsigned long length) {

    AlbumArtMimeType mimeType = ALBUMART_FRONT_UNKNOWN;
    
    ALOGV("### %s() ###", __FUNCTION__);

    if(gTagDataInfo != NULL && gTagDataInfo->MetaData_info != NULL && 
        gTagDataInfo->MetaData_info->isHasAlbumArt == TRUE) {
        ALOGE("### %s() / ERROR! isHasAlbumArt is True ###", __FUNCTION__);
        return TAG_GENERATOR_STATUS_ADD_ALBUMART_ERROR;
    }

    if(gTagDataInfo == NULL) {        
        ALOGE("### %s() / ERROR! gTagDataInfo == NULL ###", __FUNCTION__);
        return TAG_GENERATOR_STATUS_ADD_ALBUMART_ERROR;
    }
    
    if(gTagDataInfo->MetaData_info == NULL) {
        ALOGE("### %s() / ERROR! gTagDataInfo->MetaData_info == NULL ###", __FUNCTION__);
        return TAG_GENERATOR_STATUS_ADD_ALBUMART_ERROR;
    }

    if(info == WAV_METADATA_ALBUMART_DATA) {
        if((length <= 0) || (strlen((char *)buffer) <= 0)) {
            ALOGE("### %s() / ERROR! buffer == NULL ###", __FUNCTION__);
            return TAG_GENERATOR_STATUS_ADD_ALBUMART_ERROR;
        }
        
        if(NULL == (gTagDataInfo->MetaData_info->AlbumArtData = (TAG_byte *) malloc(length + 1))) {
            ALOGE("### %s() / ERROR! Malloc fail, Out of Memory! ###", __FUNCTION__);
            return TAG_GENERATOR_STATUS_ADD_ALBUMART_ERROR;
        }
        memset(gTagDataInfo->MetaData_info->AlbumArtData, 0, length + 1);
        memcpy(gTagDataInfo->MetaData_info->AlbumArtData, buffer, length);
        gTagDataInfo->MetaData_info->AlbumArtLength = (TAG_uint32) length;
        gTagDataInfo->MetaData_info->isHasAlbumArt = TRUE; 
        
        mimeType = wav_editor_tag_extract_mime_type();
        
        if(mimeType == ALBUMART_FRONT_JPEG) {
            gTagDataInfo->MetaData_info->AlbumArtType = ALBUMART_FRONT_JPEG;
        } else if(mimeType == ALBUMART_FRONT_PNG) {
            gTagDataInfo->MetaData_info->AlbumArtType = ALBUMART_FRONT_PNG;
        } else {
            ALOGE("### %s() / ERROR! Unknown Image Type! ###", __FUNCTION__);
            return TAG_GENERATOR_STATUS_ADD_ALBUMART_ERROR;
            wav_editor_tag_generator_gen_meta_free();
        }        
    }
    
    return TAG_GENERATOR_STATUS_OK;
}

TAG_GENERATOR_STATUS wav_editor_tag_generator_gen_finish() {
    ALOGV("### %s ###", __FUNCTION__);

    wav_editor_tag_generator_gen_meta_free();
    return TAG_GENERATOR_STATUS_OK;
}

#ifdef _WAV_GENERATOR_TEST_SAMPLE_CODE_
/*
void wav_editor_tag_tagGenerator_TEST() {
    ALOGV("### wav_editor_tag_tagGenerator_TEST ###");

    char inFilePointer[31] = "/storage/emulated/0/winter.wav";   
    unsigned long long nFileSize = 0;
    struct stat statbuf;
    FILE *stream = NULL;
    TAG_int32 type = 0;

    if(inFilePointer == NULL) {
        ALOGE("### %s() / ERROR: Path is invalid! ###", __FUNCTION__);
        return 0;
    }    
    if (stat(inFilePointer, &statbuf) == 0) {
        if (S_ISREG(statbuf.st_mode)) {
            type = DT_REG;
        } else if (S_ISDIR(statbuf.st_mode)) {
            type = DT_DIR;
        }
    } else {
        ALOGE("### %s() / ERROR: Path is invalid! ###", __FUNCTION__);
        return 0;
    }
    if (type == DT_DIR) {
        ALOGE("### %s() / ERROR: Path is not File ###", __FUNCTION__);
        return 0;
    }
    if((stream = fopen(inFilePointer, "ab+")) == NULL) {  
        ALOGE("### %s() / ERROR: fopen Failed ###", __FUNCTION__);
        if(stream != NULL) {
            fclose(stream);
            stream = NULL;
        }
        return 0;
    } 
    nFileSize = statbuf.st_size;
    if(0 != (fseeko(stream, nFileSize, SEEK_SET))) {
        return 0;
    }
    
    ALOGV("### %s() / open nFileSize %llu ###", __FUNCTION__, nFileSize);
    
    wav_editor_tag_generator_gen_init(-1, NULL, stream, nFileSize, WAV_METADATA_FILEPOINT);    

    struct stat srcstat;  
    unsigned long lLength = 0;
    unsigned char *lBuffer = NULL;
    if(0 == stat("/storage/emulated/0/0.jpg", &srcstat)) {
        lLength = srcstat.st_size;    
        lBuffer = (unsigned char *) malloc(lLength + 1);
        FILE *f = fopen("/storage/emulated/0/0.jpg", "rb");
        if(0 == f) {
            if(lBuffer != NULL) free(lBuffer);
        } else {                    
            if(fread(lBuffer, 1, lLength, f) == (size_t)lLength) {
                //////////////////////////////
                wav_editor_tag_generator_gen_add_albumart(WAV_METADATA_ALBUMART_DATA, (const unsigned char *)lBuffer, lLength);
                //////////////////////////////
            }
            fclose(f);
        }
    }    
    wav_editor_tag_generator_gen_meta_write();
    wav_editor_tag_generator_gen_finish();
    
    if(stream != NULL) {
        fclose(stream);
        stream = NULL;
    }
    
    //WAV_METADATA_FILEPATH
    char inFilePath[33] = "/storage/emulated/0/winter01.wav";
    wav_editor_tag_generator_gen_init(-1, inFilePath, NULL, 0, WAV_METADATA_FILEPATH);    
    wav_editor_tag_generator_gen_add_meta(WAV_METADATA_ALBUM, "AAAAAAAAAA");
    wav_editor_tag_generator_gen_add_meta(WAV_METADATA_ARTIST, "BBBBBBBBBB");
    wav_editor_tag_generator_gen_add_meta(WAV_METADATA_ALBUMARTIST, "CCCCCCCCCC");
    wav_editor_tag_generator_gen_add_meta(WAV_METADATA_COMPOSER, "DDDDDDDDDD");
    wav_editor_tag_generator_gen_add_meta(WAV_METADATA_GENRE, "ROCK");
    wav_editor_tag_generator_gen_add_meta(WAV_METADATA_TITLE, "HIMAN WHATS UP MAN");
    wav_editor_tag_generator_gen_add_meta(WAV_METADATA_YEAR, "20140422");
    wav_editor_tag_generator_gen_add_meta(WAV_METADATA_TRACKNUM, "0001");
    //wav_editor_tag_generator_gen_add_albumart(WAV_METADATA_ALBUMART_DATA, (const unsigned char *)lBuffer, lLength);
    wav_editor_tag_generator_gen_meta_write();
    wav_editor_tag_generator_gen_finish();

    //WAV_METADATA_FILEDESC
    int fd = -1;
    unsigned long long bytes = 0;
    char inFileDesc[33] = "/storage/emulated/0/winter02.wav";
    
    fd = open(inFileDesc, O_RDWR | O_CREAT | O_APPEND, 0666);
    if(fd == -1) {
        ALOGI("### %s() / open Error! %s: %s ###", __FUNCTION__, inFileDesc, strerror(errno));
        close(fd);
        if(lBuffer != NULL) free(lBuffer);
        exit(1);
    }

    //total size
    bytes = lseek64(fd, 0, SEEK_END);
    
    ALOGV("### %s() / open fd %d ###", __FUNCTION__, fd);
    ALOGV("### %s() / open bytes %llu", __FUNCTION__, bytes);
        
    wav_editor_tag_generator_gen_init(fd, NULL, NULL, 0, WAV_METADATA_FILEDESC); 
    wav_editor_tag_generator_gen_add_meta(WAV_METADATA_YEAR, "20140422");
    wav_editor_tag_generator_gen_add_meta(WAV_METADATA_TRACKNUM, "0001");
    wav_editor_tag_generator_gen_add_albumart(WAV_METADATA_ALBUMART_DATA, (const unsigned char *)lBuffer, lLength);
    wav_editor_tag_generator_gen_meta_write();
    wav_editor_tag_generator_gen_finish();
    
    if(fd != -1) close(fd);
    if(lBuffer != NULL) free(lBuffer);
    
    return;
}
*/
#endif