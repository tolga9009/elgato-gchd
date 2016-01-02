Elgato Game Capture HD Linux driver
===================================

This project provides a userspace driver to support the Elgato Game Capture HD
under Linux. This is an unofficial driver and therefore not supported by Elgato.

Use at your own risk! This software is not intended for production use.


Firmware
========

This software needs Elgato Game Capture HD firmware files to work. They're not
part of this repository, so you need to extract them yourself. A brief
instruction:

1. Download Elgato Game Capture HD Mac OS X drivers version 2.0.3 from their
official website: https://www.elgato.com/de/game-capture-hd-support

2. Install `dmg2img` and `hfsprogs`. Please refer to your specific Linux
distribution for more information.

3. Uncompress Elgato Game Capture HD Mac OS X drivers:

    ```
    dmg2img gchdm_203_970.dmg -o gchdm_203_970.dmg.img
    ```

4. Mount the uncompressed HFS+ image with root permissions:

    ```
    mkdir /tmp/dmg
    mount -o loop -t hfsplus gchdm_203_970.dmg.img /tmp/dmg
    ```

5. Copy `mb86h57_h58_idle.bin` and `mb86h57_h58_enc_h.bin` from
`/tmp/dmg/Game\ Capture\ HD.app/Contents/Resources/Firmware/Beddo/`.

6. Place the firmware files into this project's `firmware` folder.


Install
=======

1. Install the following dependencies. Please refer to your specific Linux
distribution, as package names might differ.

  * libusb >= 1.0.20
  * clang (make)
  * make (make)

2. Compile the driver:

    ```
    make
    ```


Usage
=====

According to your source, run `./elgato-gchd -r <resolution>`, whereas
`<resolution>` can be `1080p` or `720p` for a HDMI source, `c1080p`, `c1080i`,
`c720p` or `c576p` for a component source or `576i` for a composite PAL source.

Example: `./elgato-gchd -r 1080p` for HDMI 1080p60 source. This command will
create a new file `/tmp/elgato-gchd.ts`.

Please note, that this file is actually a FIFO pipe and will not grow. You will
need an external program to record footage.

You can open up this file using any media player, which supports reading from
pipes (e.g. VLC or obs-studio). There will be a slight delay, which is hardware
limited and can not be worked around.

If you're done using this driver, close the file. Then stop the terminal using
"Ctrl + C" and wait for the program to successfully terminate. The driver is
resetting your device, it may take a while. If you interrupt this step, it will
leave your device in an undefined state and you will need to manually reset your
device by reconnecting it.

Currently supported input sources:

* HDMI: 1080p60 (output 1080p30), 720p60
* Component: 576p50 (PAL), 720p60, 1080i60, 1080p60 (output 1080p30)
* Composite: 576i50 (PAL)


Contributors
============

- Join our official Groupchat at Gitter: https://gitter.im/tolga9009/elgato-gchd
- Special thanks to jedahan for writing openvizsla2c utility


License
=======

This driver is made available under the MIT License.
