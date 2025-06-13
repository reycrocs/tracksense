#define LILYGO_T_A7670
#include "utilities.h"

// ========== DEBUG & MODEM SETUP ==========
#define SerialMon Serial
#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_DEBUG SerialMon
#define TINY_GSM_RX_BUFFER 1024
#define DUMP_AT_COMMANDS

#include <TinyGsmClient.h>
#include <WiFi.h>
#include <HTTPClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

// ========== SETTINGS ==========
#define NETWORK_APN "internet"

const char *verify_url = "https://tracksense-flask-api.vercel.app/version";

#define SMS_TARGET "+639674232518"

const char* ssid = "Alba Extender 24";
const char* password = "ExtenderNova24";

// ========== POWER/MODEM INIT ==========
void initializeBoard() {
#ifdef BOARD_POWERON_PIN
    pinMode(BOARD_POWERON_PIN, OUTPUT);
    digitalWrite(BOARD_POWERON_PIN, HIGH);
#endif

#ifdef MODEM_RESET_PIN
    pinMode(MODEM_RESET_PIN, OUTPUT);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL); delay(100);
    digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL); delay(2600);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
#endif

    pinMode(BOARD_PWRKEY_PIN, OUTPUT);
    digitalWrite(BOARD_PWRKEY_PIN, LOW); delay(100);
    digitalWrite(BOARD_PWRKEY_PIN, HIGH); delay(1000);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);

    pinMode(MODEM_RING_PIN, INPUT_PULLUP);
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
    delay(3000);
}

// ========== LTE INIT ==========
void initializeLTE() {
    Serial.println("📶 Initializing modem...");
    while (!modem.testAT()) {
        delay(10);
    }

    Serial.println("📲 Waiting for SMS init...");
    modem.waitResponse(100000UL, "SMS DONE");

    Serial.printf("📡 Setting APN: %s\n", NETWORK_APN);
    modem.sendAT(GF("+CGDCONT=1,\"IP\",\""), NETWORK_APN, "\"");
    modem.waitResponse();

    RegStatus status;
    do {
        status = modem.getRegistrationStatus();
        Serial.printf("📶 Registration status: %d\n", status);
        delay(1000);
    } while (status != REG_OK_HOME && status != REG_OK_ROAMING);

    Serial.println("✅ Modem registered on network");

    if (!modem.setNetworkActive()) {
        Serial.println("❌ LTE PDP activation failed");
        return;
    }

    Serial.print("🌐 LTE IP address: ");
    Serial.println(modem.getLocalIP());
}

// ========== SMS SEND ==========
void sendSMS() {
    String imei = modem.getIMEI();
    if (modem.sendSMS(SMS_TARGET, "Hello from LilyGO A7670E (" + imei + ")")) {
        Serial.println("📩 SMS sent successfully");
    } else {
        Serial.println("❌ SMS failed");
    }
}

// ========== LTE TEST ==========
void testLTEConnection() {
    Serial.print("🌐 Verifying LTE to: ");
    Serial.println(verify_url);

    modem.https_begin();
    if (!modem.https_set_url(verify_url)) {
        Serial.println("❌ Failed to set HTTPS URL");
        return;
    }

    int httpCode = modem.https_get();
    if (httpCode != 200) {
        Serial.print("❌ HTTPS GET failed, code: ");
        Serial.println(httpCode);
        return;
    }

    String header = modem.https_header();
    String body = modem.https_body();

    Serial.println("🔽 Response Header:");
    Serial.println(header);
    Serial.println("📦 Response Body:");
    Serial.println(body);
}

// ========== WIFI INIT ==========
void connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to Wi-Fi");

    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry++ < 20) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ Wi-Fi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n❌ Wi-Fi connection failed");
    }
}

// ========== WIFI TEST ==========
void testWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ Wi-Fi not connected. Skipping HTTPS test.");
        return;
    }

    Serial.print("🌐 Verifying Wi-Fi to: ");
    Serial.println(verify_url);

    HTTPClient http;
    http.begin(verify_url);
    int httpCode = http.GET();

    if (httpCode > 0) {
        Serial.printf("✅ Wi-Fi HTTPS GET success. Code: %d\n", httpCode);
        String payload = http.getString();
        Serial.println("📦 Body:");
        Serial.println(payload);
    } else {
        Serial.printf("❌ Wi-Fi HTTPS GET failed. Code: %d\n", httpCode);
    }

    http.end();
}

// ========== GNSS ENABLE ==========
void enableGPS_SIM7600() {
    Serial.println("🛰️ Enabling GNSS...");
    modem.sendAT("+CGNSSPWR=1");
    if (modem.waitResponse(1000) != 1) {
        Serial.println("❌ Failed to enable GNSS");
    }
}

// ========== GNSS READ ==========
void readGPS_SIM7600() {
    Serial.println("📡 Requesting GNSS info...");
    modem.sendAT("+CGNSSINFO");

    String gnssLine = "";
    unsigned long start = millis();
    while (millis() - start < 1000) {
        if (modem.stream.available()) {
            String line = modem.stream.readStringUntil('\n');
            line.trim();
            if (line.startsWith("+CGNSSINFO:")) {
                gnssLine = line;
                break;
            }
        }
    }

    if (gnssLine == "") {
        Serial.println("❌ GNSS info not available");
        return;
    }

    gnssLine.replace("+CGNSSINFO: ", "");
    String fields[20];
    int idx = 0, startIdx = 0;

    while (idx < 20) {
        int comma = gnssLine.indexOf(',', startIdx);
        if (comma == -1) {
            fields[idx++] = gnssLine.substring(startIdx);
            break;
        }
        fields[idx++] = gnssLine.substring(startIdx, comma);
        startIdx = comma + 1;
    }

    String lat = fields[5];
    String latDir = fields[6];
    String lon = fields[7];
    String lonDir = fields[8];

    if (lat.length() > 0 && lon.length() > 0) {
        String fullLat = (latDir == "S" ? "-" : "") + lat;
        String fullLon = (lonDir == "W" ? "-" : "") + lon;
        Serial.println("📌 Latitude: " + fullLat);
        Serial.println("📌 Longitude: " + fullLon);
        Serial.println("✅ GPS fix OK");
    } else {
        Serial.println("❌ No GPS fix");
    }
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    initializeBoard();

    // ======= OPTIONAL =======
    connectToWiFi();
    testWiFiConnection();

    //initializeLTE();
    //sendSMS();
    //testLTEConnection();

    enableGPS_SIM7600();
    delay(2000);
}

// ========== LOOP ==========
void loop() {
    readGPS_SIM7600();
    delay(10000);
}
