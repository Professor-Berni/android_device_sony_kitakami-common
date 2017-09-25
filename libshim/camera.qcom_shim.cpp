#include <cutils/log.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <string.h>

#include <ui/GraphicBuffer.h>

extern "C" {
  int property_get(const char * key, char * value, const char * default_value) {
    if (strcmp("ro.build.type", key) == 0) {
      strcpy(value, "eng");
      return 3;
    }

    return ((int( * )(const char * , char *, const char * ))(dlsym((void * ) - 1, "property_get")))(key, value, default_value);
  }
}

extern "C" void _ZN7android13GraphicBufferC1EjjijjjP13native_handleb(
        const native_handle_t* handle,
        android::GraphicBuffer::HandleWrapMethod method,
        uint32_t width,
        uint32_t height,
        int format,
        uint32_t layerCount,
        uint64_t usage,
        uint32_t stride);

extern "C" void _ZN7android13GraphicBufferC1EjjijjP13native_handleb(
        uint32_t inWidth,
        uint32_t inHeight,
        int inFormat,
        uint32_t inUsage,
        uint32_t inStride,
        native_handle_t* inHandle,
        bool keepOwnership)
{
    android::GraphicBuffer::HandleWrapMethod inMethod =
        (keepOwnership ? android::GraphicBuffer::TAKE_HANDLE : android::GraphicBuffer::WRAP_HANDLE);
    _ZN7android13GraphicBufferC1EjjijjjP13native_handleb(inHandle, inMethod, inWidth, inHeight,
        inFormat, static_cast<uint32_t>(1), static_cast<uint64_t>(inUsage), inStride);
}

extern "C" void _ZN7android20DisplayEventReceiverC1ENS_16ISurfaceComposer11VsyncSourceE();

extern "C" void _ZN7android20DisplayEventReceiverC1Ev() {
    _ZN7android20DisplayEventReceiverC1ENS_16ISurfaceComposer11VsyncSourceE;
}
