# MSPM0L1117 + VL53L7CX Presence Tracker

Presence tracking and session analytics using the Texas Instruments MSPM0L1117 MCU and ST VL53L7CX Time-of-Flight sensor.

## Features

- VL53L7CX 4x4 ranging mode
- Motion-gated presence detection
- Blob clustering
- 3-zone tracking
- Dwell time measurement
- Session generation
- UART JSON output
- Ring buffer storage
- Time synchronization support
- Low-power MSPM0 implementation

## Hardware

### MCU
- MSPM0L1117

### Sensor
- VL53L7CX

### Communication
- UART JSON output

## Zone Layout

| Zone | Distance |
|--------|--------|
| Z1 | Near |
| Z2 | Mid |
| Z3 | Far |

## Session Output

Example:

```json
{
  "event":"session_end",
  "total_ms":4123,
  "final_zone":2,
  "zones":[
    {
      "idx":1,
      "dwell_ms":1000
    },
    {
      "idx":2,
      "dwell_ms":3123
    }
  ]
}
```

## Build Environment

- Code Composer Studio
- MSPM0 SDK
- VL53L7CX ULD

## Project Structure

source/
include/
VL53L7CX_ULD/

## Known Limitations

- Performance depends on integration time and ranging frequency.

## License

Vusion
