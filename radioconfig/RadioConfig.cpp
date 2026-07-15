/*
 * Copyright (C) 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
#include "RadioConfig.h"

namespace android {
namespace hardware {
namespace radio {
namespace config {
namespace V1_1 {
namespace implementation {

using ::android::hardware::radio::V1_0::RadioError;
using ::android::hardware::radio::V1_0::RadioResponseInfo;
using ::android::hardware::radio::V1_0::RadioResponseType;
using ::android::hardware::radio::config::V1_0::SimSlotStatus;

static RadioResponseInfo ok(int32_t serial) {
    return {RadioResponseType::SOLICITED, serial, RadioError::NONE};
}
static RadioResponseInfo notSupported(int32_t serial) {
    return {RadioResponseType::SOLICITED, serial, RadioError::REQUEST_NOT_SUPPORTED};
}

// V1_0 methods.
Return<void> RadioConfig::setResponseFunctions(
    const sp<::android::hardware::radio::config::V1_0::IRadioConfigResponse>& radioConfigResponse,
    const sp<::android::hardware::radio::config::V1_0::IRadioConfigIndication>& radioConfigIndication) {
    mRadioConfigResponse = radioConfigResponse;
    mRadioConfigIndication = radioConfigIndication;
    mRadioConfigResponseV1_1 =
        ::android::hardware::radio::config::V1_1::IRadioConfigResponse::castFrom(radioConfigResponse)
            .withDefault(nullptr);
    return Void();
}

Return<void> RadioConfig::getSimSlotsStatus(int32_t serial) {
    if (mRadioConfigResponse == nullptr) return Void();
    hidl_vec<SimSlotStatus> slotStatus;
    mRadioConfigResponse->getSimSlotsStatusResponse(ok(serial), slotStatus);
    return Void();
}

Return<void> RadioConfig::setSimSlotsMapping(int32_t serial, const hidl_vec<uint32_t>& /*slotMap*/) {
    if (mRadioConfigResponse != nullptr) mRadioConfigResponse->setSimSlotsMappingResponse(ok(serial));
    return Void();
}

// V1_1 methods.
Return<void> RadioConfig::getPhoneCapability(int32_t serial) {
    if (mRadioConfigResponseV1_1 == nullptr) return Void();
    ::android::hardware::radio::config::V1_1::PhoneCapability cap = {
        .maxActiveData = 0,
        .maxActiveInternetData = 0,
        .isInternetLingeringSupported = false,
        .logicalModemList = {},
    };
    mRadioConfigResponseV1_1->getPhoneCapabilityResponse(ok(serial), cap);
    return Void();
}

Return<void> RadioConfig::setPreferredDataModem(int32_t serial, uint8_t /*modemId*/) {
    if (mRadioConfigResponseV1_1 != nullptr)
        mRadioConfigResponseV1_1->setPreferredDataModemResponse(ok(serial));
    return Void();
}

Return<void> RadioConfig::setModemsConfig(int32_t serial,
    const ::android::hardware::radio::config::V1_1::ModemsConfig& /*modemsConfig*/) {
    if (mRadioConfigResponseV1_1 != nullptr)
        mRadioConfigResponseV1_1->setModemsConfigResponse(notSupported(serial));
    return Void();
}

Return<void> RadioConfig::getModemsConfig(int32_t serial) {
    if (mRadioConfigResponseV1_1 != nullptr)
        mRadioConfigResponseV1_1->getModemsConfigResponse(notSupported(serial), {});
    return Void();
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace config
}  // namespace radio
}  // namespace hardware
}  // namespace android
