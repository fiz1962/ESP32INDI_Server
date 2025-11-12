#include <WiFi.h>
#include <math.h>
#include "INDI.h"
#include "StreamXML.h"

// -------------------- WiFi --------------------
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

WiFiClient client;
WiFiServer indiServer(7624);

INDI myINDI("ESP32-Telescope");

StreamXML xml;

String currentTag;

// -------------------- Device --------------------
String deviceName = "ESP32_TELESCOPE";
String driverVersion = "esp32-indi-simulated-mpu6050-telescope";

double temperatureValue = 20.0;
bool focusIn = false;
bool focusOut = false;

// -------------------- Simulated MPU6050 --------------------
double pitchFiltered = 0.0;
double rollFiltered = 0.0;
bool firstRead = true;
const float alpha = 0.1;
unsigned long simStart = 0;

// -------------------- Timing --------------------
unsigned long lastSend = 0;
const int sendInterval = 250; // ms

// -------------------- Function Declarations --------------------
void sendRaw(const String &s);
void sendNumberUpdate(const char* propName, const char* elemName, double value, const char* state="Ok");
void sendTelescopeUpdate(double raVal, double decVal);
void sendSwitchUpdate();
void sendDeviceDefs();
void sendInitialValues();
void updateSensors();
void handleIncomingXML(const String &xml);
String xmlQuoteEscape(const String &s);
double mapDouble(double x, double in_min, double in_max, double out_min, double out_max);
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
  } else {
    Serial.println("\nWiFi failed, starting AP mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP_INDI_AP");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  }


  //indiServer.begin();
  //indiServer.setNoDelay(true);
  simStart = millis();
  
  myINDI.start(7624);

  xml.onTagStart = [&](const String& tag) {
    currentTag = tag;
    Serial.printf("Tag start: %s>\r\n", tag.c_str());
  };

  xml.onText = [&](const String& txt) {
    // Convert text to float when possible
    float f = txt.toFloat();
    Serial.printf("[Text] %s = %f>\r\n", currentTag.c_str(), f);
  };

  xml.onAttribute = [&](const String& tag, const String& name, const String& value) {
    float f = value.toFloat();
    Serial.printf("[Attr] %s.%s = %f>\r\n", tag.c_str(), name.c_str(), f);
  };

  xml.onTagEnd = [&](const String& tag) {
    Serial.printf("Tag end: %s>\r\n", tag.c_str());
  };

}

// -------------------- Loop --------------------
void loop() {
  /*if (!client || !client.connected()) {
    WiFiClient newClient = indiServer.available();
    if (newClient) {
      Serial.println("Client connected. Sending device definitions...");
      client = newClient;
      //sendDeviceDefs();
      //sendInitialValues();
      lastSend = 0;
    }
  }*/

  /*if (client && client.connected()) {
    while (client.available()) {
      String incoming = client.readStringUntil('>');
      incoming += '>';
      Serial.print("RX: "); Serial.println(incoming);
      //xml.feed(incoming);
      handleIncomingXML(incoming);
    }*/
   myINDI.loop();

    unsigned long now = millis();
    if (now - lastSend >= sendInterval) {
      lastSend = now;
      //updateSensors();
      myINDI.sendNumberUpdate("EQUATORIAL_EOD_COORD", "RA", 1.5, "Ok");
      myINDI.sendNumberUpdate("EQUATORIAL_EOD_COORD", "DEC", 89, "Ok");
    }
  //}
}
// -------------------- Helpers --------------------
String xmlQuoteEscape(const String &s) {
  String out = s;
  out.replace("&","&amp;");
  out.replace("\"","&quot;");
  out.replace("'","&apos;");
  out.replace("<","&lt;");
  out.replace(">","&gt;");
  return out;
}


void sendNumberUpdate(const char* propName, const char* elemName, double value, const char* state) {
  String s;
  s  = "<setNumberVector device=\"" + deviceName + "\" name=\"" + String(propName) + "\" state=\"" + String(state) + "\">\r\n";
  s += "  <number name=\"" + String(elemName) + "\">" + String(value,6) + "</number>\r\n";
  s += "</setNumberVector>";
  sendRaw(s);
}

// Send telescope coordinates together
void sendTelescopeUpdate(double raVal, double decVal) {
  String s;
  raVal = 11.253;
  decVal = 90;
  s  = "<setNumberVector device=\"" + deviceName + "\" name=\"EQUATORIAL_EOD_COORD\" state=\"Ok\">\r\n";
  s += "  <number name=\"RA\">" + String(raVal,6) + "</number>\r\n";
  s += "  <number name=\"DEC\">" + String(decVal,6) + "</number>\r\n";
  s += "  <number name=\"TRACK_RATE\">0.0</number>\r\n";
  s += "</setNumberVector>";
  sendRaw(s);
}

