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

#ifndef _FLAC_META_TAG_EDITOR_H_
#define _FLAC_META_TAG_EDITOR_H_

#include "tagseditor.h"
#include "flaceditor.h"

namespace android {

class flac_editor : public tagseditor{
    public:
        flac_editor();
        virtual bool tagsEditor_open(const char *filePath);
        virtual bool tagsEditor_save(const char* album, const char* artist,
                                     const char* albumartist, const char* composer, 
                                     const char* genre, const char* title,
                                     const char* year, const char* tracknumber, 
                                     const char* ufid, const char* gnid, 
                                     const char* copyright,
                                     const char* albumartdata, const int albumartlength);
        virtual ~flac_editor();

        virtual char* tagsEditor_getAlbum();
        virtual char* tagsEditor_getArtist();
        virtual char* tagsEditor_getAlbumArtist();
        virtual char* tagsEditor_getComposer();
        virtual char* tagsEditor_getGenre();
        virtual char* tagsEditor_getTitle();
        virtual char* tagsEditor_getYear();
        virtual char* tagsEditor_getCDTrackNumber();
        virtual char* tagsEditor_getGraceNoteUIDOwner();
        virtual char* tagsEditor_getGraceNoteTAGID();
        virtual char* tagsEditor_getCopyright();
        virtual bool tagsEditor_getIsAlbumArt();
        virtual char* tagsEditor_getAlbumArtMime();
        virtual char* tagsEditor_getAlbumArtData();
        virtual int tagsEditor_getAlbumArtLength();

    private:
        flaceditor *mFlacEditor;
        flac_editor(const flac_editor &);
        flac_editor &operator=(const flac_editor &);
            
};

}  // namespace android

#endif  // _FLAC_META_TAG_EDITOR_H_