#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include "base64.h"

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
//===========================

#define FLASH_LED_PIN 4
unsigned long previousMillis = 0;
const int Interval = 60000;
bool LED_Flash_ON = true;

// WiFi credentials
const char* ssid = "Alba Extender 24";
const char* password = "ExtenderNova24";

//=========================== Send Image to ImgHippo
// New function replacing the previous OpenAI one
void sendPhotoToImgHippo() {
  Serial.println("Capturing image...");

  if (LED_Flash_ON) {
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(800);
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed.");
    ESP.restart();
    return;
  }

  if (LED_Flash_ON) digitalWrite(FLASH_LED_PIN, LOW);
  Serial.printf("Captured %d bytes\n", fb->len);

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();  // Accept all SSL certificates

  http.begin(client, "https://api.imghippo.com/upload");  // Replace with correct ImgHippo endpoint
  http.addHeader("Content-Type", "multipart/form-data; boundary=imgboundary");
  http.addHeader("Authorization", "Bearer YOUR_API_KEY_HERE");  // Replace with actual API key if required

  // Build multipart/form-data payload
  String bodyStart = "--imgboundary\r\n"
                     "Content-Disposition: form-data; name=\"file\"; filename=\"photo.jpg\"\r\n"
                     "Content-Type: image/jpeg\r\n\r\n";
  String bodyEnd = "\r\n--imgboundary--\r\n";

  int totalLength = bodyStart.length() + fb->len + bodyEnd.length();
  http.addHeader("Content-Length", String(totalLength));

  int httpCode = http.sendRequest("POST", [&]() -> Stream* {
    WiFiClient* stream = &client;
    stream->print(bodyStart);
    stream->write((const uint8_t *)fb->buf, fb->len);
    stream->print(bodyEnd);
    return stream;
  });

  Serial.printf("HTTP Status Code: %d\n", httpCode);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("Upload Response:");
    Serial.println(response);
    // Parse response if necessary to extract image URL
  } else {
    Serial.printf("HTTP POST failed: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  esp_camera_fb_return(fb);
}

//=========================== Setup
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout
  Serial.begin(115200);
  delay(1000);

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

  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    delay(1000);
    ESP.restart();
  }

  Serial.println("Camera ready. Capturing every 20 seconds.");
}

//=========================== Loop
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= Interval) {
    previousMillis = currentMillis;
    sendPhotoToImgHippo();
  }
}
