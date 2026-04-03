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
#define LOG_TAG "FLACEditor"

#include <stdint.h>
#include <utils/Log.h>

#include "flac_editor.h"

namespace android {

static const int debug_log = 1;

flac_editor::flac_editor() {
    if(debug_log) ALOGI("#### flac_editor::flac_editor() #### [START]");
    mFlacEditor = NULL;
}
  
flac_editor::~flac_editor() {
    if(debug_log) ALOGI("#### flac_editor::~flac_editor() #### [END]");
    if(mFlacEditor != NULL)
        delete mFlacEditor;
}

bool flac_editor::tagsEditor_open(const char *filePath) {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_open()");
    bool openStatus = false;
    mFlacEditor = new flaceditor(filePath);

    openStatus = mFlacEditor->flaceditor_open();
    
    return openStatus;
}

bool flac_editor::tagsEditor_save(const char* album, const char* artist,
                                  const char* albumartist, const char* composer, 
                                  const char* genre, const char* title,
                                  const char* year, const char* tracknumber, 
                                  const char* ufid, const char* gnid, 
                                  const char* copyright,
                                  const char* albumartdata, const int albumartlength) {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_save()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_save(album, artist,
                                            albumartist, composer, 
                                            genre, title,
                                            year, tracknumber, 
                                            ufid, gnid, 
                                            copyright,
                                            albumartdata, albumartlength);
    }
    return false;
}

char* flac_editor::tagsEditor_getAlbum() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getAlbum()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getAlbum();
    }
    return NULL;
}
char* flac_editor::tagsEditor_getArtist() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getArtist()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getArtist();
    }
    return NULL;
}
char* flac_editor::tagsEditor_getAlbumArtist() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getAlbumArtist()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getAlbumArtist();
    }
    return NULL;
}
char* flac_editor::tagsEditor_getComposer() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getComposer()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getComposer();
    }
    return NULL;
}
char* flac_editor::tagsEditor_getGenre() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getGenre()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getGenre();
    }
    return NULL;
}
char* flac_editor::tagsEditor_getTitle() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getTitle()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getTitle();
    }
    return NULL;
}
char* flac_editor::tagsEditor_getYear() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getYear()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getYear();
    }
    return NULL;
}
char* flac_editor::tagsEditor_getCDTrackNumber() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getCDTrackNumber()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getCDTrackNumber();
    }
    return NULL;
}
char* flac_editor::tagsEditor_getGraceNoteUIDOwner() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getGraceNoteUIDOwner()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getGraceNoteUIDOwner();
    }
    return NULL;
}
char* flac_editor::tagsEditor_getGraceNoteTAGID() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getGraceNoteTAGID()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getGraceNoteTAGID();
    }
    return NULL;
}
char* flac_editor::tagsEditor_getCopyright() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getCopyright()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getCopyright();
    }
    return NULL;
}
bool flac_editor::tagsEditor_getIsAlbumArt() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getIsAlbumArt()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getIsAlbumArt();
    }
    return false;
}
char* flac_editor::tagsEditor_getAlbumArtMime() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getAlbumArtMime()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getAlbumArtMime();
    }
    return NULL;
}
char* flac_editor::tagsEditor_getAlbumArtData() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getAlbumArtData()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getAlbumArtData();
    }
    return NULL;
}
int flac_editor::tagsEditor_getAlbumArtLength() {
    if(debug_log) ALOGI("#### flac_editor::tagsEditor_getAlbumArtLength()");
    if(mFlacEditor != NULL) {
        return mFlacEditor->flaceditor_getAlbumArtLength();
    }
    return 0;
}
} // namespace android
