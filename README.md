# UsbAndroid

Qt 6 / CMake Android application for scanning USB OTG devices and sending a test Modbus RTU packet to USB-serial adapters such as CH340.

## Features

- Scans USB Host devices connected to an Android phone over OTG on startup and by pressing **Сканировать USB**.
- Shows device path, device class, vendor ID, product ID, product name, interfaces, and endpoint data.
- Sends a Modbus RTU test frame to the first Bulk OUT endpoint:
  - slave address: `1`
  - function: `0x06` (write single holding register)
  - register: `0xF000`
  - value: `26`
  - CRC16 Modbus appended low byte first

## Build

Open this folder as a Qt 6 CMake project and use an Android kit. The project requires Qt Quick and Android USB Host support on the target device.
