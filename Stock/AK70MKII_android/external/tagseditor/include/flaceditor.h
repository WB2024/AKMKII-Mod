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

#ifndef _FLAC_TAG_EDITOR_H_
#define _FLAC_TAG_EDITOR_H_

#include <utils/String8.h>

#include "FLAC/stream_decoder.h"
#include "FLAC/stream_encoder.h"
#include "FLAC/metadata.h"

namespace android {

class flaceditor {
    public:
        flaceditor(const char *filePath);
        virtual ~flaceditor();
        
        bool flaceditor_open();
        void flaceditor_release();
        bool flaceditor_save(const char* album, 
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
            FLAC_METADATA_ED_ALBUM = 0,
            FLAC_METADATA_ED_ARTIST = 1,
            FLAC_METADATA_ED_ALBUMARTIST = 2,
            FLAC_METADATA_ED_COMPOSER = 3,
            FLAC_METADATA_ED_GENRE = 4,
            FLAC_METADATA_ED_TITLE = 5,
            FLAC_METADATA_ED_YEAR = 6,
            FLAC_METADATA_ED_TRACKNUM = 7,
            FLAC_METADATA_ED_GREACENOTE_UIDOWNER = 8,  //Ablum TagID
            FLAC_METADATA_ED_GREACENOTE_TAGID = 9,     //Track TagID
            FLAC_METADATA_ED_COPYRIGHT = 10,
            //Add Here
            FLAC_METADATA_ED_END =11,
        } FLAC_METADATA_ED_INFO;

        char* flaceditor_getAlbum();
        char* flaceditor_getArtist();
        char* flaceditor_getAlbumArtist();
        char* flaceditor_getComposer();
        char* flaceditor_getGenre();
        char* flaceditor_getTitle();
        char* flaceditor_getYear();
        char* flaceditor_getCDTrackNumber();
        char* flaceditor_getGraceNoteUIDOwner();
        char* flaceditor_getGraceNoteTAGID();
        char* flaceditor_getCopyright();
        bool flaceditor_getIsAlbumArt();
        char* flaceditor_getAlbumArtMime();
        char* flaceditor_getAlbumArtData();
        int flaceditor_getAlbumArtLength();
        
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
        
        unsigned char *mAlbumArtMime;
        unsigned char *mAlbumArtData;
        unsigned long mAlbumArtLength;
        FLAC__StreamMetadata_Picture_Type mPictureType;
        bool mIsHasAlbumArt;  

        void flaceditor_setTags(int tagNum, char* tagData);
        bool flaceditor_addPicture(FLAC__Metadata_Iterator* metaIterator, 
                                   const char* albumartdata, 
                                   const int albumartlength);
        bool flaceditor_addVorbisComment(FLAC__Metadata_Iterator* metaIterator, 
                                         const char* album, const char* artist,
                                         const char* albumartist, const char* composer, 
                                         const char* genre, const char* title,
                                         const char* year, const char* tracknumber, 
                                         const char* ufid, const char* gnid, 
                                         const char* copyright);
        bool flaceditor_addVobisTag(FLAC__StreamMetadata *block, const char* vobistag, const char* vobistagdata);

        //DEBUG DUMP
        void flaceditor_metaTagDump();
        void flaceditor_metaTagSaveDump(const char* album, const char* artist,
                                        const char* albumartist, const char* composer, 
                                        const char* genre, const char* title,
                                        const char* year, const char* tracknumber, 
                                        const char* ufid, const char* gnid, 
                                        const char* copyright);
};

//DEBUG TEST SAMPLE CODE
void tagsFLACTestCode();
bool tagsGenFLAC(const char *mimeType);

}  // namespace android

#endif  // _FLAC_TAG_EDITOR_H_
