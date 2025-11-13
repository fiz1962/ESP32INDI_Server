#include <WiFi.h>
#include <math.h>
#include "INDI.h"

// -------------------- WiFi --------------------
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

INDI myINDI;
int lastSend;
int sendInterval = 500;

void SetupINDI(WiFiClient client);

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("Starting simulated INDI telescope...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected. IP:");
    Serial.println(WiFi.localIP());
  }
  
  myINDI.start(7624);
}

// -------------------- Loop --------------------
void loop() {

   myINDI.loop();

    unsigned long now = millis();
    if (now - lastSend >= sendInterval) {
      lastSend = now;

      myINDI.sendNumberUpdate("EQUATORIAL_EOD_COORD", "RA", 1.5, "Ok");
      myINDI.sendNumberUpdate("EQUATORIAL_EOD_COORD", "DEC", 89, "Ok");
    }
}
