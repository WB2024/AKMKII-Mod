LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	alac.cpp \
	demux.cpp \
	stream.cpp \
	wavwriter.cpp

#LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_CFLAGS += -D_REENTRANT -DPIC -DU_COMMON_IMPLEMENTATION -fPIC
LOCAL_CFLAGS += -O3 -funroll-loops -finline-functions

LOCAL_LDLIBS += -lm

LOCAL_C_INCLUDES += \
	$(JNI_H_INCLUDE) \
	$(TOP)/frameworks/av/include \
	$(TOP)/frameworks/av/media/libstagefright \
	$(TOP)/frameworks/native/include 
	
LOCAL_ARM_MODE := arm

LOCAL_MODULE := libALAC

LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)
