# ReGrip Device Firmware

## Purpose
ReGrip is a **Grip Strength Tracker App**. This folder contains the microcontroller firmware that reads grip force from a load cell and streams the measured value to the ReGrip app over Bluetooth Low Energy (BLE).

## Technologies
- **ESP32-WROOM** (Arduino-compatible C/C++)
- **Bluetooth Low Energy (BLE)** using the ESP32 Arduino BLE libraries (`BLEDevice`, `BLEServer`, `BLECharacteristic`)
- **HX711** load cell amplifier + **load cell** for force measurement (`HX711` library)
- **Serial** debugging at `115200` baud

## Hardware / Wiring (as used in `firmware.ino`)
- **HX711 DOUT** -> GPIO `23`
- **HX711 SCK**  -> GPIO `22`

## BLE GATT Overview
The firmware advertises a BLE service named `ReGrip` and exposes:
- **Service UUID**: `d4d61900-3739-4da1-8f5c-d973f6805c36`
- **Sensor characteristic (READ + NOTIFY)**: `d4d61901-3739-4da1-8f5c-d973f6805c36`
  - Sends the current force/weight reading as a string.
- **Tare characteristic (WRITE)**: `d4d61902-3739-4da1-8f5c-d973f6805c36`
  - Write a single byte `0x01` to request a tare.
- **Calibration characteristic (WRITE)**: `d4d61903-3739-4da1-8f5c-d973f6805c36`
  - Write an ASCII float (e.g. `-10160.0`) to update the calibration factor.

## What the Firmware Does
- Initializes the HX711 scale, applies a calibration factor, and tares on startup.
- Starts BLE advertising and, when a client is connected:
  - Sends readings (averaged) via notifications about every `100ms`.
  - Applies **tare** and **calibration updates** when written via BLE.

## File
- `firmware.ino` - main ESP32 firmware source.
