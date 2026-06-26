# MSPM0L1117 + VL53L7CX Motion Presence Tracker — Baseline Learning v2.0

Low-power presence detection, stationary object tracking and session analytics using the Texas Instruments MSPM0L1117 MCU and the ST VL53L7CX Time-of-Flight sensor.

---

# Overview

This firmware implements an adaptive background-learning algorithm for reliable human presence detection using the VL53L7CX operating in **8×8 ranging mode**.

Unlike the previous Motion Indicator implementation, this version continuously learns the environment and detects people by comparing each new ranging frame against an adaptive per-pixel background model.

The firmware executes entirely on the MSPM0L1117 MCU and generates compact JSON session records for transmission to an ESP32 host or cloud gateway.

---

# Firmware Architecture

This release introduces the **Baseline Learning Architecture**.

Main processing stages:

* Initial background learning
* Adaptive background update
* Foreground extraction
* Connected-component blob detection
* Zone classification
* Presence hold
* SPD 4-state session state machine
* Session JSON generation
* Ring buffer storage
* UART communication
* Automatic periodic reboot

Detailed documentation:

* **System Architecture:** `docs/system_architecture.md`
* **Software Architecture:** `docs/software_architecture.md`

---

# Features

* 8×8 VL53L7CX ranging mode
* Adaptive per-pixel background learning
* Automatic background update
* Foreground extraction
* Connected-component blob detection
* Human candidate validation
* Three-zone tracking
* Stable stationary detection
* Presence hold filtering
* SPD 4-state session state machine
* Exclusive dwell time calculation
* First-entry timestamp per zone
* Ring buffer session storage
* UART JSON output
* UART time synchronization
* Automatic software reboot
* Low-power MSPM0 implementation

---

# Processing Pipeline

```text
VL53L7CX 8x8 Frame
        │
        ▼
Initial Background Learning
        │
        ▼
Adaptive Background Update
        │
        ▼
Foreground Extraction
(Current − Background)
        │
        ▼
Connected Blob Detection
        │
        ▼
Zone Classification
        │
        ▼
Presence Hold
        │
        ▼
SPD Session State Machine
        │
        ▼
JSON Session Generator
        │
        ▼
Ring Buffer
        │
        ▼
UART Output
```

---

# Hardware

## MCU

* Texas Instruments MSPM0L1117

## Sensor

* ST VL53L7CX Time-of-Flight Sensor

## Communication

* I²C: MSPM0L1117 ↔ VL53L7CX
* UART: JSON session output to ESP32

---

# Zone Layout

| Zone | Distance       |
| ---- | -------------- |
| Z1   | 0 – 750 mm     |
| Z2   | 750 – 1500 mm  |
| Z3   | 1500 – 3000 mm |

The blob centroid distance determines the active zone.

---

# Current Firmware Configuration

| Parameter                 | Value       |
| ------------------------- | ----------- |
| Resolution                | 8×8         |
| Frame Rate                | 3 Hz        |
| Integration Time          | 300 ms      |
| Initial Background Frames | 20          |
| Background Learning       | Adaptive    |
| Presence Hold             | 2 Frames    |
| Zone Count                | 3           |
| Session Storage           | Ring Buffer |
| Auto Reboot               | 30 Minutes  |

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

# Build Environment

* Code Composer Studio (CCS)
* Texas Instruments MSPM0 SDK
* ST VL53L7CX Ultra Lite Driver (ULD)

---

# Project Structure

```text
source/
include/
VL53L7CX_ULD/
targetConfigs/
docs/
```

---

# Validated Configuration

The following configuration has been validated during development:

* BG_INIT_FRAMES = 20
* HUMAN_PIX_MIN_Z3 = 2
* PRESENT_HOLD_FRAMES = 2

Validated scenarios:

* Stable stationary detection in Zone 1
* Stable stationary detection in Zone 2
* Stable stationary detection in Zone 3
* Reliable empty-room recovery after exit

---

# Applications

* Retail analytics
* Occupancy monitoring
* Presence detection
* Dwell time measurement
* Queue analytics
* Smart buildings
* Edge AI sensing
* Embedded people tracking

---

# Known Limitations

* Background learning requires an initially empty scene.
* Very rapid environmental changes may require several frames for adaptation.
* Extremely slow background changes are intentionally absorbed into the adaptive model.
* Detection performance depends on sensor placement, target size and environmental conditions.

---

# License

Copyright © VusionGroup

All rights reserved.

