#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017-2025 The LineageOS Project
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

function blob_fixup() {
    case "${1}" in
    bin/netmgrd)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --remove-needed "librmnetctl.so" "${2}"
    ;;
    bin/pm-service)
        [ "$2" = "" ] && return 0
        grep -q libutils-v33.so "${2}" || "${PATCHELF}" --add-needed "libutils-v33.so" "${2}"
    ;;
    bin/secd)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --add-needed "lib-preload64.so" "${2}"
    ;;
    vendor/lib64/libwvhidl.so| \
    vendor/lib/mediadrm/libwvdrmengine.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --replace-needed "libprotobuf-cpp-lite.so" "libprotobuf-cpp-lite-v28.so" "${2}"
    ;;
    vendor/lib*/libsettings.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --replace-needed "libprotobuf-cpp-full.so" "libprotobuf-cpp-full-v28.so" "${2}"
    ;;
    vendor/lib/libchromaflash.so| \
    vendor/lib/liboptizoom.so| \
    vendor/lib/libseemore.so| \
    vendor/lib/libubifocus.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --replace-needed "libstdc++.so" "libc++.so" "${2}"
    ;;
    vendor/lib/libtrueportrait.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --add-needed "liblog.so" "${2}"
        "${PATCHELF}" --replace-needed "libstdc++.so" "libc++.so" "${2}"
    ;;
    vendor/lib/libmmcamera_hdr_gb_lib.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --remove-needed "libstdc++.so" "${2}"
    ;;
    vendor/lib/libwqe.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --remove-needed "libprotobuf-cpp-N.so" "${2}"
    ;;
    vendor/lib/libcneapiclient.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --add-needed "liblog.so" "${2}"
        "${PATCHELF}" --remove-needed "libprotobuf-cpp-N.so" "${2}"
    ;;
    lib/libcammw.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --add-needed "libsensor.so" "${2}"
        "${PATCHELF}" --add-needed "libsensor_vendor.so" "${2}"
