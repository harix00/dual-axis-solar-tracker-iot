# Autonomous Dual-Axis Solar Tracker with API-Based Weather Safe-Mode and IoT Cloud Monitoring

An embedded dual-axis solar tracking prototype built around an **ATmega328P** microcontroller (16 MHz, programmed in bare-metal C) paired with a **NodeMCU (ESP8266)** Wi-Fi module for live weather forecasting, protective safe-mode control, local display output, and cloud telemetry.

## Key Features
- **Closed-Loop Dual-Axis Tracking:** Four corner-mounted LDR voltage dividers (`PC0`–`PC3`) compare left/right pairs for azimuth (`PB1` / `OC1A`) and top/bottom pairs for elevation (`PB3` / `OC2A`) with deadband filtering and step clamping to prevent jitter.
- **API-Based Weather Safe-Mode:** NodeMCU queries the **OpenWeatherMap API** over Wi-Fi and sends `'R'` (rain/adverse weather) or `'C'` (clear) at 9600 baud over UART through a bi-directional logic level converter to `PD0` (`RXD`).
- **Manual Override & Status Indicator:** An active-low switch on `PD2` forces safe mode, driving both servos to a flat horizontal position, bypassing LDR tracking, and turning on the safe-mode LED on `PB5`.
- **Local Display & IoT Cloud Logging:** Periodic telemetry packets (`$az,el,tl,tr,bl,br,light_pct,moved,mode`) are sent to the NodeMCU to update a **16x2 I2C LCD** (with NTP time and weather) and log data to **ThingSpeak**.

## Hardware & Pin Mapping
| Component / Signal | Pin / Interface | Notes |
| :--- | :--- | :--- |
| Top-Left LDR Divider | `PC0` (`ADC0`) | `5V` -> LDR -> `PC0` -> `10kΩ` -> `GND` |
| Top-Right LDR Divider | `PC1` (`ADC1`) | `5V` -> LDR -> `PC1` -> `10kΩ` -> `GND` |
| Bottom-Left LDR Divider | `PC2` (`ADC2`) | `5V` -> LDR -> `PC2` -> `10kΩ` -> `GND` |
| Bottom-Right LDR Divider | `PC3` (`ADC3`) | `5V` -> LDR -> `PC3` -> `10kΩ` -> `GND` |
| Azimuth SG90 Servo | `PB1` (`OC1A`) | Timer1 PWM; powered from dedicated 5V rail + common GND |
| Elevation SG90 Servo | `PB3` (`OC2A`) | Timer2 PWM; powered from dedicated 5V rail + common GND |
| Safe-Mode LED | `PB5` | Active high via `330Ω` resistor |
| Manual Safe Switch | `PD2` | Active low to `GND` (internal pull-up enabled) |
| UART RX / TX | `PD0` / `PD1` | 9600 8N1 via 3.3V–5V bi-directional logic level converter |
| NodeMCU Serial & LCD | `D5`/`D6`, `D2`/`D1` | `SoftwareSerial` to ATmega328P; I2C (`SDA`/`SCL`) to 16x2 LCD |

## Repository Structure
- `ATmega328P_BareMetal_C/main.c` — Bare-metal register-level C firmware for ATmega328P (Microchip Studio).
- `NodeMCU_ESP8266_IoT/NodeMCU_ESP8266_IoT.ino` — Wi-Fi, OpenWeatherMap API, NTP clock, 16x2 I2C LCD, and ThingSpeak uploader (Arduino IDE).
