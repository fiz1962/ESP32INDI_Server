#include "INDI.h" // Include the corresponding header file
#include <iostream> // Example: for output
#include <vector>

#include "jsonDefs.h"

// -------------------- Device --------------------
String deviceName = "ESP32-Telescope";
String driverVersion = "esp32-indi-simulated-mpu6050-telescope";

// Default constructor implementation
INDI::INDI() : indiName(deviceName),  router(this) { // Member initializer list for m_value
    Serial.printf("INDI object created with value: %s\r\n", indiName.c_str());
    //router.registerRoute("newswitchvector", "CONNECTION", handleSwitchVectors);
    //router.registerRoute("newNumberVector", "GEOGRAPHIC_COORD", handleGeoCoords);
    //router.registerRoute("newNumberVector", "EQUATORIAL_EOD_COORD", handleEQCoords);
    isTracking = false;
    
}

// Parameterized constructor implementation
INDI::INDI(String name) : indiName(name),  router(this) {
    Serial.printf("INDI object created with value: %s\r\n", indiName.c_str());

}

// Destructor implementation
INDI::~INDI() {
    std::cout << "MyClass object destroyed." << std::endl;
    indiServer->stop();
    delete indiServer;
    isTracking = false;
}

void INDI::start(int port) {
    indiServer = new WiFiServer(port);
    indiServer->begin();
    indiServer->setNoDelay(true);
}

void INDI::sendNumberUpdate(const char* propName, 
                            const std::vector<const char*>& elemNames, 
                            const std::vector<double>& values, 
                            const char* state) {
  // Edge case protection: ensure arrays match in size
  if (elemNames.size() != values.size() || elemNames.empty()) return;

  String s;
  // Reserve memory upfront to prevent reallocations during string building
  s.reserve(256 + (elemNames.size() * 128)); 

  s  = "<setNumberVector device=\"" + indiName + "\" name=\"" + String(propName) + "\" state=\"" + String(state) + "\">\r\n";
  
  // Loop through all provided element/value pairs
  for (size_t i = 0; i < elemNames.size(); ++i) {
    s += "  <oneNumber name=\"" + String(elemNames[i]) + "\">" + String(values[i], 6) + "</oneNumber>\r\n";
  }
  
  s += "</setNumberVector>";
  sendRaw(s);
}

void INDI::sendTextUpdate(const char* propName, 
                          const std::vector<const char*>& elemNames, 
                          const std::vector<const char*>& values, 
                          const char* state) {
  // Edge case protection: ensure arrays match in size
  if (elemNames.size() != values.size() || elemNames.empty()) return;

  String s;
  // Pre-allocate memory upfront to keep heap execution stable and fast
  s.reserve(256 + (elemNames.size() * 128));

  s  = "<setTextVector device=\"" + indiName + "\" name=\"" + String(propName) + "\" state=\"" + String(state) + "\">\r\n";
  
  // Loop through and append all text elements
  for (size_t i = 0; i < elemNames.size(); ++i) {
    s += "  <oneText name=\"" + String(elemNames[i]) + "\">" + String(values[i]) + "</oneText>\r\n";
  }
  
  s += "</setTextVector>";
  sendRaw(s);
}

void INDI::sendSwitchUpdate(const char* propName, 
                            const std::vector<const char*>& elemNames, 
                            const std::vector<const char*>& values, 
                            const char* state) {
  // Edge case protection: ensure arrays match in size
  if (elemNames.size() != values.size() || elemNames.empty()) return;

  String s;
  // Pre-allocate memory upfront to keep heap execution lightning fast
  s.reserve(256 + (elemNames.size() * 128));

  s  = "<setSwitchVector device=\"ESP32-Telescope\" name=\"";
  s += propName;
  s += "\" state=\"";
  s += state;
  s += "\">\r\n";
  
  // Loop through and append all switch elements
  for (size_t i = 0; i < elemNames.size(); ++i) {
    s += "    <oneSwitch name=\"";
    s += elemNames[i];
    s += "\">";
    s += values[i];
    s += "</oneSwitch>\r\n";
  }
  
  s += "</setSwitchVector>";
  
  // printf("TX: %s\n", s.c_str());
  sendRaw(s);
}

void INDI::loop() {
    if (!client || !client.connected()) {
    WiFiClient newClient = indiServer->available();
    if (newClient) {
      Serial.println("Client connected. Sending device definitions...");
      client = newClient;
      //sendDeviceDefs();
      //sendInitialValues();
      //lastSend = 0;
    }
  }

  if (client && client.connected()) {
    while (client.available()) {
      String incoming = client.readStringUntil('>');

      incoming += '>';
      //Serial.print("RX: "); Serial.println(incoming);
      handleIncomingXML(incoming);
      router.processStream(std::string(incoming.c_str()));
    }
  }
}

void INDI::sendRaw(const String &s) {
  if (client && client.connected()) {
    client.print(s); client.print("\n");
    //Serial.print("TX: "); Serial.println(s);
  }
}

