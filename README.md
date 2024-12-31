# UClock

A simple ESP32-based clock project using the DS3231 RTC module and Bluetooth Low Energy (BLE) connectivity.

## Features

* Displays current time using the DS3231 RTC module
* Sends temperature notifications via BLE
* Uses the NimBLE stack for BLE connectivity

## Dependencies

* ESP32 board
* DS3231 RTC module
* BLE-enabled device for testing

## Build and Flash

1. Clone this repository
2. Install the ESP-IDF framework and dependencies
3. Build and flash the project using `idf.py build` and `idf.py flash`