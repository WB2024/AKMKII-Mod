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
#define LOG_NDEBUG 0
#define TAG "FLACEncoder"

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/stat.h>
#include <dirent.h>
#include <utils/Log.h>
#include <cutils/properties.h>

#include "flacencoder.h"
#include "flacencoder_albumart.h"
#include "FLAC/metadata.h"  
#include "FLAC/stream_encoder.h"

// * 3 for Korea, Japan, China ... UNICODE 16 3bytes, 
// + 10 is padding to protect string overflow
#define METADATA_MAX_LENGTH ((128 * 3) + 10) 
#define METADATA_KIND_OF_COUNT 3
#define METADATA_LONG_PADDING_LENGTH 1024
#define METADATA_SHORT_PADDING_LENGTH 16

enum {
  META_VORBIS_TAGS      = 0,
  META_VORBIS_ALBUMART  = 1,
  META_VORBIS_PADDING   = 2
} kind_of_metadata;

static FLAC__StreamEncoder *encoder = 0;
static FLAC__StreamMetadata *metadata[METADATA_KIND_OF_COUNT];
static const int debug_log = 0;
static unsigned int mBit_Depth = 0;
static unsigned long mTotal_Samples = 0;
static unsigned int mChannels = 0;
static FLAC__bool result = false; 

typedef struct {
    FLAC__byte KeyAlbum[METADATA_MAX_LENGTH];
    FLAC__byte KeyArtist[METADATA_MAX_LENGTH];
    FLAC__byte KeyAlbumArtist[METADATA_MAX_LENGTH];
    FLAC__byte KeyComposer[METADATA_MAX_LENGTH];
    FLAC__byte KeyGenre[METADATA_MAX_LENGTH];
    FLAC__byte KeyTitle[METADATA_MAX_LENGTH];
    FLAC__byte KeyYear[METADATA_MAX_LENGTH];
    FLAC__byte KeyCDTrackNumber[METADATA_MAX_LENGTH];
    FLAC__byte KeyGraceNoteUIDOwner[METADATA_MAX_LENGTH];
    FLAC__byte KeyGraceNoteTAGID[METADATA_MAX_LENGTH];
    FLAC__byte KeyCopyright[METADATA_MAX_LENGTH];
    FLAC__uint16 metaNum;    
    FLAC__byte *AlbumArtPath;
    FLAC__byte *AlbumArtData;
    FLAC__uint32 AlbumArtLength;    
    FLAC__StreamMetadata_Picture_Type type;
    FLAC__bool isHasAlbumArt;
} MetaData_Info;

static MetaData_Info *gMetaData = NULL;

static void wav_encodeTo_flac_progress_callback(const FLAC__StreamEncoder *encoder, 
    FLAC__uint64 bytes_written, 
    FLAC__uint64 samples_written, 
    FLAC__uint32 frames_written, 
    FLAC__uint32 total_frames_estimate, 
    void *client_data);  

static void wav_encodeTo_flac_meta_free();

//Must call wav_encodeTo_flac_Close() paired
FLAC_ENCODER_STATUS wav_encodeTo_flac_Open() {
    if(debug_log) ALOGI("#### wav_encodeTo_flac_Open ####");
    
    result = false;
    if(encoder != NULL) {
        ALOGE("#### wav_encodeTo_flac_Open #### warning! encoder != NULL. Please Check Under codes");
    }    
    encoder = NULL;
    
    if((encoder = FLAC__stream_encoder_new()) == NULL) {  
        ALOGE("#### wav_encodeTo_flac_Open #### ERROR: FLAC__stream_encoder_new()");  
        return FLAC_ENCODER_STATUS_ERROR_OPEN;  
    }
    
    result = true;
    return FLAC_ENCODER_STATUS_OK;
}

FLAC_ENCODER_STATUS wav_encodeTo_flac_Create_Header(unsigned long total_samples,
        unsigned int channels,
        unsigned int sample_rate,
        unsigned int bit_depth,
        unsigned int compress_level,
        unsigned int uncompressed) {
        
    unsigned long total_Samples = 0;
    unsigned int Channels = 2;
    unsigned int sample_Rate = 44100;
    unsigned int bit_Depth = 16;
    unsigned int compress_Level = 0;
    FLAC__bool retResult = false; 
    
    if(debug_log) ALOGI("#### wav_encodeTo_flac_Create_Header ####");

    if(result != true) {
        ALOGE("#### wav_encodeTo_flac_Create_Header #### ERROR! Must set true from a previous function!");  
        return FLAC_ENCODER_STATUS_ERROR_GENERAL;
    }
    
    result = false;
    
    if(total_samples <= 0) {
        ALOGE("#### wav_encodeTo_flac_Create_Header ### ERROR! total_samples is invalid");
        return FLAC_ENCODER_STATUS_ERROR_HEADER;
    } else {
        total_Samples = total_samples;
        mTotal_Samples = total_Samples;
    }
    
    if(channels == 1 || channels == 2) {
        Channels = channels;
        mChannels = Channels;
    } else {
        ALOGE("#### wav_encodeTo_flac_Create_Header ### ERROR! channels is invalid");
        return FLAC_ENCODER_STATUS_ERROR_HEADER;
    }
    
    if(sample_rate == 44100 || sample_rate == 48000 || 
        sample_rate == 64000 || sample_rate == 88200 || 
        sample_rate == 96000 || sample_rate == 176400 || 
        sample_rate == 192000) {
        sample_Rate = sample_rate;
    } else {
        ALOGE("#### wav_encodeTo_flac_Create_Header ### ERROR! sample_Rate is invalid");
        return FLAC_ENCODER_STATUS_ERROR_HEADER;
    }
    
    if(bit_depth == 16 || bit_depth == 24) {
        bit_Depth = bit_depth;
        mBit_Depth = bit_Depth;
    } else {
        ALOGE("#### wav_encodeTo_flac_Create_Header ### ERROR! bit_depth is invalid");
        return FLAC_ENCODER_STATUS_ERROR_HEADER;
    }
    
    if( 0 <= compress_level && compress_level <= 8) {
        compress_Level = compress_level;
    } else {
        ALOGE("#### wav_encodeTo_flac_Create_Header ### ERROR! compress_level is invalid");
        return FLAC_ENCODER_STATUS_ERROR_HEADER;
    }
    
    //Set the "verify" flag. If true, 
    //the encoder will verify it's own encoded output by feeding it 
    //through an internal decoder and comparing the original signal against the decoded signal. 
    //If a mismatch occurs, the process call will return false. 
    //Note that this will slow the encoding process by the extra time required for decoding and comparison.
    retResult = FLAC__stream_encoder_set_verify(encoder, true);
    
    //The compression level is roughly proportional to the amount of effort the encoder expends to compress the file. 
    //A higher level usually means more computation but higher compression. 
    //The default level is suitable for most applications.
    //Currently the levels range from 0 (fastest, least compression) to 8 (slowest, most compression). 
    //A value larger than 8 will be treated as 8.
    retResult &= FLAC__stream_encoder_set_compression_level(encoder, compress_Level); 
    
    retResult &= FLAC__stream_encoder_set_channels(encoder, Channels);  
    retResult &= FLAC__stream_encoder_set_bits_per_sample(encoder, bit_Depth);  
    retResult &= FLAC__stream_encoder_set_sample_rate(encoder, sample_Rate);  
    retResult &= FLAC__stream_encoder_set_total_samples_estimate(encoder, total_Samples);

	///////////////////////////////////////////////////////////////////////////////////////
	// 2014.04.23 iriver-eddy
	// Uncompressed FLAC
	if(uncompressed != 0) {
		retResult &= FLAC__stream_encoder_disable_constant_subframes(encoder, true);
		retResult &= FLAC__stream_encoder_disable_fixed_subframes(encoder, true);
	}
	///////////////////////////////////////////////////////////////////////////////////////
	
    if(retResult != 1) {
        ALOGE("#### wav_encodeTo_flac_Create_Header ### ERROR! WAV Header Info");
        return FLAC_ENCODER_STATUS_ERROR_HEADER;
    }
    
    result = true;
    return FLAC_ENCODER_STATUS_OK;
}

