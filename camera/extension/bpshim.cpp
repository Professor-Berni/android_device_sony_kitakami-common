#define LOG_TAG "sonycamera_bp"

#include <string.h>

#include <binder/IInterface.h>
#include <binder/IMemory.h>
#include <binder/Parcel.h>
#include <log/log.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <utils/Timers.h>

using android::BpInterface;
using android::IBinder;
using android::IInterface;
using android::IMemory;
using android::Parcel;
using android::sp;
using android::status_t;
using android::String16;
using android::String8;

namespace {

constexpr size_t kFaceSize = 100;

struct FrameMetadata {
    int32_t number_of_faces;
    void* faces;
    int32_t extra;
};

const String16& clientDescriptor() {
    static const String16 descriptor("com.sonyericsson.cameraextension.ICameraExtensionClient");
    return descriptor;
}

const String16& extensionDescriptor() {
    static const String16 descriptor("com.sonyericsson.cameraextension.ICameraExtension");
    return descriptor;
}

const String16& serviceDescriptor() {
    static const String16 descriptor("com.sonyericsson.cameraextension.ICameraExtensionService");
    return descriptor;
}

template <typename Proxy>
sp<IInterface> asProxy(const sp<IBinder>& obj, const String16& descriptor) {
    if (obj == nullptr) {
        return nullptr;
    }
    sp<IInterface> local = obj->queryLocalInterface(descriptor);
    if (local != nullptr) {
        return local;
    }
    return sp<Proxy>::make(obj);
}

class IClient : public IInterface {
  public:
    virtual const String16& getInterfaceDescriptor() const { return clientDescriptor(); }
    virtual void notifyCallback(int32_t msgType, int32_t ext1, int32_t ext2) = 0;
    virtual void dataCallback(int32_t msgType, const sp<IMemory>& mem, FrameMetadata* metadata) = 0;
    virtual void dataCallbackTimestamp(nsecs_t timestamp, int32_t msgType,
                                       const sp<IMemory>& mem) = 0;
};

class IExtension : public IInterface {
  public:
    virtual const String16& getInterfaceDescriptor() const { return extensionDescriptor(); }
    virtual void disconnect() = 0;
    virtual String8 getParameters() const = 0;
    virtual status_t connect(const sp<IInterface>& client) = 0;
    virtual status_t sendCommand(int32_t cmd, int32_t arg1, int32_t arg2) = 0;
    virtual status_t releaseFrameBuffer(const sp<IMemory>& mem) = 0;
};

class IService : public IInterface {
  public:
    virtual const String16& getInterfaceDescriptor() const { return serviceDescriptor(); }
    virtual int32_t getNumberOfCameras() = 0;
    virtual status_t connect(const sp<IInterface>& client, int32_t cameraId,
                             const String16& clientPackageName, int32_t clientUid,
                             sp<IInterface>& device) = 0;
};

class BpClient : public BpInterface<IClient> {
  public:
    explicit BpClient(const sp<IBinder>& impl) : BpInterface<IClient>(impl) {}

    void notifyCallback(int32_t msgType, int32_t ext1, int32_t ext2) override {
        Parcel data, reply;
        data.writeInterfaceToken(clientDescriptor());
        data.writeInt32(msgType);
        data.writeInt32(ext1);
        data.writeInt32(ext2);
        remote()->transact(1, data, &reply, IBinder::FLAG_ONEWAY);
    }

    void dataCallback(int32_t msgType, const sp<IMemory>& mem, FrameMetadata* metadata) override {
        Parcel data, reply;
        data.writeInterfaceToken(clientDescriptor());
        data.writeInt32(msgType);
        data.writeStrongBinder(IInterface::asBinder(mem));
        if (metadata != nullptr) {
            int32_t faces = metadata->faces != nullptr ? metadata->number_of_faces : 0;
            data.writeInt32(metadata->extra);
            data.writeInt32(faces);
            if (faces > 0) {
                data.write(metadata->faces, faces * kFaceSize);
            }
        }
        remote()->transact(2, data, &reply, IBinder::FLAG_ONEWAY);
    }

