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
#define LOG_TAG "dffeditor"
#include <utils/Log.h>

#include "dffeditor.h"

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
#include <media/stagefright/MediaErrors.h>

namespace android {

// ///////////////////////////////////////////////////////////////////////////////////////////////
#define ntohlL(x)    ( ((x) << 24) | (((x) >> 24) & 255) | (((x) << 8) & 0xff0000) | (((x) >> 8) & 0xff00) )
#define FOURCCL(c1, c2, c3, c4) \
    (c1 << 24 | c2 << 16 | c3 << 8 | c4)
    
#define SKIP_1BIT_VALUE				0x00   //Padding
#define DSD_SIGNED_0				0x00

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

static uint16_t U16_AT(const uint8_t *ptr) {
    return ptr[0] << 8 | ptr[1];
}

static uint32_t U32_AT(const uint8_t *ptr) {
    return ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];
}

static uint64_t U64_AT(const uint8_t *ptr) {
    return ((uint64_t)U32_AT(ptr)) << 32 | U32_AT(ptr + 4);
}

static uint16_t U16LE_AT(const uint8_t *ptr) {
    return ptr[0] | (ptr[1] << 8);
}

static uint32_t U32LE_AT(const uint8_t *ptr) {
    return ptr[3] << 24 | ptr[2] << 16 | ptr[1] << 8 | ptr[0];
}

static uint64_t U64LE_AT(const uint8_t *ptr) {
    return ((uint64_t)U32LE_AT(ptr + 4)) << 32 | U32LE_AT(ptr);
}

static ssize_t writeInt8(int fd, int8_t x) {
    return ::write(fd, &x, 1);
}

static ssize_t writeInt16DFF(int fd, int16_t x) {
    x = htons(x);
    return ::write(fd, &x, 2);
}

static ssize_t writeInt32DFF(int fd, int32_t x) {
    x = htonl(x);
    return ::write(fd, &x, 4);
}

