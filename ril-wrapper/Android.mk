LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE           := libril-wrapper
LOCAL_VENDOR_MODULE    := true
LOCAL_SRC_FILES        := ril-wrapper.c
LOCAL_SHARED_LIBRARIES := libdl liblog libril_sony_kitakami libcutils
LOCAL_CFLAGS           := -Wall -Werror

ifeq ($(SIM_COUNT), 2)
    LOCAL_CFLAGS += -DANDROID_MULTI_SIM -DANDROID_SIM_COUNT_2
endif

include $(BUILD_SHARED_LIBRARY)