    void dataCallbackTimestamp(nsecs_t timestamp, int32_t msgType,
                               const sp<IMemory>& mem) override {
        Parcel data, reply;
        data.writeInterfaceToken(clientDescriptor());
        data.writeInt64(timestamp);
        data.writeInt32(msgType);
        data.writeStrongBinder(IInterface::asBinder(mem));
        remote()->transact(3, data, &reply, IBinder::FLAG_ONEWAY);
    }
};

class BpExtension : public BpInterface<IExtension> {
  public:
    explicit BpExtension(const sp<IBinder>& impl) : BpInterface<IExtension>(impl) {}

    void disconnect() override {
        Parcel data, reply;
        data.writeInterfaceToken(extensionDescriptor());
        remote()->transact(1, data, &reply);
    }

    String8 getParameters() const override {
        Parcel data, reply;
        data.writeInterfaceToken(extensionDescriptor());
        if (remote()->transact(2, data, &reply) != android::OK) {
            return String8();
        }
        return reply.readString8();
    }

    status_t connect(const sp<IInterface>& client) override {
        Parcel data, reply;
        data.writeInterfaceToken(extensionDescriptor());
        data.writeStrongBinder(IInterface::asBinder(client));
        status_t status = remote()->transact(4, data, &reply);
        return status != android::OK ? status : reply.readInt32();
    }

    status_t sendCommand(int32_t cmd, int32_t arg1, int32_t arg2) override {
        Parcel data, reply;
        data.writeInterfaceToken(extensionDescriptor());
        data.writeInt32(cmd);
        data.writeInt32(arg1);
        data.writeInt32(arg2);
        status_t status = remote()->transact(3, data, &reply);
        return status != android::OK ? status : reply.readInt32();
    }

    status_t releaseFrameBuffer(const sp<IMemory>& mem) override {
        Parcel data, reply;
        data.writeInterfaceToken(extensionDescriptor());
        data.writeStrongBinder(IInterface::asBinder(mem));
        status_t status = remote()->transact(5, data, &reply);
        return status != android::OK ? status : reply.readInt32();
    }
};

class BpService : public BpInterface<IService> {
  public:
    explicit BpService(const sp<IBinder>& impl) : BpInterface<IService>(impl) {}

    int32_t getNumberOfCameras() override {
        Parcel data, reply;
        data.writeInterfaceToken(serviceDescriptor());
        if (remote()->transact(1, data, &reply) != android::OK || reply.readExceptionCode() != 0) {
            return 0;
        }
        return reply.readInt32();
    }

    status_t connect(const sp<IInterface>& client, int32_t cameraId,
                     const String16& clientPackageName, int32_t clientUid,
                     sp<IInterface>& device) override {
        Parcel data, reply;
        data.writeInterfaceToken(serviceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(client));
        data.writeInt32(cameraId);
        data.writeString16(clientPackageName);
        data.writeInt32(clientUid);
        status_t status = remote()->transact(2, data, &reply);
        if (status != android::OK) {
            return status;
        }
        int32_t exception = reply.readExceptionCode();
        if (exception != 0) {
            ALOGE("%s: remote exception %d", __func__, exception);
            return -EPROTO;
        }
        status = reply.readInt32();
        if (reply.readInt32() != 0) {
            device = asProxy<BpExtension>(reply.readStrongBinder(), extensionDescriptor());
        }
        return status;
    }
};

}  // namespace

__attribute__((visibility("default"))) sp<IInterface> clientAsInterface(const sp<IBinder>& obj)
        __asm__("_ZN7android22ICameraExtensionClient11asInterfaceERKNS_2spINS_7IBinderEEE");
__attribute__((visibility("default"))) sp<IInterface> extensionAsInterface(const sp<IBinder>& obj)
        __asm__("_ZN7android16ICameraExtension11asInterfaceERKNS_2spINS_7IBinderEEE");
__attribute__((visibility("default"))) sp<IInterface> serviceAsInterface(const sp<IBinder>& obj)
        __asm__("_ZN7android23ICameraExtensionService11asInterfaceERKNS_2spINS_7IBinderEEE");

sp<IInterface> clientAsInterface(const sp<IBinder>& obj) {
    return asProxy<BpClient>(obj, clientDescriptor());
}

sp<IInterface> extensionAsInterface(const sp<IBinder>& obj) {
    return asProxy<BpExtension>(obj, extensionDescriptor());
}

sp<IInterface> serviceAsInterface(const sp<IBinder>& obj) {
    return asProxy<BpService>(obj, serviceDescriptor());
}