static ssize_t writeInt64DFF(int fd, int64_t x) {
    x = ((int64_t)htonl(x & 0xffffffff) << 32) | htonl(x >> 32);
    return ::write(fd, &x, 8);
}
// ///////////////////////////////////////////////////////////////////////////////////////////////
dffeditor::dffeditor(const char *filePath)
	: mInitStatus(false),
      mTotalFileLength(0),
      mTotalDataSize(0),
      mDataEndOffset(0),
      mMetaChunkOffset(0),
      mHasMetaChunkTag(false),
	  mAlbum(""), mArtist(""), mTitle(""), 
	  mYearString(""), mCDTrackNumber(""), 
	  mCopyright(""), mComments(""), mModel(""),
	  mYear(0), mMonth(0), mDay(0), mHour(0), mMinute(0) {
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

dffeditor::~dffeditor() {
	ALOGV("### %s() ###", __FUNCTION__);

    if(!mInitStatus) {
        ALOGE("### %s() / ERROR! Init error! mInitStatus = false ###", __FUNCTION__);
    }
    
    dffeditor_release();
}

bool dffeditor::dffeditor_open() {
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
        if(dffeditor_findDffTag(fd) == false) {
            ALOGE("### %s() / ERROR! DFF Tag Error! ###", __FUNCTION__);
			
            if(fd != -1) {
                close(fd); 
                fd = -1;
            }
            return false;
        }
    } else {
       ALOGE("### %s() / ERROR! dffeditor_open() error! ###", __FUNCTION__); 
       return false;
    }

	if(dffeditor_hasMetaChunkTag()) {
        if(!dffeditor_getMetaChunkTag(fd)) {
            ALOGE("### %s() / ERROR! Cannot get Meta chunk ###", __FUNCTION__); 

            if(fd != -1) {
                close(fd); 
                fd = -1;
            }
            return false; 
        }
    } else {
        ALOGE("### %s() / ERROR! There is no Meta chunk ###", __FUNCTION__); 

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

void dffeditor::dffeditor_release() {
	ALOGV("### %s() ###", __FUNCTION__);
}

        
char* dffeditor::dffeditor_getAlbum() {
    if(mKeyAlbum.length() > 0)
        return (char *)mKeyAlbum.string();

    return NULL;
}
    
char* dffeditor::dffeditor_getArtist() {
    if(mKeyArtist.length() > 0)
        return (char *)mKeyArtist.string();

    return NULL;
}

char* dffeditor::dffeditor_getAlbumArtist() {
    return NULL;
}

char* dffeditor::dffeditor_getComposer() {
    return NULL;
}

char* dffeditor::dffeditor_getGenre() {
    return NULL;
}

char* dffeditor::dffeditor_getTitle() {
    if(mKeyTitle.length() > 0)
        return (char *)mKeyTitle.string();

    return NULL;
}

char* dffeditor::dffeditor_getYear() {
    if(mYearString.length() > 0)
        return (char *)mYearString.string();

    return NULL;
}

char* dffeditor::dffeditor_getCDTrackNumber() {
    if(mKeyCDTrackNumber.length() > 0)
        return (char *)mKeyCDTrackNumber.string();

    return NULL;
}

char* dffeditor::dffeditor_getGraceNoteUIDOwner() {
    return NULL;
}

char* dffeditor::dffeditor_getGraceNoteTAGID() {
    return NULL;
}

char* dffeditor::dffeditor_getCopyright() {
    if(mKeyCopyright.length() > 0)
        return (char *)mKeyCopyright.string();

    return NULL;
}

char* dffeditor::dffeditor_getComments() {
    if(mComments.length() > 0)
        return (char *)mComments.string();

    return NULL;
}

bool dffeditor::dffeditor_getIsAlbumArt() {
    return false;
}

char* dffeditor::dffeditor_getAlbumArtMime() {
    return NULL;
}

char* dffeditor::dffeditor_getAlbumArtData() {   
    return NULL;
}

int dffeditor::dffeditor_getAlbumArtLength() {
    return 0;
}

uint64_t dffeditor::dffeditor_getDataOffset() {
    ALOGV("### %s() ###", __FUNCTION__);

    return mDataEndOffset;
}

uint64_t dffeditor::dffeditor_getFileSize() {
    ALOGV("### %s() ###", __FUNCTION__);

    return mTotalFileLength;
}

bool dffeditor::dffeditor_hasMetaChunkTag() {
    ALOGV("### %s() ###", __FUNCTION__);

    return mHasMetaChunkTag;
}

uint64_t dffeditor::dffeditor_getMetaChunkOffset() {
    ALOGV("### %s() ###", __FUNCTION__);

    return mMetaChunkOffset;
}

bool dffeditor::dffeditor_findDffTag(int fd) {
    ALOGV("### %s() / fd = %d / File total size = %llu ###", __FUNCTION__, fd, mTotalFileLength);
    
	if(fd == -1)
		return false;

    uint8_t header[16];    
    off64_t offset = 0;

   //=============================================================================
    //Form DSD Chunk
    //=============================================================================
    //ckID = FRM8, formType = DSD (mandatory)
    if (read(fd, header, sizeof(header)) < (ssize_t)sizeof(header)) {
		ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
        return false;
    }
    if (memcmp(header, "FRM8", 4) || memcmp(&header[12], "DSD", 3)) {
        ALOGE("### %s() / Not DFF file / DSD ###", __FUNCTION__);
		return false;
    }    
    //FORM's data size plus the size of formType, in bytes
    size_t ckDataSize = U64_AT(&header[4]);
    offset = 32;
    
	//=============================================================================
    //Property Chunk (mandatory)
    //=============================================================================
	if(lseek64(fd, offset, SEEK_SET) < 0) {
		ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
		return false;
	}
    if (read(fd, header, sizeof(header)) < (ssize_t)sizeof(header)) {
        ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
		return false;
    }
    if (memcmp(header, "PROP", 4)) {// || memcmp(&header[12], "SND", 3)) {
		ALOGE("### %s() / Not DFF file / PROP ###", __FUNCTION__);
        return false;
    }
    size_t ckPropSize = U64_AT(&header[4]);
    offset += 16;
    
	//=============================================================================
    //Sample Rate Chunk (mandatory) of Property
    //=============================================================================
	if(lseek64(fd, offset, SEEK_SET) < 0) {
		ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
		return false;
	}
	if (read(fd, header, sizeof(header)) < (ssize_t)sizeof(header)) {
        ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
		return false;
    }    
    if (memcmp(header, "FS", 2)) {
		ALOGE("### %s() / Not DFF file / FS ###", __FUNCTION__);
        return false;
    }    
    uint32_t sampleRateDSD = U32_AT(&header[12]); 
    if(sampleRateDSD == 2822400 || sampleRateDSD == 5644800 || sampleRateDSD == 11289600) {
        offset += 16;
    } else {
		ALOGE("### %s() / Not DFF file / unsupport samplerate ###", __FUNCTION__);
        return false;
    }    
    
	//=============================================================================
    //Channels Chunk (mandatory) of Property
    //=============================================================================
    uint8_t sub_infoChannel[14];    
	if(lseek64(fd, offset, SEEK_SET) < 0) {
		ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
		return false;
	}
    if (read(fd, sub_infoChannel, sizeof(sub_infoChannel)) < (ssize_t)sizeof(sub_infoChannel)) {
        ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
		return false;
    }
    if (memcmp(sub_infoChannel, "CHNL", 4)) {
		ALOGE("### %s() / Not DFF file / CHNL ###", __FUNCTION__);
        return false;
    }
    size_t ckChannelsSize = U64_AT(&sub_infoChannel[4]);
    offset += 12;
    uint16_t nChannels = U16_AT(&sub_infoChannel[12]);    
    if(nChannels != 2 && nChannels != 1) {
		ALOGE("### %s() / Not DFF file / unsupport channel count ###", __FUNCTION__);
		return false;
    }
    offset += ckChannelsSize;
    
	//=============================================================================
    //Compression Type Chunk (mandatory) of Property
    //=============================================================================
    uint8_t sub_infoCompress[16];
	if(lseek64(fd, offset, SEEK_SET) < 0) {
		ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
		return false;
	}
    if (read(fd, sub_infoCompress, sizeof(sub_infoCompress)) < (ssize_t)sizeof(sub_infoCompress)) {
        ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
		return false;
    }
    if (memcmp(sub_infoCompress, "CMPR", 4)) {// || memcmp(&sub_infoCompress[12], "DSD", 3)) {
		ALOGE("### %s() / Not DFF file / CMPR ###", __FUNCTION__);
        return false;
    }    
    size_t ckCompressSize = U64_AT(&sub_infoCompress[4]);
    offset += (12 + ckCompressSize);
    
	//=============================================================================
    //Absolute Start Time Chunk (mandatory) of Property
    //=============================================================================
    uint8_t sub_timeInfo[20];     
	if(lseek64(fd, offset, SEEK_SET) < 0) {
		ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
		return false;
	}
    if (read(fd, sub_timeInfo, sizeof(sub_timeInfo)) < (ssize_t)sizeof(sub_timeInfo)) {
        ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
		return false;
    }
    if (memcmp(sub_timeInfo, "ABSS", 4) == 0) {
        offset += 12;

        size_t time_hours = U16_AT(&sub_timeInfo[12]);
        size_t time_minutes = (unsigned int)sub_timeInfo[14];
        size_t time_seconds = (unsigned int)sub_timeInfo[15];
        size_t time_samples = U32_AT(&sub_timeInfo[16]);
        offset += 8;
    }

    //=============================================================================
    //Pass Loudspeaker Configuration Chunk (optional) of Property 'LSCO'
    //=============================================================================
    uint8_t sub_loudInfo[4];  
	if(lseek64(fd, offset, SEEK_SET) < 0) {
		ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
		return false;
	}
	if (read(fd, sub_loudInfo, sizeof(sub_loudInfo)) < (ssize_t)sizeof(sub_loudInfo)) {
        ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
		return false;
    }
    if (memcmp(sub_loudInfo, "LSCO", 4) == 0) 
    {
        offset += 14;
    }
    //=============================================================================
    //Pass ID3 Chunk (optional)
    //=============================================================================
    uint8_t sub_ID3Info[12];  
	if(lseek64(fd, offset, SEEK_SET) < 0) {
		ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
		return false;
	}
    if (read(fd, sub_ID3Info, sizeof(sub_ID3Info)) < (ssize_t)sizeof(sub_ID3Info)) {
        ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
		return false;
    }
    if (memcmp(sub_ID3Info, "ID3", 3) == 0) {
        offset += 12;
        size_t ckID3Size = U64_AT(&sub_ID3Info[4]);
        offset += ckID3Size;
    }

    //=============================================================================
    //Padding Check, Protect Code
    //=============================================================================
    uint8_t skipChr;
    uint16_t maxSkipChar = 0;
    while(1) {
		if(lseek64(fd, offset, SEEK_SET) < 0) {
			ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
			return false;
		}
       if (read(fd, &skipChr, 1) < 1) {
           ALOGE("### %s() / failed to read Padding ###", __FUNCTION__);
           return false;
       }
       if(skipChr == DSD_SIGNED_0) {
           offset += 1; 
       } else {
           break;
       }
       if(maxSkipChar > 3) {
           ALOGE("### %s() / failed Long Padding ###", __FUNCTION__);
           return false;
       }
       maxSkipChar++;
    }    

    //=============================================================================
    //DSD Sound Data Chunk (mandatory), SKIP Unknown Tags
    //=============================================================================
    uint64_t totalDataSize = 0;
    uint16_t maxCount = 0;
    uint8_t sub_chunkdata[12];
    while (1) {
        memset(sub_chunkdata, 0, sizeof(sub_chunkdata));
		if(lseek64(fd, offset, SEEK_SET) < 0) {
			ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
			return false;
		}
        if (read(fd, sub_chunkdata, sizeof(sub_chunkdata)) < (ssize_t)sizeof(sub_chunkdata)) {
            ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
			return false;
        }
        
        if (memcmp(sub_chunkdata, "DSD", 3)) {
            size_t unusedDataSize = U64_AT(&sub_chunkdata[4]);
            offset += unusedDataSize;
            offset += 12; //header end
        } else {    
            totalDataSize = U64_AT(&sub_chunkdata[4]);
            offset += 12; //header end
            break;            
        }

        if(maxCount > 10) {
            return false;
        }
        maxCount++;
    }
    //=============================================================================
    //Setting Values
    //=============================================================================
    //Data Read, Starting Raw Data point
    mDataEndOffset = mMetaChunkOffset = offset + totalDataSize;
    mTotalDataSize = totalDataSize;

	if(mTotalFileLength > mDataEndOffset)
		mHasMetaChunkTag = true;

    return true;    
}

bool dffeditor::dffeditor_getMetaChunkTag(int fd) {
    ALOGV("### %s() / fd = %d ###", __FUNCTION__, fd);
	
	if(fd == -1)
		return false;

	uint64_t offset = dffeditor_getMetaChunkOffset();
	uint16_t maxCount = 0;
	uint8_t sub_chunkdata[12];

    while (1) {
        memset(sub_chunkdata, 0, sizeof(sub_chunkdata));
		if(lseek64(fd, offset, SEEK_SET) < 0) {
			ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
			break;
		}
        if (read(fd, sub_chunkdata, sizeof(sub_chunkdata)) < (ssize_t)sizeof(sub_chunkdata)) {
			ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
            break;
        }

		// Comment chunk
        if (memcmp(sub_chunkdata, "COMT", 4) == 0) {
            size_t chunkSize = U64_AT(&sub_chunkdata[4]);
			off64_t sub_chunkoffset = offset + 12;
            offset += 12; //header end
            offset += chunkSize;

			ALOGV("### %s() / Found Comment Chunk / chunk size = %u ###", __FUNCTION__, chunkSize);
			
			// numComments (2)
			uint8_t ushort_data[2];
			if(lseek64(fd, sub_chunkoffset, SEEK_SET) < 0) {
				ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
				break;
			}
			if (read(fd, ushort_data, sizeof(ushort_data)) < sizeof(ushort_data)) {
				ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
				break;
			}
			sub_chunkoffset += 2;
			size_t numComments = U16_AT(ushort_data);
			
			// Comment
			for(int a = 0; a < numComments; a++) {
				uint8_t commentData[14];
				if(lseek64(fd, sub_chunkoffset, SEEK_SET) < 0) {
					ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
					break;
				}
				if (read(fd, commentData, sizeof(commentData)) < sizeof(commentData)) {
					ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
					break;
				}
				sub_chunkoffset += 14;
		        size_t year = U16_AT(&commentData[0]);
		        size_t month = (unsigned int)commentData[2];
				size_t day = (unsigned int)commentData[3];
				size_t hour = (unsigned int)commentData[4];	
				size_t minutes = (unsigned int)commentData[5];	
		        size_t cmtType = U16_AT(&commentData[6]);
		        size_t cmtRef = U16_AT(&commentData[8]);
		        size_t cmtCount = U32_AT(&commentData[10]);

				if(mYearString.isEmpty()) {
					char date_str[256];
					sprintf(date_str, "%04d%02d%02d%02d%02d", year, month, day, hour, minutes);
					mYearString = String8(date_str);
				}
				
				if(mYear == 0) mYear = year;
				if(mMonth == 0) mMonth = month;
				if(mDay == 0) mDay = day;
				if(mHour == 0) mHour = hour;
				if(mMinute == 0) mMinute = minutes;

				// comment data
				char cmtbuf[cmtCount + 1];
				memset(cmtbuf, 0, cmtCount + 1);
				if(lseek64(fd, sub_chunkoffset, SEEK_SET) < 0) {
					ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
					break;
				}
				if (read(fd, cmtbuf, cmtCount) < cmtCount) {
					ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
					break;
				}
				sub_chunkoffset += cmtCount;
				// padding bytes
				if(cmtCount % 2)
					sub_chunkoffset += 1;

				if(strlen(cmtbuf) > 5 && memcmp(cmtbuf, "RFID:", 5) == 0) {
					mComments = String8((const char *)&cmtbuf[5]);
				} else if(strlen(cmtbuf) > 7 && memcmp(cmtbuf, "Vendor:", 7) == 0) {
					mKeyCopyright = String8((const char *)&cmtbuf[7]);
				} else if(strlen(cmtbuf) > 6 && memcmp(cmtbuf, "Model:", 6) == 0) {
					mModel = String8((const char *)&cmtbuf[6]);
				} else if(strlen(cmtbuf) > 6 && memcmp(cmtbuf, "Album:", 6) == 0) {
					mKeyAlbum = String8((const char *)&cmtbuf[6]);
				} else if(strlen(cmtbuf) > 6 && memcmp(cmtbuf, "Track:", 6) == 0) {
					mKeyCDTrackNumber = String8((const char *)&cmtbuf[6]);
				}

				ALOGV("### %s() / Comment[%d] / %04d-%02d-%02d %02d:%02d / type = %d / ref = %d / %s ###", 
					__FUNCTION__, a + 1, year, month, day, hour, minutes, cmtType, cmtRef, cmtbuf);
			}
        } 
		// EDITED MASTER INFORMATION CHUNK
		else if (memcmp(sub_chunkdata, "DIIN", 4) == 0) {
            size_t chunkSize = U64_AT(&sub_chunkdata[4]);
			off64_t sub_chunkoffset = offset + 12;
            offset += 12; //header end
            offset += chunkSize;

			ALOGV("### %s() / Found Edited Master Information Chunk / chunk size = %u ###", __FUNCTION__, chunkSize);

			while(sub_chunkoffset < offset) {
				if(lseek64(fd, sub_chunkoffset, SEEK_SET) < 0) {
					ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
					break;
				}
				if (read(fd, sub_chunkdata, sizeof(sub_chunkdata)) < (ssize_t)sizeof(sub_chunkdata)) {
					ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
					break;
				}
				size_t sub_chunkSize = U64_AT(&sub_chunkdata[4]);
				off64_t sub_sub_chunkoffset = sub_chunkoffset + 12;
				sub_chunkoffset += 12;
				sub_chunkoffset += sub_chunkSize;

				if (memcmp(sub_chunkdata, "EMID", 4) == 0) {
					ALOGV("### %s() / Found Edited Master ID Chunk ###", __FUNCTION__);
				} else if (memcmp(sub_chunkdata, "MARK", 4) == 0) {
					ALOGV("### %s() / Found Mark Chunk ###", __FUNCTION__);
				} else if (memcmp(sub_chunkdata, "DIAR", 4) == 0) {
					ALOGV("### %s() / Found Artist Chunk ###", __FUNCTION__);

					uint8_t ulong_data[4];
					if(lseek64(fd, sub_sub_chunkoffset, SEEK_SET) < 0) {
						ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
						break;
					}
					if (read(fd, ulong_data, sizeof(ulong_data)) < sizeof(ulong_data)) {
						ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
						break;
					}
					sub_sub_chunkoffset += 4;
					size_t artistCount = U32_AT(ulong_data);

					char artistbuf[artistCount + 1];
					memset(artistbuf, 0, artistCount + 1);
					if(lseek64(fd, sub_sub_chunkoffset, SEEK_SET) < 0) {
						ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
						break;
					}
					if (read(fd, artistbuf, artistCount) < artistCount) {
						ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
						break;
					}
					sub_sub_chunkoffset += artistCount;
					// padding bytes
					if(artistCount % 2)
						sub_sub_chunkoffset += 1;

					mKeyArtist = String8(artistbuf);

					ALOGV("### %s() / Artist: %s ###", __FUNCTION__, artistbuf);
				} else if (memcmp(sub_chunkdata, "DITI", 4) == 0) {
					ALOGV("### %s() / Found Title Chunk ###", __FUNCTION__);

					uint8_t ulong_data[4];
					if(lseek64(fd, sub_sub_chunkoffset, SEEK_SET) < 0) {
						ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
						break;
					}
					if (read(fd, ulong_data, sizeof(ulong_data)) < sizeof(ulong_data)) {
						ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
						break;
					}
					sub_sub_chunkoffset += 4;
					size_t titleCount = U32_AT(ulong_data);

					char titlebuf[titleCount + 1];
					memset(titlebuf, 0, titleCount + 1);
					if(lseek64(fd, sub_sub_chunkoffset, SEEK_SET) < 0) {
						ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
						break;
					}
					if (read(fd, titlebuf, titleCount) < titleCount) {
						ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
						break;
					}
					sub_sub_chunkoffset += titleCount;
					// padding bytes
					if(titleCount % 2)
						sub_sub_chunkoffset += 1;

					mKeyTitle = String8(titlebuf);

					ALOGV("### %s() / Title: %s ###", __FUNCTION__, titlebuf);
				}
			}
        } else if(memcmp(sub_chunkdata, "MANF", 4) == 0) {
            size_t chunkSize = U64_AT(&sub_chunkdata[4]);
			off64_t sub_chunkoffset = offset + 12;
            offset += 12; //header end
            offset += chunkSize;

			ALOGV("### %s() / Found Manufacturer Specific Chunk / chunk size = %u ###", __FUNCTION__, chunkSize);

			uint8_t manId[4];
			if(lseek64(fd, sub_chunkoffset, SEEK_SET) < 0) {
				ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
				break;
			}
			if (read(fd, manId, sizeof(manId)) < sizeof(manId)) {
				ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
				break;
			}
			sub_chunkoffset += 4;

			// manufacturer data
			size_t mandata_size = chunkSize - 4;
			char manbuf[mandata_size];
			memset(manbuf, 0, mandata_size);
			if(lseek64(fd, sub_chunkoffset, SEEK_SET) < 0) {
				ALOGE("### %s() / Error lseek64() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
				break;
			}
			if (read(fd, manbuf, mandata_size) < mandata_size) {
				ALOGE("### %s() / Error read() / errno = %d / %s ###", __FUNCTION__, errno, strerror(errno));
				break;
			}
			sub_chunkoffset += mandata_size;

			ALOGV("### %s() / Manufacturer: %s ###", __FUNCTION__, manbuf);
        }

        if(maxCount > 20) {
            break;
        }
        maxCount++;
	}

	ALOGV("### %s() / mKeyTitle = %s ###", __FUNCTION__, mKeyTitle.string());
	ALOGV("### %s() / mKeyAlbum = %s ###", __FUNCTION__, mKeyAlbum.string());
	ALOGV("### %s() / mKeyArtist = %s ###", __FUNCTION__, mKeyArtist.string());
	ALOGV("### %s() / mKeyCDTrackNumber = %s ###", __FUNCTION__, mKeyCDTrackNumber.string());
	ALOGV("### %s() / mYearString = %s ###", __FUNCTION__, mYearString.string());
	ALOGV("### %s() / mKeyCopyright = %s ###", __FUNCTION__, mKeyCopyright.string());
	ALOGV("### %s() / mComments = %s ###", __FUNCTION__, mComments.string());
	ALOGV("### %s() / mModel = %s ###", __FUNCTION__, mModel.string());

	return true;
}

bool dffeditor::dffeditor_Truncate(const char * filePath, const off64_t cutOffset) {
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

bool dffeditor::dffeditor_save(const char* album, const char* artist,
                               const char* albumartist, const char* composer, 
                               const char* genre, const char* title,
                               const char* year, const char* tracknumber, 
                               const char* ufid, const char* gnid, 
                               const char* copyright,
                               const char* albumartdata, const int albumartlength) {
    ALOGV("### %s() ###", __FUNCTION__);     

    int fd = -1;
    uint64_t bytes = 0;
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

    tagStartPoint = dffeditor_getFileSize() - dffeditor_getDataOffset();   

    //if saving action occurs the internal error!, original file will be recovered automatically
    //however How Copy? Rename? Think about this later 
    if(dffeditor_Truncate(filePath, (off64_t)dffeditor_getDataOffset()) ==  false) {
        ALOGE("### %s() / Error! dffeditor_getDataOffset() ###", __FUNCTION__);
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
    
	ALOGV("### %s() / open fd : %d, bytes : %llu ###", __FUNCTION__, fd, bytes);

	// Album
	if(album == NULL) {
		if(dffeditor_getAlbum() != NULL) {
			mAlbum = mKeyAlbum;
		}
	} else {
		ALOGV("### %s() / album : %s ###", __FUNCTION__, album);
		mAlbum = String8(album);
	}

	// Artist
	if(artist == NULL) {
		if(dffeditor_getArtist() != NULL) {
			mArtist = mKeyArtist;
		}
	} else {
		ALOGV("### %s() / artist : %s ###", __FUNCTION__, artist);
		mArtist = String8(artist);
	}

	// Title
	if(title == NULL) {
		if(dffeditor_getTitle() != NULL) {
			mTitle = mKeyTitle;
		}
	} else {
		ALOGV("### %s() / title : %s ###", __FUNCTION__, title);
		mTitle = String8(title);
	}

	// track
	if(tracknumber == NULL) {
		if(dffeditor_getCDTrackNumber() != NULL) {
			mCDTrackNumber = mKeyCDTrackNumber;
		}
	} else {
		ALOGV("### %s() / tracknumber : %s ###", __FUNCTION__, tracknumber);
		mCDTrackNumber = String8(tracknumber);
	}

	// copyright
	if(copyright == NULL) {
		if(dffeditor_getCopyright() != NULL) {
			mCopyright = mKeyCopyright;
		}
	} else {
		ALOGV("### %s() / copyright : %s ###", __FUNCTION__, copyright);
		mCopyright = String8(copyright);
	}

	int16_t chunk_size = 0;
	addCommentChunkDFF(fd, -1, &chunk_size);
	addEditedMasterChunkDFF(fd, -1, &chunk_size);

	lseek64(fd, 4, SEEK_SET);
	writeInt64DFF(fd, (int64_t)(dffeditor_getDataOffset() + chunk_size - 12));

	// write meta chunk
    if(fd != -1) {
        close(fd);
        fd = -1;
    }

    return true;
}

status_t dffeditor::addCommentChunkDFF(int fd, off64_t offset, int16_t *chunk_size) {
	ALOGV("### %s() ###", __FUNCTION__);

	status_t ret = OK;
	uint32_t write_bytes = 0;
	uint32_t n = 0;
	off64_t final_offset;
	size_t padding_size = 0;
	size_t data_size = 0;
	int16_t sub_chunk_size = 0;
	String8 data;
	char szbuf[255] = {0,};

	if(offset != -1)
		lseek64(fd, offset, SEEK_SET);
	else
		offset = lseek64(fd, 0, SEEK_CUR);

	// COMT
	const char *kHeader = "COMT";
	// COMT (4)
	n = strlen(kHeader);
	if (write(fd, kHeader, n) != n) {
		ret = ERROR_IO;
		goto done;
	}		
	write_bytes += n;

	// COMT size (8)
	n = sizeof(int64_t);
	if (writeInt64DFF(fd, 0) != n) {
		ret = ERROR_IO;
		goto done;
	}
	write_bytes += n;

	// COMT count (2)
	// RFID, Vendor(Copyright), Model, Album, Track
	n = sizeof(int16_t);
	if (writeInt16DFF(fd, 5) != n) {
		ret = ERROR_IO;
		goto done;
	}
	write_bytes += n;

	// ////////////////////////////////////////////////////////////////////////
	if(!mComments.isEmpty())
		addCommentSubChunkDFF(fd, 3, 0, "RFID", (const char*)mComments, &sub_chunk_size);
	if(!mCopyright.isEmpty())
		addCommentSubChunkDFF(fd, 3, 0, "Vendor", (const char*)mCopyright, &sub_chunk_size);
	if(!mModel.isEmpty())
		addCommentSubChunkDFF(fd, 3, 0, "Model", (const char*)mModel, &sub_chunk_size);
	if(!mAlbum.isEmpty())
		addCommentSubChunkDFF(fd, 3, 0, "Album", (const char*)mAlbum, &sub_chunk_size);
	if(!mCDTrackNumber.isEmpty())
		addCommentSubChunkDFF(fd, 3, 0, "Track", (const char*)mCDTrackNumber, &sub_chunk_size);

	write_bytes += sub_chunk_size;

	final_offset = lseek64(fd, 0, SEEK_CUR);

	// /////////////////////////////////////////////
	// set LIST size
	lseek64(fd, offset + 4, SEEK_SET);
	n = sizeof(int64_t);
	if (writeInt64DFF(fd, write_bytes - 12) != n) {
		ret = ERROR_IO;
		goto done;
	}

	lseek64(fd, final_offset, SEEK_SET);

done:
	*chunk_size = *chunk_size + write_bytes;

	ALOGD("### %s() / write size = %d bytes / total chunk size = %d bytes ###", __FUNCTION__, write_bytes, *chunk_size);

	return ret;
}

status_t dffeditor::addCommentSubChunkDFF(int fd, uint8_t type, uint8_t ref, const char *prefix, const char *data, int16_t *chunk_size) {
	ALOGV("### %s() / %s = %s ###", __FUNCTION__, prefix, data);
	
	status_t ret = OK;
	uint32_t write_bytes = 0;
	size_t padding_size = 0;
	size_t data_size = 0;
	uint32_t n = 0;

	char str[256];
	sprintf(str, "%s:%s", prefix, data);
	data_size = strlen(str);
	if(data_size % 2)
		padding_size = 2 - (data_size % 2);
	else
		padding_size = 0;

	// Year (2)
	n = sizeof(int16_t);
	if (writeInt16DFF(fd, mYear) != n) {
		ret = ERROR_IO;
		goto done;
	}
	write_bytes += n;

	// Month (1)
	n = sizeof(int8_t);
	if (writeInt8(fd, mMonth) != n) {
		ret = ERROR_IO;
		goto done;
	}
	write_bytes += n;

	// Day (1)
	n = sizeof(int8_t);
	if (writeInt8(fd, mDay) != n) {
		ret = ERROR_IO;
		goto done;
	}
	write_bytes += n;
 
 	// Hour (1)
	n = sizeof(int8_t);
	if (writeInt8(fd, mHour) != n) {
		ret = ERROR_IO;
		goto done;
	}
	write_bytes += n;

	// Minute (1)
	n = sizeof(int8_t);
	if (writeInt8(fd, mMinute) != n) {
		ret = ERROR_IO;
		goto done;
	}
	write_bytes += n;

	// cmtType (2)
	n = sizeof(int16_t);
	if (writeInt16DFF(fd, type) != n) {
		ret = ERROR_IO;
		goto done;
	}
	write_bytes += n;

	// cmtRef (2)
	n = sizeof(int16_t);
	if (writeInt16DFF(fd, ref) != n) {
		ret = ERROR_IO;
		goto done;
	}
	write_bytes += n;

	// Commnet length (4)
	n = sizeof(int32_t);
	if (writeInt32DFF(fd, data_size) != n) {
		ret = ERROR_IO;
		goto done;
	}
	write_bytes += n;

	// COMT data
	n = data_size;
	if (write(fd, str, n) != n) {
		ret = ERROR_IO;
		goto done;
	}		
	write_bytes += n;
	
	// Padding (1 x n)
	for(int a = 0; a < padding_size; a++) {
		n = sizeof(int8_t);
		if (writeInt8(fd, 0) != n) {
			ret = ERROR_IO;
			goto done;
		}
		write_bytes += n;
	}

done:
	*chunk_size = *chunk_size + write_bytes;

	return ret;
}

status_t dffeditor::addEditedMasterChunkDFF(int fd, off64_t offset, int16_t *chunk_size) {
	ALOGV("### %s() ###", __FUNCTION__);

	status_t ret = OK;
	uint32_t write_bytes = 0;
	uint32_t n = 0;
	off64_t final_offset;
	size_t padding_size = 0;
	size_t data_size = 0;
	String8 data;

	if(offset != -1)
		lseek64(fd, offset, SEEK_SET);
	else
		offset = lseek64(fd, 0, SEEK_CUR);

	// DIIN
	const char *kHeader = "DIIN";
	// DIIN (4)
	n = strlen(kHeader);
	if (write(fd, kHeader, n) != n) {
		ret = ERROR_IO;
		goto done;
	}		
	write_bytes += n;

	// DIIN size (8)
	n = sizeof(int64_t);
	if (writeInt64DFF(fd, 0) != n) {
		ret = ERROR_IO;
		goto done;
	}
	write_bytes += n;

	// /////////////////////////////////////////////
	// Title (DITI)
	if(!mTitle.isEmpty()) {
		kHeader = "DITI";
		// DITI (4)
		n = strlen(kHeader);
		if (write(fd, kHeader, n) != n) {
			ret = ERROR_IO;
			goto done;
		}		
		write_bytes += n;

		// DITI size
		data = mTitle;
		data_size = data.bytes();
		if(data_size % 2)
			padding_size = 2 - (data_size % 2);
		else
			padding_size = 0;
		n = sizeof(int64_t);
		if (writeInt64DFF(fd, data_size + padding_size + 4) != n) {
			ret = ERROR_IO;
			goto done;
		}
		write_bytes += n;
		
		// Title length (4)
		n = sizeof(int32_t);
		if (writeInt32DFF(fd, data_size) != n) {
			ret = ERROR_IO;
			goto done;
		}
		write_bytes += n;

		// DITI data
		n = data_size;
		if (write(fd, (const char *)data, n) != n) {
			ret = ERROR_IO;
			goto done;
		}		
		write_bytes += n;
		
		// Padding (1 x n)
		for(int a = 0; a < padding_size; a++) {
			n = sizeof(int8_t);
			if (writeInt8(fd, 0) != n) {
				ret = ERROR_IO;
				goto done;
			}
			write_bytes += n;
		}
	}
	// /////////////////////////////////////////////

	// /////////////////////////////////////////////
	if(!mArtist.isEmpty()) {
		kHeader = "DIAR";
		// DITI (4)
		n = strlen(kHeader);
		if (write(fd, kHeader, n) != n) {
			ret = ERROR_IO;
			goto done;
		}		
		write_bytes += n;

		// DIAR size
		data = mArtist;
		data_size = data.bytes();
		if(data_size % 2)
			padding_size = 2 - (data_size % 2);
		else
			padding_size = 0;
		n = sizeof(int64_t);
		if (writeInt64DFF(fd, data_size + padding_size + 4) != n) {
			ret = ERROR_IO;
			goto done;
		}
		write_bytes += n;
		
		// Artist length (4)
		n = sizeof(int32_t);
		if (writeInt32DFF(fd, data_size) != n) {
			ret = ERROR_IO;
			goto done;
		}
		write_bytes += n;

		// DIAR data
		n = data_size;
		if (write(fd, (const char *)data, n) != n) {
			ret = ERROR_IO;
			goto done;
		}		
		write_bytes += n;
		
		// Padding (1 x n)
		for(int a = 0; a < padding_size; a++) {
			n = sizeof(int8_t);
			if (writeInt8(fd, 0) != n) {
				ret = ERROR_IO;
				goto done;
			}
			write_bytes += n;
		}
	}
	// /////////////////////////////////////////////

	final_offset = lseek64(fd, 0, SEEK_CUR);

	// /////////////////////////////////////////////
	// set LIST size
	lseek64(fd, offset + 4, SEEK_SET);
	n = sizeof(int64_t);
	if (writeInt64DFF(fd, write_bytes - 12) != n) {
		ret = ERROR_IO;
		goto done;
	}

	lseek64(fd, final_offset, SEEK_SET);

done:
	*chunk_size = *chunk_size + write_bytes;

	ALOGD("### %s() / write size = %d bytes / total chunk size = %d bytes ###", __FUNCTION__, write_bytes, *chunk_size);

	return ret;
}

status_t dffeditor::addManufacturChunkDFF(int fd, off64_t offset, int16_t *chunk_size) {
	status_t ret = OK;
	uint32_t write_bytes = 0;
	uint32_t n = 0;
	size_t padding_size = 0;
	size_t data_size = 0;
	String8 data;

	if(offset != -1)
		lseek64(fd, offset, SEEK_SET);
	else
		offset = lseek64(fd, 0, SEEK_CUR);

	// MANF / Manufactureer chunk
	const char *kHeader = "MANF";
	// MANF (4)
	n = strlen(kHeader);
	if (write(fd, kHeader, n) != n) {
		ret = ERROR_IO;
		goto done;
	}		
	write_bytes += n;

	// MANF size (8)
	data = mModel;
	data_size = data.bytes();
	if(data_size % 2)
		padding_size = 2 - (data_size % 2);
	else
		padding_size = 0;
	n = sizeof(int64_t);
	if (writeInt64DFF(fd, data_size + padding_size + 4) != n) {
		ret = ERROR_IO;
		goto done;
	}
	write_bytes += n;

	// Manufacturer ID
	kHeader = "IRIV";
	// IRIV (4)
	n = strlen(kHeader);
	if (write(fd, kHeader, n) != n) {
		ret = ERROR_IO;
		goto done;
	}		
	write_bytes += n;

	// MANF data
	n = data_size;
	if (write(fd, (const char *)data, n) != n) {
		ret = ERROR_IO;
		goto done;
	}		
	write_bytes += n;
	
	// Padding (1 x n)
	for(int a = 0; a < padding_size; a++) {
		n = sizeof(int8_t);
		if (writeInt8(fd, 0) != n) {
			ret = ERROR_IO;
			goto done;
		}
		write_bytes += n;
	}

done:
	*chunk_size = *chunk_size + write_bytes;

	ALOGD("### %s() / write size = %d bytes / total chunk size = %d bytes ###", __FUNCTION__, write_bytes, *chunk_size);

	return ret;
}

bool tagsGenDFF(const char *mimeType) {
    ALOGV("### %s() ###", __FUNCTION__);
    
    if(mimeType == NULL || strlen(mimeType) <= 0) {
        return false;
    }
    
    ALOGV("### %s() / mimeType : %s ###", __FUNCTION__, mimeType);

    if(memcmp("audio/dff", mimeType, 9)) {
        return false;
    }
    
    return true;
}

} // namespace android
