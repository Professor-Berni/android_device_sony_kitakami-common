/*
 * SPDX-FileCopyrightText: 2019 The LineageOS Project
 * SPDX-FileCopyrightText: 2026 The LineageOS Project (AIDL port)
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/vendor/lineage/livedisplay/BnDisplayModes.h>

namespace aidl {
namespace vendor {
namespace lineage {
namespace livedisplay {
namespace legacymm {

class DisplayModes : public BnDisplayModes {
  public:
    DisplayModes(void* libHandle, bool initialized);

    bool isSupported();

    // Methods from BnDisplayModes follow.
    ndk::ScopedAStatus getDisplayModes(std::vector<DisplayMode>* _aidl_return) override;
    ndk::ScopedAStatus getCurrentDisplayMode(DisplayMode* _aidl_return) override;
    ndk::ScopedAStatus getDefaultDisplayMode(DisplayMode* _aidl_return) override;
    ndk::ScopedAStatus setDisplayMode(int32_t modeID, bool makeDefault) override;

  private:
    void* mLibHandle;
    bool mInitialized;

    int (*disp_api_supported)(int32_t, int32_t);
    int (*disp_api_get_num_display_modes)(int32_t, int32_t, int*);
    int (*disp_api_get_display_modes)(int32_t, int32_t, void*, int);
    int (*disp_api_get_active_display_mode)(int32_t, int*, uint32_t*);
    int (*disp_api_set_active_display_mode)(int32_t, int);
    int (*disp_api_get_default_display_mode)(int32_t, int*);
    int (*disp_api_set_default_display_mode)(int32_t, int);

    std::vector<DisplayMode> getDisplayModesInternal();
    DisplayMode getDisplayModeById(int32_t id);
    DisplayMode getCurrentDisplayModeInternal();
    DisplayMode getDefaultDisplayModeInternal();
};

}  // namespace legacymm
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
}  // namespace aidl