#        "${PATCHELF}" --remove-needed "libmemalloc.so" "${2}"
    ;;
    lib*/libsys-utils.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --add-needed "libsensor.so" "${2}"
    ;;
    vendor/lib*/hw/flp.default.so| \
    vendor/lib*/libflp.so| \
    vendor/lib*/libgeofence.so| \
    vendor/lib*/libulp2.so| \
    vendor/lib*/libmm-qdcm.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --remove-needed "libqdutils.so" "${2}"
        "${PATCHELF}" --remove-needed "libqservice.so" "${2}"
    ;;
    vendor/lib*/liblbs_core.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --add-needed "liblog.so" "${2}"
        "${PATCHELF}" --remove-needed "libqdutils.so" "${2}"
        "${PATCHELF}" --remove-needed "libqservice.so" "${2}"
    ;;
    vendor/lib*/libmm-abl.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --remove-needed "libqdutils.so" "${2}"
        "${PATCHELF}" --remove-needed "libqservice.so" "${2}"
    ;;
    vendor/bin/mm-pp-daemon)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --remove-needed "libqdutils.so" "${2}"
    ;;
    lib64/libjni_latinimegoogle.so| \
    lib/libcacao_client.so| \
    lib/libcacao_service.so| \
    lib*/lib_fpc_tac_shared.so| \
    lib*/libidd.so| \
    lib*/libloc_api_v02.so| \
    lib*/libloc_core.so| \
    lib/libmm-qcamera.so| \
    lib/libqomx_core.so| \
    lib/libsony_fooddetect.so| \
    vendor/lib64/libthermalioctl.so| \
    vendor/lib*/libcne.so| \
    vendor/lib*/libdiag.so| \
    vendor/lib*/libdsi_netctrl.so| \
    vendor/lib/libjpegdhw.so| \
    vendor/lib/libjpegdmahw.so| \
    vendor/lib/libjpegehw.so| \
    vendor/lib*/libloc_api_v02.so| \
    vendor/lib*/liblocationservice.so| \
    vendor/lib*/libloc_ds_api.so| \
    vendor/lib*/liblowi_client.so| \
    vendor/lib*/libmdmdetect.so| \
    vendor/lib*/libmm-als.so| \
    vendor/lib/libmmcamera2_c2d_module.so| \
    vendor/lib/libmmcamera2_cpp_module.so| \
    vendor/lib/libmmcamera2_frame_algorithm.so| \
    vendor/lib/libmmcamera2_isp_modules.so| \
    vendor/lib/libmmcamera2_is.so| \
    vendor/lib/libmmcamera2_pp_buf_mgr.so| \
    vendor/lib*/libmmcamera2_q3a_core.so| \
    vendor/lib*/libmmcamera2_stats_algorithm.so| \
    vendor/lib/libmmcamera2_stats_modules.so| \
    vendor/lib/libmmcamera2_vpe_module.so| \
    vendor/lib/libmmcamera_cac2_lib.so| \
    vendor/lib/libmmcamera_eztune_module.so| \
    vendor/lib/libmmcamera_hdr_gb_lib.so| \
    vendor/lib/libmmcamera_isp_abf44.so| \
    vendor/lib/libmmcamera_isp_bcc44.so| \
    vendor/lib/libmmcamera_isp_be_stats44.so| \
    vendor/lib/libmmcamera_isp_bf_scale_stats46.so| \
    vendor/lib/libmmcamera_isp_bf_stats44.so| \
    vendor/lib/libmmcamera_isp_bg_stats46.so| \
    vendor/lib/libmmcamera_isp_bhist_stats44.so| \
    vendor/lib/libmmcamera_isp_bpc44.so| \
    vendor/lib/libmmcamera_isp_chroma_enhan40.so| \
    vendor/lib/libmmcamera_isp_chroma_suppress40.so| \
    vendor/lib/libmmcamera_isp_clamp_encoder40.so| \
    vendor/lib/libmmcamera_isp_clamp_video40.so| \
    vendor/lib/libmmcamera_isp_clamp_viewfinder40.so| \
    vendor/lib/libmmcamera_isp_clf46.so| \
    vendor/lib/libmmcamera_isp_color_correct46.so| \
    vendor/lib/libmmcamera_isp_color_xform_encoder46.so| \
    vendor/lib/libmmcamera_isp_color_xform_video46.so| \
    vendor/lib/libmmcamera_isp_color_xform_viewfinder46.so| \
    vendor/lib/libmmcamera_isp_cs_stats46.so| \
    vendor/lib/libmmcamera_isp_demosaic44.so| \
    vendor/lib/libmmcamera_isp_demux40.so| \
    vendor/lib/libmmcamera_isp_fovcrop_encoder46.so| \
    vendor/lib/libmmcamera_isp_fovcrop_video46.so| \
    vendor/lib/libmmcamera_isp_fovcrop_viewfinder46.so| \
    vendor/lib/libmmcamera_isp_gamma44.so| \
    vendor/lib/libmmcamera_isp_gic46.so| \
    vendor/lib/libmmcamera_isp_gtm46.so| \
    vendor/lib/libmmcamera_isp_hdr46.so| \
    vendor/lib/libmmcamera_isp_hdr_be_stats46.so| \
    vendor/lib/libmmcamera_isp_ihist_stats46.so| \
    vendor/lib/libmmcamera_isp_linearization40.so| \
    vendor/lib/libmmcamera_isp_ltm44.so| \
    vendor/lib/libmmcamera_isp_mce40.so| \
    vendor/lib/libmmcamera_isp_mesh_rolloff44.so| \
    vendor/lib/libmmcamera_isp_pedestal_correct46.so| \
    vendor/lib/libmmcamera_isp_rs_stats46.so| \
    vendor/lib/libmmcamera_isp_scaler_encoder46.so| \
    vendor/lib/libmmcamera_isp_scaler_video46.so| \
    vendor/lib/libmmcamera_isp_scaler_viewfinder46.so| \
    vendor/lib/libmmcamera_isp_sce40.so| \
    vendor/lib/libmmcamera_isp_sub_module.so| \
    vendor/lib/libmmcamera_isp_wb46.so| \
    vendor/lib/libmmcamera_optizoom_lib.so| \
    vendor/lib/libmmcamera_pdafcamif.so| \
    vendor/lib/libmmcamera_pdaf.so| \
    vendor/lib/libmmcamera_pdaf_v3.so| \
    vendor/lib/libmmcamera_ppbase_module.so| \
    vendor/lib/libmmcamera_tintless_bg_pca_algo.so| \
    vendor/lib/libmmcamera_vpu_module.so| \
    vendor/lib/libmmosal.so| \
    vendor/lib/libois_lc898122.so| \
    vendor/lib*/libperipheral_client.so| \
    vendor/lib*/libqmi_client_helper.so| \
    vendor/lib/libqomx_jpegdec.so| \
    vendor/lib/libqomx_jpegenc_pipe.so| \
    vendor/lib/libqomx_jpegenc.so| \
    vendor/lib*/libQSEEComAPI.so| \
    vendor/lib*/libquipc_os_api.so| \
    vendor/lib*/libril-qc-qmi-1.so| \
    vendor/lib*/libsubsystem_control.so| \
    vendor/lib*/libthermalclient.so| \
    vendor/lib*/libtime_genoff.so| \
    vendor/lib/soundfx/libqcbassboost.so| \
    vendor/lib/soundfx/libqcreverb.so| \
    vendor/lib/soundfx/libqcvirt.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --add-needed "liblog.so" "${2}"
    ;;
    vendor/lib*/libizat_core.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --add-needed "libshims_get_process_name.so" "${2}"
        "${PATCHELF}" --add-needed "liblog.so" "${2}"
    ;;
    lib*/libgps.utils.so)
        [ "$2" = "" ] && return 0
        "${PATCHELF}" --add-needed "libprocessgroup.so" "${2}"
    ;;
    *)
        return 1
        ;;
    esac
}

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

while [ "${#}" -gt 0 ]; do
    case "${1}" in
        -n | --no-cleanup )
                CLEAN_VENDOR=false
                ;;
        -s | --section )
                SECTION="${2}"; shift
                CLEAN_VENDOR=false
                ;;
        * )
                SRC="${1}"
                ;;
    esac
    shift
done

if [ -z "${SRC}" ]; then
    SRC="adb"
fi

# Initialize the helper for common device
setup_vendor "${DEVICE_COMMON}" "${VENDOR}" "${ANDROID_ROOT}" true "${CLEAN_VENDOR}"

extract "${MY_DIR}/proprietary-files.txt" "${SRC}" "${SECTION}"
extract "${MY_DIR}/proprietary-files-radio.txt" "${SRC}" "${SECTION}"
extract "${MY_DIR}/proprietary-files-pn547.txt" "${SRC}" "${SECTION}"

if [ -s "${MY_DIR}/../${DEVICE}/proprietary-files.txt" ]; then
    # Reinitialize the helper for device
    setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"

    extract "${MY_DIR}/../${DEVICE}/proprietary-files.txt" "${SRC}" "${SECTION}"
fi

"${MY_DIR}/setup-makefiles.sh"
