#define LOG_TAG "sonycamera"

#include <dlfcn.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <android/hardware/camera/device/1.0/ICameraDevice.h>
#include <android/hardware/camera/provider/2.4/ICameraProvider.h>
#include <binder/IPCThreadState.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <binder/ProcessState.h>
#include <hardware/camera.h>
#include <hidl/HidlTransportSupport.h>
#include <log/log.h>

using android::IPCThreadState;
using android::MemoryBase;
using android::MemoryHeapBase;
using android::ProcessState;
using android::RefBase;
using android::sp;
using android::hardware::hidl_handle;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::camera::common::V1_0::Status;
using android::hardware::camera::device::V1_0::CameraFrameMetadata;
using android::hardware::camera::device::V1_0::CameraInfo;
using android::hardware::camera::device::V1_0::CommandType;
using android::hardware::camera::device::V1_0::DataCallbackMsg;
using android::hardware::camera::device::V1_0::HandleTimestampMessage;
using android::hardware::camera::device::V1_0::ICameraDevice;
using android::hardware::camera::device::V1_0::ICameraDeviceCallback;
using android::hardware::camera::device::V1_0::NotifyCallbackMsg;
using android::hardware::camera::provider::V2_4::ICameraProvider;

namespace {

constexpr uint32_t kNoMemory = UINT32_MAX;
constexpr int32_t kSonyMetadataMagic = 0x534f4e59;
constexpr size_t kSonyFaceSize = 100;

struct SonyFrameMetadata {
    int32_t number_of_faces;
    const uint8_t* faces;
    int32_t extra;
};

class HeapMemory : public RefBase {
  public:
    HeapMemory(int fd, size_t bufSize, uint32_t numBufs) : mBufSize(bufSize), mNumBufs(numBufs) {
        mHeap = new MemoryHeapBase(fd, bufSize * numBufs);
        mBuffers = new sp<MemoryBase>[numBufs];
        for (uint32_t i = 0; i < numBufs; i++) {
            mBuffers[i] = new MemoryBase(mHeap, i * bufSize, bufSize);
        }
        handle.data = mHeap->getBase();
        handle.size = bufSize * numBufs;
        handle.handle = this;
        handle.release = [](camera_memory_t*) {};
    }

    ~HeapMemory() override { delete[] mBuffers; }

    size_t mBufSize;
    uint32_t mNumBufs;
    sp<MemoryHeapBase> mHeap;
    sp<MemoryBase>* mBuffers;
    camera_memory_t handle;
};

class DeviceCallback : public ICameraDeviceCallback {
  public:
    void setCallbacks(camera_notify_callback notify, camera_data_callback data,
                      camera_data_timestamp_callback dataTimestamp, void* user) {
        std::lock_guard<std::mutex> lock(mLock);
        mNotify = notify;
        mData = data;
        mDataTimestamp = dataTimestamp;
        mUser = user;
    }

    bool findFrame(const void* opaque, uint32_t* memId, uint32_t* index) {
        std::lock_guard<std::mutex> lock(mLock);
        auto frame = reinterpret_cast<uintptr_t>(opaque);
        for (const auto& [id, mem] : mHeaps) {
            auto base = reinterpret_cast<uintptr_t>(mem->handle.data);
            if (mem->mBufSize != 0 && frame >= base && frame < base + mem->handle.size) {
                *memId = id;
                *index = (frame - base) / mem->mBufSize;
                return true;
            }
        }
        return false;
    }

    Return<void> notifyCallback(NotifyCallbackMsg msgType, int32_t ext1, int32_t ext2) override {
        camera_notify_callback notify;
        void* user;
        {
            std::lock_guard<std::mutex> lock(mLock);
            notify = mNotify;
            user = mUser;
        }
        if (notify != nullptr) {
            notify(static_cast<int32_t>(msgType), ext1, ext2, user);
        }
        return Void();
    }

