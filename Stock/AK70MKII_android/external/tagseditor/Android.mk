#
# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# 

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
        flac_editor/flaceditor.cpp \
	flac_editor/flacencoder.c \
	flac_editor/flacencoder_albumart.c \
	flac_editor/flac_editor.cpp \
	wav_editor/tag_edit_generator.c \
	wav_editor/waveditor.cpp \
	wav_editor/wav_editor.cpp \
	dff_editor/dffeditor.cpp \
	dff_editor/dff_editor.cpp \
	tagseditor.cpp

LOCAL_C_INCLUDES:= \
        $(TOP)/external/flac/include \
        $(TOP)/external/tagseditor/include \
        $(TOP)/external/tagseditor \
        $(TOP)/frameworks/av/media/libstagefright/include \

LOCAL_STATIC_LIBRARIES := \
        libstagefright_id3 \
        libFLAC \

LOCAL_MODULE:= libtagseditor

# LOCAL_CFLAGS+= -D_REENTRANT -DPIC -D_FILE_OFFSET_BITS=64
LOCAL_CFLAGS+= -D_REENTRANT -DPIC

LOCAL_CFLAGS+= -O3  

LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

