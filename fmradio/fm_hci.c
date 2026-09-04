#define _GNU_SOURCE
#include "fm_hci.h"

#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define OP_ACQUIRE          0x01
#define OP_RELEASE          0x02
#define OP_COMMAND          0x03
#define OP_EVENT            0x04
#define OP_ACQUIRE_DONE     0x81
#define OP_RELEASE_DONE     0x82

#define STATUS_OK           0x00

#define EVT_CMD_COMPLETE    0x0e
#define EVT_VENDOR          0xff

#define MAX_MESSAGE         260
#define ACQUIRE_TIMEOUT_MS  20000
#define RELEASE_TIMEOUT_MS  2000
#define EVENT_QUEUE_MAX     8

struct pending_event {
    uint8_t data[MAX_MESSAGE];
    int len;
};

struct fm_hci {
    int fd;
    struct fm_transport transport;
    struct pending_event queue[EVENT_QUEUE_MAX];
    int queue_head;
    int queue_count;
};

static int recv_message(struct fm_hci *h, uint8_t *msg, int msgsz, int timeout_ms)
{
    struct pollfd p = { h->fd, POLLIN, 0 };
    int n;

    if (poll(&p, 1, timeout_ms) <= 0)
        return -1;
    n = (int)recv(h->fd, msg, msgsz, 0);
    if (n <= 0)
        return -1;
    return n;
}

static int send_message(struct fm_hci *h, const uint8_t *msg, int len)
{
    return send(h->fd, msg, len, MSG_NOSIGNAL) == len ? 0 : -1;
}

static void queue_push(struct fm_hci *h, const uint8_t *evt, int len)
{
    int idx;

    if (h->queue_count == EVENT_QUEUE_MAX) {
        h->queue_head = (h->queue_head + 1) % EVENT_QUEUE_MAX;
        h->queue_count--;
    }
    idx = (h->queue_head + h->queue_count) % EVENT_QUEUE_MAX;
    if (len > (int)sizeof(h->queue[idx].data))
        len = (int)sizeof(h->queue[idx].data);
    memcpy(h->queue[idx].data, evt, len);
    h->queue[idx].len = len;
    h->queue_count++;
}

static int queue_pop(struct fm_hci *h, uint8_t *out, int outsz)
{
    struct pending_event *e;
    int len;

    if (h->queue_count == 0)
        return -1;
    e = &h->queue[h->queue_head];
    len = e->len > outsz ? outsz : e->len;
    memcpy(out, e->data, len);
    h->queue_head = (h->queue_head + 1) % EVENT_QUEUE_MAX;
    h->queue_count--;
    return len;
}

static int recv_event(struct fm_hci *h, uint8_t *evt, int evtsz, int timeout_ms)
{
    uint8_t msg[MAX_MESSAGE];
    int n;

    for (;;) {
        n = recv_message(h, msg, sizeof(msg), timeout_ms);
        if (n < 2)
            return -1;
        if (msg[0] != OP_EVENT)
            continue;
        n--;
        if (n > evtsz)
            n = evtsz;
        memcpy(evt, msg + 1, n);
        return n;
    }
}

static int transport_vsc(void *priv, const uint8_t *cmd, uint8_t len,
                         uint8_t *resp, int respsz, int timeout_ms)
{
    struct fm_hci *h = (struct fm_hci *)priv;
    uint8_t msg[4 + 255];
    uint8_t evt[MAX_MESSAGE];
    int n, tries;

    msg[0] = OP_COMMAND;
    msg[1] = (uint8_t)(FM_VSC_OPCODE & 0xff);
    msg[2] = (uint8_t)(FM_VSC_OPCODE >> 8);
    msg[3] = len;
    if (len)
        memcpy(msg + 4, cmd, len);
    if (send_message(h, msg, 4 + len) < 0)
        return -1;

    for (tries = 0; tries < 16; tries++) {
        n = recv_event(h, evt, sizeof(evt), timeout_ms);
        if (n < 0)
            return -1;
        if (evt[0] == EVT_CMD_COMPLETE && n >= 5) {
            uint16_t op = (uint16_t)evt[3] | ((uint16_t)evt[4] << 8);
            if (op != FM_VSC_OPCODE)
                continue;
            if (resp) {
                int cp = n - 5 > respsz ? respsz : n - 5;
                memcpy(resp, evt + 5, cp);
                return cp;
            }
            return n - 5;
        }
        if (evt[0] == EVT_VENDOR)
            queue_push(h, evt, n);
    }
    return -1;
}

static int transport_wait_event(void *priv, uint8_t *evt, int evtsz, int timeout_ms)
{
    struct fm_hci *h = (struct fm_hci *)priv;
    int n;

    n = queue_pop(h, evt, evtsz);
    if (n > 0)
        return n;
    return recv_event(h, evt, evtsz, timeout_ms);
}

struct fm_hci *fm_hci_open(const char *path)
{
    struct sockaddr_un addr;
    struct fm_hci *h;
    uint8_t msg[2];

    h = calloc(1, sizeof(*h));
    if (!h)
        return NULL;

    h->fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (h->fd < 0) {
        free(h);
        return NULL;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    if (connect(h->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        goto fail;

    msg[0] = OP_ACQUIRE;
    if (send_message(h, msg, 1) < 0)
        goto fail;
    if (recv_message(h, msg, sizeof(msg), ACQUIRE_TIMEOUT_MS) != 2)
        goto fail;
    if (msg[0] != OP_ACQUIRE_DONE || msg[1] != STATUS_OK)
        goto fail;

    h->transport.priv = h;
    h->transport.vsc = transport_vsc;
    h->transport.wait_event = transport_wait_event;
    return h;

fail:
    close(h->fd);
    free(h);
    return NULL;
}

void fm_hci_close(struct fm_hci *h)
{
    uint8_t msg[2];

    if (!h)
        return;
    msg[0] = OP_RELEASE;
    if (send_message(h, msg, 1) == 0)
        recv_message(h, msg, sizeof(msg), RELEASE_TIMEOUT_MS);
    close(h->fd);
    free(h);
}

struct fm_transport *fm_hci_transport(struct fm_hci *h)
{
    return &h->transport;
}
