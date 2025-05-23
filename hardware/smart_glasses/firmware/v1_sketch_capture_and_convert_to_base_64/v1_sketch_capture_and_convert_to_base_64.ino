#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include "base64.h"  // Base64 library

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
const int Interval = 20000; // 20 seconds

bool LED_Flash_ON = true;

//=========================== Capture and Print Base64
void sendPhotoToSerialBase64() {
  Serial.println();
  Serial.println("-----------");
  Serial.println("Capturing image...");

  if (LED_Flash_ON) {
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(1000);
  }

  for (int i = 0; i <= 3; i++) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed during warm-up.");
      ESP.restart();
      return;
    }
    esp_camera_fb_return(fb);
    delay(200);
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed.");
    ESP.restart();
    return;
  }

  if (LED_Flash_ON) digitalWrite(FLASH_LED_PIN, LOW);

  Serial.println("Photo captured. Encoding to Base64...");

  // Encode image to base64
  String base64Image = base64::encode(fb->buf, fb->len);

  // Optional: split into smaller chunks for readability
  const int chunkSize = 64;
  for (int i = 0; i < base64Image.length(); i += chunkSize) {
    Serial.println(base64Image.substring(i, i + chunkSize));
  }

  Serial.println("-----------");
  esp_camera_fb_return(fb);
}
//===========================

//=========================== Capture and send to open api
void sendPhotoToOpenAI() {
  Serial.println("Capturing image...");

  if (LED_Flash_ON) {
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(1000);
  }

  for (int i = 0; i <= 3; i++) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera warm-up failed.");
      ESP.restart();
      return;
    }
    esp_camera_fb_return(fb);
    delay(200);
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed.");
    ESP.restart();
    return;
  }

  if (LED_Flash_ON) digitalWrite(FLASH_LED_PIN, LOW);

  String base64Image = base64::encode(fb->buf, fb->len);
  esp_camera_fb_return(fb);

  WiFiClientSecure client;
  client.setInsecure(); // WARNING: For testing only, skips certificate validation

  const char* host = "api.openai.com";
  const int httpsPort = 443;

  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection to OpenAI API failed");
    return;
  }

  String payload = 
    "{"
    "\"model\": \"gpt-4o\","
    "\"messages\": [{"
      "\"role\": \"user\","
      "\"content\": ["
        "{ \"type\": \"text\", \"text\": \"Describe this image in detail.\" },"
        "{ \"type\": \"image_url\", \"image_url\": {"
          "\"url\": \"data:image/jpeg;base64," + base64Image + "\""
        "} }"
      "]"
    "}],"
    "\"max_tokens\": 500"
    "}";

  String request = 
    String("POST /v1/chat/completions HTTP/1.1\r\n") +
    "Host: api.openai.com\r\n" +
    "Authorization: Bearer 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\r\n" +  // 🔐 <---- REPLACE THIS
    "Content-Type: application/json\r\n" +
    "Content-Length: " + String(payload.length()) + "\r\n\r\n" +
    payload;

  Serial.println("Sending request to OpenAI...");
  client.print(request);

  // Wait for response
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  Serial.println("Response:");
  while (client.available()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
  }

  client.stop();
}
//===========================

//=========================== Setup
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout detector
  Serial.begin(115200);
  Serial.println();

  pinMode(FLASH_LED_PIN, OUTPUT);

  // Initialize camera
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

  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;  // 800x600
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_VGA;   // 640x480
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_CIF); // Optional: reduce for faster upload

  Serial.println("Camera ready.");
  Serial.println("Capturing and printing Base64 every 20 seconds.");
}
//===========================

//=========================== Loop
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= Interval) {
    previousMillis = currentMillis;
    //sendPhotoToSerialBase64();
    sendPhotoToOpenAI();
  }
}
