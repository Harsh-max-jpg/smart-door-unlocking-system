# Hardware Guide

## Components

| Component            | Purpose                                   |
|-----------------------|--------------------------------------------|
| ESP32 (WROOM-32)      | Main microcontroller / door controller     |
| IR Sensor             | Detects presence of a person/object        |
| 5V Relay Module       | Switches the door lock mechanism           |
| 16x2 I2C LCD          | Displays system status messages            |
| Servo Motor           | Physically moves the door latch            |
| RGB / discrete LEDs   | Red = access denied, Green = access granted|
| Resistors             | Current-limiting for LEDs                  |
| Breadboard, jumper wires | Prototyping / wiring                    |
| Laptop + Webcam       | Runs face detection/recognition (Python)   |

This list matches the "Components used" section of the original project
report and the parts referenced in the ESP32 sketch.

## Pin Configuration (verified against firmware source)

These pin numbers come directly from `arduino/smart_door_esp32/smart_door_esp32.ino`
(`#define` block), which is the authoritative source — not the report text.

| Signal          | ESP32 GPIO |
|-----------------|-----------:|
| IR Sensor (OUT) |         13 |
| Red LED         |         25 |
| Green LED       |         26 |
| Relay (IN)      |         27 |
| Servo (signal)  |         14 |
| I2C SDA (LCD)   |         21 |
| I2C SCL (LCD)   |         22 |

LCD I2C address used in code: **0x27**.

> **Discrepancy note:** the circuit diagram included in the original
> project report shows a slightly different pin mapping (IR sensor on
> D12, Green LED on D33) than the mapping actually compiled into the
> firmware (IR sensor on GPIO 13, Green LED on GPIO 26, as listed
> above). If you are wiring hardware from the report's diagram image,
> double-check against the table above — the table reflects the pins
> the code will actually read/drive.

## Logic levels / behavior notes

- **Relay:** the module used here is **active-LOW** — `HIGH` = relay
  de-energized = door locked (the safe/idle state), `LOW` = relay
  energized = door unlocked. This is set on boot (`RELAY_PIN, HIGH`).
- **Servo:** 0° = locked position, 90° = unlocked position.
- **IR sensor:** output reads `LOW` when an object/person is detected
  in range, `HIGH` otherwise (standard for the common IR obstacle
  sensor modules used with the LM393 comparator).

## Wiring summary

- IR sensor: VCC → 3.3V, GND → GND, OUT → GPIO13
- Red LED: anode → GPIO25 (through resistor), cathode → GND
- Green LED: anode → GPIO26 (through resistor), cathode → GND
- Relay module: IN → GPIO27, VCC → 5V, GND → GND
- Servo: signal → GPIO14, VCC → 5V, GND → GND
- LCD (I2C): SDA → GPIO21, SCL → GPIO22, VCC → 3.3V/5V (per your LCD backpack), GND → GND

Power the relay, servo and LCD from a stable 5V supply capable of
handling the servo's stall current; sharing the ESP32's onboard 3.3V
regulator for these is not recommended.
