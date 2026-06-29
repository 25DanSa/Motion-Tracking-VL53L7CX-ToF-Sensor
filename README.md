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

## Firmware Comparison

The repository currently contains two firmware architectures optimized for different use cases.

| Feature                     | Motion Presence Tracker v1.0             | Baseline Learning v2.0                        |
| --------------------------- | ---------------------------------------- | --------------------------------------------- |
| Primary Goal                | Low-power presence and pass-by detection | High-stability presence tracking              |
| Detection Method            | Motion Indicator                         | Adaptive Background Learning                  |
| Sensor Resolution           | 4×4                                      | 8×8                                           |
| Background Model            | No                                       | Adaptive Per-Pixel                            |
| Foreground Mask             | No                                       | Yes                                           |
| Integration Time            | Default (~20 ms)                         | Increased (300 ms)                            |
| Frame Rate                  | 2 Hz                                     | 3 Hz                                          |
| Reliable Detection Range    | Up to ~2 m                               | Up to ~3 m                                    |
| Lateral Pass-by Detection   | Excellent                                | Moderate                                      |
| Stationary Person Detection | Good                                     | Excellent                                     |
| Long-Dwell Tracking         | Good                                     | Excellent                                     |
| Processing Complexity       | Low                                      | Higher                                        |
| Power Consumption           | Low                                      | Higher                                        |
| Intended Application        | Battery-powered sensors                  | Mains-powered or performance-oriented systems |

### Motion Presence Tracker v1.0

This firmware is optimized for **low-power operation** and is intended for battery-powered sensors.

Characteristics:

* Motion Indicator-based detection
* 4×4 ranging mode
* Default integration time (~20 ms)
* 2 Hz frame rate
* Reliable detection up to approximately 2 m
* Excellent lateral pass-by detection
* Low MCU workload
* Optimized power consumption

Typical applications:

* Retail entrance monitoring
* Pass-by counting
* Battery-powered occupancy sensors
* Fast-moving targets

---

### Baseline Learning v2.0

This firmware is optimized for **maximum detection stability** and **longer detection range**.

Instead of relying on the VL53L7CX Motion Indicator, it continuously learns the environment and detects foreground objects using an adaptive per-pixel background model.

Characteristics:

* Adaptive background learning
* 8×8 ranging mode
* Increased integration time (300 ms)
* 3 Hz frame rate
* Reliable detection up to approximately 3 m
* Excellent stationary presence detection
* Stable tracking of people remaining within the field of view
* Higher computational load
* Increased power consumption

Typical applications:

* Smart shelves
* Customer dwell-time analytics
* Queue monitoring
* People approaching products
* Long-duration occupancy monitoring


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
| Z3   | 1.50 m – 3.00 m* |

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
* Multi-person tracking
* Dynamic parameter configuration
* Enhanced analytics
* Additional JSON metrics
* Power optimization

---

# License

Copyright © VusionGroup

All rights reserved.
