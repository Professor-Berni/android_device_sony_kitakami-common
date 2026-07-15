#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

namespace android {

class IBinder;

class IPowerManager : public virtual RefBase {
  public:
    static sp<IPowerManager> asInterface(const sp<IBinder>& obj);
};

sp<IPowerManager> IPowerManager::asInterface(const sp<IBinder>& /*obj*/) {
    return sp<IPowerManager>();
}

}  // namespace android
