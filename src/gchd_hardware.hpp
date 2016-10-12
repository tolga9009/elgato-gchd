/**
 * Copyright (c) 2016 Scott Dossey <seveirein@yahoo.com>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#ifndef GCHD_HARDWARE_H
#define GCHD_HARDWARE_H

#define EP_OUT		0x02 //Used for firmware load/

enum class DeviceType {
	Unknown,
	GameCaptureHD,
	GameCaptureHDNew,
	GameCaptureHD60, //Not supported at all yet, but here for future's sake
	GameCaptureHD60S //Not supported at all yet, but here for future's sake
};

/* These next three registers go together to get a 8 digit textual
 * version number, prefixed with a "V" and terminated with 0x26
 * which is ctrl-z.
 *
 * On HD, the registers HDNEW_VERSION_REGISTER0 and
 *                      HDNEW_VERSION_REGISTER1 read back zero,
 * so it is safe to read these first, then switch to the HD version
 * of the registers.
 */

/* This appears to hold a version number. It is read first, and on HDNew
 * Devices it reads back as 4 text bytes == "V130" 
 * On the original HD device, it appears to read back as 4 zeroes.
 */
#define HDNEW_VERSION_REGISTER0 0xbc, 0x0800, 0x0094 //4 byte read

/* This also appears to be a continuation of the version number, 
 * or some form other form of identification value. It reads back 
 * "7230" on one of my HDNew captures. 
 * On HD, it reads back zeroes.
 */
#define HDNEW_VERSION_REGISTER1 0xbc, 0x0800, 0x0098 //4 byte read

/* This also holds 0x30, 0x1a, 0x00, 0x01 on my hdnew
 * It is believed the first byte is the last digit of the
 * version number, the 0x1a might be a terminator (it is ctrl-z)
 * and the 0x01 at the end might indicate that this is HDNew,
 * as the HD version of this register reads back the 
 * the same but that byte is 0.
 */
#define HDNEW_VERSION_REGISTER2 0xbc, 0x0800, 0x009c //4 byte read

/* And now for the HD versions. These are exactly the same as 
 * the HDNew ones, but at a different location.  You can 
 * read HDNEW_VERSION_REGISTER0 and HDNEW_VERSION_REGISTER1 safely
 * on HD to check them for zero (but skip HDNEW_VERSION_REGISTER2).
 * If they are 0, read these.
 */
#define HD_VERSION_REGISTER0 0xbc, 0x800, 0x0010 //0x56313130 on one board. "V110"
#define HD_VERSION_REGISTER1 0xbc, 0x800, 0x0014 //0x35303930 "5090"
#define HD_VERSION_REGISTER2 0xbc, 0x800, 0x0018 //0x301a0000 "0" + ctrl-z + 0s.
 
/* ENABLE_REGISTER
 * 
 *This register is used to actually enable certain devices on the
 * board. I believe it actually controls a bank of GPIOS.
 * See the ENABLE BITS for a list.
 */
#define ENABLE_REGISTER 0xbc, 0x0900, 0x0018 //2 byte read, write

/* 
  * These are the ENABLE BITS.
  *
  * These essentially educated guesses as to what
  * what bit position in register ENABLE_REGISTER and its shadow MAIL_SEND_ENABLE_STATE_REGISTER
  * do.
  */
