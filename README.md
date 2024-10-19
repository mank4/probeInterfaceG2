# probeInterfaceG2

probeInterfaceG2 is an USB controlled interface box for transmission and reception of digital signals on 12-pins of a 14-pin flat band cable. The pin number layout is common with [GGB Picoprobe multi-contact wedge probes](https://ggb.com/home/multi-contact-wedges/). It features galvanically isolated USB for control, adjustable logic levels from 0.65 to 3.3 V and relays for disabling unused pins. The device is controlled via IEEE 488.2 compatible SCPI commands.

Currently the following interfaces are supported:
- GPIO
- SPI

probeInterfaceG2 is licensed under GPL v3, except for the case's 3D model file which is (c) Hammond Manufacturing Ltd., originally obtained from https://www.hammfg.com/electronics/small-case/extruded/1455#product-tables.

probeInterfaceG2 uses
- [tinyusb](https://github.com/hathach/tinyusb)
- [scpi-parser](https://github.com/j123b567/scpi-parser)

## Hardware Features

- Raspberry RP2040 microcontroller
  - pio cells for high-speed, unconstraint muxable IO functions
  - integrated USB for programming and IEEE 488.2 SCPI control
- 0.65V - 3.3V level shifters
  - Controllable common logic level by discrete 12-bit R-2R DAC or external input
- Relays for enabling/disabling IOs

## SCPI commands

Pin numbering is based on [GGB Picoprobe multi-contact wedge probes](https://ggb.com/home/multi-contact-wedges/) numbering scheme.

__*IDN?__

Returns device type.

__*RST__

Resets settings made and clears error.

__*CLS__

Clears error. Device stores only a single error.

__:SYSTem:ERRor[:NEXT]?__

Returns <errorNumber>,<errorDescription> as described by [Keysight](https://na.support.keysight.com/pna/help/latest/Support/SCPI_Errors.htm) and clears error.

__:SYSTem:ERRor:COUNt?__

Returns number of errors.

__:SYSTem:VERSion?__

Returns system version number.

__ECHO `<text>`__

Echos argument.

__BOOTSEL__

Reboots the pin in bootloader mode.

__IO:LEVel `<V>`__

Sets the IO level voltage.

__IO<1:12>:FUnc `<IN|OUT|CS|SCK|MISO|MOSI>`__

Sets the pins function to one of the following. IN and OUT can be used multiple times while the SPI related functions can only be used once. Pins are left in last configuration when changing SPI related functions from one pin to another. This function does not enable the according output relay.

__IO<1:12>:ENable <1|0>__

Enables or disables the relay of the corresponding IO pin.

__IO<1:12>:WRite <1|0>__

Sets an OUT configured pin high or low.

__IO<1:12>:REad__

Reads an IN configured pin and returns 0 or 1.

__SPI:TRANSfer? <binary values>__

Writes the given bytes to the SPI bus using the configured settings and returns the response of MISO as binary values.

__SPI:CPHA <1|0>__

Sets the Clock phase (CPHA) to the corresponding value.

__SPI:CPOL <1|0>__

Sets the Clock polarity (CPOL) to the corresponding value.

__SPI:BAUDrate <MHz>__

Speed of the probes SPI interface is set to defined float value in MHz.

__SPI:BAUDrate?__

Reads the speed of the currently configured baudrate as float in MHz.

## RP2040 toolchain

Assuming Windows or Ubuntu/Debian the following setup is required for compiling the firmware. For normal use this is not required, VISA drivers (on Windows) are sufficient for common use.

- Only on windows: Install WSL / Ubuntu on Windows, run `sudo apt update`
- Make sure the following packages exist: `sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib git build-essential`
- Clone dependencies: `git submodule init` and `git submodule update`
- Clone the [pico-sdk](https://github.com/raspberrypi/pico-sdk) into the same folder as probeInterfaceG2 and update submodules if requested by cmake in the next steps
- Create build folder in firmware and cd into. `cd fw && mkdir build && cd build`
- Then run `export PICO_SDK_PATH=../../../pico-sdk` and `cmake ..`
- Then `make -j4` (4 is the number of cores used for compilation)
- If compilation fails with USB descriptor length errors, do the following: cd into pico-sdk folder and `cd lib/tinyusb/` and then `git pull origin master`
- On Windows and Linux: Copy the uf2 onto the mass storage of the RP2040. Bootloader mode is entered by pressing the button while plugging in.
- With correctly set up udev rules: Run `../load.sh` with the RP2040 connected via USB. When not running the probeInterfaceG2 firmware ress the button while pluggin in. [udev rules](https://gist.github.com/alejoseb/c7a7b4c67f0cf665dadabb26a5a87597) for the probeInterfaceG2:
```
/etc/udev/rules.d/99-probeInterfaceG2.rules
SUBSYSTEMS=="usb", ACTION=="add", ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="03e8",  MODE="0666", GROUP="usbtmc"
```

For programming hints see the [Getting started](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) guide.

## Possible improvements

- Better 3.3 V overvoltage protection circuit currently realized using X0202NN
- Ethernet interface VXI11
- Integrated power supply for DUT, with high precision measurement functions and grounding feature

## Resources

- Frontend
  - Agilent E3631 Service Guide
  - HP 6114A Service Guide
  - Power Designs 5020 Service Guide
  - The Art of Electronics by Horowitz
- Microcontroller
  - [RP2040](https://www.raspberrypi.com/documentation/microcontrollers/rp2040.html), [Pi pico](https://www.raspberrypi.com/products/raspberry-pi-pico/), RP2040 Minimal Kicad reference design
  - [SCPI specifications](https://www.ivifoundation.org/specifications/default.aspx), [SCPI-99 standard](https://www.ivifoundation.org/docs/scpi-99.pdf), [SCPI library](https://github.com/j123b567/scpi-parser)
  - [tinyusb for usbtmc](https://github.com/hathach/tinyusb), [example](https://github.com/markb139/pico_logic)
- Ethernet
  - Wiznet w5500 [eval](https://www.wiznet.io/product-item/w5500-evb-pico/) [doc](https://docs.wiznet.io/Product/iEthernet/W5500/w5500-evb-pico)