void sendSwitchUpdate() {
  String s;
  s  = "<setSwitchVector device=\"" + deviceName + "\" name=\"CONNECTION\" state=\"Ok\">\r\n";
  s += "    <oneSwitch name=\"DISCONNECT\">Off</oneSwitch>\r\n";
  s += "    <oneSwitch name=\"CONNECT\">On</oneSwitch>\r\n";
  s += "</setSwitchVector>";
  sendRaw(s);
}

// -------------------- Send Device Definitions --------------------
void sendDeviceDefs() {
  String s;

  s  = "<defSwitchVector device=\"" + deviceName + "\" name=\"CONNECTION\" label=\"Connection\" group=\"Connection\" state=\"Idle\" perm=\"rw\" rule=\"OneOfMany\" timeout=\"60\">\r\n";
  s += "  <defSwitch name=\"CONNECT\" label=\"Connect\">\r\n";
  s += "On\n";
  s += "</defSwitch>\r\n";
  s += "  <defSwitch name=\"DISCONNECT\" label=\"Disconnect\">\r\n";
  s += "Off\n";
  s += " </defSwitch>\r\n";
  s += "</defSwitchVector>\r\n";
  sendRaw(s);

  sendRaw("<defTextVector device=\"" + deviceName + "\" name=\"DRIVER_INFO\" label=\"Driver Info\" group=\"Connection\" state=\"Idle\" perm=\"ro\">\r\n"
    "<defText name=\"DRIVER_NAME\" label=\"Name\">\r\n"
"Telescope Simulator"
    "</defText>\r\n"
    "<defText name=\"DRIVER_EXEC\" label=\"Exec\">\r\n"
    "indi_simulator_telescope"
    "</defText>\r\n"
    "<defText name=\"DRIVER_VERSION\" label=\"Version\">\r\n"
"1.0\n"
    "</defText>\r\n"
    "<defText name=\"DRIVER_INTERFACE\" label=\"Interface\">\r\n"
"5\n"
    "</defText>\r\n"
"</defTextVector>\r\n");

sendSwitchUpdate();

  sendRaw("<defSwitchVector device=\"" + deviceName + "\" name=\"ON_COORD_SET\" label=\"On Set\" group=\"Main Control\" state=\"Idle\" perm=\"rw\" rule=\"OneOfMany\" timeout=\"60\" timestamp=\"2025-11-09T13:45:31\">\r\n"
    "<defSwitch name=\"TRACK\" label=\"Track\">\r\n"
"On\n"
    "</defSwitch>\r\n"
    "<defSwitch name=\"SLEW\" label=\"Slew\">\r\n"
"Off\n"
    "</defSwitch>\r\n"
    "<defSwitch name=\"SYNC\" label=\"Sync\">\r\n"
"Off\n"
    "</defSwitch>\r\n"
"</defSwitchVector>\r\n");

  s  = "<defNumberVector device=\"" + deviceName + "\" name=\"EQUATORIAL_EOD_COORD\" label=\"Telescope Coordinates\" group=\"Telescope\" state=\"Idle\" perm=\"rw\">\r\n";
  s += "  <defNumber name=\"RA\" label=\"Right Ascension\" format=\"%.6f\" min=\"0\" max=\"24\"/>\r\n";
  s += "  <defNumber name=\"DEC\" label=\"Declination\" format=\"%.6f\" min=\"-90\" max=\"90\"/>\r\n";
  //s += "  <defNumber name=\"TRACK_RATE\" label=\"Tracking Rate\" format=\"%.6f\" min=\"-5\" max=\"5\"/>\r\n";
  s += "</defNumberVector>";
sendRaw(s);

    sendRaw("<defNumberVector device=\"" + deviceName + "\" name=\"TARGET_EOD_COORD\" label=\"Slew Target\" group=\"Motion Control\" state=\"Idle\" perm=\"ro\">\r\n"
    "<defNumber name=\"RA\" label=\"RA (hh:mm:ss)\" format=\"%010.6m\" min=\"0\" max=\"24\" step=\"0\">\r\n"
"0\n"
    "</defNumber>\r\n"
    "<defNumber name=\"DEC\" label=\"DEC (dd:mm:ss)\" format=\"%010.6m\" min=\"-90\" max=\"90\" step=\"0\">\r\n"
"0\n"
    "</defNumber>\r\n"
"</defNumberVector>\r\n");




  // Geo coords
  s = "<defNumberVector device=\"" + deviceName + "\" name=\"GEOGRAPHIC_COORD\" label=\"Scope Location\" group=\"Site Management\" state=\"Idle\" perm=\"rw\" timeout=\"60\" timestamp=\"2025-11-07T19:43:40\">";
  s +=  "<defNumber name=\"LAT\" label=\"Lat (dd:mm:ss.s)\" format=\"%012.8m\" min=\"-90\" max=\"90\" step=\"0\">";
  s += "34.73166666666666913";
  s += "    </defNumber>";
  s += "  <defNumber name=\"LONG\" label=\"Lon (dd:mm:ss.s)\" format=\"%012.8m\" min=\"0\" max=\"360\" step=\"0\">";
  s += "273.41333333333329847";
  s += "    </defNumber>";
  s += "    <defNumber name=\"ELEV\" label=\"Elevation (m)\" format=\"%g\" min=\"-200\" max=\"10000\" step=\"0\">";
  s += "193.52000000000001023";
  s += "    </defNumber>";
  s += "</defNumberVector>";
  sendRaw(s);



  // IMU_ORIENTATION
  /*s  = "<defNumberVector device=\"" + deviceName + "\" name=\"IMU_ORIENTATION\" label=\"IMU Orientation\" group=\"Sensors\" state=\"Idle\" perm=\"ro\">\r\n";
  s += "  <defNumber name=\"PITCH\" label=\"Pitch\" format=\"%.2f\" min=\"-180\" max=\"180\"/>\r\n";
  s += "  <defNumber name=\"ROLL\"  label=\"Roll\"  format=\"%.2f\" min=\"-180\" max=\"180\"/>\r\n";
  s += "</defNumberVector>";
  sendRaw(s);*/
/*
  // Telescope RA/DEC vector
  s  = "<defNumberVector device=\"" + deviceName + "\" name=\"EQUATORIAL_EOD_COORD\" label=\"Telescope Coordinates\" group=\"Telescope\" state=\"Idle\" perm=\"rw\">\r\n";
  s += "  <defNumber name=\"RA\" label=\"Right Ascension\" format=\"%.6f\" min=\"0\" max=\"24\"/>\r\n";
  s += "  <defNumber name=\"DEC\" label=\"Declination\" format=\"%.6f\" min=\"-90\" max=\"90\"/>\r\n";
  //s += "  <defNumber name=\"TRACK_RATE\" label=\"Tracking Rate\" format=\"%.6f\" min=\"-5\" max=\"5\"/>\r\n";
  s += "</defNumberVector>";
  sendRaw(s);*/
}

