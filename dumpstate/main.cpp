/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <aidl/android/hardware/dumpstate/BnDumpstateDevice.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>

namespace {
using ::aidl::android::hardware::dumpstate::BnDumpstateDevice;
using ::aidl::android::hardware::dumpstate::IDumpstateDevice;
using ::ndk::ScopedAStatus;
using ::ndk::ScopedFileDescriptor;

const char kVerboseLoggingProperty[] = "persist.dumpstate.verbose_logging.enabled";

struct DumpstateDevice : public BnDumpstateDevice {
    ScopedAStatus dumpstateBoard(const std::vector<ScopedFileDescriptor>& fds,
                                 IDumpstateDevice::DumpstateMode mode,
                                 int64_t /*timeoutMillis*/) override {
        if (fds.empty()) {
            ALOGE("no FDs\n");
            return ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT, "No FDs");
        }

        switch (mode) {
            case IDumpstateDevice::DumpstateMode::FULL:
            case IDumpstateDevice::DumpstateMode::DEFAULT:
                return ScopedAStatus::ok();

            default:
                ALOGE("The requested mode is not supported: %d\n", static_cast<int>(mode));
                return ScopedAStatus::fromServiceSpecificError(
                        IDumpstateDevice::ERROR_UNSUPPORTED_MODE);
        }
    }

    ScopedAStatus setVerboseLoggingEnabled(bool enable) override {
        ::android::base::SetProperty(kVerboseLoggingProperty, enable ? "true" : "false");
        return ScopedAStatus::ok();
    }

    ScopedAStatus getVerboseLoggingEnabled(bool* _aidl_return) override {
        *_aidl_return = ::android::base::GetBoolProperty(kVerboseLoggingProperty, false);
        return ScopedAStatus::ok();
    }
};
}  // namespace

int main(int, char**) {
    ABinderProcess_setThreadPoolMaxThreadCount(0);

    auto dumpstate = ::ndk::SharedRefBase::make<DumpstateDevice>();
    const std::string instance = std::string(DumpstateDevice::descriptor) + "/default";

    if (AServiceManager_registerLazyService(dumpstate->asBinder().get(), instance.c_str()) !=
        STATUS_OK) {
        ALOGE("Could not register service.");
        return 1;
    }

    ABinderProcess_joinThreadPool();
    return 0;
}
