#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>
#include <signal.h>

extern "C" {

static void drop_invalid_fds(struct pollfd* fds, nfds_t nfds) {
    if (!fds) return;
    for (nfds_t i = 0; i < nfds; i++) {
        if (fds[i].fd >= 0 && fcntl(fds[i].fd, F_GETFD) == -1 && errno == EBADF) {
            fds[i].fd = -1;
        }
    }
}

int ppoll(struct pollfd* fds, nfds_t nfds, const struct timespec* timeout,
          const sigset_t* sigmask) {
    static int (*real_ppoll)(struct pollfd*, nfds_t, const struct timespec*,
                             const sigset_t*) = nullptr;
    if (!real_ppoll) {
        real_ppoll = reinterpret_cast<int (*)(struct pollfd*, nfds_t,
                     const struct timespec*, const sigset_t*)>(
                     dlsym(RTLD_NEXT, "ppoll"));
    }
    drop_invalid_fds(fds, nfds);
    return real_ppoll(fds, nfds, timeout, sigmask);
}

int poll(struct pollfd* fds, nfds_t nfds, int timeout) {
    static int (*real_poll)(struct pollfd*, nfds_t, int) = nullptr;
    if (!real_poll) {
        real_poll = reinterpret_cast<int (*)(struct pollfd*, nfds_t, int)>(
                    dlsym(RTLD_NEXT, "poll"));
    }
    drop_invalid_fds(fds, nfds);
    return real_poll(fds, nfds, timeout);
}

}
