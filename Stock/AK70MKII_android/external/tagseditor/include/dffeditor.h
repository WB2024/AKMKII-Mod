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

#ifndef _DFF_TAG_EDITOR_H_
#define _DFF_TAG_EDITOR_H_

#include <utils/String8.h>

namespace android {

class dffeditor {
    public:
        dffeditor(const char *filePath);
        virtual ~dffeditor();

		bool dffeditor_open();
		void dffeditor_release();

        bool dffeditor_save(const char* album, 
                            const char* artist,
                            const char* albumartist, 
                            const char* composer, 
                            const char* genre, 
                            const char* title,
                            const char* year, 
                            const char* tracknumber, 
                            const char* ufid, 
                            const char* gnid, 
                            const char* copyright,
                            const char* albumartdata, 
                            const int albumartlength);

        char* dffeditor_getAlbum();
        char* dffeditor_getArtist();
        char* dffeditor_getAlbumArtist();
        char* dffeditor_getComposer();
        char* dffeditor_getGenre();
        char* dffeditor_getTitle();
        char* dffeditor_getYear();
        char* dffeditor_getCDTrackNumber();
        char* dffeditor_getGraceNoteUIDOwner();
        char* dffeditor_getGraceNoteTAGID();
        char* dffeditor_getCopyright();
        bool dffeditor_getIsAlbumArt();
        char* dffeditor_getAlbumArtMime();
        char* dffeditor_getAlbumArtData();
		char* dffeditor_getComments();
        int dffeditor_getAlbumArtLength();

    private:
        bool mInitStatus;
        String8 mFilepath;
        
        String8 mKeyAlbum;
        String8 mKeyArtist;
        String8 mKeyTitle;
        String8 mKeyCDTrackNumber;
        String8 mKeyCopyright;

        String8 mAlbum;
        String8 mArtist;
        String8 mTitle;
        String8 mYearString;
        String8 mCDTrackNumber;
        String8 mCopyright;
		String8 mComments;
		String8 mModel;

		uint16_t mYear;
		uint8_t mMonth;
		uint8_t mDay;
		uint8_t mHour;
		uint8_t mMinute;

        uint64_t mTotalFileLength;
        uint64_t mTotalDataSize;
        uint64_t mDataEndOffset;

        uint64_t mMetaChunkOffset;
        bool mHasMetaChunkTag;

		bool dffeditor_findDffTag(int fd);
		bool dffeditor_getMetaChunkTag(int fd);
		bool dffeditor_hasMetaChunkTag();
		bool dffeditor_Truncate(const char * filePath, const off64_t cutOffset);

		uint64_t dffeditor_getDataOffset();
		uint64_t dffeditor_getFileSize();
		uint64_t dffeditor_getMetaChunkOffset();

		status_t addCommentChunkDFF(int fd, off64_t offset, int16_t *chunk_size);
		status_t addCommentSubChunkDFF(int fd, uint8_t type, uint8_t ref, const char *prefix, const char *data, int16_t *chunk_size);
		status_t addEditedMasterChunkDFF(int fd, off64_t offset, int16_t *chunk_size);
		status_t addManufacturChunkDFF(int fd, off64_t offset, int16_t *chunk_size);
};

bool tagsGenDFF(const char *mimeType);

}  // namespace android

#endif  // _DFF_TAG_EDITOR_H_