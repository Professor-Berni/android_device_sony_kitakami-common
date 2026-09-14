#define LOG_TAG "cammw_sensor_shim"

#include <aidl/android/frameworks/sensorservice/BnEventQueueCallback.h>
#include <aidl/android/frameworks/sensorservice/IEventQueue.h>
#include <aidl/android/frameworks/sensorservice/ISensorManager.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>

#include <errno.h>
#include <time.h>

#include <algorithm>
#include <deque>
#include <mutex>
#include <string>

using aidl::android::frameworks::sensorservice::BnEventQueueCallback;
using aidl::android::frameworks::sensorservice::IEventQueue;
using aidl::android::frameworks::sensorservice::ISensorManager;
using aidl::android::hardware::sensors::Event;
using aidl::android::hardware::sensors::SensorInfo;
using aidl::android::hardware::sensors::SensorType;

namespace {

enum CammwSensorId : int32_t {
    kLight = 0,
    kGyro = 1,
    kAccel = 2,
};

constexpr size_t kMaxSamples = 201;

struct Sample {
    int64_t timestamp;
    float value[3];
};

struct MotionData {
    int32_t id;
    int32_t reserved;
    int64_t timestamp;
    float value[3];
    int32_t reserved2;
};
static_assert(sizeof(MotionData) == 0x20);

struct LightData {
    int32_t id;
    int32_t reserved;
    int64_t timestamp;
    float lux;
    int32_t reserved2;
};
static_assert(sizeof(LightData) == 0x18);

int64_t clockNs(clockid_t clock) {
    timespec ts{};
    clock_gettime(clock, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

class EventSink : public BnEventQueueCallback {
  public:
    explicit EventSink(SensorType type) : mType(type) {}

    ndk::ScopedAStatus onEvent(const Event& event) override {
        if (event.sensorType != mType) {
            return ndk::ScopedAStatus::ok();
        }

        Sample sample{};
        switch (event.payload.getTag()) {
            case Event::EventPayload::Tag::vec3: {
                const auto& vec3 = event.payload.get<Event::EventPayload::Tag::vec3>();
                sample.value[0] = vec3.x;
                sample.value[1] = vec3.y;
                sample.value[2] = vec3.z;
                break;
            }
            case Event::EventPayload::Tag::scalar:
                sample.value[0] = event.payload.get<Event::EventPayload::Tag::scalar>();
                break;
            default:
                return ndk::ScopedAStatus::ok();
        }
        const int64_t boottime = clockNs(CLOCK_BOOTTIME);
        sample.timestamp = event.timestamp - boottime + clockNs(CLOCK_MONOTONIC);

        std::lock_guard<std::mutex> lock(mLock);
        if (mSamples.size() >= kMaxSamples) {
            mSamples.pop_back();
        }
        mSamples.push_front(sample);
        return ndk::ScopedAStatus::ok();
    }

    uint32_t copyNewest(int32_t id, uint32_t max, void* out) {
        std::lock_guard<std::mutex> lock(mLock);
        const uint32_t count = std::min<size_t>(max, mSamples.size());
        for (uint32_t i = 0; i < count; i++) {
            const Sample& s = mSamples[i];
            if (id == kLight) {
                static_cast<LightData*>(out)[i] = {id, 0, s.timestamp, s.value[0], 0};
            } else {
                static_cast<MotionData*>(out)[i] = {
                        id, 0, s.timestamp, {s.value[0], s.value[1], s.value[2]}, 0};
            }
        }
        return count;
    }

  private:
    const SensorType mType;
    std::mutex mLock;
    std::deque<Sample> mSamples;
};

std::shared_ptr<ISensorManager> getSensorManager() {
    static std::mutex lock;
    static std::shared_ptr<ISensorManager> manager;

    std::lock_guard<std::mutex> guard(lock);
    if (manager == nullptr || !AIBinder_isAlive(manager->asBinder().get())) {
        if (!ABinderProcess_isThreadPoolStarted()) {
            ABinderProcess_startThreadPool();
        }
        const std::string name = std::string(ISensorManager::descriptor) + "/default";
        manager = ISensorManager::fromBinder(
                ndk::SpAIBinder(AServiceManager_checkService(name.c_str())));
    }
    return manager;
}

class CammwSensor {
  public:
    explicit CammwSensor(int32_t id) : mId(id) {}
    ~CammwSensor() { stop(); }

    bool start(uint32_t periodMs) {
        SensorType type;
        switch (mId) {
            case kLight:
                type = SensorType::LIGHT;
                break;
            case kGyro:
                type = SensorType::GYROSCOPE;
                break;
            case kAccel:
                type = SensorType::ACCELEROMETER;
                break;
            default:
                return false;
        }

        std::lock_guard<std::mutex> lock(mLock);
        if (mQueue != nullptr) {
            return false;
        }
        std::shared_ptr<ISensorManager> manager = getSensorManager();
        if (manager == nullptr) {
            ALOGE("%s/default unavailable", ISensorManager::descriptor);
            return false;
        }
        SensorInfo info;
        ndk::ScopedAStatus status = manager->getDefaultSensor(type, &info);
        if (!status.isOk()) {
            ALOGE("getDefaultSensor(%d): %s", static_cast<int32_t>(type),
                  status.getDescription().c_str());
            return false;
        }
        std::shared_ptr<EventSink> sink = ndk::SharedRefBase::make<EventSink>(type);
        std::shared_ptr<IEventQueue> queue;
        status = manager->createEventQueue(sink, &queue);
        if (!status.isOk() || queue == nullptr) {
            ALOGE("createEventQueue: %s", status.getDescription().c_str());
            return false;
        }
        status = queue->enableSensor(info.sensorHandle, static_cast<int32_t>(periodMs * 1000), 0);
        if (!status.isOk()) {
            ALOGE("enableSensor(%s): %s", info.name.c_str(), status.getDescription().c_str());
            return false;
        }
        ALOGI("%s started, %u ms", info.name.c_str(), periodMs);
        mHandle = info.sensorHandle;
        mQueue = queue;
        mSink = sink;
        return true;
    }

    void stop() {
        std::lock_guard<std::mutex> lock(mLock);
        if (mQueue == nullptr) {
            return;
        }
        ndk::ScopedAStatus status = mQueue->disableSensor(mHandle);
        if (!status.isOk()) {
            ALOGW("disableSensor(%d): %s", mHandle, status.getDescription().c_str());
        }
        mQueue.reset();
        mSink.reset();
    }

    bool getData(uint32_t max, void* out, uint32_t* count) {
        std::shared_ptr<EventSink> sink;
        {
            std::lock_guard<std::mutex> lock(mLock);
            sink = mSink;
        }
        if (sink == nullptr || max == 0 || out == nullptr || count == nullptr) {
            return false;
        }
        *count = sink->copyNewest(mId, max, out);
        return true;
    }

  private:
    const int32_t mId;
    std::mutex mLock;
    std::shared_ptr<IEventQueue> mQueue;
    std::shared_ptr<EventSink> mSink;
    int32_t mHandle = 0;
};

}  // namespace

extern "C" __attribute__((visibility("default")))
void* cammw_sensor_util_dev_open(int32_t id) {
    return new CammwSensor(id);
}

extern "C" __attribute__((visibility("default")))
int32_t cammw_sensor_util_dev_start(void* dev, uint32_t periodMs) {
    if (dev == nullptr) {
        return 0;
    }
    return static_cast<CammwSensor*>(dev)->start(periodMs) ? 0 : -ENOTCONN;
}

extern "C" __attribute__((visibility("default")))
int32_t cammw_sensor_util_dev_stop(void* dev) {
    if (dev != nullptr) {
        static_cast<CammwSensor*>(dev)->stop();
    }
    return 0;
}

extern "C" __attribute__((visibility("default")))
int32_t cammw_sensor_util_dev_close(void* dev) {
    delete static_cast<CammwSensor*>(dev);
    return 0;
}

extern "C" __attribute__((visibility("default")))
int32_t cammw_sensor_util_get_data(void* dev, uint32_t max, void* out, uint32_t* count) {
    if (dev == nullptr) {
        return 0;
    }
    return static_cast<CammwSensor*>(dev)->getData(max, out, count) ? 0 : -ECONNREFUSED;
}
