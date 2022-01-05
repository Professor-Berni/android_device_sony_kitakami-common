/*
 * Copyright (C) 2017-2019, The LineageOS Project
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

/**
* @file CameraWrapper.cpp
*
* This file wraps a vendor camera module.
*
*/

//#define LOG_NDEBUG 0
//#define LOG_PARAMETERS

#define LOG_TAG "CameraWrapper"
#include <camera/Camera.h>
#include <camera/CameraParameters.h>

#include "CameraWrapper.h"

using namespace android;

// Wrapper specific parameters names
static const char KEY_ISO_MODE[] = "iso";
static const char KEY_SUPPORTED_ISO_MODES[] = "iso-values";

// Wrapper Sony specific parameters names
static const char KEY_SONY_IMAGE_STABILISER_VALUES[] = "sony-is-values";
static const char KEY_SONY_IMAGE_STABILISER[] = "sony-is";
static const char KEY_SONY_VIDEO_STABILISER[] = "sony-vs";
static const char KEY_SONY_VIDEO_STABILISER_VALUES[] = "sony-vs-values";
static const char KEY_SONY_VIDEO_HDR[] = "sony-video-hdr";
static const char KEY_SONY_VIDEO_HDR_VALUES[] = "sony-video-hdr-values";
static const char KEY_SONY_ISO_AVAIL_MODES[] = "sony-iso-values";
static const char KEY_SONY_ISO_MODE[] = "sony-iso";
static const char KEY_SONY_AE_MODE_VALUES[] = "sony-ae-mode-values";
static const char KEY_SONY_AE_MODE[] = "sony-ae-mode";

// Wrapper Sony specific parameters values
static const char VALUE_SONY_ON[] = "on";
static const char VALUE_SONY_OFF[] = "off";
static const char VALUE_SONY_STILL_HDR[] = "on-still-hdr";
static const char VALUE_SONY_INTELLIGENT_ACTIVE[] = "on-intelligent-active";

static camera_notify_callback gUserNotifyCb = NULL;
static camera_data_callback gUserDataCb = NULL;
static camera_data_timestamp_callback gUserDataCbTimestamp = NULL;
static camera_request_memory gUserGetMemory = NULL;
static void *gUserCameraDevice = NULL;

static int camera_device_open(const hw_module_t *module, const char *name,
        hw_device_t **device);
static int camera_device_open_legacy(const hw_module_t *module,
         const char *name, uint32_t halVersion, hw_device_t **device);
static int camera_device_close(hw_device_t *device);
static int camera_get_number_of_cameras(void);
static int camera_get_camera_info(int camera_id, struct camera_info *info);
static int camera_set_callbacks(const camera_module_callbacks_t *callbacks);
static void camera_get_vendor_tag_ops(vendor_tag_ops_t *ops);
static int camera_open_legacy(const struct hw_module_t *module,
        const char *id, uint32_t halVersion, struct hw_device_t **device);
static int camera_set_torch_mode(const char *camera_id, bool enabled);

static struct hw_module_methods_t camera_module_methods = {
    .open = camera_device_open
};

camera_module_t HAL_MODULE_INFO_SYM = {
    .common = {
         .tag = HARDWARE_MODULE_TAG,
         .module_api_version = CAMERA_MODULE_API_VERSION_2_4,
         .hal_api_version = HARDWARE_HAL_API_VERSION,
         .id = CAMERA_HARDWARE_MODULE_ID,
         .name = "Sony Camera Wrapper",
         .author = "The LineageOS Project",
         .methods = &camera_module_methods,
         .dso = NULL, /* remove compilation warnings */
         .reserved = {0}, /* remove compilation warnings */
    },
    .get_number_of_cameras = camera_get_number_of_cameras,
    .get_camera_info = camera_get_camera_info,
    .set_callbacks = camera_set_callbacks,
    .get_vendor_tag_ops = camera_get_vendor_tag_ops,
    .open_legacy = camera_open_legacy,
    .set_torch_mode = camera_set_torch_mode,
    .init = NULL, /* remove compilation warnings */
    .reserved = {0}, /* remove compilation warnings */
};

