/*
 * Copyright (C) 2026 LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 */

#include <android-base/logging.h>
#include <android/binder_interface_utils.h>
#include <health-impl/ChargerUtils.h>
#include <health-impl/Health.h>
#include <health/utils.h>

using aidl::android::hardware::health::charger::ChargerCallback;
using aidl::android::hardware::health::charger::ChargerModeMain;
using aidl::android::hardware::health::HalHealthLoop;
using aidl::android::hardware::health::Health;

static constexpr const char* gInstanceName = "default";
static constexpr std::string_view gChargerArg{"--charger"};

static void init_healthd_config(healthd_config* config) {
    config->batteryChargeCounterPath =
            android::String8("/sys/class/power_supply/bms/charge_now");
    config->batteryFullChargeDesignCapacityUahPath =
            android::String8("/sys/class/power_supply/bms/charge_full_design");
}

int main(int argc, char** argv) {
#ifdef __ANDROID_RECOVERY__
    android::base::InitLogging(argv, android::base::KernelLogger);
#endif

    auto config = std::make_unique<healthd_config>();
    ::android::hardware::health::InitHealthdConfig(config.get());
    init_healthd_config(config.get());

    auto binder = ndk::SharedRefBase::make<Health>(gInstanceName, std::move(config));

    if (argc >= 2 && argv[1] == gChargerArg) {
        return ChargerModeMain(binder, std::make_shared<ChargerCallback>(binder));
    }

    LOG(INFO) << "Starting health HAL.";
    auto hal_health_loop = std::make_shared<HalHealthLoop>(binder, binder);
    return hal_health_loop->StartLoop();
}
