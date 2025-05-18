# TrackSense - ESP32-CAM Audio Description System

This project uses an ESP32-CAM to capture images and receive audio descriptions of the captured scenes. The system captures images, sends them to a cloud API for processing, and plays back the audio description through a MAX98357A I2S amplifier.

## Hardware Requirements

- ESP32-CAM AI-Thinker
- MAX98357A I2S Amplifier
- Speaker (3W or higher recommended)
- USB to TTL Serial Adapter (for programming)
- 5V Power Supply
- Jumper Wires

## Wiring Diagram

### ESP32-CAM to MAX98357A I2S Amplifier

| MAX98357A Pin | ESP32-CAM Pin | GPIO | Notes                                    |
| ------------- | ------------- | ---- | ---------------------------------------- |
| VIN           | 5V            | —    | Power the amp (use 5V for louder output) |
| GND           | GND           | —    | Common ground                            |
| LRC/WS        | IO15          | 15   | Word Select / Left-Right Clock           |
| BCLK          | IO13          | 13   | Bit Clock                                |
| DIN           | IO12          | 12   | Audio Data (I2S signal)                  |

## Software Requirements

- Arduino IDE
- Required Libraries:
  - ESP32 Arduino Core
  - ESP32 Camera
  - ESP32 I2S
  - ESP32 SPIFFS
  - ESP32 HTTP Client
  - ESP32 WiFi
  - ESP32 Base64

## Features

- Automatic image capture at regular intervals
- WiFi connectivity for cloud communication
- Image processing via cloud API
- Text-to-speech conversion
- Audio playback through I2S amplifier
- LED flash control for better image quality
- Detailed logging via Serial Monitor

## Setup Instructions

1. **Hardware Setup**

   - Connect the ESP32-CAM to the MAX98357A amplifier according to the wiring diagram
   - Connect a speaker to the MAX98357A output
   - Connect the USB to TTL adapter for programming

2. **Software Setup**

   - Install Arduino IDE
   - Install ESP32 board support in Arduino IDE
   - Install required libraries
   - Configure WiFi credentials in the code
   - Upload the code to ESP32-CAM

3. **Configuration**
   - Set your WiFi credentials in the code:
     ```cpp
     const char* ssid = "YOUR_WIFI_SSID";
     const char* password = "YOUR_WIFI_PASSWORD";
     ```
   - Adjust the capture interval if needed:
     ```cpp
     const int Interval = 20000; // 20 seconds
     ```

## Usage

1. Power on the device
2. The ESP32-CAM will:
   - Connect to WiFi
   - Initialize the camera
   - Test the connection to the API
   - Start capturing images at regular intervals
   - Play audio descriptions through the speaker

## Troubleshooting

### Common Issues

1. **No Audio Output**

   - Check speaker connections
   - Verify I2S wiring
   - Ensure 5V power supply is adequate

2. **WiFi Connection Issues**

   - Verify WiFi credentials
   - Check WiFi signal strength
   - Ensure proper power supply

3. **Camera Issues**
   - Check camera module connections
   - Verify power supply stability
   - Check for proper initialization in Serial Monitor

### Serial Monitor

The device outputs detailed logs to the Serial Monitor at 115200 baud. Monitor this for:

- WiFi connection status
- Camera initialization
- API communication
- Audio playback status
- Error messages

## Technical Details

### Camera Configuration

- Resolution: SVGA (800x600)
- JPEG Quality: 8 (high quality)
- Auto Gain: Enabled
- Auto Exposure: Enabled
- Brightness: +2

### Audio Configuration

- I2S Sample Rate: 44.1kHz
- Audio Format: MP3
- Amplifier Gain: 0.8

## API Integration

The system communicates with a cloud API that:

1. Receives the captured image
2. Generates a description
3. Converts the description to speech
4. Returns an MP3 audio file

## Contributing

Feel free to submit issues and enhancement requests!

## License

This project is licensed under the MIT License - see the LICENSE file for details.
