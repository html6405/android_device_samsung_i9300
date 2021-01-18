
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	sec_keyboard.c

LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_CFLAGS := -DANDROID_COMPATIBLE

LOCAL_MODULE := sec_keyboard

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)