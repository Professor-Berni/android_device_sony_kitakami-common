/*
 * Copyright (C) 2026 The LineageOS Project
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

#define LOG_TAG "sensors.kitakami"

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <hardware/sensors.h>
#include <log/log.h>

static pthread_once_t load_once = PTHREAD_ONCE_INIT;
static struct sensors_module_t *ssc_module;
static struct sensor_t *sensor_list;
static int sensor_count;

static void load_ssc(void) {
    void *handle = dlopen("sensors.ssc.so", RTLD_NOW);
    if (!handle) {
        ALOGE("dlopen: %s", dlerror());
        return;
    }
    ssc_module = (struct sensors_module_t *)dlsym(handle, HAL_MODULE_INFO_SYM_AS_STR);
    if (!ssc_module) {
        ALOGE("dlsym: %s", dlerror());
        dlclose(handle);
        return;
    }

    const struct sensor_t *list;
    int count = ssc_module->get_sensors_list(ssc_module, &list);
    sensor_list = calloc(count, sizeof(*sensor_list));
    if (!sensor_list) return;
    memcpy(sensor_list, list, count * sizeof(*sensor_list));
    sensor_count = count;

    for (int i = 0; i < count; i++) {
        if (sensor_list[i].type == SENSOR_TYPE_PROXIMITY)
            sensor_list[i].resolution = sensor_list[i].maxRange;
    }
}

static int open_sensors(const struct hw_module_t *module __unused, const char *name,
                        struct hw_device_t **device) {
    pthread_once(&load_once, load_ssc);
    if (!ssc_module) return -EINVAL;
    return ssc_module->common.methods->open(&ssc_module->common, name, device);
}

static int get_sensors_list(struct sensors_module_t *module __unused,
                            const struct sensor_t **list) {
    pthread_once(&load_once, load_ssc);
    *list = sensor_list;
    return sensor_count;
}

static struct hw_module_methods_t sensors_module_methods = {
    .open = open_sensors,
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = SENSORS_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = SENSORS_HARDWARE_MODULE_ID,
        .name = "Kitakami sensors module",
        .author = "The LineageOS Project",
        .methods = &sensors_module_methods,
    },
    .get_sensors_list = get_sensors_list,
};
