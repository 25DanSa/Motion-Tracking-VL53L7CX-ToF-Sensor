# Changelog

All notable changes to this project will be documented in this file.

The format follows a chronological release history.

---

# v2.0.0 — Baseline Learning Architecture

Initial public release of the Baseline Learning firmware generation.

Release Date: *YYYY-MM-DD*

---

## Overview

This release replaces the previous Motion Indicator detection pipeline with an adaptive background-learning architecture for improved stationary presence detection.

The firmware performs all processing locally on the MSPM0L1117 MCU using the VL53L7CX Time-of-Flight sensor operating in 8×8 resolution.

---

## Added

### Background Learning

* Initial 20-frame background learning
* Adaptive per-pixel background model
* Automatic background update
* Support for wall, shelf and open-space installations

### Foreground Detection

* Background subtraction
* Foreground mask generation
* Configurable foreground threshold
* Noise suppression

### Blob Detection

* Connected-component blob detection
* Largest blob selection
* Human candidate validation
* Small object rejection

### Zone Tracking

* Three distance zones
* Blob centroid mapping
* Exclusive zone occupancy
* Zone transition detection

### Presence Detection

* Presence hold filtering
* Stable stationary detection
* Reduced detection flicker
* Empty-room recovery

### Session Analytics

* SPD 4-state session state machine
* Session start/end detection
* Exclusive dwell time calculation
* First-entry timestamp per zone
* Final zone reporting

### Communication

* JSON session generation
* UART session transmission
* UART time synchronization
* Ring buffer session storage

### Reliability

* Automatic software reboot
* Long-term stability improvements
* Sensor reinitialization after reboot

---

## Validated Configuration

* Resolution: 8×8
* Frame Rate: 3 Hz
* Integration Time: 300 ms
* BG_INIT_FRAMES = 20
* HUMAN_PIX_MIN_Z3 = 2
* PRESENT_HOLD_FRAMES = 2
* Auto Reboot = 30 minutes

---

## Validated Scenarios

Successfully validated for:

* Stationary detection in Zone 1
* Stationary detection in Zone 2
* Stationary detection in Zone 3
* Empty-room recovery
* Stable session generation
* JSON session reporting

---

## Known Limitations

* Background learning requires an initially empty environment.
* Detection performance depends on target size and environmental conditions.
* Large environmental changes may require additional background adaptation time.

---

## Documentation

This release includes:

* README.md
* docs/system_architecture.md
* docs/software_architecture.md
---

# Future Development

Planned improvements include:

* Improved pass-by detection by extended range
* Multi-person tracking
* Dynamic parameter configuration
* Additional analytics metrics