typedef struct wrapper_camera_device {
    camera_device_t base;
    int camera_released;
    int id;
    camera_device_t *vendor;
} wrapper_camera_device_t;

#define VENDOR_CALL(device, func, ...) ({ \
    wrapper_camera_device_t *__wrapper_dev = (wrapper_camera_device_t*) device; \
    __wrapper_dev->vendor->ops->func(__wrapper_dev->vendor, ##__VA_ARGS__); \
})

#define CAMERA_ID(device) (((wrapper_camera_device_t *)(device))->id)

static int check_vendor_module()
{
    int rv = 0;
    ALOGV("%s", __FUNCTION__);

    if (gVendorModule)
        return 0;

    rv = hw_get_module_by_class("camera", "vendor",
            (const hw_module_t**)&gVendorModule);

    if (rv)
        ALOGE("Failed to open vendor camera module");
    return rv;
}

static char *camera_fixup_getparams(int id __unused, const char *settings)
{
    CameraParameters params;
    params.unflatten(String8(settings));

#ifdef LOG_PARAMETERS
    ALOGV("%s: Original parameters:", __FUNCTION__);
    params.dump();
#endif

    if (params.get(KEY_SONY_IMAGE_STABILISER_VALUES)) {
        const char *supportedIsModes = params.get(
                KEY_SONY_IMAGE_STABILISER_VALUES);

        if (strstr(supportedIsModes, VALUE_SONY_STILL_HDR) != NULL) {
            char buffer[512];
            const char *supportedSceneModes = params.get(
                    CameraParameters::KEY_SUPPORTED_SCENE_MODES);
            sprintf(buffer, "%s,hdr", supportedSceneModes);
            params.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES,
                    buffer);
        }
    }

    if (params.get(KEY_SONY_ISO_AVAIL_MODES)) {
        const char *isoModeList = params.get(KEY_SONY_ISO_AVAIL_MODES);
        char buffer[255] = "ISO";
        size_t bufferPos = 3;
        for (size_t pos = 0; pos < strlen(isoModeList); pos++) {
            if (isoModeList[pos] != ',') {
                buffer[bufferPos++] = isoModeList[pos];
            } else {
                strcat(buffer, ",ISO");
                bufferPos += 4;
            }
        }
        strcat(buffer, ",auto");
        params.set(KEY_SUPPORTED_ISO_MODES, buffer);
    }

    if (params.get(KEY_SONY_IMAGE_STABILISER)) {
        const char *sony_is = params.get(KEY_SONY_IMAGE_STABILISER);
        if (strcmp(sony_is, VALUE_SONY_STILL_HDR) == 0) {
            params.set(CameraParameters::KEY_SCENE_MODE, "hdr");
        }
    }

    if (params.get(KEY_SONY_VIDEO_HDR) &&
            params.get(KEY_SONY_VIDEO_HDR_VALUES)) {
        params.set("video-hdr-values", params.get(KEY_SONY_VIDEO_HDR_VALUES));
        params.set("video-hdr", params.get(KEY_SONY_VIDEO_HDR));
    }

    if (params.get(KEY_SONY_ISO_MODE)) {
        if (params.get(KEY_SONY_AE_MODE_VALUES)) {
            const char *aeMode = params.get(KEY_SONY_AE_MODE);
            if (strcmp(aeMode, "auto") == 0 ) {
                params.set(KEY_ISO_MODE, "auto");
                params.set("shutter-speed", "auto");
            } else if (strcmp(aeMode, "iso-prio") == 0) {
                char *isoVal = (char*)malloc(sizeof(char) * (3 + strlen(
                        params.get(KEY_SONY_ISO_MODE))));
                sprintf(isoVal, "ISO%s", params.get(KEY_SONY_ISO_MODE));
                params.set(KEY_ISO_MODE, isoVal);
                params.set("shutter-speed", "auto");
            } else if (strcmp(aeMode, "shutter-prio") == 0) {
                params.set(KEY_ISO_MODE, "auto");
                const char *shutterSpeed = params.get("sony-shutter-speed");
                if (shutterSpeed) {
                    params.set("shutter-speed",shutterSpeed);
                }
            } else if (strcmp(aeMode, "manual") == 0) {
                const char *shutterSpeed = params.get("sony-shutter-speed");
                if (shutterSpeed) {
                    params.set("shutter-speed",shutterSpeed);
                }
                char *isoVal = (char*)malloc(sizeof(char) * (3 + strlen(
                        params.get(KEY_SONY_ISO_MODE))));
                sprintf(isoVal, "ISO%s" ,params.get(KEY_SONY_ISO_MODE));
                params.set(KEY_ISO_MODE, isoVal);
            } else {
                params.set(KEY_ISO_MODE, "auto");
                params.set("shutter-speed", "auto");
            }
        }
    }

