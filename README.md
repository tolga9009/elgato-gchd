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


First Step
==========

In order to write a driver, we need to fully understand, how the device works.
I have now got my hands on a working OpenVizsla USB 2.0 analyzer and was able to
capture all necessary USB packets. Our job is to mimic the traffic between the
GCHD and the host.


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

Steven Toth from Kernel Labs noted, that the device is outputting native
ISO13818 TS packets. Once the device is set up, we should be able to get the
video stream without any encoding. Thank you Steve for the hint!

However, the difficult part is to setup the device in order to stream the data.
Everything we have are multiple capture logs, which we need to study and
understand. You can find the latest capture logs under issue #2 at GitHub. If
you find anything interesting, please let us know.


Drivers
=======

Currently, we've fully implemented the transfer of both binary files. The next
step is to figure out, how the host is configuring the GCHD and receiving audio
and video.


What we need and how you can help
=================================

The primary platform, where we share our efforts, will be GitHub. If you're
interested, just shoot us a message and tell us, what you want to do and we will
add you ;). This project surely can't be realized by one person in a reasonable
amount of time.
