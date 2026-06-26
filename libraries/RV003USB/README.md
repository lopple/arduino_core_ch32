# RV003USB

Experimental USB HID support for CH32V003, based on
[cnlohr/rv003usb](https://github.com/cnlohr/rv003usb).

This library currently provides:

- USB HID keyboard support with explicit US and JIS layout headers.
- USB HID mouse support.
- USB HID monitor support for lightweight debug and stream-style I/O.

## HID Monitor

The HID monitor firmware support is intended to work with the
[`arduino-hid-monitor`](https://github.com/lopple/arduino-hid-monitor) tool.
It implements the version 1 monitor protocol used by the v0.1.0 pre-release.

Current descriptor and protocol expectations:

- Default VID/PID: `1209:C003`.
- Product string: `RV003USB HID Monitor`.
- USB serial string: generated from the MCU unique ID for multi-device
  identification.
- Feature report ID: `0xA0`.
- Feature report size: 64 bytes.
- Protocol version: `0x01`.
- Commands: `PING` (`0x01`), `WRITE` (`0x10`), `READ` (`0x11`), and
  `STATUS` (`0x12`).
- Optional interrupt IN notification report: `0xA1`, 8 bytes.

The `HIDMonitorEcho` example is the current smoke-test sketch for this
interface.

## Package Integration

This core contains the firmware/library side of HID monitor support. Arduino
Boards Manager integration is intentionally handled as release packaging work,
not as part of this library change.

When preparing a release, the board package should add the
`arduino-hid-monitor` tool dependency and the matching `platform.txt`
pluggable discovery/monitor patterns. Until then, sketches can still build the
firmware side, but Arduino IDE/CLI monitor integration depends on the local
package setup.
