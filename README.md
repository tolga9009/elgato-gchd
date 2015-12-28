Elgato Game Capture HD - Linux drivers
======================================

[![Join the chat at https://gitter.im/tolga9009/elgato-gchd](https://badges.gitter.im/tolga9009/elgato-gchd.svg)](https://gitter.im/tolga9009/elgato-gchd?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Reverse engineering the Elgato Game Capture HD to make it work under Linux.

We all love our Elgatos, we all love our Linux machines. I've been in contact
with Elgato several times and talked to them about this topic, but they don't
plan adding Linux support and they also can't help Linux developers, due to NDA
restrictions. As a result, they can't support any reverse engineering efforts.

Everyone who can help us is welcome, it doesn't matter if you want to program,
improve the documentation or research. Please note, that this driver is in a
very early development stage and not ready for use.

Currently, it's possible to receive a 720p60 HDMI input and save it to a file.

Please note, that we can't take any feature requests, as we are currently in the
process of figuring out, how the device actually works.


Current TODOs
=============

- implement sparam, statechange and boot functions
- figure out i2c write and i2c stat functions
- 1080p support


About the device
================

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

Steven Toth from Kernel Labs noted, that the device is outputting native
ISO13818 TS packets. Once the device is set up, we should be able to get the
video stream without any encoding. Thank you Steve for the hint!


What we need and how you can help
=================================

The official repository of this driver can be found at
https://github.com/tolga9009/elgato-gchd. This is our primary platform for
sharing information. If you are willing to help, please get in touch with us.