#ifdef LOG_PARAMETERS
    ALOGV("%s: Fixed parameters:", __FUNCTION__);
    params.dump();
#endif

    String8 strParams = params.flatten();
    char *ret = strdup(strParams.string());

    return ret;
}

static char *camera_fixup_setparams(int id __unused, const char *settings)
{
    CameraParameters params;
    params.unflatten(String8(settings));

#ifdef LOG_PARAMETERS
    ALOGV("%s: Original parameters:", __FUNCTION__);
    params.dump();
#endif

    params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "1000,60000");

    const char *shutterSpeed = params.get("shutter-speed");
    if (shutterSpeed) {
        if (strcmp(shutterSpeed, "auto") != 0) {
            params.set("sony-shutter-speed", shutterSpeed);
            params.set(KEY_SONY_AE_MODE, "shutter-prio");
        } else {
            const char *aeModes = params.get(KEY_SONY_AE_MODE_VALUES);
            if (strstr(aeModes, "auto") != NULL) {
                params.set(KEY_SONY_AE_MODE, "auto");
            }
        }
    }

    if (params.get(KEY_ISO_MODE)) {
        const char *isoMode = params.get(KEY_ISO_MODE);
        if (strcmp(isoMode, "auto") != 0) {
            params.set(KEY_SONY_ISO_MODE, isoMode + 3);
        }
        if (params.get(KEY_SONY_AE_MODE_VALUES)) {
            const char *aeModes = params.get(KEY_SONY_AE_MODE_VALUES);
            if (strcmp(isoMode, "auto") == 0) {
                if ((strstr(aeModes, "auto") != NULL) && (strcmp(params.get(
                        KEY_SONY_AE_MODE), "shutter-prio") != 0)) {
                    params.set(KEY_SONY_AE_MODE, "auto");
                }
            } else if (strstr(aeModes, "iso-prio") != NULL) {
                if (strcmp(params.get(KEY_SONY_AE_MODE),
                        "shutter-prio") == 0) {
                    params.set(KEY_SONY_AE_MODE, "manual");
                } else {
                    params.set(KEY_SONY_AE_MODE, "iso-prio");
                }
            }
        }
    }

    if (params.get(CameraParameters::KEY_SCENE_MODE)) {
        const char *sceneMode = params.get(
                CameraParameters::KEY_SCENE_MODE);
        if (strcmp(sceneMode, "hdr") == 0) {
            params.set(KEY_SONY_IMAGE_STABILISER, VALUE_SONY_STILL_HDR);
            params.set(CameraParameters::KEY_SCENE_MODE,
                    CameraParameters::SCENE_MODE_AUTO);
        } else {
            params.set(KEY_SONY_IMAGE_STABILISER, VALUE_SONY_ON);
        }
    }

    if (params.get(KEY_SONY_VIDEO_HDR) && params.get("video-hdr")) {
        params.set(KEY_SONY_VIDEO_HDR, params.get("video-hdr"));
    }

    if (params.get(CameraParameters::KEY_RECORDING_HINT) &&
            strcmp(params.get(CameraParameters::KEY_RECORDING_HINT),
            CameraParameters::TRUE) == 0) {
        if (params.get(KEY_SONY_VIDEO_STABILISER_VALUES) &&
                (strstr(params.get(KEY_SONY_VIDEO_STABILISER_VALUES),
                    VALUE_SONY_INTELLIGENT_ACTIVE) != NULL) ) {
            params.set(KEY_SONY_VIDEO_STABILISER,
                    VALUE_SONY_INTELLIGENT_ACTIVE);
        } else {
            params.set(KEY_SONY_VIDEO_STABILISER, VALUE_SONY_ON);
        }
        params.set(KEY_SONY_IMAGE_STABILISER, VALUE_SONY_OFF);
    }

