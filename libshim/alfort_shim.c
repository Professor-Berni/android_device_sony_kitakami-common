/* Alfort ships its shaders as 2015 v0x10002 Adreno program binaries that the
 * 2017 driver rejects, so translate them on the way in. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>

#define TAG "z5p_alfort"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define GL_LINK_STATUS 0x8B82

/* ckb::CkbPalBuf::createInstance news 0x88 bytes for an android::GraphicBuffer,
 * the Android 7 size. It is 0x6f8 here, so the constructor smashes the heap. */
#define CKB_LEGACY_GB_SIZE 0x88
#define CKB_GB_SAFE_SIZE   4096

typedef void *(*newop_t)(size_t);

static newop_t real_new;
static int resolving_new;

static newop_t get_real_new(void)
{
    void *h;

    if (real_new || resolving_new) return real_new;
    resolving_new = 1;
    h = dlopen("libc++.so", RTLD_NOW);
    if (h) real_new = (newop_t)dlsym(h, "_Znwj");
    resolving_new = 0;
    return real_new;
}

static int from_chokoballpal(void *ra)
{
    Dl_info i;

    return dladdr(ra, &i) && i.dli_fname && strstr(i.dli_fname, "chokoballpal");
}

void *_Znwj(size_t n)
{
    static int logged;
    newop_t f;

    if (n == CKB_LEGACY_GB_SIZE && from_chokoballpal(__builtin_return_address(0))) {
        if (!logged) {
            logged = 1;
            LOGI("CkbPalBuf asks %u B for a %u B GraphicBuffer, giving it %u",
                 (unsigned)n, 0x6f8u, (unsigned)CKB_GB_SAFE_SIZE);
        }
        n = CKB_GB_SAFE_SIZE;
    }
    f = get_real_new();
    return f ? f(n) : malloc(n);
}

int translate_v2_to_v7(const unsigned char *in, int len, unsigned char **out, int *outlen);

typedef void (*pb_t)(unsigned, unsigned, const void *, int);
typedef void (*gpiv_t)(unsigned, unsigned, int *);

static pb_t real_pb;
static gpiv_t real_gpiv;

/* RTLD_NEXT would leave the vendor namespace, so go through the driver by name. */
static void load_real(void)
{
    void *h = dlopen("libGLESv3.so", RTLD_NOW);

    if (!h) h = dlopen("libGLESv2.so", RTLD_NOW);
    if (!h) { LOGE("cannot dlopen the GLES driver: %s", dlerror()); return; }
    real_pb = (pb_t)dlsym(h, "glProgramBinary");
    real_gpiv = (gpiv_t)dlsym(h, "glGetProgramiv");
    if (!real_pb) LOGE("cannot resolve glProgramBinary");
}

static int is_sony_v2(const unsigned char *b, int len)
{
    return len >= 0x50 && b &&
           b[0] == 0xce && b[1] == 0xca && b[2] == 0x0b && b[3] == 0xb1 &&
           b[4] == 0x02 && b[5] == 0x00 && b[6] == 0x01 && b[7] == 0x00;
}

__attribute__((visibility("default")))
void glProgramBinary(unsigned prog, unsigned fmt, const void *bin, int len)
{
    static unsigned done;
    const unsigned char *in = (const unsigned char *)bin;
    unsigned char *out = NULL;
    int outlen = 0, rc, link = -1;

    if (!real_pb) load_real();
    if (!real_pb) return;

    if (!is_sony_v2(in, len)) {
        real_pb(prog, fmt, bin, len);
        return;
    }

    rc = translate_v2_to_v7(in, len, &out, &outlen);
    if (rc != 0 || !out) {
        LOGE("prog %u: translate failed rc=%d (%d bytes), passing original through",
             prog, rc, len);
        real_pb(prog, fmt, bin, len);
        return;
    }

    real_pb(prog, fmt, out, outlen);
    free(out);

    if (real_gpiv) real_gpiv(prog, GL_LINK_STATUS, &link);
    if (link != 1) LOGE("prog %u: v2 %d B -> v7 %d B, LINK_STATUS=%d", prog, len, outlen, link);
    else if (++done % 25 == 0) LOGI("%u program binaries translated", done);
}
