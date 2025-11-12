#include "INDI.h" // Include the corresponding header file
#include <iostream> // Example: for output


#include "jsonDefs.h"

// Default constructor implementation
INDI::INDI() : indiName("DefName") { // Member initializer list for m_value
    Serial.printf("INDI object created with value: %s\r\n", indiName.c_str());
}

// Parameterized constructor implementation
INDI::INDI(String name) : indiName(name) {
    Serial.printf("INDI object created with value: %s\r\n", indiName.c_str());

}

// Destructor implementation
INDI::~INDI() {
    std::cout << "MyClass object destroyed." << std::endl;
    indiServer->stop();
    delete indiServer;
}

void INDI::start(int port) {
    indiServer = new WiFiServer(port);
    indiServer->begin();
    indiServer->setNoDelay(true);
}

void INDI::sendNumberUpdate(const char* propName, const char* elemName, double value, const char* state) {
  String s;
  s  = "<setNumberVector device=\"" + indiName + "\" name=\"" + String(propName) + "\" state=\"" + String(state) + "\">\r\n";
  s += "  <oneNumber name=\"" + String(elemName) + "\" message=\"Setting number\">" + String(value,6) + "</oneNumber>\r\n";
  s += "</setNumberVector>";
  sendRaw(s);
}

void INDI::sendSwitchUpdate(const char* propName, const char* elemName, const char* value, const char* state) {
  String s;
  s  = "<setSwitchVector device=\"ESP32-Telescope\" name=\"CONNECTION\" state=\"Ok\">\r\n";
  s += "    <oneSwitch name=\"DISCONNECT\">Off</oneSwitch>\r\n";
  s += "    <oneSwitch name=\"CONNECT\">On</oneSwitch>\r\n";
  s += "</setSwitchVector>";
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
      Serial.print("RX: "); Serial.println(incoming);
      handleIncomingXML(incoming);
    }
  }
}

void INDI::sendRaw(const String &s) {
  if (client && client.connected()) {
    client.print(s); client.print("\n");
    Serial.print("TX: "); Serial.println(s);
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
      //String pretty;
//serializeJsonPretty(groupObj["NumberVector"], pretty);
//Serial.println(pretty);

      nv.name = nvObj["name"].as<const char*>();
      nv.label = nvObj["label"].as<const char*>();

      for (JsonObject n : nvObj["numbers"].as<JsonArray>()) {
        NumberEntry ne;
        ne.name  = n["name"].as<const char*>();
        ne.label = n["label"].as<const char*>();
        ne.value = n["value"].as<float>();
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
        sendRaw("<defSwitchVector device=\"ESP32-Telescope\" name=\"" + sv.name + "\" label=\"" + sv.label + "\" group=\"" + g.name + "\" state=\"Idle\" perm=\"rw\" rule=\"OneOfMany\">");
        for (auto &s : sv.switches) {
          sendRaw("<defSwitch name=\"" + s.name + "\" label=\"" + s.label + "\">" + s.value + "</defSwitch>");
        }
        sendRaw("</defSwitchVector>");
      }
    }

    if (!g.numberVectors.empty()) {
      Serial.println("  NumberVectors:");
      for (auto &nv : g.numberVectors) {
        sendRaw("<defNumberVector device=\"ESP32-Telescope\" name=\"" + nv.name + "\" label=\"" + nv.label + "\" group=\"" + g.name + "\" state=\"Idle\" perm=\"rw\">");
        for (auto &n : nv.numbers) {
          sendRaw("<defNumber name=\"" + n.name + "\" label=\"" + n.label + "\" format=\"%010.6m\">" + n.value + "</defNumber>");
        }
        sendRaw("</defNumberVector>");
      }
    }

    if (!g.textVectors.empty()) {
      Serial.println("  TextVectors:");
      for (auto &tv : g.textVectors) {
        sendRaw("<defTextVector device=\"ESP32-Telescope\" name=\"" + tv.name + "\" label=\"" + tv.label + "\" group=\"" + g.name + "\" state=\"Idle\" perm=\"ro\">");
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
  if (xml.indexOf("<getProperties") >= 0) {
    SetupINDI();
    //sendDeviceDefs();
    //sendInitialValues();
    sendSwitchUpdate("CONNECTION", "CONNECT", "On", "Ok");
  }
  if( xml.indexOf("</newSwitchVector>") >= 0) {
    //sendSwitchUpdate();
  }
}