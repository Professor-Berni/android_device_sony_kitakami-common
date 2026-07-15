/*
 * SPDX-FileCopyrightText: 2019 The LineageOS Project
 * SPDX-FileCopyrightText: 2026 The LineageOS Project (AIDL port)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "PictureAdjustment.h"

#include <cstring>
#include <dlfcn.h>
#include <memory>

#include "Constants.h"
#include "Types.h"

namespace aidl {
namespace vendor {
namespace lineage {
namespace livedisplay {
namespace legacymm {

static std::weak_ptr<PictureAdjustment> sInstance;

PictureAdjustment::PictureAdjustment(void* libHandle, bool initialized) {
    mLibHandle = libHandle;
    mInitialized = initialized;
    disp_api_supported =
            reinterpret_cast<int (*)(int32_t, int32_t)>(dlsym(mLibHandle, "disp_api_supported"));
    disp_api_get_pa_range =
            reinterpret_cast<int (*)(int32_t, void*)>(dlsym(mLibHandle, "disp_api_get_pa_range"));
    disp_api_get_pa_config =
            reinterpret_cast<int (*)(int32_t, void*)>(dlsym(mLibHandle, "disp_api_get_pa_config"));
    disp_api_set_pa_config =
            reinterpret_cast<int (*)(int32_t, void*)>(dlsym(mLibHandle, "disp_api_set_pa_config"));
    std::memset(&mDefaultPictureAdjustment, 0, sizeof(HSIC));
}

bool PictureAdjustment::isSupported() {
    mm_pa_range r{};

    if (!mInitialized) {
        return false;
    }

    if (disp_api_supported == nullptr || disp_api_supported(0, PICTURE_ADJUSTMENT_FEATURE) == 0) {
        return false;
    }

    if (disp_api_get_pa_range == nullptr || disp_api_get_pa_range(0, &r) != 0) {
        return false;
    }

    return r.max.hue != 0 && r.min.hue != 0 && r.max.saturation != 0 && r.min.saturation != 0 &&
           r.max.intensity != 0 && r.min.intensity != 0 && r.max.contrast != 0 &&
           r.min.contrast != 0;
}

HSIC PictureAdjustment::getPictureAdjustmentInternal() {
    mm_pa_config config{};

    if (disp_api_get_pa_config != nullptr) {
        if (disp_api_get_pa_config(0, &config) == 0) {
            HSIC hsic;
            hsic.hue = static_cast<float>(config.data.hue);
            hsic.saturation = static_cast<float>(config.data.saturation);
            hsic.intensity = static_cast<float>(config.data.intensity);
            hsic.contrast = static_cast<float>(config.data.contrast);
            hsic.saturationThreshold = static_cast<float>(config.data.saturationThreshold);
            return hsic;
        }
    }

    return HSIC{};
}

void PictureAdjustment::setInstance(const std::shared_ptr<PictureAdjustment>& instance) {
    sInstance = instance;
}

void PictureAdjustment::updateDefaultPictureAdjustment() {
    if (auto self = sInstance.lock()) {
        self->mDefaultPictureAdjustment = self->getPictureAdjustmentInternal();
    }
}

ndk::ScopedAStatus PictureAdjustment::getHueRange(FloatRange* _aidl_return) {
    FloatRange range{};
    mm_pa_range r{};

    if (disp_api_get_pa_range != nullptr) {
        if (disp_api_get_pa_range(0, &r) == 0) {
            range.max = r.max.hue;
            range.min = r.min.hue;
            range.step = 1;
        }
    }

    *_aidl_return = range;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PictureAdjustment::getSaturationRange(FloatRange* _aidl_return) {
    FloatRange range{};
    mm_pa_range r{};

    if (disp_api_get_pa_range != nullptr) {
        if (disp_api_get_pa_range(0, &r) == 0) {
            range.max = r.max.saturation;
            range.min = r.min.saturation;
            range.step = 1;
        }
    }

    *_aidl_return = range;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PictureAdjustment::getIntensityRange(FloatRange* _aidl_return) {
    FloatRange range{};
    mm_pa_range r{};

    if (disp_api_get_pa_range != nullptr) {
        if (disp_api_get_pa_range(0, &r) == 0) {
            range.max = r.max.intensity;
            range.min = r.min.intensity;
            range.step = 1;
        }
    }

    *_aidl_return = range;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PictureAdjustment::getContrastRange(FloatRange* _aidl_return) {
    FloatRange range{};
    mm_pa_range r{};

    if (disp_api_get_pa_range != nullptr) {
        if (disp_api_get_pa_range(0, &r) == 0) {
            range.max = r.max.contrast;
            range.min = r.min.contrast;
            range.step = 1;
        }
    }

    *_aidl_return = range;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PictureAdjustment::getSaturationThresholdRange(FloatRange* _aidl_return) {
    FloatRange range{};
    mm_pa_range r{};

    if (disp_api_get_pa_range != nullptr) {
        if (disp_api_get_pa_range(0, &r) == 0) {
            range.max = r.max.saturationThreshold;
            range.min = r.min.saturationThreshold;
            range.step = 1;
        }
    }

    *_aidl_return = range;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PictureAdjustment::getPictureAdjustment(HSIC* _aidl_return) {
    *_aidl_return = getPictureAdjustmentInternal();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PictureAdjustment::getDefaultPictureAdjustment(HSIC* _aidl_return) {
    *_aidl_return = mDefaultPictureAdjustment;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus PictureAdjustment::setPictureAdjustment(const HSIC& hsic) {
    mm_pa_config config = {0xF,
                           {static_cast<int>(hsic.hue), static_cast<int>(hsic.saturation),
                            static_cast<int>(hsic.intensity), static_cast<int>(hsic.contrast),
                            static_cast<int>(hsic.saturationThreshold)}};

    if (disp_api_set_pa_config != nullptr) {
        if (disp_api_set_pa_config(0, &config) == 0) {
            return ndk::ScopedAStatus::ok();
        }
    }

    return ndk::ScopedAStatus::fromServiceSpecificError(EIO);
}

}  // namespace legacymm
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
}  // namespace aidl
