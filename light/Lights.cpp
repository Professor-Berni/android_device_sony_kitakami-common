/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.light-service.kitakami"

#include "Lights.h"
#include <android-base/logging.h>
#include <errno.h>
#include <stdio.h>

namespace aidl {
namespace android {
namespace hardware {
namespace light {

static_assert(LIGHT_FLASH_NONE == static_cast<int>(FlashMode::NONE),
              "FlashMode::NONE must match legacy value.");
static_assert(LIGHT_FLASH_TIMED == static_cast<int>(FlashMode::TIMED),
              "FlashMode::TIMED must match legacy value.");
static_assert(LIGHT_FLASH_HARDWARE == static_cast<int>(FlashMode::HARDWARE),
              "FlashMode::HARDWARE must match legacy value.");

static_assert(BRIGHTNESS_MODE_USER == static_cast<int>(BrightnessMode::USER),
              "BrightnessMode::USER must match legacy value.");
static_assert(BRIGHTNESS_MODE_SENSOR == static_cast<int>(BrightnessMode::SENSOR),
              "BrightnessMode::SENSOR must match legacy value.");
static_assert(BRIGHTNESS_MODE_LOW_PERSISTENCE == static_cast<int>(BrightnessMode::LOW_PERSISTENCE),
              "BrightnessMode::LOW_PERSISTENCE must match legacy value.");

static const std::map<LightType, const char*> kLogicalLights = {
        {LightType::BACKLIGHT, LIGHT_ID_BACKLIGHT},
        {LightType::BATTERY, LIGHT_ID_BATTERY},
        {LightType::NOTIFICATIONS, LIGHT_ID_NOTIFICATIONS},
        {LightType::ATTENTION, LIGHT_ID_ATTENTION},
};

Lights::Lights() {
    const hw_module_t* module = nullptr;

    int ret = hw_get_module(LIGHTS_HARDWARE_MODULE_ID, &module);
    if (ret != 0) {
        LOG(ERROR) << "Failed to load " << LIGHTS_HARDWARE_MODULE_ID << " module: " << ret;
        return;
    }

    for (const auto& [type, name] : kLogicalLights) {
        light_device_t* device = nullptr;

        ret = module->methods->open(module, name, reinterpret_cast<hw_device_t**>(&device));
        if (ret != 0 || device == nullptr) {
            LOG(ERROR) << "Failed to open light " << name << ": " << ret;
            continue;
        }

        mLights.emplace(type, device);
    }
}

ndk::ScopedAStatus Lights::setLightState(int32_t id, const HwLightState& state) {
    auto it = mLights.find(static_cast<LightType>(id));
    if (it == mLights.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    light_device_t* device = it->second;

    light_state_t legacyState = {
            .color = static_cast<unsigned int>(state.color),
            .flashMode = static_cast<int>(state.flashMode),
            .flashOnMS = state.flashOnMs,
            .flashOffMS = state.flashOffMs,
            .brightnessMode = static_cast<int>(state.brightnessMode),
    };

    int ret = device->set_light(device, &legacyState);
    if (ret == -ENOSYS) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    } else if (ret != 0) {
        LOG(ERROR) << "Failed to set light " << id << ": " << ret;
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Lights::getLights(std::vector<HwLight>* _aidl_return) {
    for (const auto& [type, device] : mLights) {
        (void)device;
        _aidl_return->push_back({
                .id = static_cast<int32_t>(type),
                .ordinal = 0,
                .type = type,
        });
    }

    return ndk::ScopedAStatus::ok();
}

binder_status_t Lights::dump(int fd, const char** /*args*/, uint32_t /*numArgs*/) {
    dprintf(fd, "Lights AIDL (legacy lights.msm8994 wrapper):\n");
    for (const auto& [type, device] : mLights) {
        (void)device;
        dprintf(fd, "  %s\n", kLogicalLights.at(type));
    }

    return STATUS_OK;
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
