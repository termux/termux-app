LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_LDLIBS := -llog
LOCAL_MODULE := local-filesystem-socket
LOCAL_SRC_FILES := local-filesystem-socket.cpp
include $(BUILD_SHARED_LIBRARY)
