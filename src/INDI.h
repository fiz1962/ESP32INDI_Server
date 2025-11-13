#ifndef INDI_H
#define INDI_H

#include <vector>
#include "INDIDefs.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include "StreamXML.h"

class INDI {
public:
    // Constructor(s)
    INDI(); // Default constructor
    INDI(String Name); // Parameterized constructor

    // Destructor
    ~INDI();

    // Public member functions (methods)
    void SetupINDI();
    void PrintIt();
    void parseDeviceJson(JsonDocument& doc);
    void sendRaw(const String &s);
    void loop();
    void handleIncomingXML(const String &xml);
    void start(int port);
    void sendNumberUpdate(const char* propName, const char* elemName, double value, const char* state);
    void sendSwitchUpdate(const char* propName, const char* elemName, const char* value, const char* state);
    void onTagStart(const String& tag);
    void onText(const String& txt);
    void onAttribute(const String& tag, const String& name, const String& value);
    void onTagEnd(const String& tag);

private:
    // Private member variables (data)
    String indiName; // Conventionally prefixed with 'm_' for member variables
    std::vector<Group> allGroups;
    WiFiClient client;
    WiFiServer* indiServer;
    StreamXML myXML;
    String currentTag;
};

#endif // MYCLASS_H
