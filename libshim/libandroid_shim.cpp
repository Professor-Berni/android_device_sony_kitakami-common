#include <stdint.h>
#include <sys/types.h>

extern "C" {

#define ALOOPER_POLL_TIMEOUT (-3)

struct ASensorManager;
struct ASensor;
struct ASensorEventQueue;
struct ALooper;
typedef int (*ALooper_callbackFunc)(int fd, int events, void* data);

__attribute__((visibility("default")))
ASensorManager* ASensorManager_getInstanceForPackage(const char* /*name*/) {
    return nullptr;
}

__attribute__((visibility("default")))
const ASensor* ASensorManager_getDefaultSensor(ASensorManager*, int /*type*/) {
    return nullptr;
}

__attribute__((visibility("default")))
const char* ASensor_getName(const ASensor*) {
    return "stub";
}

__attribute__((visibility("default")))
const char* ASensor_getVendor(const ASensor*) {
    return "stub";
}

__attribute__((visibility("default")))
int32_t ASensor_getMinDelay(const ASensor*) {
    return 0;
}

__attribute__((visibility("default")))
ALooper* ALooper_forThread(void) {
    return nullptr;
}

__attribute__((visibility("default")))
ALooper* ALooper_prepare(int /*opts*/) {
    return nullptr;
}

__attribute__((visibility("default")))
ASensorEventQueue* ASensorManager_createEventQueue(
        ASensorManager*, ALooper*, int /*ident*/,
        ALooper_callbackFunc /*callback*/, void* /*data*/) {
    return nullptr;
}

__attribute__((visibility("default")))
int ALooper_pollOnce(int /*timeoutMillis*/, int* /*outFd*/,
                     int* /*outEvents*/, void** /*outData*/) {
    return ALOOPER_POLL_TIMEOUT;
}

__attribute__((visibility("default")))
int ASensorManager_destroyEventQueue(ASensorManager*, ASensorEventQueue*) {
    return 0;
}

__attribute__((visibility("default")))
void ALooper_wake(ALooper*) {
}

__attribute__((visibility("default")))
int ASensorEventQueue_enableSensor(ASensorEventQueue*, const ASensor*) {
    return 0;
}

__attribute__((visibility("default")))
int ASensorEventQueue_disableSensor(ASensorEventQueue*, const ASensor*) {
    return 0;
}

__attribute__((visibility("default")))
int ASensorEventQueue_setEventRate(ASensorEventQueue*, const ASensor*,
                                   int32_t /*usec*/) {
    return 0;
}

__attribute__((visibility("default")))
ssize_t ASensorEventQueue_getEvents(ASensorEventQueue*, void* /*events*/,
                                    size_t /*count*/) {
    return 0;
}

__attribute__((visibility("default")))
ASensorManager* ASensorManager_getInstance(void) {
    return nullptr;
}

// AHardwareBuffer / ANativeWindow stubs (used by libcammw / libmmcamera_*)
struct AHardwareBuffer;
struct ANativeWindow;
struct ANativeWindowBuffer;

__attribute__((visibility("default")))
const struct native_handle* AHardwareBuffer_getNativeHandle(const AHardwareBuffer*) {
    return nullptr;
}

__attribute__((visibility("default")))
int AHardwareBuffer_lock(AHardwareBuffer*, uint64_t /*usage*/, int32_t /*fence*/,
                        const void* /*rect*/, void** out) {
    if (out) *out = nullptr;
    return -1;
}

__attribute__((visibility("default")))
int AHardwareBuffer_unlock(AHardwareBuffer*, int32_t* /*fence*/) {
    return 0;
}

__attribute__((visibility("default")))
AHardwareBuffer* ANativeWindowBuffer_getHardwareBuffer(ANativeWindowBuffer*) {
    return nullptr;
}

__attribute__((visibility("default")))
int ANativeWindow_cancelBuffer(ANativeWindow*, ANativeWindowBuffer*,
                              int /*fenceFd*/) {
    return 0;
}

__attribute__((visibility("default")))
int ANativeWindow_dequeueBuffer(ANativeWindow*, ANativeWindowBuffer** out,
                               int* /*fenceFd*/) {
    if (out) *out = nullptr;
    return -1;
}

__attribute__((visibility("default")))
int ANativeWindow_queueBuffer(ANativeWindow*, ANativeWindowBuffer*,
                             int /*fenceFd*/) {
    return 0;
}

__attribute__((visibility("default")))
void ANativeWindow_release(ANativeWindow*) {
}

__attribute__((visibility("default")))
int ANativeWindow_setBuffersGeometry(ANativeWindow*, int32_t /*w*/,
                                    int32_t /*h*/, int32_t /*format*/) {
    return 0;
}

} // extern "C"