    Return<uint32_t> registerMemory(const hidl_handle& descriptor, uint32_t bufferSize,
                                    uint32_t bufferCount) override {
        const native_handle_t* nh = descriptor.getNativeHandle();
        if (nh == nullptr || nh->numFds != 1 || nh->data[0] < 0) {
            ALOGE("%s: invalid memory descriptor", __func__);
            return 0;
        }
        sp<HeapMemory> mem = new HeapMemory(nh->data[0], bufferSize, bufferCount);
        if (mem->mHeap->getHeapID() < 0) {
            ALOGE("%s: cannot map %u buffers of %u bytes", __func__, bufferCount, bufferSize);
            return 0;
        }
        std::lock_guard<std::mutex> lock(mLock);
        uint32_t id = mNextId++;
        mHeaps[id] = mem;
        return id;
    }

    Return<void> unregisterMemory(uint32_t memId) override {
        std::lock_guard<std::mutex> lock(mLock);
        mHeaps.erase(memId);
        return Void();
    }

    Return<void> dataCallback(DataCallbackMsg msgType, uint32_t data, uint32_t bufferIndex,
                              const CameraFrameMetadata& metadata) override {
        camera_data_callback callback;
        void* user;
        sp<HeapMemory> mem;
        {
            std::lock_guard<std::mutex> lock(mLock);
            callback = mData;
            user = mUser;
            mem = findHeapLocked(data, bufferIndex);
        }
        if (callback == nullptr || (mem == nullptr && data != kNoMemory)) {
            return Void();
        }
        SonyFrameMetadata md = {};
        SonyFrameMetadata* mdp = nullptr;
        const auto& faces = metadata.faces;
        if (faces.size() > 0 && faces[0].rect[0] == kSonyMetadataMagic) {
            md.extra = faces[0].rect[1];
            md.number_of_faces = faces[0].rect[2];
            size_t available = (faces.size() - 1) * sizeof(faces[0]);
            if (md.number_of_faces < 0 || md.number_of_faces * kSonyFaceSize > available) {
                ALOGE("%s: bad face metadata (%d faces, %zu bytes)", __func__,
                      md.number_of_faces, available);
                return Void();
            }
            md.faces = md.number_of_faces > 0 ? reinterpret_cast<const uint8_t*>(&faces[1])
                                               : nullptr;
            mdp = &md;
        }
        callback(static_cast<int32_t>(msgType), mem != nullptr ? &mem->handle : nullptr,
                 bufferIndex, reinterpret_cast<camera_frame_metadata_t*>(mdp), user);
        return Void();
    }

    Return<void> dataCallbackTimestamp(DataCallbackMsg msgType, uint32_t data,
                                       uint32_t bufferIndex, int64_t timestamp) override {
        camera_data_timestamp_callback callback;
        void* user;
        sp<HeapMemory> mem;
        {
            std::lock_guard<std::mutex> lock(mLock);
            callback = mDataTimestamp;
            user = mUser;
            mem = findHeapLocked(data, bufferIndex);
        }
        if (callback != nullptr && mem != nullptr) {
            callback(timestamp, static_cast<int32_t>(msgType), &mem->handle, bufferIndex, user);
        }
        return Void();
    }

    Return<void> handleCallbackTimestamp(DataCallbackMsg, const hidl_handle&, uint32_t, uint32_t,
                                         int64_t) override {
        ALOGW("%s: native handle video frames are not supported", __func__);
        return Void();
    }

    Return<void> handleCallbackTimestampBatch(DataCallbackMsg,
                                              const hidl_vec<HandleTimestampMessage>&) override {
        ALOGW("%s: native handle video frames are not supported", __func__);
        return Void();
    }

  private:
    sp<HeapMemory> findHeapLocked(uint32_t memId, uint32_t index) {
        auto it = mHeaps.find(memId);
        if (it == mHeaps.end()) {
            if (memId != kNoMemory) {
                ALOGE("%s: unknown memory %u", __func__, memId);
            }
            return nullptr;
        }
        if (index >= it->second->mNumBufs) {
            ALOGE("%s: buffer %u out of range for memory %u", __func__, index, memId);
            return nullptr;
        }
        return it->second;
    }

