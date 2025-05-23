import serial
import os
from datetime import datetime
from gtts import gTTS
import shutil

# Serial port for ESP32
ser = serial.Serial('COM3', 115200, timeout=10)

# Path to SD card (adjust this to match your system)
sd_card_path = "E:e/mp3"  # Ensure this folder exists on the SD card

# Optional: also save log to Desktop
desktop_path = os.path.join(os.environ['USERPROFILE'], 'Desktop')
txt_file = os.path.join(desktop_path, "weather_log.txt")

print("Listening to ESP32...")

while True:
    line = ser.readline().decode('utf-8').strip()
    if "------ Weather Info ------" in line:
        weather_lines = [line]
        while True:
            data = ser.readline().decode('utf-8').strip()
            weather_lines.append(data)
            if "--------------------------" in data:
                break

        full_text = "\n".join(weather_lines)
        print("Got weather report:\n", full_text)

        # Save to log file
        timestamp = datetime.now().strftime("%Y-%m-%d %H-%M-%S")
        with open(txt_file, "a") as file:
            file.write(f"\n[{timestamp}]\n{full_text}\n")

        # Prepare speech
        tts_text = ". ".join([line for line in weather_lines if "------" not in line])
        tts = gTTS(text=tts_text, lang='en')

        # Save MP3 to SD card with DFPlayer-compatible name
        mp3_path = os.path.join(sd_card_path, "0001.mp3")
        tts.save(mp3_path)

        print("Audio saved to SD card as 0001.mp3 ✅")
        break
