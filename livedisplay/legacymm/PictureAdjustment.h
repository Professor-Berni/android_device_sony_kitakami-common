/*
 * SPDX-FileCopyrightText: 2019 The LineageOS Project
 * SPDX-FileCopyrightText: 2026 The LineageOS Project (AIDL port)
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/vendor/lineage/livedisplay/BnPictureAdjustment.h>

namespace aidl {
namespace vendor {
namespace lineage {
namespace livedisplay {
namespace legacymm {

class PictureAdjustment : public BnPictureAdjustment {
  public:
    PictureAdjustment(void* libHandle, bool initialized);

    bool isSupported();

    // Methods from BnPictureAdjustment follow.
    ndk::ScopedAStatus getHueRange(FloatRange* _aidl_return) override;
    ndk::ScopedAStatus getSaturationRange(FloatRange* _aidl_return) override;
    ndk::ScopedAStatus getIntensityRange(FloatRange* _aidl_return) override;
    ndk::ScopedAStatus getContrastRange(FloatRange* _aidl_return) override;
    ndk::ScopedAStatus getSaturationThresholdRange(FloatRange* _aidl_return) override;
    ndk::ScopedAStatus getPictureAdjustment(HSIC* _aidl_return) override;
    ndk::ScopedAStatus getDefaultPictureAdjustment(HSIC* _aidl_return) override;
    ndk::ScopedAStatus setPictureAdjustment(const HSIC& hsic) override;

    static void setInstance(const std::shared_ptr<PictureAdjustment>& instance);
    static void updateDefaultPictureAdjustment();

  private:
    void* mLibHandle;
    bool mInitialized;

    int (*disp_api_supported)(int32_t, int32_t);
    int (*disp_api_get_pa_range)(int32_t, void*);
    int (*disp_api_get_pa_config)(int32_t, void*);
    int (*disp_api_set_pa_config)(int32_t, void*);

    HSIC getPictureAdjustmentInternal();

    HSIC mDefaultPictureAdjustment;
};

}  // namespace legacymm
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
}  // namespace aidl
