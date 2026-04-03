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

#ifndef _WAV_TAG_EDITOR_H_
#define _WAV_TAG_EDITOR_H_

#include <utils/String8.h>

namespace android {

class waveditor {
    public:
        waveditor(const char *filePath);
        virtual ~waveditor();
        bool waveditor_open();
        void waveditor_release();
        uint64_t waveditor_getDataOffset();

        bool waveditor_save(const char* album, 
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
    
        typedef enum {
            WAV_METADATA_ED_ALBUM = 0,
            WAV_METADATA_ED_ARTIST = 1,
            WAV_METADATA_ED_ALBUMARTIST = 2,
            WAV_METADATA_ED_COMPOSER = 3,
            WAV_METADATA_ED_GENRE = 4,
            WAV_METADATA_ED_TITLE = 5,
            WAV_METADATA_ED_YEAR = 6,
            WAV_METADATA_ED_TRACKNUM = 7,
            WAV_METADATA_ED_GREACENOTE_UIDOWNER = 8,  //Ablum TagID
            WAV_METADATA_ED_GREACENOTE_TAGID = 9,     //Track TagID
            WAV_METADATA_ED_COPYRIGHT = 10,
			WAV_METADATA_ED_COMMENTS = 11,
            //Add Here
            WAV_METADATA_ED_END = 12,
        } WAV_METADATA_ED_INFO;
    
        char* waveditor_getAlbum();
        char* waveditor_getArtist();
        char* waveditor_getAlbumArtist();
        char* waveditor_getComposer();
        char* waveditor_getGenre();
        char* waveditor_getTitle();
        char* waveditor_getYear();
        char* waveditor_getCDTrackNumber();
        char* waveditor_getGraceNoteUIDOwner();
        char* waveditor_getGraceNoteTAGID();
        char* waveditor_getCopyright();
        bool waveditor_getIsAlbumArt();
        char* waveditor_getAlbumArtMime();
        char* waveditor_getAlbumArtData();
		char* waveditor_getComments();
        int waveditor_getAlbumArtLength();
        
    private:
        bool mInitStatus;
        String8 mFilepath;
        
        String8 mKeyAlbum;
        String8 mKeyArtist;
        String8 mKeyAlbumArtist;
        String8 mKeyComposer;
        String8 mKeyGenre;
        String8 mKeyTitle;
        String8 mKeyYear;
        String8 mKeyCDTrackNumber;
        String8 mKeyGraceNoteUIDOwner;
        String8 mKeyGraceNoteTAGID;
        String8 mKeyCopyright;
		String8 mKeyComments;
        
        unsigned char *mAlbumArtMime;
        unsigned char *mAlbumArtData;
        unsigned long mAlbumArtLength;
        bool mIsHasAlbumArt;
		bool mIsAKRecord;

        uint64_t mTotalFileLength;
        uint64_t mTotalDataSize;
        uint64_t mDataEndOffset;

        uint64_t mId3Offset;
        uint64_t mListOffset;
        bool mHasID3Tag;
        bool mHasListTag;

        bool waveditor_findWavTag(int fd);
        bool waveditor_checkMetaData(int fd);
        bool waveditor_getID3MetaTag(int fd);
        bool waveditor_getListMetaTag(int fd);

		bool waveditor_isID3Tag();
        uint64_t waveditor_getID3Offset();

        bool waveditor_isListTag();
        uint64_t waveditor_getListOffset();

        void waveditor_setTags(int tagNum, char* tagData);
        uint64_t waveditor_getFileSize();
        bool waveditor_Truncate(const char * filePath, const off64_t cutOffset);
        
        //DEBUG DUMP
        void waveditor_metaTagDump();
        void waveditor_metaTagSaveDump(const char* album, const char* artist,
                                       const char* albumartist, const char* composer, 
                                       const char* genre, const char* title,
                                       const char* year, const char* tracknumber, 
                                       const char* ufid, const char* gnid, 
                                       const char* copyright);
};

//DEBUG TEST SAMPLE CODE
void tagsWAVTestCode();
bool tagsGenWAV(const char *mimeType);

}  // namespace android
#endif  // _FLAC_TAG_EDITOR_H_