#ifdef LOG_PARAMETERS
    ALOGV("%s: Fixed parameters:", __FUNCTION__);
    params.dump();
#endif

    String8 strParams = params.flatten();
    char *ret = strdup(strParams.string());

    return ret;
}

/*******************************************************************
 * implementation of camera_device_ops_t functions
 *******************************************************************/

static int camera_set_preview_window(struct camera_device *device,
        struct preview_stream_ops *window)
{
    if (!device)
        return -EINVAL;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    return VENDOR_CALL(device, set_preview_window, window);
}

void camera_notify_cb(int32_t msg_type, int32_t ext1, int32_t ext2, void *user) {
    gUserNotifyCb(msg_type, ext1, ext2, gUserCameraDevice);
}

void camera_data_cb(int32_t msg_type, const camera_memory_t *data, unsigned int index,
        camera_frame_metadata_t *metadata, void *user) {
    gUserDataCb(msg_type, data, index, metadata, gUserCameraDevice);
}

void camera_data_cb_timestamp(nsecs_t timestamp, int32_t msg_type,
        const camera_memory_t *data, unsigned index, void *user) {
    gUserDataCbTimestamp(timestamp, msg_type, data, index, gUserCameraDevice);
}

camera_memory_t* camera_get_memory(int fd, size_t buf_size,
        uint_t num_bufs, void *user) {
    return gUserGetMemory(fd, buf_size, num_bufs, gUserCameraDevice);
}

static void camera_set_callbacks(struct camera_device *device,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void *user)
{
    if (!device)
        return;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    gUserNotifyCb = notify_cb;
    gUserDataCb = data_cb;
    gUserDataCbTimestamp = data_cb_timestamp;
    gUserGetMemory = get_memory;
    gUserCameraDevice = user;

    VENDOR_CALL(device, set_callbacks, camera_notify_cb, camera_data_cb,
            camera_data_cb_timestamp, camera_get_memory, user);
}

static void camera_enable_msg_type(struct camera_device *device,
        int32_t msg_type)
{
    if (!device)
        return;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    VENDOR_CALL(device, enable_msg_type, msg_type);
}

static void camera_disable_msg_type(struct camera_device *device,
        int32_t msg_type)
{
    if (!device)
        return;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    VENDOR_CALL(device, disable_msg_type, msg_type);
}

static int camera_msg_type_enabled(struct camera_device *device,
        int32_t msg_type)
{
    if (!device)
        return 0;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    return VENDOR_CALL(device, msg_type_enabled, msg_type);
}

static int camera_start_preview(struct camera_device *device)
{
    if (!device)
        return -EINVAL;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    return VENDOR_CALL(device, start_preview);
}

static void camera_stop_preview(struct camera_device *device)
{
    if (!device)
        return;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    VENDOR_CALL(device, stop_preview);
}

static int camera_preview_enabled(struct camera_device *device)
{
    if (!device)
        return -EINVAL;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    return VENDOR_CALL(device, preview_enabled);
}

static int camera_store_meta_data_in_buffers(struct camera_device *device,
        int enable)
{
    if (!device)
        return -EINVAL;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    return VENDOR_CALL(device, store_meta_data_in_buffers, enable);
}

static int camera_start_recording(struct camera_device *device)
{
    if (!device)
        return EINVAL;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    return VENDOR_CALL(device, start_recording);
}

static void camera_stop_recording(struct camera_device *device)
{
    if (!device)
        return;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    VENDOR_CALL(device, stop_recording);
}

static int camera_recording_enabled(struct camera_device *device)
{
    if (!device)
        return -EINVAL;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    return VENDOR_CALL(device, recording_enabled);
}

static void camera_release_recording_frame(struct camera_device *device,
                const void *opaque)
{
    if (!device)
        return;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    VENDOR_CALL(device, release_recording_frame, opaque);
}

