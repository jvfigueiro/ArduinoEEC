# 🚗 ArduinoEEC (Ford DCL Protocol Interface)

A hardware and software interface for the **Arduino Mega 2560 R3** to communicate with Ford EEC-IV Engine Control Units (ECUs) from the 1990s using the **DCL (Data Communication Link)** protocol.

This project implements a state machine capable of syncing with the ECU at 2400 baud, handling the DCL parity/checksum rules, switching to 9600 baud for high-speed communication, and extracting raw diagnostic data.

## ⚠️ Disclaimer
This project involves direct communication with your vehicle's ECU. **Use it at your own risk.** I am not responsible for any damage caused to your ECU, wiring harness, or Arduino board. Always ensure your wiring is correct before plugging it into the diagnostic port.

## 🛠️ Hardware Requirements

* **Arduino Mega 2560 R3** (Requires multiple hardware serial ports; `Serial1` is used for ECU communication, `Serial` for PC debugging).
* **TTL to RS485 Module** (e.g., MAX485) to handle the differential signals from the Ford diagnostic port.
* **3x Push Buttons** for mode selection.

### 🔌 Pinout & Wiring

Based on the current implementation, wire your hardware as follows:

| Component | Arduino Mega Pin | Description |
| :--- | :--- | :--- |
| RS485 Module `RO` (RX) | `RX1` (Pin 19) | Serial1 Receive |
| RS485 Module `DI` (TX) | `TX1` (Pin 18) | Serial1 Transmit |
| RS485 Module `DE/RE` | `D2` | Transmit/Receive Enable Pin |
| Button 1 | `D4` | Mode: Read Fault Codes (Pull-up) |
| Button 2 | `D5` | Mode: KOEO Test (Pull-up) |
| Button 3 | `D6` | Mode: Live Data (Pull-up) |

*Note: The buttons are configured with `INPUT_PULLUP`, meaning they should connect the digital pin to `GND` when pressed.*

## 🚀 Features & Operation Modes

The system operates using an interrupt-driven state machine with three main modes selectable via the physical buttons:

1. **Read Fault Codes:** Retrieves stored DTCs (Diagnostic Trouble Codes) from the ECU memory.
2. **KOEO Test:** Triggers the Key On Engine Off self-test routine.
3. **Live Data:** Requests and streams real-time engine parameters (RPM, TPS, ECT, etc.) using the internal PID map.

All raw hexadecimal data and debug messages are printed to the primary `Serial` monitor at `115200 baud`.

## 🤝 Credits & Acknowledgements

This project was built upon the incredible work and documentation from the open-source community:
* [babroval - FORD EEC-IV diagnostic scanner](https://github.com/babroval/ford-eec-iv-diagnostic)
* [flxkrmr - Arduino EEC IV Reader](https://github.com/flxkrmr/eec-iv-reader-arduino)

Contributions, pull requests, and help decoding specific PID hex values are always welcome!
