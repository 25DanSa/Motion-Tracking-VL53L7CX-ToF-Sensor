# Code Walkthrough

This document explains the main software components and functions used in the MSPM0L1117 + VL53L7CX Motion Presence Tracker firmware.

---

# Overview

The firmware uses the VL53L7CX Time-of-Flight sensor to detect human presence and generate session analytics.

Unlike later baseline-learning versions, this implementation does not build a background model.

Detection is based on:

* Distance filtering
* Blob extraction
* Motion indicator gating
* Session state machine

Processing pipeline:

```text
VL53L7CX Frame
↓
Blob Extraction
↓
Motion Gate
↓
Session State Machine
↓
Ring Buffer
↓
UART JSON Output
```

---

# Time Functions

## millis()

```c
static inline uint32_t millis(void)
```

Returns milliseconds since MCU startup.

Used for:

* session timing
* dwell time measurement
* timeout handling
* time synchronization

---

## current_epoch_ms()

```c
static uint64_t current_epoch_ms(void)
```

Returns current absolute time in milliseconds.

Calculation:

```text
epoch_at_sync_ms
+
(millis() - millis_at_sync)
```

Used when generating session records.

---

## parse_u64()

```c
static uint64_t parse_u64(const char *s)
```

Converts incoming UART timestamp strings into 64-bit integers.

Example:

```text
T,1748450012
```

---

# Session Management

## reset_session()

```c
static void reset_session(void)
```

Resets all session tracking variables.

Clears:

* session state
* current zone
* dwell times
* zone history
* timestamps

Called:

* after boot
* after session completion

---

## store_session()

```c
static void store_session(void)
```

Creates a completed session record.

Stores:

* session duration
* final zone
* zone dwell times
* absolute timestamps

Output destination:

```text
ring buffer
```

---

## send_buffer()

```c
static void send_buffer(void)
```

Transmits all stored session records.

Output format:

```text
BUF_BEGIN
JSON
JSON
BUF_END
```

Used when:

* buffer becomes full
* host requests data

---

# UART Functions

## handle_uart_command()

```c
static void handle_uart_command(const char *cmd)
```

Processes commands received from the host.

Supported commands:

### Time Synchronization

```text
T,<epoch>
```

Example:

```text
T,1748450012
```

Stores:

* epoch_at_sync_ms
* millis_at_sync

---

### Buffer Request

```text
GET
```

Immediately transmits all buffered records.

---

## poll_uart_commands()

```c
static void poll_uart_commands(void)
```

Checks UART receive buffer.

Responsibilities:

* collect incoming bytes
* build command strings
* execute completed commands

Runs continuously in the main loop.

---

# Utility Functions

## ts_valid()

```c
static inline bool ts_valid(uint8_t ts)
```

Validates VL53L7CX target status.

Accepted values:

```text
5
9
```

These correspond to reliable ranging results.

---

## zone_from_dist()

```c
static inline uint8_t zone_from_dist(int d)
```

Maps distance to a tracking zone.

| Zone | Distance     |
| ---- | ------------ |
| Z1   | < 750 mm     |
| Z2   | 750–1500 mm  |
| Z3   | 1500–2800 mm |

---

## row_of()

```c
static inline int row_of(int idx)
```

Converts a 4x4 sensor index into row number.

---

## col_of()

```c
static inline int col_of(int idx)
```

Converts a 4x4 sensor index into column number.

---

## abs_i()

```c
static inline int abs_i(int x)
```

Integer absolute value helper.

Used during blob clustering.

---

# Blob Extraction

## extract_human_blob()

```c
static blob_t extract_human_blob(
    const VL53L7CX_ResultsData *R)
```

Most important detection function in the firmware.

Purpose:

Convert raw VL53L7CX data into a valid human candidate.

Processing steps:

### 1. Distance Filtering

Reject:

```text
distance < MIN_TRACK_MM
distance > MAX_TRACK_MM
invalid target status
```

---

### 2. Seed Selection

Find nearest valid pixel.

This pixel becomes the starting point for clustering.

---

### 3. Flood Fill Clustering

Neighbouring pixels are added when:

```text
row difference <= 1
column difference <= 1
distance difference <= MAX_NEIGHBOR_DELTA
```

---

### 4. Blob Metrics

Calculate:

* pixel count
* minimum distance
* row span
* column span

---

### 5. Human Candidate Validation

Reject blobs that:

* contain too few pixels
* are too wide
* are too tall
* fall outside valid zones

---

### Output

Returns:

```c
blob_t
```

Containing:

* found flag
* pixel count
* best distance
* zone
* dimensions

---

# Motion Indicator

## motion_gate_pass()

```c
static uint8_t motion_gate_pass(
    const VL53L7CX_ResultsData *R)
```

Uses the VL53L7CX motion indicator plugin.

Purpose:

Prevent false session starts.

Operation:

Count motion zones where:

```text
motion >= MOTION_START_THR
```

If enough zones report motion:

```text
hitZones >= MOTION_ZONES_MIN
```

Motion gate passes.

Otherwise session start is blocked.

---

# Session State Machine

The session logic is implemented directly inside the main loop.

## Session Start

Requirements:

```text
Valid Blob
+
Motion Gate Pass
+
START_CONSEC_FRAMES consecutive frames
```

When satisfied:

```text
sessionActive = true
```

Session begins.

---

## Session Tracking

While session is active:

* current zone tracked
* dwell times accumulated
* last seen timestamp updated

---

## Zone Changes

When a new zone is entered:

```text
zoneEntered[]
zoneFirstEntryMs[]
```

are updated.

---

## Session End

If no valid blob is detected for:

```text
END_MISSING_MS
```

the session ends.

If:

```text
duration >= MIN_SESSION_MS
```

the session is stored.

Otherwise it is discarded as noise.

---

# Main Function

## main()

```c
int main(void)
```

Main firmware entry point.

Initialization:

```text
System Init
↓
UART Init
↓
SysTick Init
↓
I2C Init
↓
VL53L7CX Init
↓
Motion Indicator Init
↓
Start Ranging
↓
Reset Session State
```

Runtime Loop:

```text
Poll UART Commands
↓
Wait For Frame
↓
Read VL53L7CX Data
↓
Extract Blob
↓
Check Motion Gate
↓
Update Session FSM
↓
Store Session
↓
Transmit JSON
↓
Repeat
```

---

# Design Goals

* Low power operation
* Simple implementation
* Robust presence detection
* Motion-gated session start
* Minimal memory footprint
* UART JSON integration
* Easy host MCU integration

---

# Future Improvements

* Direction tracking
* Improved pass-by detection
* Multi-person tracking
* Baseline learning variants
* Enhanced analytics

```
```