static int camera_auto_focus(struct camera_device *device)
{
    if (!device)
        return -EINVAL;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    return VENDOR_CALL(device, auto_focus);
}

static int camera_cancel_auto_focus(struct camera_device *device)
{
    if (!device)
        return -EINVAL;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    return VENDOR_CALL(device, cancel_auto_focus);
}

static int camera_take_picture(struct camera_device *device)
{
    if (!device)
        return -EINVAL;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    return VENDOR_CALL(device, take_picture);
}

static int camera_cancel_picture(struct camera_device *device)
{
    if (!device)
        return -EINVAL;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    return VENDOR_CALL(device, cancel_picture);
}

static int camera_set_parameters(struct camera_device *device,
        const char *params)
{
    if (!device)
        return -EINVAL;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    char *tmp = NULL;
    tmp = camera_fixup_setparams(CAMERA_ID(device), params);

    int ret = VENDOR_CALL(device, set_parameters, tmp);

    return ret;
}

static char *camera_get_parameters(struct camera_device *device)
{
    if (!device)
        return NULL;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    char *params = VENDOR_CALL(device, get_parameters);

    char *tmp = camera_fixup_getparams(CAMERA_ID(device), params);
    VENDOR_CALL(device, put_parameters, params);
    params = tmp;

    return params;
}

static void camera_put_parameters(struct camera_device *device, char *params)
{
    if (params)
        free(params);

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));
}

static int camera_send_command(struct camera_device *device,
        int32_t cmd, int32_t arg1, int32_t arg2)
{
    if (!device)
        return -EINVAL;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    return VENDOR_CALL(device, send_command, cmd, arg1, arg2);
}

static void camera_release(struct camera_device *device)
{
    wrapper_camera_device_t* wrapper_dev = NULL;

    if (!device)
        return;

    wrapper_dev = (wrapper_camera_device_t*) device;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(wrapper_dev->vendor));

    VENDOR_CALL(device, release);

    wrapper_dev->camera_released = true;
}

static int camera_dump(struct camera_device *device, int fd)
{
    if (!device)
        return -EINVAL;

    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)device,
            (uintptr_t)(((wrapper_camera_device_t*)device)->vendor));

    return VENDOR_CALL(device, dump, fd);
}

static int camera_device_close(hw_device_t *device)
{
    int ret = 0;
    wrapper_camera_device_t *wrapper_dev = NULL;

    ALOGV("%s", __FUNCTION__);

    Mutex::Autolock lock(gCameraWrapperLock);

    if (!device) {
        ret = -EINVAL;
        goto done;
    }

    wrapper_dev = (wrapper_camera_device_t*) device;

    if (!wrapper_dev->camera_released) {
        ALOGI("%s: releasing camera device with id %d", __FUNCTION__,
                wrapper_dev->id);

        VENDOR_CALL(wrapper_dev, release);

        wrapper_dev->camera_released = true;
    }

    wrapper_dev->vendor->common.close((hw_device_t*)wrapper_dev->vendor);

    if (wrapper_dev->base.ops)
        free(wrapper_dev->base.ops);

    free(wrapper_dev);

done:
    return ret;
}

/*******************************************************************
 * implementation of camera_module functions
 *******************************************************************/

static int camera_device_open(const hw_module_t *module, const char *name,
        hw_device_t **device)
{
    int rv = 0;

    if (name != NULL) {
        if (check_vendor_module())
            return -EINVAL;

        rv = camera3_device_open(module, name, device);
    }

    return rv;
}

