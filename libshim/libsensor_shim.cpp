#include <stdint.h>
#include <sys/types.h>
#include <sys/eventfd.h>

namespace android {

class Sensor;
class String8;
class String16;
struct ASensorEvent;

class SensorManager;
class SensorEventQueue;

// Sentinel non-null pointers so the blob's chained calls don't deref null.
static SensorManager* const kFakeSensorManager =
    reinterpret_cast<SensorManager*>(0x1);
static SensorEventQueue* const kFakeEventQueue =
    reinterpret_cast<SensorEventQueue*>(0x2);

extern "C" __attribute__((visibility("default")))
SensorManager* _ZN7android13SensorManager21getInstanceForPackageERKNS_8String16E(
        const String16* /*name*/);

SensorManager* _ZN7android13SensorManager21getInstanceForPackageERKNS_8String16E(
        const String16*) {
    return kFakeSensorManager;
}

extern "C" __attribute__((visibility("default")))
Sensor* _ZN7android13SensorManager16getDefaultSensorEi(
        SensorManager* /*this*/, int /*type*/);

Sensor* _ZN7android13SensorManager16getDefaultSensorEi(
        SensorManager*, int type) {
    // SENSOR_TYPE_LIGHT (5) -> null: else libcammw spawns a Looper-polling
    // thread that crashes in Looper::pollOnce (Sony A7.1 Looper layout vs A16).
    if (type == 5) {
        return nullptr;
    }
    return reinterpret_cast<Sensor*>(0x3);
}

extern "C" __attribute__((visibility("default")))
SensorEventQueue* _ZN7android13SensorManager16createEventQueueENS_7String8Ei(
        SensorManager* /*this*/, void* /*packageName_string8*/, int /*mode*/);

SensorEventQueue* _ZN7android13SensorManager16createEventQueueENS_7String8Ei(
        SensorManager*, void*, int) {
    return kFakeEventQueue;
}

extern "C" __attribute__((visibility("default")))
ssize_t _ZN7android16SensorEventQueue4readEP12ASensorEventj(
        SensorEventQueue* /*this*/, ASensorEvent* /*events*/, uint32_t /*count*/);

ssize_t _ZN7android16SensorEventQueue4readEP12ASensorEventj(
        SensorEventQueue*, ASensorEvent*, uint32_t) {
    return 0;
}

extern "C" __attribute__((visibility("default")))
int _ZNK7android16SensorEventQueue12enableSensorEPKNS_6SensorE(
        const SensorEventQueue* /*this*/, const Sensor* /*sensor*/);

int _ZNK7android16SensorEventQueue12enableSensorEPKNS_6SensorE(
        const SensorEventQueue*, const Sensor*) {
    return 0;
}

extern "C" __attribute__((visibility("default")))
int _ZNK7android16SensorEventQueue12setEventRateEPKNS_6SensorEx(
        const SensorEventQueue* /*this*/, const Sensor* /*sensor*/,
        int64_t /*ns*/);

int _ZNK7android16SensorEventQueue12setEventRateEPKNS_6SensorEx(
        const SensorEventQueue*, const Sensor*, int64_t) {
    return 0;
}

extern "C" __attribute__((visibility("default")))
int _ZNK7android16SensorEventQueue13disableSensorEPKNS_6SensorE(
        const SensorEventQueue* /*this*/, const Sensor* /*sensor*/);

int _ZNK7android16SensorEventQueue13disableSensorEPKNS_6SensorE(
        const SensorEventQueue*, const Sensor*) {
    return 0;
}

extern "C" __attribute__((visibility("default")))
int _ZNK7android16SensorEventQueue5getFdEv(const SensorEventQueue* /*this*/);

int _ZNK7android16SensorEventQueue5getFdEv(const SensorEventQueue*) {
    // Return a real eventfd, not -1: ReqClient3::open adds this fd to a Looper
    // via addEventListener, and -1 fails that -> -38 ENOSYS on HAL3 open.
    static int s_fakeFd = -1;
    if (s_fakeFd < 0) {
        s_fakeFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    }
    return s_fakeFd;
}

} // namespace android
