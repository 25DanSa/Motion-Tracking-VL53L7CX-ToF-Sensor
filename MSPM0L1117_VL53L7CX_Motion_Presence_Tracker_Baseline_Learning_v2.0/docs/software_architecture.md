# Software Architecture v2.0

## Overview

This document describes the internal software architecture of the MSPM0L1117 + VL53L7CX Baseline Learning firmware.

The firmware implements adaptive background learning, foreground extraction, connected-component blob detection, zone tracking, dwell time analytics, session generation and UART JSON reporting.

The implementation is optimized for low-power operation on the Texas Instruments MSPM0L1117 MCU.

---

# Source File Responsibilities

## main.c

Main application and processing pipeline.

Responsibilities:

* VL53L7CX initialization
* Background learning
* Foreground extraction
* Blob detection
* Zone classification
* Presence hold
* Session state machine
* Ring buffer management
* JSON generation
* UART time synchronization
* Auto reboot handling

---

## vl53l7cx_i2c_if.c / .h

VL53L7CX platform layer.

Responsibilities:

* I2C read/write transactions
* ULD interface support
* Sensor communication

---

## vl53l7cx_uart_if.c / .h

UART transport layer.

Responsibilities:

* UART transmit
* UART receive
* JSON output
* Time synchronization commands

---

## ti_msp_dl_config.c / .h

TI SysConfig generated hardware configuration.

Responsibilities:

* Clock setup
* UART setup
* GPIO configuration
* I2C configuration

---

# Processing Pipeline

The firmware executes the following processing sequence for every frame.

```text
VL53L7CX Frame
↓
Background Learning
↓
Foreground Extraction
↓
Blob Detection
↓
Zone Classification
↓
Presence Hold
↓
SPD Session FSM
↓
Session Statistics
↓
JSON Generation
↓
Ring Buffer
↓
UART Output
```

---

# Background Learning

## Purpose

Create a stable background model of the environment.

Examples:

* Wall
* Shelf
* Corridor
* Empty room

---

## Initial Learning Phase

Configuration:

```c
BG_INIT_FRAMES = 20
```

For the first 20 frames:

* Distance values are accumulated
* Per-pixel average is calculated
* Background model is created

State:

```text
BG_NOT_READY
```

After completion:

```text
BG_READY
```

---

## Adaptive Background Update

After initialization:

* Background continuously adapts
* Slow environmental changes are absorbed
* Temporary foreground objects are ignored

Purpose:

* Long-term stability
* Reduced false detections

---

# Foreground Extraction

## Purpose

Detect changes relative to background.

Calculation:

```text
Foreground = Current Distance - Background Distance
```

A pixel becomes foreground when:

```c
abs(current - background) > FG_DELTA_MIN_MM
```

Configuration:

```c
FG_DELTA_MIN_MM = 250
```

Output:

```text
Foreground Mask
```

---

# Blob Detection

## Purpose

Convert foreground pixels into candidate objects.

Method:

```text
Connected Component Analysis
```

Neighbour rule:

```text
8-connected neighbourhood
```

Processing:

1. Find foreground pixels
2. Group connected pixels
3. Calculate blob size
4. Select largest blob
5. Reject small blobs

---

# Zone Classification

The selected blob is assigned to one of three zones.

| Zone | Distance       |
| ---- | -------------- |
| Z1   | 0 – 750 mm     |
| Z2   | 750 – 1500 mm  |
| Z3   | 1500 – 3000 mm |

The blob centroid distance determines zone assignment.

---

# Presence Hold

Purpose:

Prevent detection flicker.

Configuration:

```c
PRESENT_HOLD_FRAMES = 2
```

Operation:

* Detection remains valid for 2 missing frames
* Short dropouts are ignored
* Session continuity improves

---

# SPD Session State Machine

The firmware uses a four-state session model.

## IDLE

No person detected.

---

## POSSIBLE_PERSON

Candidate detected.

Waiting for confirmation.

---

## PERSON_PRESENT

Confirmed presence.

Session active.

Zone statistics collected.

---

## LEAVING

Object disappeared.

Waiting for absence timeout.

---

## SESSION_END

Session finalized.

Statistics stored.

JSON generated.

---

# Session Statistics

Collected per session:

* Session start time
* Session end time
* Total duration
* Final zone
* Zone entry history
* Zone dwell times
* First entry time per zone

---

# Ring Buffer

Purpose:

Store completed sessions before transmission.

Benefits:

* Non-blocking operation
* Burst protection
* Reliable UART transfer

Stored records:

```text
Session JSON
Session JSON
Session JSON
...
```

---

# UART Time Synchronization

The ESP32 acts as time authority.

Supported command:

```text
T,<epoch_ms>
```

Example:

```text
T,1748450012345
```

The MSPM0 stores:

* epoch_at_sync_ms
* millis_at_sync

and calculates timestamps locally between synchronizations.

---

# JSON Output

When a session ends:

```text
session_end JSON
```

is generated and stored.

Typical fields:

* total_ms
* final_zone
* dwell_ms
* first_entry_ms
* conversionEvent

---

# Auto Reboot

Purpose:

Long-term stability.

Configuration:

```c
AUTO_REBOOT_MS
```

Operation:

* Runtime monitored continuously
* MCU automatically restarts
* Sensor reinitializes
* Background learning restarts

---

# Design Goals

* Stable stationary detection
* Low power operation
* Minimal RAM usage
* Robust empty-room behavior
* Reliable dwell time analytics
* Simple ESP32 integration
* Cloud-ready JSON output