FLAC_ENCODER_STATUS wav_encodeTo_flac_init_meta() {

    if(debug_log) ALOGI("#### wav_encodeTo_flac_init_meta ####");
    
    if(result != true) {
        ALOGE("#### wav_encodeTo_flac_init_meta #### ERROR! Must set true from a previous function!");
        return FLAC_ENCODER_STATUS_ERROR_GENERAL;
    }

    if(gMetaData != NULL) {
        ALOGE("#### wav_encodeTo_flac_init_meta #### warning! gMetaData != NULL. Please Check Under codes");
    }
    
    result = false;
    
    wav_encodeTo_flac_meta_free();
    
    if(NULL == (gMetaData = (MetaData_Info *) malloc(sizeof(MetaData_Info)))) {
        ALOGE("#### wav_encodeTo_flac_init_meta #### ERROR! Malloc failed, Out of memory!");
        return FLAC_ENCODER_STATUS_ERROR_GENERAL;
    }
    memset(gMetaData, 0 , sizeof(MetaData_Info));
    gMetaData->AlbumArtPath = NULL; 
    gMetaData->AlbumArtData = NULL;
    gMetaData->AlbumArtLength = 0;    
    gMetaData->type = FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER;
    gMetaData->metaNum = 0;
    gMetaData->isHasAlbumArt = false;

    if(metadata[META_VORBIS_TAGS] != NULL || metadata[META_VORBIS_ALBUMART] != NULL || metadata[META_VORBIS_PADDING] != NULL) {
        ALOGE("#### wav_encodeTo_flac_init_meta #### warning! metadata != NULL. Please Check Under codes");
    }
    
    metadata[META_VORBIS_TAGS] = NULL;
    metadata[META_VORBIS_ALBUMART] = NULL;
    metadata[META_VORBIS_PADDING] = NULL;
    
    if((metadata[META_VORBIS_TAGS] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == NULL) {
        ALOGE("wav_encodeTo_flac_init_meta ### ERROR! Making Meta"); 
        wav_encodeTo_flac_meta_free();
        return FLAC_ENCODER_STATUS_ERROR_META;
    }
        
    result = true;
    return FLAC_ENCODER_STATUS_OK;    
}

FLAC_ENCODER_STATUS wav_encodeTo_flac_finish_meta() {
    FLAC__bool hasVorbis = false; 
    FLAC__bool hasAlbumart = false; 
    FLAC__StreamMetadata_VorbisComment_Entry entry;
    FLAC__bool retResult = false; 
    const char *error;
    
    if(debug_log) ALOGI("#### wav_encodeTo_flac_finish_meta ####");
    
    if(result != true) {
        ALOGE("#### wav_encodeTo_flac_finish_meta #### ERROR! Must set true from a previous function!");
        return FLAC_ENCODER_STATUS_ERROR_GENERAL;
    }

    if(gMetaData == NULL) {
        ALOGE("#### wav_encodeTo_flac_finish_meta #### ERROR! gMetaData == NULL");
        return FLAC_ENCODER_STATUS_ERROR_GENERAL;
    }
    
    result = false;

    //MetaData Checking 
    if(gMetaData->metaNum > 0) {
        if((metadata[META_VORBIS_TAGS] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) != NULL) {
            if((strlen((char *)gMetaData->KeyAlbum) > 0)) {
                if(debug_log) ALOGI("#### gMetaData->KeyAlbum : %s, %d", 
                    gMetaData->KeyAlbum, strlen((char *)gMetaData->KeyAlbum));
                retResult = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(
                    &entry, "ALBUM", (const char *)gMetaData->KeyAlbum);
                retResult &= FLAC__metadata_object_vorbiscomment_append_comment(
                    metadata[META_VORBIS_TAGS], entry, false);
                if(retResult != 1) {
                    ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Vorbis KeyAlbum Meta"); 
                    return FLAC_ENCODER_STATUS_ERROR_META;
                }
            }
            if((strlen((char *)gMetaData->KeyArtist) > 0)) {
                if(debug_log) ALOGI("#### gMetaData->KeyArtist : %s, %d", 
                    gMetaData->KeyArtist, strlen((char *)gMetaData->KeyArtist));
                retResult = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(
                    &entry, "ARTIST", (const char *)gMetaData->KeyArtist);
                retResult &= FLAC__metadata_object_vorbiscomment_append_comment(
                    metadata[META_VORBIS_TAGS], entry, false);
                if(retResult != 1) {
                    ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Vorbis KeyArtist Meta"); 
                    return FLAC_ENCODER_STATUS_ERROR_META;
                }
            }
            if((strlen((char *)gMetaData->KeyAlbumArtist) > 0)) {
                if(debug_log) ALOGI("#### gMetaData->KeyAlbumArtist : %s, %d", 
                    gMetaData->KeyAlbumArtist, strlen((char *)gMetaData->KeyAlbumArtist));
                retResult = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(
                    &entry, "ALBUMARTIST", (const char *)gMetaData->KeyAlbumArtist);
                retResult &= FLAC__metadata_object_vorbiscomment_append_comment(
                    metadata[META_VORBIS_TAGS], entry, false);
                if(retResult != 1) {
                    ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Vorbis KeyAlbumArtist Meta"); 
                    return FLAC_ENCODER_STATUS_ERROR_META;
                }
            }
            if((strlen((char *)gMetaData->KeyComposer) > 0)) {
                if(debug_log) ALOGI("#### gMetaData->KeyComposer : %s, %d", 
                    gMetaData->KeyComposer, strlen((char *)gMetaData->KeyComposer));
                retResult = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(
                    &entry, "COMPOSER", (const char *)gMetaData->KeyComposer);
                retResult &= FLAC__metadata_object_vorbiscomment_append_comment(
                    metadata[META_VORBIS_TAGS], entry, false);
                if(retResult != 1) {
                    ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Vorbis KeyComposer Meta"); 
                    return FLAC_ENCODER_STATUS_ERROR_META;
                }
            }
            if((strlen((char *)gMetaData->KeyGenre) > 0)) {
                if(debug_log) ALOGI("#### gMetaData->KeyGenre : %s, %d", 
                    gMetaData->KeyGenre, strlen((char *)gMetaData->KeyGenre));
                retResult = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(
                    &entry, "GENRE", (const char *)gMetaData->KeyGenre);
                retResult &= FLAC__metadata_object_vorbiscomment_append_comment(
                    metadata[META_VORBIS_TAGS], entry, false);
                if(retResult != 1) {
                    ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Vorbis KeyGenre Meta"); 
                    return FLAC_ENCODER_STATUS_ERROR_META;
                }
            }
            if((strlen((char *)gMetaData->KeyTitle) > 0)) {
                if(debug_log) ALOGI("#### gMetaData->KeyTitle : %s, %d", 
                    gMetaData->KeyTitle, strlen((char *)gMetaData->KeyTitle));
                retResult = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(
                    &entry, "TITLE", (const char *)gMetaData->KeyTitle);
                retResult &= FLAC__metadata_object_vorbiscomment_append_comment(
                    metadata[META_VORBIS_TAGS], entry, false);
                if(retResult != 1) {
                    ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Vorbis KeyTitle Meta"); 
                    return FLAC_ENCODER_STATUS_ERROR_META;
                }
            }
            if((strlen((char *)gMetaData->KeyYear) > 0)) {
                if(debug_log) ALOGI("#### gMetaData->KeyYear : %s, %d", 
                    gMetaData->KeyYear, strlen((char *)gMetaData->KeyYear));
                retResult = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(
                    &entry, "DATE", (const char *)gMetaData->KeyYear);
                retResult &= FLAC__metadata_object_vorbiscomment_append_comment(
                    metadata[META_VORBIS_TAGS], entry, false);
                if(retResult != 1) {
                    ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Vorbis KeyYear Meta"); 
                    return FLAC_ENCODER_STATUS_ERROR_META;
                }
            }
            if((strlen((char *)gMetaData->KeyCDTrackNumber) > 0)) {
                if(debug_log) ALOGI("#### gMetaData->KeyCDTrackNumber : %s, %d", 
                    gMetaData->KeyCDTrackNumber, strlen((char *)gMetaData->KeyCDTrackNumber));
                retResult = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(
                    &entry, "TRACKNUMBER", (const char *)gMetaData->KeyCDTrackNumber);
                retResult &= FLAC__metadata_object_vorbiscomment_append_comment(
                    metadata[META_VORBIS_TAGS], entry, false);
                if(retResult != 1) {
                    ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Vorbis KeyCDTrackNumber Meta"); 
                    return FLAC_ENCODER_STATUS_ERROR_META;
                }
            }
            if((strlen((char *)gMetaData->KeyGraceNoteUIDOwner) > 0)) {
                if(debug_log) ALOGI("#### gMetaData->KeyGraceNoteUIDOwner : %s, %d", 
                    gMetaData->KeyGraceNoteUIDOwner, strlen((char *)gMetaData->KeyGraceNoteUIDOwner));
                retResult = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(
                    &entry, "UFID", (const char *)gMetaData->KeyGraceNoteUIDOwner);
                retResult &= FLAC__metadata_object_vorbiscomment_append_comment(
                    metadata[META_VORBIS_TAGS], entry, false);
                if(retResult != 1) {
                    ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Vorbis KeyGraceNoteUIDOwner Meta"); 
                    return FLAC_ENCODER_STATUS_ERROR_META;
                }
            }
            if((strlen((char *)gMetaData->KeyGraceNoteTAGID) > 0)) {
                if(debug_log) ALOGI("#### gMetaData->KeyGraceNoteTAGID : %s, %d", 
                    gMetaData->KeyGraceNoteTAGID, strlen((char *)gMetaData->KeyGraceNoteTAGID));
                retResult = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(
                    &entry, "GNID", (const char *)gMetaData->KeyGraceNoteTAGID);
                retResult &= FLAC__metadata_object_vorbiscomment_append_comment(
                    metadata[META_VORBIS_TAGS], entry, false);
                if(retResult != 1) {
                    ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Vorbis KeyGraceNoteTAGID Meta"); 
                    return FLAC_ENCODER_STATUS_ERROR_META;
                }
            }               
            if((strlen((char *)gMetaData->KeyCopyright) > 0)) {
                if(debug_log) ALOGI("#### gMetaData->KeyCopyright : %s, %d", 
                    gMetaData->KeyCopyright, strlen((char *)gMetaData->KeyCopyright));
                retResult = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(
                    &entry, "COPYRIGHT", (const char *)gMetaData->KeyCopyright);
                retResult &= FLAC__metadata_object_vorbiscomment_append_comment(
                    metadata[META_VORBIS_TAGS], entry, false);
                if(retResult != 1) {
                    ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Vorbis KeyCDTrackNumber Meta"); 
                    return FLAC_ENCODER_STATUS_ERROR_META;
                }
            }
        }else {
            ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Making Meta Vorbis"); 
            return FLAC_ENCODER_STATUS_ERROR_META;
        }
    } else {
        if((metadata[META_VORBIS_TAGS] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL) {
            ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Making Meta Padding"); 
            return FLAC_ENCODER_STATUS_ERROR_META;
        }
        metadata[META_VORBIS_TAGS]->length = METADATA_SHORT_PADDING_LENGTH; /* set the padding length */ 
    }
    
    //AlbumArt Checking    
    if(gMetaData->isHasAlbumArt) {
        if(gMetaData->AlbumArtLength == 0) {
            if(0 == (metadata[META_VORBIS_ALBUMART] = grabbag__picture_parse_specification(
                (const char *)gMetaData->AlbumArtPath, &error))) {
                ALOGE("#### wav_encodeTo_flac_finish_meta #### ERROR! grabbag__picture_parse_specification : %s", error);
                if(metadata[META_VORBIS_TAGS] != NULL) FLAC__metadata_object_delete(metadata[META_VORBIS_TAGS]); 
                return FLAC_ENCODER_STATUS_ERROR_META;
            }
        } else {
            if(0 == (metadata[META_VORBIS_ALBUMART] = grabbag__picture_parse_buffer_specification(
                (const char *)gMetaData->AlbumArtData, gMetaData->AlbumArtLength, &error))) {
                ALOGE("#### wav_encodeTo_flac_finish_meta #### ERROR! grabbag__picture_parse_buffer_specification : %s", error);
                if(metadata[META_VORBIS_TAGS] != NULL) FLAC__metadata_object_delete(metadata[META_VORBIS_TAGS]); 
                return FLAC_ENCODER_STATUS_ERROR_META;
            }
        }
    } else {
        if((metadata[META_VORBIS_ALBUMART] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL) {
            ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Making Meta Padding"); 
            if(metadata[META_VORBIS_TAGS] != NULL) FLAC__metadata_object_delete(metadata[META_VORBIS_TAGS]); 
            return FLAC_ENCODER_STATUS_ERROR_META;
        }
        metadata[META_VORBIS_ALBUMART]->length = METADATA_SHORT_PADDING_LENGTH; /* set the padding length */ 
    }

    //Adding Padding 
    if((metadata[META_VORBIS_PADDING] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL) {
        ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Making Meta Padding");
        if(metadata[META_VORBIS_TAGS] != NULL) FLAC__metadata_object_delete(metadata[META_VORBIS_TAGS]); 
        if(metadata[META_VORBIS_ALBUMART] != NULL) FLAC__metadata_object_delete(metadata[META_VORBIS_ALBUMART]);
        return FLAC_ENCODER_STATUS_ERROR_META;
    } 

    metadata[META_VORBIS_PADDING]->length = METADATA_LONG_PADDING_LENGTH; /* set the padding length */ 
    
    retResult = false; //init
    retResult = FLAC__stream_encoder_set_metadata(encoder, metadata, METADATA_KIND_OF_COUNT); 

    if(retResult != 1) {
        ALOGE("wav_encodeTo_flac_finish_meta ### ERROR! Creating Meta is Failed"); 
        if(metadata[META_VORBIS_TAGS] != NULL) FLAC__metadata_object_delete(metadata[META_VORBIS_TAGS]); 
        if(metadata[META_VORBIS_ALBUMART] != NULL) FLAC__metadata_object_delete(metadata[META_VORBIS_ALBUMART]);
        if(metadata[META_VORBIS_PADDING] != NULL) FLAC__metadata_object_delete(metadata[META_VORBIS_PADDING]);
        return FLAC_ENCODER_STATUS_ERROR_META;
    }
        
    result = true;
    return FLAC_ENCODER_STATUS_OK; 
}
    
FLAC_ENCODER_STATUS wav_encodeTo_flac_add_meta(FLAC_METADATA_INFO info, const char *infoData) {
    FLAC__StreamMetadata_VorbisComment_Entry entry;
    FLAC__bool success = true;
    unsigned int info_length = 0;
    
    if(debug_log) ALOGI("#### wav_encodeTo_flac_add_meta ####");
    
    if((info_length = strlen(infoData)) <= 0) {
        ALOGE("#### wav_encodeTo_flac_add_meta #### ERROR! Length is worng!");
        return FLAC_ENCODER_STATUS_ERROR_GENERAL;
    }
    
    if(gMetaData == NULL) {
        ALOGE("#### wav_encodeTo_flac_add_meta #### ERROR! gMetaData == NULL");
        return FLAC_ENCODER_STATUS_ERROR_GENERAL;
    }

    if(info_length >= METADATA_MAX_LENGTH) {
        info_length = METADATA_MAX_LENGTH - 1;
    }
    
    switch(info) {
        case FLAC_METADATA_ALBUM:
            memset(gMetaData->KeyAlbum, 0, METADATA_MAX_LENGTH);
            memcpy(gMetaData->KeyAlbum, infoData, info_length);
        break;
        case FLAC_METADATA_ARTIST:
            memset(gMetaData->KeyArtist, 0, METADATA_MAX_LENGTH);
            memcpy(gMetaData->KeyArtist, infoData, info_length);
        break;
        case FLAC_METADATA_ALBUMARTIST:
            memset(gMetaData->KeyAlbumArtist, 0, METADATA_MAX_LENGTH);
            memcpy(gMetaData->KeyAlbumArtist, infoData, info_length);
        break;
        case FLAC_METADATA_COMPOSER:
            memset(gMetaData->KeyComposer, 0, METADATA_MAX_LENGTH);
            memcpy(gMetaData->KeyComposer, infoData, info_length); 
        break;
        case FLAC_METADATA_GENRE:
            memset(gMetaData->KeyGenre, 0, METADATA_MAX_LENGTH);
            memcpy(gMetaData->KeyGenre, infoData, info_length); 
        break;
        case FLAC_METADATA_TITLE:
            memset(gMetaData->KeyTitle, 0, METADATA_MAX_LENGTH);
            memcpy(gMetaData->KeyTitle, infoData, info_length); 
        break;
        case FLAC_METADATA_YEAR:
            memset(gMetaData->KeyYear, 0, METADATA_MAX_LENGTH);
            memcpy(gMetaData->KeyYear, infoData, info_length); 
        break;
        case FLAC_METADATA_TRACKNUM:
            memset(gMetaData->KeyCDTrackNumber, 0, METADATA_MAX_LENGTH);
            memcpy(gMetaData->KeyCDTrackNumber, infoData, info_length); 
        break;
        case FLAC_METADATA_GREACENOTE_UIDOWNER:
            memset(gMetaData->KeyGraceNoteUIDOwner, 0, METADATA_MAX_LENGTH);
            memcpy(gMetaData->KeyGraceNoteUIDOwner, infoData, info_length); 
        break;
        case FLAC_METADATA_GREACENOTE_TAGID:
            memset(gMetaData->KeyGraceNoteTAGID, 0, METADATA_MAX_LENGTH);
            memcpy(gMetaData->KeyGraceNoteTAGID, infoData, info_length); 
        break;
        case FLAC_METADATA_COPYRIGHT:
            memset(gMetaData->KeyCopyright, 0, METADATA_MAX_LENGTH);
            memcpy(gMetaData->KeyCopyright, infoData, info_length);
        break;
        default:
            success = false;
        break;
    }

    if(success) {
        gMetaData->metaNum += 1;
        if(debug_log) ALOGI("#### wav_encodeTo_flac_add_meta #### info : %d , infoData : %s", info, infoData);
    }

    return FLAC_ENCODER_STATUS_OK;
}

FLAC_ENCODER_STATUS wav_encodeTo_flac_add_AlbumArt(FLAC_METADATA_INFO info, 
    const unsigned char *imageFile, 
    const unsigned char *buffer,
    const unsigned long length, 
    const unsigned int type) {
    
    unsigned int path_length;

    if(debug_log) ALOGI("#### wav_encodeTo_flac_add_AlbumArt #### info : %d", info);
    
    if(gMetaData != NULL && gMetaData->isHasAlbumArt == true) {
        ALOGE("#### wav_encodeTo_flac_add_AlbumArt #### ERROR! isHasAlbumArt is True");
        return FLAC_ENCODER_STATUS_ERROR_META;
    }

    if(info == FLAC_METADATA_ALBUMART_DATA) {
        if((length <= 0) || (strlen((char *)buffer) <= 0)) {
            ALOGE("#### wav_encodeTo_flac_add_AlbumArt #### ERROR! buffer == NULL");
            return FLAC_ENCODER_STATUS_ERROR_META;
        }
        
        if(NULL == (gMetaData->AlbumArtData = (FLAC__byte *) malloc(length + 1))) {
            ALOGE("#### wav_encodeTo_flac_add_AlbumArt #### ERROR! Malloc fail, Out of Memory!");
            return FLAC_ENCODER_STATUS_ERROR_GENERAL;
        }
        memset(gMetaData->AlbumArtData, 0, length);
        memcpy(gMetaData->AlbumArtData, buffer, length);
        gMetaData->AlbumArtLength = (FLAC__uint32) length;
        gMetaData->type = (FLAC__StreamMetadata_Picture_Type) type; //Not Working
        gMetaData->isHasAlbumArt = true;        
    } else if(info == FLAC_METADATA_ALBUMART_PATH) {        
        if(imageFile == NULL) {
            ALOGE("#### wav_encodeTo_flac_add_AlbumArt #### ERROR! imageFile == NULL");
            return FLAC_ENCODER_STATUS_ERROR_META;
        }
    
        if((path_length = strlen((char *)imageFile)) < 5) {
            ALOGE("#### wav_encodeTo_flac_add_AlbumArt #### ERROR! file path length is wrong!");
            return FLAC_ENCODER_STATUS_ERROR_META;
        }
        
        if(imageFile != NULL) {
            if(NULL == (gMetaData->AlbumArtPath = (FLAC__byte *) malloc(path_length + 1))) {
                ALOGE("#### wav_encodeTo_flac_add_AlbumArt #### ERROR! Malloc fail, Out of Memory!");
                return FLAC_ENCODER_STATUS_ERROR_GENERAL;
            }
            memset(gMetaData->AlbumArtPath, 0, path_length);
            memcpy(gMetaData->AlbumArtPath, imageFile, path_length);
            gMetaData->AlbumArtLength = 0;
            gMetaData->type = (FLAC__StreamMetadata_Picture_Type) type; //Not Working
            gMetaData->isHasAlbumArt = true;                
        } else {
            ALOGE("wav_encodeTo_flac_add_AlbumArt ### ERROR! Making AlbumArt"); 
            return FLAC_ENCODER_STATUS_ERROR_META;
        }
    } else {
        ALOGE("wav_encodeTo_flac_add_AlbumArt ### ERROR! Making AlbumArt"); 
        return FLAC_ENCODER_STATUS_ERROR_META;
    }

    return FLAC_ENCODER_STATUS_OK;
}

FLAC_ENCODER_STATUS wav_encodeTo_flac_EncodeBufferInit(const char *outFlacMakeFile) {
    FLAC__StreamEncoderInitStatus init_status;  
    struct stat statbuf;
    int type = 0;
    
    if(debug_log) ALOGI("#### wav_encodeTo_flac_EncodeBufferInit #### outFlacMakeFile : %s", outFlacMakeFile);

    if(result != true) {
        ALOGE("#### wav_encodeTo_flac_EncodeBufferInit #### ERROR! Must set true from a previous function!");
        return FLAC_ENCODER_STATUS_ERROR_GENERAL;
    }
    
    result = false;
    init_status = FLAC__stream_encoder_init_file(encoder, outFlacMakeFile, wav_encodeTo_flac_progress_callback, /*client_data=*/NULL); 
    
    if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) { 
        ALOGE("#### wav_encodeTo_flac_EncodeBufferInit #### ERROR: initializing encoder: %s\n", FLAC__StreamEncoderInitStatusString[init_status]); 
        return FLAC_ENCODER_STATUS_ERROR_ENCODE;
    }   

    result = true;
    return FLAC_ENCODER_STATUS_OK;
}

FLAC_ENCODER_STATUS wav_encodeTo_flac_EncodeBuffer(unsigned char *buffer, unsigned long length) {
    FLAC__bool retResult = false; 
    char *bufferCopy = NULL;
    unsigned int needSize = 0;

    if(debug_log) ALOGI("#### wav_encodeTo_flac_EncodeBuffer ####");

    if(result != true) {
        ALOGE("#### wav_encodeTo_flac_EncodeBuffer #### ERROR! Must set true from a previous function!");
        return FLAC_ENCODER_STATUS_ERROR_GENERAL;
    }
    
    result = false;
    
    if(length <= 0) {
        ALOGE("wav_encodeTo_flac_EncodeBuffer ### ERROR! buffer is empty"); 
        return FLAC_ENCODER_STATUS_ERROR_ENCODE;
    }

    if(mBit_Depth == 16 || mBit_Depth == 24) {
        if(mBit_Depth == 16 && mBit_Depth % 2 != 0) {
            ALOGE("wav_encodeTo_flac_EncodeBuffer ### ERROR! 16Bits Length is wrong"); 
            return FLAC_ENCODER_STATUS_ERROR_ENCODE;
        } else if(mBit_Depth == 24 && mBit_Depth % 3 != 0) {
            ALOGE("wav_encodeTo_flac_EncodeBuffer ### ERROR! 24Bits Length is wrong"); 
            return FLAC_ENCODER_STATUS_ERROR_ENCODE;
        }
        
        char *clone_buf = (char*) malloc(length);
        size_t i = 0; 
        size_t j = 0;         
        
        memset(clone_buf, 0, length);
        memcpy(clone_buf, buffer, length);            
          
        if(mBit_Depth == 24) {
            FLAC__int32 pcm[length / 3];
            for(i = 0; i < length; i += 3) { 
                FLAC__int32 x = (FLAC__int32) (( clone_buf[i] & 0xff) | 
                           (FLAC__int32) ((clone_buf[i+1] & 0xff) << 8) |
                           (FLAC__int32) ((clone_buf[i+2] & 0xff) << 16));
                x = (x << 8) >> 8; // sign extension
                pcm[j] = x; 
                j ++;
            }
            needSize = length / (3 * mChannels);
            retResult = FLAC__stream_encoder_process_interleaved(encoder, pcm, needSize);
        } else if(mBit_Depth == 16) {
            FLAC__int32 pcm[length / 2];
            for(i = 0; i < length; i += 2) { 
                pcm[j] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)(clone_buf[i+1] & 0xff) << 8) | 
                                                    (FLAC__int16)(clone_buf[i] & 0xff));
                j ++;
            }
            needSize = length / (2 * mChannels);
            retResult = FLAC__stream_encoder_process_interleaved(encoder, pcm, needSize);
        }            

        if(clone_buf != NULL) {
            free(clone_buf);
            clone_buf = NULL;
        }
            
        if(retResult != 1) {
            ALOGE("#### wav_encodeTo_flac_EncodeBuffer ### ERROR! Encoding");
            return FLAC_ENCODER_STATUS_ERROR_HEADER;
        }
    } else {
        ALOGE("wav_encodeTo_flac_EncodeBuffer ### ERROR! buffer is empty"); 
        return FLAC_ENCODER_STATUS_ERROR_ENCODE;
    }

    mTotal_Samples -= needSize;   
    
    if(debug_log) ALOGI("#### wav_encodeTo_flac_EncodeBuffer #### mTotal_Samples : %lu", mTotal_Samples);
    
    result = true;
    return FLAC_ENCODER_STATUS_OK;
}

