#include "fm_proto.h"

#include <string.h>
#include <time.h>

#define FM_SCAN_METHOD_NORMAL   0x00
#define FM_SCAN_DIRECT_MASK     0xf0
#define FM_ENABLE_DELAY_MS      300
#define FM_TUNE_TIMEOUT_MS      4000
#define FM_SEEK_TIMEOUT_MS      20000
#define FM_DEFAULT_RSSI_THRESH  105

static void fm_msleep(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

int fm_reg_write(struct fm_transport *t, uint8_t reg, const uint8_t *val, uint8_t len)
{
    uint8_t cmd[64], resp[64];
    int rc;

    if (len > sizeof(cmd) - 2)
        return -1;
    cmd[0] = reg;
    cmd[1] = FM_REG_WR;
    if (len)
        memcpy(cmd + 2, val, len);
    rc = t->vsc(t->priv, cmd, (uint8_t)(2 + len), resp, sizeof(resp), 3000);
    if (rc < 1 || resp[0] != 0)
        return -1;
    return 0;
}

int fm_reg_write8(struct fm_transport *t, uint8_t reg, uint8_t val)
{
    return fm_reg_write(t, reg, &val, 1);
}

int fm_reg_write16(struct fm_transport *t, uint8_t reg, uint16_t val)
{
    uint8_t p[2];
    p[0] = (uint8_t)(val & 0xff);
    p[1] = (uint8_t)(val >> 8);
    return fm_reg_write(t, reg, p, 2);
}

int fm_reg_read(struct fm_transport *t, uint8_t reg, uint8_t len, uint8_t *out, int outsz)
{
    uint8_t cmd[3], resp[128];
    int rc, n;

    cmd[0] = reg;
    cmd[1] = FM_REG_RD;
    cmd[2] = len;
    rc = t->vsc(t->priv, cmd, 3, resp, sizeof(resp), 3000);
    if (rc < 3 || resp[0] != 0)
        return -1;
    n = rc - 3;
    if (n > outsz)
        n = outsz;
    if (n > 0)
        memcpy(out, resp + 3, n);
    return n;
}

int fm_enable(struct fm_transport *t, int rds)
{
    uint8_t val = FM_SYS_ON;

    if (rds)
        val |= FM_SYS_RDS_ON;
    if (fm_reg_write8(t, FM_REG_RDS_SYS, val) < 0)
        return -1;
    fm_msleep(FM_ENABLE_DELAY_MS);
    return 0;
}

int fm_disable(struct fm_transport *t)
{
    fm_set_mute(t, 1);
    return fm_reg_write8(t, FM_REG_RDS_SYS, FM_SYS_OFF);
}

int fm_set_band_europe(struct fm_transport *t)
{
    if (fm_reg_write16(t, FM_REG_SEARCH_BOUND, 0) < 0)
        return -1;
    if (fm_reg_write8(t, FM_REG_FM_CTRL, FM_CTRL_STEREO_AUTO | FM_CTRL_BAND_WEST) < 0)
        return -1;
    return fm_reg_write8(t, FM_REG_SCH_STEP, FM_STEP_100KHZ);
}

int fm_set_audio(struct fm_transport *t, int i2s, int dac, int muted)
{
    uint16_t ctl = FM_AUD_ZMUTE_L_OFF | FM_AUD_ZMUTE_R_OFF;

    if (i2s)
        ctl |= FM_AUD_I2S_ON;
    if (dac)
        ctl |= FM_AUD_DAC_ON;
    if (muted)
        ctl |= FM_AUD_MANUAL_MUTE;
    return fm_reg_write16(t, FM_REG_AUD_CTL0, ctl);
}

int fm_set_mute(struct fm_transport *t, int mute)
{
    uint8_t cur[2];
    uint16_t ctl;

    if (fm_reg_read(t, FM_REG_AUD_CTL0, 2, cur, sizeof(cur)) < 2)
        return -1;
    ctl = (uint16_t)cur[0] | ((uint16_t)cur[1] << 8);
    if (mute)
        ctl |= FM_AUD_MANUAL_MUTE;
    else
        ctl &= (uint16_t)~FM_AUD_MANUAL_MUTE;
    return fm_reg_write16(t, FM_REG_AUD_CTL0, ctl);
}

int fm_set_volume(struct fm_transport *t, uint8_t vol)
{
    return fm_reg_write8(t, FM_REG_VOLUME_CTRL, vol);
}

static int fm_wait_tune(struct fm_transport *t, int timeout_ms, uint16_t *flags)
{
    uint8_t evt[300];
    int n;

    for (;;) {
        n = t->wait_event(t->priv, evt, sizeof(evt), timeout_ms);
        if (n < 0)
            return -1;
        if (n > 2 && evt[0] == 0xff && evt[2] == FM_VSE_SUBCODE)
            break;
    }
    return fm_read_flags(t, flags);
}

int fm_tune(struct fm_transport *t, unsigned int khz)
{
    uint16_t flags = 0;

    if (fm_reg_write16(t, FM_REG_FM_FREQ, fm_khz_to_reg(khz)) < 0)
        return -1;
    if (fm_reg_write16(t, FM_REG_FM_RDS_MSK, FM_MASK_TUNE_CMPL | FM_MASK_TUNE_FAIL) < 0)
        return -1;
    if (fm_reg_write8(t, FM_REG_SCH_TUNE, FM_TUNE_MODE_PRESET) < 0)
        return -1;
    if (fm_wait_tune(t, FM_TUNE_TIMEOUT_MS, &flags) < 0)
        return -1;
    if (flags & FM_MASK_TUNE_FAIL)
        return -1;
    return 0;
}

int fm_seek(struct fm_transport *t, unsigned int start_khz, int up, unsigned int *found_khz)
{
    uint16_t flags = 0;
    uint8_t sch_ctl;

    if (fm_reg_write8(t, FM_REG_SEARCH_METH, FM_SCAN_METHOD_NORMAL) < 0)
        return -1;
    if (fm_reg_write8(t, FM_REG_PRESET_MAX, 0) < 0)
        return -1;
    sch_ctl = (uint8_t)(FM_DEFAULT_RSSI_THRESH |
                        ((up ? FM_SCAN_UP : 0) & FM_SCAN_DIRECT_MASK));
    if (fm_reg_write8(t, FM_REG_SCH_CTL0, sch_ctl) < 0)
        return -1;
    if (fm_reg_write16(t, FM_REG_FM_FREQ, fm_khz_to_reg(start_khz)) < 0)
        return -1;
    if (fm_reg_write16(t, FM_REG_FM_RDS_MSK, FM_MASK_TUNE_CMPL | FM_MASK_TUNE_FAIL) < 0)
        return -1;
    if (fm_reg_write8(t, FM_REG_SCH_TUNE, FM_TUNE_MODE_SEEK) < 0)
        return -1;
    if (fm_wait_tune(t, FM_SEEK_TIMEOUT_MS, &flags) < 0)
        return -1;
    if (flags & FM_MASK_TUNE_FAIL)
        return -1;
    return fm_get_frequency(t, found_khz);
}

int fm_get_frequency(struct fm_transport *t, unsigned int *khz)
{
    uint8_t buf[2];

    if (fm_reg_read(t, FM_REG_FM_FREQ, 2, buf, sizeof(buf)) < 2)
        return -1;
    *khz = fm_reg_to_khz((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));
    return 0;
}

int fm_get_rssi(struct fm_transport *t, int *rssi)
{
    uint8_t buf[1];

    if (fm_reg_read(t, FM_REG_RSSI, 1, buf, sizeof(buf)) < 1)
        return -1;
    *rssi = (int)(int8_t)buf[0];
    return 0;
}

int fm_get_snr(struct fm_transport *t, int *snr)
{
    uint8_t buf[1];

    if (fm_reg_read(t, FM_REG_SNR, 1, buf, sizeof(buf)) < 1)
        return -1;
    *snr = buf[0];
    return 0;
}

int fm_read_flags(struct fm_transport *t, uint16_t *flags)
{
    uint8_t buf[2];

    if (fm_reg_read(t, FM_REG_FM_RDS_FLAG, 2, buf, sizeof(buf)) < 2)
        return -1;
    *flags = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    return 0;
}

int fm_enable_rds(struct fm_transport *t, int on)
{
    uint8_t cur[1];
    uint8_t val;

    if (fm_reg_read(t, FM_REG_RDS_SYS, 1, cur, sizeof(cur)) < 1)
        return -1;
    val = cur[0];
    if (on)
        val |= FM_SYS_RDS_ON;
    else
        val &= (uint8_t)~FM_SYS_RDS_ON;
    return fm_reg_write8(t, FM_REG_RDS_SYS, val);
}

int fm_read_rds(struct fm_transport *t, uint8_t *buf, int bufsz)
{
    return fm_reg_read(t, FM_REG_RDS_DATA, (uint8_t)(bufsz > 255 ? 255 : bufsz),
                       buf, bufsz);
}
