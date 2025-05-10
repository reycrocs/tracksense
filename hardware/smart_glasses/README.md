# 🦯 Smart Blind Stick – Hardware Module

This directory contains all hardware-related files, schematics, and code specific to the **Smart Blind Stick** component of the TrackSense project.

The smart stick is designed to assist visually impaired individuals by detecting nearby obstacles, enabling real-time tracking, and providing emergency communication features.

---

## 🔧 Features

- **Obstacle Detection** using ultrasonic sensors
- **Haptic Feedback** via vibration motor and buzzer
- **SOS Button** to send emergency alerts to a guardian
- **GPS Tracking** for real-time location monitoring
- **Weather Updates** delivered via connected mobile app

---

## 📁 Contents

- `/smart_stick/`
  - `firmware/`          # Microcontroller code (ESP32/C++)
  - `schematics/`        # Circuit diagrams and wiring schematics
  - `parts_list.md`      # Bill of Materials (BOM)
  - `notes.md`           # Build notes or setup instructions
  - `README.md`          # This file

---

## 🔌 Hardware Components

- ESP32 / ESP32 Dev Board
- Ultrasonic Sensor (e.g., HC-SR04)
- Vibration Motor
- Buzzer
- GPS Module (e.g., Neo-6M)
- Push Button (for SOS)
- Power Supply (battery pack or USB)

---

## 🛠 Setup Instructions

1. Connect all components based on the circuit diagram in `/schematics/`.
2. Flash the code from `/firmware/` onto the ESP32 using Arduino IDE or PlatformIO.
3. Power up the device and test:
   - Obstacle detection (vibration + buzzer)
   - GPS location fix
   - SOS alert triggering
4. Ensure Bluetooth/Wi-Fi is configured if communicating with the mobile app.

---

## 📌 Notes

- Be sure to test all components individually before final assembly.
- Proper cable management and casing are recommended for durability and portability.
- You may need to adjust ultrasonic sensor sensitivity in the firmware.
