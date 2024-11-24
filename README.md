# arduino-lora-receiver

An Arduino LoRa receiver for ESP32

<p align="center">
<img src="assets/screenshot1.png" width="300">
<img src="assets/screenshot2.png" width="300">
</p>

## Contents

 * [About](#about)
   * [Built With](#built-with)
 * [Getting Started](#getting-started)
   * [Prerequisites](#prerequisites)
   * [Installation](#installation)
 * [Usage](#usage)
 * [Roadmap](#roadmap)
 * [Release History](#release-history)
 * [License](#license)
 * [Contact](#contact)
 * [Acknowledgements](#acknowledgements)

## About

The arduino-lora-receiver receives a 61 characters string and adds RSSI/LQI.

```
Z:60,N:87,T1:29,H1:817,T2:25,T3:42,P1:9260,A1:753,V1:38

Z = package length
N = node id
I = package counter
T = temperature
H = humidity
P = pressure
A = altitude
Q = air quality
V = voltage
M = motion
S = switch

, = delimiter

T1 = si7021
T2 = ds18b20
T3 = bmp280
T4 = bme680

RSSI = Signal strength
LQI = Signal quality
RN = receiver node id
```

#### Features
- Websocket
- mDNS
- NTP
- OTA Update

### Built With

* [VSCode](https://github.com/microsoft/vscode)
* [PlatformIO](https://platformio.org/)
* [arduino-LoRa](https://github.com/sandeepmistry/arduino-LoRa)

## Getting Started

### Prerequisites

* An Arduino/ESP32 with a LoRa module as a receiver
   * Wiring diagram: []()
* An Arduino with a LoRa module as a transmitter [13/arduino-lora-transmitter](https://github.com/13/arduino-lora-transmitter)
   * Wiring diagram: [AskSin++](https://asksinpp.de/Grundlagen/01_hardware.html#stromversorgung)
* VSCode
* PlatformIO

### Installation

```sh
git clone https://github.com/13/arduino-lora-receiver.git
```

## Usage

* Edit `main.cpp` to your needs
* Edit `platformio.ini` to your needs
* Build & upload to your Arduino

## Roadmap

- [ ] ...

## Release History
* v13
    * Stable release
* v10
    * Initial release

## Contact

* **13** - *Initial work* - [13](https://github.com/13)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Thank you