// -------------------- Send Initial Values --------------------
void sendInitialValues() {
  //sendNumberUpdate("TEMPERATURE","TEMP",temperatureValue,"Ok");
  //sendNumberUpdate("IMU_ORIENTATION","PITCH",pitchFiltered,"Ok");
  //sendNumberUpdate("IMU_ORIENTATION","ROLL",rollFiltered,"Ok");
  //sendSwitchUpdate();

  sendNumberUpdate("GEOGRAPHIC_COORD","LAT",34.7316666,"Ok");
  sendNumberUpdate("GEOGRAPHIC_COORD","LONG",273.41333333333335531,"Ok");
  sendNumberUpdate("GEOGRAPHIC_COORD","ELEV",193.5200,"Ok");
  //sendNumberUpdate("DEVICE_INTERFACE","DEVICE_INTERFACE",1,"Ok");

  double raVal = 1.53;//mapDouble(rollFiltered, -180, 180, 0, 24);
  double decVal = 90;//mapDouble(pitchFiltered, -90, 90, -90, 90);
  sendTelescopeUpdate(raVal, decVal);
}

// -------------------- Update Sensors (Simulated) --------------------
void updateSensors() {
  // Temperature
  temperatureValue = 20.0 + sin(millis()/5000.0)*2.0;
  //sendNumberUpdate("TEMPERATURE","TEMP",temperatureValue,"Ok");

  // Pitch/Roll
  unsigned long t = millis() - simStart;
  double pitchRaw = 30.0 * sin(t / 2000.0);
  double rollRaw  = 45.0 * cos(t / 3000.0);

  if (firstRead) {
    pitchFiltered = pitchRaw;
    rollFiltered  = rollRaw;
    firstRead = false;
  } else {
    pitchFiltered = alpha*pitchRaw + (1-alpha)*pitchFiltered;
    rollFiltered  = alpha*rollRaw  + (1-alpha)*rollFiltered;
  }

  //sendNumberUpdate("IMU_ORIENTATION","PITCH",pitchFiltered,"Ok");
  //sendNumberUpdate("IMU_ORIENTATION","ROLL",rollFiltered,"Ok");

  // Update telescope RA/DEC
  double raVal = 1.5;//mapDouble(rollFiltered, -180, 180, 0, 24);
  double decVal = 90;//mapDouble(pitchFiltered, -90, 90, -90, 90);
  sendTelescopeUpdate(raVal, decVal);
}

// -------------------- Map helper --------------------
double mapDouble(double x, double in_min, double in_max, double out_min, double out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void handleIncomingXML(const String &xml) {
  

  if (xml.indexOf("<getProperties") >= 0) {
    //SetupINDI(client);
    sendDeviceDefs();
    //sendInitialValues();
    //sendSwitchUpdate();
  }
  if( xml.indexOf("</newSwitchVector>") >= 0) {
    //sendSwitchUpdate();
  }
}

void sendRaw(const String &s) {
  if (client && client.connected()) {
    client.print(s); client.print("\n");
    Serial.print("TX: "); Serial.println(s);
  }
  else
  {
    Serial.print("DENIED:");Serial.println(s);
  }
}