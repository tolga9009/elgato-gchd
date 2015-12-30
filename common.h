/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Elgato Game Capture HD Linux driver and is
 * distributed under the MIT License. For more information, see LICENSE file.
 */

#ifndef COMMON_H
#define COMMON_H

#include <libusb-1.0/libusb.h>

// constants
#define DATA_BUF		0x4000
#define FW_MB86H57_H58_IDLE	"firmware/mb86h57_h58_idle.bin"
#define FW_MB86H57_H58_ENC	"firmware/mb86h57_h58_enc_h.bin"

struct libusb_device_handle *devh;

#endif