FLAC_ENCODER_STATUS wav_encodeTo_flac_Close() {
    if(debug_log) ALOGI("#### wav_encodeTo_flac_Close ####");

    if(encoder != NULL) FLAC__stream_encoder_finish(encoder); 
    if(debug_log) ALOGI("#### wav_encodeTo_flac_Close ### state: %s\n", FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder)]);

    if(metadata[META_VORBIS_TAGS] != NULL) FLAC__metadata_object_delete(metadata[META_VORBIS_TAGS]); 
    if(metadata[META_VORBIS_ALBUMART] != NULL) FLAC__metadata_object_delete(metadata[META_VORBIS_ALBUMART]);
    if(metadata[META_VORBIS_PADDING] != NULL) FLAC__metadata_object_delete(metadata[META_VORBIS_PADDING]);

    metadata[META_VORBIS_TAGS] = NULL;
    metadata[META_VORBIS_ALBUMART] = NULL;
    metadata[META_VORBIS_PADDING] = NULL;
        
    if(encoder != NULL) FLAC__stream_encoder_delete(encoder);
    wav_encodeTo_flac_meta_free();
    
    encoder = NULL;    
    result = false; 
    
    return FLAC_ENCODER_STATUS_OK;
}

void wav_encodeTo_flac_progress_callback(const FLAC__StreamEncoder *encoder, 
        FLAC__uint64 bytes_written, 
        FLAC__uint64 samples_written, 
        FLAC__uint32 frames_written, 
        FLAC__uint32 total_frames_estimate, 
        void *client_data) { 
        
    (void)encoder, (void)client_data; 
    if(debug_log) ALOGV("#### wav_encodeTo_flac_progress_callback #### bytes_written : %lld, samples_written : %lld, total_samples : %lu samples, %u/%u frames", bytes_written, samples_written, mTotal_Samples, frames_written, total_frames_estimate); 
}

