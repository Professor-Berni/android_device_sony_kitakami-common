#include <stddef.h>
#include <stdint.h>

struct cbs_st {
    const uint8_t *data;
    size_t len;
};

void CBS_init(struct cbs_st *cbs, const uint8_t *data, size_t len) {
    cbs->data = data;
    cbs->len = len;
}
