#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

struct HMAC_CTX_real;
struct EVP_CIPHER_CTX_real;
struct EVP_MD;
struct EVP_CIPHER;
struct ENGINE;

static inline void** ptr_slot(void* ctx_buf) {
    return reinterpret_cast<void**>(ctx_buf);
}

static inline void* get_real(void* ctx_buf) {
    return *ptr_slot(ctx_buf);
}

static inline void set_real(void* ctx_buf, void* real) {
    *ptr_slot(ctx_buf) = real;
}

#define DECLARE_REAL(name, ret, ...) \
    typedef ret (*name##_t)(__VA_ARGS__); \
    static name##_t real_##name = nullptr;

#define LOAD_REAL(name) \
    do { if (!real_##name) real_##name = (name##_t)dlsym(RTLD_NEXT, #name); } while (0)

DECLARE_REAL(HMAC_CTX_new,      void*)
DECLARE_REAL(HMAC_CTX_free,     void, void*)
DECLARE_REAL(HMAC_CTX_reset,    int, void*)
DECLARE_REAL(HMAC_Init_ex,      int, void*, const void*, int, const EVP_MD*, ENGINE*)
DECLARE_REAL(HMAC_Update,       int, void*, const unsigned char*, size_t)
DECLARE_REAL(HMAC_Final,        int, void*, unsigned char*, unsigned int*)

DECLARE_REAL(EVP_CIPHER_CTX_new,      void*)
DECLARE_REAL(EVP_CIPHER_CTX_free,     void, void*)
DECLARE_REAL(EVP_CIPHER_CTX_reset,    int, void*)
DECLARE_REAL(EVP_EncryptInit_ex,      int, void*, const EVP_CIPHER*, ENGINE*, const unsigned char*, const unsigned char*)
DECLARE_REAL(EVP_EncryptUpdate,       int, void*, unsigned char*, int*, const unsigned char*, int)
DECLARE_REAL(EVP_EncryptFinal_ex,     int, void*, unsigned char*, int*)
DECLARE_REAL(EVP_DecryptInit_ex,      int, void*, const EVP_CIPHER*, ENGINE*, const unsigned char*, const unsigned char*)
DECLARE_REAL(EVP_DecryptUpdate,       int, void*, unsigned char*, int*, const unsigned char*, int)
DECLARE_REAL(EVP_DecryptFinal_ex,     int, void*, unsigned char*, int*)

extern "C" {

// ---- HMAC ----

__attribute__((visibility("default")))
void HMAC_CTX_init(void* ctx_buf) {
    LOAD_REAL(HMAC_CTX_new);
    set_real(ctx_buf, real_HMAC_CTX_new());
}

__attribute__((visibility("default")))
int HMAC_Init_ex(void* ctx_buf, const void* key, int key_len,
                 const EVP_MD* md, ENGINE* impl) {
    LOAD_REAL(HMAC_Init_ex);
    LOAD_REAL(HMAC_CTX_new);
    LOAD_REAL(HMAC_CTX_reset);
    void* real = get_real(ctx_buf);
    if (!real) {
        real = real_HMAC_CTX_new();
        set_real(ctx_buf, real);
    } else if (real_HMAC_CTX_reset) {
        real_HMAC_CTX_reset(real);
    }
    return real_HMAC_Init_ex(real, key, key_len, md, impl);
}

__attribute__((visibility("default")))
int HMAC_Update(void* ctx_buf, const unsigned char* data, size_t len) {
    LOAD_REAL(HMAC_Update);
    return real_HMAC_Update(get_real(ctx_buf), data, len);
}

__attribute__((visibility("default")))
int HMAC_Final(void* ctx_buf, unsigned char* md, unsigned int* len) {
    LOAD_REAL(HMAC_Final);
    return real_HMAC_Final(get_real(ctx_buf), md, len);
}

__attribute__((visibility("default")))
void HMAC_CTX_cleanup(void* ctx_buf) {
    LOAD_REAL(HMAC_CTX_free);
    void* real = get_real(ctx_buf);
    if (real) {
        real_HMAC_CTX_free(real);
        set_real(ctx_buf, nullptr);
    }
}

// ---- EVP_CIPHER ----

__attribute__((visibility("default")))
void EVP_CIPHER_CTX_init(void* ctx_buf) {
    LOAD_REAL(EVP_CIPHER_CTX_new);
    set_real(ctx_buf, real_EVP_CIPHER_CTX_new());
}

__attribute__((visibility("default")))
int EVP_EncryptInit_ex(void* ctx_buf, const EVP_CIPHER* cipher,
                       ENGINE* impl, const unsigned char* key,
                       const unsigned char* iv) {
    LOAD_REAL(EVP_EncryptInit_ex);
    LOAD_REAL(EVP_CIPHER_CTX_new);
    LOAD_REAL(EVP_CIPHER_CTX_reset);
    void* real = get_real(ctx_buf);
    if (!real) {
        real = real_EVP_CIPHER_CTX_new();
        set_real(ctx_buf, real);
    } else if (real_EVP_CIPHER_CTX_reset) {
        real_EVP_CIPHER_CTX_reset(real);
    }
    return real_EVP_EncryptInit_ex(real, cipher, impl, key, iv);
}

__attribute__((visibility("default")))
int EVP_EncryptUpdate(void* ctx_buf, unsigned char* out, int* outl,
                      const unsigned char* in, int inl) {
    LOAD_REAL(EVP_EncryptUpdate);
    return real_EVP_EncryptUpdate(get_real(ctx_buf), out, outl, in, inl);
}

__attribute__((visibility("default")))
int EVP_EncryptFinal_ex(void* ctx_buf, unsigned char* out, int* outl) {
    LOAD_REAL(EVP_EncryptFinal_ex);
    return real_EVP_EncryptFinal_ex(get_real(ctx_buf), out, outl);
}

__attribute__((visibility("default")))
int EVP_DecryptInit_ex(void* ctx_buf, const EVP_CIPHER* cipher,
                       ENGINE* impl, const unsigned char* key,
                       const unsigned char* iv) {
    LOAD_REAL(EVP_DecryptInit_ex);
    LOAD_REAL(EVP_CIPHER_CTX_new);
    LOAD_REAL(EVP_CIPHER_CTX_reset);
    void* real = get_real(ctx_buf);
    if (!real) {
        real = real_EVP_CIPHER_CTX_new();
        set_real(ctx_buf, real);
    } else if (real_EVP_CIPHER_CTX_reset) {
        real_EVP_CIPHER_CTX_reset(real);
    }
    return real_EVP_DecryptInit_ex(real, cipher, impl, key, iv);
}

__attribute__((visibility("default")))
int EVP_DecryptUpdate(void* ctx_buf, unsigned char* out, int* outl,
                      const unsigned char* in, int inl) {
    LOAD_REAL(EVP_DecryptUpdate);
    return real_EVP_DecryptUpdate(get_real(ctx_buf), out, outl, in, inl);
}

__attribute__((visibility("default")))
int EVP_DecryptFinal_ex(void* ctx_buf, unsigned char* out, int* outl) {
    LOAD_REAL(EVP_DecryptFinal_ex);
    return real_EVP_DecryptFinal_ex(get_real(ctx_buf), out, outl);
}

__attribute__((visibility("default")))
void EVP_CIPHER_CTX_cleanup(void* ctx_buf) {
    LOAD_REAL(EVP_CIPHER_CTX_free);
    void* real = get_real(ctx_buf);
    if (real) {
        real_EVP_CIPHER_CTX_free(real);
        set_real(ctx_buf, nullptr);
    }
}

} // extern "C"
