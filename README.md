<p align="center">
  <a href="https://locha.io/">
  <img height="200px" src="doc/LogotipoTurpial-Color.20-09-19.svg">
  </a>
</p>

<h1 align="center">Radio Firmware</h1>

Open-source firmware for the radio module used in Locha Mesh networking devices.

# Getting started

First start by cloning the repository:

```bash
git clone https://github.com/btcven/radio-firmware.git

git submodule update --init --recursive
```

## Install tools

Next you need to install the necessary tools to build and correctly flash
Turpial's `radio-firmware` :

### SRecord

*Note: (make sure it's on `PATH`)*:

- Windows: Check the SRecord downloads.
- Ubuntu: `sudo apt-get install srecord`

### Build essentials (make)

- Windows: you may need to install make manually or install MSYS2.
- Ubuntu `sudo apt-get install build-essentials`

### ARM compiler

You should download it from [GNU-RM Downloads][arm-gcc].

#### On Ubuntu/Any Linux Distribution

```bash
wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2019q4/RC2.1/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2

tar xjf gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2
```

This will create a directory named
`gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux` in your current working dir.
Add `<working-directory>/gcc-arm-none-eabi-5_2-2015q4/bin` to your `$PATH`.

#### Windows

[arm-gcc](GNU-RM Downloads) has a downloadable installer for the GCC ARM
compiler. Make sure after you install it is on your `PATH`, if not add it.

### Uniflash

Install TI's [Uniflash][uniflash] tool for your operating system. This tool
allows you to upload the built firmware to the CC1312R radio module.

### Building

The build process is very simple, just call make and it will build the
firmware and then put it under the `build/simplelink/`.

```bash
make TARGET=simplelink BOARD=launchpad/cc1312r
```

[arm-gcc]: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads 
[uniflash]: https://www.ti.com/tool/UNIFLASH
