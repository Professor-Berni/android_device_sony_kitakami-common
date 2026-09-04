#ifndef FM_HCI_H
#define FM_HCI_H

#include "fm_proto.h"

struct fm_hci;

struct fm_hci *fm_hci_open(const char *path);
void fm_hci_close(struct fm_hci *h);
struct fm_transport *fm_hci_transport(struct fm_hci *h);

#endif
