#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

static char g_process_name[256] = {0};
static pthread_once_t g_once = PTHREAD_ONCE_INIT;

static void init_process_name(void) {
    int fd = open("/proc/self/cmdline", O_RDONLY);
    if (fd < 0) {
        strcpy(g_process_name, "unknown");
        return;
    }
    ssize_t n = read(fd, g_process_name, sizeof(g_process_name) - 1);
    close(fd);
    if (n <= 0) {
        strcpy(g_process_name, "unknown");
        return;
    }
    g_process_name[n] = 0;
    char* slash = strrchr(g_process_name, '/');
    if (slash) {
        memmove(g_process_name, slash + 1, strlen(slash + 1) + 1);
    }
}

extern "C" const char* get_process_name(void) {
    pthread_once(&g_once, init_process_name);
    return g_process_name;
}
