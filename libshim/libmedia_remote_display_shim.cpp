#include <stdint.h>

#define UNKNOWN_TRANSACTION 17

extern "C" int32_t remote_display_on_transact_stub(
        void* /*this*/,
        uint32_t /*code*/,
        const void* /*data Parcel*/,
        void* /*reply Parcel*/,
        uint32_t /*flags*/) {
    return UNKNOWN_TRANSACTION;
}

extern "C" int32_t _ZN7android21BnRemoteDisplayClient10onTransactEjRKNS_6ParcelEPS1_j(
        void*, uint32_t, const void*, void*, uint32_t)
    __attribute__((alias("remote_display_on_transact_stub")));

extern "C" int32_t _ZThn8_N7android21BnRemoteDisplayClient10onTransactEjRKNS_6ParcelEPS1_j(
        void*, uint32_t, const void*, void*, uint32_t)
    __attribute__((alias("remote_display_on_transact_stub")));
