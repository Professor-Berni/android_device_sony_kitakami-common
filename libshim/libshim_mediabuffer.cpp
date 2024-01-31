/*
 * Copyright (C) 2016 The CyanogenMod Project
 * Copyright (C) 2017-2021 The LineageOS Project
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

#include <ui/GraphicBuffer.h>
#include <media/stagefright/MediaBuffer.h>

extern "C" {

int _ZNK7android11MediaBuffer8refcountEv(android::MediaBuffer *thisptr) {
    return thisptr->refcount();
}

void _ZN7android13GraphicBuffer4lockEjPPv() {}
void _ZNK7android11MediaBuffer13graphicBufferEv() {}

void _ZN7android7SurfaceC1ERKNS_2spINS_22IGraphicBufferProducerEEEb() {}
}
