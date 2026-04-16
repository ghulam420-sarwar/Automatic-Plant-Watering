# Automatic Plant Watering System

An ESP32-based automatic irrigation system that monitors soil moisture, temperature, and humidity, and drives a small 5V pump via a MOSFET to keep your plant at the right moisture level. A live OLED shows the moisture bar graph, temperature, and pump status. Includes tank-low detection and a safety cooldown timer.

> Inspired by the AI-based Agronomist Bachelor thesis — bringing smart sensing to practical plant care.

![Circuit diagram](https://github.com/ghulam420-sarwar/Automatic-Plant-Watering/blob/main/circuit_diagram.png)

## Features

- **Hysteresis control**: pump ON below 35 %, OFF above 65 % — prevents on/off cycling
- **Safety limits**: 10 s max pump runtime, 5 min cooldown between cycles
- **Tank-low detection**: alerts if moisture barely rises after a full pump cycle
- **OLED moisture bar** graph + temperature/humidity readout
- **Serial log** for validation with logic analyser or PC terminal

## Hardware

| Component                  | Role                         | Pin    |
|----------------------------|------------------------------|--------|
| ESP32 DevKit               | MCU                          | —      |
| Capacitive soil sensor     | Moisture (ADC)               | GPIO34 |
| DHT22                      | Air temp + humidity          | GPIO4  |
| IRLZ44N N-MOSFET           | Pump driver                  | GPIO26 |
| 5V mini submersible pump   | Watering actuator            | —      |
| SSD1306 OLED 128×64        | Local display                | I²C    |
| Red LED                    | Tank-low / fault alert       | GPIO27 |
| Green LED                  | Pump running indicator       | GPIO14 |

### MOSFET Wiring

```
GPIO26 ──[220Ω]── GATE
GND   ──────────── SOURCE
PUMP+ ─────────── DRAIN
PUMP- ─────────── 5V GND  (shared with ESP32 GND)
```

Add a **1N4007 flyback diode** across the pump terminals (cathode to +5V).

## Calibration

1. Run the sketch and open Serial Monitor
2. Hold sensor **in air** → note the raw ADC value → set `AIR_VALUE`
3. Hold sensor **in water** → note the raw ADC value → set `WATER_VALUE`
4. Re-flash — moisture readings will now be in %

## Build

```bash
pio run -t upload
pio device monitor
```

## Serial Output

```
Plant Watering System ready
Moisture= 28%  T=24.1  H=52.3  Pump=0  TankLow=0
Moisture= 28%  T=24.1  H=52.3  Pump=1  TankLow=0   ← pump ON
Moisture= 31%  T=24.1  H=52.5  Pump=1  TankLow=0
Moisture= 67%  T=24.2  H=53.0  Pump=0  TankLow=0   ← wet threshold reached
```

## What I Learned

- Capacitive vs. resistive soil sensors: capacitive avoids corrosion
- MOSFET gate resistor selection to limit switching current from GPIO
- Flyback diode necessity when switching inductive loads (pump motor)
- Hysteresis prevents relay/pump chatter — simple but crucial control design

## License

MIT © Ghulam Sarwar
