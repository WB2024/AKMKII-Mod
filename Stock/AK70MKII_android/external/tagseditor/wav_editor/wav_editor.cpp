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

// #define LOG_NDEBUG 0
#define LOG_TAG "wav_editor"

#include <stdint.h>
#include <utils/Log.h>

#include "wav_editor.h"

namespace android {

wav_editor::wav_editor() {
    ALOGV("### %s() ###", __FUNCTION__);
    
	mWavEditor = NULL;
}
  
wav_editor::~wav_editor() {
    ALOGV("### %s() ###", __FUNCTION__);
    
	if(mWavEditor != NULL)
        delete mWavEditor;
}

bool wav_editor::tagsEditor_open(const char *filePath) {
	ALOGV("### %s() / %s ###", __FUNCTION__, filePath);
	
    bool openStatus = false;
    mWavEditor = new waveditor(filePath);    
    openStatus = mWavEditor->waveditor_open();
    
    return openStatus;
}

bool wav_editor::tagsEditor_save(const char* album, const char* artist,
                                 const char* albumartist, const char* composer, 
                                 const char* genre, const char* title,
                                 const char* year, const char* tracknumber, 
                                 const char* ufid, const char* gnid, 
                                 const char* copyright,
                                 const char* albumartdata, const int albumartlength) {
    ALOGV("### %s() ###", __FUNCTION__);

    if(mWavEditor != NULL) {
        return mWavEditor->waveditor_save(album, artist,
                                          albumartist, composer, 
                                          genre, title,
                                          year, tracknumber, 
                                          ufid, gnid, 
                                          copyright,
                                          albumartdata, albumartlength);
    }
    return false;
}

char* wav_editor::tagsEditor_getAlbum() {
    ALOGV("### %s() ###", __FUNCTION__);

	if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getAlbum();
    }    
    return NULL;
}
char* wav_editor::tagsEditor_getArtist() {
    ALOGV("### %s() ###", __FUNCTION__);

	if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getArtist();
    }    
    return NULL;
}
char* wav_editor::tagsEditor_getAlbumArtist() {
    ALOGV("### %s() ###", __FUNCTION__);

	if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getAlbumArtist();
    } 
    return NULL;
}
char* wav_editor::tagsEditor_getComposer() {
    ALOGV("### %s() ###", __FUNCTION__);

	if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getComposer();
    } 
    return NULL;
}
char* wav_editor::tagsEditor_getGenre() {
    ALOGV("### %s() ###", __FUNCTION__);

	if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getGenre();
    } 
    return NULL;
}
char* wav_editor::tagsEditor_getTitle() {
    ALOGV("### %s() ###", __FUNCTION__);

	if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getTitle();
    } 
    return NULL;
}
char* wav_editor::tagsEditor_getYear() {
	ALOGV("### %s() ###", __FUNCTION__);

    if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getYear();
    } 
    return NULL;
}
char* wav_editor::tagsEditor_getCDTrackNumber() {
	ALOGV("### %s() ###", __FUNCTION__);

    if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getCDTrackNumber();
    } 
    return NULL;
}
char* wav_editor::tagsEditor_getGraceNoteUIDOwner() {
    ALOGV("### %s() ###", __FUNCTION__);

    if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getGraceNoteUIDOwner();
    } 
    return NULL;
}
char* wav_editor::tagsEditor_getGraceNoteTAGID() {
    ALOGV("### %s() ###", __FUNCTION__);

	if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getGraceNoteTAGID();
    } 
    return NULL;
}
char* wav_editor::tagsEditor_getCopyright() {
    ALOGV("### %s() ###", __FUNCTION__);

	if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getCopyright();
    } 
    return NULL;
}
bool wav_editor::tagsEditor_getIsAlbumArt() {
    ALOGV("### %s() ###", __FUNCTION__);

	if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getIsAlbumArt();
    } 
    return false;
}
char* wav_editor::tagsEditor_getAlbumArtMime() {
    ALOGV("### %s() ###", __FUNCTION__);

	if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getAlbumArtMime();
    } 
    return NULL;
}
char* wav_editor::tagsEditor_getAlbumArtData() {
    ALOGV("### %s() ###", __FUNCTION__);

	if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getAlbumArtData();
    } 
    return NULL;
}
int wav_editor::tagsEditor_getAlbumArtLength() {
    ALOGV("### %s() ###", __FUNCTION__);

	if(mWavEditor != NULL) {
        return mWavEditor->waveditor_getAlbumArtLength();
    } 
    return 0;
}

} // namespace android
