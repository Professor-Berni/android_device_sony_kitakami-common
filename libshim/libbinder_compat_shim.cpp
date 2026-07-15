#include <utils/String16.h>

namespace android {

extern "C" __attribute__((visibility("default")))
bool _ZN7android22checkCallingPermissionERKNS_8String16E(const String16& /*permission*/);

bool _ZN7android22checkCallingPermissionERKNS_8String16E(const String16& /*permission*/) {
    return true;
}

extern "C" __attribute__((visibility("default")))
bool _ZN7android22checkCallingPermissionERKNS_8String16EPiS3_(const String16& /*permission*/,
                                                                int32_t* outPid, int32_t* outUid);

bool _ZN7android22checkCallingPermissionERKNS_8String16EPiS3_(const String16& /*permission*/,
                                                                int32_t* outPid, int32_t* outUid) {
    if (outPid) *outPid = 0;
    if (outUid) *outUid = 0;
    return true;
}

} // namespace android
