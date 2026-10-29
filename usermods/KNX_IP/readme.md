# KNX IP Usermod

This usermod enables KNX/EIB integration for WLED, allowing control of LED strips and segments via KNX bus communication.

## Installation

Copy the example `platformio_override_KNX_IP.ini` to the root directory and rename it to `platformio_override.ini`.  This file should be placed in the same directory as `platformio.ini`.

## Features

- **Central Control**: Control main strip power, brightness, color, and effects via KNX
- **Per-Segment Control**: Individual KNX control for each WLED segment with configurable Group Address offsets
- **GA Conflict Detection**: Automatic validation to prevent duplicate Group Addresses
- **Bidirectional Communication**: Send and receive KNX telegrams
- **Multiple Data Types**: Support for boolean, 1-byte, 2-byte, and 3-byte KNX data types

## Testing

Comprehensive test suites are available in the `/test/` directory:
- **Unit Tests**: Core functionality validation
- **Integration Tests**: End-to-end testing with real KNX stack
- **GA Conflict Tests**: Address conflict detection validation

See `/test/README_TESTING.md` and `/test/GA_CONFLICT_TESTS.md` for detailed testing information.

## Debug Logging

See `README_KNX_DEBUG.md` for information on enabling detailed debug output for troubleshooting.
## Per-segment colour objects, segment status and read responses

Additions (branch `knx-segment-color`):

* **Per-segment IN objects** for colour: RGB (DPST-232-600), CCT (DPT 7.600, Kelvin),
  WW / CW (DPT 5.010) and W (DPT 5.010). Registered only if the segment has the
  matching light capability (RGB / CCT / white).
* **Per-segment status (OUT)**: Power (DPT 1.001), Brightness (DPT 5.001, segment opacity,
  0 % when off), Effect, RGB, CCT, WW, CW, W. Sent automatically when the value changes
  (polled every `max(300 ms, tx_rate_limit_ms)`); with `periodic_enabled` all segment
  status objects are re-sent periodically. (Previously the segment OUT objects were
  registered but never sent.)
* **GroupValue_Read is answered** on every central and per-segment status object
  (option `answer_read_requests`, default on) – needed so that Home Assistant,
  visualisations or wall panels get the current state after a restart.
* Segment GA scheme is unchanged: `GA(segment n) = central GA + n × (L/M/N offset)`.
  Segment 0 is always identical to the central objects.
* The GA conflict check now also covers the colour objects and the central
  CCT/WW/CW addresses; changing the segment offsets now triggers a re-registration.
* Changing the number of segments in the UI re-registers the segment objects after 1 s.

Config keys: `segment_color_objects` (bool, default true), `answer_read_requests` (bool, default true).

Note: `cct_kelvin_min` / `cct_kelvin_max` should be set to the real warm/cold white
colour temperature of the strip – Kelvin values are mapped linearly onto WLED's 0..255 CCT.
