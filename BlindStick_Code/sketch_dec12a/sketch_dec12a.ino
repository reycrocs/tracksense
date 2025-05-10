#include <WiFi.h>            // Include the Wi-Fi library for ESP32
#include <TinyGPS++.h>       // Include the TinyGPS++ library
#include <HTTPClient.h>      // Include HTTPClient library for making HTTP requests

TinyGPSPlus gps;              // Create a GPS object
HardwareSerial gpsSerial(1);  // Use UART1 for GPS communication

// GPIO Pin Definitions
const int buttonPin = 4;       // GPIO pin for the button
const int buzzerPin = 19;      // GPIO 19 for Passive Buzzer
const int trigPin = 5;         // GPIO 5 for Ultrasonic Trigger Pin
const int echoPin = 18;        // GPIO 18 for Ultrasonic Echo Pin
const int vibrationPin = 23;   // GPIO 23 for Vibration Module
const int led1 = 21;           // GPIO 21 for Wi-Fi Connected Indicator LED
const int led2 = 22;           // GPIO 22 for Wi-Fi Not Connected Indicator LED

// Ultrasonic Sensor Settings
const int distanceThreshold = 50;  // Trigger at 50 cm distance
const int buzzerFrequency = 4000;  // Resonant frequency for buzzer

// Wi-Fi Credentials
const char* ssid = "CCS-WIFI";       // Replace with your Wi-Fi SSID
const char* password = "Inn0v@teCC$143";     // Replace with your Wi-Fi password

// API Endpoint URLs
const char* alertURL = "https://elemsys-api.vercel.app/alert"; // Alert API URL
const char* periodicURL = "https://elemsys-api.vercel.app/save-coordinates"; // Periodic coordinates API URL

// Function Declarations
void handleUltrasonicSensor();
void handleGPS();
void sendDataToServer(const char* url, float latitude, float longitude);
void updateWiFiStatusLED();
void sendCoordinatesPeriodically();

// Timer for periodic coordinate sending
unsigned long lastSentTime = 0; // Store the last time data was sent
const unsigned long interval = 60000; // 1 minute interval (60,000 ms)

void setup() {
  Serial.begin(115200);       // Start the Serial Monitor
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);  // Start the GPS Serial (TX = 17, RX = 16)

  // Configure GPIO pins
  pinMode(buttonPin, INPUT_PULLUP);  // Configure the button pin as input with pull-up resistor
  pinMode(buzzerPin, OUTPUT);        // Set buzzer pin as output
  pinMode(trigPin, OUTPUT);          // Set trigger pin as output
  pinMode(echoPin, INPUT);           // Set echo pin as input
  pinMode(vibrationPin, OUTPUT);     // Set vibration module pin as output
  pinMode(led1, OUTPUT);             // Set Wi-Fi Connected LED pin as output
  pinMode(led2, OUTPUT);             // Set Wi-Fi Not Connected LED pin as output

  // Wi-Fi Setup
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);  // Start the Wi-Fi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    updateWiFiStatusLED(); // Update Wi-Fi status LEDs
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());  // Print the ESP32's IP address
}

void loop() {
  updateWiFiStatusLED();      // Update Wi-Fi status LEDs in the loop
  handleUltrasonicSensor();   // Check and handle ultrasonic sensor
  handleGPS();                // Check and handle GPS
  sendCoordinatesPeriodically(); // Periodically send coordinates
}

