#define LOG_TAG "FMLIB_BRCM"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <log/log.h>

#include "fmr.h"

#define FMD_SOCKET_PATH "/dev/socket/fmd"
#define FMD_TIMEOUT_MS  25000
#define FMD_VALID_RSSI_DBM (-103)

static int fmd_connect(void)
{
    struct sockaddr_un addr;
    int fd;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        ALOGE("socket: %s", strerror(errno));
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, FMD_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ALOGE("connect %s: %s", FMD_SOCKET_PATH, strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

static int fmd_request(int fd, const char *cmd, char *reply, int replysz)
{
    char buf[256];
    int len, got = 0;

    if (fd < 0)
        return -1;
    len = snprintf(buf, sizeof(buf), "%s\n", cmd);
    if (write(fd, buf, len) != len)
        return -1;

    for (;;) {
        struct pollfd p = { fd, POLLIN, 0 };
        int n;
        if (poll(&p, 1, FMD_TIMEOUT_MS) <= 0)
            return -1;
        n = read(fd, reply + got, replysz - got - 1);
        if (n <= 0)
            return -1;
        got += n;
        reply[got] = 0;
        if (strchr(reply, '\n'))
            break;
        if (got >= replysz - 1)
            break;
    }
    if (strncmp(reply, "ok", 2) != 0) {
        ALOGE("fmd '%s' -> %s", cmd, reply);
        return -1;
    }
    return 0;
}

static int fmd_simple(int fd, const char *cmd)
{
    char reply[256];
    return fmd_request(fd, cmd, reply, sizeof(reply));
}

static int brcm_open_dev(const char *name, int *fd)
{
    (void)name;
    *fd = fmd_connect();
    if (*fd < 0)
        return -ERR_INVALID_FD;
    if (fmd_simple(*fd, "open") < 0) {
        close(*fd);
        *fd = -1;
        return -ERR_INVALID_FD;
    }
    return 0;
}

static int brcm_close_dev(int fd)
{
    if (fd < 0)
        return 0;
    fmd_simple(fd, "close");
    close(fd);
    return 0;
}

static int brcm_pwr_up(int fd, int band, int freq)
{
    char cmd[64];
    (void)band;
    snprintf(cmd, sizeof(cmd), "tune %d", freq * 100);
    return fmd_simple(fd, cmd) < 0 ? -1 : 0;
}

static int brcm_pwr_down(int fd, int type)
{
    (void)type;
    return fmd_simple(fd, "mute 1") < 0 ? -1 : 0;
}

static int brcm_tune(int fd, int freq, int band)
{
    char cmd[64];
    (void)band;
    snprintf(cmd, sizeof(cmd), "tune %d", freq * 100);
    return fmd_simple(fd, cmd) < 0 ? -1 : 0;
}

static int brcm_seek(int fd, int *freq, int band, int dir, int lev)
{
    char cmd[64];
    char reply[256];
    (void)band;
    (void)lev;
    snprintf(cmd, sizeof(cmd), "seek %s %d", dir ? "up" : "down", *freq * 100);
    if (fmd_request(fd, cmd, reply, sizeof(reply)) < 0)
        return -1;
    *freq = (int)(strtol(reply + 2, NULL, 10) / 100);
    return 0;
}

static int brcm_scan(int fd, uint16_t *tbl, int *num, int band, int sort)
{
    int found = 0;
    int freq = 875;
    (void)band;
    (void)sort;

    while (found < *num && freq < 1080) {
        int cur = freq;
        if (brcm_seek(fd, &cur, band, 1, 0) < 0)
            break;
        if (cur <= freq)
            break;
        tbl[found++] = (uint16_t)cur;
        freq = cur;
    }
    *num = found;
    return 0;
}

static int brcm_stop_scan(int fd)
{
    (void)fd;
    return 0;
}

static int brcm_set_mute(int fd, int mute)
{
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "mute %d", mute ? 1 : 0);
    return fmd_simple(fd, cmd) < 0 ? -1 : 0;
}

static int brcm_is_rdsrx_support(int fd, int *supt)
{
    (void)fd;
    *supt = 1;
    return 0;
}

static int brcm_turn_on_off_rds(int fd, int onoff)
{
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "rds %d", onoff ? 1 : 0);
    return fmd_simple(fd, cmd) < 0 ? -1 : 0;
}

