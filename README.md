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

**Note**: If you're a Mac OS X user, simply install the official drivers. You
don't need to manually extract the firmware files.

## Getting Firmware from Windows "Game Capture" software:

1. Download Game Capture from:

https://www.elgato.com/en/game-capture-hd-support

Game Capture 3.2 is known to work. You will not need the hardware driver,
just the Game Capture software (which is a much bigger download).

2. Make sure p7zip is installed on your Linux box. On Debian based systems
this can be achieved with the following command (as root).

```
apt-get install p7zip-full
```

3. Copy/Move the GameCapture*.msi file to the
firmware_extract directory in this tree.

4. Change directory to firmware_extract and then run:

```
./extract_firmware_windows
```

This will create a "Firmware.tgz" file.

5. Change directory to either /usr/lib/firmware or /usr/local/lib/firmware
You may need to be root.

6. Untar Firmware.tgz from one of those directories. You will probably
need to be root.

```
tar xvf /full/path/to/elgato-gchd/firmware_extract/Firmware.tgz
```

7. The firmware files should end up in `/usr/lib/firmware/gchd` or
`/usr/local/lib/firmware/gchd` directories.

## Getting Firmware from Macintosh Driver:

1. Download Elgato Game Capture HD Mac OS X driver version 2.0.3 from official
website: https://www.elgato.com/en/game-capture-hd-support

2. Install the tools to loopback mount the gchd_*.dmg file.
There are two ways to do this:
A) Using `dmg2img` and `hfsprogs`. These tools currently don't work
properly on .dmg files create on later versions of OSX. So you will
want to make sure you get the 2.0.3 version of the driver for this.
B) Install `darling-dmg` from https://github.com/darlinghq/darling-dmg
and use it to do this step. This is a much better method, but
darling-dmg isn't currently available as a package for most
Linux distros.

3. Once the tools are installed, mount .dmg file.
For method A:

3A.1. Uncompress Elgato Game Capture HD Mac OS X drivers:

```
dmg2img gchdm_203_970.dmg -o gchdm_203_970.dmg.img
```

3A.2 Mount the uncompressed HFS+ image with root permissions:

```
mkdir /tmp/dmg
mount -o loop -t hfsplus gchdm_203_970.dmg.img /tmp/dmg
```

For method B:

3B.1 Mount the HFS+ image directly. Root permission not required.
```
mkdir /tmp/dmg
darling-dmg gchdm_203_970.dmg /tmp/dmg
```

4. Copy `mb86h57_h58_idle.bin`, `mb86h57_h58_enc_h.bin`,
`mb86m01_assp_nsec_idle.bin`, and `mb86m01_assp_nsec_enc_h.bin`
from `/tmp/dmg/Game\ Capture\ HD.app/Contents/Resources/Firmware/Beddo/`.

6. Place the firmware files either in `/usr/lib/firmware/gchd` or
`/usr/local/lib/firmware/gchd` folder. You might need root permissions.

**Note**: for testing purposes, you can also place the firmware files in the
same directory, where the compiled executable `gchd` is located.


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
Usage: gchd [options]

Options:
-c <color-space>   Color Space settings [default: autodetect]
-f <format>        Format for <output> [default: fifo]
-h                 Print this screen
-i <input-source>  Input Source [default: autodetect]
-n <ip-address>    IP address for UDP streaming [default: 0.0.0.0]
-o <output>        Output Path [default: /tmp/gchd.ts]
-p <port>          Port for UDP streaming [default: 57384]
-r <resolution>    Output resolution [default: same as input source]
-v                 Print program version
-P <pid-path>      PID path [default: /var/run/gchd.pid]
```

Options for `<color-space>` are `yuv` and `rgb`. Consoles and PCs output in
either format and usually don't support switching Color Spaces. If this option
is set incorrectly, your capture will either have a green or purple tint.
If this is not set the driver will attempt to autodetect.

Options for `<format>` are `disk`, `fifo` and `socket`. Use `disk`, if you want
to directly record to your harddrive. Else, FIFO will cover almost all use cases
(default). Please note, that FIFOs won't grow in size, making them optimal for
streaming and more controlled capturing on systems with limited amount of memory
or SSDs. When set to `socket`, this driver will stream the output via UDP to
`<ip-address>`:`<port>` (default `0.0.0.0:57384`).

Options for `<input-source>` are `composite`, `component` and `hdmi`. Choose,
whichever source you're using. Some resolutions are not available on all input
sources. If you do not specify the input source, the driver will attempt
to autodetect.

You can specify `<ip-address>` using `-n` option. The driver will stream to this
IP address, instead of default `0.0.0.0`. Please note, UDP streaming is highly
experimental at this point. Unicast works well, but Multicasting has performance
issues, causing artefacts. Multicast IPs are in the range of `224.0.0.0` -
`239.255.255.255` (RFC 5771).

`<output>` sets the location either for the FIFO or the capture file,
depending on whether `-d` has been set or not.

You can change the default `<port>` using `-p` option. Default is set in private
port range (RFC 6335) to `57384` (EGCHD).

Options for `<resolution>` are `ntsc`, `pal`, `720` and `1080`. If unspecified
this defaults to using whatever resolution the input source is at. Note that
currently some users are experiencing video and audio artifacts if the
resolution doesn't match the input size.

`<pid-path>` is unused at the moment. Once implemented, it will provide a
single-instance mechanism, which will prevent this driver from opening multiple
times. You can use this option in the future, if you're running this driver as a
non-root user and don't have write access to the default `<pid-path>` location.


### General

This driver must be run as root, as it needs to access your Game Capture HD
device. Under Linux, you can alternatively configure udev to make your device
accessible by non-root users. See the file udev-rules/55-elgato-game-capture.rules

If no commandline options are set, the device will autodetect what source you
have, HDMI, Component, or Composite. It will also autodetect the streamed
resolution and colorspace. It will not however handle these changing on the fly.
The detection for whetjer you are using HDMI/Component/Composite may malfunction
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
resetting your device. If you interrupt this step, it will leave your device in
an undefined state and you will need to manually reset your device by
reconnecting it.

Currently supported input sources:

* HDMI: 480p60 (NTSC), 576p60 (PAL), 720p60, 1080i60, 1080p60
* Component: 480i60 (NTSC), 480p60 (NTSC), 576i50 (PAL), 576p50 (PAL), 720p60,
1080i60, 1080p60
* Composite: 480i60 (NTSC), 576i50 (PAL)


## Contribution

In order to contribute to this project, you need to read and agree the Developer
Certificate of Origin, which can be found in the CONTRIBUTING file. Therefore,
commits need to be signed-off. You can do this by adding the `-s` flag when
commiting: `git commit -s`. Pseudonyms and anonymous contributions will not be
accepted.


## License

This project is made available under the MIT License. For more information,
please refer to the LICENSE file.
