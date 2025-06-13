#define LILYGO_T_A7670

#include "utilities.h"

// Serial debug settings
#define TINY_GSM_DEBUG SerialMon
#define SerialMon Serial

#define DUMP_AT_COMMANDS
#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024

#include <TinyGsmClient.h>
// #include <WiFi.h> // Wi-Fi disabled for LTE testing

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

// ======== Wi-Fi (commented out) ========
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

    Serial.println("Start modem...");
    delay(3000);

    while (!modem.testAT()) {
        delay(10);
    }

    Serial.println("Wait SMS Done.");
    if (!modem.waitResponse(100000UL, "SMS DONE")) {
        Serial.println("Can't wait from sms register ....");
        return;
    }

    // Configure APN
#ifdef NETWORK_APN
    Serial.printf("Set network apn : %s\n", NETWORK_APN);
    modem.sendAT(GF("+CGDCONT=1,\"IP\",\""), NETWORK_APN, "\"");
    if (modem.waitResponse() != 1) {
        Serial.println("Set network apn error !");
    }
#endif

    int16_t sq;
    Serial.print("Wait for the modem to register with the network.");
    RegStatus status = REG_NO_RESULT;
    while (status == REG_NO_RESULT || status == REG_SEARCHING || status == REG_UNREGISTERED) {
        status = modem.getRegistrationStatus();
        switch (status) {
            case REG_UNREGISTERED:
            case REG_SEARCHING:
                sq = modem.getSignalQuality();
                Serial.printf("[%lu] Signal Quality:%d\n", millis() / 1000, sq);
                delay(1000);
                break;
            case REG_DENIED:
                Serial.println("Network registration was rejected, please check if the APN is correct");
                return;
            case REG_OK_HOME:
                Serial.println("Online registration successful");
                break;
            case REG_OK_ROAMING:
                Serial.println("Roaming registration successful");
                break;
            default:
                Serial.printf("Registration Status:%d\n", status);
                delay(1000);
                break;
        }
    }

    Serial.printf("Registration Status:%d\n", status);
    delay(1000);

    // Activate PDP context for LTE internet
    if (!modem.setNetworkActive()) {
        Serial.println("❌ PDP context activation failed");
        return;
    }

    Serial.print("🌐 LTE IP address: ");
    Serial.println(modem.getLocalIP());

    verifyInternetConnection(); // Perform HTTPS GET request
}

void loop() {
    if (SerialAT.available()) {
        Serial.write(SerialAT.read());
    }
    if (Serial.available()) {
        SerialAT.write(Serial.read());
    }
    delay(1);
}

#ifndef TINY_GSM_FORK_LIBRARY
#error "No correct definition detected. Please copy all [lib directories] from the LilyGO repo to the Arduino libraries directory."
#endif