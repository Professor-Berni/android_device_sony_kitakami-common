/*
 * Copyright (C) 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
#define LOG_TAG "android.hardware.radio.config@1.1-service.satsuki"

#include <android/hardware/radio/config/1.1/IRadioConfig.h>
#include <hidl/HidlTransportSupport.h>
#include <log/log.h>

#include "RadioConfig.h"

using android::OK;
using android::sp;
using android::status_t;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::radio::config::V1_1::IRadioConfig;
using android::hardware::radio::config::V1_1::implementation::RadioConfig;

int main() {
    configureRpcThreadpool(1, true);

    sp<IRadioConfig> radioConfig = new RadioConfig;
    status_t status = radioConfig->registerAsService();
    ALOGW_IF(status != OK, "Could not register IRadioConfig 1.1");
    ALOGD("RadioConfig 1.1 service (satsuki) is ready.");

    joinRpcThreadpool();
    return 0;
}
