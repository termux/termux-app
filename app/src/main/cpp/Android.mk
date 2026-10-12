LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libtermux-bootstrap
LOCAL_SRC_FILES := termux-bootstrap-zip.S termux-bootstrap.c
include $(BUILD_SHARED_LIBRARY)

# The precompiled library must be added separately from the uploaded ZIP.
# If absent, ship the APK without the optional native encoder.
VORBISLIB_PREBUILT := $(LOCAL_PATH)/../jniLibs/$(TARGET_ARCH_ABI)/libvorbislib.so
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
ifneq ($(wildcard $(VORBISLIB_PREBUILT)),)
include $(CLEAR_VARS)
LOCAL_MODULE := vorbislib
LOCAL_SRC_FILES := ../jniLibs/arm64-v8a/libvorbislib.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mlbb_vorbis_jni
LOCAL_SRC_FILES := mlbb_vorbis_jni.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_SHARED_LIBRARIES := vorbislib
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)
endif
endif
