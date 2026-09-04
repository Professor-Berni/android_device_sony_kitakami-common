#define _GNU_SOURCE
#include "fm_hci.h"
#include "fm_proto.h"
#include "fm_uart.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_ENV      "ANDROID_SOCKET_fmd"
#define SOCKET_PATH     "/dev/socket/fmd"
#define BT_HAL_PATH     "/dev/socket/fm_hci"
#define TTY_PATH        "/dev/ttyHS0"
#define RFKILL_PATH     "/sys/class/rfkill/rfkill0/state"
#define FIRMWARE_PATH   "/system/etc/firmware/BCM43xx.hcd"

#define BAND_LOW_KHZ    87500
#define BAND_HIGH_KHZ   108000
#define DEFAULT_KHZ     100000

struct fmd_state {
    struct fm_hci *hci;
    struct fm_uart *uart;
    struct fm_transport *tp;
    unsigned int freq_khz;
    int powered;
    int rds_on;
};

static struct fmd_state fmd;

static void reply(int fd, const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf) - 2, fmt, ap);
    va_end(ap);
    if (n < 0)
        return;
    buf[n++] = '\n';
    (void)!write(fd, buf, n);
}

static void fm_transport_close(void)
{
    fm_hci_close(fmd.hci);
    fm_uart_close(fmd.uart);
    fmd.hci = NULL;
    fmd.uart = NULL;
    fmd.tp = NULL;
}

static int fm_transport_open(void)
{
    if (access(BT_HAL_PATH, F_OK) == 0) {
        fmd.hci = fm_hci_open(BT_HAL_PATH);
        if (!fmd.hci)
            return -1;
        fmd.tp = fm_hci_transport(fmd.hci);
        return 0;
    }

    fmd.uart = fm_uart_open(TTY_PATH, RFKILL_PATH, FIRMWARE_PATH);
    if (!fmd.uart)
        return -1;
    fmd.tp = fm_uart_transport(fmd.uart);
    return 0;
}

static int fm_power_up(void)
{
    if (fmd.powered)
        return 0;
    if (fm_transport_open() < 0)
        return -1;
    if (fm_enable(fmd.tp, 1) < 0)
        goto fail;
    if (fm_set_band_europe(fmd.tp) < 0)
        goto fail;
    if (fm_set_audio(fmd.tp, 1, 0, 0) < 0)
        goto fail;
    fmd.powered = 1;
    fmd.rds_on = 1;
    return 0;
fail:
    fm_transport_close();
    return -1;
}

static void fm_power_down(void)
{
    if (!fmd.powered)
        return;
    fm_disable(fmd.tp);
    fm_transport_close();
    fmd.powered = 0;
}

static int valid_freq(unsigned int khz)
{
    return khz >= BAND_LOW_KHZ && khz <= BAND_HIGH_KHZ;
}

