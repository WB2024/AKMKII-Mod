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

#ifndef _TAG_EDITOR_INTERFACE_H_
#define _TAG_EDITOR_INTERFACE_H_

#include "tagsbase.h"

namespace android {
class tagseditor {
    public:
        static metaTag<tagseditor> TagEditorCreate(const char *mimeType);
        
        tagseditor();
        virtual bool tagsEditor_open(const char *filePath) = 0;
        virtual bool tagsEditor_save(const char* album, const char* artist,
                                     const char* albumartist, const char* composer, 
                                     const char* genre, const char* title,
                                     const char* year, const char* tracknumber, 
                                     const char* ufid, const char* gnid, 
                                     const char* copyright,
                                     const char* albumartdata, const int albumartlength) = 0;
        virtual ~tagseditor();
        
        typedef enum {
            AUDIO_MIMETYPE_FLAC = 0,
            AUDIO_MIMETYPE_WAV = 1,
			AUDIO_MIMETYPE_DFF = 2,
            //Add Here
            AUDIO_MIMETYPE_END =3,
        } AUDIO_MIMETYPE;
        
        virtual char* tagsEditor_getAlbum() = 0;
        virtual char* tagsEditor_getArtist() = 0;
        virtual char* tagsEditor_getAlbumArtist() = 0;
        virtual char* tagsEditor_getComposer() = 0;
        virtual char* tagsEditor_getGenre() = 0;
        virtual char* tagsEditor_getTitle() = 0;
        virtual char* tagsEditor_getYear() = 0;
        virtual char* tagsEditor_getCDTrackNumber() = 0;
        virtual char* tagsEditor_getGraceNoteUIDOwner() = 0;
        virtual char* tagsEditor_getGraceNoteTAGID() = 0;
        virtual char* tagsEditor_getCopyright() = 0;
        virtual bool tagsEditor_getIsAlbumArt() = 0;
        virtual char* tagsEditor_getAlbumArtMime() = 0;
        virtual char* tagsEditor_getAlbumArtData() = 0;
        virtual int tagsEditor_getAlbumArtLength() = 0;

        //TestCode for Debugging
        void tagsEditor_testModule();        
    private:
        tagseditor(const tagseditor &);
        tagseditor &operator=(const tagseditor &);
};

}  // namespace android

#endif  // _TAG_EDITOR_INTERFACE_H_