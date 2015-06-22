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

struct libusb_device_handle *devh;

#endif
