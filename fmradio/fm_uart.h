#ifndef FM_UART_H
#define FM_UART_H

#include "fm_proto.h"

struct fm_uart;

struct fm_uart *fm_uart_open(const char *tty, const char *rfkill, const char *firmware);
void fm_uart_close(struct fm_uart *u);
struct fm_transport *fm_uart_transport(struct fm_uart *u);
int fm_uart_chip_name(struct fm_uart *u, char *out, int outsz);

#endif
