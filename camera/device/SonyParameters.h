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

#ifndef ANDROID_HARDWARE_CAMERA_DEVICE_V1_0_SONYPARAMETERS_H
#define ANDROID_HARDWARE_CAMERA_DEVICE_V1_0_SONYPARAMETERS_H

#include <string>

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V1_0 {
namespace implementation {

std::string fixupGetParameters(const std::string& settings);
std::string fixupSetParameters(const std::string& settings);

}  // namespace implementation
}  // namespace V1_0
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CAMERA_DEVICE_V1_0_SONYPARAMETERS_H
