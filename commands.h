/**
 * Copyright (c) 2014 - 2015 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Elgato Game Capture HD Linux driver and is
 * distributed under the MIT License. For more information, see LICENSE file.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

// scmd commands
#define SCMD_IDLE		1
#define SCMD_INIT		4
#define SCMD_STATE_CHANGE	5

// Getting replaced by reverse engineered functions.
void read_config(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength);
int read_config4(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3);
void write_config2(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1);
void write_config3(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2);
void write_config4(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3);
void write_config5(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4);
void write_config6(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5);
void write_config8(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char data0, unsigned char data1, unsigned char data2, unsigned char data3, unsigned char data4, unsigned char data5, unsigned char data6, unsigned char data7);

/**
 * Reverse engineered function from official drivers. Used to set device states
 * and modes. scmd() is the short form for system command.
 *
 * @param command there are currently three commands, we have identified. A
 *  value of 1 stands for IDLE command. A value of 4 means INIT and is used for
 *  loading firmwares onto the device. A value of 5 means STATE_CHANGE and is
 *  used to control the device's encoding procedure.
 * @param mode there is only one use case known so far. It's for setting
 *  encoding mode.
 * @param data applies to send[4] and send[5]. The 16-bit integer data needs to
 *  be split up into two 8-bit integers. It holds data to further specify
 *  the command parameter. In conjunction with STATE_CHANGE, it is used to set
 *  specific encoding states: 1 means STOP, 2 means START and 4 means NULL.
 *  Setting STATE_CHANGE to NULL will make the device output an empty data
 *  stream during the encoding process.
 */
void scmd(uint8_t command, uint8_t mode, uint16_t data);

/**
 * Reverse engineered function from official drivers. Used to set specific
 * device parameters, like H.264 profile, video size and audio bitrate.
 *
 * @param wIndex offset, where requests are passed to.
 * @param shift bit shift left operations to the whole send[8] array. If any
 *  bit gets cut off, substracting wIndex by 1 will be equal to a bit shift
 *  right operation of 8. It's used for readibility. Minimum value is 0 and
 *  maximum is 16.
 * @param range applies to send[4] and send[5]. Sets n bits to 1, starting from
 *  the first bit in send[5]. If it's greater than 8, bits are set in send[4].
 *  Minimum value is 0 and maximum is 16.
 * @param data applies to send[0] and send[1]. The 16-bit integer data needs to
 *  be split up into two 8-bit integers. It holds the actual data, like video
 *  vertical size and bitrate.
 */
void sparam(uint16_t wIndex, uint8_t shift, uint8_t range, uint16_t data);

/**
 * Reverse engineered function from official drivers. Used to configure the
 * device.
 *
 * @param wIndex offset, where requests are passed to.
 * @param data applies to send[0] and send[1]. The 16-bit integer data needs to
 *  be split up into two 8-bit integers. It holds the actual data, like video
 *  vertical size and bitrate.
 */
void slsi(uint16_t wIndex, uint16_t data);

/**
 * Loads firmware to the device
 *
 * @param file relative path to binary firmware file
 */
void dlfirm(const char *file);

void receive_data();

#endif
