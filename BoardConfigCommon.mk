#
# Copyright (C) 2016 The CyanogenMod Project
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

COMMON_PATH := device/sony/kitakami-common

TARGET_SPECIFIC_HEADER_PATH += $(COMMON_PATH)/include

BOARD_VENDOR := sony

# Bootloader
TARGET_BOOTLOADER_BOARD_NAME := MSM8994
TARGET_NO_BOOTLOADER := true

# Platform
TARGET_BOARD_PLATFORM := msm8994
TARGET_BOARD_PLATFORM_GPU := qcom-adreno430

# Architecture
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_ABI2 :=
TARGET_CPU_VARIANT := generic

TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv7-a-neon
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi
TARGET_2ND_CPU_VARIANT := cortex-a53.a57

TARGET_USES_64_BIT_BINDER := true
TARGET_USES_64_BIT_BCMDHD := true

ENABLE_CPUSETS := true

# Boot image/kernel
BOARD_KERNEL_CMDLINE := androidboot.hardware=qcom user_debug=31 msm_rtb.filter=0x237 ehci-hcd.park=3 lpm_levels.sleep_disabled=1 boot_cpus=0-5 dwc3_msm.hvdcp_max_current=1500 dwc3_msm.prop_chg_detect=Y coherent_pool=2M androidboot.selinux=enforcing
BOARD_KERNEL_IMAGE_NAME := Image.gz-dtb
BOARD_KERNEL_PAGESIZE := 4096
BOARD_KERNEL_BASE := 0x00000000
BOARD_KERNEL_TAGS_OFFSET := 0x00000100
BOARD_RAMDISK_OFFSET := 0x02000000
TARGET_KERNEL_APPEND_DTB := true
TARGET_KERNEL_ARCH := arm64
TARGET_KERNEL_HEADER_ARCH := arm64
TARGET_KERNEL_CROSS_COMPILE_PREFIX := aarch64-linux-android-
TARGET_KERNEL_SOURCE := kernel/sony/msm8994

# Audio
AUDIO_FEATURE_ENABLED_FLAC_OFFLOAD := true
AUDIO_FEATURE_ENABLED_KPI_OPTIMIZE := true
AUDIO_FEATURE_ENABLED_MULTI_VOICE_SESSIONS := true

AUDIO_USE_LL_AS_PRIMARY_OUTPUT := true
BOARD_USES_ALSA_AUDIO := true

# Bluetooth
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(COMMON_PATH)/bluetooth
BOARD_CUSTOM_BT_CONFIG := $(COMMON_PATH)/bluetooth/vnd_generic.txt
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true

# Camera
TARGET_CAMERASERVICE_CLOSES_NATIVE_HANDLES := true
TARGET_USES_MEDIA_EXTENSIONS := true

# Charger
BOARD_CHARGER_ENABLE_SUSPEND := true
BOARD_CHARGER_SHOW_PERCENTAGE := true
BOARD_CHARGER_DISABLE_INIT_BLANK := true

# Display
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3
TARGET_USES_ION := true
TARGET_USES_NEW_ION_API := true
TARGET_USES_C2D_COMPOSITION := true
USE_OPENGL_RENDERER := true

MAX_EGL_CACHE_KEY_SIZE := 12*1024
MAX_EGL_CACHE_SIZE := 2048*1024

HAVE_ADRENO_SOURCE := false
OVERRIDE_RS_DRIVER := libRSDriver_adreno.so

VSYNC_EVENT_PHASE_OFFSET_NS := 2000000
SF_VSYNC_EVENT_PHASE_OFFSET_NS := 6000000

# Partitions types
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true

# Lights
TARGET_PROVIDES_LIBLIGHT := true

# Init
TARGET_PLATFORM_DEVICE_BASE := /devices/soc.0/

# Power
TARGET_POWERHAL_VARIANT := qcom

# RPC
TARGET_NO_RPC := true

# Keymaster
TARGET_PROVIDES_KEYMASTER := true

# Sensors
USE_SENSOR_MULTI_HAL := true

# Qualcomm support
BOARD_USES_QCOM_HARDWARE := true

# Recovery
TARGET_RECOVERY_FSTAB := $(COMMON_PATH)/rootdir/fstab.qcom
TARGET_USERIMAGES_USE_EXT4 := true

# RIL
BOARD_PROVIDES_LIBRIL := true
TARGET_RIL_VARIANT := caf

# SELinux
include device/qcom/sepolicy/sepolicy.mk

BOARD_SEPOLICY_DIRS += \
    $(COMMON_PATH)/sepolicy
