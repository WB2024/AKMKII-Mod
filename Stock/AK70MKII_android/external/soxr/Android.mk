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
#libsoxr

LIB_VERSION:=libsoxr

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_CFLAGS := -Wall -DSOXR_LIB -DPFFFT_SIMD_DISABLE

LOCAL_SRC_FILES := \
		src/data-io.c \
		src/dbesi0.c \
		src/fft4g32.c \
		src/fft4g64.c \
		src/filter.c \
		src/lsr.c \
		src/pffft.c \
		src/rate32.c \
		src/rate64.c \
		src/soxr.c \
		src/vr32.c

LOCAL_C_INCLUDES += \
		$(LOCAL_PATH)/src

LOCAL_LDLIBS := 

LOCAL_MODULE := libsoxr

include $(BUILD_SHARED_LIBRARY)