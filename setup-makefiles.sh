#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017-2023 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set -e

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

# Initialize the helper for common
setup_vendor "${DEVICE_COMMON}" "${VENDOR}" "${ANDROID_ROOT}" true

# Copyright headers and guards
write_headers "ivy karin karin_windy satsuki sumire suzuran"

# The standard common blobs
write_makefiles "${MY_DIR}/proprietary-files.txt" true

printf '\n' >> "${PRODUCTMK}"
printf 'ifneq ($(BOARD_HAVE_RADIO),false)\n' >> "${PRODUCTMK}"

write_makefiles "${MY_DIR}/proprietary-files-radio.txt" true

echo "ifeq (\$(strip \$(BOARD_NFC_CHIPSET)),pn547)" >> "${ANDROIDMK}"
write_makefiles "${MY_DIR}/proprietary-files-pn547.txt" true
echo "endif" >> "${ANDROIDMK}"

printf 'endif\n' >> "${PRODUCTMK}"

# We are done!
write_footers

if [ -s "${MY_DIR}/../${DEVICE}/proprietary-files.txt" ]; then
    # Reinitialize the helper for device
    setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false

    # Copyright headers and guards
    write_headers

    # The standard device blobs
    write_makefiles "${MY_DIR}/../${DEVICE}/proprietary-files.txt" true

    # We are done!
    write_footers
fi
