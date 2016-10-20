# Game Capture HD Linux driver

This project provides a userspace driver to natively support the Elgato Game
Capture HD under Linux and Mac OS X. This is an unofficial driver and in no way
supported by Elgato.

Use at your own risk! This software is experimental and not intended for
production use.

Official Groupchat at Gitter: https://gitter.im/tolga9009/elgato-gchd


### Supported devices

* Elgato Game Capture HD

### Unsupported devices

* Elgato Game Capture HD60 (work in progress)
* Elgato Game Capture HD60 S
* Elgato Game Capture HD60 Pro


## Firmware

This software needs Elgato Game Capture HD firmware files to work. Due to
licensing issues, they're not part of this repository.

You need to extract them yourself from the Windows "Game Capture" software,
or the official Mac OS X drivers. Getting it from each is a different process.

See https://github.com/tolga9009/elgato-gchd/wiki/Firmware

**Note**: If you're a Mac OS X user, simply install the official drivers. You
don't need to manually extract the firmware files.

## Install

1. Install the following dependencies. Please refer to your specific Linux
distribution, as package names might differ.

* libusb >= 1.0.20
* cmake (make)
* make (make)
* qt5 (optional) - for GUI support (not usable, work in progress)

2. Compile the driver:

1. Either clone or download the Git repository and extract it.

2. Open up a terminal inside the project's root directory.

3. Create a new directory `build` in the project's root directory and
navigate into it:

```
mkdir build
cd build
```

4. Run CMake from inside the `build` directory to setup the make
environment and compile the driver:

```
cmake ..
make
```

5. The compiled executable `gchd` is located in `build/src`. If you have
Qt5 installed on your system, the GUI `qgchd` will be located at
`build/src/gui`.
**Note**: you can copy the firmware files into these directories and test
the application, without making any system-wide modifications.

3. If the application works for you, you can optionally install it system-wide,
running `make install` from within the `build` directory. This will install the
executables to `/usr/bin`.


## Usage

### Commandline

```
Usage:
./gchd [options] [<destination>]
For `disk` and `fifo` output formats <destination> is a filename.
For the `socket` output format, <destination> is [<ip address>][:<port>].

The default for `fifo` is `/tmp/gchd.ts`.
The default for `socket` is `0.0.0.0:57384`
There is no default for `disk`, <destination> is required.

Input Options:
-i, -input <input-source>
Set input source to `hdmi`, `component`, `composite` or `auto` (default)
With no input, or multiple input types `auto` may detect incorrectly.

-c, -color-space <color-space>
Set the input color space to `yuv`, `rgb`, or `auto` (default).
Only has meaning for hdmi and component input.

Output Options:
-of, -output-format <format>
Format is `disk`, `socket, or `fifo`. `disk` is default if a
<destination> file is specified, otherwise the default is `fifo`

-or, -output-resolution <resolution>
Output resolution can be `ntsc`, `pal`, `720`, `1080`, or `auto`.
`auto` (default) matches input resolution. `480` and `576` can be used
instead of `ntsc` and `pal`. You can also use <xres>x<yres>, IE 720x480.

-br, -bit-rate <mbit-rate>
<mbit-rate> is either `auto` which will do relatively high quality output,
or a bit rate number in mbps. You can do variable bit rate by specifying
the bit rate in a [max]:[average]:[min] format.

General Options:
-h, -?, -help
Displays commonly used switches

-hh, -??, -full-help
Displays extended help
```

Options for `<input-source>` are `composite`, `component` and `hdmi`. Choose,
whichever source you're using. Some resolutions are not available on all input
sources. If you do not specify the input source, the driver will attempt
to autodetect.

Options for `<color-space>` are `yuv` and `rgb`. Consoles and PCs output in
either format and usually don't support switching Color Spaces. If this option
is set incorrectly, your capture will either have a green or purple tint. The
autodetection for this is not currently working.

Options for `<format>` are `disk`, `fifo` and `socket`. Use `disk`, if you want
to directly record to your harddrive. Else, FIFO will cover almost all use cases
(default). Please note, that FIFOs won't grow in size, making them optimal for
streaming and more controlled capturing on systems with limited amount of memory
or SSDs. When set to `socket`, this driver will stream the output via UDP.

You can specify the UDP ip address and port to bind to for for udp streaming
via the `<destination> field passed on the command line.

Please note, UDP streaming is highly experimental at this point. Unicast works
well, but Multicasting has performance issues, causing artifacts. Multicast IPs
are in the range of `224.0.0.0` - `239.255.255.255` (RFC 5771).


### General

This driver must be run as root, as it needs to access your Game Capture HD
device. Under Linux, you can alternatively follow the directions at:

https://github.com/tolga9009/elgato-gchd/wiki/Configuring-the-Driver-to-be-Run-Without-Root-Permissions

This will make it accessible to non-root users.

If no commandline options are set, the device will autodetect what source you
have, HDMI, Component, or Composite. It will also autodetect the streamed
resolution and colorspace. It will not however handle these changing on the fly.
The detection for whether you are using HDMI/Component/Composite may malfunction
in the following cases:

* There is no signal at all. It often will detect no signal as a signal of
various types.
* Multiple cables are connected between devices, IE: HDMI and Composite. Just
being connected through to another device, even one that is off, can mess up
the autodetection.
* Multiple signals being sent simultaneously.

By default, a FIFO will be created at `/tmp/gchd.ts`. You can open it up using
any media player, which supports reading from pipes (e.g. VLC or obs-studio).
There will be a slight delay, which is a hardware limitation and can't be worked
around.

If you're done using this driver, close the file, stop the terminal using
"Ctrl + C" and wait for the program to successfully terminate. The driver will
reset your device. If you interrupt this step, it will leave your device in
an undefined state and you will need to manually reset your device by
reconnecting it.

Currently supported input sources:

* HDMI: 480p60 (NTSC), 576p60 (PAL), 720p60, 1080i60, 1080p60
* Component: 480i60 (NTSC), 480p60 (NTSC), 576i50 (PAL), 576p50 (PAL), 720p60,
1080i60, 1080p60
* Composite: 480i60 (NTSC), 576i50 (PAL)


## License

This project is made available under the MIT License. For more information,
please refer to the LICENSE file.
