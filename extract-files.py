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
    (
        'vendor/bin/pm-service',
    ): blob_fixup()
        .add_needed('liblog.so')
        .add_needed('libutils-v33.so'),
    (
        'vendor/bin/secd',
    ): blob_fixup()
        .add_needed('libcutils_vendor.so')
        .add_needed('liblog.so')
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
    ): blob_fixup()
        .add_needed('liblog.so')
        .replace_needed('libprotobuf-cpp-lite.so', 'libprotobuf-cpp-lite-v28.so'),
    (
        'vendor/lib/libcneapiclient.so',
    ): blob_fixup()
        .add_needed('liblog.so')
        .replace_needed('libprotobuf-cpp-N.so', 'libprotobuf-cpp-lite-v28.so'),
    (
        'vendor/lib64/libjni_latinimegoogle.so',
        'vendor/lib64/libssl_vendor.so',
        'vendor/lib/libchromaflash.so',
        'vendor/lib/liboptizoom.so',
        'vendor/lib/libssl_vendor.so',
        'vendor/lib/libtrueportrait.so',
        'vendor/lib/libubifocus.so',
    ): blob_fixup()
        .replace_needed('libstdc++.so', 'libstdc++_vendor.so'),
    (
        'vendor/lib64/libsettings.so',
        'vendor/lib/libsettings.so',
    ): blob_fixup()
        .replace_needed('libprotobuf-cpp-full.so', 'libprotobuf-cpp-full-v28.so'),
    (
        'vendor/lib64/libqti_performance.so',
        'vendor/lib/libqti_performance.so',
    ): blob_fixup()
        .replace_needed('libnativehelper.so', 'libnativehelper_vendor.so')
        .remove_needed('libhwui.so'),
    (
        'vendor/lib64/libmm-qdcm.so',
        'vendor/lib/libmm-qdcm.so',
    ): blob_fixup()
        .remove_needed('libpowermanager.so'),
    (
        'vendor/lib64/libril-qc-qmi-1.so',
        'vendor/lib/libril-qc-qmi-1.so',
    ): blob_fixup()
        .replace_needed('libmedia.so', 'libshims_AudioSystem.so'),
    (
        'vendor/lib64/libwqe.so',
    ): blob_fixup()
        .remove_needed('libwpa_client.so')
        .replace_needed('libandroid.so', 'libandroid_vendor.so'),
    (
        'vendor/lib/libwqe.so',
    ): blob_fixup()
        .remove_needed('libprotobuf-cpp-N.so')
        .remove_needed('libwpa_client.so')
        .replace_needed('libandroid.so', 'libandroid_vendor.so'),
    (
        'vendor/lib64/libcne.so',
        'vendor/lib/libcne.so',
    ): blob_fixup()
        .remove_needed('libwpa_client.so')
        .replace_needed('libprotobuf-cpp-lite.so', 'libprotobuf-cpp-lite-v28.so'),
    (
        'vendor/bin/loc_launcher',
        'vendor/bin/mlog_qmi_service',
        'vendor/bin/mm-pp-daemon',
        'vendor/bin/msm_irqbalance',
        'vendor/bin/perfd',
        'vendor/bin/pm-proxy',
        'vendor/bin/rmt_storage',
        'vendor/bin/sct_service',
        'vendor/bin/ta_qmi_service',
        'vendor/bin/updatemiscta',
        'vendor/lib64/egl/libQTapGLES.so',
        'vendor/lib64/libdiag.so',
        'vendor/lib64/libdsi_netctrl.so',
        'vendor/lib64/liblbs_core.so',
        'vendor/lib64/libloc_api_v02.so',
        'vendor/lib64/liblocationservice.so',
        'vendor/lib64/libloc_core.so',
        'vendor/lib64/libloc_ds_api.so',
        'vendor/lib64/liblowi_client.so',
        'vendor/lib64/libmdmdetect.so',
        'vendor/lib64/libperipheral_client.so',
        'vendor/lib64/libquipc_os_api.so',
        'vendor/lib64/libsubsystem_control.so',
        'vendor/lib64/libta.so',
        'vendor/lib64/libthermalioctl.so',
        'vendor/lib64/libtime_genoff.so',
        'vendor/lib/egl/libQTapGLES.so',
        'vendor/lib/libdiag.so',
        'vendor/lib/libdsi_netctrl.so',
        'vendor/lib/liblbs_core.so',
        'vendor/lib/libloc_api_v02.so',
        'vendor/lib/liblocationservice.so',
        'vendor/lib/libloc_core.so',
        'vendor/lib/libloc_ds_api.so',
        'vendor/lib/liblowi_client.so',
        'vendor/lib/libmdmdetect.so',
        'vendor/lib/libmmosal.so',
        'vendor/lib/libperipheral_client.so',
        'vendor/lib/libquipc_os_api.so',
        'vendor/lib/libsubsystem_control.so',
        'vendor/lib/libta.so',
        'vendor/lib/libtime_genoff.so',
        'vendor/lib/soundfx/libqcbassboost.so',
        'vendor/lib/soundfx/libqcreverb.so',
        'vendor/lib/soundfx/libqcvirt.so',
    ): blob_fixup()
        .add_needed('liblog.so'),
    (
        'vendor/lib64/libgps.utils.so',
        'vendor/lib/libgps.utils.so',
    ): blob_fixup()
        .add_needed('libprocessgroup.so'),
    (
        'vendor/lib64/libizat_core.so',
        'vendor/lib/libizat_core.so',
    ): blob_fixup()
        .add_needed('liblog.so')
        .add_needed('libshims_get_process_name.so'),
    (
        'vendor/lib64/libtpm.so',
        'vendor/lib/libtpm.so',
    ): blob_fixup()
        .add_needed('libbinder_shim.so'),
    (
        'vendor/lib64/libwvhidl.so',
    ): blob_fixup()
        .add_needed('libssl_vendor.so'),
    (
        'vendor/bin/iddd',
    ): blob_fixup()
        .add_needed('liblog.so')
        .add_needed('libbinder_shim.so'),
    (
        'vendor/lib64/libmm-als.so',
        'vendor/lib/libmm-als.so',
    ): blob_fixup()
        .add_needed('liblog.so')
        .replace_needed('libandroid.so', 'libandroid_vendor.so'),
    (
        'vendor/lib/hw/sound_trigger.primary.msm8994.so',
        'vendor/lib/libqti-perfd-client.so',
    ): blob_fixup()
        .replace_needed('libc.so', 'libc_vendor.so'),
    (
        'vendor/lib64/libthermalclient.so',
        'vendor/lib/libthermalclient.so',
    ): blob_fixup()
        .add_needed('liblog.so')
        .replace_needed('libc.so', 'libc_vendor.so'),
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
