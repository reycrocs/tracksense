#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// WiFi Credentials
const char* ssid = "Alba Extender 24";
const char* password = "ExtenderNova24";

// Weather API
String URL = "http://api.openweathermap.org/data/2.5/weather?";
String ApiKey = "63092ec95cae11dbcb350347be1a981d";
String lat = "10.2805";
String lon = "123.8563";

// DFPlayer setup
HardwareSerial mp3Serial(1);  // Use Serial1
DFRobotDFPlayerMini mp3;

void setup() {
  Serial.begin(115200);
  mp3Serial.begin(9600, SERIAL_8N1, 16, 17);  // RX=16, TX=17

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // DFPlayer setup
  if (!mp3.begin(mp3Serial)) {
    Serial.println("DFPlayer not found!");
    while (1);
  }

  mp3.volume(25); // volume level (0–30)
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String finalURL = URL + "lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + ApiKey;

    http.begin(finalURL);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String JSON_Data = http.getString();

      DynamicJsonDocument doc(2048);
      deserializeJson(doc, JSON_Data);
      JsonObject obj = doc.as<JsonObject>();

      const char* description = obj["weather"][0]["description"];
      float temp = obj["main"]["temp"];
      float humidity = obj["main"]["humidity"];

      Serial.println("------ Weather Info ------");
      Serial.print("Condition: ");
      Serial.println(description);
      Serial.print("Temperature: ");
      Serial.print(temp);
      Serial.println(" °C");
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");
      Serial.println("--------------------------");

      // Play 0001.mp3 from SD card
      delay(2000); // short delay to ensure file is written
      mp3.play(1); // plays 0001.mp3

    } else {
      Serial.print("HTTP request failed: ");
      Serial.println(httpCode);
    }

    http.end();
  } else {
    Serial.println("WiFi disconnected!");
  }

  delay(30000); // fetch every 30 seconds
}
