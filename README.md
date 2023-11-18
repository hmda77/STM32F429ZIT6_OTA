# STM32F429ZIT6 OTA Project
![total downloads](https://img.shields.io/github/downloads/hmda77/STM32F429ZIT6_OTA/total.svg)
![stars](https://img.shields.io/github/stars/hmda77/STM32F429ZIT6_OTA.svg)
![forks](https://img.shields.io/github/forks/hmda77/STM32F429ZIT6_OTA.svg)
![watchers](https://img.shields.io/github/watchers/hmda77/STM32F429ZIT6_OTA.svg)
## Overview
Welcome to the Over-the-Air (OTA) Update Project with STM32! This project is designed to facilitate firmware updates for STM32 microcontrollers using a wireless communication protocol. With OTA updates, you can remotely deploy firmware updates to STM32 devices without the need for physical access.

## To-Do:

- [x] Writing Bootloader with OTA supported
- [x] Writing A PC Tool for download Application Binary File
- [x] Writing A Protocol between host and client for requesting and handling OTA state
- [x] Writing An Application For NodeMCU to Get and Store the Binary File From Internet
- [x] Writing An Application For NodeMCU to Upgrade STM32 IC
- [ ] New Tasks will be updated ASAP...

### OTA Flowchart
![OTA Flowchart](./images/OTA_Flowchart.svg)

## Getting Started

## Prerequisites:
### Hardware:
- STM32F429ZIT6 Discovery Board
- NodeMCU
- CH340G (or Any USB to TTL Module)

### Software:
![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![php](https://img.shields.io/badge/PHP-777BB4?style=for-the-badge&logo=php&logophpor=white)
- STM32CubeIDE V1.11.2
- Any Serial Communication Software Like PuTTY, Docklight, ...
- STM32CubeProgrammer (Optional)

- Clone this Repositpry.
- Set STM32CubeIDE Workspace to the repository directory.
- Full Erase Flash.
- Download Bootloader via Programmer.

## Installation:
- Clone this Repositpry.
  ```
  git clone https://github.com/hmda77/STM32F429ZIT6_OTA.git
  ```
- Set STM32CubeIDE Workspace to the repository directory.

## Usage:
- Full Erase Flash.
- Download Bootloader via Programmer.

### First Time Boot:
- Make and build Application. copy `Application.bin` file to PC Tool directory.
- Make and build PC Tool with this command (or download released tool):
  ```
  gcc main.c RS232\rs232.c -IRS232 -Wall -Wextra -o2 -o <app_name>
  ```
- in the first boot, the MCU is on the DFU mode. Run this command for download the file to the MCU Flash:
  ```
  ./<app_name>.exe -c <COM_port> -f <Application_bin_file> -v <ver_major>.<ver_minor> -b
  ```
### Firmware Update:

you can update firmware in two way: offline or Online programming. 

#### Ofline Updating:
- You can make another bin file from Application project and download it whit this command:
  ```
  ./<app_name>.exe -c <COM_port> -f <Application_bin_file> -v <ver_major>.<ver_minor>
  ```
  (Use the `-b` argument when the device is in DFU mode.)

- Note that you should insert a higher version number in arguments.

#### Online Updating:
- Upload WebTool in your Server/Host.
- Set your password in `index.php` file.
- Go to `https://<your_website>/index.php` and enter new firmware information and your password. then Submit the form.
- Note that you should enter a higher version number.
- you can check the last firmware information in `https://<your_website>/index.php?getinfo`.
- before programming NodeMCU, insert your WiFi SSID and PASSWORD and the address of Server/Host for getting information.
- Program NodeMCU (ESP-12E/F).
- Connect the Main Serial Port of NodeMCU to the STM32 UART with baudrate equals to 9600.
- STM32 Will Check the new firmware and download it automatically.

## License
[![MIT LICENSE](https://img.shields.io/github/license/hmda77/STM32F429ZIT6_OTA.svg)](LICENSE)

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments
- [STM32 Refference Manual RM0090](https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-and-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)
- [Embetronicx Bootloader](https://github.com/Embetronicx/STM32-Bootloader/blob/main/)
- [ChatGPT3.5](https://chat.openai.com)

[![Website](https://img.shields.io/badge/website-000000?style=for-the-badge&logo=About.me&logoColor=white)](https://hamidarabsorkhi.ir/eng)
[![follow](https://img.shields.io/github/followers/hmda77.svg?style=social&label=Follow&maxAge=2592000)](https://github.com/hmda77)