// Function to update Wi-Fi status LEDs in real-time
void updateWiFiStatusLED() {
  static bool wasConnected = false; // Track previous Wi-Fi connection state

  if (WiFi.status() == WL_CONNECTED) {
    if (!wasConnected) { // Check if connection state has changed to connected
      Serial.println("Wi-Fi connected!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());

      // Play a "do re mi fa so" tone sequence
      tone(buzzerPin, 524, 200); // Do (higher)
      delay(200);
      tone(buzzerPin, 588, 200); // Re (higher)
      delay(200);
      tone(buzzerPin, 660, 200); // Mi (higher)
      delay(200);
      tone(buzzerPin, 698, 200); // Fa (higher)
      delay(200);
      tone(buzzerPin, 784, 200); // So (higher)

      delay(200);
      noTone(buzzerPin);

      wasConnected = true;
    }
    digitalWrite(led1, HIGH); // Turn on Wi-Fi Connected LED
    digitalWrite(led2, LOW);  // Turn off Wi-Fi Not Connected LED
  } else {
    if (wasConnected) { // Check if connection state has changed to disconnected
      Serial.println("Wi-Fi disconnected!");
      wasConnected = false;
    }
    digitalWrite(led1, LOW);  // Turn off Wi-Fi Connected LED
    digitalWrite(led2, HIGH); // Turn on Wi-Fi Not Connected LED
  }
}

// Function to handle the ultrasonic sensor
void handleUltrasonicSensor() {
  long duration;
  int distance;

  // Send a 10us pulse to trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the echo pin and calculate distance
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;  // Convert the duration to distance in cm

  // Print distance for debugging
  if (distance > 0 && distance < distanceThreshold) {
    // Activate buzzer and vibration module
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    tone(buzzerPin, buzzerFrequency);  // Set buzzer to its resonant frequency
    digitalWrite(vibrationPin, HIGH);  // Turn on the vibration module
  } else {
    // Deactivate buzzer and vibration module
    noTone(buzzerPin);                // Turn off the buzzer
    digitalWrite(vibrationPin, LOW);  // Turn off the vibration module
  }

  delay(100);  // Small delay to avoid excessive processing
}

// Function to handle GPS and send data
void handleGPS() {
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    gps.encode(c);  // Feed the GPS data into the TinyGPS++ library
  }

  int buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) {  // Button is pressed
    if (gps.location.isValid()) {
      float latitude = gps.location.lat();
      float longitude = gps.location.lng();

      // Print GPS coordinates to Serial Monitor
      Serial.print("Latitude: ");
      Serial.println(latitude, 6);
      Serial.print("Longitude: ");
      Serial.println(longitude, 6);

      // Send GPS coordinates to the alert server
      sendDataToServer(alertURL, latitude, longitude);
    } else {
      Serial.println("Waiting for GPS signal...");
    }
    delay(1000);  // Debounce delay
  }
}

// Function to send data to the server
void sendDataToServer(const char* url, float latitude, float longitude) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Prepare the HTTP POST request
    http.begin(url);  // Specify server URL
    http.addHeader("Content-Type", "application/json"); // Set content type to JSON

    // Prepare the JSON payload with latitude and longitude
    String jsonPayload = "{\"latitude\": " + String(latitude, 6) + ", \"longitude\": " + String(longitude, 6) + "}";

    // Send the POST request
    int httpResponseCode = http.POST(jsonPayload);

    // Handle the HTTP response
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      Serial.println("Data sent successfully.");
    } else {
      Serial.print("Error in sending POST request. Response code: ");
      Serial.println(httpResponseCode);
    }

    // End the HTTP connection
    http.end();
  } else {
    Serial.println("Wi-Fi not connected!");
  }
}

// Function to send coordinates periodically
void sendCoordinatesPeriodically() {
  static bool gpsErrorDisplayed = false; // Flag to track if the error message has been displayed
  unsigned long currentTime = millis();
  
  if (currentTime - lastSentTime >= interval) {
    if (gps.location.isValid()) {
      float latitude = gps.location.lat();
      float longitude = gps.location.lng();

      // Print GPS coordinates to Serial Monitor
      Serial.println("Sending periodic coordinates:");
      Serial.print("Latitude: ");
      Serial.println(latitude, 6);
      Serial.print("Longitude: ");
      Serial.println(longitude, 6);

      // Send GPS coordinates to the periodic server
      sendDataToServer(periodicURL, latitude, longitude);

      // Update the last sent time
      lastSentTime = currentTime;

      // Reset the error flag as the signal is valid
      gpsErrorDisplayed = false;
    } else if (!gpsErrorDisplayed) {
      // Display the error message only once
      Serial.println("GPS signal is not valid for periodic send.");
      gpsErrorDisplayed = true;
    }
  }
}

