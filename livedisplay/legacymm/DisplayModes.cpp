/*
 * SPDX-FileCopyrightText: 2019 The LineageOS Project
 * SPDX-FileCopyrightText: 2026 The LineageOS Project (AIDL port)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DisplayModes.h"

#include <dlfcn.h>

#include "Constants.h"
#include "PictureAdjustment.h"
#include "Types.h"

namespace aidl {
namespace vendor {
namespace lineage {
namespace livedisplay {
namespace legacymm {

DisplayModes::DisplayModes(void* libHandle, bool initialized) {
    mLibHandle = libHandle;
    mInitialized = initialized;
    disp_api_supported =
            reinterpret_cast<int (*)(int32_t, int32_t)>(dlsym(mLibHandle, "disp_api_supported"));
    disp_api_get_num_display_modes = reinterpret_cast<int (*)(int32_t, int32_t, int*)>(
            dlsym(mLibHandle, "disp_api_get_num_display_modes"));
    disp_api_get_display_modes = reinterpret_cast<int (*)(int32_t, int32_t, void*, int)>(
            dlsym(mLibHandle, "disp_api_get_display_modes"));
    disp_api_get_active_display_mode = reinterpret_cast<int (*)(int32_t, int*, uint32_t*)>(
            dlsym(mLibHandle, "disp_api_get_active_display_mode"));
    disp_api_set_active_display_mode = reinterpret_cast<int (*)(int32_t, int)>(
            dlsym(mLibHandle, "disp_api_set_active_display_mode"));
    disp_api_get_default_display_mode = reinterpret_cast<int (*)(int32_t, int*)>(
            dlsym(mLibHandle, "disp_api_get_default_display_mode"));
    disp_api_set_default_display_mode = reinterpret_cast<int (*)(int32_t, int)>(
            dlsym(mLibHandle, "disp_api_set_default_display_mode"));
}

bool DisplayModes::isSupported() {
    int count = 0;

    if (!mInitialized) {
        return false;
    }

    if (disp_api_supported == nullptr || disp_api_supported(0, DISPLAY_MODES_FEATURE) == 0) {
        return false;
    }

    if (disp_api_get_num_display_modes != nullptr) {
        if (disp_api_get_num_display_modes(0, 0, &count) == 0) {
            return count > 0;
        }
    }

    return false;
}

std::vector<DisplayMode> DisplayModes::getDisplayModesInternal() {
    std::vector<DisplayMode> modes;
    int count = 0;

    if (disp_api_get_num_display_modes == nullptr ||
        disp_api_get_num_display_modes(0, 0, &count) != 0) {
        return modes;
    }

    if (disp_api_get_display_modes != nullptr) {
        mm_disp_mode* tmp = new mm_disp_mode[count];
        for (int i = 0; i < count; i++) {
            tmp[i].id = -1;
            tmp[i].name = new char[128];
            tmp[i].len = 128;
        }

        if (disp_api_get_display_modes(0, 0, tmp, count) == 0) {
            for (int i = 0; i < count; i++) {
                DisplayMode mode;
                mode.id = tmp[i].id;
                mode.name = std::string(tmp[i].name);
                modes.push_back(mode);
            }
        }

        for (int i = 0; i < count; i++) {
            delete[] tmp[i].name;
        }

        delete[] tmp;
    }

    return modes;
}

DisplayMode DisplayModes::getDisplayModeById(int32_t id) {
    std::vector<DisplayMode> modes = getDisplayModesInternal();

    for (const DisplayMode& mode : modes) {
        if (mode.id == id) {
            return mode;
        }
    }

    DisplayMode invalid;
    invalid.id = -1;
    invalid.name = "";
    return invalid;
}

DisplayMode DisplayModes::getCurrentDisplayModeInternal() {
    int id = 0;
    uint32_t mask = 0;

    if (disp_api_get_active_display_mode != nullptr) {
        if (disp_api_get_active_display_mode(0, &id, &mask) == 0 && id >= 0) {
            return getDisplayModeById(id);
        }
    }

    DisplayMode invalid;
    invalid.id = -1;
    invalid.name = "";
    return invalid;
}

DisplayMode DisplayModes::getDefaultDisplayModeInternal() {
    int id = 0;

    if (disp_api_get_default_display_mode != nullptr) {
        if (disp_api_get_default_display_mode(0, &id) == 0 && id >= 0) {
            return getDisplayModeById(id);
        }
    }

    DisplayMode invalid;
    invalid.id = -1;
    invalid.name = "";
    return invalid;
}

ndk::ScopedAStatus DisplayModes::getDisplayModes(std::vector<DisplayMode>* _aidl_return) {
    *_aidl_return = getDisplayModesInternal();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus DisplayModes::getCurrentDisplayMode(DisplayMode* _aidl_return) {
    *_aidl_return = getCurrentDisplayModeInternal();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus DisplayModes::getDefaultDisplayMode(DisplayMode* _aidl_return) {
    *_aidl_return = getDefaultDisplayModeInternal();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus DisplayModes::setDisplayMode(int32_t modeID, bool makeDefault) {
    DisplayMode currentMode = getCurrentDisplayModeInternal();

    if (currentMode.id >= 0 && currentMode.id == modeID) {
        return ndk::ScopedAStatus::ok();
    }

    DisplayMode mode = getDisplayModeById(modeID);
    if (mode.id < 0) {
        return ndk::ScopedAStatus::fromServiceSpecificError(EINVAL);
    }

    if (disp_api_set_active_display_mode == nullptr ||
        disp_api_set_active_display_mode(0, modeID)) {
        return ndk::ScopedAStatus::fromServiceSpecificError(EIO);
    }

    if (makeDefault && (disp_api_set_default_display_mode == nullptr ||
                        disp_api_set_default_display_mode(0, modeID))) {
        return ndk::ScopedAStatus::fromServiceSpecificError(EIO);
    }

    PictureAdjustment::updateDefaultPictureAdjustment();

    return ndk::ScopedAStatus::ok();
}

}  // namespace legacymm
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
}  // namespace aidl