static void wav_encodeTo_flac_meta_free() {

    if(debug_log) ALOGI("#### wav_encodeTo_flac_meta_free ####");
    
    if(gMetaData != NULL) {
        if(gMetaData->AlbumArtPath != NULL) {
            free(gMetaData->AlbumArtPath);
            gMetaData->AlbumArtPath = NULL;            
        }
        
        if(gMetaData->AlbumArtData != NULL) {
            free(gMetaData->AlbumArtData);
            gMetaData->AlbumArtData = NULL;            
        }
        
        free(gMetaData);
        gMetaData = NULL;
    }
}

#define _FLAC_ENCODER_TEST_SAMPLE_CODE_
#ifndef _FLAC_ENCODER_TEST_SAMPLE_CODE_
/////////////////////////////////////////////////////////////////
//Under test sample codes 
//To convert WAV to FLAC without lossless.
//
//Compression Level Default 5
//16/24Bit can be encoded to FLAC
//44100 ~ 192000hz limited
/////////////////////////////////////////////////////////////////
#define READSIZE 1200 
static FLAC__byte buffer[READSIZE/*samples*/ * 2/*bytes_per_sample*/ * 2/*channels*/]; /* we read the WAVE data into here */  
static FLAC__byte buffer24[READSIZE/*samples*/ * 3/*bytes_per_sample*/ * 2/*channels*/]; /* we read the WAVE data into here */ 

