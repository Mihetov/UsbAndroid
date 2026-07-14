# UsbAndroid

Qt 6 / CMake Android application for scanning USB OTG devices and sending a test Modbus RTU packet to USB-serial adapters such as CH340.

## Features

- Scans USB Host devices connected to an Android phone over OTG on startup and by pressing **Сканировать USB**.
- Shows device path, device class, vendor ID, product ID, product name, interfaces, and endpoint data.
- Sends a Modbus RTU test frame through `usb-serial-for-android` instead of custom USB-serial driver code:
  - slave address: `1`
  - function: `0x06` (write single holding register)
  - register: `0xF000`
  - value: `26`
  - serial parameters: `9600 8N1`
  - CRC16 Modbus appended low byte first

## Build

Open this folder as a Qt 6 CMake project and use an Android kit. The Android package source is `android_backup/`; it adds the JitPack repository and the `com.github.mik3y:usb-serial-for-android:3.10.0` dependency.
