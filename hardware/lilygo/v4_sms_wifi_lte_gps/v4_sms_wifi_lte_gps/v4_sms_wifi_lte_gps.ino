#define LILYGO_T_A7670

#include "utilities.h"

// ======== Debug & Serial Settings ========
#define SerialMon Serial
#define TINY_GSM_DEBUG SerialMon
#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024
#define DUMP_AT_COMMANDS

#include <TinyGsmClient.h>

// Comment out or change as needed
#define SMS_TARGET  "+639674232518"

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

// ======== LTE SETTINGS ========
#define NETWORK_APN "internet"
const char *verify_url = "https://tracksense-flask-api.vercel.app/version";

// ======== Wi-Fi SETTINGS (COMMENTED OUT) ========
// #include <WiFi.h>
// const char* ssid = "Alba Extender 24";
// const char* password = "ExtenderNova24";

// void connectToWiFi() {
//     WiFi.mode(WIFI_STA);
//     WiFi.begin(ssid, password);
//     Serial.print("Connecting to Wi-Fi");
//     int retry = 0;
//     while (WiFi.status() != WL_CONNECTED && retry < 20) {
//         delay(500);
//         Serial.print(".");
//         retry++;
//     }
//     if (WiFi.status() == WL_CONNECTED) {
//         Serial.println("\nWi-Fi connected!");
//         Serial.print("IP address: ");
//         Serial.println(WiFi.localIP());
//     } else {
//         Serial.println("\nFailed to connect to Wi-Fi.");
//     }
// }

void verifyInternetConnection() {
    Serial.print("🌐 Verifying connectivity to: ");
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

void enableGPS_SIM7600() {
    // Turn on GNSS power
    Serial.println("🛰️ Enabling GNSS...");
    modem.sendAT("+CGNSSPWR=1");
    if (modem.waitResponse(1000) != 1) {
        Serial.println("❌ Failed to enable GNSS. Check module version.");
        return;
    }
}

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
        Serial.println("❌ Failed to read GNSS info.");
        return;
    }

    Serial.println("📍 GNSS Raw: " + gnssLine);

    // Clean the prefix
    gnssLine.replace("+CGNSSINFO: ", "");

    String fields[20];
    int idx = 0;
    int startIdx = 0;

    while (idx < 20) {
        int comma = gnssLine.indexOf(',', startIdx);
        if (comma == -1) {
            fields[idx++] = gnssLine.substring(startIdx);
            break;
        }
        fields[idx++] = gnssLine.substring(startIdx, comma);
        startIdx = comma + 1;
    }

    // ✅ Use correct indices
    String lat = fields[5];
    String latDir = fields[6];
    String lon = fields[7];
    String lonDir = fields[8];

    if (lat.length() > 0 && lon.length() > 0) {
        String fullLat = (latDir == "S" ? "-" : "") + lat;
        String fullLon = (lonDir == "W" ? "-" : "") + lon;
        Serial.println("📌 Latitude: " + fullLat);
        Serial.println("📌 Longitude: " + fullLon);
        Serial.println("✅ Valid GPS fix");
    } else {
        Serial.println("❌ No valid GPS fix yet");
    }
}

void setup() {
    Serial.begin(115200);

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
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    delay(100);
    digitalWrite(BOARD_PWRKEY_PIN, HIGH);
    delay(1000);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);

    pinMode(MODEM_RING_PIN, INPUT_PULLUP);

    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
    delay(3000);

//     Serial.println("Start modem...");
//     while (!modem.testAT()) {
//         delay(10);
//     }

//     Serial.println("Wait SMS Done.");
//     if (!modem.waitResponse(100000UL, "SMS DONE")) {
//         Serial.println("Can't wait from sms register ....");
//         return;
//     }

// #ifdef NETWORK_APN
//     Serial.printf("Set network apn : %s\n", NETWORK_APN);
//     modem.sendAT(GF("+CGDCONT=1,\"IP\",\""), NETWORK_APN, "\"");
//     if (modem.waitResponse() != 1) {
//         Serial.println("Set network apn error !");
//     }
// #endif

//     int16_t sq;
//     Serial.print("Wait for the modem to register with the network.");
//     RegStatus status = REG_NO_RESULT;
//     while (status == REG_NO_RESULT || status == REG_SEARCHING || status == REG_UNREGISTERED) {
//         status = modem.getRegistrationStatus();
//         switch (status) {
//             case REG_UNREGISTERED:
//             case REG_SEARCHING:
//                 sq = modem.getSignalQuality();
//                 Serial.printf("[%lu] Signal Quality:%d\n", millis() / 1000, sq);
//                 delay(1000);
//                 break;
//             case REG_DENIED:
//                 Serial.println("❌ Network registration was rejected. Check APN.");
//                 return;
//             case REG_OK_HOME:
//                 Serial.println("✅ Home network registration successful.");
//                 break;
//             case REG_OK_ROAMING:
//                 Serial.println("✅ Roaming network registration successful.");
//                 break;
//             default:
//                 Serial.printf("Registration Status:%d\n", status);
//                 delay(1000);
//                 break;
//         }
//     }

//     Serial.printf("Final Registration Status:%d\n", status);
//     delay(1000);

//     // ======== SMS sending disabled for now ========
    
//     Serial.print("Init success, start to send message to ");
//     Serial.println(SMS_TARGET);

//     String imei = modem.getIMEI();
//     bool res = modem.sendSMS(SMS_TARGET, String("Hello from ") + imei);
//     Serial.print("Send sms message ");
//     Serial.println(res ? "OK" : "fail");
    

//     if (!modem.setNetworkActive()) {
//         Serial.println("❌ PDP context activation failed");
//         return;
//     }

//     Serial.print("🌐 LTE IP address: ");
//     Serial.println(modem.getLocalIP());

    // -------METHOD CALLS--------
    verifyInternetConnection();  // HTTPS test

    enableGPS_SIM7600();     // Enable GNSS system
    delay(2000);             // Give it time to lock
}

void loop() {
    // Serial bridge (optional for debugging modem)
    if (SerialAT.available()) {
        Serial.write(SerialAT.read());
    }
    if (Serial.available()) {
        SerialAT.write(Serial.read());
    }

    readGPS_SIM7600();       // Read GNSS info
    delay(10000);            // Every 10 seconds
}