void test_flac_encoder() {
    char prop_dump_value[10];
    char inFilePath[31] = "/storage/emulated/0/winter.wav";
    char outFilePath[32] = "/storage/emulated/0/winter.flac";
    
    if(debug_log) ALOGI("#### test_flac_encoder #### [START]");
    
    property_get("iriver.flac.convert", prop_dump_value, "false");
    if(!strcmp(prop_dump_value,"true")) {        
        wav_encodeTo_flac_DEMO_FUNCTION(inFilePath, outFilePath);
    } else {
        wav_encodeTo_flac_DEMO_API(inFilePath, outFilePath);
    }
    if(debug_log) ALOGI("#### test_flac_encoder #### [END]");
}

void wav_encodeTo_flac_DEMO_FUNCTION(char *inWavSourceFile, char *outFlacMakeFile) {
    FLAC__bool ok = true;  
    FLAC__StreamEncoder *encoder = 0;  
    FILE *fin = NULL;  
    FLAC__uint32 sample_rate = 0;  
    FLAC__uint16 channels = 0;  
    FLAC__uint16 bps = 0;
    struct stat statbuf;
    int type = 0;
    FLAC__uint32 total_samples = 0;

    if(debug_log) ALOGV("wav_encodeTo_flac_DEMO_FUNCTION ### inWavSourceFile : %s", inWavSourceFile);
    if(debug_log) ALOGV("wav_encodeTo_flac_DEMO_FUNCTION ### outFlacMakeFile : %s", outFlacMakeFile);

    if (stat(inWavSourceFile, &statbuf) == 0) {
        if (S_ISREG(statbuf.st_mode)) {
            type = DT_REG;
        } else if (S_ISDIR(statbuf.st_mode)) {
            type = DT_DIR;
        }
    } else {
        ALOGE("wav_encodeTo_flac_DEMO_FUNCTION ### Error! Path is invalid!");
        return;
    }

    if (type == DT_DIR) {
        ALOGE("wav_encodeTo_flac_DEMO_FUNCTION ### Error! Path is not File");
        return;
    }

    if((fin = fopen(inWavSourceFile, "rb")) == NULL) {  
        if(fin != NULL) {
            fclose(fin);
            fin = NULL;
        }
        return;
    }  
    
    /* read wav header and validate it */  
    if(fread(buffer24, 1, 68, fin) != 68 ||  
        memcmp(buffer24, "RIFF", 4)) {  
        ALOGE("wav_encodeTo_flac_DEMO_FUNCTION ### ERROR: invalid/unsupported WAVE file");  
        if(fin != NULL) {
            fclose(fin);
            fin = NULL;
        }
        return;  
    }
    
    sample_rate = ((((((unsigned)buffer24[27] << 8) | buffer24[26]) << 8) | buffer24[25]) << 8) | buffer24[24];
    channels = 2; 
    char prop_dump_value[10];
    property_get("iriver.flac.bps", prop_dump_value, "false");
    if(!strcmp(prop_dump_value,"true")) {
        bps = 16;        
    } else {
        bps = 24;
    }
    
    if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_FUNCTION ### buffer[34] : %d", buffer24[34]);
    if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_FUNCTION ### bps : %d", bps);

    if(bps == 16) {
        total_samples = (((((((unsigned)buffer24[43] << 8) | buffer24[42]) << 8) | buffer24[41]) << 8) | buffer24[40]) / 4; 
        fseek(fin, 44, SEEK_SET);
        if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_FUNCTION ### 16bps ftell : %ld", ftell(fin));
    } else if(bps == 24) {
        total_samples = (((((((unsigned)buffer24[67] << 8) | buffer24[66]) << 8) | buffer24[65]) << 8) | buffer24[64]) / 6; 
        fseek(fin, 68, SEEK_SET);
        if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_FUNCTION ### 24bps ftell : %ld", ftell(fin));
    }

    if(debug_log) {
        ALOGI("wav_encodeTo_flac_DEMO_FUNCTION ### total_samples : %lu", total_samples);
        ALOGI("wav_encodeTo_flac_DEMO_FUNCTION ### sample_rate : %lu", sample_rate);
        ALOGI("wav_encodeTo_flac_DEMO_FUNCTION ### channels : %d", channels);    
    }

    wav_encodeTo_flac_Open();
    if(bps == 16) {
        wav_encodeTo_flac_Create_Header(total_samples, 2, 44100, 16, 5);
    } else if(bps == 24) {
        wav_encodeTo_flac_Create_Header(total_samples, 2, 44100, 24, 5);
    }
    wav_encodeTo_flac_Create_meta();
    wav_encodeTo_flac_EncodeBufferInit(outFlacMakeFile);

    /* read blocks of samples from WAVE file and feed to encoder */ 
    if(ok) { 
        size_t left = (size_t)total_samples; 
        if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_FUNCTION ### channels*(bps/8) : %d", channels*(bps/8));
        while(ok && left) { 
            size_t need = ((left > READSIZE) ? (size_t)READSIZE : (size_t)left);
            if(bps == 16) {
                if(fread(buffer, channels*(bps/8), need, fin) != need) {
                    ALOGE("wav_encodeTo_flac_DEMO_FUNCTION ### ERROR: reading from WAVE file\n"); 
                    ok = false; 
                } else {
                    ok = true;
                }
            } else if(bps == 24) {
                if(fread(buffer24, channels*(bps/8), need, fin) != need) { 
                    ALOGE("wav_encodeTo_flac_DEMO_FUNCTION ### ERROR: reading from WAVE file\n"); 
                    ok = false; 
                } else {
                    ok = true;
                }
            }

            if(ok) { 
                /* convert the packed little-endian 16-bit PCM samples from WAVE into an interleaved FLAC__int32 buffer for libFLAC */
                if(bps == 16) {
                    wav_encodeTo_flac_EncodeBuffer(buffer, need*channels*(bps/8));
                } else if(bps == 24) {
                    wav_encodeTo_flac_EncodeBuffer(buffer24, need*channels*(bps/8));
                }
            } 
            left -= need; 
        } 
    } 

    wav_encodeTo_flac_Close();
    
    if(fin != NULL) {
        fclose(fin);
        fin = NULL;
    } 
    
    return;
}

