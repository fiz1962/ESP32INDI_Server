#include <WiFi.h>
#include <math.h>
#include "INDI.h"
#include "core.h"
#include <vector>

void runCore1Logic(bool isTracking);

// -------------------- WiFi --------------------
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

INDI myINDI;
int lastSend;
int sendInterval = 500;
bool lastTracking;

void SetupINDI(WiFiClient client);

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting simulated INDI telescope...");

    Serial.println("Alt/Az C++ Structured Tracker Initialized.");
    Serial.println("Send 's' to Start tracking, or 'x' to Stop tracking.");

    pinMode(AZ_STEP_PIN, OUTPUT);
    pinMode(AZ_DIR_PIN, OUTPUT);
    pinMode(ALT_STEP_PIN, OUTPUT);
    pinMode(ALT_DIR_PIN, OUTPUT);

    startTime = millis();
    
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
  
      xTaskCreatePinnedToCore(
        stepperTask,   
        "StepperTask", 
        4096,          
        NULL,          
        3,             
        NULL,          
        0              
    );
    Serial.println("[SYSTEM] Stepper Task spawned on Core 0.");
    
  myINDI.start(7624);
  lastTracking = myINDI.isTracking;

}

// -------------------- Loop --------------------
void loop() {

   myINDI.loop();

   runCore1Logic(myINDI.isTracking);

    unsigned long now = millis();
    if (now - lastSend >= sendInterval) {
      lastSend = now;

      //Serial.printf("Tracking %d\r\n", myINDI.isTracking);

      //myINDI.sendRaw("<getProperties version=\"1.7\" device=\"ESP32-Telescope\" name=\"HORIZONTAL_COORD\"/>");

      Serial.printf("1 Alt/Az %lf, %lf,  RA/Dec %lf, %lf\r\n", currentAlt, currentAz, currentRA, currentDec);
      calcRaDec();
      const std::vector<const char*> elemNames = {"RA", "DEC"};
      const std::vector<double> values = {currentRA, currentDec};
      myINDI.sendNumberUpdate("EQUATORIAL_EOD_COORD", elemNames, values, "Ok");

      const std::vector<const char*> elemNamesHorz = {"ALT", "AZ"};
      const std::vector<double> valuesHorz = {currentAlt, currentAz};
      myINDI.sendNumberUpdate("HORIZONTAL_COORD", elemNamesHorz, valuesHorz, "Ok");
    }
}