static int camera_device_open_legacy(const hw_module_t *module,
        const char *name, uint32_t halVersion, hw_device_t **device)
{
    int rv = 0;
    int num_cameras = 0;
    int cameraid;
    wrapper_camera_device_t *camera_device = NULL;
    camera_device_ops_t *camera_ops = NULL;

    Mutex::Autolock lock(gCameraWrapperLock);

    ALOGV("%s", __FUNCTION__);

    if (name != NULL) {
        if (check_vendor_module())
            return -EINVAL;

        cameraid = atoi(name);
        num_cameras = gVendorModule->get_number_of_cameras();

        if (cameraid > num_cameras) {
            ALOGE("Camera service provided cameraid out of bounds, "
                    "cameraid = %d, num supported = %d",
                    cameraid, num_cameras);
            rv = -EINVAL;
            goto fail;
        }

        camera_device = (wrapper_camera_device_t*)malloc(sizeof(*camera_device));
        if (!camera_device) {
            ALOGE("camera_device allocation fail");
            rv = -ENOMEM;
            goto fail;
        }
        memset(camera_device, 0, sizeof(*camera_device));
        camera_device->camera_released = false;
        camera_device->id = cameraid;

        rv = gVendorModule->open_legacy(
                (const hw_module_t*)gVendorModule, name,
                halVersion, (hw_device_t**)&(camera_device->vendor));
        if (rv) {
            ALOGE("Vendor camera open_legacy fail");
            goto fail;
        }
        ALOGI("%s: got vendor camera device 0x%08X",
                __FUNCTION__, (uintptr_t)(camera_device->vendor));

        camera_ops = (camera_device_ops_t*)malloc(sizeof(*camera_ops));
        if (!camera_ops) {
            ALOGE("camera_ops allocation fail");
            rv = -ENOMEM;
            goto fail;
        }

        memset(camera_ops, 0, sizeof(*camera_ops));

        camera_device->base.common.tag = HARDWARE_DEVICE_TAG;
        camera_device->base.common.version = HARDWARE_DEVICE_API_VERSION(1, 0);
        camera_device->base.common.module = (hw_module_t *)(module);
        camera_device->base.common.close = camera_device_close;
        camera_device->base.ops = camera_ops;

        camera_ops->set_preview_window = camera_set_preview_window;
        camera_ops->set_callbacks = camera_set_callbacks;
        camera_ops->enable_msg_type = camera_enable_msg_type;
        camera_ops->disable_msg_type = camera_disable_msg_type;
        camera_ops->msg_type_enabled = camera_msg_type_enabled;
        camera_ops->start_preview = camera_start_preview;
        camera_ops->stop_preview = camera_stop_preview;
        camera_ops->preview_enabled = camera_preview_enabled;
        camera_ops->store_meta_data_in_buffers = camera_store_meta_data_in_buffers;
        camera_ops->start_recording = camera_start_recording;
        camera_ops->stop_recording = camera_stop_recording;
        camera_ops->recording_enabled = camera_recording_enabled;
        camera_ops->release_recording_frame = camera_release_recording_frame;
        camera_ops->auto_focus = camera_auto_focus;
        camera_ops->cancel_auto_focus = camera_cancel_auto_focus;
        camera_ops->take_picture = camera_take_picture;
        camera_ops->cancel_picture = camera_cancel_picture;
        camera_ops->set_parameters = camera_set_parameters;
        camera_ops->get_parameters = camera_get_parameters;
        camera_ops->put_parameters = camera_put_parameters;
        camera_ops->send_command = camera_send_command;
        camera_ops->release = camera_release;
        camera_ops->dump = camera_dump;

        *device = &camera_device->base.common;
    }

    return rv;

fail:
    if (camera_device) {
        free(camera_device);
        camera_device = NULL;
    }
    if (camera_ops) {
        free(camera_ops);
        camera_ops = NULL;
    }
    *device = NULL;
    return rv;
}

static int camera_get_number_of_cameras(void)
{
    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;

    return gVendorModule->get_number_of_cameras();
}

static int camera_get_camera_info(int camera_id, struct camera_info *info)
{
    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;

    return gVendorModule->get_camera_info(camera_id, info);
}

static int camera_set_callbacks(const camera_module_callbacks_t *callbacks)
{
    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;

    return gVendorModule->set_callbacks(callbacks);
}

static void camera_get_vendor_tag_ops(vendor_tag_ops_t *ops)
{
    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return;

    gVendorModule->get_vendor_tag_ops(ops);
}

static int camera_open_legacy(const struct hw_module_t *module,
        const char *id, uint32_t halVersion, struct hw_device_t **device)
{
    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;

    return camera_device_open_legacy(module, id, halVersion, device);
}

static int camera_set_torch_mode(const char *camera_id, bool enabled)
{
    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;

    return gVendorModule->set_torch_mode(camera_id, enabled);
}
