LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE:= libnative-entrypoint
LOCAL_SRC_FILES:= main.cpp
LOCAL_LDLIBS:= -llog -landroid -ldl
include $(BUILD_SHARED_LIBRARY)
