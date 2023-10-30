# STM32F429ZIT6 OTA Project

## To-Do:

- [x] Write Bootloader with OTA supported
- [x] Write A PC Tool for download Application Binary File
- [ ] Write A Protocol between host and client for requesting and handling OTA state
- [ ] Write An Application For NodeMCU to Get and Store the Binary File From Internet
- [ ] Write An Application For NodeMCU to Upgrade STM32 IC

### Bootloader Flowchart
![Bootloader Flowchart](./images/Bootloader.svg)

## Requirements:
### Hardware:
- STM32F429ZIT6 Discovery Board
- NodeMCU
- CH340G USB to TTL Module

### Software:
- STM32CubeIDE V1.11.2
- Any Serial Communication Like PuTTY, Docklight, ...
- STM32CubeProgrammer (Optional)

## Usage:
- Clone this Repositpry.
- set STM32CubeIDE Workspace to this Project.
