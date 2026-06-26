# README.md

# ESP32 BLE WiFi Provisioning + Cloud Upload

## Overview

This project implements a prototype-oriented ESP32 firmware for ToF sensor devices.

The firmware combines:

* BLE WiFi provisioning
* Secure WiFi credential storage
* Automatic WiFi connection management
* NTP time synchronization
* UART communication with an STM32/MSPM0 sensor node
* Session JSON generation
* HTTPS cloud upload

The firmware is designed so that WiFi credentials can be updated at runtime without rebooting the device.

---

# Main Features

* BLE GATT WiFi provisioning
* CBOR encoded WiFi credentials
* Runtime WiFi reprovisioning
* Automatic reconnect
* Retry with backoff
* Preferences (NVS) credential storage
* NTP synchronization
* UART JSON parser
* Session JSON builder
* HTTPS three-phase chunk upload
* SHA1 integrity verification
* MD5 file ID generation

---

# Hardware

Host MCU

* ESP32-C3 Super Mini

Sensor MCU

* STM32 / TI MSPM0

Communication

* UART

BLE

* NimBLE

Cloud

* HTTPS REST API

---

# Software

Arduino IDE

Main libraries

* NimBLE-Arduino
* WiFi
* HTTPClient
* WiFiClientSecure
* Preferences
* mbedTLS

---

# Repository Structure

```
src/
docs/
images/
```

---

# Firmware Architecture

Detailed documentation is available in

```
docs/software_architecture.md
```

---

# Build

1. Install ESP32 Arduino Core.
2. Configure cloud parameters.
3. Compile.
4. Flash.
5. Provision WiFi through BLE.

---

# WiFi Provisioning

The Android application (Connect App) connects over BLE and sends CBOR encoded WiFi credentials.

The firmware

* validates the payload
* stores credentials in NVS
* replies using BLE indications
* connects to WiFi
* synchronizes time
* starts cloud communication

---

# Cloud Upload

After receiving a session summary from the sensor MCU, the firmware

* builds the final JSON
* computes MD5 and SHA1
* uploads using

1. Init
2. Append
3. Complete

---

# License

Add your preferred license here.

