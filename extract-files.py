#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2025 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.file import File
from extract_utils.fixups_blob import (
    BlobFixupCtx,
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)
from extract_utils.tools import (
    llvm_objdump_path,
)
from extract_utils.utils import (
    run_cmd,
)

namespace_imports = [
    'device/sony/kitakami-common',
    'hardware/qcom-caf/msm8994',
    'hardware/qcom-caf/wlan',
    'vendor/qcom/opensource/dataservices',
    'vendor/sony/suzuran',
]

blob_fixups: blob_fixups_user_type = {
    'vendor/bin/pm-service': blob_fixup()
        .add_needed('libutils-v33.so'),
    'vendor/bin/secd': blob_fixup()
        .add_needed('lib-preload64.so'),
    (
        'vendor/lib64/libsys-utils.so',
        'vendor/lib/libsys-utils.so',
    ): blob_fixup()
        .add_needed('libsensor_vendor.so')
        .add_needed('libshim_sensors.so'),
    (
        'vendor/lib64/libmm-abl.so',
        'vendor/lib/libmm-abl.so',
    ): blob_fixup()
        .add_needed('libshims_postproc.so')
        .remove_needed('libpowermanager.so'),
    (
        'vendor/lib64/libcneapiclient.so',
        'vendor/lib/libcneapiclient.so',
    ): blob_fixup()
        .remove_needed('libprotobuf-cpp-N.so'),
    (
        'vendor/lib64/libjni_latinimegoogle.so',
        'vendor/lib/libchromaflash.so',
        'vendor/lib/liboptizoom.so',
        'vendor/lib/libtrueportrait.so',
        'vendor/lib/libubifocus.so',
    ): blob_fixup()
        .replace_needed('libstdc++.so', 'libstdc++_vendor.so'),
    'vendor/lib64/libsettings.so': blob_fixup()
        .replace_needed('libprotobuf-cpp-full.so', 'libprotobuf-cpp-full-v28.so'),
    (
        'vendor/lib64/libqti_performance.so',
        'vendor/lib/libqti_performance.so',
    ): blob_fixup()
        .remove_needed('libnativehelper.so')
        .remove_needed('libhwui.so'),
    (
        'vendor/lib64/libmm-als.so',
        'vendor/lib/libmm-als.so',
    ): blob_fixup()
        .remove_needed('libandroid.so'),
    (
        'vendor/lib64/libmm-qdcm.so',
        'vendor/lib/libmm-qdcm.so',
    ): blob_fixup()
        .remove_needed('libpowermanager.so'),
    (
        'vendor/lib64/libril-qc-qmi-1.so',
        'vendor/lib/libril-qc-qmi-1.so',
    ): blob_fixup()
        .remove_needed('libmedia.so'),
    'vendor/lib64/libwqe.so': blob_fixup()
        .remove_needed('libandroid.so')
        .remove_needed('libwpa_client.so'),
    'vendor/lib/libwqe.so': blob_fixup()
        .remove_needed('libandroid.so')
        .remove_needed('libprotobuf-cpp-N.so')
        .remove_needed('libwpa_client.so'),
    (
        'vendor/lib64/libcne.so',
        'vendor/lib/libcne.so',
    ): blob_fixup()
        .remove_needed('libwpa_client.so'),
}  # fmt: skip

module = ExtractUtilsModule(
    'kitakami-common',
    'sony',
    blob_fixups=blob_fixups,
    namespace_imports=namespace_imports,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
