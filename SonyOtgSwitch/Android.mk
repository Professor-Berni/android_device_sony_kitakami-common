LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    $(call all-java-files-under, src)

LOCAL_RESOURCE_DIR := \
    $(LOCAL_PATH)/res

LOCAL_STATIC_JAVA_LIBRARIES := \
    org.cyanogenmod.platform.internal

LOCAL_AAPT_FLAGS := \
    --auto-add-overlay

LOCAL_CERTIFICATE := platform
LOCAL_MODULE_TAGS := optional

LOCAL_PACKAGE_NAME := SonyOtgSwitch

include $(BUILD_PACKAGE)
