Elgato Game Capture HD Linux driver
===================================

This project provides a userspace driver to support the Elgato Game Capture HD
under Linux. This is an unofficial driver and therefore not supported by Elgato.

Use at your own risk! This software is not intended for production use.


Install
=======

Install the following dependencies. Please refer to your specific Linux
distribution, as package names might differ.

- libusb >= 1.0.20
- clang (make)
- make (make)

Compile the driver:

`make`


Usage
=====

According to your HDMI source, run `./elgato-gchd -r 720p` or
`./elgato-gchd -r 1080p` with root permissions in a terminal and leave it open.
This driver will create a new file `/tmp/elgato-gchd.ts`.

Please note, that this file is actually a FIFO pipe and will not grow. You will
need an external program to record footage onto your harddisk.

You can open up this file using any media player, which supports reading from
pipes. There will be a slight delay, which is hardware limited and can not be
worked around.

After closing the file, you will not be able to reopen it again. You will need
to stop the terminal using "Ctrl + C", disconnect and reconnect your Elgato Game
Capture HD's USB cable and start over again.

Currently, 1080p30 and 720p60 are supported.


Contributors
============

- Join our official Groupchat at Gitter: https://gitter.im/tolga9009/elgato-gchd
- Special thanks to jedahan for writing openvizsla2c utility


License
=======

Elgato Game Capture HD Linux driver is made available under the MIT License.