    std::mutex mLock;
    camera_notify_callback mNotify = nullptr;
    camera_data_callback mData = nullptr;
    camera_data_timestamp_callback mDataTimestamp = nullptr;
    void* mUser = nullptr;
    std::unordered_map<uint32_t, sp<HeapMemory>> mHeaps;
    uint32_t mNextId = 1;
};

struct FacadeDevice {
    camera_device_t base;
    sp<ICameraDevice> hidl;
    sp<DeviceCallback> callback;
};

FacadeDevice* facade(camera_device_t* dev) {
    return reinterpret_cast<FacadeDevice*>(dev);
}

int toStatusT(Status status) {
    switch (status) {
        case Status::OK:
            return 0;
        case Status::ILLEGAL_ARGUMENT:
            return -EINVAL;
        case Status::CAMERA_IN_USE:
            return -EBUSY;
        case Status::MAX_CAMERAS_IN_USE:
            return -EUSERS;
        case Status::METHOD_NOT_SUPPORTED:
        case Status::OPERATION_NOT_SUPPORTED:
            return -ENOSYS;
        case Status::CAMERA_DISCONNECTED:
            return -EPIPE;
        default:
            return -ENODEV;
    }
}

int toStatusT(const Return<Status>& ret) {
    return toStatusT(ret.withDefault(Status::CAMERA_DISCONNECTED));
}

int setPreviewWindow(camera_device_t*, preview_stream_ops_t*) {
    return -ENOSYS;
}

void setCallbacks(camera_device_t* dev, camera_notify_callback notify, camera_data_callback data,
                  camera_data_timestamp_callback dataTimestamp, camera_request_memory,
                  void* user) {
    facade(dev)->callback->setCallbacks(notify, data, dataTimestamp, user);
}

void enableMsgType(camera_device_t* dev, int32_t msgType) {
    facade(dev)->hidl->enableMsgType(msgType).isOk();
}

void disableMsgType(camera_device_t* dev, int32_t msgType) {
    facade(dev)->hidl->disableMsgType(msgType).isOk();
}

int msgTypeEnabled(camera_device_t* dev, int32_t msgType) {
    return facade(dev)->hidl->msgTypeEnabled(msgType).withDefault(false);
}

int startPreview(camera_device_t* dev) {
    return toStatusT(facade(dev)->hidl->startPreview());
}

void stopPreview(camera_device_t* dev) {
    facade(dev)->hidl->stopPreview().isOk();
}

int previewEnabled(camera_device_t* dev) {
    return facade(dev)->hidl->previewEnabled().withDefault(false);
}

int storeMetaDataInBuffers(camera_device_t* dev, int enable) {
    return toStatusT(facade(dev)->hidl->storeMetaDataInBuffers(enable));
}

int startRecording(camera_device_t* dev) {
    return toStatusT(facade(dev)->hidl->startRecording());
}

void stopRecording(camera_device_t* dev) {
    facade(dev)->hidl->stopRecording().isOk();
}

int recordingEnabled(camera_device_t* dev) {
    return facade(dev)->hidl->recordingEnabled().withDefault(false);
}

void releaseRecordingFrame(camera_device_t* dev, const void* opaque) {
    uint32_t memId;
    uint32_t index;
    if (facade(dev)->callback->findFrame(opaque, &memId, &index)) {
        facade(dev)->hidl->releaseRecordingFrame(memId, index).isOk();
    } else {
        ALOGE("%s: unknown frame %p", __func__, opaque);
    }
}

int autoFocus(camera_device_t* dev) {
    return toStatusT(facade(dev)->hidl->autoFocus());
}

int cancelAutoFocus(camera_device_t* dev) {
    return toStatusT(facade(dev)->hidl->cancelAutoFocus());
}

int takePicture(camera_device_t* dev) {
    return toStatusT(facade(dev)->hidl->takePicture());
}

int cancelPicture(camera_device_t* dev) {
    return toStatusT(facade(dev)->hidl->cancelPicture());
}

int setParameters(camera_device_t* dev, const char* params) {
    return toStatusT(facade(dev)->hidl->setParameters(params));
}

char* getParameters(camera_device_t* dev) {
    std::string params;
    facade(dev)->hidl->getParameters([&](const hidl_string& value) { params = value; }).isOk();
    return strdup(params.c_str());
}

void putParameters(camera_device_t*, char* params) {
    free(params);
}

int sendCommand(camera_device_t* dev, int32_t cmd, int32_t arg1, int32_t arg2) {
    return toStatusT(facade(dev)->hidl->sendCommand(static_cast<CommandType>(cmd), arg1, arg2));
}

void releaseDevice(camera_device_t*) {}

int dumpDevice(camera_device_t*, int) {
    return 0;
}

int closeDevice(hw_device_t* device) {
    FacadeDevice* dev = reinterpret_cast<FacadeDevice*>(device);
    dev->callback->setCallbacks(nullptr, nullptr, nullptr, nullptr);
    dev->hidl->close().isOk();
    delete dev;
    return 0;
}

camera_device_ops_t makeOps() {
    camera_device_ops_t ops = {};
    ops.set_preview_window = setPreviewWindow;
    ops.set_callbacks = setCallbacks;
    ops.enable_msg_type = enableMsgType;
    ops.disable_msg_type = disableMsgType;
    ops.msg_type_enabled = msgTypeEnabled;
    ops.start_preview = startPreview;
    ops.stop_preview = stopPreview;
    ops.preview_enabled = previewEnabled;
    ops.store_meta_data_in_buffers = storeMetaDataInBuffers;
    ops.start_recording = startRecording;
    ops.stop_recording = stopRecording;
    ops.recording_enabled = recordingEnabled;
    ops.release_recording_frame = releaseRecordingFrame;
    ops.auto_focus = autoFocus;
    ops.cancel_auto_focus = cancelAutoFocus;
    ops.take_picture = takePicture;
    ops.cancel_picture = cancelPicture;
    ops.set_parameters = setParameters;
    ops.get_parameters = getParameters;
    ops.put_parameters = putParameters;
    ops.send_command = sendCommand;
    ops.release = releaseDevice;
    ops.dump = dumpDevice;
    return ops;
}

camera_device_ops_t gOps = makeOps();

std::mutex gProviderLock;
sp<ICameraProvider> gProvider;
std::vector<std::string> gDeviceNames;

bool loadProviderLocked() {
    if (gProvider != nullptr) {
        return true;
    }
    sp<ICameraProvider> provider = ICameraProvider::getService("legacy/0");
    if (provider == nullptr) {
        ALOGE("camera provider legacy/0 is not available");
        return false;
    }
    std::vector<std::string> names;
    auto ret = provider->getCameraIdList([&](Status status, const hidl_vec<hidl_string>& ids) {
        if (status != Status::OK) {
            return;
        }
        for (const auto& id : ids) {
            std::string name = id;
            if (name.rfind("device@1.0/", 0) != 0) {
                continue;
            }
            size_t camera = atoi(name.substr(name.rfind('/') + 1).c_str());
            if (camera >= names.size()) {
                names.resize(camera + 1);
            }
            names[camera] = name;
        }
    });
    if (!ret.isOk() || names.empty()) {
        ALOGE("cannot list the HALv1 cameras");
        return false;
    }
    gProvider = provider;
    gDeviceNames = names;
    return true;
}

sp<ICameraDevice> getDevice(int camera) {
    for (int attempt = 0; attempt < 2; attempt++) {
        sp<ICameraProvider> provider;
        std::string name;
        {
            std::lock_guard<std::mutex> lock(gProviderLock);
            if (!loadProviderLocked() || camera < 0 ||
                    camera >= static_cast<int>(gDeviceNames.size()) ||
                    gDeviceNames[camera].empty()) {
                return nullptr;
            }
            provider = gProvider;
            name = gDeviceNames[camera];
        }
        sp<ICameraDevice> device;
        auto ret = provider->getCameraDeviceInterface_V1_x(
                name, [&](Status status, const sp<ICameraDevice>& d) {
                    if (status == Status::OK) {
                        device = d;
                    }
                });
        if (ret.isOk()) {
            return device;
        }
        std::lock_guard<std::mutex> lock(gProviderLock);
        gProvider = nullptr;
    }
    return nullptr;
}

int getNumberOfCameras() {
    std::lock_guard<std::mutex> lock(gProviderLock);
    return loadProviderLocked() ? static_cast<int>(gDeviceNames.size()) : 0;
}

int getCameraInfo(int camera, struct camera_info* info) {
    sp<ICameraDevice> device = getDevice(camera);
    if (device == nullptr) {
        return -ENODEV;
    }
    Status status = Status::INTERNAL_ERROR;
    CameraInfo hidlInfo;
    auto ret = device->getCameraInfo([&](Status s, const CameraInfo& i) {
        status = s;
        hidlInfo = i;
    });
    if (!ret.isOk() || status != Status::OK) {
        return -ENODEV;
    }
    memset(info, 0, sizeof(*info));
    info->facing = static_cast<int>(hidlInfo.facing);
    info->orientation = hidlInfo.orientation;
    info->device_version = CAMERA_DEVICE_API_VERSION_1_0;
    info->resource_cost = 100;
    return 0;
}

int openLegacy(const hw_module_t* module, const char* id, uint32_t halVersion,
               hw_device_t** device) {
    if (id == nullptr || halVersion != CAMERA_DEVICE_API_VERSION_1_0) {
        return -EINVAL;
    }
    sp<ICameraDevice> hidl = getDevice(atoi(id));
    if (hidl == nullptr) {
        return -ENODEV;
    }
    sp<DeviceCallback> callback = new DeviceCallback();
    int rc = toStatusT(hidl->open(callback));
    if (rc != 0) {
        ALOGE("cannot open camera %s: %d", id, rc);
        return rc;
    }
    FacadeDevice* dev = new FacadeDevice();
    dev->base.common.tag = HARDWARE_DEVICE_TAG;
    dev->base.common.version = CAMERA_DEVICE_API_VERSION_1_0;
    dev->base.common.module = const_cast<hw_module_t*>(module);
    dev->base.common.close = closeDevice;
    dev->base.ops = &gOps;
    dev->hidl = hidl;
    dev->callback = callback;
    *device = &dev->base.common;
    ALOGI("camera %s opened", id);
    return 0;
}

int openDevice(const hw_module_t* module, const char* id, hw_device_t** device) {
    return openLegacy(module, id, CAMERA_DEVICE_API_VERSION_1_0, device);
}

hw_module_methods_t gMethods = {openDevice};

camera_module_t makeModule() {
    camera_module_t module = {};
    module.common.tag = HARDWARE_MODULE_TAG;
    module.common.module_api_version = CAMERA_MODULE_API_VERSION_2_4;
    module.common.hal_api_version = HARDWARE_HAL_API_VERSION;
    module.common.id = CAMERA_HARDWARE_MODULE_ID;
    module.common.name = "Sony camera extension facade";
    module.common.author = "kitakami";
    module.common.methods = &gMethods;
    module.get_number_of_cameras = getNumberOfCameras;
    module.get_camera_info = getCameraInfo;
    module.open_legacy = openLegacy;
    return module;
}

camera_module_t gModule = makeModule();

}  // namespace

extern "C" __attribute__((visibility("default"))) int hw_get_module(const char* id,
                                                                    const hw_module_t** module) {
    if (id != nullptr && strcmp(id, CAMERA_HARDWARE_MODULE_ID) == 0) {
        *module = &gModule.common;
        return 0;
    }
    return hw_get_module_by_class(id, nullptr, module);
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    android::hardware::configureRpcThreadpool(4, true);
    std::thread([] { android::hardware::joinRpcThreadpool(); }).detach();

    sp<ProcessState> proc(ProcessState::self());
    void* service = dlopen("libcameraextensionservice.so", RTLD_NOW);
    if (service == nullptr) {
        ALOGE("%s", dlerror());
        return 1;
    }
    auto instantiate = reinterpret_cast<void (*)()>(dlsym(service, "instantiate"));
    if (instantiate == nullptr) {
        ALOGE("%s", dlerror());
        return 1;
    }
    instantiate();
    ALOGI("media.cameraextension published");

    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
    return 0;
}
