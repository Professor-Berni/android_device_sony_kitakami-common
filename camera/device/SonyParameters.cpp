/*
 * Copyright (C) 2017-2019 The LineageOS Project
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

#include "SonyParameters.h"

#include <string.h>
#include <map>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V1_0 {
namespace implementation {

namespace {

const char KEY_PREVIEW_FPS_RANGE[] = "preview-fps-range";
const char KEY_SCENE_MODE[] = "scene-mode";
const char KEY_SUPPORTED_SCENE_MODES[] = "scene-mode-values";
const char KEY_RECORDING_HINT[] = "recording-hint";
const char KEY_ISO_MODE[] = "iso";
const char KEY_SUPPORTED_ISO_MODES[] = "iso-values";

const char KEY_SONY_IMAGE_STABILISER_VALUES[] = "sony-is-values";
const char KEY_SONY_IMAGE_STABILISER[] = "sony-is";
const char KEY_SONY_VIDEO_STABILISER[] = "sony-vs";
const char KEY_SONY_VIDEO_STABILISER_VALUES[] = "sony-vs-values";
const char KEY_SONY_VIDEO_HDR[] = "sony-video-hdr";
const char KEY_SONY_VIDEO_HDR_VALUES[] = "sony-video-hdr-values";
const char KEY_SONY_ISO_AVAIL_MODES[] = "sony-iso-values";
const char KEY_SONY_ISO_MODE[] = "sony-iso";
const char KEY_SONY_AE_MODE_VALUES[] = "sony-ae-mode-values";
const char KEY_SONY_AE_MODE[] = "sony-ae-mode";

const char VALUE_SONY_ON[] = "on";
const char VALUE_SONY_OFF[] = "off";
const char VALUE_SONY_STILL_HDR[] = "on-still-hdr";
const char VALUE_SONY_INTELLIGENT_ACTIVE[] = "on-intelligent-active";

class Parameters {
  public:
    explicit Parameters(const std::string& flattened) {
        size_t from = 0;
        for (;;) {
            size_t eq = flattened.find('=', from);
            if (eq == std::string::npos) {
                break;
            }
            size_t end = flattened.find(';', eq + 1);
            mMap[flattened.substr(from, eq - from)] = flattened.substr(eq + 1,
                    end == std::string::npos ? std::string::npos : end - eq - 1);
            if (end == std::string::npos) {
                break;
            }
            from = end + 1;
        }
    }

    const char* get(const char* key) const {
        auto it = mMap.find(key);
        return it == mMap.end() ? nullptr : it->second.c_str();
    }

    std::string value(const char* key) const {
        const char* v = get(key);
        return v == nullptr ? "" : v;
    }

    void set(const char* key, const char* value) {
        if (value == nullptr || strpbrk(value, "=;") != nullptr) {
            return;
        }
        mMap[key] = value;
    }

    void set(const char* key, const std::string& value) { set(key, value.c_str()); }

    std::string flatten() const {
        std::string flattened;
        for (const auto& [key, value] : mMap) {
            if (!flattened.empty()) {
                flattened += ';';
            }
            flattened += key + '=' + value;
        }
        return flattened;
    }

  private:
    std::map<std::string, std::string> mMap;
};

bool contains(const char* list, const char* item) {
    return list != nullptr && strstr(list, item) != nullptr;
}

}  // namespace

std::string fixupGetParameters(const std::string& settings) {
    Parameters params(settings);

    if (contains(params.get(KEY_SONY_IMAGE_STABILISER_VALUES), VALUE_SONY_STILL_HDR) &&
            !contains(params.get(KEY_SUPPORTED_SCENE_MODES), "hdr")) {
        params.set(KEY_SUPPORTED_SCENE_MODES, params.value(KEY_SUPPORTED_SCENE_MODES) + ",hdr");
    }

    const char* isoModes = params.get(KEY_SONY_ISO_AVAIL_MODES);
    if (isoModes != nullptr) {
        std::string list = "ISO";
        for (const char* c = isoModes; *c != '\0'; c++) {
            list += *c == ',' ? std::string(",ISO") : std::string(1, *c);
        }
        params.set(KEY_SUPPORTED_ISO_MODES, list + ",auto");
    }

    if (params.value(KEY_SONY_IMAGE_STABILISER) == VALUE_SONY_STILL_HDR) {
        params.set(KEY_SCENE_MODE, "hdr");
    }

    if (params.get(KEY_SONY_VIDEO_HDR) != nullptr &&
            params.get(KEY_SONY_VIDEO_HDR_VALUES) != nullptr) {
        params.set("video-hdr-values", params.get(KEY_SONY_VIDEO_HDR_VALUES));
        params.set("video-hdr", params.get(KEY_SONY_VIDEO_HDR));
    }

    const char* sonyIso = params.get(KEY_SONY_ISO_MODE);
    if (sonyIso != nullptr && params.get(KEY_SONY_AE_MODE_VALUES) != nullptr) {
        std::string aeMode = params.value(KEY_SONY_AE_MODE);
        std::string iso = std::string("ISO") + sonyIso;
        const char* shutterSpeed = params.get("sony-shutter-speed");
        if (aeMode == "iso-prio") {
            params.set(KEY_ISO_MODE, iso);
            params.set("shutter-speed", "auto");
        } else if (aeMode == "shutter-prio") {
            params.set(KEY_ISO_MODE, "auto");
            params.set("shutter-speed", shutterSpeed);
        } else if (aeMode == "manual") {
            params.set("shutter-speed", shutterSpeed);
            params.set(KEY_ISO_MODE, iso);
        } else {
            params.set(KEY_ISO_MODE, "auto");
            params.set("shutter-speed", "auto");
        }
    }

    return params.flatten();
}

std::string fixupSetParameters(const std::string& settings) {
    Parameters params(settings);

    params.set(KEY_PREVIEW_FPS_RANGE, "1000,60000");

    const char* shutterSpeed = params.get("shutter-speed");
    if (shutterSpeed != nullptr) {
        if (strcmp(shutterSpeed, "auto") != 0) {
            params.set("sony-shutter-speed", shutterSpeed);
            params.set(KEY_SONY_AE_MODE, "shutter-prio");
        } else if (contains(params.get(KEY_SONY_AE_MODE_VALUES), "auto")) {
            params.set(KEY_SONY_AE_MODE, "auto");
        }
    }

    const char* isoMode = params.get(KEY_ISO_MODE);
    if (isoMode != nullptr) {
        bool autoIso = strcmp(isoMode, "auto") == 0;
        if (!autoIso && strlen(isoMode) >= 3) {
            params.set(KEY_SONY_ISO_MODE, isoMode + 3);
        }
        const char* aeModes = params.get(KEY_SONY_AE_MODE_VALUES);
        if (aeModes != nullptr) {
            std::string aeMode = params.value(KEY_SONY_AE_MODE);
            if (autoIso) {
                if (contains(aeModes, "auto") && aeMode != "shutter-prio") {
                    params.set(KEY_SONY_AE_MODE, "auto");
                }
            } else if (contains(aeModes, "iso-prio")) {
                params.set(KEY_SONY_AE_MODE, aeMode == "shutter-prio" ? "manual" : "iso-prio");
            }
        }
    }

    const char* sceneMode = params.get(KEY_SCENE_MODE);
    if (sceneMode != nullptr) {
        if (strcmp(sceneMode, "hdr") == 0) {
            params.set(KEY_SONY_IMAGE_STABILISER, VALUE_SONY_STILL_HDR);
            params.set(KEY_SCENE_MODE, "auto");
        } else {
            params.set(KEY_SONY_IMAGE_STABILISER, VALUE_SONY_ON);
        }
    }

    if (params.get(KEY_SONY_VIDEO_HDR) != nullptr && params.get("video-hdr") != nullptr) {
        params.set(KEY_SONY_VIDEO_HDR, params.get("video-hdr"));
    }

    if (params.value(KEY_RECORDING_HINT) == "true") {
        params.set(KEY_SONY_VIDEO_STABILISER,
                contains(params.get(KEY_SONY_VIDEO_STABILISER_VALUES),
                         VALUE_SONY_INTELLIGENT_ACTIVE) ?
                VALUE_SONY_INTELLIGENT_ACTIVE : VALUE_SONY_ON);
        params.set(KEY_SONY_IMAGE_STABILISER, VALUE_SONY_OFF);
    }

    return params.flatten();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