void INDI::parseDeviceJson(JsonDocument& doc) {
  JsonObject root = doc.as<JsonObject>();

  for (JsonPair groupPair : root) {
    const char* groupName = groupPair.key().c_str();
    JsonObject groupObj = groupPair.value().as<JsonObject>();

    Group group;
    group.name = groupName;

    // SWITCH VECTOR
    for (JsonObject svObj : groupObj["SwitchVector"].as<JsonArray>()) {
      SwitchVector sv;
      sv.name = svObj["name"].as<const char*>();
      sv.label = svObj["label"].as<const char*>();

      for (JsonObject s : svObj["Switches"].as<JsonArray>()) {
        SwitchEntry se;
        se.name  = s["name"].as<const char*>();
        se.label = s["label"].as<const char*>();
        se.value = s["value"].as<const char*>();
        sv.switches.push_back(se);
      }

      group.switchVectors.push_back(sv);
    }

    // NUMBER VECTOR
    for (JsonObject nvObj : groupObj["NumberVector"].as<JsonArray>()) {
      NumberVector nv;

      nv.name = nvObj["name"].as<const char*>();
      nv.label = nvObj["label"].as<const char*>();

      for (JsonObject n : nvObj["numbers"].as<JsonArray>()) {
        NumberEntry ne;
        ne.name   = n["name"].as<const char*>();
        ne.label  = n["label"].as<const char*>();
        ne.format = n["format"].as<const char*>();
        ne.value  = n["value"].as<float>();
        nv.numbers.push_back(ne);
      }

      group.numberVectors.push_back(nv);
    }

    // TEXT VECTOR
    for (JsonObject tvObj : groupObj["TextVector"].as<JsonArray>()) {
      // tvGroup keys are dynamic ("Driver Info")
      TextVector tv;
      tv.name = tvObj["name"].as<const char*>();
      tv.label = tvObj["label"].as<const char*>();
      for (JsonObject t : tvObj["texts"].as<JsonArray>()) {
            TextEntry te;
            te.name  = t["name"].as<const char*>();
            te.label = t["label"].as<const char*>();
            te.value = t["value"].as<const char*>();
            tv.texts.push_back(te);
      }

      group.textVectors.push_back(tv);
    }

    allGroups.push_back(group);
  }
}

void INDI::PrintIt() {
  for (auto &g : allGroups) {
    Serial.println();
    Serial.println("========== Group: " + g.name + " ==========");

    if (!g.switchVectors.empty()) {
      Serial.println("  SwitchVectors:");
      for (auto &sv : g.switchVectors) {
        sendRaw("<defSwitchVector device=\"ESP32-Telescope\" name=\"" + sv.name + "\" label=\"" + sv.label + "\" group=\"" + g.name + "\" state=\"Ok\" perm=\"rw\" rule=\"OneOfMany\">");
        for (auto &s : sv.switches) {
          sendRaw("<defSwitch name=\"" + s.name + "\" label=\"" + s.label + "\">" + s.value + "</defSwitch>");
        }
        sendRaw("</defSwitchVector>");
      }
    }

    if (!g.numberVectors.empty()) {
      Serial.println("  NumberVectors:");
      for (auto &nv : g.numberVectors) {
        sendRaw("<defNumberVector device=\"ESP32-Telescope\" name=\"" + nv.name + "\" label=\"" + nv.label + "\" group=\"" + g.name + "\" state=\"Ok\" perm=\"rw\">");
        for (auto &n : nv.numbers) {
          sendRaw("<defNumber name=\"" + n.name + "\" label=\"" + n.label + "\" format=\"" + n.format + "\">" + n.value + "</defNumber>");
        }
        sendRaw("</defNumberVector>");
      }
    }

    if (!g.textVectors.empty()) {
      Serial.println("  TextVectors:");
      for (auto &tv : g.textVectors) {
        sendRaw("<defTextVector device=\"ESP32-Telescope\" name=\"" + tv.name + "\" label=\"" + tv.label + "\" group=\"" + g.name + "\" state=\"Ok\" perm=\"rw\">");
        for (auto &t : tv.texts) {
          sendRaw("<defText name=\"" + t.name + "\" label=\"" + t.label + "\">" + t.value + "</defText>");
        }
        sendRaw("</defTextVector>");
      }
    }
  }
}

void INDI::SetupINDI() {
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, indiJSON);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  parseDeviceJson(doc);
  PrintIt();

}

void INDI::handleIncomingXML(const String &xml) {
  //Serial.printf("Feeding [%s]\r\n", xml.c_str());
  
   myXML.feed(xml);

  if (xml.indexOf("<getProperties") >= 0) {
     SetupINDI();
       
     const std::vector<const char*> elemNames = {"ALT", "AZ"};
     const std::vector<double> values = {42.125432, 180.052119};
     sendNumberUpdate("HORIZONTAL_COORD", elemNames, values, "Ok");
  }
}