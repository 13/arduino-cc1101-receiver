# arduino-cc1101-receiver

An Arduino CC1101 receiver

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

The arduino-cc1101-receiver receives a 61 characters string and adds RSSI/LQI.

Moreover, it filters ERR_LENGTH_MISMATCH, ERR_CRC_MISMATCH AND ERR_RX_TIMEOUT.

### Built With

* [VSCode](https://github.com/microsoft/vscode)
* [PlatformIO](https://platformio.org/)
* [RadioLib](https://github.com/jgromes/RadioLib)

## Getting Started

### Prerequisites

* An Arduino with a CC1101 module as a receiver
* An Arduino with a CC1101 module as a transmitter
* VSCode
* PlatformIO

### Installation

```sh
git clone https://github.com/13/arduino-cc1101-receiver.git
```

## Usage

* Edit `main.cpp` to your needs
* Edit `platformio.ini` to your needs
* Build & upload to your Arduino

## Roadmap

- [ ] ...

## Release History

* 1.1.0
    * Initial release

## Contact

* **13** - *Initial work* - [13](https://github.com/13)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Thank you
