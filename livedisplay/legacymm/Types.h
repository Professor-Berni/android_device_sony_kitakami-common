/*
 * SPDX-FileCopyrightText: 2019 The LineageOS Project
 * SPDX-FileCopyrightText: 2026 The LineageOS Project (AIDL port)
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace aidl {
namespace vendor {
namespace lineage {
namespace livedisplay {
namespace legacymm {

struct mm_disp_mode {
    int id;
    char* name;
    uint32_t len;
    int32_t type;
};

struct mm_pa_data {
    int hue;
    int saturation;
    int intensity;
    int contrast;
    int saturationThreshold;
};

struct mm_pa_config {
    int flags;
    struct mm_pa_data data;
};

struct mm_pa_range {
    struct mm_pa_data max;
    struct mm_pa_data min;
};

}  // namespace legacymm
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
}  // namespace aidl
