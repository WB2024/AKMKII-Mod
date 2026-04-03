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
#define LOG_TAG "TagsEditor"

#include <stdint.h>
#include <utils/Log.h>

#include "tagseditor.h"
#include "flac_editor.h"
#include "wav_editor.h"
#include "dff_editor.h"

//For Testing
#include <sys/stat.h>
#include "flaceditor.h"
#include "waveditor.h"
#include "dffeditor.h"

//tagseditor Interface
namespace android {

const char* sAudioMimeType[] = {
    "audio/flac",        //AUDIO_MIMETYPE_FLAC, MEDIA_MIMETYPE_AUDIO_FLAC
    "audio/x-wav",       //AUDIO_MIMETYPE_WAV, MEDIA_MIMETYPE_CONTAINER_WAV
    "audio/dff",		//AUDIO_MIMETYPE_DFF, MEDIA_MIMETYPE_CONTAINER_WAV
    //Add Here
};

metaTag<tagseditor> tagseditor::TagEditorCreate(const char *mimeType) {
	ALOGV("### %s ###", __FUNCTION__);

    tagseditor *ret = NULL;
    
    if(mimeType == NULL || strlen(mimeType) <= 0) {
        ALOGE("### %s() / ERROR! mimeType error! ###", __FUNCTION__);
        return ret;
    }

    ALOGV("### %s() / mimeType : %s ###", __FUNCTION__, mimeType);

    //Comparing audio mime type
    //FLAC = AUDIO_MIMETYPE_FLAC
    if (!strcasecmp(mimeType, sAudioMimeType[AUDIO_MIMETYPE_FLAC])) {
        ALOGV("### %s() / Detecting FLAC Audio ###", __FUNCTION__);
        if(tagsGenFLAC(mimeType)) { 
            ret = new flac_editor();
        } else {
            ALOGE("### %s() / Fail to load flac_editor ###", __FUNCTION__);
        }
    } 
    //WAV = AUDIO_MIMETYPE_WAV
    else if (!strcasecmp(mimeType, sAudioMimeType[AUDIO_MIMETYPE_WAV])) {
        ALOGV("### %s() / Detecting WAVE Audio ###", __FUNCTION__);
        if(tagsGenWAV(mimeType)) {
            ret = new wav_editor();
        } else {
            ALOGE("### %s() / Fail to load wav_editor ###", __FUNCTION__);
        }
    }           
    //add here
    //DFF = AUDIO_MIMETYPE_DFF
    else if (!strcasecmp(mimeType, sAudioMimeType[AUDIO_MIMETYPE_DFF])) {
        ALOGV("### %s() / Detecting DFF Audio ###", __FUNCTION__);
        if(tagsGenDFF(mimeType)) {
            ret = new dff_editor();
        } else {
            ALOGE("### %s() / Fail to load dff_editor ###", __FUNCTION__);
        }
    }     

    return ret;
}
    
tagseditor::tagseditor() {
	ALOGV("### %s ###", __FUNCTION__);
}
  
tagseditor::~tagseditor() {
    ALOGV("### %s ###", __FUNCTION__);
}

//TestCode for Debugging
void tagseditor::tagsEditor_testModule() {
    ALOGV("### %s ###", __FUNCTION__); 

    char outFilePathFlac[34] = "/storage/emulated/0/lovesong.flac";
    char outFilePathWav[33] = "/storage/emulated/0/lovesong.wav";
    bool retStatus = false;

    flaceditor *flacEditor = new flaceditor(outFilePathFlac);
    retStatus = flacEditor->flaceditor_open();

    ALOGE("tagseditor::tagsEditor_testModule #### Getting Meta Data ==========");
    
    if(flacEditor->flaceditor_getAlbum() != NULL)
        ALOGV("tagseditor::tagsEditor_testModule() #### flaceditor_getAlbum() : %s", flacEditor->flaceditor_getAlbum()); 
    if(flacEditor->flaceditor_getArtist() != NULL)
        ALOGV("tagseditor::tagsEditor_testModule() #### flaceditor_getArtist() : %s", flacEditor->flaceditor_getArtist()); 
    if(flacEditor->flaceditor_getAlbumArtist() != NULL)
        ALOGV("tagseditor::tagsEditor_testModule() #### flaceditor_getAlbumArtist() : %s", flacEditor->flaceditor_getAlbumArtist()); 
    if(flacEditor->flaceditor_getComposer() != NULL)
        ALOGV("tagseditor::tagsEditor_testModule() #### flaceditor_getComposer() : %s", flacEditor->flaceditor_getComposer()); 
    if(flacEditor->flaceditor_getGenre() != NULL)
        ALOGV("tagseditor::tagsEditor_testModule() #### flaceditor_getGenre() : %s", flacEditor->flaceditor_getGenre()); 
    if(flacEditor->flaceditor_getTitle() != NULL)
        ALOGV("tagseditor::tagsEditor_testModule() #### flaceditor_getTitle() : %s", flacEditor->flaceditor_getTitle()); 
    if(flacEditor->flaceditor_getYear() != NULL)
        ALOGV("tagseditor::tagsEditor_testModule() #### flaceditor_getYear() : %s", flacEditor->flaceditor_getYear()); 
    if(flacEditor->flaceditor_getCDTrackNumber() != NULL)
        ALOGV("tagseditor::tagsEditor_testModule() #### flaceditor_getCDTrackNumber() : %s", flacEditor->flaceditor_getCDTrackNumber()); 
    if(flacEditor->flaceditor_getGraceNoteUIDOwner() != NULL)
        ALOGV("tagseditor::tagsEditor_testModule() #### flaceditor_getGraceNoteUIDOwner() : %s", flacEditor->flaceditor_getGraceNoteUIDOwner()); 
    if(flacEditor->flaceditor_getGraceNoteTAGID() != NULL)
        ALOGV("tagseditor::tagsEditor_testModule() #### flaceditor_getGraceNoteTAGID() : %s", flacEditor->flaceditor_getGraceNoteTAGID()); 
    if(flacEditor->flaceditor_getCopyright() != NULL)
        ALOGV("tagseditor::tagsEditor_testModule() #### flaceditor_getCopyright() : %s", flacEditor->flaceditor_getCopyright()); 
    if(flacEditor->flaceditor_getIsAlbumArt() != false)
        ALOGV("tagseditor::tagsEditor_testModule() #### flaceditor_getIsAlbumArt() : %d", flacEditor->flaceditor_getIsAlbumArt()); 
    if(flacEditor->flaceditor_getAlbumArtMime() != NULL)
        ALOGV("tagseditor::tagsEditor_testModule() #### flaceditor_getAlbumArtMime() : %s", flacEditor->flaceditor_getAlbumArtMime()); 
    if(flacEditor->flaceditor_getAlbumArtLength() != 0)
        ALOGV("tagseditor::tagsEditor_testModule() #### flaceditor_getAlbumArtLength() : %s", flacEditor->flaceditor_getAlbumArtLength());

    ALOGE("tagseditor::tagsEditor_testModule #### Saving Meta Data ==========");

#if 1        
        struct stat srcstat;  
        unsigned long lLength = 0;
        unsigned char *lBuffer = NULL;
        if(0 == stat("/storage/emulated/0/0.jpg", &srcstat)) {
            lLength = srcstat.st_size;    
            lBuffer = (unsigned char *) malloc(lLength + 1);
            FILE *f = fopen("/storage/emulated/0/0.jpg", "rb");
            if(0 == f) {
                if(lBuffer != NULL) free(lBuffer);
            } else {                    
                if(fread(lBuffer, 1, lLength, f) == (size_t)lLength) {
                    flacEditor->flaceditor_save("album", 
                                 "artist",
                                 "albumartist", 
                                 "composer", 
                                 "genre", 
                                 "\xED\x95\x9C\xEA\xB8\x80\xEA\xB3\xBC\x20\xEC\x9C\xA0\xEB\x8B\x88\xEC\xBD\x94\xEB\x93\x9C", //한글과 유니코드
                                 "year", 
                                 "tracknumber", 
                                 "ufid", 
                                 "gnid", 
                                 "copyright",
                                 (const char *)lBuffer, 
                                 (const int)lLength);
                }
                fclose(f);
            }
            if(lBuffer != NULL)
                free(lBuffer);
            lBuffer = NULL;
        }
#endif

    delete flacEditor;

    ALOGE("tagseditor::tagsEditor_testModule #### Test 1 ========== Wav editor");
    
    waveditor *wavEditor = new waveditor(outFilePathWav);
    delete wavEditor;

    ALOGE("tagseditor::tagsEditor_testModule #### Test 2 ========== Wav interface");
    
    tagseditor *tagsEditor = NULL;
    tagsEditor = new wav_editor();
    tagsEditor->tagsEditor_open(outFilePathWav);
    tagsEditor->tagsEditor_save(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0);
    delete tagsEditor;
    tagsEditor = NULL;

    ALOGE("tagseditor::tagsEditor_testModule #### Test 3 ========== Flac interface");
    
    tagsEditor = new flac_editor();
    tagsEditor->tagsEditor_open(outFilePathFlac);
    tagsEditor->tagsEditor_save(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0);
    delete tagsEditor;
    tagsEditor = NULL;

    ALOGE("tagseditor::tagsEditor_testModule #### Test 4 ========== Flac TagEditorCreate");
    
    metaTag<tagseditor> extractorFlac = tagseditor::TagEditorCreate("audio/flac");
    extractorFlac->tagsEditor_open(outFilePathFlac);
    extractorFlac->tagsEditor_save(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0);

    ALOGE("tagseditor::tagsEditor_testModule #### Test 5 ========== Wav TagEditorCreate");
    
    metaTag<tagseditor> extractorWav = tagseditor::TagEditorCreate("audio/x-wav");
    extractorWav->tagsEditor_open(outFilePathWav);
    extractorWav->tagsEditor_save(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0);   
     
}

} // namespace android
