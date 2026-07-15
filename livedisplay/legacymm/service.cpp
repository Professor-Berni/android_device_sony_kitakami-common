/*
 * SPDX-FileCopyrightText: 2019 The LineageOS Project
 * SPDX-FileCopyrightText: 2026 The LineageOS Project (AIDL port)
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "vendor.lineage.livedisplay-service.legacymm"

#include <dlfcn.h>

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "DisplayModes.h"
#include "PictureAdjustment.h"

#define MM_DISP_LIB "libmm-disp-apis.so"

using ::aidl::vendor::lineage::livedisplay::legacymm::DisplayModes;
using ::aidl::vendor::lineage::livedisplay::legacymm::PictureAdjustment;

int main() {
    void* libHandle = nullptr;
    int (*disp_api_init)(int32_t) = nullptr;
    std::shared_ptr<DisplayModes> dm;
    std::shared_ptr<PictureAdjustment> pa;
    int status = 0;

    LOG(INFO) << "LiveDisplay HAL service is starting.";

    libHandle = dlopen(MM_DISP_LIB, RTLD_NOW);
    if (libHandle == nullptr) {
        LOG(ERROR) << "Can not get " << MM_DISP_LIB << " (" << dlerror() << ")";
        goto shutdown;
    }

    disp_api_init = reinterpret_cast<int (*)(int32_t)>(dlsym(libHandle, "disp_api_init"));
    if (disp_api_init == nullptr) {
        LOG(ERROR) << "Can not get disp_api_init from " << MM_DISP_LIB << " (" << dlerror() << ")";
        goto shutdown;
    }

    status = disp_api_init(0);
    if (status != 0) {
        LOG(ERROR) << "Can not initialize " << MM_DISP_LIB << " (" << status << ")";
    }

    dm = ndk::SharedRefBase::make<DisplayModes>(libHandle, status == 0);
    pa = ndk::SharedRefBase::make<PictureAdjustment>(libHandle, status == 0);

    PictureAdjustment::setInstance(pa);

    ABinderProcess_setThreadPoolMaxThreadCount(0);

    {
        const std::string instance = std::string() + DisplayModes::descriptor + "/default";
        binder_status_t binder_status =
                AServiceManager_addService(dm->asBinder().get(), instance.c_str());
        if (binder_status != STATUS_OK) {
            LOG(ERROR) << "Could not register DisplayModes (" << binder_status << ")";
            goto shutdown;
        }
        LOG(INFO) << "Registered DisplayModes (supported=" << dm->isSupported() << ")";
    }

    {
        const std::string instance = std::string() + PictureAdjustment::descriptor + "/default";
        binder_status_t binder_status =
                AServiceManager_addService(pa->asBinder().get(), instance.c_str());
        if (binder_status != STATUS_OK) {
            LOG(ERROR) << "Could not register PictureAdjustment (" << binder_status << ")";
            goto shutdown;
        }
        LOG(INFO) << "Registered PictureAdjustment (supported=" << pa->isSupported() << ")";
    }

    LOG(INFO) << "LiveDisplay HAL service is ready.";
    ABinderProcess_joinThreadPool();

shutdown:
    if (disp_api_init != nullptr) {
        disp_api_init(1);
    }
    if (libHandle != nullptr) {
        dlclose(libHandle);
    }

    LOG(ERROR) << "LiveDisplay HAL service is shutting down.";
    return EXIT_FAILURE;
}
