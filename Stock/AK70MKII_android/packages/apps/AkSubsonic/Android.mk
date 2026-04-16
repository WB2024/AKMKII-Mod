#
# AkSubsonic — Subsonic/Navidrome streaming client for AK70 MKII
# Build: AOSP Android.mk (minSdkVersion 21, HDPI, ARM)
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_PACKAGE_NAME := AkSubsonic

# Match AkIME: HDPI-only resources, shared certificate, system priv-app
LOCAL_AAPT_FLAGS += -c hdpi

LOCAL_PRIVILEGED_MODULE := true
LOCAL_CERTIFICATE := shared

include $(BUILD_PACKAGE)
