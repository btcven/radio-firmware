<p align="center">
  <a href="https://locha.io/">
  <img height="200px" src="doc/LogotipoTurpial-Color.20-09-19.svg">
  </a>
  <br/>
  <a href="https://travis-ci.com/btcven/radio-firmware">
    <img src="https://travis-ci.com/btcven/radio-firmware.svg?branch=master" title="Build Status">
  </a>
</p>

<p align="center">
  <a href="https://locha.io/">Project Website</a> |
  <a href="https://locha.io/donate">Donate</a> |
  <a href="https://github.com/sponsors/rdymac">Sponsor</a> |
  <a href="https://locha.io/buy">Buy</a>
</p>

<h1 align="center">Radio Firmware</h1>

We are happy of your visit and that you can read more about us. Here you can
find the main firmware for your device compatible with Locha Mesh (the radio)
and be aware of our development process.

## What's Locha Mesh?

The Locha Mesh network is a secure radio network for text messaging and bitcoin
transactions. The main objetive is a long range network for everyone and
everywhere, for this reason, we are working not only in a protocol, also the
firmware for affordable devices like our "Turpial".

If you want to learn more about Locha Mesh feel free to read
[the Locha Mesh main repository](https://github.com/btcven/locha) or take a
look at our website [locha.io](https://www.locha.io).

## Sponsor.

If you want to support this project you can make a donation to the Locha Mesh
effort to build a private censorship-resistant mesh network devices for Bitcoin and Lightning Network payments without Internet.

Here are some places if you want to support us:

- Donate: https://locha.io/donate
- Buy Turpial devices: https://locha.io/buy

## Supported boards.

This firmware should run on all the platform that Contiki-NG supports, however
our main focus is to support the SimpleLink MCUs, these are the ones used in
the Turpial board, mainly the CC1312R.

## Table of Contents.

[Radio Firmware](#radio-firmware)

[What's Locha Mesh?](#whats-locha-mesh)

[Sponsor.](#sponsor)

[Supported boards.](#supported-boards)

* [Getting started.](#getting-started)
  - [Install SRecord.](#install-srecord)
  - [Install build essentials (make, etc).](#install-build-essentials-make-etc)
  - [Install ARM compiler.](#install-arm-compiler)
  - [Install Uniflash.](#install-uniflash)
  - [Compiling the project.](#compiling-the-project)
  - [Testing with Renode.](#testing-with-renode)

[License.](#license)

## Getting started.

First of all you need to clone the repository:

```bash
git clone https://github.com/btcven/radio-firmware.git

git submodule update --init --recursive
```

Also you need to install the necessary tools to build and flash correct
Turpial's `radio-firmware` :

### Install SRecord.

*Note: (make sure it's on `PATH`)*:

- Windows: Check the SRecord downloads.
- Ubuntu: `sudo apt-get install srecord`

### Install build essentials (make, etc).

- Windows: you may need to install make manually or install MSYS2.
- Ubuntu `sudo apt-get install build-essentials`

### Install ARM compiler.

You should download it from [GNU-RM Downloads][arm-gcc].

#### On Ubuntu/Any Linux Distribution.

```bash
wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2019q4/RC2.1/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2

tar xjf gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2
```

This will create a directory named
`gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux` in your current working dir.
Add `<working-directory>/gcc-arm-none-eabi-5_2-2015q4/bin` to your `$PATH`.

#### Windows.

[arm-gcc](GNU-RM Downloads) has a downloadable installer for the GCC ARM
compiler, make sure after you install it is on your `PATH`, if not add it.

### Install Uniflash.

Insall TI's [Uniflash][uniflash] tool for your operating system. This tool
allows you to upload the built firmware to the CC1312R.

### Compiling the project.

The build process is very simple, just call `make` and it will build the
firmware. The built firmware is located under the `build/simplelink/`
directory.

```bash
make
```

[arm-gcc]: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
[uniflash]: https://www.ti.com/tool/UNIFLASH

### Testing with Renode.

Currently we can test the code without using the actual SimpleLink radio MCU,
for this case we use Renode as our emulation environment. This allows us to
develop and test with more flexibility.

 - **Installing Renode**: You can follow the Renode installation guide in their
[homepage][renode], it can be installed on Linux, Windows and Mac OS.

[renode]: https://renode.io/#downloads

Now you need to build the firmware enabling Renode support:

```bash
make RENODE=1
```

To run Renode, from the command line execute this:

```
renode radio-nodes.resc
```

You can modify the radio-nodes.resc file to add your own nodes, modify their
position in relation to each other. Learn more about Renode scripts
[here][scripts].

[scripts]: https://renode.readthedocs.io/en/latest/introduction/using.html#basic-interactive-workflow


## License.

Copyright © 2019 Bitcoin Venezuela and Locha Mesh developers.

Licensed under the **Apache License, Version 2.0**

---
**A text quote is shown below**

>Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
___
Read the full text:
[Locha Mesh Apache License 2.0](LICENSE)
