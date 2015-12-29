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

Run `./elgato-gchd` with root permissions in a terminal and leave it open. This
driver will create a new file `/tmp/elgato-gchd.ts`.

Please note, that this file is actually a FIFO pipe and will not grow. You will
need an external program to record footage onto your harddisk.

You can open up this file using any media player, which supports reading from
pipes. There will be a slight delay, which is hardware limited and can not be
worked around.

After closing the file, you will not be able to reopen it again. You will need
to stop the terminal using "Ctrl + C", disconnect and reconnect your Elgato Game
Capture HD's USB cable and start over again.

Currently, only 720p source is supported.


Contributors
============

- Join our official Groupchat at Gitter: https://gitter.im/tolga9009/elgato-gchd
- Special thanks to jedahan for writing openvizsla2c utility


License
=======

Provided firmwares have been extracted from Elgato's official Mac OS X
drivers. Please refer to https://www.elgato.com for more information.

Elgato Game Capture HD Linux driver is made available under the MIT License.
