/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include <system/window.h>
#include <ui/Rect.h>
#include <gui/SurfaceControl.h>

int32_t outBytesPerPixel = 0;
int32_t outBytesPerStride = 0;

namespace android {

// ---------------------------------------------------------------------------

//android::GraphicBufferMapper::lock(native_handle const*, unsigned int, android::Rect const&, void**, int*, int*)

extern "C" {

    void _ZN7android19GraphicBufferMapper4lockEPK13native_handlejRKNS_4RectEPPvPiS9_(native_handle const*, unsigned int, android::Rect const&, void**, int*, int*);

    void _ZN7android19GraphicBufferMapper4lockEPK13native_handlejRKNS_4RectEPPv(buffer_handle_t handle, uint32_t usage, const android::Rect& bounds, void** vaddr) {
        _ZN7android19GraphicBufferMapper4lockEPK13native_handlejRKNS_4RectEPPvPiS9_(handle, usage, bounds, vaddr, &outBytesPerPixel, &outBytesPerStride);
    }
}

// ---------------------------------------------------------------------------

}; // namespace android
