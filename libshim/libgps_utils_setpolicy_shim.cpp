#include <dlfcn.h>

/* set_sched_policy */

extern "C" int set_sched_policy(int tid, int policy) {
    using real_fn_t = int (*)(int, int);
    static real_fn_t real_set_sched_policy = nullptr;
    static bool resolved = false;

    if (!resolved) {
        resolved = true;
        void* h = dlopen("libprocessgroup.so", RTLD_NOW | RTLD_GLOBAL);
        if (h) {
            real_set_sched_policy =
                reinterpret_cast<real_fn_t>(dlsym(h, "set_sched_policy"));
        }
    }

    if (real_set_sched_policy) {
        return real_set_sched_policy(tid, policy);
    }
    return 0;
}

/* reportSv(QcomSvStatus) → forward to GnssSvStatus impl */

using reportSv_fn_t = void (*)(void*, void*, void*, void*);

static reportSv_fn_t resolve_reportSv(const char* mangled_name) {
    static thread_local bool recursing = false;
    if (recursing) return nullptr;
    recursing = true;
    auto fn = reinterpret_cast<reportSv_fn_t>(dlsym(RTLD_DEFAULT, mangled_name));
    recursing = false;
    return fn;
}

extern "C" void shim_LocApiBase_reportSv_qcom(
    void* this_, void* svStatus, void* locExtended, void* svExt)
    asm("_ZN8loc_core10LocApiBase8reportSvER12QcomSvStatusR19GpsLocationExtendedPv");

void shim_LocApiBase_reportSv_qcom(
        void* this_, void* svStatus, void* locExtended, void* svExt) {
    static reportSv_fn_t real_fn = nullptr;
    if (!real_fn) {
        real_fn = resolve_reportSv(
            "_ZN8loc_core10LocApiBase8reportSvER12GnssSvStatusR19GpsLocationExtendedPv");
    }
    if (real_fn) real_fn(this_, svStatus, locExtended, svExt);
}

extern "C" void shim_LocAdapterBase_reportSv_qcom(
    void* this_, void* svStatus, void* locExtended, void* svExt)
    asm("_ZN8loc_core14LocAdapterBase8reportSvER12QcomSvStatusR19GpsLocationExtendedPv");

void shim_LocAdapterBase_reportSv_qcom(
        void* this_, void* svStatus, void* locExtended, void* svExt) {
    static reportSv_fn_t real_fn = nullptr;
    if (!real_fn) {
        real_fn = resolve_reportSv(
            "_ZN8loc_core14LocAdapterBase8reportSvER12GnssSvStatusR19GpsLocationExtendedPv");
    }
    if (real_fn) real_fn(this_, svStatus, locExtended, svExt);
}

/* reportGpsMeasurementData(GpsData) no-op stubs */

extern "C" void shim_LocApiBase_reportGpsMeasurementData(void*, void*)
    asm("_ZN8loc_core10LocApiBase24reportGpsMeasurementDataER7GpsData");

extern "C" void shim_LocAdapterBase_reportGpsMeasurementData(void*, void*)
    asm("_ZN8loc_core14LocAdapterBase24reportGpsMeasurementDataER7GpsData");

void shim_LocApiBase_reportGpsMeasurementData(void*, void*) {}
void shim_LocAdapterBase_reportGpsMeasurementData(void*, void*) {}