static int brcm_get_chip_id(int fd, int *chipid)
{
    (void)fd;
    *chipid = 0x4356;
    return 0;
}

static int brcm_read_rds_data(int fd, RDSData_Struct *rds, uint16_t *rds_status)
{
    (void)fd;
    (void)rds;
    *rds_status = 0;
    return -1;
}

static int brcm_get_ps(int fd, RDSData_Struct *rds, uint8_t **ps, int *ps_len)
{
    (void)fd;
    (void)rds;
    (void)ps;
    *ps_len = 0;
    return -1;
}

static int brcm_get_rt(int fd, RDSData_Struct *rds, uint8_t **rt, int *rt_len)
{
    (void)fd;
    (void)rds;
    (void)rt;
    *rt_len = 0;
    return -1;
}

static int brcm_active_af(int fd, RDSData_Struct *rds, int band, uint16_t cur_freq,
                          uint16_t *ret_freq)
{
    (void)fd;
    (void)rds;
    (void)band;
    *ret_freq = cur_freq;
    return -1;
}

static int brcm_ana_switch(int fd, int antenna)
{
    (void)fd;
    (void)antenna;
    return 0;
}

static int brcm_soft_mute_tune(int fd, fm_softmute_tune_t *para)
{
    char cmd[64];
    char reply[256];
    long rssi = 0;
    char *p;

    para->valid = fm_false;
    snprintf(cmd, sizeof(cmd), "smtune %d", para->freq * 10);
    if (fmd_request(fd, cmd, reply, sizeof(reply)) < 0)
        return -1;

    p = strchr(reply + 2, ' ');
    if (p == NULL)
        return -1;
    rssi = strtol(p + 1, NULL, 10);
    para->rssi = (int)rssi;
    para->valid = (rssi >= FMD_VALID_RSSI_DBM) ? fm_true : fm_false;
    return 0;
}

static int brcm_desense_check(int fd, int freq, int rssi)
{
    (void)fd;
    (void)freq;
    (void)rssi;
    return 0;
}

static int brcm_pre_search(int fd)
{
    (void)fd;
    return 0;
}

static int brcm_restore_search(int fd)
{
    (void)fd;
    return 0;
}

void FM_interface_init(struct fm_cbk_tbl *cbk_tbl)
{
    cbk_tbl->open_dev = brcm_open_dev;
    cbk_tbl->close_dev = brcm_close_dev;
    cbk_tbl->pwr_up = brcm_pwr_up;
    cbk_tbl->pwr_down = brcm_pwr_down;
    cbk_tbl->seek = brcm_seek;
    cbk_tbl->scan = brcm_scan;
    cbk_tbl->stop_scan = brcm_stop_scan;
    cbk_tbl->tune = brcm_tune;
    cbk_tbl->set_mute = brcm_set_mute;
    cbk_tbl->is_rdsrx_support = brcm_is_rdsrx_support;
    cbk_tbl->turn_on_off_rds = brcm_turn_on_off_rds;
    cbk_tbl->get_chip_id = brcm_get_chip_id;
    cbk_tbl->read_rds_data = brcm_read_rds_data;
    cbk_tbl->get_ps = brcm_get_ps;
    cbk_tbl->get_rt = brcm_get_rt;
    cbk_tbl->active_af = brcm_active_af;
    cbk_tbl->ana_switch = brcm_ana_switch;
    cbk_tbl->soft_mute_tune = brcm_soft_mute_tune;
    cbk_tbl->desense_check = brcm_desense_check;
    cbk_tbl->pre_search = brcm_pre_search;
    cbk_tbl->restore_search = brcm_restore_search;
}
