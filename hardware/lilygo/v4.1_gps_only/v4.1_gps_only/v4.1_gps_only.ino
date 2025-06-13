#define LILYGO_T_A7670
#include "utilities.h"

#define TINY_GSM_RX_BUFFER 1024
#define SerialMon Serial
#define TINY_GSM_DEBUG SerialMon

#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

void setup()
{
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

    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
    delay(3000);

    while (!modem.testAT(1000)) {
        Serial.println("Retrying modem...");
        delay(1000);
    }

    String modemName = modem.getModemName();
    if (modemName.startsWith("A7670G")) {
        Serial.println("A7670G does not support built-in GPS. Use external GPS module.");
        while (true) delay(1000);
    }

    modem.sendAT("+SIMCOMATI");
    modem.waitResponse();

    Serial.println("Enabling GPS...");
    while (!modem.enableGPS(MODEM_GPS_ENABLE_GPIO, MODEM_GPS_ENABLE_LEVEL)) {
        Serial.print(".");
        delay(500);
    }

    modem.setGPSBaud(115200);
}

void loop()
{
    float lat, lon;
    int year, month, day, hour, min, sec;
    uint8_t fixMode;

    if (modem.getGPS(&fixMode, &lat, &lon, nullptr, nullptr, nullptr, nullptr, nullptr,
                     &year, &month, &day, &hour, &min, &sec)) {
        Serial.print("Latitude: ");
        Serial.println(lat, 6);
        Serial.print("Longitude: ");
        Serial.println(lon, 6);
        modem.disableGPS();
        while (true);  // Stop here
    } else {
        Serial.println("Waiting for GPS fix...");
        delay(10000);
    }
}
