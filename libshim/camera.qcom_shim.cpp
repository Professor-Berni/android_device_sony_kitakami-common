#include <cutils/log.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <string.h>

#include <gui/BufferQueue.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <utils/Errors.h>
#include <utils/String8.h>
#include <utils/StrongPointer.h>

#include <ui/GraphicBuffer.h>
#include <private/gui/ComposerService.h>

using android::sp;
using android::status_t;
using android::IBinder;
using android::IGraphicBufferConsumer;
using android::IGraphicBufferProducer;
using android::PixelFormat;
using android::SurfaceControl;
using android::SurfaceComposerClient;

android::SurfaceComposerClient::Transaction *t = nullptr;
void* sc = nullptr;

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

// BufferItemConsumer(const sp<IGraphicBufferConsumer& consumer,
//                    uint64_t consumerUsage,
//                    int bufferCount = DEFAULT_MAX_BUFFERS,
//                    bool controlledByApp = false)

extern "C" void _ZN7android18BufferItemConsumerC1ERKNS_2spINS_22IGraphicBufferConsumerEEEyib(
    const sp<IGraphicBufferConsumer>& consumer, uint64_t consumerUsage,
    int bufferCount, bool controlledByApp);

extern "C" void _ZN7android18BufferItemConsumerC1ERKNS_2spINS_22IGraphicBufferConsumerEEEjib(
    const sp<IGraphicBufferConsumer>& consumer, uint32_t consumerUsage,
    int bufferCount, bool controlledByApp) {
  _ZN7android18BufferItemConsumerC1ERKNS_2spINS_22IGraphicBufferConsumerEEEyib(
      consumer, static_cast<uint64_t>(consumerUsage), bufferCount, controlledByApp);
}

extern "C" void _ZN7android11BufferQueue17createBufferQueueEPNS_2spINS_22IGraphicBufferProducerEEEPNS1_INS_22IGraphicBufferConsumerEEERKNS1_INS_19IGraphicBufferAllocEEE(
    sp<IGraphicBufferProducer>* outProducer,
    sp<IGraphicBufferConsumer>* outConsumer,
    void*) {
  // createBufferQueue is a static method, call it directly
  android::BufferQueue::createBufferQueue(outProducer, outConsumer);
}

// GraphicBuffer(uint32_t inWidth, uint32_t inHeight, PixelFormat inFormat,
//               uint32_t inUsage, std::string requestorName = "<Unknown>");
extern "C" void _ZN7android13GraphicBufferC1EjjijNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE(
    uint32_t inWidth, uint32_t inHeight, PixelFormat inFormat,
    uint32_t inUsage, std::string requestorName = "<Unknown>");

extern "C" void _ZN7android13GraphicBufferC1Ejjij(
    uint32_t inWidth, uint32_t inHeight, PixelFormat inFormat,
    uint32_t inUsage) {
  _ZN7android13GraphicBufferC1EjjijNSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE(
      inWidth, inHeight, inFormat, inUsage);
}

//sp<SurfaceControl> SurfaceComposerClient::createSurface(const String8& name, uint32_t w, uint32_t h,
//                                                        PixelFormat format, uint32_t flags,
//                                                        const sp<IBinder>& parentHandle,
//                                                        LayerMetadata metadata,
//                                                        uint32_t* outTransformHint)
extern "C" void* _ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8EjjijRKNS_2spINS_7IBinderEEENS_13LayerMetadataEPj(const android::String8& name, uint32_t w, uint32_t h,
                                                        PixelFormat format, uint32_t flags,
                                                        const sp<IBinder>& parentHandle,
                                                        android::LayerMetadata metadata,
                                                        uint32_t* outTransformHint);



extern "C" void* _ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8Ejjij(
    const android::String8& name, uint32_t w, uint32_t h, PixelFormat format,
    uint32_t flags) {
  android::LayerMetadata metadata;
  sc = _ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8EjjijRKNS_2spINS_7IBinderEEENS_13LayerMetadataEPj(name, w, h, format, flags, nullptr, metadata, nullptr);
  return sc;
}

// status_t setLayer(uint32_t layer);
extern "C" status_t _ZN7android14SurfaceControl8setLayerEj(uint32_t layer) {
  t->setLayer((SurfaceControl*)sc, layer);
  return android::NO_ERROR;
}

// android::Fence::~Fence()
extern "C" void _ZN7android5FenceD1Ev() {
  // no-op, the explicit destructor was replaced with = default;
}

extern "C" void _ZN7android21SurfaceComposerClient21openGlobalTransactionEv() {
  t = new(android::SurfaceComposerClient::Transaction);
}

extern "C" void _ZN7android21SurfaceComposerClient17setDisplaySurfaceERKNS_2spINS_7IBinderEEERKNS1_INS_22IGraphicBufferProducerEEE(
    const sp<IBinder>& token, const sp<IGraphicBufferProducer>& bufferProducer) {
  t->setDisplaySurface(token, bufferProducer);
}

extern "C" void _ZN7android21SurfaceComposerClient20setDisplayProjectionERKNS_2spINS_7IBinderEEEjRKNS_4RectES8_(
    const sp<IBinder>& token,
    uint32_t orientation,
    const android::Rect& layerStackRect,
    const android::Rect& displayRect) {
  t->setDisplayProjection(token, static_cast<android::ui::Rotation>(orientation), layerStackRect, displayRect);
}

extern "C" void _ZN7android21SurfaceComposerClient20setDisplayLayerStackERKNS_2spINS_7IBinderEEEj(
    const sp<IBinder>& token, uint32_t layerStack){
  t->setDisplayLayerStack(token, layerStack);
}

extern "C" status_t _ZN7android14SurfaceControl7setSizeEjj(uint32_t w, uint32_t h){
  t->setSize((SurfaceControl*)sc, w, h);
  return android::NO_ERROR;
}

extern "C" status_t _ZN7android14SurfaceControl11setPositionEff(float x, float y){
  t->setPosition((SurfaceControl*)sc, x, y);
  return android::NO_ERROR;
}

extern "C" void _ZN7android21SurfaceComposerClient22closeGlobalTransactionEb(){
  t->apply();
  delete t;
  t = nullptr;
}

extern "C" void _ZN7android14SurfaceControlD0Ev(void);
extern "C" void _ZN7android14SurfaceControlD1Ev(void);
extern "C" void _ZN7android14SurfaceControlD2Ev(void);

extern "C" void _ZN7android14SurfaceControl5clearEv(void){
  _ZN7android14SurfaceControlD0Ev();
}

//android::GraphicBuffer::lock(uint32_t inUsage, void** vaddr, int32_t* outBytesPerPixel,
//                             int32_t* outBytesPerStride);
extern "C" status_t _ZN7android13GraphicBuffer4lockEjPPvPiS3_(uint32_t inUsage, void** vaddr, int32_t* outBytesPerPixel,
                             int32_t* outBytesPerStride);

//status_t GraphicBuffer::lock(uint32_t inUsage, void** vaddr)
extern "C" status_t _ZN7android13GraphicBuffer4lockEjPPv(uint32_t inUsage, void** vaddr){
  return _ZN7android13GraphicBuffer4lockEjPPvPiS3_(inUsage, vaddr, nullptr, nullptr);
}