static void progress_callback(const FLAC__StreamEncoder *encoder, 
    FLAC__uint64 bytes_written, 
    FLAC__uint64 samples_written, 
    FLAC__uint32 frames_written, 
    FLAC__uint32 total_frames_estimate, 
    void *client_data);  
static FLAC__uint32 total_samples_wav = 0; /* can use a 32-bit number due to WAVE size limitations */  
static FLAC__int32 pcmBuffer[READSIZE/*samples*/ * 2/*channels*/];

void wav_encodeTo_flac_DEMO_API(char *inWavSourceFile, char *outFlacMakeFile) {
    FLAC__bool ok = true;  
    FLAC__StreamEncoder *encoder = 0;  
    FLAC__StreamEncoderInitStatus init_status;  
    FLAC__StreamMetadata *metadata[2];  
    FLAC__StreamMetadata_VorbisComment_Entry entry;  
    FILE *fin = NULL;  
    FLAC__uint32 sample_rate = 0;  
    FLAC__uint16 channels = 0;  
    FLAC__uint16 bps = 0;
    struct stat statbuf;
    int type = 0;

    if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API ### inWavSourceFile : %s", inWavSourceFile);
    if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API ### outFlacMakeFile : %s", outFlacMakeFile);

    if (stat(inWavSourceFile, &statbuf) == 0) {
        if (S_ISREG(statbuf.st_mode)) {
            type = DT_REG;
        } else if (S_ISDIR(statbuf.st_mode)) {
            type = DT_DIR;
        }
    } else {
        ALOGE("wav_encodeTo_flac_DEMO_API ### Error! Path is invalid!");
        return;
    }

    if (type == DT_DIR) {
        ALOGE("wav_encodeTo_flac_DEMO_API ### Error! Path is not File");
        return;
    }

    if((fin = fopen(inWavSourceFile, "rb")) == NULL) {  
        if(fin != NULL) {
            fclose(fin);
            fin = NULL;
        }
        return;
    }  
    
    /* read wav header and validate it */  
    if(fread(buffer24, 1, 68, fin) != 68 ||  
        memcmp(buffer24, "RIFF", 4)) {  
        ALOGE("wav_encodeTo_flac_DEMO_API ### ERROR: invalid/unsupported WAVE file");  
        if(fin != NULL) {
            fclose(fin);
            fin = NULL;
        }
        return;  
    }
    
    sample_rate = ((((((unsigned)buffer24[27] << 8) | buffer24[26]) << 8) | buffer24[25]) << 8) | buffer24[24];
    channels = 2; 
    char prop_dump_value[10];
    property_get("iriver.flac.bps", prop_dump_value, "false");
    if(!strcmp(prop_dump_value,"true")) {
        bps = 16;        
    } else {
        bps = 24;
    }

    if(bps == 16) {
        total_samples_wav = (((((((unsigned)buffer24[43] << 8) | buffer24[42]) << 8) | buffer24[41]) << 8) | buffer24[40]) / 4; 
        fseek(fin, 44, SEEK_SET);
        ALOGI("wav_encodeTo_flac_DEMO_API ### 16bps ftell : %ld", ftell(fin));
    } else {
        total_samples_wav = (((((((unsigned)buffer24[67] << 8) | buffer24[66]) << 8) | buffer24[65]) << 8) | buffer24[64]) / 6; 
        fseek(fin, 68, SEEK_SET);
        ALOGI("wav_encodeTo_flac_DEMO_API ### 24bps ftell : %ld", ftell(fin));
    }
    if(debug_log) {
        ALOGD("wav_encodeTo_flac_DEMO_API ### bps : %d", bps);
        ALOGI("wav_encodeTo_flac_DEMO_API ### total_samples_wav : %lu", total_samples_wav);
        ALOGI("wav_encodeTo_flac_DEMO_API ### sample_rate : %lu", sample_rate);
        ALOGI("wav_encodeTo_flac_DEMO_API ### channels : %d", channels);    
        ALOGI("wav_encodeTo_flac_DEMO_API ### ftell : %ld", ftell(fin));
    }

    /* allocate the encoder */  
    if((encoder = FLAC__stream_encoder_new()) == NULL) {  
        ALOGE("wav_encodeTo_flac_DEMO_API ### ERROR: allocating encoder");  
        if(fin != NULL) {
            fclose(fin);
            fin = NULL;
        }  
        return;  
    }  

    ok &= FLAC__stream_encoder_set_verify(encoder, true);  

    char prop_com[2];
    property_get("iriver.flac.compress", prop_com, "05");
    if(!strcmp(prop_com,"00")) {
        ok &= FLAC__stream_encoder_set_compression_level(encoder, 0); 
        if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API ### FLAC__stream_encoder_set_compression_level : 0");
    } else if(!strcmp(prop_com,"01")) {
        ok &= FLAC__stream_encoder_set_compression_level(encoder, 1);
        if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API ### FLAC__stream_encoder_set_compression_level : 1");
    } else if(!strcmp(prop_com,"02")) {
        ok &= FLAC__stream_encoder_set_compression_level(encoder, 2);
        if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API ### FLAC__stream_encoder_set_compression_level : 2");
    } else if(!strcmp(prop_com,"03")) {
        ok &= FLAC__stream_encoder_set_compression_level(encoder, 3);
        if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API ### FLAC__stream_encoder_set_compression_level : 3");
    } else if(!strcmp(prop_com,"04")) {
        ok &= FLAC__stream_encoder_set_compression_level(encoder, 4);
        if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API ### FLAC__stream_encoder_set_compression_level : 4");
    } else if(!strcmp(prop_com,"05")) {
        ok &= FLAC__stream_encoder_set_compression_level(encoder, 5);
        if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API ### FLAC__stream_encoder_set_compression_level : 5");
    } else if(!strcmp(prop_com,"06")) {
        ok &= FLAC__stream_encoder_set_compression_level(encoder, 6);
        if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API ### FLAC__stream_encoder_set_compression_level : 6");
    } else if(!strcmp(prop_com,"07")) {
        ok &= FLAC__stream_encoder_set_compression_level(encoder, 7);
        if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API ### FLAC__stream_encoder_set_compression_level : 7");
    } else if(!strcmp(prop_com,"08")) {
        ok &= FLAC__stream_encoder_set_compression_level(encoder, 8);
        if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API ### FLAC__stream_encoder_set_compression_level : 8");
    } else {
        ok &= FLAC__stream_encoder_set_compression_level(encoder, 5);
        if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API ### FLAC__stream_encoder_set_compression_level : 5");
    }

    ok &= FLAC__stream_encoder_set_channels(encoder, channels);  
    ok &= FLAC__stream_encoder_set_bits_per_sample(encoder, bps);  
    ok &= FLAC__stream_encoder_set_sample_rate(encoder, sample_rate);  
    ok &= FLAC__stream_encoder_set_total_samples_estimate(encoder, total_samples_wav);  
    
    /* now add some metadata; we'll add some tags and a padding block */ 
    if(ok) { 
        if((metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == NULL || 
            (metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL || 
            /* there are many tag (vorbiscomment) functions but these are convenient for this particular use: */ 
            !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", "Some Artist") || 
            !FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false) || /* copy=false: let metadata object take control of entry's allocated string */ 
            !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "YEAR", "1984") || 
            !FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false) 
            ) { 
            ALOGE("wav_encodeTo_flac_DEMO_API ### ERROR: out of memory or tag error"); 
            ok = false; 
        } 
        metadata[1]->length = 1234; /* set the padding length */ 
        ok = FLAC__stream_encoder_set_metadata(encoder, metadata, 2); 
    } 
    /* initialize encoder */ 
    if(ok) { 
        init_status = FLAC__stream_encoder_init_file(encoder, outFlacMakeFile, progress_callback, /*client_data=*/NULL);        
        if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) { 
            ALOGE("wav_encodeTo_flac_DEMO_API ### ERROR: initializing encoder: %s\n", FLAC__StreamEncoderInitStatusString[init_status]); 
            ok = false; 
        } 
    } 

    /* read blocks of samples from WAVE file and feed to encoder */ 
    if(ok) { 
        size_t left = (size_t)total_samples_wav; 
        if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API #### channels*(bps/8) : %d", channels*(bps/8));
        while(ok && left) { 
            size_t need = ((left > READSIZE) ? (size_t)READSIZE : (size_t)left);
            if(bps == 16) {
                if(fread(buffer, channels*(bps/8), need, fin) != need) {
                    ALOGE("wav_encodeTo_flac_DEMO_API ### ERROR: reading from WAVE file\n"); 
                    ok = false; 
                } else {
                    ok = true;
                }
            } else if(bps == 24) {
                if(fread(buffer24, channels*(bps/8), need, fin) != need) { 
                    ALOGE("wav_encodeTo_flac_DEMO_API ### ERROR: reading from WAVE file\n"); 
                    ok = false; 
                } else {
                    ok = true;
                }
            }

            if(ok) { 
                if(debug_log) ALOGD("wav_encodeTo_flac_DEMO_API #### need : %u", need);
                /* convert the packed little-endian 16-bit PCM samples from WAVE into an interleaved FLAC__int32 buffer for libFLAC */ 
                size_t i; 
                size_t j = 0;

                if(bps == 16) {
                    for(i = 0; i < need*channels; i++) { 
                        pcmBuffer[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)buffer[2*i+1] << 8) | (FLAC__int16)buffer[2*i]);
                    }
                } else if(bps == 24) {
                    for(i = 0; i < need*channels*(bps/8); i += 3) { 
                        FLAC__int32 x = (FLAC__int32) (( buffer24[i] & 0xff) | 
                                   (FLAC__int32) ((buffer24[i+1] & 0xff) << 8) |
                                   (FLAC__int32) ((buffer24[i+2] & 0xff) << 16));
                        x = (x << 8) >> 8; // sign extension
                        pcmBuffer[j] = x; 
                        j ++;
                    }
                }                
                /* feed samples to encoder */ 
                ok = FLAC__stream_encoder_process_interleaved(encoder, pcmBuffer, need); 
            } 
            left -= need; 
        } 
    } 

    ok &= FLAC__stream_encoder_finish(encoder); 
    if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API ### encoding: %s\n", ok? "succeeded" : "FAILED"); 
    if(debug_log) ALOGI("wav_encodeTo_flac_DEMO_API ### state: %s\n", FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder)]); 

    /* now that encoding is finished, the metadata can be freed */ 
    FLAC__metadata_object_delete(metadata[0]); 
    FLAC__metadata_object_delete(metadata[1]); 

    FLAC__stream_encoder_delete(encoder); 

    if(fin != NULL) {
        fclose(fin);
        fin = NULL;
    } 
    
    return;
}

