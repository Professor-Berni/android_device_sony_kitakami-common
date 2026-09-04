#define _GNU_SOURCE
#include "fm_uart.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define HCI_COMMAND_PKT     0x01
#define HCI_EVENT_PKT       0x04
#define EVT_CMD_COMPLETE    0x0e
#define EVT_VENDOR          0xff

#define HCI_RESET               0x0c03
#define HCI_READ_LOCAL_NAME     0x0c14
#define HCI_VSC_DOWNLOAD_MINIDRV 0xfc2e

#define EVENT_QUEUE_MAX     8

struct pending_event {
    uint8_t data[300];
    int len;
};

struct fm_uart {
    int fd;
    struct fm_transport transport;
    struct pending_event queue[EVENT_QUEUE_MAX];
    int queue_head;
    int queue_count;
    char chip_name[64];
};

static void uart_msleep(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

static int sysfs_write(const char *path, const char *val)
{
    int fd = open(path, O_WRONLY);
    ssize_t n;

    if (fd < 0)
        return -1;
    n = write(fd, val, strlen(val));
    close(fd);
    return n < 0 ? -1 : 0;
}

static int tty_configure(int fd)
{
    struct termios t;

    tcflush(fd, TCIOFLUSH);
    if (tcgetattr(fd, &t) < 0)
        return -1;
    cfmakeraw(&t);
    t.c_cflag |= CRTSCTS | CLOCAL | CREAD;
    if (tcsetattr(fd, TCSANOW, &t) < 0)
        return -1;
    tcflush(fd, TCIOFLUSH);
    cfsetospeed(&t, B115200);
    cfsetispeed(&t, B115200);
    if (tcsetattr(fd, TCSANOW, &t) < 0)
        return -1;
    tcflush(fd, TCIOFLUSH);
    return 0;
}

static int read_exact(struct fm_uart *u, uint8_t *buf, int n, int timeout_ms)
{
    int got = 0;

    while (got < n) {
        struct pollfd p = { u->fd, POLLIN, 0 };
        int r = poll(&p, 1, timeout_ms);
        if (r <= 0)
            return -1;
        r = read(u->fd, buf + got, n - got);
        if (r <= 0)
            return -1;
        got += r;
    }
    return got;
}

static int hci_send(struct fm_uart *u, uint16_t opcode, const uint8_t *param, uint8_t plen)
{
    uint8_t buf[260];
    int len = 4 + plen;

    buf[0] = HCI_COMMAND_PKT;
    buf[1] = (uint8_t)(opcode & 0xff);
    buf[2] = (uint8_t)(opcode >> 8);
    buf[3] = plen;
    if (plen)
        memcpy(buf + 4, param, plen);
    return write(u->fd, buf, len) == len ? 0 : -1;
}

static int hci_recv(struct fm_uart *u, uint8_t *evt, int evtsz, int timeout_ms)
{
    uint8_t hdr[3];

    do {
        if (read_exact(u, hdr, 1, timeout_ms) != 1)
            return -1;
    } while (hdr[0] != HCI_EVENT_PKT);
    if (read_exact(u, hdr + 1, 2, 500) != 2)
        return -1;
    if (hdr[2] > evtsz - 2)
        return -1;
    evt[0] = hdr[1];
    evt[1] = hdr[2];
    if (hdr[2] && read_exact(u, evt + 2, hdr[2], 500) != hdr[2])
        return -1;
    return hdr[2] + 2;
}

static void queue_push(struct fm_uart *u, const uint8_t *evt, int len)
{
    int idx;

    if (u->queue_count == EVENT_QUEUE_MAX) {
        u->queue_head = (u->queue_head + 1) % EVENT_QUEUE_MAX;
        u->queue_count--;
    }
    idx = (u->queue_head + u->queue_count) % EVENT_QUEUE_MAX;
    if (len > (int)sizeof(u->queue[idx].data))
        len = (int)sizeof(u->queue[idx].data);
    memcpy(u->queue[idx].data, evt, len);
    u->queue[idx].len = len;
    u->queue_count++;
}

static int queue_pop(struct fm_uart *u, uint8_t *out, int outsz)
{
    struct pending_event *e;
    int len;

    if (u->queue_count == 0)
        return -1;
    e = &u->queue[u->queue_head];
    len = e->len > outsz ? outsz : e->len;
    memcpy(out, e->data, len);
    u->queue_head = (u->queue_head + 1) % EVENT_QUEUE_MAX;
    u->queue_count--;
    return len;
}

static int hci_command(struct fm_uart *u, uint16_t opcode, const uint8_t *param,
                       uint8_t plen, uint8_t *resp, int respsz, int timeout_ms)
{
    uint8_t evt[300];
    int n, tries;

    if (hci_send(u, opcode, param, plen) < 0)
        return -1;
    for (tries = 0; tries < 16; tries++) {
        n = hci_recv(u, evt, sizeof(evt), timeout_ms);
        if (n < 0)
            return -1;
        if (evt[0] == EVT_CMD_COMPLETE && n >= 5) {
            uint16_t op = (uint16_t)evt[3] | ((uint16_t)evt[4] << 8);
            if (op != opcode)
                continue;
            if (resp) {
                int cp = n - 5 > respsz ? respsz : n - 5;
                memcpy(resp, evt + 5, cp);
                return cp;
            }
            return n - 5;
        }
        if (evt[0] == EVT_VENDOR)
            queue_push(u, evt, n);
    }
    return -1;
}

static int transport_vsc(void *priv, const uint8_t *cmd, uint8_t len,
                         uint8_t *resp, int respsz, int timeout_ms)
{
    return hci_command((struct fm_uart *)priv, FM_VSC_OPCODE, cmd, len,
                       resp, respsz, timeout_ms);
}

static int transport_wait_event(void *priv, uint8_t *evt, int evtsz, int timeout_ms)
{
    struct fm_uart *u = (struct fm_uart *)priv;
    int n;

    n = queue_pop(u, evt, evtsz);
    if (n > 0)
        return n;
    return hci_recv(u, evt, evtsz, timeout_ms);
}

static int download_firmware(struct fm_uart *u, const char *firmware)
{
    uint8_t rec[260], resp[64];
    int fd, count = 0;

    fd = open(firmware, O_RDONLY);
    if (fd < 0)
        return -1;
    if (hci_command(u, HCI_VSC_DOWNLOAD_MINIDRV, NULL, 0, resp, sizeof(resp), 2000) < 0) {
        close(fd);
        return -1;
    }
    uart_msleep(50);
    while (read(fd, rec, 3) == 3) {
        uint16_t op = (uint16_t)rec[0] | ((uint16_t)rec[1] << 8);
        uint8_t len = rec[2];
        if (len && read(fd, rec + 3, len) != len)
            break;
        if (hci_command(u, op, rec + 3, len, resp, sizeof(resp), 2000) < 0) {
            close(fd);
            return -1;
        }
        count++;
    }
    close(fd);
    if (count == 0)
        return -1;
    uart_msleep(200);
    if (hci_command(u, HCI_RESET, NULL, 0, resp, sizeof(resp), 2000) < 0)
        return -1;
    uart_msleep(100);
    return 0;
}

struct fm_uart *fm_uart_open(const char *tty, const char *rfkill, const char *firmware)
{
    struct fm_uart *u;
    uint8_t resp[260];
    int rc;

    u = calloc(1, sizeof(*u));
    if (!u)
        return NULL;
    u->fd = -1;

    sysfs_write(rfkill, "0");
    uart_msleep(200);
    if (sysfs_write(rfkill, "1") < 0) {
        free(u);
        return NULL;
    }
    uart_msleep(300);

    u->fd = open(tty, O_RDWR | O_NOCTTY);
    if (u->fd < 0) {
        free(u);
        return NULL;
    }
    if (tty_configure(u->fd) < 0) {
        fm_uart_close(u);
        return NULL;
    }
    if (hci_command(u, HCI_RESET, NULL, 0, resp, sizeof(resp), 2000) < 0) {
        fm_uart_close(u);
        return NULL;
    }
    if (download_firmware(u, firmware) < 0) {
        fm_uart_close(u);
        return NULL;
    }
    rc = hci_command(u, HCI_READ_LOCAL_NAME, NULL, 0, resp, sizeof(resp), 2000);
    if (rc > 1) {
        int n = rc - 1;
        if (n > (int)sizeof(u->chip_name) - 1)
            n = (int)sizeof(u->chip_name) - 1;
        memcpy(u->chip_name, resp + 1, n);
        u->chip_name[n] = 0;
    }

    u->transport.priv = u;
    u->transport.vsc = transport_vsc;
    u->transport.wait_event = transport_wait_event;
    return u;
}

void fm_uart_close(struct fm_uart *u)
{
    if (!u)
        return;
    if (u->fd >= 0)
        close(u->fd);
    free(u);
}

struct fm_transport *fm_uart_transport(struct fm_uart *u)
{
    return &u->transport;
}

int fm_uart_chip_name(struct fm_uart *u, char *out, int outsz)
{
    int n = (int)strlen(u->chip_name);

    if (n >= outsz)
        n = outsz - 1;
    memcpy(out, u->chip_name, n);
    out[n] = 0;
    return n;
}
