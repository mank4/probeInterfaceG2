# probeInterfaceG2

probeInterfaceG2 is under development

## (planned) Hardware

- 12 channel interface with identical multi-purpose channels, each with
  - discrete transistor/op-amp power supply from 0-4.x V with ~500 mA max current
  - precision voltage and current amplifiers with current self calibration
  - level shifter with switchable directions capable to work down to 0.65 V
  - mosfet for providing short to ground
- 24-bit common ADC with precision reference and 16-bit common DAC
- Raspberry RP2040 microcontroller with versatile IO architecture for full software control of digital output capabilities like SPI
- Ethernet interface for SCPI control

## RP2040 toolchain

Clone probeInterfaceG2 next to pico folder containing pico-examples, pico-sdk and picotool.

- Create build folder in firmware and cd into.
- Then run `export PICO_SDK_PATH=../../../pico/pico-sdk` and `cmake ..`
- Then `make -j4`
- Then `../load.sh` with the RP2040 connected via USB. Make sure to have set up the [udev rules](https://gist.github.com/alejoseb/c7a7b4c67f0cf665dadabb26a5a87597) and that picotool is build. (With cutecom it is necessary to set DTR checkbox to communicate with RP2040)

For details see the [Getting started](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) guide.

## Resources

- Frontend
  - Agilent E3631 Service Guide
  - HP 6114A Service Guide
  - Power Designs 5020 Service Guide
  - The Art of Electronics by Horowitz
- Microcontroller
  - [RP2040](https://www.raspberrypi.com/documentation/microcontrollers/rp2040.html)
  - [Pi pico](https://www.raspberrypi.com/products/raspberry-pi-pico/)
  - RP2040 Minimal Kicad reference design
- Ethernet
  - Wiznet w5500 [eval](https://www.wiznet.io/product-item/w5500-evb-pico/) [doc](https://docs.wiznet.io/Product/iEthernet/W5500/w5500-evb-pico)