#define EB_FIRMWARE_PROCESSOR (1<<1)  /* Believed to map to gpio to turn on the processor we load firmware for. */
#define EB_ANALOG_INPUT       (1<<2)  /* Set=composite/component Clear=HDMI. */
#define EB_ENCODER_ENABLE     (1<<3)  /* Turned on shortly after encode firmware is loaded. */
#define EB_ENCODER_TRIGGER    (1<<4)  /* No idea what this does, but it is set 
                                       * to 1 for one command only, and not held at 1,
                                       * which leads me to believe it is a GPIO triggering
                                       * or resetting something. Once it is set to 1,
                                       * (briefly), the equivalent MAIL_ENABLE_STATE_REGISTER bit is 
                                       * set to 1 but stickily, and doesn't clear until 
                                       * specific event that clears most of the ENCODE_STATE_REGISTER
                                       * bits (as described at that register's description).
                                       */
#define EB_COMPOSITE_MUX     (1<<8)   /* This only set if using composite input. */
#define EB_ANALOG_MUX        (1<<9)   /* Set if using composite or component input. */

/* This is used to determine whether we entered state where things are still initialized. */
#define EB_STILL_INITIALIZED_MASK (EB_ENCODER_ENABLE|EB_ENCODER_TRIGGER|EB_COMPOSITE_MUX|EB_ANALOG_MUX)

/* MAIL_SEND_ENABLE_REGISTER_STATE
 *
 * This register's bit position definitions are the same as the 
 * ENABLE_REGISTER's and it is apparently a communication register
 * used to tell another chip/process whether a bit inside the ENABLE_REGISTER
 * has been configured yet. On the GameCaptureHD it also might be ther register 
 * that signals an active mail write request.
 *
 * So this ultimately looks like a classic mailbox register, our
 * writes to it notify another device/process, and it notifies us
 * when it is done processing it.
 *
 * See MAIL_REQUEST_READY to see how to poll on this being sent
 * to other device/process.
 *
 * So unlike the ENABLE_REGISTER, this register sets a 1 when
 * a bit is configured (whether configure to 1 or 0, or even edge triggered like 
 * EB_ENCODER_TRIGGER).
 *
 * Once enabled, the the only way bits get cleared (besides hardware
 * reset) that we know about is a point where all the known bits but
 * (EB_ANALOG_INPUT | EB_ENCODER_ENABLE) are cleared in ENABLE_REGISTER.
 * This is hypothesized to shut off off the internal CPU.
 *
 * This hypothesis is formed:
 *     a) Because the EB_FIRMWARE_PROCESSOR bit is cleared
 *     b) Almost all MAIL_SEND_ENABLE_REGISTER_STATE bit are clear by the driver when this happens,
 *        suggesting configuration/states are lost.
 *
 * When EB_FIRMWARE_PROCESSOR is cleared in the ENABLE_REGISTER,
 * and it is set to (EB_ANALOG_INPUT | EB_ENCODER_ENABLE)
 * the MAIL_SEND_ENABLE_REGISTER_STATE is set to
 *    (EB_ANALOG_INPUT | EB_FIRMWARE_PROCESSOR) (on HD)
 *    (EB_ANALOG_INPUT | EB_FIRMWARE_PROCESSOR) | 0xd080 (on HDNew)
 * Suggesting that the driver thinks those 2 things are configured.
 * It is believed EB_FIRMWARE_PROCESSOR set in MAIL_SEND_ENABLE_REGISTER_STATE
 * but clear in MAIL_SEND_ENABLE_REGISTER_STATE means the processor is off,
 * but firmware has been loaded.
 * 
 * It is not clear why the EB_ENCODE_ENABLE bit is left on.
 * 
 * In the case of the HDNew device, after firmware load there are bits set
 * in the ENABLE_STATE_REGISTER that are not understood. The value of these
 * bits seems to be 0xd080. They are probably configured by the firmware on
 * the other side.
 *
 * These may or may not map into the ENABLE_REGISTER in some state, but their meaning is
 * unknown. We have not as yet seen these set in the ENABLE_REGISTER. 
 */
#define MAIL_SEND_ENABLE_REGISTER_STATE 0xbc, 0x0900, 0x0014 //2 byte read, write


/* HDNEW_INTERRUPT_STATUS 
 *
 * To be honest, I have no idea what this register does, but it is read after every 
 * usb interrupt, and unfailingly returns 0x09, 0x00. 
 *
 * It probably is an interrupt status specifying which interrupts
 * have been triggered. As such, bit 0 (or bit 4) likely signifies that
 * the interrupt was caused by what we wanted. Likely clears on read.
 * Be a good thing to test.
 *
 * This is only on GameCaptureHdNew devices. Only it uses interrupts anyways.
 */
#define HDNEW_INTERRUPT_STATUS 0xbc, 0x0800, 0x0016 //2 byte read

/* HDNEW_MAIL_READ 
 *
 * This is used on GameCaptureHDNew devices to read back requested data 
 *
 * This provides a slow asynchronous mechanism for reads
 * between the host and the device.
 *
 * The interface for this is much more complicated than the one for the
 * GameCaptureHD, but provides the same functionality.
 *
 * To performa a read:
 * 1. We use the HDNEW_MAIL_REQUEST_CONFIGURE register to set up a read.
 * 2. After the request, there will be an interrupt.
 * 3. Read HDNEW_INTERRUPT_DONE
 * 4. Make sure the read is complete via MAIL_REQUEST_READY
 * 5. Read the data from HDNEW_MAIL_READ. The size of the 
 *     read transaction must be: 
 *        2+(size requested), with extra 1 byte of padding if size is odd.
 *
 * The size of the read from this register is variable, but it is only
 * capable of reading in 2 byte increments. You configure the exact number of 
 * bytes you want to read via HDNEW_MAIL_REQUEST_CONFIGURE in
 * the state before, and the you read the data from this register.
 *  
 * The size read is 2+(size requested)+(1 byte padding if 
 * size requested was odd).
 *
 * The read input buffer is always prefixed by 2 bytes, which may possibly 
 * be status bytes, and postfixed (if necessary) with the extra padding
 * byte.
 */
#define HDNEW_MAIL_READ 0xbc, 0x0800, 0x23be //Variable size read.

/* HDNEW_MAIL_WRITE
 *
 * This is used on GameCaptureHDNew devices to write data 
 * asynchronously. 
 *
 * The interface for this is much more complicated than the one for the
 * GameCaptureHD, but provides the same functionality.
 * 
 * The size of the transaction is variable, but it is only
 * capable of writing in 2 byte words increments.  You send the data
 * with an extra byte of zero padding if necessary.
 *
 * To write we do the following:
 * 1. Write the data to HDNEW_MAIL_WRITE, with extra 1 byte of padding if 
 *    necessary.
 * 2. We use the HDNEW_MAIL_REQUEST_CONFIGURE register immediately
 *   after the write to set up what port we are writing to and the
 *   exact number of bytes written (so we can write odd numbered
 *   length transactions)
 * 3. After the request, there will be an interrupt.
 * 4. Read HDNEW_INTERRUPT_DONE
 * 5. Makee sure the write is complete via MAIL_REQUEST_READY, this
 *     also validates we are ready to read 2 dummy bytes.
 * 6. Read 2 dummy bytes via HDNEW_MAIL_READ. These possibly
 *    might contain status information on error.
 * You are done, but the subroutine that reads the mail, also
 * always updates the MAIL_SEND_ENABLE_REGISTER_STATE after it. 
 * And as mentioned there, that requires a MAIL_REQUEST_READY too.
 */
#define HDNEW_MAIL_WRITE 0xbc, 0x0800, 0x00c0 //Variable size write

/* HDNEW_MAIL_REQUEST_CONFIGURE
 * 
 * This register must be written to on GameCaptureHDNew devices
 * after using HDNEW_MAIL_WRITE (which is ready immediately),
 * and before reading HDNEW_MAIL_READ (which is ready
 * after an interrupt and we get all clear from MAIL_REQUEST_READY 
 * register.
 *
 * It is the write to this register that appears to trigger the 
 * usb interrupt.
 *
 * The write is always 4 bytes long.
 *    byte 0: This is always 0x09. and it is supsected that one of 
 *            the set bits there might be an interrupt enable.
 *    byte 1: This is the read byte. It is 0x1 if we are requesting a 
 *            read, and 0x0 if we just did a write.
 *    byte 2: This is the port number<<1. HDNew and HD share the same
 *            port numbers as long as you do this shift.
 *    byte 3: This is the exact byte size of the write or read.
 *            This is the exact amount we want to read, without extra
 *             padding
 *
 * Any time this register is used, it clears the status of 
 * MAIL_REQUEST_READY. It will generate an interrupt shortly before done,
 * and then poll on MAIL_REQUEST_READY for completion.
 */
#define HDNEW_MAIL_REQUEST_CONFIGURE 0xb9, 0x0000, 0x0000 //4 byte write

/* HD_MAIL_REGISTER 
 *
 * Read functionality:
 *   This is used on GameCaptureHD devices to read back data 
 *   via the "mail" asynchronously. Unlike HDNew, requesting
 *   a read doesn't require additional configuration, and the
 *   data comes back via the read response itself. There is
 *   no usb interrupt. The usb command won't complete
 *   till the data is ready, and there is no need to check
 *   MAIL_REQUEST_READY
 *
 *   To perform a read:
 *   1. Read the data from HD_MAIL_REGISTER. The size of the
 *      read transaction should be exactly the size of the data
 *      you expect back. No padding is necessary.
 *
 *      The "port" is encoded, in wValue (see end of comment for how), and 
 *      the size of the transaction is wLength.
 *
 * Write functionality:
 *   This same register is also used on GameCaptureHD devices to write
 *   back data asynchronously.
 *
 *   1. Wrote the data from HD_MAIL_REGISTER. The size of the
 *      write transaction should be exactly the size of the data
 *      you want to write. No padding is necessary.
 *
 *      The "port" is encoded, in wValue (see end of comment for how), and 
 *      the size of the transaction is wLength.
 *   
 *   2. Write MAIL_SEND_ENABLE_REGISTER_STATE with the current enable state.
 *      It is believe that this is the operation on the HD that actually 
 *      triggers the remote side to read the mail (and the
 *      MAIL_SEND_ENABLE_REGISTER_STATE).
 *
 *   3. Poll on MAIL_REQUEST_READY for completion.
 *      It is unknown whether you need to poll on MAIL_REQUEST_READY and/or
 *      then you must wait on it for completion, because the software
 *      always does the following.
 *
 * General:
 *   Note that only the usb bRequest and wValue are specified in this macro, as the wIndex 
 *   (which is the third parameter) must be set to:
 *       mail port number<<8
 */ 
#define HD_MAIL_REGISTER 0xbd, 0x0000 //Variable size read/write

/* MAIL_REQUEST_READY
 *
 * Bit 0 (lsb) of this appears to be 0 when the mail isn't ready, an 1 when it is.
 * There are other bits that fluctuate in this register, so probably some sort of status,
 * but none of those seem important.
 *
 * On HDNew, this must be polled for completion after:
 *   1. Any operation setup with HDNEW_MAIL_REQUEST_CONFIGURE
 *   2. Any write to MAIL_SEND_ENABLE_REGISTER_STATE
 *
 * On HD, this must be polled for completion after:
 *   1. Any write to HD_MAIL_REGISTER, which is always accompanied
 *      by a write to MAIL_SEND_ENABLE_REGISTER_STATE.
 *   2. Any write to MAIL_SEND_ENABLE_REGISTER_STATE.
 * So really, just after any write to MAIL_SEND_ENABLE_REGISTER_STATE
 *
 * Bits 8-9 of this register can be used to detect where signal is
 * coming from: 
 *   00=HDMI
 *   10=Component
 *   11=Composite
 */
#define MAIL_REQUEST_READY 0xbc, 0x0900, 0x001c //2 byte read

/* SCMD_REGISTER
 *
 * This is used to switch varius state of the device. such as going from idle to encode.
 * for more documentation, see the scmd() method of gchd in commands.cpp
 */
#define SCMD_REGISTER 0xb8, 0x0000, 0x0000   //6 byte write on HD, 4 byte write on HDNEW

// system commands
#define SCMD_IDLE		1
#define SCMD_RESET      2  //This is the state the device goes when we are done with it
                           //and the state we request to go to if firmware is already
                           //loaded, in lieu of loading formware. This 
                           //probably resets the firmware state.
#define SCMD_INIT		4
#define SCMD_STATE_CHANGE	5
#define SCMD_STATE_UNITIALIZED 0x0000 //Never written, read back via SCMD_STATE_READBACK_REGISTER
#define SCMD_STATE_STOP		0x0001
#define SCMD_STATE_START	0x0002
#define SCMD_STATE_NULL		0x0004

/* SCMD_STATE_READBACK_REGISTER
 *
 * This register can be used to readback the state of the device. 
 * It may take some time to update after a change request.
 *  See STATE_CHANGE_COMPLETION
 * bits 0-3:
 *    The SCMD_STATE, as set by SCMD_STATE_CHANGE 
 *    (this can be SCMD_STATE_UNINITIALIZED before STATE is set  
 *     to SCMD_STATE_STOP implicitly via switch to SCMD_IDLE state, which is done during
 *     device boot.
 *
 * bit 4
 *    1 if idling (SCMD_IDLE has been sent last)
 *    0 if not idling (SCMD_INIT has been sent last, or no loaded firmware).
 *
 * Once the idle firmware is loaded, this is set by the device to get 0x10.
 *
 * Then the driver code explicitly calls scmd(SCMD_IDLE, 0,0), which has the
 * side offect of causing the device to set  bits 0-3 to be 
 * SCMD_STATE_STOP.
 *
 * Since the only way to set bit 4 is to issue SCMD_IDLE this means:
 * 0x00 = only happens at boot
 * 0x10 = flash loaded, no SCMD_IDLE command sent.
 * 0x11 = IDLE mode. To enter IDLE, send SCMD_IDLE command. This will automatically 
 *         also set state to SCMD_STATE_STOP, so it is not possible for the low bit 4 to be 
 *         set and bits 0-3 to be anything but 1 after the first successful SCMD_IDLE command.         
 *         
 *         To leave idle an SCMD_INIT command is sent which transitions us to 0x01 (Not idle, but
 *         stopped)
 * 0x01 = Not idle, but stopped.
 * 0x02 = Stream started.
 * 0x04 = Null Stream started.
 * 0x12 = Special state entered when shutdown, reads as IDLING, STARTED, but really 
 *        I think is impossible value used to mean suspended/done.
 */ 
#define SCMD_STATE_READBACK_REGISTER 0xbc, 0x0800, 0x2008

/* SCMD_STATE_CHANGE_COMPLETE
 *
 * Not sure what other bits of this mean, but reading bit 2 (0x4)
 * signifies that a scmd triggered state change is complete.
 * After we do a scmd, we poll on this bit until it is set.....
 * When it is set we can read the SCMD_STATE_READBACK_REGISTER
 * To get the new state.
 *
 * I believe this bit is sticky, after each read that shows it is complee
 * the  driver writes bit 2 back (0x4) to clear and/or possibly acknowledge
 * that it has read the bit. This probably clears the bit, but that is not 
 * confirmed yet.
 */
#define SCMD_STATE_CHANGE_COMPLETE 0xbc, 0x0900, 0x0074

/* HDNEW_SCMD_READBACK_REGISTER 
 *
 * This is used to read back the scmd mode, (and another byte which 
 * seems to always come back 0)
 *
 * The first byte read back has the scmd mode..
 *
 * This appears to differ from SCMD_STATE_READBACK_REGISTER in that its
 * purpose is actually to verify completion of the send of 
 * the command, and that it gives the current mode, not the state.
 *
 * This appears to only be used in scmd() calls that trigger
 * an interrupt, see the scmd() code for more details.
 *
 * After this is confirmed, you may still have to poll
 * on SCMD_STATE_CHANGE_COMPLETE and SCMD_STATE_CHANGE_REGISTER
 * to verify that the state has fully changed.
 *
 * This only is used and may only exist for the GameCaptureHDNew device.
 */
#define HDNEW_SCMD_READBACK_REGISTER 0xbc, 0x0800, 0x0014  //2 byte read

/* BANKSEL
 *
 * I have no idea what this register does, presumably bank
 * selects between things. I only know the name of it 
 * do to a command found in script files.
 */
#define BANKSEL 0xbc, 0x0900, 0x0000
 
/* SEND_H264_TRANSCODER_WORD
 *
 * This is used to send a full 16 bit value to the 
 * Fujitsu mb86H58.  The wIndex passed after
 * this specifies an address of a register that must
 * be 2 byte aligned.  
 *
 * Writes to this register are done using the slsi function
 * to match the name of commands mentioned by name in scripts found
 * in Macintosh version of driver.
 */
#define SEND_H264_TRANSCODER_WORD 0xbc, 0x0000 //2 byte write
  
/* SEND_H264_TRANSCODER_BITFIELD
 * 
 * This is used to send a command to a register
 * on the Fujitsu mb86H58.  The wIndex passed after
 * this specifies an addres of a register that must
 * be 4 byte aligned. An 8 byte value is then sent
 * the first 4 bytes hold a value, and the second
 * 4 bytes hold a mask. Any register bit that is 0 in the mask
 * will not be affected by a write.
 *
 * Writes to this register are done using the sparam function
 * to match the name of commands mentioned by name in scripts found
 * in Macintosh version of driver.
 */
#define SEND_H264_TRANSCODER_BITFIELD 0xbc, 0x0001 //Its complicated. 8 byte write to set bit field in 4 byte register.

//Known transcoder bitfields. We don't have the data sheet...
//But we pulled these from some script files.
typedef struct
{
    uint16_t address;
    uint8_t lsb; //Least significant bit.
    uint8_t bits; //number of bits in field.
} bitfield_t;

namespace Transcoder
{
    static constexpr bitfield_t v_vinpelclk                        = {0x1002,  1,  1};
    static constexpr bitfield_t tbc_mode                           = {0x1002,  4,  1};
    static constexpr bitfield_t stoutclk                           = {0x1004,  0,  7};
    static constexpr bitfield_t stoutclk_io                        = {0x1004,  7,  1};
    static constexpr bitfield_t dma_sel_out                        = {0x1004,  8,  2};
    static constexpr bitfield_t stoutclk_hl                        = {0x1004, 10,  1};
    static constexpr bitfield_t tsout_packet_size                  = {0x1004, 11,  2};
    static constexpr bitfield_t extra_info_bit                     = {0x1004, 13,  2};
    static constexpr bitfield_t stout_mode                         = {0x1004, 15,  1};
    static constexpr bitfield_t stout2clk_io                       = {0x1006,  7,  1};
    static constexpr bitfield_t stout2clk_hl                       = {0x1006, 10,  1};
    static constexpr bitfield_t stout2_mode                        = {0x1006, 15,  1};
    static constexpr bitfield_t v_ts_out2_mode                     = {0x100a,  0,  4};
    static constexpr bitfield_t v_ts_out_mode                      = {0x100a,  4,  4};
    static constexpr bitfield_t sout2_mode                         = {0x100a,  8,  4};
    static constexpr bitfield_t sout_mode                          = {0x100a, 12,  4};
    static constexpr bitfield_t a_ts_out2_mode                     = {0x100c,  0,  4};
    static constexpr bitfield_t a_ts_out_mode                      = {0x100c,  4,  4};
    static constexpr bitfield_t a2_mode                            = {0x100c,  8,  4};
    static constexpr bitfield_t a_mode                             = {0x100c, 12,  4};
    static constexpr bitfield_t auto_null                          = {0x1102,  0,  2};
    static constexpr bitfield_t pcr_first                          = {0x1102,  2,  1};
    static constexpr bitfield_t system_rate                        = {0x1104,  0, 16};
    static constexpr bitfield_t system_min_rate                    = {0x1106,  0, 16};
    static constexpr bitfield_t initial_time_stamp_h               = {0x1108,  0, 16};
    static constexpr bitfield_t initial_time_stamp_l               = {0x110a,  0, 16};
    static constexpr bitfield_t initial_stc_h                      = {0x110c,  0, 16};
    static constexpr bitfield_t initial_stc_l                      = {0x110e,  0, 16};
    static constexpr bitfield_t initial_stc_ext                    = {0x1110,  0,  9};
    static constexpr bitfield_t initial_stc_ll                     = {0x1110, 15,  1};
    static constexpr bitfield_t v_pid_out                          = {0x1126,  0, 13};
    static constexpr bitfield_t a_pid_out                          = {0x1128,  0, 13};
    static constexpr bitfield_t pmt_pid_out                        = {0x112c,  0, 13};
    static constexpr bitfield_t pcr_pid_out                        = {0x112e,  0, 13};
    static constexpr bitfield_t v_sid_out                          = {0x1130,  0,  8};
    static constexpr bitfield_t a_sid_out                          = {0x1132,  0,  8};
    static constexpr bitfield_t sit_pid_out                        = {0x1134,  0, 13};
    static constexpr bitfield_t pmt_length                         = {0x1138,  0,  8};
    static constexpr bitfield_t pat_length                         = {0x1138,  8,  8};
    static constexpr bitfield_t pat_cyc                            = {0x113c,  0,  8};
    static constexpr bitfield_t pmt_cyc                            = {0x113e,  0,  8};
    static constexpr bitfield_t sit_length                         = {0x113e,  8,  8};
    static constexpr bitfield_t pcr_cyc                            = {0x1140,  0,  8};
    static constexpr bitfield_t sit_cyc                            = {0x1142,  0, 10};
    static constexpr bitfield_t auto_null_ts_out2                  = {0x1242,  0,  2};
    static constexpr bitfield_t pcr_first_ts_out2                  = {0x1242,  2,  1};
    static constexpr bitfield_t system_rate_ts_out2                = {0x1244,  0, 16};
    static constexpr bitfield_t system_min_rate_ts_out2            = {0x1246,  0, 16};
    static constexpr bitfield_t initial_time_stamp_h_ts_out2       = {0x1248,  0, 16};
    static constexpr bitfield_t initial_time_stamp_l_ts_out2       = {0x124a,  0, 16};
    static constexpr bitfield_t initial_stc_h_ts_out2              = {0x124c,  0, 16};
    static constexpr bitfield_t initial_stc_l_ts_out2              = {0x124e,  0, 16};
    static constexpr bitfield_t initial_stc_ext_ts_out2            = {0x1250,  0,  9};
    static constexpr bitfield_t initial_stc_ll_ts_out2             = {0x1250, 15,  1};
    static constexpr bitfield_t v_pid_out_ts_out2                  = {0x1266,  0, 13};
    static constexpr bitfield_t a_pid_out_ts_out2                  = {0x1268,  0, 13};
    static constexpr bitfield_t thumbnail_pid_out_ts_out2          = {0x126a,  0, 13};
    static constexpr bitfield_t pmt_pid_out_ts_out2                = {0x126c,  0, 13};
    static constexpr bitfield_t pcr_pid_out_ts_out2                = {0x126e,  0, 13};
    static constexpr bitfield_t v_sid_out_ts_out2                  = {0x1270,  0,  8};
    static constexpr bitfield_t a_sid_out_ts_out2                  = {0x1272,  0,  8};
    static constexpr bitfield_t sit_pid_out_ts_out2                = {0x1274,  0, 13};
    static constexpr bitfield_t pmt_length_ts_out2                 = {0x1278,  0,  8};
    static constexpr bitfield_t pat_length_ts_out2                 = {0x1278,  8,  8};
    static constexpr bitfield_t pat_cyc_ts_out2                    = {0x127c,  0,  8};
    static constexpr bitfield_t pmt_cyc_ts_out2                    = {0x127e,  0,  8};
    static constexpr bitfield_t sit_length_ts_out2                 = {0x127e,  8,  8};
    static constexpr bitfield_t pcr_cyc_ts_out2                    = {0x1280,  0,  8};
    static constexpr bitfield_t sit_cyc_ts_out2                    = {0x1282,  0, 10};
    static constexpr bitfield_t v_format                           = {0x1502,  0,  8};
    static constexpr bitfield_t vin_swap                           = {0x1502, 10,  1};
    static constexpr bitfield_t tfrff                              = {0x1504,  0,  2};
    static constexpr bitfield_t v_in_source_type                   = {0x1504,  8,  3};
    static constexpr bitfield_t v_rate_mode                        = {0x1510,  0,  2};
    static constexpr bitfield_t v_filler                           = {0x1510,  2,  1};
    static constexpr bitfield_t v_vlc_mode                         = {0x1510,  3,  1};
    static constexpr bitfield_t v_temporal_direct_on               = {0x1510,  4,  1};
    static constexpr bitfield_t v_cl_gop                           = {0x1510,  5,  1};
    static constexpr bitfield_t v_eos                              = {0x1510,  8,  2};
    static constexpr bitfield_t v_ip_format                        = {0x1512,  8,  7};
    static constexpr bitfield_t v_vbr_converge_mode                = {0x1514,  8,  3}; 
    static constexpr bitfield_t v_gop_struct                       = {0x1518,  0,  8};
    static constexpr bitfield_t v_gop_size                         = {0x1518,  8,  8};
    static constexpr bitfield_t v_idr_interval_h                   = {0x151a,  0, 16};
    static constexpr bitfield_t v_idr_interval_l                   = {0x151c,  0, 16};
    static constexpr bitfield_t v_h264_level                       = {0x1526,  0,  8};
    static constexpr bitfield_t v_h264_profile                     = {0x1526,  8,  8};
    static constexpr bitfield_t v_hsize_out                        = {0x152c,  0, 16};
    static constexpr bitfield_t v_vsize_out                        = {0x152e,  0, 16};
    static constexpr bitfield_t v_bitrate                          = {0x1532,  0, 16};
    static constexpr bitfield_t v_max_bitrate                      = {0x1534,  0, 16};
    static constexpr bitfield_t v_ave_bitrate                      = {0x1536,  0, 16};
    static constexpr bitfield_t v_min_bitrate                      = {0x1538,  0, 16};
    static constexpr bitfield_t v_filler_bitrate                   = {0x153a,  0, 16};
    static constexpr bitfield_t error_rate_ppic                    = {0x1552,  0,  8};
    static constexpr bitfield_t error_rate_ipic                    = {0x1552,  8,  8};
    static constexpr bitfield_t error_rate_bpic                    = {0x1554,  0,  8};
    static constexpr bitfield_t error_rate_refpic                  = {0x1554,  8,  8};
    static constexpr bitfield_t v_error_level_th_h                 = {0x1556,  0, 16};
    static constexpr bitfield_t v_error_level_th_l                 = {0x1558,  0, 16};
    static constexpr bitfield_t v_pic_order_present_flag           = {0x1570,  0,  1};
    static constexpr bitfield_t insert_recovery_point_sei          = {0x1570, 11,  1};
    static constexpr bitfield_t insert_pic_struct                  = {0x1570, 14,  1};
    static constexpr bitfield_t insert_buf_ctrl_param              = {0x1570, 15,  1};
    static constexpr bitfield_t v_max_cpb_delay                    = {0x1574,  0,  5};
    static constexpr bitfield_t insert_restriction                 = {0x157c, 11,  1};
    static constexpr bitfield_t overscan_appropriate_flag          = {0x157c, 14,  1};
    static constexpr bitfield_t overscan_info_present_flag         = {0x157c, 15,  1};
    static constexpr bitfield_t video_full_range_flag              = {0x157e,  0,  1};
    static constexpr bitfield_t video_format                       = {0x157e,  4,  3};
    static constexpr bitfield_t video_signal_type_present_flag     = {0x157e, 12,  1};
    static constexpr bitfield_t colour_primaries                   = {0x1580,  0,  8};
    static constexpr bitfield_t colour_description_present_flag    = {0x1580, 12,  1};
    static constexpr bitfield_t matrix_coefficients                = {0x1582,  0,  8};
    static constexpr bitfield_t transfer_characteristics           = {0x1582,  8,  8};
    static constexpr bitfield_t time_scale_h                       = {0x1584,  0, 16};
    static constexpr bitfield_t time_scale_l                       = {0x1586,  0, 16};
    static constexpr bitfield_t num_units_in_tick                  = {0x1588,  0, 16};
    static constexpr bitfield_t v_disable_aspect_ratio_info_present_flag = {0x158a, 12,  1};
    static constexpr bitfield_t v_rate_mode_bl                     = {0x1610,  0,  2};
    static constexpr bitfield_t v_filler_bl                        = {0x1610,  2,  1};
    static constexpr bitfield_t v_vlc_mode_bl                      = {0x1610,  3,  1};
    static constexpr bitfield_t v_cl_gop_bl                        = {0x1610,  5,  1};
    static constexpr bitfield_t v_eos_bl                           = {0x1610,  8,  2};
    static constexpr bitfield_t v_ip_format_bl                     = {0x1612,  8,  7};
    static constexpr bitfield_t v_vbr_converge_mode_bl             = {0x1614,  8,  3}; 
    static constexpr bitfield_t v_gop_struct_bl                    = {0x1618,  0,  8};
    static constexpr bitfield_t v_gop_size_bl                      = {0x1618,  8,  8};
    static constexpr bitfield_t v_idr_interval_h_bl                = {0x161a,  0, 16};
    static constexpr bitfield_t v_idr_interval_l_bl                = {0x161c,  0, 16};
    static constexpr bitfield_t v_h264_level_bl                    = {0x1626,  0,  8};
    static constexpr bitfield_t v_h264_profile_bl                  = {0x1626,  8,  8};
    static constexpr bitfield_t v_hsize_out_bl                     = {0x162c,  0, 16};
    static constexpr bitfield_t v_vsize_out_bl                     = {0x162e,  0, 16};
    static constexpr bitfield_t v_bitrate_bl                       = {0x1632,  0, 16};
    static constexpr bitfield_t v_max_bitrate_bl                   = {0x1634,  0, 16};
    static constexpr bitfield_t v_ave_bitrate_bl                   = {0x1636,  0, 16};
    static constexpr bitfield_t v_min_bitrate_bl                   = {0x1638,  0, 16};
    static constexpr bitfield_t v_filler_bitrate_bl                = {0x163a,  0, 16};
    static constexpr bitfield_t v_pic_order_present_flag_bl        = {0x1670,  0,  1};
    static constexpr bitfield_t insert_recovery_point_sei_bl       = {0x1670, 11,  1};
    static constexpr bitfield_t insert_pic_struct_bl               = {0x1670, 14,  1};
    static constexpr bitfield_t insert_buf_ctrl_param_bl           = {0x1670, 15,  1};
    static constexpr bitfield_t v_max_cpb_delay_bl                 = {0x1674,  0,  5};
    static constexpr bitfield_t insert_restriction_bl              = {0x167c, 11,  1};
    static constexpr bitfield_t overscan_appropriate_flag_bl       = {0x167c, 14,  1};
    static constexpr bitfield_t overscan_info_present_flag_bl      = {0x167c, 15,  1};
    static constexpr bitfield_t video_full_range_flag_bl           = {0x167e,  0,  1};
    static constexpr bitfield_t video_format_bl                    = {0x167e,  4,  3};
    static constexpr bitfield_t video_signal_type_present_flag_bl  = {0x167e, 12,  1};
    static constexpr bitfield_t colour_primaries_bl                = {0x1680,  0,  8};
    static constexpr bitfield_t colour_description_present_flag_bl = {0x1680, 12,  1};
    static constexpr bitfield_t matrix_coefficients_bl             = {0x1682,  0,  8};
    static constexpr bitfield_t transfer_characteristics_bl        = {0x1682,  8,  8};
    static constexpr bitfield_t time_scale_h_bl                    = {0x1684,  0, 16};
    static constexpr bitfield_t time_scale_l_bl                    = {0x1686,  0, 16};
    static constexpr bitfield_t num_units_in_tick_bl               = {0x1688,  0, 16};
    static constexpr bitfield_t v_disable_aspect_ratio_info_present_flag_bl = {0x168a, 12,  1};
    static constexpr bitfield_t v_v420                             = {0x1710, 12,  2};
    static constexpr bitfield_t a_sample                           = {0x1a02,  0,  2};
    static constexpr bitfield_t a_bitrate                          = {0x1a04,  0, 16};
    static constexpr bitfield_t ach_sel                            = {0x1a06,  0,  1};
    static constexpr bitfield_t ain_master                         = {0x1a08,  0,  1};
    static constexpr bitfield_t ain_lsb_first                      = {0x1a08,  1,  1};
    static constexpr bitfield_t ain_bit_posb                       = {0x1a08,  2,  1};
    static constexpr bitfield_t amck_mode                          = {0x1a08,  3,  1};
    static constexpr bitfield_t ain_dlength                        = {0x1a08,  4,  2};
    static constexpr bitfield_t ain_lr_hlset                       = {0x1a08,  6,  1};
    static constexpr bitfield_t ain_i2s                            = {0x1a08,  7,  1};
    static constexpr bitfield_t ain_lrclk_sel                      = {0x1a08,  8,  3};
    static constexpr bitfield_t ain_sclk_sel                       = {0x1a08, 11,  2};
    static constexpr bitfield_t ain_bclk_sel                       = {0x1a08, 13,  1};
    static constexpr bitfield_t ain_bclk_hl                        = {0x1a08, 14,  1};
    static constexpr bitfield_t ain_offset_mode                    = {0x1a08, 15,  1};
    static constexpr bitfield_t ain_offset                         = {0x1a0c,  0, 16};
    static constexpr bitfield_t e_mpeg_protect                     = {0x1a16,  0,  1};
    static constexpr bitfield_t e_mpeg_mode                        = {0x1a16,  1,  2};
    static constexpr bitfield_t e_mpeg_copyr                       = {0x1a16,  3,  1};
    static constexpr bitfield_t e_mpeg_orig                        = {0x1a16,  4,  1};
    static constexpr bitfield_t e_mpeg_emp                         = {0x1a16,  5,  2};
    static constexpr bitfield_t a_bitrate_bl                       = {0x1a24,  0, 16};
    static constexpr bitfield_t ach_sel_bl                         = {0x1a26,  0,  1};
    static constexpr bitfield_t e_mpeg_protect_bl                  = {0x1a36,  0,  1};
    static constexpr bitfield_t e_mpeg_mode_bl                     = {0x1a36,  1,  2};
    static constexpr bitfield_t e_mpeg_copyr_bl                    = {0x1a36,  3,  1};
    static constexpr bitfield_t e_mpeg_orig_bl                     = {0x1a36,  4,  1};
    static constexpr bitfield_t e_mpeg_emp_bl                      = {0x1a36,  5,  2};
    static constexpr bitfield_t e_aac_protect_abs                  = {0x1a1c,  0,  1};
    static constexpr bitfield_t e_aac_profile                      = {0x1a1c,  1,  2};
    static constexpr bitfield_t e_aac_ch_config                    = {0x1a1c,  3,  2};
    static constexpr bitfield_t e_aac_orig                         = {0x1a1c,  5,  1};
    static constexpr bitfield_t e_aac_pce                          = {0x1a1c,  8,  2};
    static constexpr bitfield_t e_aac_header_format                = {0x1a1c, 11,  2};
    static constexpr bitfield_t e_aac_mpeg4                        = {0x1a1c, 13,  1};
    static constexpr bitfield_t e_aac_max_bitrate                  = {0x1a1e,  0, 16};
    static constexpr bitfield_t e2_aac_protect_abs                 = {0x1a3c,  0,  1};
    static constexpr bitfield_t e2_aac_profile                     = {0x1a3c,  1,  2};
    static constexpr bitfield_t e2_aac_ch_config                   = {0x1a3c,  3,  2};
    static constexpr bitfield_t e2_aac_orig                        = {0x1a3c,  5,  1};
    static constexpr bitfield_t e2_aac_pce                         = {0x1a3c,  8,  2};
    static constexpr bitfield_t e2_aac_header_format               = {0x1a3c, 11,  2};
    static constexpr bitfield_t e2_aac_mpeg4                       = {0x1a3c, 13,  1};
    static constexpr bitfield_t lch_scale                          = {0x1a64,  0, 16};
    static constexpr bitfield_t rch_scale                          = {0x1a66,  0, 16};
    static constexpr bitfield_t lsch_scale                         = {0x1a68,  0, 16};
    static constexpr bitfield_t rsch_scale                         = {0x1a6a,  0, 16};
    static constexpr bitfield_t cch_scale                          = {0x1a6c,  0, 16};
    static constexpr bitfield_t lfech_scale                        = {0x1a6e,  0, 16};


    //In addition to the table above the following are thought to be true:
    // [0x1010 - ??? ) = SIT Table
    // [0x1144 - 0x1174) = PAT table
    // [0x1174 - ???? ) = PMT table
    // [0x1284 - 0x12B4) = PAT table ts_out2
    // [0x12b4 - ???? ) = PMT TABLE ts_out2
    // [0x1370 - ???? ) =SIT Table ts_out2
    static const unsigned sit_table_start         = 0x1010;
    static const unsigned sit_table_ts_out2_start = 0x1370;

    static const unsigned pat_table_start         = 0x1144;
    static const unsigned pat_table_ts_out2_start = 0x1284;

    static const unsigned pmt_table_start         = 0x1174;
    static const unsigned pmt_table_ts_out2_start = 0x12b4;
};


#endif
