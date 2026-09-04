#ifndef FM_PROTO_H
#define FM_PROTO_H

#include <stdint.h>

#define FM_VSC_OPCODE       0xfc15
#define FM_VSE_SUBCODE      0x08

#define FM_REG_RDS_SYS      0x00
#define FM_REG_FM_CTRL      0x01
#define FM_REG_RDS_CTL0     0x02
#define FM_REG_AUD_CTL0     0x05
#define FM_REG_SCH_CTL0     0x07
#define FM_REG_SCH_TUNE     0x09
#define FM_REG_FM_FREQ      0x0a
#define FM_REG_RSSI         0x0f
#define FM_REG_FM_RDS_MSK   0x10
#define FM_REG_FM_RDS_FLAG  0x12
#define FM_REG_RDS_WLINE    0x14
#define FM_REG_RDS_DATA     0x80
#define FM_REG_PCM_ROUTE    0x4d
#define FM_REG_SNR          0xdf
#define FM_REG_VOLUME_CTRL  0xf8
#define FM_REG_SEARCH_BOUND 0xfb
#define FM_REG_SEARCH_METH  0xfc
#define FM_REG_SCH_STEP     0xfd
#define FM_REG_PRESET_MAX   0xfe

#define FM_REG_RD           0x01
#define FM_REG_WR           0x00

#define FM_SYS_OFF          0x00
#define FM_SYS_ON           0x01
#define FM_SYS_RDS_ON       0x02

#define FM_CTRL_BAND_WEST   0x00
#define FM_CTRL_BAND_EAST   0x01
#define FM_CTRL_STEREO_AUTO 0x02
#define FM_CTRL_STEREO_MAN  0x04
#define FM_CTRL_STEREO_SW   0x08
#define FM_CTRL_HI_LO_INJ   0x10

#define FM_AUD_RF_MUTE      0x0001
#define FM_AUD_MANUAL_MUTE  0x0002
#define FM_AUD_ZMUTE_L_OFF  0x0004
#define FM_AUD_ZMUTE_R_OFF  0x0008
#define FM_AUD_DAC_ON       0x0010
#define FM_AUD_I2S_ON       0x0020
#define FM_AUD_DEEMPH_75    0x0040
#define FM_AUD_BANDWIDTH    0x0080

#define FM_TUNE_MODE_PRESET 0x01
#define FM_TUNE_MODE_SEEK   0x02
#define FM_TUNE_MODE_STOP   0x00
#define FM_SCAN_UP          0x80

#define FM_MASK_TUNE_CMPL   0x0001
#define FM_MASK_TUNE_FAIL   0x0002
#define FM_MASK_RDS_DATA    0x0004

#define FM_STEP_100KHZ      0x00
#define FM_STEP_50KHZ       0x01
#define FM_STEP_200KHZ      0x02

struct fm_transport {
    void *priv;
    int (*vsc)(void *priv, const uint8_t *cmd, uint8_t len,
               uint8_t *resp, int respsz, int timeout_ms);
    int (*wait_event)(void *priv, uint8_t *evt, int evtsz, int timeout_ms);
};

int fm_reg_write(struct fm_transport *t, uint8_t reg, const uint8_t *val, uint8_t len);
int fm_reg_write8(struct fm_transport *t, uint8_t reg, uint8_t val);
int fm_reg_write16(struct fm_transport *t, uint8_t reg, uint16_t val);
int fm_reg_read(struct fm_transport *t, uint8_t reg, uint8_t len, uint8_t *out, int outsz);

int fm_enable(struct fm_transport *t, int rds);
int fm_disable(struct fm_transport *t);
int fm_set_band_europe(struct fm_transport *t);
int fm_set_audio(struct fm_transport *t, int i2s, int dac, int muted);
int fm_set_mute(struct fm_transport *t, int mute);
int fm_set_volume(struct fm_transport *t, uint8_t vol);
int fm_tune(struct fm_transport *t, unsigned int khz);
int fm_seek(struct fm_transport *t, unsigned int start_khz, int up, unsigned int *found_khz);
int fm_get_frequency(struct fm_transport *t, unsigned int *khz);
int fm_get_rssi(struct fm_transport *t, int *rssi);
int fm_get_snr(struct fm_transport *t, int *snr);
int fm_read_flags(struct fm_transport *t, uint16_t *flags);
int fm_enable_rds(struct fm_transport *t, int on);
int fm_read_rds(struct fm_transport *t, uint8_t *buf, int bufsz);

static inline uint16_t fm_khz_to_reg(unsigned int khz)
{
    return (uint16_t)(khz - 64000);
}

static inline unsigned int fm_reg_to_khz(uint16_t reg)
{
    return (unsigned int)reg + 64000;
}

#endif
