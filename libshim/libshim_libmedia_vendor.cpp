#include <utils/String8.h>
#include <utils/Errors.h>

#define LOG_TAG "libmedia-stub"
#include <log/log.h>

namespace android {

typedef int audio_io_handle_t;
typedef void (*audio_error_callback)(status_t err);

class AudioSystem {
public:
    static String8 getParameters(audio_io_handle_t ioHandle, const String8& keys);

    static status_t setParameters(audio_io_handle_t ioHandle, const String8& keyValuePairs);

    static void setErrorCallback(audio_error_callback cb);
};

String8 AudioSystem::getParameters(audio_io_handle_t /*ioHandle*/,
                                    const String8& /*keys*/) {
    ALOGV("AudioSystem::getParameters stub called");
    return String8("");
}

status_t AudioSystem::setParameters(audio_io_handle_t /*ioHandle*/,
                                     const String8& /*keyValuePairs*/) {
    ALOGV("AudioSystem::setParameters stub called");
    return NO_ERROR;
}

void AudioSystem::setErrorCallback(audio_error_callback /*cb*/) {
    ALOGV("AudioSystem::setErrorCallback stub called (ignored)");
}

} // namespace android
