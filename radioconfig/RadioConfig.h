/*
 * Copyright (C) 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef KITAKAMI_RADIO_CONFIG_V1_1_RADIOCONFIG_H
#define KITAKAMI_RADIO_CONFIG_V1_1_RADIOCONFIG_H

#include <android/hardware/radio/config/1.1/IRadioConfig.h>
#include <android/hardware/radio/config/1.1/IRadioConfigResponse.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace radio {
namespace config {
namespace V1_1 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct RadioConfig : public V1_1::IRadioConfig {
    sp<::android::hardware::radio::config::V1_0::IRadioConfigResponse> mRadioConfigResponse;
    sp<::android::hardware::radio::config::V1_1::IRadioConfigResponse> mRadioConfigResponseV1_1;
    sp<::android::hardware::radio::config::V1_0::IRadioConfigIndication> mRadioConfigIndication;

    // Methods from ::android::hardware::radio::config::V1_0::IRadioConfig follow.
    Return<void> setResponseFunctions(
        const sp<::android::hardware::radio::config::V1_0::IRadioConfigResponse>& radioConfigResponse,
        const sp<::android::hardware::radio::config::V1_0::IRadioConfigIndication>& radioConfigIndication) override;
    Return<void> getSimSlotsStatus(int32_t serial) override;
    Return<void> setSimSlotsMapping(int32_t serial, const hidl_vec<uint32_t>& slotMap) override;

    // Methods from ::android::hardware::radio::config::V1_1::IRadioConfig follow.
    Return<void> getPhoneCapability(int32_t serial) override;
    Return<void> setPreferredDataModem(int32_t serial, uint8_t modemId) override;
    Return<void> setModemsConfig(int32_t serial,
        const ::android::hardware::radio::config::V1_1::ModemsConfig& modemsConfig) override;
    Return<void> getModemsConfig(int32_t serial) override;
};

}  // namespace implementation
}  // namespace V1_1
}  // namespace config
}  // namespace radio
}  // namespace hardware
}  // namespace android

#endif  // KITAKAMI_RADIO_CONFIG_V1_1_RADIOCONFIG_H
