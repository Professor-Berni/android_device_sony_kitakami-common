#
# Copyright (C) 2017 The LineageOS Project
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

# RRO (Runtime Resource Overlay)
PRODUCT_ENFORCE_RRO_EXCLUDED_OVERLAYS += $(LOCAL_PATH)/overlay-radio/packages/apps/CarrierConfig

# Permissions
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.telephony.gsm.xml

# Common
PRODUCT_PACKAGES += \
    libqdMetaData.system

# RCS
PRODUCT_PACKAGES += \
    rcs_service_aidl \
    rcs_service_aidl.xml \
    rcs_service_api \
    rcs_service_api.xml

# RIL
PRODUCT_PACKAGES += \
    android.hardware.radio@1.2-radio-service \
    android.hardware.radio.config@1.0-service \
    android.hardware.secure_element@1.2

# Telephony
PRODUCT_PACKAGES += \
    telephony-ext

PRODUCT_BOOT_JARS += \
    telephony-ext

# Properties
PRODUCT_PROPERTY_OVERRIDES += \
    persist.data.netmgrd.qos.enable=true \
    persist.data.qmi.adb_logmask=0 \
    persist.radio.add_power_save=1 \
    persist.radio.apm_sim_not_pwdn=1 \
    persist.radio.block_allow_data=1 \
    persist.radio.custom_ecc=1 \
    persist.radio.mt_sms_ack=19 \
    persist.radio.oem_socket=true \
    persist.radio.redir_party_num=0 \
    persist.radio.sib16_support=1 \
    persist.radio.wait_for_pbm=1

PRODUCT_PROPERTY_OVERRIDES += \
    vendor.rild.libargs=-d[SPACE]/dev/smd0 \
    vendor.rild.libpath=/vendor/lib64/libril-wrapper.so \
    ril.subscription.types=NV,RUIM

PRODUCT_PROPERTY_OVERRIDES += \
    ro.data.large_tcp_window_size=true \
    ro.ril.telephony.mqanelements=5 \
    ro.telephony.call_ring.multiple=false \
    ro.use_data_netmgrd=true

