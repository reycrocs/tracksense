#define LILYGO_T_A7670
#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024
#define DUMP_AT_COMMANDS

#include "utilities.h"
#include <TinyGsmClient.h>
#include <HTTPClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

// 🔧 Replace with your network's APN if different
#define NETWORK_APN "internet"

// ✅ URL to verify connection
const char *verify_url = "https://tracksense-flask-api.vercel.app/version"; // Replace with your actual test URL

void verifyInternetConnection() {
  const char *url = verify_url;

  Serial.print("🌐 Verifying connectivity to: ");
  Serial.println(url);

  modem.https_begin();  // 🔐 Required for HTTPS

  if (!modem.https_set_url(url)) {
    Serial.println("❌ Failed to set HTTPS URL");
    return;
  }

  int httpCode = modem.https_get();
  if (httpCode != 200) {
    Serial.print("❌ HTTPS GET failed, code: ");
    Serial.println(httpCode);
    return;
  }

  // ✅ Print HTTPS header and body
  String header = modem.https_header();
  String body = modem.https_body();

  Serial.println("🔽 Response Header:");
  Serial.println("────────────────────────────────────────────");
  Serial.println(header);
  Serial.println("────────────────────────────────────────────");

  Serial.println("📦 Response Body:");
  Serial.println("────────────────────────────────────────────");
  Serial.println(body);
  Serial.println("────────────────────────────────────────────");
}

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("🚀 Starting Setup...");

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);

#ifdef BOARD_POWERON_PIN
  pinMode(BOARD_POWERON_PIN, OUTPUT);
  digitalWrite(BOARD_POWERON_PIN, HIGH);
#endif

#ifdef MODEM_RESET_PIN
  pinMode(MODEM_RESET_PIN, OUTPUT);
  digitalWrite(MODEM_RESET_PIN, LOW); delay(100);
  digitalWrite(MODEM_RESET_PIN, HIGH); delay(2600);
  digitalWrite(MODEM_RESET_PIN, LOW);
#endif

  pinMode(BOARD_PWRKEY_PIN, OUTPUT);
  digitalWrite(BOARD_PWRKEY_PIN, LOW); delay(100);
  digitalWrite(BOARD_PWRKEY_PIN, HIGH); delay(100);
  digitalWrite(BOARD_PWRKEY_PIN, LOW);

  Serial.println("📞 Starting modem...");
  while (!modem.testAT(1000)) {
    Serial.println("Waiting for modem...");
    delay(1000);
  }

  while (modem.getSimStatus() != SIM_READY) {
    Serial.println("🔐 Waiting for SIM...");
    delay(1000);
  }
  Serial.println("📶 SIM ready");

  modem.setNetworkMode(MODEM_NETWORK_AUTO);

#ifdef NETWORK_APN
  modem.sendAT(GF("+CGDCONT=1,\"IP\",\""), NETWORK_APN, "\"");
  modem.waitResponse();
#endif

  RegStatus status = REG_NO_RESULT;
  while (status != REG_OK_HOME && status != REG_OK_ROAMING) {
    status = modem.getRegistrationStatus();
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\n📡 Registered on network");

  if (!modem.isNetworkConnected()) {
    Serial.println("❌ Network connection failed");
    return;
  }

  if (!modem.setNetworkActive()) {
    Serial.println("❌ PDP context activation failed");
    return;
  }

  Serial.print("🌐 Cellular IP: ");
  Serial.println(modem.getLocalIP());

  verifyInternetConnection();  // 🌍 Check internet access
}

void loop() {
  // Optional: Periodically check connectivity
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 300000) {  // Every 5 minutes
    verifyInternetConnection();
    lastCheck = millis();
  }

  // Forward AT commands for debugging
  if (SerialAT.available()) Serial.write(SerialAT.read());
  if (Serial.available()) SerialAT.write(Serial.read());
}
