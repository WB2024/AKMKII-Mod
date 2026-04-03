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

// #define LOG_NDEBUG 0
#define LOG_TAG "waveditor"
#include <utils/Log.h>

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <strings.h>
#include <media/stagefright/MetaData.h>

#include "ID3.h"
#include "waveditor.h"
#include "tag_edit_generator.h"

// * 3 for Korea, Japan, China ... UNICODE 16 3bytes, 
// + 10 is padding to protect string overflow
#define METADATA_MAX_LENGTH ((128 * 3) + 10) 
#define METADATA_MAX_ALBUMART_LENGTH (1024 * 1024 * 2) //limited 2MB

//DEBUG
#define _WAV_METATAG_EDITOR_DEBUG_

namespace android {

// ///////////////////////////////////////////////////////////////////////////////////////////////
#define ntohlL(x)    ( ((x) << 24) | (((x) >> 24) & 255) | (((x) << 8) & 0xff0000) | (((x) >> 8) & 0xff00) )
#define FOURCCL(c1, c2, c3, c4) \
    (c1 << 24 | c2 << 16 | c3 << 8 | c4)
    
#define SKIP_1BIT_VALUE 0x00   //Padding

static unsigned int U32_LE_AT(const unsigned char *ptr) {
    return ptr[3] << 24 | ptr[2] << 16 | ptr[1] << 8 | ptr[0];
}

static void MakeFourCCString(unsigned int x, char *s) {
    s[0] = x >> 24;
    s[1] = (x >> 16) & 0xff;
    s[2] = (x >> 8) & 0xff;
    s[3] = x & 0xff;
    s[4] = '\0';
}

// ///////////////////////////////////////////////////////////////////////////////////////////////
waveditor::waveditor(const char *filePath) 
    : mAlbumArtMime(NULL),
      mAlbumArtData(NULL),
      mAlbumArtLength(0),
      mIsHasAlbumArt(false),
	  mIsAKRecord(false),
      mInitStatus(false),
      mTotalFileLength(0),
      mTotalDataSize(0),
      mDataEndOffset(0),
      mId3Offset(0),
      mListOffset(0),
      mHasID3Tag(false),
      mHasListTag(false) {
	ALOGV("### %s() ###", __FUNCTION__);

    if(filePath == NULL || strlen(filePath) <= 0) {
        ALOGE("### %s() / ERROR! filePath error! ###", __FUNCTION__);
        return;
    }

	ALOGV("### %s() / %s ###", __FUNCTION__, filePath);
    
    struct stat srcstat; 
    int fileNameType = 0;
    
    if (stat(filePath, &srcstat) == 0) {
        if (S_ISREG(srcstat.st_mode)) {
            fileNameType = DT_REG;
        } else if (S_ISDIR(srcstat.st_mode)) {
            fileNameType = DT_DIR;
        }
        
        mTotalFileLength = srcstat.st_size;
        
        if(mTotalFileLength <= 0) {
            ALOGE("### %s() / ERROR! File Size is invalid! ###", __FUNCTION__);
            return;
        }
    } else {
        ALOGE("### %s() / ERROR! Path is invalid! ###", __FUNCTION__);
        return;
    }

    if (fileNameType == DT_DIR) {
        ALOGE("### %s() / ERROR! Path is not File ###", __FUNCTION__);
        return;
    }
        
    if (filePath != NULL) {
        mFilepath.setTo(filePath);
        mInitStatus = true;
    }    
}

bool waveditor::waveditor_open() {
    ALOGV("### %s() ###", __FUNCTION__);

    if(mInitStatus == false) {
        ALOGE("### %s() / ERROR! Init error! mInitStatus = false ###", __FUNCTION__);
        return false;
    }
    
    struct stat srcstat;  
    int fd = -1;
    char *filePath = (char *)mFilepath.string();
    bool fileStatus = false;
    
    if(0 == stat(filePath, &srcstat)) {        
        fd = open(filePath, O_RDONLY | O_LARGEFILE);
        if(fd != -1) {
            fileStatus = true;
        } else {
            ALOGE("### %s() / ERROR! fopen error! ###", __FUNCTION__); 
            return false;
        }
    } else {
        ALOGE("### %s() / ERROR! stat error! ###", __FUNCTION__); 
        return false;
    }

    if(fileStatus) {
        if(waveditor_findWavTag(fd) == false) {
            ALOGE("### %s() / ERROR! Wav Tag Error! ###", __FUNCTION__);
			
            if(fd != -1) {
                close(fd); 
                fd = -1;
            }
            return false;
        }
    } else {
       ALOGE("### %s() / ERROR! waveditor_open() error! ###", __FUNCTION__); 
       return false;
    }

    if(waveditor_isID3Tag()) {
        if(!waveditor_getID3MetaTag(fd)) {
            ALOGE("### %s() / ERROR! Cannot get ID3 Tag ###", __FUNCTION__); 

            if(fd != -1) {
                close(fd); 
                fd = -1;
            }
            return false; 
        }
    } 
	// 2016.02.23 iriver-eddy
	else if(waveditor_isListTag()) {
        if(!waveditor_getListMetaTag(fd)) {
            ALOGE("### %s() / ERROR! Cannot get FourCC Tag ###", __FUNCTION__); 

            if(fd != -1) {
                close(fd); 
                fd = -1;
            }
            return false; 
        }
	} else {
        ALOGE("### %s() / ERROR! There is no ID3 Tag ###", __FUNCTION__); 

		if(fd != -1) {
			close(fd); 
			fd = -1;
		}
        return false; 
    }

    if(fd != -1) {
        close(fd); 
        fd = -1;
    }
    
    return true;    
}

bool waveditor::waveditor_getID3MetaTag(int fd) {
    ALOGV("### %s() / fd = %d ###", __FUNCTION__, fd);
	
	if(fd == -1)
		return false;

    uint64_t offset = waveditor_getID3Offset();
    char id3Chunk[12];

    ALOGV("### %s() / offset = %llu ###", __FUNCTION__, offset);

    if(lseek64(fd, offset, SEEK_SET) < 0) {
        ALOGE("### %s() / lseek64 Error! / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
        return false;
    }

    if(read(fd, id3Chunk, 12) < 12) {
        ALOGE("### %s() / ERROR! failed to read Id3 tag ###", __FUNCTION__);
        return false;
    }
    
    if ((memcmp(id3Chunk, "id3", 3) == 0) && (memcmp(&id3Chunk[8], "ID3", 3) == 0)) {
        size_t chunkTagSize = U32_LE_AT((unsigned char*) &id3Chunk[4]);
        offset += 8;
        
        if(chunkTagSize > (METADATA_MAX_ALBUMART_LENGTH) /* 2M byte */) {
            ALOGE("### %s() / ERROR! id3 chunkSize = %u ###", __FUNCTION__, chunkTagSize);
            return false;
        }
        
        char *id3_tag = NULL;
        
        if(chunkTagSize > 0) {
            id3_tag = (char*) malloc(chunkTagSize + 1);
            if(id3_tag == NULL) {
                ALOGE("### %s() / Error! id3_tag == NULL ###", __FUNCTION__);
                return false;
            }        
            memset(id3_tag, 0, chunkTagSize);
        } else {
            ALOGE("### %s() / Error! chunkTagSize size is Big! ###", __FUNCTION__);
            return false;
        }

        if(lseek64(fd, -4, SEEK_CUR) < 0) {
            ALOGE("### %s() / lseek64 Error! / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
            return false;
        }

        if(read(fd, id3_tag, chunkTagSize) < chunkTagSize) {
            ALOGE("### %s() / failed to get ID3 data ###", __FUNCTION__);
            if(id3_tag != NULL) {
                free(id3_tag); 
                id3_tag = NULL;
            }
            return false;
        }

        ID3 id3((const uint8_t *)id3_tag, chunkTagSize);

        if (!id3.isValid()) {
            ALOGE("### %s() / Invalid ID3TAG ###", __FUNCTION__);
            if(id3_tag != NULL) {
                free(id3_tag); 
                id3_tag = NULL;
            }
            return false;
        }

		struct Map {
			WAV_METADATA_ED_INFO key;
			const char *tag1;
			const char *tag2;
		};

		static const Map kMap[] = {
			{ WAV_METADATA_ED_ALBUM, "TALB", "TAL" },                 //WAV_METADATA_ALBUM
			{ WAV_METADATA_ED_ARTIST, "TPE1", "TP1" },                //WAV_METADATA_ARTIST
			{ WAV_METADATA_ED_ALBUMARTIST, "TPE2", "TP2" },           //WAV_METADATA_ALBUMARTIST
			{ WAV_METADATA_ED_COMPOSER, "TCOM", "TCM" },              //WAV_METADATA_COMPOSER
			{ WAV_METADATA_ED_GENRE, "TCON", "TCO" },                 //WAV_METADATA_GENRE
			{ WAV_METADATA_ED_TITLE, "TIT2", "TT2" },                 //WAV_METADATA_TITLE
			{ WAV_METADATA_ED_YEAR, "TYE", "TYER" },                  //WAV_METADATA_YEAR
			{ WAV_METADATA_ED_TRACKNUM, "TRK", "TRCK" },              //WAV_METADATA_TRACKNUM
			{ WAV_METADATA_ED_GREACENOTE_UIDOWNER, "UFID", "UFID" },  //WAV_METADATA_GREACENOTE_UIDOWNER
			{ WAV_METADATA_ED_GREACENOTE_TAGID, "GNID", "GNID" },     //WAV_METADATA_GREACENOTE_TAGID
			{ WAV_METADATA_ED_COPYRIGHT, "TCR", "TCOP" },             //WAV_METADATA_COPYRIGHT
			{ WAV_METADATA_ED_COMMENTS, "COM", "COMM" },              //WAV_METADATA_ED_COMMENTS
		};

		static const size_t kNumMapEntries = sizeof(kMap) / sizeof(kMap[0]);

        for (size_t i = 0; i < kNumMapEntries; ++i) {
            ID3::Iterator *it = new ID3::Iterator(id3, kMap[i].tag1);
            if (it->done()) {
                delete it;
                it = new ID3::Iterator(id3, kMap[i].tag2);
            }

            if (it->done()) {
                delete it;
                continue;
            }
                
            String8 s;
            it->getString(&s);
            delete it;
            waveditor_setTags((int)kMap[i].key, (char *)s.string());
        }

        size_t dataSize;
        String8 mime;
        const void *data = id3.getAlbumArt(&dataSize, &mime);

        ALOGV("### %s() / dataSize = %ld ###", __FUNCTION__, dataSize);

        if(data) {
            ALOGV("### %s() / Picture data is existed ###", __FUNCTION__);

            if(NULL == (mAlbumArtMime = (unsigned char *) malloc(strlen((const char *)mime.string()) + 1))) {
               ALOGE("### %s() / ERROR! mAlbumArtMime Malloc failed, Out of memory! ###", __FUNCTION__);
               mInitStatus = false;
               if(id3_tag != NULL) {
                   free(id3_tag); 
                   id3_tag = NULL;
               }
               waveditor_release();
               return false;
            }
            memset(mAlbumArtMime, 0 , strlen((const char *)mime.string()) + 1);
            memcpy(mAlbumArtMime, (const char *) mime.string(), strlen((const char *)mime.string()));
            
            mAlbumArtLength = dataSize;

			ALOGV("### %s() / mAlbumArtLength : %ld ###", __FUNCTION__, mAlbumArtLength);

            if(NULL == (mAlbumArtData = (unsigned char *) malloc(mAlbumArtLength + 1))) {
               ALOGE("### %s() / ERROR! mAlbumArtData Malloc failed, Out of memory! ###", __FUNCTION__);
               mInitStatus = false;
               if(id3_tag != NULL) {
                   free(id3_tag); 
                   id3_tag = NULL;
               }
               waveditor_release();
               return false;
            }
            memset(mAlbumArtData, 0 , (mAlbumArtLength + 1));
            memcpy(mAlbumArtData, (const char *)data, mAlbumArtLength);
            
            mIsHasAlbumArt = true; 
        }

        if(id3_tag != NULL) {
            free(id3_tag); 
            id3_tag = NULL;
        }
    }

    waveditor_metaTagDump();
        
    return true;
}

bool waveditor::waveditor_getListMetaTag(int fd) {
    ALOGV("### %s() / fd = %d ###", __FUNCTION__, fd);

	if(fd == -1)
		return false;

    uint64_t offset = waveditor_getListOffset();
    uint8_t listChunk[12];
    off64_t chunkOffset = 0;

    ALOGV("### %s() / offset = %llu ###", __FUNCTION__, offset);

    if(lseek64(fd, offset, SEEK_SET) < 0) {
        ALOGE("### %s() / lseek64 Error! / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
        return false;
    }

    if (read(fd, listChunk, sizeof(listChunk)) < (ssize_t)sizeof(listChunk)) {
        ALOGE("### %s() / failed to read LIST tag ###", __FUNCTION__);
        return false;
    }

    if ((memcmp(listChunk, "LIST", 4) == 0) && (memcmp(&listChunk[8], "INFO", 4) == 0)) {
        uint32_t chunkListSize = U32_LE_AT(&listChunk[4]);
		offset += 12;
        chunkOffset += 12;
        //List_Tag_Size_Limit_1M
        if(chunkListSize > (1024*1024)) {
            ALOGE("### %s() / ERROR / List Tags chunkSize = %u ###", __FUNCTION__, chunkListSize);
            return false;
        }           
        while (chunkListSize > chunkOffset) {    
             uint32_t listPropHdr[2];
               
		    lseek64(fd, offset, SEEK_SET);
            if (read(fd, listPropHdr, 8) < 8) {
                ALOGE("### %s() / failed to read LIST info ###", __FUNCTION__);
                return false;
            }
            
			offset += 8;  
            chunkOffset += 8;
            uint32_t list_chunk_type = ntohlL(listPropHdr[0]);
            uint64_t list_chunk_size = U32_LE_AT((uint8_t *)&listPropHdr[1]);
            uint32_t metadataKey = 0;
            
			ALOGV("### %s() / list_chunk_size = %llu / offset = %llu ###", __FUNCTION__, list_chunk_size, chunkOffset);

            switch(list_chunk_type) {
                case FOURCCL('I', 'P', 'R', 'D'): {
                    ALOGD("### %s() / IPRD ###", __FUNCTION__);  // Product 
                    metadataKey = kKeyCopyright;
                    break;
                }
                case FOURCCL('I', 'N', 'A', 'M'): {
                    ALOGD("### %s() / INAM ###", __FUNCTION__);  // File title, usually Song Name
                    metadataKey = kKeyTitle;
                    break;
                }
                case FOURCCL('I', 'A', 'R', 'T'): {
                    ALOGD("### %s() / IART ###", __FUNCTION__); // Primary Artist
                    metadataKey = kKeyArtist;
                    break;
                }
                case FOURCCL('T', 'A', 'L', 'B'): {
                    ALOGD("### %s() / TALB ###", __FUNCTION__); // Album Title
                    metadataKey = kKeyAlbum;
                    break;
                }
                case FOURCCL('T', 'R', 'C', 'K'): {
                    ALOGD("### %s() / TRCK ###", __FUNCTION__); // one based track number 
                    metadataKey = kKeyCDTrackNumber;
                    break;
                }
                case FOURCCL('T', 'P', 'E', '1'): {
                    ALOGD("### %s() / TEP1 ###", __FUNCTION__); // Lead performer(s)/Soloist(s) 
                    metadataKey = kKeyAlbumArtist;
                    break;
                }
                case FOURCCL('I', 'G', 'N', 'R'): {
                    ALOGD("### %s() / IGNR ###", __FUNCTION__); // one or more genre names 
                    metadataKey = kKeyGenre;
                    break;
                }
                case FOURCCL('I', 'C', 'R', 'D'): {
                    ALOGD("### %s() / ICRD ###", __FUNCTION__); // Year
                    metadataKey = kKeyYear;
                    break;
                }
                case FOURCCL('T', 'C', 'O', 'M'): {
                    ALOGD("### %s() / TCOM ###", __FUNCTION__); // Composer
                    metadataKey = kKeyComposer;                        
                    break;
                }
                //Not Support
                case FOURCCL('U', 'S', 'L', 'T'): {
                    ALOGD("### %s() / USLT ###", __FUNCTION__); // Lyrics
                    break;
                }
                //Not Support
                case FOURCCL('A', 'P', 'I', 'C'): {
                    ALOGD("### %s() / APIC ###", __FUNCTION__); // Picture
                    break;
                }
				// /////////////////////////////////////////////////////////////////////////////////////////
				// 2016.02.12 iriver-eddy [REC_META]
				// AK Recorder UUID
				case FOURCCL('I', 'C', 'M', 'T'): {
					ALOGD("### %s() / ICMT ###", __FUNCTION__); // Picture
					metadataKey = kKeyComments;
					break;
				}
				// AK Recorder Model name
				case FOURCCL('I', 'S', 'F', 'T'): {
					ALOGD("### %s() / ISFT ###", __FUNCTION__); // Picture
					metadataKey = kKeyRecorderModel;
					break;
				}
				case FOURCCL('I', 'C', 'O', 'P'): {
					ALOGD("### %s() / ICOP ###", __FUNCTION__); // Picture
					metadataKey = kKeyCopyright;
					break;
				}
				// /////////////////////////////////////////////////////////////////////////////////////////
                default: {
                    ALOGV("### %s() / default ###", __FUNCTION__);
                    break;  
                }                    
            }
            
            if(metadataKey != 0) {
                if(metadataKey == kKeyAlbumArt) {
                    //Not Support
                } else if (metadataKey == kKeyLyric) {
                    //Not Support
                } else {
					// 2016.02.12 iriver-eddy
					ssize_t length = list_chunk_size + 1;
                    uint8_t listPropData[length];
                    memset(listPropData, 0, length);
                    lseek64(fd, offset, SEEK_SET);
					if (read(fd, listPropData, list_chunk_size) < list_chunk_size) {
                        ALOGE("### %s() / failed to read LIST Prop Datas ###", __FUNCTION__);
                        return false;
                    } 

					// ///////////////////////////////////////////////////////////////////////////
					if(metadataKey == kKeyTitle)
						waveditor_setTags(WAV_METADATA_ED_TITLE, (char *)listPropData);

					else if(metadataKey == kKeyArtist)
						waveditor_setTags(WAV_METADATA_ED_ARTIST, (char *)listPropData);
					
					else if(metadataKey == kKeyAlbum)
						waveditor_setTags(WAV_METADATA_ED_ALBUM, (char *)listPropData);
					
					else if(metadataKey == kKeyAlbumArtist)
						waveditor_setTags(WAV_METADATA_ED_ALBUMARTIST, (char *)listPropData);

					else if(metadataKey == kKeyGenre)
						waveditor_setTags(WAV_METADATA_ED_GENRE, (char *)listPropData);

					else if(metadataKey == kKeyYear)
						waveditor_setTags(WAV_METADATA_ED_YEAR, (char *)listPropData);

					else if(metadataKey == kKeyCopyright)
						waveditor_setTags(WAV_METADATA_ED_COPYRIGHT, (char *)listPropData);

					else if(metadataKey == kKeyCDTrackNumber)
						waveditor_setTags(WAV_METADATA_ED_TRACKNUM, (char *)listPropData);

					else if(metadataKey == kKeyComposer)
						waveditor_setTags(WAV_METADATA_ED_COMPOSER, (char *)listPropData);

					else if(metadataKey == kKeyComments)
						waveditor_setTags(WAV_METADATA_ED_COMMENTS, (char *)listPropData);
                }
            }
			offset += list_chunk_size;
            chunkOffset += list_chunk_size;

            //DEBUG
            //////////////////////////////////////////////////////////
            //uint8_t chunkTypeString[5];
            //MakeFourCCString(list_chunk_type, chunkTypeString);
            //ALOGV("### FOURCCParser::getListMetaData() / chunkTypeString : %s ###", chunkTypeString);
            //////////////////////////////////////////////////////////
            if(chunkListSize <= chunkOffset) {
				ALOGE("### %s() / chunkListSize(%u) <= chunkOffset(%llu) ###", __FUNCTION__, chunkListSize, chunkOffset);
				return true;
            }

            //protect code
            uint8_t skipChr;
            uint16_t maxSkipChar = 0;
            while(1) {
               lseek64(fd, offset, SEEK_SET);
			   if (read(fd, &skipChr, 1) < 1) {
                   ALOGE("### %s() / failed to read Padding ###", __FUNCTION__);
                   return false;
               }
               if(skipChr == SKIP_1BIT_VALUE) {
				   offset += 1; 
                   chunkOffset += 1; 
               } else {
                   break;
               }
               if(maxSkipChar > 3) {
                   ALOGE("### %s() / failed Long Padding ###", __FUNCTION__);
                   return false;
               }
               maxSkipChar++;
           }
        }
        
		waveditor_metaTagDump();

        return true;
    } 
    return false;
}

void waveditor::waveditor_setTags(int tagNum, char* tagData) {    
	ALOGV("### %s() / tagNum = %d / data = %s ###", __FUNCTION__, tagNum, tagData);

//The length od meta data is limited as default value refering to METADATA_MAX_LENGTH
//however when getting meta datas, max length?? 
#if 0
    char *metaCutData = NULL;

    //not yet
    if(strlen(tagData) > METADATA_MAX_LENGTH) {
		ALOGV("### %s() / MetaData Size is over METADATA_MAX_LENGTH! ###", __FUNCTION__);

        metaCutData = (char *) malloc(METADATA_MAX_LENGTH + 1);
        memset(metaCutData, 0, METADATA_MAX_LENGTH + 1);
        memcpy(metaCutData, tagData, METADATA_MAX_LENGTH);
    }
#endif    
    
    switch(tagNum) {
        case WAV_METADATA_ED_ALBUM:
            if (tagData != NULL) {
                mKeyAlbum.clear();
                mKeyAlbum.setTo(tagData);
            }
        break;
        case WAV_METADATA_ED_ARTIST:
			if (tagData != NULL) {
                mKeyArtist.clear();
                mKeyArtist.setTo(tagData);
            }
        break;
        case WAV_METADATA_ED_ALBUMARTIST:
            if (tagData != NULL) {
                mKeyAlbumArtist.clear();
                mKeyAlbumArtist.setTo(tagData);
            }
        break;
        case WAV_METADATA_ED_COMPOSER:
            if (tagData != NULL) {
                mKeyComposer.clear();
                mKeyComposer.setTo(tagData);
            }
        break;
        case WAV_METADATA_ED_GENRE:
            if (tagData != NULL) {
                mKeyGenre.clear();
                mKeyGenre.setTo(tagData);
            }
        break;
        case WAV_METADATA_ED_TITLE:
            if (tagData != NULL) {
                mKeyTitle.clear();
                mKeyTitle.setTo(tagData);
            }
        break;
        case WAV_METADATA_ED_YEAR:
            if (tagData != NULL) {
                mKeyYear.clear();
                mKeyYear.setTo(tagData);
            }
        break;
        case WAV_METADATA_ED_TRACKNUM:
            if (tagData != NULL) {
                mKeyCDTrackNumber.clear();
                mKeyCDTrackNumber.setTo(tagData);
            }
        break;
        case WAV_METADATA_ED_GREACENOTE_UIDOWNER: //Ablum TagID
            if (tagData != NULL) {
                mKeyGraceNoteUIDOwner.clear();
                mKeyGraceNoteUIDOwner.setTo(tagData);
            }
        break;
        case WAV_METADATA_ED_GREACENOTE_TAGID: //Track TagID
            if (tagData != NULL) {
                mKeyGraceNoteTAGID.clear();
                mKeyGraceNoteTAGID.setTo(tagData);
            }
        break;
        case WAV_METADATA_ED_COPYRIGHT:
            if (tagData != NULL) {
                mKeyCopyright.clear();
                mKeyCopyright.setTo(tagData);
            }
        break;
        case WAV_METADATA_ED_COMMENTS:
            if (tagData != NULL) {
                mKeyComments.clear();
                mKeyComments.setTo(tagData);

				// 2015.02.26 iriver-eddy
				if(strlen(tagData) > 5 && memcmp((const char *)tagData, "RFID:", 5) == 0) {
					mIsAKRecord = true;
				}
            }
        break;
    }  
}   

bool waveditor::waveditor_findWavTag(int fd) {
    ALOGV("### %s() / fd = %d / File total size = %llu ###", __FUNCTION__, fd, mTotalFileLength);
    
	if(fd == -1)
		return false;

    char header[12];
    if(read(fd, header, 12) < 12) {
        return false;
    }

    if (memcmp(header, "RIFF", 4) || memcmp(&header[8], "WAVE", 4)) {
        return false;
    }

    unsigned int totalSize = U32_LE_AT((unsigned char*)&header[4]);

    off64_t offset = 12;
    uint64_t remainingSize = totalSize;

    mTotalDataSize = totalSize;
    
    bool dataFoundStatus = false;
	bool cueFoundStatus = false;
	uint64_t dataOffset = 0;
	uint64_t cueOffset = 0;

    while(remainingSize >= 8) {
        char chunkHeader[8];
        if(read(fd, chunkHeader, 8) < 8) {
            ALOGE("### %s() / fread Error! ###", __FUNCTION__);
            return false;
        }

        remainingSize -= 8;
        offset += 8;

        uint64_t chunkSize = U32_LE_AT((unsigned char*)&chunkHeader[4]);

        if(chunkSize > remainingSize) {
            return false;
        }
        
		ALOGV("### %s() / offset : %llu / chunkSize : %llu / remain size : %llu ###", __FUNCTION__, offset, chunkSize, remainingSize);
		ALOGV("### %s() / chunkHeader : %c%c%c%c ###", __FUNCTION__, chunkHeader[0], chunkHeader[1], chunkHeader[2], chunkHeader[3]);

        if(!memcmp(chunkHeader, "data", 4)) {
            dataFoundStatus = true;
        } else if(!memcmp(chunkHeader, "cue ", 4) || !memcmp(chunkHeader, "CUE ", 4)) {
            cueFoundStatus = true;
        }
        
        if(lseek64(fd, chunkSize, SEEK_CUR) < 0) {
            ALOGE("### %s() / lseek64 Error! / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
            return false;
        }

		offset += chunkSize;
        remainingSize -= chunkSize;

		ALOGV("### %s() / offset : %llu ###", __FUNCTION__, offset);

        if(dataFoundStatus && dataOffset == 0) {
			ALOGD("### %s() / WavData is found! ###", __FUNCTION__);
            dataOffset = offset;
        }

        if(cueFoundStatus && cueOffset == 0) {
			ALOGD("### %s() / WavData cue is found! ###", __FUNCTION__);
            cueOffset = offset;
        }

		if(dataFoundStatus && cueFoundStatus)
			break;
    }

	if(cueOffset > dataOffset)
		mDataEndOffset = cueOffset;
	else
		mDataEndOffset = dataOffset;

    if(!dataFoundStatus) {
        ALOGE("### %s() / Error! No finding Data filed! ###", __FUNCTION__);
        return false;
    }

    if(!cueFoundStatus) {
        ALOGE("### %s() / No finding CUE filed! ###", __FUNCTION__);
    }

    if(!waveditor_checkMetaData(fd)) {
        ALOGE("### %s() / Error! %s() ###", __FUNCTION__, __FUNCTION__);
        return false;
    }
    
    return true;    
}

bool waveditor::waveditor_isID3Tag() {
    ALOGV("### %s() ###", __FUNCTION__);

    return mHasID3Tag;
}

uint64_t waveditor::waveditor_getID3Offset() {
    ALOGV("### %s() ###", __FUNCTION__);

    return mId3Offset;
}

bool waveditor::waveditor_isListTag() {
    ALOGV("### %s() ###", __FUNCTION__);

    return mHasListTag;
}

uint64_t waveditor::waveditor_getListOffset() {
    ALOGV("### %s() ###", __FUNCTION__);

    return mListOffset;
}

bool waveditor::waveditor_checkMetaData(int fd) {
    ALOGV("### %s() / fd = %d ###", __FUNCTION__, fd);
    
	if(fd == -1)
		return false;

    uint64_t remainingSize = mTotalFileLength; 
    uint64_t tagOffset = waveditor_getDataOffset();
    bool findMetaStatus = false;
    
	// 2016.02.26 iriver-eddy
	lseek64(fd, mDataEndOffset, SEEK_SET);

    while (remainingSize > tagOffset) {
        unsigned int chunkType[2];

        if(read(fd, chunkType, 8) < 8) {
            ALOGE("### %s() / failed to get meta ###", __FUNCTION__);
            return false;
        }                
        unsigned int chunk_type = ntohlL(chunkType[0]);
        uint64_t chunkRealSize = U32_LE_AT((unsigned char*)&chunkType[1]);

        //DEBUG
        ///////////////////////////////////////////////
        char chunkString[5] = {0,};  
        MakeFourCCString(chunk_type, chunkString);        
        ALOGV("### %s() / chunkString : %s ###", __FUNCTION__, chunkString);        
        ///////////////////////////////////////////////
        
        if (chunk_type == FOURCCL('L', 'I', 'S', 'T')) {
			ALOGD("### %s() / Finding LIST FourCC ###", __FUNCTION__);
			ALOGV("### %s() / FourCC tagOffset : %llu ###", __FUNCTION__, tagOffset);

            mHasListTag = true;
            mListOffset = tagOffset;
            if(mHasID3Tag) {
                findMetaStatus = true;
                break;
            }
        }        
        if ((chunk_type == FOURCCL('i', 'd', '3', 0x20)) || (chunk_type == FOURCCL('I', 'D', '3', 0x20))) {
			ALOGD("### %s() / Finding ID3Tag ###", __FUNCTION__);
			ALOGV("### %s() / ID3Tag tagOffset : %llu ###", __FUNCTION__, tagOffset);

            mHasID3Tag = true;
            mId3Offset = tagOffset;
            if(mHasListTag) {
                findMetaStatus = true;
                break;
            }
        } 
    
        tagOffset += 8;
        tagOffset += chunkRealSize;

        if(lseek64(fd, chunkRealSize, SEEK_CUR) < 0) {
            ALOGE("### %s() / lseek64 Error! / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
            return false;
        }

        if(remainingSize <= tagOffset) 
			break;
        
		if(mHasID3Tag && mHasListTag) {
			ALOGV("### %s() / findMetaStatus (LIST & ID3) META DATAs ###", __FUNCTION__);
            findMetaStatus = true;
            break;
        }
            
        //protect code
        char skipChr[1] = {0};
        int maxSkipChar = 0;
        while(1) {
            if(read(fd, skipChr, 1) < 1) {
                ALOGE("### %s() / failed to read Padding ###", __FUNCTION__);
                return false;
            }
            if(skipChr[0] == SKIP_1BIT_VALUE) {
                tagOffset += 1;
            } else {
                if(lseek64(fd, -1, SEEK_CUR) < 0) {
                    ALOGE("### %s / lseek64 back Error! / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
                    return false;
                }
                break;
            }
            if(maxSkipChar > 3) {
                ALOGE("### %s() / failed Long Padding ###", __FUNCTION__);
                return false;
            }
            maxSkipChar++;
        }        
    }
    
	// 2016.02.23 iriver-eddy
	if(mHasID3Tag || mHasListTag) {
		ALOGD("### %s() / findMetaStatus (LIST & ID3) META DATAs ###", __FUNCTION__);
		findMetaStatus = true;
	}

    if(!findMetaStatus) {
        ALOGE("### %s() / There are no meta tags ###", __FUNCTION__);
        return false;
    }
    
    return true;
}

uint64_t waveditor::waveditor_getDataOffset() {
    ALOGV("### %s() ###", __FUNCTION__);

    return mDataEndOffset;
}

uint64_t waveditor::waveditor_getFileSize() {
    ALOGV("### %s() ###", __FUNCTION__);

    return mTotalFileLength;
}

bool waveditor::waveditor_Truncate(const char * filePath, const off64_t cutOffset) {
    ALOGV("### %s() / cutOffset = %llu ###", __FUNCTION__, cutOffset);

    if(filePath == NULL) {
        ALOGE("### %s() / Error! filePath == NULL ###", __FUNCTION__);
        return false;
    }

    if(strlen(filePath) <= 3) {
        ALOGE("### %s() / Error! filePath length(%d) ###", __FUNCTION__, strlen(filePath));
        return false;
    }
    
    int fd = open(filePath, O_WRONLY | O_LARGEFILE);
	if(fd == -1) {
		ALOGE("### %s() / Failed to open %s / errno = %d / %s ###", __FUNCTION__, filePath, errno, strerror(errno));
		return false;
	}

    if(ftruncate64(fd, cutOffset) != 0) {
        ALOGE("### %s() / Error! fail ftruncate64 / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
        if(fd != -1) {
            close(fd);
            fd = -1;
        }  
        return false;
    }
    
    if(fd != -1) {
        close(fd);
        fd = -1;
    }
    
    return true;
}

bool waveditor::waveditor_save(const char* album, const char* artist,
                               const char* albumartist, const char* composer, 
                               const char* genre, const char* title,
                               const char* year, const char* tracknumber, 
                               const char* ufid, const char* gnid, 
                               const char* copyright,
                               const char* albumartdata, const int albumartlength) {
    ALOGV("### %s() ###", __FUNCTION__);     

    //DEBUG
    waveditor_metaTagSaveDump(album, artist,
                              albumartist, composer, 
                              genre, title,
                              year, tracknumber, 
                              ufid, gnid, 
                              copyright);
    
    int fd = -1;
    int64_t bytes = 0;
    uint64_t tagStartPoint = 0;

    char *filePath = (char *)mFilepath.string();

    if(filePath == NULL) {
        ALOGE("### %s() / Error! filePath == NULL ###", __FUNCTION__);
        return false;
    }

    if(strlen(filePath) <= 3) {
        ALOGE("### %s() / Error! filePath length ###", __FUNCTION__);
        return false;
    }

    tagStartPoint = waveditor_getFileSize() - waveditor_getDataOffset();   

    //if saving action occurs the internal error!, original file will be recovered automatically
    //however How Copy? Rename? Think about this later 
    if(waveditor_Truncate(filePath, (off64_t)waveditor_getDataOffset()) ==  false) {
        ALOGE("### %s() / Error! waveditor_Truncate() ###", __FUNCTION__);
        return false;
    }

    fd = open(filePath, O_RDWR | O_LARGEFILE);
    if(fd == -1) {
        ALOGE("### %s() / open Error! %s / errno = %d / %s ###", __FUNCTION__, filePath, errno, strerror(errno));
        if(fd != -1) {
            close(fd);
            fd = -1;
        }
        return false;
    }

    bytes = lseek64(fd, 0, SEEK_END);    
    if(bytes < 0) {
        ALOGE("### %s() / Error! tagStartPoint is wrong! / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
        if(fd != -1) {
            close(fd);
            fd = -1;
        }
        return false;
    }
    
	ALOGV("### %s() / open fd : %d, bytes : %lld ###", __FUNCTION__, fd, bytes);
    
    wav_editor_tag_generator_gen_init(fd, NULL, NULL, 0, WAV_METADATA_FILEDESC); 

    if(album == NULL) {
        if(waveditor_getAlbum() != NULL) {
			ALOGV("### %s() / mKeyAlbum.string() : %s ###", __FUNCTION__, mKeyAlbum.string());
            wav_editor_tag_generator_gen_add_meta(WAV_METADATA_ALBUM, (const unsigned char *)mKeyAlbum.string());
        }
    } else {
		ALOGV("### %s() / album : %s ###", __FUNCTION__, album);
        wav_editor_tag_generator_gen_add_meta(WAV_METADATA_ALBUM, (const unsigned char *)album);
    }

    if(artist == NULL) {
        if(waveditor_getArtist() != NULL) {
			ALOGV("### %s() / mKeyArtist.string() : %s ###", __FUNCTION__, mKeyArtist.string());
            wav_editor_tag_generator_gen_add_meta(WAV_METADATA_ARTIST, (const unsigned char *)mKeyArtist.string());
        }
    } else {
		ALOGV("### %s() / artist : %s ###", __FUNCTION__, artist);
        wav_editor_tag_generator_gen_add_meta(WAV_METADATA_ARTIST, (const unsigned char *)artist);
    }

    if(albumartist == NULL) {
        if(waveditor_getAlbumArtist() != NULL) {
			ALOGV("### %s() / mKeyAlbumArtist.string() : %s ###", __FUNCTION__, mKeyAlbumArtist.string());
            wav_editor_tag_generator_gen_add_meta(WAV_METADATA_ALBUMARTIST, (const unsigned char *)mKeyAlbumArtist.string());
        }
    } else {
		ALOGV("### %s() / albumartist : %s ###", __FUNCTION__, albumartist);
        wav_editor_tag_generator_gen_add_meta(WAV_METADATA_ALBUMARTIST, (const unsigned char *)albumartist);
    }

    if(composer == NULL) {
        if(waveditor_getComposer() != NULL) {
			ALOGV("### %s() / mKeyComposer.string() : %s ###", __FUNCTION__, mKeyComposer.string());
            wav_editor_tag_generator_gen_add_meta(WAV_METADATA_COMPOSER, (const unsigned char *)mKeyComposer.string());
        }
    } else {
		ALOGV("### %s() / composer : %s ###", __FUNCTION__, composer);
        wav_editor_tag_generator_gen_add_meta(WAV_METADATA_COMPOSER, (const unsigned char *)composer);
    }
        
    if(genre == NULL) {
        if(waveditor_getGenre() != NULL) {
			ALOGV("### %s() / mKeyGenre.string() : %s ###", __FUNCTION__, mKeyGenre.string());
            wav_editor_tag_generator_gen_add_meta(WAV_METADATA_GENRE, (const unsigned char *)mKeyGenre.string());
        }
    } else {
		ALOGV("### %s() / genre : %s ###", __FUNCTION__, genre);
        wav_editor_tag_generator_gen_add_meta(WAV_METADATA_GENRE, (const unsigned char *)genre);
    }
    
    if(title == NULL) {
        if(waveditor_getTitle() != NULL) {
			ALOGV("### %s() / mKeyTitle.string() : %s ###", __FUNCTION__, mKeyTitle.string());
            wav_editor_tag_generator_gen_add_meta(WAV_METADATA_TITLE, (const unsigned char *)mKeyTitle.string());
        }
    } else {
		ALOGV("### %s() / title : %s ###", __FUNCTION__, title);
        wav_editor_tag_generator_gen_add_meta(WAV_METADATA_TITLE, (const unsigned char *)title);
    }
    
    if(year == NULL || mIsAKRecord) {
        if(waveditor_getYear() != NULL) {
			ALOGV("### %s() / mKeyYear.string() : %s ###", __FUNCTION__, mKeyYear.string());
            wav_editor_tag_generator_gen_add_meta(WAV_METADATA_YEAR, (const unsigned char *)mKeyYear.string());
        }
    } else {
		ALOGV("### %s() / year.string() : %s ###", __FUNCTION__, year);
        wav_editor_tag_generator_gen_add_meta(WAV_METADATA_YEAR, (const unsigned char *)year);
    }
    
    if(tracknumber == NULL) {
        if(waveditor_getCDTrackNumber() != NULL) {
			ALOGV("### %s() / mKeyCDTrackNumber.string() : %s ###", __FUNCTION__, mKeyCDTrackNumber.string());
            wav_editor_tag_generator_gen_add_meta(WAV_METADATA_TRACKNUM, (const unsigned char *)mKeyCDTrackNumber.string());
        }
    } else {
		ALOGV("### %s() / tracknumber : %s ###", __FUNCTION__, tracknumber);
        wav_editor_tag_generator_gen_add_meta(WAV_METADATA_TRACKNUM, (const unsigned char *)tracknumber);
    }

    if(ufid == NULL) {
        if(waveditor_getGraceNoteUIDOwner() != NULL) {
			ALOGV("### %s() / mKeyGraceNoteUIDOwner.string() : %s ###", __FUNCTION__, mKeyGraceNoteUIDOwner.string());
            wav_editor_tag_generator_gen_add_meta(WAV_METADATA_GRACENOTE_UIDOWNER, (const unsigned char *)mKeyGraceNoteUIDOwner.string());
        }
    } else {
		ALOGV("### %s() / ufid : %s ###", __FUNCTION__, ufid);
        wav_editor_tag_generator_gen_add_meta(WAV_METADATA_GRACENOTE_UIDOWNER, (const unsigned char *)ufid);
    }

    if(gnid == NULL) {
        if(waveditor_getGraceNoteTAGID() != NULL) {
			ALOGV("### %s() / mKeyGraceNoteTAGID.string() : %s ###", __FUNCTION__, mKeyGraceNoteTAGID.string());
            wav_editor_tag_generator_gen_add_meta(WAV_METADATA_GRACENOTE_TAGID, (const unsigned char *)mKeyGraceNoteTAGID.string());
        }
    } else {
		ALOGV("### %s() / gnid : %s ###", __FUNCTION__, gnid);
        wav_editor_tag_generator_gen_add_meta(WAV_METADATA_GRACENOTE_TAGID, (const unsigned char *)gnid);
    }

    if(copyright == NULL) {
        if(waveditor_getCopyright() != NULL) {
			ALOGV("### %s() / mKeyCopyright.string() : %s ###", __FUNCTION__, mKeyCopyright.string());
            wav_editor_tag_generator_gen_add_meta(WAV_METADATA_COPYRIGHT, (const unsigned char *)mKeyCopyright.string());
        }
    } else {
		ALOGV("### %s() / copyright : %s ###", __FUNCTION__, copyright);
        wav_editor_tag_generator_gen_add_meta(WAV_METADATA_COPYRIGHT, (const unsigned char *)copyright);
    }

	// 2016.02.23 iriver-eddy
	if(waveditor_getComments() != NULL) {
		ALOGV("### %s() / mKeyComments.string() : %s ###", __FUNCTION__, mKeyComments.string());
		wav_editor_tag_generator_gen_add_meta(WAV_METADATA_COMMENTS, (const unsigned char *)mKeyComments.string());
	}

    //Album Art
    if(albumartlength <= METADATA_MAX_ALBUMART_LENGTH && albumartlength > 0 && albumartdata != NULL) {
        if(strlen(albumartdata) > 0) {
            wav_editor_tag_generator_gen_add_albumart(WAV_METADATA_ALBUMART_DATA, (const unsigned char *)albumartdata, albumartlength);
        } else {
			ALOGV("### %s() / No! AlbumArtData, strlen(albumartdata) : %d ###", __FUNCTION__, strlen(albumartdata));
            if(waveditor_getAlbumArtData() != NULL) {
                if(waveditor_getAlbumArtLength() <= METADATA_MAX_ALBUMART_LENGTH) {
   	        		wav_editor_tag_generator_gen_add_albumart(WAV_METADATA_ALBUMART_DATA, (const unsigned char *)waveditor_getAlbumArtData(), waveditor_getAlbumArtLength());
                }
            }
        }
    } else {
		ALOGV("### %s() / No! AlbumArt, albumartlength : %d ###", __FUNCTION__, albumartlength);
        if(waveditor_getAlbumArtData() != NULL) {
            if(waveditor_getAlbumArtLength() <= METADATA_MAX_ALBUMART_LENGTH) {
            wav_editor_tag_generator_gen_add_albumart(WAV_METADATA_ALBUMART_DATA, (const unsigned char *)waveditor_getAlbumArtData(), waveditor_getAlbumArtLength());
            }
        }
    }
    
    if(TAG_GENERATOR_STATUS_OK != wav_editor_tag_generator_gen_meta_write()) {
        ALOGE("### %s() / Error! wav_editor_tag_generator_gen_meta_write! ###", __FUNCTION__);
        wav_editor_tag_generator_gen_finish();
        if(fd != -1) {
            close(fd);
            fd = -1;
        }
        return false;        
    }
    
    wav_editor_tag_generator_gen_finish();
    
    if(fd != -1) {
        close(fd);
        fd = -1;
    }
    
    return true;
}
                                 
void waveditor::waveditor_release() {
	ALOGV("### %s() ###", __FUNCTION__);

    if(mFilepath.length() > 0) {
		ALOGV("### %s() / mFilepath : %s ###", __FUNCTION__, mFilepath.string());
        mFilepath.clear();    
    }
    if(mKeyAlbum.length() > 0) {
		ALOGV("### %s() / mKeyAlbum : %s ###", __FUNCTION__, mKeyAlbum.string());
        mKeyAlbum.clear();    
    }
    if(mKeyArtist.length() > 0) {
		ALOGV("### %s() / mKeyArtist : %s ###", __FUNCTION__, mKeyArtist.string());
        mKeyArtist.clear();    
    }
    if(mKeyAlbumArtist.length() > 0) {
		ALOGV("### %s() / mKeyAlbumArtist : %s ###", __FUNCTION__, mKeyAlbumArtist.string());
        mKeyAlbumArtist.clear();    
    }
    if(mKeyComposer.length() > 0) {
		ALOGV("### %s() / mKeyComposer : %s ###", __FUNCTION__, mKeyComposer.string());
        mKeyComposer.clear();    
    }
    if(mKeyGenre.length() > 0) {
		ALOGV("### %s() / mKeyGenre : %s ###", __FUNCTION__, mKeyGenre.string());
        mKeyGenre.clear();    
    }
    if(mKeyTitle.length() > 0) {
		ALOGV("### %s() / mKeyTitle : %s ###", __FUNCTION__, mKeyTitle.string());
        mKeyTitle.clear();    
    }
    if(mKeyYear.length() > 0) {
		ALOGV("### %s() / mKeyYear : %s ###", __FUNCTION__, mKeyYear.string());
        mKeyYear.clear();    
    }
    if(mKeyCDTrackNumber.length() > 0) {
		ALOGV("### %s() / mKeyCDTrackNumber : %s ###", __FUNCTION__, mKeyCDTrackNumber.string());
        mKeyCDTrackNumber.clear();    
    }
    if(mKeyGraceNoteUIDOwner.length() > 0) {
		ALOGV("### %s() / mKeyGraceNoteUIDOwner : %s ###", __FUNCTION__, mKeyGraceNoteUIDOwner.string());
        mKeyGraceNoteUIDOwner.clear();    
    }
    if(mKeyGraceNoteTAGID.length() > 0) {
		ALOGV("### %s() / mKeyGraceNoteTAGID : %s ###", __FUNCTION__, mKeyGraceNoteTAGID.string());
        mKeyGraceNoteTAGID.clear();    
    }
    if(mKeyCopyright.length() > 0) {
		ALOGV("### %s() / mKeyCopyright : %s ###", __FUNCTION__, mKeyCopyright.string());
        mKeyCopyright.clear();    
    }
    if(mKeyComments.length() > 0) {
		ALOGV("### %s() / mKeyComments : %s ###", __FUNCTION__, mKeyComments.string());
        mKeyComments.clear();    
    }
    if(mAlbumArtMime != NULL) {
        free(mAlbumArtMime);
        mAlbumArtMime = NULL;            
    }    
    if(mAlbumArtData != NULL) {
        free(mAlbumArtData);
        mAlbumArtData = NULL;            
    }
    
    mAlbumArtLength = 0;
    mIsHasAlbumArt = false;
	mIsAKRecord = false;
    mTotalDataSize = 0;
    mTotalFileLength = 0;
    mDataEndOffset = 0;
    mId3Offset = 0;
    mListOffset = 0;
    mHasID3Tag = false;
    mHasListTag = false;
}

waveditor::~waveditor() {
	ALOGV("### %s() ###", __FUNCTION__);

    if(!mInitStatus) {
        ALOGE("### %s() / ERROR! Init error! mInitStatus = false ###", __FUNCTION__);
    }
    
    waveditor_release();
}
        
char* waveditor::waveditor_getAlbum() {
    if(mKeyAlbum.length() > 0)
        return (char *)mKeyAlbum.string();

    return NULL;
}
    
char* waveditor::waveditor_getArtist() {
    if(mKeyArtist.length() > 0)
        return (char *)mKeyArtist.string();

    return NULL;
}

char* waveditor::waveditor_getAlbumArtist() {
    if(mKeyAlbumArtist.length() > 0)
        return (char *)mKeyAlbumArtist.string();

    return NULL;
}

char* waveditor::waveditor_getComposer() {
    if(mKeyComposer.length() > 0)
        return (char *)mKeyComposer.string();

    return NULL;
}

char* waveditor::waveditor_getGenre() {
    if(mKeyGenre.length() > 0)
        return (char *)mKeyGenre.string();

    return NULL;
}

char* waveditor::waveditor_getTitle() {
    if(mKeyTitle.length() > 0)
        return (char *)mKeyTitle.string();

    return NULL;
}

char* waveditor::waveditor_getYear() {
    if(mKeyYear.length() > 0)
        return (char *)mKeyYear.string();

    return NULL;
}

char* waveditor::waveditor_getCDTrackNumber() {
    if(mKeyCDTrackNumber.length() > 0)
        return (char *)mKeyCDTrackNumber.string();

    return NULL;
}

char* waveditor::waveditor_getGraceNoteUIDOwner() {
    if(mKeyGraceNoteUIDOwner.length() > 0)
        return (char *)mKeyGraceNoteUIDOwner.string();

    return NULL;
}

char* waveditor::waveditor_getGraceNoteTAGID() {
    if(mKeyGraceNoteTAGID.length() > 0)
        return (char *)mKeyGraceNoteTAGID.string();

    return NULL;
}

char* waveditor::waveditor_getCopyright() {
    if(mKeyCopyright.length() > 0)
        return (char *)mKeyCopyright.string();

    return NULL;
}

char* waveditor::waveditor_getComments() {
    if(mKeyComments.length() > 0)
        return (char *)mKeyComments.string();

    return NULL;
}

bool waveditor::waveditor_getIsAlbumArt() {
    return mIsHasAlbumArt;
}

char* waveditor::waveditor_getAlbumArtMime() {
    if(mIsHasAlbumArt)
        return (char *)mAlbumArtMime;

    return NULL;
}

char* waveditor::waveditor_getAlbumArtData() {   
    if(mIsHasAlbumArt)
        return (char *)mAlbumArtData;

    return NULL;
}

int waveditor::waveditor_getAlbumArtLength() {
    if(mIsHasAlbumArt)
        return (int)mAlbumArtLength;

    return 0;
}

void waveditor::waveditor_metaTagDump() {
#ifdef _WAV_METATAG_EDITOR_DEBUG_
    //Test Debug
    if(waveditor_getAlbum() != NULL)
        ALOGD("### waveditor_getAlbum() : %s", waveditor_getAlbum());    
    if(waveditor_getArtist() != NULL)
        ALOGD("### waveditor_getArtist() : %s", waveditor_getArtist());
    if(waveditor_getAlbumArtist() != NULL)
        ALOGD("### waveditor_getAlbumArtist() : %s", waveditor_getAlbumArtist());
    if(waveditor_getComposer() != NULL)
        ALOGD("### waveditor_getComposer() : %s", waveditor_getComposer());
    if(waveditor_getGenre() != NULL)
        ALOGD("### waveditor_getGenre() : %s", waveditor_getGenre());
    if(waveditor_getTitle() != NULL)
        ALOGD("### waveditor_getTitle() : %s", waveditor_getTitle());
    if(waveditor_getYear() != NULL)
        ALOGD("### waveditor_getYear() : %s", waveditor_getYear());
    if(waveditor_getCDTrackNumber() != NULL)
        ALOGD("### waveditor_getCDTrackNumber() : %s", waveditor_getCDTrackNumber());
    if(waveditor_getGraceNoteUIDOwner() != NULL)
        ALOGD("### waveditor_getGraceNoteUIDOwner() : %s", waveditor_getGraceNoteUIDOwner());
    if(waveditor_getGraceNoteTAGID() != NULL)
        ALOGD("### waveditor_getGraceNoteTAGID() : %s", waveditor_getGraceNoteTAGID());
    if(waveditor_getCopyright() != NULL)
        ALOGD("### waveditor_getCopyright() : %s", waveditor_getCopyright());
    if(waveditor_getAlbumArtMime() != NULL)
        ALOGD("### waveditor_getAlbumArtMime() : %s", waveditor_getAlbumArtMime());  
    if(waveditor_getComments() != NULL)
        ALOGD("### waveditor_getComments() : %s", waveditor_getComments());  
#endif

    return;
}

void waveditor::waveditor_metaTagSaveDump(const char* album, const char* artist,
                                          const char* albumartist, const char* composer, 
                                          const char* genre, const char* title,
                                          const char* year, const char* tracknumber, 
                                          const char* ufid, const char* gnid, 
                                          const char* copyright) {
#ifdef _WAV_METATAG_EDITOR_DEBUG_
    //Test Debug
    if(album != NULL && strlen(album) > 0)
        ALOGD("### Saving album : %s", album);    
    if(artist != NULL && strlen(artist) > 0)
        ALOGD("### Saving artist : %s", artist);
    if(albumartist != NULL && strlen(albumartist) > 0)
        ALOGD("### Saving albumartist : %s", albumartist);
    if(composer != NULL && strlen(composer) > 0)
        ALOGD("### Saving composer : %s", composer);
    if(genre != NULL && strlen(genre) > 0)
        ALOGD("### Saving genre : %s", genre);
    if(title != NULL && strlen(title) > 0)
        ALOGD("### Saving title : %s", title);
    if(year != NULL && strlen(year) > 0)
        ALOGD("### Saving year : %s", year);
    if(tracknumber != NULL && strlen(tracknumber) > 0)
        ALOGD("### Saving tracknumber : %s", tracknumber);
    if(ufid != NULL && strlen(ufid) > 0)
        ALOGD("### Saving ufid : %s", ufid);
    if(gnid != NULL && strlen(gnid) > 0)
        ALOGD("### Saving gnid : %s", gnid);
    if(copyright != NULL && strlen(copyright) > 0)
        ALOGD("### Saving copyright : %s", copyright);
#endif

    return;
}

//DEBUG TEST SAMPLE CODE
void tagsWAVTestCode() {
    ALOGV("### %s() ###", __FUNCTION__);

#ifdef _WAV_METATAG_EDITOR_DEBUG_

#endif    
    return;
}

bool tagsGenWAV(const char *mimeType) {
    ALOGV("### %s() ###", __FUNCTION__);
    
    if(mimeType == NULL || strlen(mimeType) <= 0) {
        return false;
    }
    
    ALOGV("### %s() / mimeType : %s ###", __FUNCTION__, mimeType);

    if(memcmp("audio/x-wav", mimeType, 11)) {
        return false;
    }
    
    return true;
}

} // namespace android
