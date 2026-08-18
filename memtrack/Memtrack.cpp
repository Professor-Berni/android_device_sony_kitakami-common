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

#define LOG_TAG "android.hardware.memtrack-service.kitakami"

#include "Memtrack.h"
#include <android-base/logging.h>
#include <hardware/hardware.h>

namespace aidl {
namespace android {
namespace hardware {
namespace memtrack {

static_assert(MEMTRACK_TYPE_OTHER == static_cast<int>(MemtrackType::OTHER),
              "MemtrackType::OTHER must match legacy value.");
static_assert(MEMTRACK_TYPE_GL == static_cast<int>(MemtrackType::GL),
              "MemtrackType::GL must match legacy value.");
static_assert(MEMTRACK_TYPE_GRAPHICS == static_cast<int>(MemtrackType::GRAPHICS),
              "MemtrackType::GRAPHICS must match legacy value.");
static_assert(MEMTRACK_TYPE_MULTIMEDIA == static_cast<int>(MemtrackType::MULTIMEDIA),
              "MemtrackType::MULTIMEDIA must match legacy value.");
static_assert(MEMTRACK_TYPE_CAMERA == static_cast<int>(MemtrackType::CAMERA),
              "MemtrackType::CAMERA must match legacy value.");

Memtrack::Memtrack() : mModule(nullptr) {
    const hw_module_t* hwModule = nullptr;

    int ret = hw_get_module(MEMTRACK_HARDWARE_MODULE_ID, &hwModule);
    if (ret != 0) {
        LOG(ERROR) << "Failed to load " << MEMTRACK_HARDWARE_MODULE_ID << " module: " << ret;
        return;
    }

    // memtrack_msm leaves methods->open NULL; the module itself is the device.
    if (hwModule->methods == nullptr || hwModule->methods->open == nullptr) {
        mModule = reinterpret_cast<const memtrack_module_t*>(hwModule);
    } else {
        memtrack_module_t* device = nullptr;
        ret = hwModule->methods->open(hwModule, MEMTRACK_HARDWARE_MODULE_ID,
                                      reinterpret_cast<hw_device_t**>(&device));
        if (ret != 0 || device == nullptr) {
            LOG(ERROR) << "Failed to open legacy memtrack HAL: " << ret;
            return;
        }
        mModule = device;
    }

    if (mModule->init != nullptr) {
        ret = mModule->init(mModule);
        if (ret != 0) {
            LOG(ERROR) << "Legacy memtrack HAL init failed: " << ret;
        }
    }
}

ndk::ScopedAStatus Memtrack::getMemory(int32_t pid, MemtrackType type,
                                       std::vector<MemtrackRecord>* _aidl_return) {
    _aidl_return->clear();

    if (pid < 0) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    if (mModule == nullptr || mModule->getMemory == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    // First call with num_records == 0 only asks the HAL how many records it has.
    size_t numRecords = 0;
    int ret = mModule->getMemory(mModule, pid, static_cast<memtrack_type>(type), nullptr,
                                 &numRecords);
    if (ret != 0 || numRecords == 0) {
        return ndk::ScopedAStatus::ok();
    }

    std::vector<memtrack_record> legacyRecords(numRecords);
    ret = mModule->getMemory(mModule, pid, static_cast<memtrack_type>(type), legacyRecords.data(),
                             &numRecords);
    if (ret != 0) {
        return ndk::ScopedAStatus::ok();
    }

    numRecords = std::min(numRecords, legacyRecords.size());
    _aidl_return->resize(numRecords);
    for (size_t i = 0; i < numRecords; i++) {
        (*_aidl_return)[i].sizeInBytes = static_cast<int64_t>(legacyRecords[i].size_in_bytes);
        (*_aidl_return)[i].flags = static_cast<int32_t>(legacyRecords[i].flags);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Memtrack::getGpuDeviceInfo(std::vector<DeviceInfo>* _aidl_return) {
    // The legacy HAL predates this call and exposes no per-GPU-device accounting.
    _aidl_return->clear();
    return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
            EX_UNSUPPORTED_OPERATION, "legacy memtrack HAL has no GPU device info");
}

}  // namespace memtrack
}  // namespace hardware
}  // namespace android
}  // namespace aidl
