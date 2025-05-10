# 🕶️ Smart Glasses – Hardware Module

This directory contains all the hardware-related files, schematics, and firmware code for the **Smart Glasses** component of the TrackSense assistive technology system.

The Smart Glasses are designed to provide real-time **scene recognition** and **audio feedback** to visually impaired users by capturing images of the surroundings and interpreting them using AI-powered vision models.

---

## 🔧 Features

- **Image Capture** via camera module (ESP32-CAM)
- **AI-Powered Scene Description** through integration with OpenAI/Gemini Vision
- **Voice Feedback** using text-to-speech to read descriptions aloud
- **Button Trigger** to capture images on demand
- **Wireless Communication** (Wi-Fi/Bluetooth) with backend or mobile app

---

## 📁 Contents

/smart_glasses
├── firmware/ # Microcontroller code (ESP32-CAM)
├── schematics/ # Circuit diagrams and wiring schematics
├── parts_list.md # Bill of Materials (BOM)
├── notes.md # Setup notes or build instructions
└── README.md # This file

---

## 🔌 Hardware Components

- ESP32-CAM module
- Push Button (to trigger camera)
- Power Supply (LiPo battery or USB)
- Optional: Microphone & speaker (for onboard TTS feedback)

---

## 🛠 Setup Instructions

1. Assemble the components using the wiring guide in `/schematics/`.
2. Upload the firmware from `/firmware/` to the ESP32-CAM using Arduino IDE or PlatformIO.
3. Connect to Wi-Fi or Bluetooth (as configured) for sending images to the backend.
4. Test the image capture and feedback system by pressing the button and verifying the audio description output.

---

## 🗣️ Voice Feedback

- Captured images are processed by an AI service (e.g., OpenAI/Gemini Vision) on the backend.
- Results are sent back and converted to speech using a TTS engine.
- Audio is either played through a connected speaker or sent to a paired device (e.g., phone or earpiece).

---

## 📌 Notes

- Ensure a stable power supply for the ESP32-CAM to avoid reboot loops.
- For accurate AI descriptions, ensure the camera is angled properly in the glasses frame.
- Optimize image resolution and compression settings in firmware for faster transmission.
