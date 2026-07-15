#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include <utils/Errors.h>

namespace android {

extern "C" __attribute__((visibility("default")))
status_t _ZN7android19GraphicBufferMapper6unlockEPK13native_handle(
        void* /* this */, buffer_handle_t handle);

status_t _ZN7android19GraphicBufferMapper6unlockEPK13native_handle(
        void* /* this */, buffer_handle_t handle) {
    return GraphicBufferMapper::get().unlock(handle, nullptr);
}

} // namespace android

extern "C" void _ZN7android13GraphicBufferC1EPK13native_handleNS0_16HandleWrapMethodEjjijyj(
        void* thisptr,
        const native_handle_t* handle,
        android::GraphicBuffer::HandleWrapMethod method,
        uint32_t width,
        uint32_t height,
        int format,
        uint32_t layerCount,
        uint64_t usage,
        uint32_t stride);

extern "C" __attribute__((visibility("default")))
void _ZN7android13GraphicBufferC1EjjijjP13native_handleb(
        void* thisptr,
        uint32_t inWidth,
        uint32_t inHeight,
        int inFormat,
        uint32_t inUsage,
        uint32_t inStride,
        native_handle_t* inHandle,
        bool keepOwnership)
{
    android::GraphicBuffer::HandleWrapMethod inMethod =
        (keepOwnership ? android::GraphicBuffer::TAKE_HANDLE
                       : android::GraphicBuffer::WRAP_HANDLE);
    _ZN7android13GraphicBufferC1EPK13native_handleNS0_16HandleWrapMethodEjjijyj(
        thisptr,
        inHandle, inMethod, inWidth, inHeight, inFormat,
        1u,
        static_cast<uint64_t>(inUsage),
        inStride);
}
