#
# Copyright (C) 2009 The Android Open Source Project
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

VERSION=1.3.12

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := scsi_linux.c mtxl.c mtxl.h mtx.h 
LOCAL_SHARED_LIBRARIES := libcutils libutils
LOCAL_CFLAGS := -DLONG_PRINT_REQUEST_SENSE=1 -DVERSION="\"$(VERSION)\"" 
LOCAL_C_INCLUDES := ../../bionic/libc/common/ $(LOCAL_PATH)/include

LOCAL_MODULE := libmtxl
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include \

include $(BUILD_STATIC_LIBRARY)

