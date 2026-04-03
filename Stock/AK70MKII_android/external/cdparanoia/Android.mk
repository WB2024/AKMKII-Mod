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

###############################################################
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES:= \
	$(TOP)/external/flac/include \

LOCAL_SRC_FILES := \
	paranoia/paranoia.c \
	paranoia/p_block.c \
	paranoia/overlap.c \
	paranoia/gap.c \
	paranoia/isort.c \
	flacencoder/flacencoder.c \
	flacencoder/flacencoder_albumart.c \
	taggenerator/taggenerator.c \

LOCAL_STATIC_LIBRARIES := \
	libFLAC \

# ##################################################################
# 2018.07.18 iriver-eddy #MQA_DECODE
ifeq ($(BOARD_SUPPORT_MQA_DECODE),true)
LOCAL_CFLAGS += -D_SUPPORT_MQA_DECODE_
endif
# ##################################################################

LOCAL_MODULE:= libparanoia

LOCAL_CFLAGS+= -D_REENTRANT -DPIC
LOCAL_CFLAGS+= -O3  

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/paranoia

include $(BUILD_STATIC_LIBRARY)

###############################################################
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	interface/scan_devices.c \
	interface/common_interface.c \
	interface/cooked_interface.c \
	interface/interface.c \
        interface/scsi_interface.c \
	interface/smallft.c \
	interface/toc.c \
	interface/test_interface.c \

LOCAL_MODULE:= libcdda_interface

LOCAL_CFLAGS+= -D_REENTRANT -DPIC
LOCAL_CFLAGS+= -O3 

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/interface \
	$(LOCAL_PATH)/scsi/include

include $(BUILD_STATIC_LIBRARY)

###############################################################
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	scsilib/astoi.c \
	scsilib/comerr.c \
	scsilib/error.c \
	scsilib/fgetline.c \
	scsilib/filewrite.c \
	scsilib/fillbytes.c \
	scsilib/flag.c \
	scsilib/flush.c \
	scsilib/geterrno.c \
	scsilib/gettime.c \
	scsilib/movebytes.c \
	scsilib/port_lib.c \
	scsilib/raisecond.c \
	scsilib/saveargs.c \
	scsilib/scsi_cmds.c \
	scsilib/scsierrs.c \
	scsilib/scsiopen.c \
	scsilib/scsitransp.c \
	scsilib/streql.c \

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/scsilib \
	$(LOCAL_PATH)/scsilib/include \
	$(LOCAL_PATH)/scsilib/scg \
	$(LOCAL_PATH)/scsi/include

LOCAL_SHARED_LIBRARIES:= libc libcutils

LOCAL_MODULE:= libscsi_interface
LOCAL_CFLAGS+= -D_REENTRANT -DPIC
LOCAL_CFLAGS+= -O3  

include $(BUILD_STATIC_LIBRARY)

###############################################################
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= interface paranoia
LOCAL_SHARED_LIBRARIES:= \
   libutils \
   libcutils
   
LOCAL_SRC_FILES:= main.c report.c header.c buffering_write.c cachetest.c
LOCAL_MODULE := cdparanoia
LOCAL_STATIC_LIBRARIES:= libparanoia libcdda_interface libFLAC
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)


