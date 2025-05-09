
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

namespace android {
    //android::String16::String16(char const*)
    static void _ZN7android8String16C1EPKc(void **str16P, const char *str);

    //android::String16::~String16()
    static void _ZN7android8String16D1Ev(void **str16P);

    //android::SensorManager::createEventQueue(android::String8, int, android::String16)
    static void _ZN7android13SensorManager16createEventQueueENS_7String8EiNS_8String16E(void **retVal, void *sensorMgr, void **str8P, int mode, void **str16P);

    //android::SensorManager::createEventQueue(android::String8, int)
    static void _ZN7android13SensorManager16createEventQueueENS_7String8Ei(void **retVal, void *sensorMgr, void **str8P, int mode)
    {
        void *string;

        _ZN7android8String16C1EPKc(&string, "");
        _ZN7android13SensorManager16createEventQueueENS_7String8EiNS_8String16E(retVal, sensorMgr, str8P, mode, &string);
        _ZN7android8String16D1Ev(&string);
    }

}