void progress_callback(const FLAC__StreamEncoder *encoder, 
        FLAC__uint64 bytes_written, 
        FLAC__uint64 samples_written, 
        FLAC__uint32 frames_written, 
        FLAC__uint32 total_frames_estimate, 
        void *client_data) { 
        
    (void)encoder, (void)client_data; 
    if(debug_log) ALOGV("#### progress_callback #### bytes_written : %lld, samples_written : %lld, total_samples_wav : %lu samples, %u/%u frames", bytes_written, samples_written, total_samples_wav, frames_written, total_frames_estimate); 
}

//Test Meta code
FLAC_ENCODER_STATUS wav_encodeTo_flac_Create_meta() {
    FLAC__bool retResult = false;  
    FLAC__StreamMetadata_VorbisComment_Entry entry;
    const char *error;
    FLAC__StreamMetadata *obj;
    
    if(debug_log) ALOGI("#### wav_encodeTo_flac_Create_meta ####");

    if(result != true) {
       return FLAC_ENCODER_STATUS_ERROR_GENERAL;
    }
    
    result = false;
    
    if(0 == (metadata[1] = grabbag__picture_parse_specification("/storage/emulated/0/0.gif", &error))) {
        ALOGE("#### wav_encodeTo_flac_Create_meta #### ERROR! grabbag__picture_parse_specification : %s", error);
    }
    
    if((metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == NULL || 
        (metadata[2] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL ||
        !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", "Artist") || 
        !FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false) || 
        !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "YEAR", "2014") || 
        !FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, false) 
        ) 
    { 
        ALOGE("wav_encodeTo_flac_Create_meta ### ERROR! Making Meta"); 
        return FLAC_ENCODER_STATUS_ERROR_META;
    } 

    metadata[2]->length = 1234; /* set the padding length */ 

    retResult = FLAC__stream_encoder_set_metadata(encoder, metadata, 3); 

    if(retResult != 1) {
        ALOGE("wav_encodeTo_flac_Create_meta ### ERROR! Creating Meta"); 
        return FLAC_ENCODER_STATUS_ERROR_META;
    }

    result = true;
    return FLAC_ENCODER_STATUS_OK;
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
#endif //_FLAC_ENCODER_TEST_SAMPLE_CODE_
