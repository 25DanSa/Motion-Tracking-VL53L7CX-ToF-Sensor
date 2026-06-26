# Motion-Tracking-Sensor

Embedded firmware collection for presence detection, motion tracking and session analytics using the **Texas Instruments MSPM0L1117** MCU and the **ST VL53L7CX Time-of-Flight** sensor.

The repository contains multiple firmware architectures that explore different detection algorithms, power profiles and application scenarios while sharing the same hardware platform.

---

# Repository Overview

The firmware performs onboard processing of VL53L7CX ranging data to detect human presence, classify objects into distance zones and generate session analytics. Completed sessions are exported as compact JSON records over UART for further processing by an ESP32 host MCU or cloud gateway.

Current firmware architectures:

| Firmware                                             | Description                                                                        | Primary Goal                  |
| ---------------------------------------------------- | ---------------------------------------------------------------------------------- | ----------------------------- |
| **Motion Presence Tracker v1.0**                     | Motion-gated detection optimized for low-power operation.                          | Battery-powered retail sensor |
| **Motion Presence Tracker – Baseline Learning v2.0** | Adaptive background-learning algorithm for improved filtering of phantom sessions. | Maximum detection stability   |

Additional firmware generations will be added as the project evolves.

---

# Common Features

All firmware versions provide:

* VL53L7CX ranging (4×4 or 8×8)
* Human presence detection
* Blob clustering
* SPD (Smart Presence Detection) Session State Machine
* Three-zone tracking
* Zone dwell time measurement
* Session generation
* UART JSON output
* Ring buffer session storage
* Time synchronization support
* MSPM0L1117 embedded implementation

---

# Firmware Comparison

| Feature               | Motion Tracker v1.0     | Baseline Learning v2.0       |
| --------------------- | ----------------------- | ---------------------------- |
| Detection Method      | Motion Indicator        | Adaptive Background Learning |
| Sensor Resolution     | 4×4                     | 8×8                          |
| Background Model      | No                      | Adaptive Per-Pixel           |
| Foreground Mask       | No                      | Yes                          |
| Stationary Detection  | Good                    | Excellent                    |
| Pass-by Detection     | Excellent               | Good                         |
| Processing Complexity | Low                     | Medium                       |
| Power Consumption     | Low                     | Higher                       |
| Integration Time      | Short                   | Increased                    |
| Ranging Frequency     | Reduced                 | Higher                   |
| Intended Application  | Battery-powered devices | Highest detection accuracy   |

---

# Hardware Platform

## MCU

* Texas Instruments MSPM0L1117

## Sensor

* ST VL53L7CX Time-of-Flight Sensor

## Communication

* I²C: MSPM0L1117 ↔ VL53L7CX
* UART: JSON session output

---

# Zone Layout

| Zone | Distance         |
| ---- | ---------------- |
| Z1   | 0.0 m – 0.75 m   |
| Z2   | 0.75 m – 1.50 m  |
| Z3   | 1.50 m – 2.20 m* |

*Maximum reliable detection distance depends on firmware configuration, integration time and ranging frequency.

---

# Session Output Example

```json
{
  "event":"session_end",
  "total_ms":2945,
  "final_zone":2,
  "zones":[
    {
      "idx":1,
      "entered":1,
      "first_entry_ms":0,
      "dwell_ms":1964
    },
    {
      "idx":2,
      "entered":1,
      "first_entry_ms":3927,
      "dwell_ms":981
    },
    {
      "idx":3,
      "entered":0,
      "first_entry_ms":0,
      "dwell_ms":0
    }
  ],
  "conversionEvent":0
}
```

---

# Repository Structure

```text
Motion-Tracking-Sensor
│
├── MSPM0L1117_VL53L7CX_Motion_Presence_Tracker_v1.0
│   ├── README.md
│   └── docs/
│
├── MSPM0L1117_VL53L7CX_Motion_Presence_Tracker_Baseline_Learning_v2.0
│   ├── README.md
│   └── docs/
│
└── Future firmware versions...
```

---

# Build Environment

* Code Composer Studio (CCS)
* Texas Instruments MSPM0 SDK
* ST VL53L7CX Ultra Lite Driver (ULD)

---

# Applications

* Retail analytics
* Smart shelves
* Occupancy monitoring
* Presence detection
* Dwell time measurement
* Motion tracking
* Smart buildings
* Edge AI sensing

---

# Documentation

Each firmware version contains its own documentation:

* README.md
* System Architecture
* Software Architecture
* CHANGELOG
* GitHub Release Notes

---

# Roadmap

Planned future firmware generations include:

* Improved pass-by detection
* Multi-person tracking
* Dynamic parameter configuration
* Enhanced analytics
* Additional JSON metrics
* Power optimization

---

# License

Copyright © VusionGroup

All rights reserved.
