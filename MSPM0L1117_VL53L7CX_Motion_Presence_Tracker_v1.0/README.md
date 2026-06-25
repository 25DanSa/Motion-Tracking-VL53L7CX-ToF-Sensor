# MSPM0L1117 + VL53L7CX Motion Presence Tracker

Presence, motion tracking and session analytics using the Texas Instruments MSPM0L1117 MCU and the ST VL53L7CX Time-of-Flight sensor.

---

## Overview

This project implements low-power human presence detection and motion tracking using the VL53L7CX Time-of-Flight sensor operating in 4x4 ranging mode.

The firmware runs entirely on the MSPM0L1117 MCU and performs:

* Distance filtering
* Blob extraction
* Motion-indicator gating
* Zone classification
* Session tracking
* Dwell time measurement
* JSON session generation

Completed sessions are stored in a ring buffer and transmitted over UART for processing by an external host MCU or cloud gateway.

---

## Firmware Architecture

This release implements the **Motion Indicator Architecture**.

Characteristics:

* No baseline learning
* No background model
* VL53L7CX motion-indicator gating
* Blob-based object detection
* 3-zone tracking
* Session state machine
* UART time synchronization
* Ring-buffered session storage

Detailed documentation:

* Architecture Overview: `docs/architecture.md`
* Code Walkthrough: `docs/code_walkthrough.md`

---

## Features

* VL53L7CX 4x4 ranging mode
* Motion-indicator gated detection
* Distance-based blob extraction
* Human candidate validation
* 3-zone tracking (Z1 / Z2 / Z3)
* Session state machine
* Dwell time analytics
* Ring buffer storage
* UART JSON output
* UART time synchronization
* Low-power MSPM0 implementation

---

## Processing Pipeline

```text
VL53L7CX Frame
↓
Distance Filtering
↓
Blob Extraction
↓
Motion Indicator Gate
↓
Zone Classification
↓
Session State Machine
↓
JSON Generator
↓
UART Output
```

---

## Hardware

### MCU

* Texas Instruments MSPM0L1117

### Sensor

* ST VL53L7CX Time-of-Flight Sensor

### Communication

* I2C: MSPM0L1117 ↔ VL53L7CX
* UART: Session JSON output

---

## Zone Layout

| Zone | Distance        |
| ---- | --------------- |
| Z1   | 0.0 m – 0.75 m  |
| Z2   | 0.75 m – 1.50 m |
| Z3   | 1.50 m – 2.20 m |

---

## Current Configuration

| Parameter         | Value                  |
| ----------------- | ---------------------- |
| Resolution        | 4x4                    |
| Frequency         | 2 Hz                   |
| Integration Time  | Default Sensor Setting |
| System Clock      | 4 MHz                  |
| Tracking Range    | 200–3000 mm            |
| Motion Indicator  | Enabled                |
| Baseline Learning | Disabled               |
| Session Storage   | Ring Buffer            |

---

## Session Output Example

```json
{
  "event": "session_end",
  "total_ms": 2945,
  "final_zone": 2,
  "zones": [
    {
      "idx": 1,
      "entered": 1,
      "first_entry_ms": 0,
      "dwell_ms": 1964
    },
    {
      "idx": 2,
      "entered": 1,
      "first_entry_ms": 3927,
      "dwell_ms": 981
    },
    {
      "idx": 3,
      "entered": 0,
      "first_entry_ms": 0,
      "dwell_ms": 0
    }
  ],
  "conversionEvent": 0
}
```

---

## Build Environment

* Code Composer Studio (CCS)
* MSPM0 SDK
* VL53L7CX ULD

---

## Project Structure

```text
source/
include/
VL53L7CX_ULD/
targetConfigs/
docs/
```

---

## Applications

* Retail analytics
* Occupancy monitoring
* Presence detection
* Dwell time measurement
* Motion tracking
* Smart buildings
* Edge sensor analytics

---

## Known Limitations

* Detection performance depends on environment, target size and movement pattern.
* Long-range lateral pass-by detection and stationary detection improved in next firmware version.
* Detection range and reliability depend on ranging frequency and sensor configuration.
---

## License

Copyright © VusionGroup

All rights reserved.
