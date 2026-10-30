# SmartDry: IoT-Based Intelligent Drying Monitoring and Control System

## Project Overview

SmartDry is an IoT-based smart drying system developed as a proof of concept for the AEC Internship Smart Device Design Challenge, under Case 3: Smart Living & Intelligent Automation.

The project explores how environmental monitoring, automated fan control, and sensor fault detection can support a more responsive drying process. The prototype was developed using an ESP32 simulation in Wokwi, MQTT communication, and a Node-RED Dashboard for real-time monitoring.

## Problem Identification

Conventional drying activities often depend on manual observation and adjustments. Changes in temperature and humidity may go unnoticed, while users may need to repeatedly check environmental conditions and adjust equipment.

SmartDry investigates how a sensor-based control system could reduce the need for continuous manual monitoring and provide clearer information about operating conditions.

## Proposed Solution

SmartDry integrates temperature, humidity, and load measurements with automated control and monitoring.

The prototype includes:

- Real-time temperature and humidity monitoring using DHT22.
- Load measurement using HX711 and a simulated load cell.
- Automatic fan control using three PWM levels.
- Temperature-based safety alerts and simulated cooling override.
- Sensor fault detection for DHT22 and HX711.
- MQTT-based communication between ESP32 and Node-RED.
- Connection status monitoring using a heartbeat mechanism.
- CSV data logging for monitoring and analysis.

## Hardware and Software

| Component | Purpose |
|---|---|
| ESP32 | Main microcontroller and control logic |
| DHT22 | Temperature and relative humidity measurement |
| HX711 and Load Cell | Load mass measurement |
| Three LEDs | Visual representation of PWM fan operating modes |
| Wokwi | Virtual hardware simulation |
| MQTT / HiveMQ | Communication between ESP32 and Node-RED |
| Node-RED Dashboard 2.0 | Real-time monitoring interface |
| CSV Logging | Recording sensor data and system states |

## Control Logic

During normal operation, the fan mode is determined by relative humidity.

| Humidity | Fan Mode | PWM Duty |
|---|---|---|
| Below 50% | LOW | 30% |
| 50% to below 60% | MEDIUM | 60% |
| 60% and above | HIGH | 100% |

Temperature monitoring takes priority when the temperature reaches a defined test threshold.

| Temperature | Status | System Response |
|---|---|---|
| Below 45°C | NORMAL | Humidity-based fan control |
| 45°C to below 50°C | WARNING | Cooling override, HIGH mode |
| 50°C and above | CRITICAL | Cooling override, HIGH mode |

These temperature thresholds were selected for simulation testing and should not be interpreted as validated safety limits for physical drying equipment.

## Sensor Fault Detection

SmartDry includes fault detection to distinguish invalid sensor measurements from normal operating conditions.

When DHT22 data becomes invalid, temperature and humidity values are transmitted as `null`, and the simulated fan indicators are disabled.

When HX711 becomes unavailable, the load measurement is transmitted as `null`. Temperature-based and humidity-based control can continue as long as DHT22 remains valid.

The Node-RED Dashboard displays sensor health information, allowing users to identify sensor failures separately from communication interruptions.

## System Architecture

The ESP32 collects sensor measurements and applies the control logic locally. It then publishes monitoring data through MQTT to Node-RED.

Node-RED processes the incoming messages, updates the dashboard, monitors the arrival of new data through a heartbeat mechanism, and records selected parameters in CSV format.

The simulation uses the public HiveMQ MQTT broker for demonstration purposes.

## Repository Contents

| File | Description |
|---|---|
| `sketch.ino` | ESP32 firmware and control logic |
| `diagram.json` | Wokwi circuit configuration |
| `libraries.txt` | Wokwi library dependencies |
| `node-red-flow.json` | Node-RED flow configuration |
| `README.md` | Project documentation |

## How to Run

1. Open the SmartDry project in Wokwi.
2. Confirm that the required libraries are available.
3. Start the ESP32 simulation and observe the Serial Monitor.
4. Run Node-RED locally and import `node-red-flow.json`.
5. Install the required Node-RED Dashboard 2.0 components if they are not already available.
6. Check the MQTT broker configuration and dashboard settings.
7. Open the Node-RED Dashboard to monitor incoming measurements, fan states, and sensor health.
8. Review the generated CSV data for recorded system activity.

## Testing and Results

The prototype was evaluated through simulation-based integration testing.

Five operating scenarios were tested to verify humidity-based fan control, temperature warnings, and critical-temperature responses.

Additional fault injection tests were performed by simulating DHT22 and HX711 disconnections. The system successfully reported sensor error states through MQTT, and the dashboard was configured to handle invalid readings without interpreting them as valid zero values.

The system also demonstrated recovery to normal monitoring after the simulated sensor connections were restored.

## Engineering Limitations

SmartDry remains a simulation-based proof of concept.

The LED outputs represent PWM control commands rather than physical fan speed. Load cell calibration requires further verification, and the current drying stage indicator is based on ambient humidity rather than direct confirmation of material dryness.

The prototype does not demonstrate verified energy savings, physical drying performance, or hardware-level safety protection.

Future development could include a physical fan and heater prototype, calibrated load measurements, moisture estimation using mass trends, and independent thermal protection.

## Project Context

Developed for the AEC Internship Smart Device Design Challenge.

**Selected Case:** Case 3 — Smart Living & Intelligent Automation

**Development Approach:** Simulation, IoT integration, automated control, and fault detection.
