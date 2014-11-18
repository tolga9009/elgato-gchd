Elgato Game Capture HD - Linux drivers
======================================

Reverse engineering the Elgato Game Capture HD to make it work under Linux.

We all love out Elgatos, we all love our Linux machines. I've been in contact
with Elgato several times and talked with them about this topic; they don't plan
on adding Linux support and they also can't help Linux developers, due to NDA
restrictions.

So, the only chance is by getting our hands dirty and reverse engineer it on our
own. Everyone who can help us is welcome, it doesn't matter if you want to
program, improve the documentation, research, provide webspace or simply test
and give us feedback.


Download USB Logs and Docs
==========================

v1.0 as 7zip:   https://docs.google.com/file/d/0B29z6-xPIPLEQVBMTWZHbUswYjg  
v1.0 as rar:    https://docs.google.com/file/d/0B29z6-xPIPLEcENMWnh1MklPdTQ  
v1.0 as zip:    https://docs.google.com/file/d/0B29z6-xPIPLEQWtibWk3T3AtVjA

Due to the fact, that GitHub is not made for huge binary files, we will keep
track of such files on external / private hosters. Google Drive is fast, offers
enough space and is easily accessable. If there are any files to add, please
upload it somewhere and give me the link. I will add the file, repackage the
archive, increment the version and update the links here.


First Step
==========

In order to write a driver, we need to fully understand, how the device works.
I have now got my hands on a working OpenVizsla USB 2.0 analyzer and was able to
capture all necessary USB packets. What we found out: the driver is uploading
two firmwares onto the device, after the device has been configured using USB
control transfers. Our job is to mimic this behaviour.


How the device works
============================

After studying the video output and USB packets, the Elgato Game Capture HD
seems to be one hell of device! This little thing directly outputs compressed
video and audio data, in the following format:

    General
    Format                         : MPEG-TS
    Overall bit rate mode          : Variable
    Overall bit rate               : 22.4 Mbps
    
    Video
    ID                             : 100 (0x64)
    Menu ID                        : 2 (0x2)
    Format                         : AVC
    Format/Info                    : Advanced Video Codec
    Format profile                 : High@L4.0
    Format settings, CABAC         : Yes
    Format settings, ReFrames      : 2 frames
    Codec ID                       : 27
    Duration                       : 12s 217ms
    Bit rate mode                  : Variable
    Bit rate                       : 21.1 Mbps
    Maximum bit rate               : 30.0 Mbps
    Width                          : 1 280 pixels
    Height                         : 720 pixels
    Display aspect ratio           : 16:9
    Frame rate                     : 60.000 fps
    Color space                    : YUV
    Chroma subsampling             : 4:2:0
    Bit depth                      : 8 bits
    Scan type                      : Progressive
    Bits/(Pixel*Frame)             : 0.382
    Stream size                    : 30.7 MiB (92%)
    Color primaries                : BT.709
    Transfer characteristics       : BT.709
    Matrix coefficients            : BT.709
    
    Audio
    ID                             : 101 (0x65)
    Menu ID                        : 2 (0x2)
    Format                         : AAC
    Format/Info                    : Advanced Audio Codec
    Format version                 : Version 4
    Format profile                 : LC
    Muxing mode                    : ADTS
    Codec ID                       : 15
    Duration                       : 12s 245ms
    Bit rate mode                  : Variable
    Bit rate                       : 220 Kbps
    Channel(s)                     : 2 channels
    Channel positions              : Front: L R
    Sampling rate                  : 48.0 KHz
    Compression mode               : Lossy
    Delay relative to video        : -333ms
    Stream size                    : 329 KiB (1%)

All we have to do is simply writing all raw USB packets to disk, assemble them
together and write a working file header. Making the Elgato Game Capture HD work
with Linux seems quite trivial and nearly too easy to be true, but these are my
current findings.

Steven Toth from Kernel Labs noted, that the device is outputting native
ISO13818 TS packets. Once the device is set up, we should be able to get the
video stream without any encoding. Thank you Steve for the hint!


Drivers
=======

Currently, we're stuck at loading the firmware onto the device. For some reason,
the Elgato device only accepts the first 1536 Bytes of a bulk transfer, then
closes endpoint 0x02 (OUT) and sends out a NYET signal (no response yet).
Usually, this is perfectly normal and actually happens on the Windows driver
aswell. However, the bulk transfer function doesn't return on our Linux setup. I
have a few ideas, what might be the problem. But this is something, were you
could help me. Here are some ideas:

- the Elgato's main processor is not correctly set to accept a firmware using
USB control transfers. So, after the FIFO buffer (1536 Bytes) is full, the
Elgato locks endpoint 0x02 (OUT).

- it's a libusb problem. Might simply go away, when writing a kernel driver,
using the kernel USB API. According to OpenVizsla's output, we're sending
correct packets, so there might be simply something wrong with libusb.

Loading the firmware onto the device doesn't brick it (as far as I know so far).
It's more like an FPGA, which needs to have it's firmware loaded everytime it
boots up.


What we need and how you can help
=================================

The primary platform, where we share our efforts, will be GitHub. If you're
interested, just shoot us a message and tell us, what you want to do and we will
add you ;). This project surely can't be realized by one person in a reasonable
amount of time.
