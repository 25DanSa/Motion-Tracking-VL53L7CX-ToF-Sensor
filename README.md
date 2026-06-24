# MSPM0L1117 + VL53L7CX Presence & Motion Tracker

Presence, motion tracking and session analytics using the Texas Instruments MSPM0L1117 MCU and the ST VL53L7CX Time-of-Flight sensor.

---

## Overview

This project implements presence detection and motion tracking using the VL53L7CX ToF sensor. The firmware performs foreground extraction, blob clustering, zone tracking and session generation directly on the MSPM0L1117 MCU.

Detected sessions are exported as JSON records over UART for further processing by an external host MCU or cloud gateway.

---

## Features

* VL53L7CX 4x4 ranging mode
* Motion-gated presence detection
* Blob clustering
* 3-zone tracking
* Dwell time measurement
* Session generation
* UART JSON output
* Ring buffer storage
* Time synchronization support
* Low-power MSPM0 implementation

---

## Hardware

### MCU

* Texas Instruments MSPM0L1117

### Sensor

* ST VL53L7CX Time-of-Flight Sensor

### Communication

* I2C: MSPM0L1117 ↔ VL53L7CX
* UART: JSON session output

---

## Zone Layout

| Zone | Distance        |
| ---- | --------------- |
| Z1   | 0.0 m – 0.75 m  |
| Z2   | 0.75 m – 1.50 m |
| Z3   | 1.50 m – 2.20 m |

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
```

---

## Known Limitations

* Detection performance depends on environment, target size and movement pattern.
* Range and reliability depend on integration time and ranging frequency.

---

## Applications

* Retail analytics
* Presence detection
* Dwell time measurement
* Occupancy monitoring
* Motion tracking
---

## License

Copyright © VusionGroup

All rights reserved.
