# AGENTS.md

This file provides guidance to Codex (Codex.ai/code) when working with code in this repository.

## Project Overview

Bell-Manager ESP32 is an embedded system for managing school/workplace bell schedules using a **Sonoff POW Elite 16A** (POWR316D) power switch with an ESP32 microcontroller. The project consists of ESP32 firmware and a web interface for schedule management.

## Build & Upload

**Platform**: Arduino IDE with ESP32 Board Support Package

```bash
# Build in Arduino IDE
Sketch → Export Compiled Binary

# Upload via USB (CH340 adapter)
Sketch → Upload

# Serial Monitor
115200 baud
```

No automated tests or linting configured.

## Project Structure

```
Codice Esp32 Bell-Manager/
  └── Sketch_Esp32BellManager/
      └── Esp32BellManager.ino     # Main firmware (Arduino sketch)

Interfaccia Web Bell-Manager/
  └── Bell-Manager.html            # Complete web UI (SPA, vanilla JS)

Progettazione e Analisi dei Requisiti/
  └── *.pdf                        # Requirements and UI mockups
```

## Hardware Architecture

**Target**: Sonoff POW Elite 16A with ESP32

### GPIO Pinout (Critical)
| GPIO | Function | Notes |
|------|----------|-------|
| GPIO0 | User Button | Active LOW, BOOT pin |
| GPIO5 | WiFi LED | Active LOW |
| GPIO13 | Relay Control | Active HIGH, 16A relay |
| GPIO14 | TM1621 DATA | LCD display |
| GPIO16 | CSE7759B RX | Energy sensor @ 4800 baud, even parity |
| GPIO18 | Relay Status LED | Active LOW |
| GPIO25 | TM1621 CS | LCD chip select |
| GPIO26 | TM1621 RD | LCD read |
| GPIO27 | TM1621 WR | LCD write/clock |

### Key Components
- **Display**: TM1621 LCD controller - uses custom 4-wire serial protocol (NOT I2C/SPI)
- **Energy Sensor**: CSE7759B via UART (4800 baud, even parity)
- **Relay**: Monostable, driven by GPIO13 (HIGH = ON)

## Firmware Architecture

**Dependencies** (built-in ESP32):
```cpp
#include <WebServer.h>
#include <WiFi.h>
```

**Network Config** (hardcoded):
- WiFi Mode: Access Point
- SSID: `Bell-Manager`
- Password: `12345678910`
- Web Server: Port 80

**Current State**: Minimal implementation - web server serves static HTML. Missing:
- GPIO control logic (relay/LEDs)
- CSE7759B energy sensor parsing
- TM1621 display driver (requires bit-banging)
- Schedule execution engine
- EEPROM persistence for schedules
- REST API endpoints for UI↔firmware communication

## Web Interface Architecture

Single-page application with vanilla JavaScript. Features:
- Bell schedule table with add/edit/delete
- Per-bell toggle switches
- Settings modal (institution name, master disable, bell log)
- Day-of-week selection (Italian: Lun, Mar, Mer...)

**Data stored in JS arrays** - no backend integration yet.

## Language

All code comments and UI text are in **Italian**.
