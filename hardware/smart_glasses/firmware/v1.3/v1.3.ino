#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include "base64.h"
#include "SPIFFS.h"
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

//=========================== CAMERA_MODEL_AI_THINKER GPIO
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FLASH_LED_PIN     4
#define BUTTON1_PIN       14
#define BUTTON2_PIN       2
 
//=========================== WiFi & Interval
const char* ssid = "Alba Extender 24";
const char* password = "***********";

unsigned long previousMillis = 0;
const int Interval = 20000; // 20 seconds
bool LED_Flash_ON = true;

//=========================== URL Encode (for base64)
String urlencode(const String& str) {
  String encoded = "";
  char c, code0, code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      code0 = (c >> 4) & 0xF;
      code1 = c & 0xF;
      encoded += '%';
      encoded += char(code0 > 9 ? code0 - 10 + 'A' : code0 + '0');
      encoded += char(code1 > 9 ? code1 - 10 + 'A' : code1 + '0');
    }
  }
  return encoded;
}

//=========================== Test HTTPS connection
void testConnection() {
  WiFiClientSecure testClient;
  testClient.setInsecure();
  Serial.print("Testing connection to tracksense api... ");
  if (testClient.connect("tracksense-flask-api.vercel.app", 443)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
  }
}

//=========================== Send Photo to API
void sendPhotoToMyAPI() {
  Serial.println("Capturing image...");

  if (LED_Flash_ON) {
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(800);
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed.");
    return;
  }

  if (LED_Flash_ON) digitalWrite(FLASH_LED_PIN, LOW);
  Serial.printf("Captured %d bytes\n", fb->len);

  // Base64 encode the image
  String imageBase64 = base64::encode(fb->buf, fb->len);
  esp_camera_fb_return(fb);  // Free the buffer right after

  // Prepare JSON payload
  String jsonPayload = "{\"image\":\"" + imageBase64 + "\"}";
  Serial.println("Payload size: " + String(jsonPayload.length()));

  WiFiClientSecure client;
  client.setInsecure();  // Accept all certificates

  HTTPClient http;
  String url = "https://tracksense-flask-api.vercel.app/process-image";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(30000);  // Increased timeout to 30 seconds

  Serial.println("Sending POST request...");
  int httpCode = http.POST(jsonPayload);
  Serial.printf("HTTP Response code: %d\n", httpCode);

  if (httpCode == 200) {
    Serial.println("200 OK");
    // Get the response as a stream
    int len = http.getSize();
    Serial.printf("Response size: %d bytes\n", len);
    
    if (len > 0) {
      // Create a temporary file to store the MP3
      File f = SPIFFS.open("/temp.mp3", "w");
      if (f) {
        // Get the response stream
        WiFiClient *stream = http.getStreamPtr();
        // Read the response in chunks
        uint8_t buff[128] = {0};
        int totalBytes = 0;
        while (http.connected() && (len > 0 || len == -1)) {
          size_t size = stream->available();
          if (size) {
            int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
            f.write(buff, c);
            totalBytes += c;
            if (len > 0) {
              len -= c;
            }
            //Serial.printf("Downloaded: %d bytes\n", totalBytes);
          }
          delay(1);
        }
        f.close();
        Serial.printf("Total downloaded: %d bytes\n", totalBytes);
        
        // Play the saved MP3 file
        playAudioFromFile("/temp.mp3");
      } else {
        Serial.println("Failed to open file for writing");
      }
    } else {
      Serial.println("No response data");
    }
  } else {
    Serial.printf("HTTP POST failed: %s\n", http.errorToString(httpCode).c_str());
    String response = http.getString();
    Serial.println("Response: " + response);
  }

  http.end();
}

//=========================== Play audio from file
void playAudioFromFile(const char* filename) {
  AudioFileSourceSPIFFS *file = new AudioFileSourceSPIFFS(filename);
  AudioGeneratorMP3 *mp3 = new AudioGeneratorMP3();
  AudioOutputI2S *out = new AudioOutputI2S(1);  // Use I2S1

  out->SetPinout(13, 15, 12); // BCLK=13, LRC=15, DIN=12
  out->SetGain(1.0);

  if (mp3->begin(file, out)) {
    Serial.println("Playing audio...");
    while (mp3->isRunning()) {
      if (!mp3->loop()) break;
      Serial.print("-._."); // playback heartbeat
      delay(1);
    }
    Serial.println("Playback complete");
  } else {
    Serial.println("Failed to start audio playback");
  }

  mp3->stop();
  delete mp3;
  delete file;
  delete out;
}

//=========================== Setup
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout
  Serial.begin(115200);
  delay(1000);

  // Initialize buttons
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  Serial.println("Initializing camera...");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_SVGA;  // 800x600
  config.jpeg_quality = 8;             // Better image
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    delay(1000);
    ESP.restart();
  }

  // Sensor tuning
  sensor_t *s = esp_camera_sensor_get();
  s->set_gain_ctrl(s, 1);        // Enable Auto Gain
  s->set_exposure_ctrl(s, 1);    // Enable Auto Exposure
  s->set_brightness(s, 2);       // Brighten the image (-2 to 2)
  s->set_contrast(s, 0);         // Neutral contrast

  Serial.println("Camera ready. Testing connection...");
  testConnection();
  delay(2000);
  //sendPhotoToMyAPI();
}

//=========================== Loop
void loop() {
  // Check button states
  if (digitalRead(BUTTON1_PIN) == LOW) {
    Serial.println("Button 1 pressed");
    sendPhotoToMyAPI();
    delay(2000); // Simple debounce
  }
  
  if (digitalRead(BUTTON2_PIN) == LOW) {
    Serial.println("Button 2 pressed");
    Serial.println("Fetch weather API");
    delay(200); // Simple debounce
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= Interval) {
    previousMillis = currentMillis;
    Serial.println("Program still running...");
    //sendPhotoToMyAPI();  // Capture and process image every interval
  }
}