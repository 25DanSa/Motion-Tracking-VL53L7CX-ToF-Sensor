# Software Architecture

## 1. Project Overview

The firmware acts as a communication gateway between the sensor MCU and the cloud backend.

Responsibilities include:

* BLE provisioning
* WiFi management
* Time synchronization
* UART communication
* Session processing
* JSON generation
* HTTPS upload

---

# 2. High-Level Architecture

```
Android App
      │
      │ BLE
      ▼
ESP32
 ├── BLE Provisioning
 ├── WiFi Manager
 ├── NTP Client
 ├── UART Parser
 ├── JSON Builder
 ├── Upload Manager
 └── Cloud API
      │
      ▼
Cloud
```

---

# 3. Main Runtime Flow

```
BOOT

↓

BLE Advertising

↓

Waiting for WiFi Credentials

↓

Credentials Received

↓

Store to Preferences

↓

Connect WiFi

↓

NTP Synchronization

↓

Wait for UART Session

↓

Build JSON

↓

Upload

↓

Wait for next Session
```

---

# 4. BLE WiFi Provisioning

The firmware exposes one BLE service.

## Service

WiFi Service

UUID

```
E46A6E40-1008-4001-1333-DF45C65AE082
```

Characteristics

Settings

```
E46A6E41...
```

Connection Details

```
E46A6E42...
```

---

# 5. Provisioning Flow

1. Android discovers ESP32.
2. Connects.
3. Writes CBOR credentials.
4. ESP32 validates.
5. Stores credentials.
6. Sends Settings indication.
7. Starts WiFi connection.
8. Sends ConnectionDetails indication.
9. Application reports success.

---

# 6. WiFi State Machine

```
WAITING

↓

CONNECTING

↓

CONNECTED

↓

Connection Lost

↓

Retry 1

↓

Retry 2

↓

Retry 3

↓

WAITING
```

Features

* runtime reprovisioning
* reconnect after AP loss
* retry backoff
* non-blocking operation

---

# 7. Preferences Storage

Namespace

```
wifi
```

Stored keys

* auth
* ssid
* pass

---

# 8. NTP Synchronization

NTP Server

```
20.113.173.199
```

Synchronization occurs after successful WiFi connection.

Generated timestamps are used for

* session start
* session end
* entry timestamps

---

# 9. UART Communication

UART

9600 baud

Line-oriented JSON.

Expected message

```
session_end
```

---

# 10. Session Processing

The ESP32 extracts

* total duration
* zones
* dwell times
* first entry timestamps
* final zone

---

# 11. JSON Builder

Generated fields include

* sessionID
* sensorID
* objectID
* startTime
* endTime
* totalDuration
* conversionEvent
* zones
* finalState

---

# 12. Cloud Upload

Upload sequence

```
Init

↓

Append

↓

Complete
```

Chunk size

1024 bytes

Integrity

* SHA1 per chunk
* SHA1 complete file
* MD5 file identifier

---

# 13. Error Handling

BLE

* invalid CBOR
* malformed credentials

WiFi

* retries
* reconnect
* runtime reprovisioning

Cloud

* HTTP error detection
* upload status reporting

---

# 14. Configuration

Main configurable parameters

* Device ID
* Assigned ID
* Store ID
* Campaign ID
* Cloud URL
* API key
* BLE name
* UART pins
* Chunk size
* Retry count
* NTP server

---

# 15. Future Improvements

* OTA firmware update
* Upload queue for offline operation
* Persistent upload cache
* TLS certificate validation
* Device diagnostics
* Remote configuration
* Cloud acknowledgements
* Firmware version reporting
