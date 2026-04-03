/*
 * Copyright (C) 2012 The Android Open Source Project
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

//20140709 MetaExtractor_Editor

#define LOG_NDEBUG 0
#define LOG_TAG "flaceditor"
#include <utils/Log.h>

#include "flaceditor.h"

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <strings.h>

#include "flacencoder_albumart.h"

// * 3 for Korea, Japan, China ... UNICODE 16 3bytes, 
// + 10 is padding to protect string overflow
#define METADATA_MAX_LENGTH ((128 * 3) + 10) 
#define METADATA_MAX_ALBUMART_LENGTH (1024 * 1024 * 2) //limited 2MB

//DEBUG
#define _FLAC_METATAG_EDITOR_DEBUG_

namespace android {

//DEBUG 
#ifdef _FLAC_METATAG_EDITOR_DEBUG_
static const int debug_log = 1;
#else
static const int debug_log = 0;
#endif

const char* sFLACMataTags[] = {
    "ALBUM",        //FLAC_METADATA_ED_ALBUM
    "ARTIST",       //FLAC_METADATA_ED_ARTIST
    "ALBUMARTIST",  //FLAC_METADATA_ED_ALBUMARTIST
    "COMPOSER",     //FLAC_METADATA_ED_COMPOSER
    "GENRE",        //FLAC_METADATA_ED_GENRE
    "TITLE",        //FLAC_METADATA_ED_TITLE
    "DATE",         //FLAC_METADATA_ED_YEAR
    "TRACKNUMBER",  //FLAC_METADATA_ED_TRACKNUM
    "UFID",         //FLAC_METADATA_ED_GREACENOTE_UIDOWNER
    "GNID",         //FLAC_METADATA_ED_GREACENOTE_TAGID
    "COPYRIGHT"     //FLAC_METADATA_ED_COPYRIGHT
    //Add Here
};

flaceditor::flaceditor(const char *filePath)
    : mAlbumArtMime(NULL),
      mAlbumArtData(NULL),
      mAlbumArtLength(0),
      mIsHasAlbumArt(false),
      mInitStatus(false) {

    if(debug_log) ALOGI("#### flaceditor::flaceditor() #### [START]");
    
    if(filePath == NULL || strlen(filePath) <= 0) {
        ALOGE("#### flaceditor::flaceditor() #### ERROR! filePath error!");
        return;
    }

    if(debug_log) ALOGI("flaceditor::flaceditor() [START] , filePath : %s", filePath);

    mPictureType = FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER;

    struct stat srcstat; 
    unsigned long lLength = 0;
    int fileNameType = 0;
    
    if (stat(filePath, &srcstat) == 0) {
        if (S_ISREG(srcstat.st_mode)) {
            fileNameType = DT_REG;
        } else if (S_ISDIR(srcstat.st_mode)) {
            fileNameType = DT_DIR;
        }
        
        lLength = srcstat.st_size;
        
        if(lLength <= 0) {
            ALOGE("#### flaceditor::flaceditor() #### ERROR! File Size is invalid!");
            return;
        }
    } else {
        ALOGE("#### flaceditor::flaceditor() #### ERROR! Path is invalid!");
        return;
    }

    if (fileNameType == DT_DIR) {
        ALOGE("#### flaceditor::flaceditor() #### ERROR! Path is not File");
        return;
    }
        
    if (filePath != NULL) {
        mFilepath.setTo(filePath);
        mInitStatus = true;
    }
}

bool flaceditor::flaceditor_open() {
    if(debug_log) ALOGI("#### flaceditor::flaceditor_open() ####");
    
    unsigned long durationTemp = 0;
    int m_nBitRate = 0;
        
    FLAC__StreamMetadata streaminfo;
    FLAC__StreamMetadata *tags = NULL;
    FLAC__StreamMetadata *picture = NULL;

    if(!mInitStatus) {
        ALOGE("#### flaceditor::flaceditor_open() #### ERROR! Init error! mInitStatus = false");
        return false;
    }

    if(FLAC__metadata_get_streaminfo((const char *)mFilepath.string(), &streaminfo) == 1) {
        if(debug_log) {
            ALOGD("#### flaceditor::flaceditor #### FLAC__metadata_get_streaminfo total_samples : %ld ", streaminfo.data.stream_info.total_samples);
            ALOGD("#### flaceditor::flaceditor #### FLAC__metadata_get_streaminfo sample_rate : %d ", streaminfo.data.stream_info.sample_rate);
        }
        durationTemp = (unsigned long)(streaminfo.data.stream_info.total_samples / streaminfo.data.stream_info.sample_rate);
        if(debug_log)  ALOGD("#### flaceditor::flaceditor #### durationTemp : %ld", durationTemp);

        if(FLAC__metadata_get_tags((const char *)mFilepath.string(), &tags) == true && tags != NULL) {
            for (int count = 0; count < (int)tags->data.vorbis_comment.num_comments; count++) {            
                if(debug_log) ALOGD("#### flaceditor::flaceditor #### FLAC__metadata_get_streaminfo TAG : %s, count : %d", tags->data.vorbis_comment.comments[count].entry, tags->data.vorbis_comment.num_comments); 
    
                char *tagMetaData = NULL;
                int checkNum = 0;
                
                if(tags->data.vorbis_comment.comments[count].length <= METADATA_MAX_LENGTH) {
                    tagMetaData = strchr((const char *)tags->data.vorbis_comment.comments[count].entry, '=');
                } else {
                    ALOGE("#### flaceditor::flaceditor #### ERROR! length is over! length : %d" , tags->data.vorbis_comment.comments[count].length);  
                }
                
                if((tagMetaData != NULL) && (strlen(tagMetaData) > 0))
                    ALOGE("#### flaceditor::flaceditor #### FLAC__metadata_get_streaminfo tagMetaData : %s", tagMetaData);   

                if(tagMetaData == NULL) {
                    continue;
                }
                
                for (int i = 0; i < FLAC_METADATA_ED_END; ++i) {
                    if(strncmp((const char *)tags->data.vorbis_comment.comments[count].entry, sFLACMataTags[i], strlen(sFLACMataTags[i])) == 0) {
                        if(i == FLAC_METADATA_ED_ALBUM) {
                            if(strncmp((const char *)tags->data.vorbis_comment.comments[count].entry, sFLACMataTags[FLAC_METADATA_ED_ALBUMARTIST], strlen(sFLACMataTags[FLAC_METADATA_ED_ALBUMARTIST])) == 0) {
                                checkNum = FLAC_METADATA_ED_ALBUMARTIST;   
                            } else {
                                checkNum = i;
                            }
                        } else {
                            checkNum = i;
                        }
                        break;
                    }
                }                
                flaceditor_setTags(checkNum, tagMetaData+1);
            }            
            FLAC__metadata_object_delete(tags);
        } else {
            ALOGE("#### flaceditor::flaceditor_open() #### ERROR! FLAC__metadata_get_tags = false");
            mInitStatus = false;
            flaceditor_release();
            return false;
        }

        if(FLAC__metadata_get_picture((const char *)mFilepath.string(), 
                                      &picture, 
                                      (FLAC__StreamMetadata_Picture_Type)(-1), 
                                      0, 
                                      0, 
                                      (unsigned)(-1), 
                                      (unsigned)(-1), 
                                      (unsigned)(-1), 
                                      (unsigned)(-1)) == true && picture != NULL) {

            if(debug_log) ALOGI("#### flaceditor::flaceditor #### FLAC__metadata_get_streaminfo picture->data.picture.type : %d, mPictureType : %d", picture->data.picture.type, mPictureType);
            if(picture->data.picture.type == mPictureType) {
                if(debug_log) {
                    ALOGI("#### flaceditor::flaceditor #### FLAC__metadata_get_streaminfo mime_type : %s", picture->data.picture.mime_type);
                    ALOGI("#### flaceditor::flaceditor #### FLAC__metadata_get_streaminfo description : %s", picture->data.picture.description);
                    ALOGI("#### flaceditor::flaceditor #### FLAC__metadata_get_streaminfo width : %d", picture->data.picture.width);
                    ALOGI("#### flaceditor::flaceditor #### FLAC__metadata_get_streaminfo height : %d", picture->data.picture.height);
                    ALOGI("#### flaceditor::flaceditor #### FLAC__metadata_get_streaminfo depth : %d", picture->data.picture.depth);
                    ALOGI("#### flaceditor::flaceditor #### FLAC__metadata_get_streaminfo colors : %d", picture->data.picture.colors);
                    ALOGI("#### flaceditor::flaceditor #### FLAC__metadata_get_streaminfo data_length : %d", picture->data.picture.data_length);
                }

               if(NULL == (mAlbumArtMime = (unsigned char *) malloc(strlen((const char *)picture->data.picture.mime_type) + 1))) {
                   ALOGE("#### flaceditor::flaceditor_open() #### ERROR! mAlbumArtMime Malloc failed, Out of memory!");
                   mInitStatus = false;
                   FLAC__metadata_object_delete(picture);
                   flaceditor_release();
                   return false;
               }
               memset(mAlbumArtMime, 0 , strlen((const char *)picture->data.picture.mime_type) + 1);
               memcpy(mAlbumArtMime, picture->data.picture.mime_type, strlen((const char *)picture->data.picture.mime_type));

               mAlbumArtLength = picture->data.picture.data_length;
               if(NULL == (mAlbumArtData = (unsigned char *) malloc(mAlbumArtLength + 1))) {
                   ALOGE("#### flaceditor::flaceditor_open() #### ERROR! mAlbumArtData Malloc failed, Out of memory!");
                   mInitStatus = false;
                   FLAC__metadata_object_delete(picture);
                   flaceditor_release();
                   return false;
               }
               memset(mAlbumArtData, 0 , (mAlbumArtLength + 1));
               memcpy(mAlbumArtData, picture->data.picture.data, mAlbumArtLength);

               mIsHasAlbumArt = true;                
            } else {
                ALOGE("#### flaceditor::flaceditor_open() #### fail! mPictureType is wrong, mIsHasAlbumArt = false");
                mIsHasAlbumArt = false;
            }            
            FLAC__metadata_object_delete(picture);            
        } else {
            ALOGE("#### flaceditor::flaceditor_open() #### fail! No Picture, mIsHasAlbumArt = false");
            mIsHasAlbumArt = false; 
        }
    } else {
        ALOGE("####flaceditor::flaceditor #### FLAC__metadata_get_streaminfo Init error!");
        mInitStatus = false;
        flaceditor_release();
        return false;
    }

    //DEBUG DUMP
    flaceditor_metaTagDump();

    return true;
}

void flaceditor::flaceditor_setTags(int tagNum, char* tagData) {    
    if(debug_log) ALOGI("#### flaceditor::flaceditor_setTags() #### tagNum : %d, tagData : %s", tagNum, tagData);
    
    switch(tagNum) {
        case FLAC_METADATA_ED_ALBUM:
            if(debug_log) ALOGI("#### flaceditor::flaceditor_open() FLAC_METADATA_ALBUM");
            if (tagData != NULL) {
                mKeyAlbum.clear();
                mKeyAlbum.setTo(tagData);
            }
        break;
        case FLAC_METADATA_ED_ARTIST:
            if(debug_log) ALOGI("#### flaceditor::flaceditor_open() FLAC_METADATA_ARTIST");
            if (tagData != NULL) {
                mKeyArtist.clear();
                mKeyArtist.setTo(tagData);
            }
        break;
        case FLAC_METADATA_ED_ALBUMARTIST:
            if(debug_log) ALOGI("#### flaceditor::flaceditor_open() FLAC_METADATA_ALBUMARTIST");
            if (tagData != NULL) {
                mKeyAlbumArtist.clear();
                mKeyAlbumArtist.setTo(tagData);
            }
        break;
        case FLAC_METADATA_ED_COMPOSER:
            if(debug_log) ALOGI("#### flaceditor::flaceditor_open() FLAC_METADATA_COMPOSER");
            if (tagData != NULL) {
                mKeyComposer.clear();
                mKeyComposer.setTo(tagData);
            }
        break;
        case FLAC_METADATA_ED_GENRE:
            if(debug_log) ALOGI("#### flaceditor::flaceditor_open() FLAC_METADATA_GENRE");
            if (tagData != NULL) {
                mKeyGenre.clear();
                mKeyGenre.setTo(tagData);
            }
        break;
        case FLAC_METADATA_ED_TITLE:
            if(debug_log) ALOGI("#### flaceditor::flaceditor_open() FLAC_METADATA_TITLE");
            if (tagData != NULL) {
                mKeyTitle.clear();
                mKeyTitle.setTo(tagData);
            }
        break;
        case FLAC_METADATA_ED_YEAR:
            if(debug_log) ALOGI("#### flaceditor::flaceditor_open() FLAC_METADATA_YEAR");
            if (tagData != NULL) {
                mKeyYear.clear();
                mKeyYear.setTo(tagData);
            }
        break;
        case FLAC_METADATA_ED_TRACKNUM:
            if(debug_log) ALOGI("#### flaceditor::flaceditor_open() FLAC_METADATA_TRACKNUM");
            if (tagData != NULL) {
                mKeyCDTrackNumber.clear();
                mKeyCDTrackNumber.setTo(tagData);
            }
        break;
        case FLAC_METADATA_ED_GREACENOTE_UIDOWNER: //Ablum TagID
            if(debug_log) ALOGI("#### flaceditor::flaceditor_open() FLAC_METADATA_GREACENOTE_UIDOWNER");
            if (tagData != NULL) {
                mKeyGraceNoteUIDOwner.clear();
                mKeyGraceNoteUIDOwner.setTo(tagData);
            }
        break;
        case FLAC_METADATA_ED_GREACENOTE_TAGID: //Track TagID
            if(debug_log) ALOGI("#### flaceditor::flaceditor_open() FLAC_METADATA_GREACENOTE_TAGID");
            if (tagData != NULL) {
                mKeyGraceNoteTAGID.clear();
                mKeyGraceNoteTAGID.setTo(tagData);
            }
        break;
        case FLAC_METADATA_ED_COPYRIGHT:
            if(debug_log) ALOGI("#### flaceditor::flaceditor_open() FLAC_METADATA_COPYRIGHT");
            if (tagData != NULL) {
                mKeyCopyright.clear();
                mKeyCopyright.setTo(tagData);
            }
        break;
    }  
}   
        
char* flaceditor::flaceditor_getAlbum() {
    if(mKeyAlbum.length() > 0)
        return (char *)mKeyAlbum.string();

    return NULL;
}
    
char* flaceditor::flaceditor_getArtist() {
    if(mKeyArtist.length() > 0)
        return (char *)mKeyArtist.string();

    return NULL;
}

char* flaceditor::flaceditor_getAlbumArtist() {
    if(mKeyAlbumArtist.length() > 0)
        return (char *)mKeyAlbumArtist.string();

    return NULL;
}

char* flaceditor::flaceditor_getComposer() {
    if(mKeyComposer.length() > 0)
        return (char *)mKeyComposer.string();

    return NULL;
}

char* flaceditor::flaceditor_getGenre() {
    if(mKeyGenre.length() > 0)
        return (char *)mKeyGenre.string();

    return NULL;
}

char* flaceditor::flaceditor_getTitle() {
    if(mKeyTitle.length() > 0)
        return (char *)mKeyTitle.string();

    return NULL;
}

char* flaceditor::flaceditor_getYear() {
    if(mKeyYear.length() > 0)
        return (char *)mKeyYear.string();

    return NULL;
}

char* flaceditor::flaceditor_getCDTrackNumber() {
    if(mKeyCDTrackNumber.length() > 0)
        return (char *)mKeyCDTrackNumber.string();

    return NULL;
}

char* flaceditor::flaceditor_getGraceNoteUIDOwner() {
    if(mKeyGraceNoteUIDOwner.length() > 0)
        return (char *)mKeyGraceNoteUIDOwner.string();

    return NULL;
}

char* flaceditor::flaceditor_getGraceNoteTAGID() {
    if(mKeyGraceNoteTAGID.length() > 0)
        return (char *)mKeyGraceNoteTAGID.string();

    return NULL;
}

char* flaceditor::flaceditor_getCopyright() {
    if(mKeyCopyright.length() > 0)
        return (char *)mKeyCopyright.string();

    return NULL;
}

bool flaceditor::flaceditor_getIsAlbumArt() {
    return mIsHasAlbumArt;
}

char* flaceditor::flaceditor_getAlbumArtMime() {
    if(mIsHasAlbumArt)
        return (char *)mAlbumArtMime;

    return NULL;
}

char* flaceditor::flaceditor_getAlbumArtData() {   
    if(mIsHasAlbumArt)
        return (char *)mAlbumArtData;

    return NULL;
}

int flaceditor::flaceditor_getAlbumArtLength() {
    if(mIsHasAlbumArt)
        return (int)mAlbumArtLength;

    return 0;
}

bool flaceditor::flaceditor_save(const char* album, const char* artist,
                                 const char* albumartist, const char* composer, 
                                 const char* genre, const char* title,
                                 const char* year, const char* tracknumber, 
                                 const char* ufid, const char* gnid, 
                                 const char* copyright,
                                 const char* albumartdata, const int albumartlength) {

    if(debug_log) ALOGI("#### flaceditor::flaceditor_save() ####");
   
    bool mRetStatus = true;
    mPictureType = FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER; //default

    if(mFilepath.length() <= 0 || strlen(mFilepath.string()) <= 0) {
        ALOGE("#### flaceditor::flaceditor_save() #### ERROR! filePath error! Init");
        return false;
    }
    
    FLAC__Metadata_Chain* metaChain = NULL;

    if(NULL != (metaChain = FLAC__metadata_chain_new())) {        
        FLAC__metadata_chain_read(metaChain, (const char *)mFilepath.string());
        FLAC__StreamMetadata *block = NULL;
        FLAC__Metadata_Iterator *metaIterator = NULL;
        
        if(NULL == (metaIterator = FLAC__metadata_iterator_new())) { 
            ALOGE("#### flaceditor::flaceditor_save() #### ERROR! Malloc Fail! FLAC__metadata_iterator_new");
            if(metaChain != NULL)
                FLAC__metadata_chain_delete(metaChain);
            return false;
        }
        
        FLAC__metadata_iterator_init(metaIterator, metaChain);
        bool checkContinue = true;
    
        //////////////////////////////////////////////////////////////////////////
        // deleing existed tags
        if(debug_log) ALOGW("#### flaceditor::flaceditor_save() #### Deleting Meta tags");
        do {            
            if(NULL == (block = FLAC__metadata_iterator_get_block(metaIterator))) {
                ALOGE("#### flaceditor::flaceditor_save() #### ERROR! FLAC__metadata_iterator_get_block");
                if(metaIterator != NULL)
                    FLAC__metadata_iterator_delete(metaIterator);
                if(metaChain != NULL)
                    FLAC__metadata_chain_delete(metaChain);
                return false;
            }
            
            if(block!= NULL) {
                if(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
                    if(!FLAC__metadata_iterator_delete_block(metaIterator, true)) {
                        ALOGE("#### flaceditor::flaceditor_save() #### ERROR! COMMENT FLAC__metadata_iterator_delete_block");
                        if(metaIterator != NULL)
                            FLAC__metadata_iterator_delete(metaIterator);                        
                        if(metaChain != NULL)
                            FLAC__metadata_chain_delete(metaChain);                        
                        return false;
                    }
                }
                if(block->type == FLAC__METADATA_TYPE_PICTURE) {
                    if(!FLAC__metadata_iterator_delete_block(metaIterator, true)) {
                        ALOGE("#### flaceditor::flaceditor_save() #### ERROR! PICTURE FLAC__metadata_iterator_delete_block");
                        if(metaIterator != NULL)
                            FLAC__metadata_iterator_delete(metaIterator);                        
                        if(metaChain != NULL)
                            FLAC__metadata_chain_delete(metaChain);                        
                        return false;
                    }
                }
            }
            checkContinue = FLAC__metadata_iterator_next(metaIterator);
        } while(checkContinue);

        //////////////////////////////////////////////////////////////////////////
        // Saving tags
        FLAC__metadata_iterator_init(metaIterator, metaChain);
        
        if(debug_log) ALOGW("#### flaceditor::flaceditor_save() #### Saving Meta tags");
        
        do {
            if(NULL == (block = FLAC__metadata_iterator_get_block(metaIterator))) {
                if(metaIterator != NULL)
                    FLAC__metadata_iterator_delete(metaIterator);
                if(metaChain != NULL)
                    FLAC__metadata_chain_delete(metaChain);
                return false;
            }
            
            if(block != NULL) {
                if(block->type == FLAC__METADATA_TYPE_STREAMINFO) {
                    mRetStatus = flaceditor_addVorbisComment(metaIterator, 
                                             album, artist,
                                             albumartist, composer, 
                                             genre, title,
                                             year, tracknumber, 
                                             ufid, gnid, 
                                             copyright);
                    if(!mRetStatus) {
                        ALOGE("#### flaceditor::flaceditor_save() #### ERROR! flaceditor_addVorbisComment");
                        if(metaIterator != NULL)
                            FLAC__metadata_iterator_delete(metaIterator);
                        if(metaChain != NULL)
                            FLAC__metadata_chain_delete(metaChain);
                        return mRetStatus;
                    }

                    if(albumartdata != NULL) {
                        if(albumartlength <= METADATA_MAX_ALBUMART_LENGTH && albumartlength > 0 && strlen(albumartdata) > 0) {                        
                            mRetStatus = flaceditor_addPicture(metaIterator, albumartdata, albumartlength);
                            
                            if(!mRetStatus) {
                                ALOGE("#### flaceditor::flaceditor_save() #### ERROR! flaceditor_addPicture");
                                if(metaIterator != NULL)
                                    FLAC__metadata_iterator_delete(metaIterator);                                
                                if(metaChain != NULL)
                                    FLAC__metadata_chain_delete(metaChain);
                                return mRetStatus;
                            }
                        }
                    }
                    break;
                }
            }
            checkContinue = FLAC__metadata_iterator_next(metaIterator);
        } while(checkContinue);
        
        FLAC__metadata_iterator_delete(metaIterator);
        mRetStatus = FLAC__metadata_chain_write(metaChain, true, true);
        if(!mRetStatus) 
            ALOGE("#### flaceditor::flaceditor_save() #### ERROR! FLAC__metadata_chain_write");
        FLAC__metadata_chain_delete(metaChain);    
    }
       
    return mRetStatus;
}

bool flaceditor::flaceditor_addPicture(FLAC__Metadata_Iterator* metaIterator, 
                          const char* albumartdata, 
                          const int albumartlength) {
                          
    if(debug_log) ALOGI("#### flaceditor::flaceditor_addPicture() ####");
    
    if(albumartdata == NULL || strlen(albumartdata) <= 0 ||  albumartlength <= 0) {
        ALOGE("#### flaceditor::flaceditor_addPicture() #### ERROR! albumartdata = NULL");
        return false;
    }

    FLAC__StreamMetadata* picture = NULL;
    const char *error = NULL;
    if(NULL == FLAC__metadata_object_new(FLAC__METADATA_TYPE_PICTURE)) {
        ALOGE("#### flaceditor::flaceditor_addPicture() #### ERROR! FLAC__metadata_object_new");
        return false;
    }    
    if(NULL == (picture = grabbag__picture_parse_buffer_specification((const char *)albumartdata, 
                                                         albumartlength, &error))) {
        ALOGE("#### flaceditor::flaceditor_addPicture() #### ERROR! grabbag__picture_parse_buffer_specification error %s", error);
        return false;
    }

    if(picture != NULL) {
        if(false == FLAC__metadata_iterator_insert_block_after(metaIterator, picture)) {
            ALOGE("#### flaceditor::flaceditor_addPicture() #### ERROR! FLAC__metadata_iterator_insert_block_after");
            return false;
        }
    }

    return true;
}

bool flaceditor::flaceditor_addVorbisComment(FLAC__Metadata_Iterator* metaIterator, 
                                             const char* album, const char* artist,
                                             const char* albumartist, const char* composer, 
                                             const char* genre, const char* title,
                                             const char* year, const char* tracknumber, 
                                             const char* ufid, const char* gnid, 
                                             const char* copyright) {
                                             
    if(debug_log) ALOGI("#### flaceditor::flaceditor_addVorbisComment() ####");

    flaceditor_metaTagSaveDump(album, artist,
                               albumartist, composer, 
                               genre, title,
                               year, tracknumber, 
                               ufid, gnid, 
                               copyright);
        
    if(metaIterator == NULL) {
        ALOGE("#### flaceditor::flaceditor_addVorbisComment() #### ERROR! metaIterator = NULL");
        return false;
    }
    
    FLAC__StreamMetadata* comment = NULL;   
    if(NULL == (comment = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT))) {
        ALOGE("#### flaceditor::flaceditor_addVorbisComment() #### ERROR! comment = NULL");
        return false;
    }
    if(FLAC__metadata_iterator_insert_block_after(metaIterator, comment) == false) {
        ALOGE("#### flaceditor::flaceditor_addVorbisComment() #### ERROR! FLAC__metadata_iterator_insert_block_after");
        return false;
    }

    bool mRetStatus = true;

    for(int i = 0; i < FLAC_METADATA_ED_END; i++) {
        if(mRetStatus == false)
            break;

        switch(i) {
            case FLAC_METADATA_ED_ALBUM: 
            {
                if(album != NULL && strlen((const char *)album) > 0) {
                    if(false == (mRetStatus = flaceditor_addVobisTag(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_ALBUM], album))) {
                        ALOGE("#### flaceditor::flaceditor_addVorbisComment() #### ERROR! FLAC_METADATA_ALBUM");
                    }
                } else {
                    FLAC__metadata_object_vorbiscomment_remove_entry_matching(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_ALBUM]);
                }
            }
            break; 
            case FLAC_METADATA_ED_ARTIST:
            {
                if(artist != NULL && strlen((const char *)artist) > 0) {
                    if(false == (mRetStatus = flaceditor_addVobisTag(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_ARTIST], artist))) {
                        ALOGE("#### flaceditor::flaceditor_addVorbisComment() #### ERROR! FLAC_METADATA_ARTIST");
                    }
                } else {
                    FLAC__metadata_object_vorbiscomment_remove_entry_matching(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_ARTIST]);
                }
            }

            break;
            case FLAC_METADATA_ED_ALBUMARTIST:
            {
                if(albumartist != NULL && strlen((const char *)albumartist) > 0) {
                    if(false == (mRetStatus = flaceditor_addVobisTag(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_ALBUMARTIST], albumartist))) {
                        ALOGE("#### flaceditor::flaceditor_addVorbisComment() #### ERROR! FLAC_METADATA_ALBUMARTIST");
                    }
                } else {
                    FLAC__metadata_object_vorbiscomment_remove_entry_matching(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_ALBUMARTIST]);
                }
            }
            break;
            case FLAC_METADATA_ED_COMPOSER:
            {
                if(composer != NULL && strlen((const char *)composer) > 0) {
                    if(false == (mRetStatus = flaceditor_addVobisTag(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_COMPOSER], composer))) {
                        ALOGE("#### flaceditor::flaceditor_addVorbisComment() #### ERROR! FLAC_METADATA_COMPOSER");
                    }
                } else {
                    FLAC__metadata_object_vorbiscomment_remove_entry_matching(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_COMPOSER]);
                }
            }
            break;
            case FLAC_METADATA_ED_GENRE:
            {
                if(genre != NULL && strlen((const char *)genre) > 0) {
                    if(false == (mRetStatus = flaceditor_addVobisTag(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_GENRE], genre))) {
                        ALOGE("#### flaceditor::flaceditor_addVorbisComment() #### ERROR! FLAC_METADATA_GENRE");
                    }
                } else {
                    FLAC__metadata_object_vorbiscomment_remove_entry_matching(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_GENRE]);
                }
            }
            break;
            case FLAC_METADATA_ED_TITLE:
            {
                if(title != NULL && strlen((const char *)title) > 0) {
                    if(false == (mRetStatus = flaceditor_addVobisTag(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_TITLE], title))) {
                        ALOGE("#### flaceditor::flaceditor_addVorbisComment() #### ERROR! FLAC_METADATA_TITLE");
                    }
                } else {
                    FLAC__metadata_object_vorbiscomment_remove_entry_matching(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_TITLE]);
                }
            }
            break;
            case FLAC_METADATA_ED_YEAR:
            {
                if(year != NULL && strlen((const char *)year) > 0) {
                    if(false == (mRetStatus = flaceditor_addVobisTag(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_YEAR], year))) {
                        ALOGE("#### flaceditor::flaceditor_addVorbisComment() #### ERROR! FLAC_METADATA_YEAR");
                    }
                } else {
                    FLAC__metadata_object_vorbiscomment_remove_entry_matching(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_YEAR]);
                }
            }
            break;
            case FLAC_METADATA_ED_TRACKNUM:
            {
                if(tracknumber != NULL && strlen((const char *)tracknumber) > 0) {
                    if(false == (mRetStatus = flaceditor_addVobisTag(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_TRACKNUM], tracknumber))) {
                        ALOGE("#### flaceditor::flaceditor_addVorbisComment() #### ERROR! FLAC_METADATA_TRACKNUM");
                    }
                } else {
                    FLAC__metadata_object_vorbiscomment_remove_entry_matching(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_TRACKNUM]);
                }
            }
            break;
            case FLAC_METADATA_ED_GREACENOTE_UIDOWNER:
            {
                if(ufid != NULL && strlen((const char *)ufid) > 0) {
                    if(false == (mRetStatus = flaceditor_addVobisTag(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_GREACENOTE_UIDOWNER], ufid))) {
                        ALOGE("#### flaceditor::flaceditor_addVorbisComment() #### ERROR! FLAC_METADATA_GREACENOTE_UIDOWNER");
                    }
                } else {
                    FLAC__metadata_object_vorbiscomment_remove_entry_matching(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_GREACENOTE_UIDOWNER]);
                }
            }
            break;
            case FLAC_METADATA_ED_GREACENOTE_TAGID:
            {
                if(gnid != NULL && strlen((const char *)gnid) > 0) {
                    if(false == (mRetStatus = flaceditor_addVobisTag(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_GREACENOTE_TAGID], gnid))) {
                        ALOGE("#### flaceditor::flaceditor_addVorbisComment() #### ERROR! FLAC_METADATA_GREACENOTE_TAGID");
                    }
                } else {
                    FLAC__metadata_object_vorbiscomment_remove_entry_matching(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_GREACENOTE_TAGID]);
                }
            }
            break;
            case FLAC_METADATA_ED_COPYRIGHT:
            {
                if(copyright != NULL && strlen((const char *)copyright) > 0) {
                    if(false == (mRetStatus = flaceditor_addVobisTag(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_COPYRIGHT], copyright))) {
                        ALOGE("#### flaceditor::flaceditor_addVorbisComment() #### ERROR! FLAC_METADATA_COPYRIGHT");
                    }
                } else {
                    FLAC__metadata_object_vorbiscomment_remove_entry_matching(comment, 
                        sFLACMataTags[FLAC_METADATA_ED_COPYRIGHT]);
                }
            }
            break;           
         }
    }
    
    return mRetStatus;
}

bool flaceditor::flaceditor_addVobisTag(FLAC__StreamMetadata *block, const char* vobistag, 
                                        const char* vobistagdata) {
                                        
    if(debug_log) ALOGI("#### flaceditor::flaceditor_addVobisTag() ####");
    
    FLAC__StreamMetadata_VorbisComment_Entry entry;
    String8 vobisTagFull;
    
    if(vobistag == NULL || strlen(vobistag) <= 0) {
        ALOGE("#### flaceditor::flaceditor_addVobisTag() #### ERROR! vobistag == NULL");
        return false;
    }
    if(vobistagdata == NULL || strlen(vobistagdata) <= 0) {
        ALOGE("#### flaceditor::flaceditor_addVobisTag() #### ERROR! vobistagdata == NULL");
        return false;
    }
    
    vobisTagFull.setTo(vobistag);
    vobisTagFull.append("=");
    vobisTagFull.append(vobistagdata);
    
    if(debug_log) ALOGI("#### flaceditor::flaceditor_addVobisTag() #### length : %d, vobisTagFull : %s", strlen((const char *)vobisTagFull.string()), vobisTagFull.string());
    
    entry.entry = (FLAC__byte *)vobisTagFull.string();
    if(METADATA_MAX_LENGTH < strlen((const char *)vobisTagFull.string())) {
        entry.length = METADATA_MAX_LENGTH;
        ALOGE("#### flaceditor::flaceditor_addVobisTag() #### ERROR! Length is over!");
        //Unimplemented Cut and Copy String  
    } else {
        entry.length = strlen((const char *)vobisTagFull.string());
    }
    
    if(!FLAC__metadata_object_vorbiscomment_replace_comment(block, entry, false, true)) {
        ALOGE("#### flaceditor::flaceditor_addVobisTag() #### ERROR! FLAC__metadata_object_vorbiscomment_replace_comment");
        return false;
    }
    
    return true;
}

void flaceditor::flaceditor_release() {
    
    if(debug_log) ALOGI("#### flaceditor::flaceditor_release() ####");
    
    if(mFilepath.length() > 0) {
        if(debug_log) ALOGI("flaceditor::~flaceditor mFilepath : %s", mFilepath.string());
        mFilepath.clear();    
    }
    if(mKeyAlbum.length() > 0) {
        if(debug_log) ALOGI("flaceditor::~flaceditor mKeyAlbum : %s", mKeyAlbum.string());
        mKeyAlbum.clear();    
    }
    if(mKeyArtist.length() > 0) {
        if(debug_log) ALOGI("flaceditor::~flaceditor mKeyArtist : %s", mKeyArtist.string());
        mKeyArtist.clear();    
    }
    if(mKeyAlbumArtist.length() > 0) {
        if(debug_log) ALOGI("flaceditor::~flaceditor mKeyAlbumArtist : %s", mKeyAlbumArtist.string());
        mKeyAlbumArtist.clear();    
    }
    if(mKeyComposer.length() > 0) {
        if(debug_log) ALOGI("flaceditor::~flaceditor mKeyComposer : %s", mKeyComposer.string());
        mKeyComposer.clear();    
    }
    if(mKeyGenre.length() > 0) {
        if(debug_log) ALOGI("flaceditor::~flaceditor mKeyGenre : %s", mKeyGenre.string());
        mKeyGenre.clear();    
    }
    if(mKeyTitle.length() > 0) {
        if(debug_log) ALOGI("flaceditor::~flaceditor mKeyTitle : %s", mKeyTitle.string());
        mKeyTitle.clear();    
    }
    if(mKeyYear.length() > 0) {
        if(debug_log) ALOGI("flaceditor::~flaceditor mKeyYear : %s", mKeyYear.string());
        mKeyYear.clear();    
    }
    if(mKeyCDTrackNumber.length() > 0) {
        if(debug_log) ALOGI("flaceditor::~flaceditor mKeyCDTrackNumber : %s", mKeyCDTrackNumber.string());
        mKeyCDTrackNumber.clear();    
    }
    if(mKeyGraceNoteUIDOwner.length() > 0) {
        if(debug_log) ALOGI("flaceditor::~flaceditor mKeyGraceNoteUIDOwner : %s", mKeyGraceNoteUIDOwner.string());
        mKeyGraceNoteUIDOwner.clear();    
    }
    if(mKeyGraceNoteTAGID.length() > 0) {
        if(debug_log) ALOGI("flaceditor::~flaceditor mKeyGraceNoteTAGID : %s", mKeyGraceNoteTAGID.string());
        mKeyGraceNoteTAGID.clear();    
    }
    if(mKeyCopyright.length() > 0) {
        if(debug_log) ALOGI("flaceditor::~flaceditor mKeyCopyright : %s", mKeyCopyright.string());
        mKeyCopyright.clear();    
    }
    if(mAlbumArtMime != NULL) {
        if(debug_log) ALOGI("flaceditor::~flaceditor mAlbumArtMime");
        free(mAlbumArtMime);
        mAlbumArtMime = NULL;            
    }    
    if(mAlbumArtData != NULL) {
        if(debug_log) ALOGI("flaceditor::~flaceditor mAlbumArtData");
        free(mAlbumArtData);
        mAlbumArtData = NULL;            
    }
    
    mAlbumArtLength = 0;
    mPictureType = FLAC__STREAM_METADATA_PICTURE_TYPE_OTHER;
    mIsHasAlbumArt = false; 
}
    
flaceditor::~flaceditor() {
    if(debug_log) ALOGI("#### flaceditor::~flaceditor() #### [END]");

    if(!mInitStatus) {
        ALOGE("#### flaceditor::~flaceditor() #### ERROR! Init error! mInitStatus = false");
    }
    
    flaceditor_release();
}

void flaceditor::flaceditor_metaTagDump() {
#ifdef _FLAC_METATAG_EDITOR_DEBUG_
    //Test Debug
    if(flaceditor_getAlbum() != NULL)
        ALOGD("#### flaceditor_getAlbum() : %s", flaceditor_getAlbum());    
    if(flaceditor_getArtist() != NULL)
        ALOGD("#### flaceditor_getArtist() : %s", flaceditor_getArtist());
    if(flaceditor_getAlbumArtist() != NULL)
        ALOGD("#### flaceditor_getAlbumArtist() : %s", flaceditor_getAlbumArtist());
    if(flaceditor_getComposer() != NULL)
        ALOGD("#### flaceditor_getComposer() : %s", flaceditor_getComposer());
    if(flaceditor_getGenre() != NULL)
        ALOGD("#### flaceditor_getGenre() : %s", flaceditor_getGenre());
    if(flaceditor_getTitle() != NULL)
        ALOGD("#### flaceditor_getTitle() : %s", flaceditor_getTitle());
    if(flaceditor_getYear() != NULL)
        ALOGD("#### flaceditor_getYear() : %s", flaceditor_getYear());
    if(flaceditor_getCDTrackNumber() != NULL)
        ALOGD("#### flaceditor_getCDTrackNumber() : %s", flaceditor_getCDTrackNumber());
    if(flaceditor_getGraceNoteUIDOwner() != NULL)
        ALOGD("#### flaceditor_getGraceNoteUIDOwner() : %s", flaceditor_getGraceNoteUIDOwner());
    if(flaceditor_getGraceNoteTAGID() != NULL)
        ALOGD("#### flaceditor_getGraceNoteTAGID() : %s", flaceditor_getGraceNoteTAGID());
    if(flaceditor_getCopyright() != NULL)
        ALOGD("#### flaceditor_getCopyright() : %s", flaceditor_getCopyright());
    if(flaceditor_getAlbumArtMime() != NULL)
        ALOGD("#### flaceditor_getAlbumArtMime() : %s", flaceditor_getAlbumArtMime());  
#endif

    return;
}

void flaceditor::flaceditor_metaTagSaveDump(const char* album, const char* artist,
                                            const char* albumartist, const char* composer, 
                                            const char* genre, const char* title,
                                            const char* year, const char* tracknumber, 
                                            const char* ufid, const char* gnid, 
                                            const char* copyright) {
#ifdef _FLAC_METATAG_EDITOR_DEBUG_
    //Test Debug
    if(album != NULL && strlen(album) > 0)
        ALOGD("#### Saving album : %s", album);    
    if(artist != NULL && strlen(artist) > 0)
        ALOGD("#### Saving artist : %s", artist);
    if(albumartist != NULL && strlen(albumartist) > 0)
        ALOGD("#### Saving albumartist : %s", albumartist);
    if(composer != NULL && strlen(composer) > 0)
        ALOGD("#### Saving composer : %s", composer);
    if(genre != NULL && strlen(genre) > 0)
        ALOGD("#### Saving genre : %s", genre);
    if(title != NULL && strlen(title) > 0)
        ALOGD("#### Saving title : %s", title);
    if(year != NULL && strlen(year) > 0)
        ALOGD("#### Saving year : %s", year);
    if(tracknumber != NULL && strlen(tracknumber) > 0)
        ALOGD("#### Saving tracknumber : %s", tracknumber);
    if(ufid != NULL && strlen(ufid) > 0)
        ALOGD("#### Saving ufid : %s", ufid);
    if(gnid != NULL && strlen(gnid) > 0)
        ALOGD("#### Saving gnid : %s", gnid);
    if(copyright != NULL && strlen(copyright) > 0)
        ALOGD("#### Saving copyright : %s", copyright);
#endif

    return;
}

//DEBUG TEST SAMPLE CODE
void tagsFLACTestCode() {
    if(debug_log) ALOGI("#### tagsFLACTestCode ####");
#ifdef _FLAC_METATAG_EDITOR_DEBUG_

#endif

    return;
}

bool tagsGenFLAC(const char *mimeType) {
    if(debug_log) ALOGI("#### tagsGenFLAC ####");
    
    if(mimeType == NULL || strlen(mimeType) <= 0) {
        return false;
    }
    
    if(debug_log) ALOGI("#### tagsGenFLAC #### mimeType : %s", mimeType);
    
    if(memcmp("audio/flac", mimeType, 10)) {
        return false;
    }
    
    return true;
}

} // namespace android
