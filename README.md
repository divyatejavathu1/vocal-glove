# vocal-glove
# VocalGlove – Translating Silent Gestures into Real-Time Audible Speech

## Project Overview

VocalGlove is a wearable assistive technology project designed to support communication for individuals with speech and hearing impairments. The system uses sensors attached to a glove to detect finger movements and hand orientation. The collected sensor data is processed by an ESP32 microcontroller to identify gesture patterns and support their conversion into meaningful communication outputs.

## Objectives

- Develop a wearable glove for sensor-based gesture detection.
- Detect finger bending using flexible sensors.
- Monitor hand movement and orientation using an MPU6050 IMU.
- Process sensor readings using an ESP32 microcontroller.
- Apply threshold-based recognition to identify gesture patterns.
- Support wireless monitoring through a connected interface.
- Establish a foundation for future text-to-speech communication.

## Hardware Components

- ESP32 Development Board
- Flexible (Flex) Sensors
- MPU6050 Inertial Measurement Unit (IMU)
- Breakout PCB
- Connecting Wires
- Glove for Sensor Mounting

## Software and Technologies

- Arduino IDE
- Embedded C/C++ for ESP32 firmware
- ESP32 Bluetooth and sensor libraries, as required by the implementation
- Wireless monitoring interface

## Working Principle

1. Flexible sensors measure the bending of the fingers.
2. The MPU6050 captures hand movement and orientation data.
3. The ESP32 reads and processes the available sensor measurements.
4. Predefined threshold values are used to identify gesture patterns.
5. The system supports wireless communication for monitoring sensor readings and system responses.
6. Recognized gestures can be mapped to corresponding text or speech outputs, depending on the implemented functionality.

## Recorded Pin Mapping

| Component | ESP32 Pin |
|---|---|
| Index Finger Flex Sensor | GPIO 34 |
| Middle Finger Flex Sensor | GPIO 35 |
| Ring Finger Flex Sensor | GPIO 32 |
| MPU6050 SDA | GPIO 21 |
| MPU6050 SCL | GPIO 22 |
| Sensor Supply | 3.3V |
| Ground | GND |

Verify these connections against the final hardware assembly before use.

## Source Code

The ESP32 firmware is provided in the file:

`VocalGlove_ESP32.ino`

Open the file using the Arduino IDE to inspect or modify the firmware.

## Limitations

The performance of gesture recognition depends on sensor calibration, threshold selection, and the available sensor channels. Additional testing and integration may be required to improve recognition reliability and expand the number of supported gestures.

## Future Enhancements

- Integration of additional flex sensors.
- Improved sensor calibration and noise filtering.
- Support for a larger gesture vocabulary.
- Exploration of machine-learning-based gesture recognition.
- Improved text-to-speech functionality.
- Enhancements to wireless connectivity and the user interface.

## Project Information

**Project Title:** VocalGlove – Translating Silent Gestures into Real-Time Audible Speech

**Course:** ECS3001 – Engineering Clinics II

**Institution:** VIT-AP University

**Academic Year:** 2026–2027

## License

This project is intended for academic and educational purposes.