static void handle_command(int fd, char *line)
{
    char *cmd = strtok(line, " \t");
    char *arg1 = strtok(NULL, " \t");
    char *arg2 = strtok(NULL, " \t");

    if (!cmd)
        return;

    if (!strcmp(cmd, "open")) {
        if (fm_power_up() < 0) {
            reply(fd, "err power-up failed");
            return;
        }
        {
            char name[64] = "";
            if (fmd.uart)
                fm_uart_chip_name(fmd.uart, name, sizeof(name));
            else
                strncpy(name, "bluetooth-hal", sizeof(name) - 1);
            reply(fd, "ok %s", name);
        }
        return;
    }

    if (!strcmp(cmd, "close")) {
        fm_power_down();
        reply(fd, "ok");
        return;
    }

    if (!fmd.powered) {
        reply(fd, "err not-open");
        return;
    }

    if (!strcmp(cmd, "tune")) {
        unsigned int khz = arg1 ? (unsigned int)strtoul(arg1, NULL, 10) : DEFAULT_KHZ;
        if (!valid_freq(khz)) {
            reply(fd, "err bad-freq");
            return;
        }
        if (fm_tune(fmd.tp, khz) < 0) {
            reply(fd, "err tune-failed");
            return;
        }
        fmd.freq_khz = khz;
        reply(fd, "ok %u", khz);
    } else if (!strcmp(cmd, "seek")) {
        unsigned int found = 0;
        unsigned int start = arg2 ? (unsigned int)strtoul(arg2, NULL, 10) : fmd.freq_khz;
        int up = !arg1 || !strcmp(arg1, "up");
        if (!valid_freq(start))
            start = DEFAULT_KHZ;
        if (fm_seek(fmd.tp, start, up, &found) < 0) {
            reply(fd, "err seek-failed");
            return;
        }
        fmd.freq_khz = found;
        reply(fd, "ok %u", found);
    } else if (!strcmp(cmd, "smtune")) {
        unsigned int khz = arg1 ? (unsigned int)strtoul(arg1, NULL, 10) : 0;
        int rssi = 0, snr = 0;
        if (!valid_freq(khz)) {
            reply(fd, "err bad-freq");
            return;
        }
        if (fm_tune(fmd.tp, khz) < 0) {
            reply(fd, "err tune-failed");
            return;
        }
        fmd.freq_khz = khz;
        if (fm_get_rssi(fmd.tp, &rssi) < 0) {
            reply(fd, "err");
            return;
        }
        fm_get_snr(fmd.tp, &snr);
        reply(fd, "ok %u %d %d", khz, rssi, snr);
    } else if (!strcmp(cmd, "freq")) {
        unsigned int khz = 0;
        if (fm_get_frequency(fmd.tp, &khz) < 0)
            reply(fd, "err");
        else
            reply(fd, "ok %u", khz);
    } else if (!strcmp(cmd, "rssi")) {
        int rssi = 0;
        if (fm_get_rssi(fmd.tp, &rssi) < 0)
            reply(fd, "err");
        else
            reply(fd, "ok %d", rssi);
    } else if (!strcmp(cmd, "snr")) {
        int snr = 0;
        if (fm_get_snr(fmd.tp, &snr) < 0)
            reply(fd, "err");
        else
            reply(fd, "ok %d", snr);
    } else if (!strcmp(cmd, "mute")) {
        int on = arg1 ? atoi(arg1) : 1;
        reply(fd, fm_set_mute(fmd.tp, on) < 0 ? "err" : "ok");
    } else if (!strcmp(cmd, "volume")) {
        int vol = arg1 ? atoi(arg1) : 128;
        if (vol < 0)
            vol = 0;
        if (vol > 255)
            vol = 255;
        reply(fd, fm_set_volume(fmd.tp, (uint8_t)vol) < 0 ? "err" : "ok");
    } else if (!strcmp(cmd, "rds")) {
        int on = arg1 ? atoi(arg1) : 1;
        if (fm_enable_rds(fmd.tp, on) < 0) {
            reply(fd, "err");
            return;
        }
        fmd.rds_on = on;
        reply(fd, "ok");
    } else if (!strcmp(cmd, "audio")) {
        int i2s = !arg1 || !strcmp(arg1, "i2s") || !strcmp(arg1, "both");
        int dac = arg1 && (!strcmp(arg1, "dac") || !strcmp(arg1, "both"));
        reply(fd, fm_set_audio(fmd.tp, i2s, dac, 0) < 0 ? "err" : "ok");
    } else if (!strcmp(cmd, "flags")) {
        uint16_t flags = 0;
        if (fm_read_flags(fmd.tp, &flags) < 0)
            reply(fd, "err");
        else
            reply(fd, "ok 0x%04x", flags);
    } else if (!strcmp(cmd, "status")) {
        reply(fd, "ok on %u rds %d", fmd.freq_khz, fmd.rds_on);
    } else {
        reply(fd, "err unknown-command");
    }
}

static void serve_client(int fd)
{
    char buf[512];
    int used = 0;

    for (;;) {
        int n = read(fd, buf + used, (int)sizeof(buf) - used - 1);
        char *nl;
        if (n <= 0)
            return;
        used += n;
        buf[used] = 0;
        while ((nl = strchr(buf, '\n')) != NULL) {
            int consumed;
            *nl = 0;
            if (nl > buf && nl[-1] == '\r')
                nl[-1] = 0;
            if (!strcmp(buf, "quit"))
                return;
            handle_command(fd, buf);
            consumed = (int)(nl - buf) + 1;
            memmove(buf, buf + consumed, used - consumed + 1);
            used -= consumed;
        }
        if (used >= (int)sizeof(buf) - 1)
            used = 0;
    }
}

static int listen_socket(const char *path)
{
    struct sockaddr_un addr;
    const char *env = getenv(SOCKET_ENV);
    int fd;

    if (env) {
        fd = atoi(env);
        if (fd > 0 && listen(fd, 4) == 0)
            return fd;
    }
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    unlink(path);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    chmod(path, 0660);
    if (listen(fd, 4) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : SOCKET_PATH;
    int lfd;

    signal(SIGPIPE, SIG_IGN);

    lfd = listen_socket(path);
    if (lfd < 0) {
        fprintf(stderr, "fmd: cannot listen on %s: %s\n", path, strerror(errno));
        return 1;
    }
    fmd.freq_khz = DEFAULT_KHZ;

    for (;;) {
        int cfd = accept(lfd, NULL, NULL);
        if (cfd < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        serve_client(cfd);
        close(cfd);
        fm_power_down();
    }
    close(lfd);
    return 0;
}
