#include "VectorRouter.h"
#include <iostream>
#include "INDI.h"
#include "core.h"

VectorStreamRouter::VectorStreamRouter(INDI *device) : indiDevice(device) {
    registerRoute("newSwitchVector", "CONNECTION", [this](const std::string& name, const std::string& chunk) {
        handleConnect(name, chunk);
    });

    registerRoute("newSwitchVector", "ON_COORD_SET", [this](const std::string& name, const std::string& chunk) {
        handleOnCoord(name, chunk);
    });

    registerRoute("newNumberVector", "GEOGRAPHIC_COORD", [this](const std::string& name, const std::string& chunk) {
        handleGeoCoords(name, chunk);
    });
    
    registerRoute("newNumberVector", "EQUATORIAL_EOD_COORD", [this](const std::string& name, const std::string& chunk) {
        handleEQCoords(name, chunk);
    });
    
    registerRoute("newNumberVector", "HORIZONTAL_COORD", [this](const std::string& name, const std::string& chunk) {
        handleAltAzCoords(name, chunk);
    });
    
    registerRoute("newTextVector", "TIME_UTC", [this](const std::string& name, const std::string& chunk) {
        handleTimeUTC(name, chunk);
    });

}

void VectorStreamRouter::registerRoute(const std::string& tagName, const std::string& nameAttr, VectorCallback cb) {
    routes_[{tagName, nameAttr}] = cb;
}

void VectorStreamRouter::processStream(const std::string& xmlData) {
    size_t pos = 0;
    while (pos < xmlData.length()) {
        size_t startTag = xmlData.find('<', pos);
        if (startTag == std::string::npos) {
            if (capturing_) accumulator_.append(xmlData.substr(pos));
            break;
        }

        // Capture intermediate text leading up to a tag
        if (capturing_ && startTag > pos) {
            accumulator_.append(xmlData.substr(pos, startTag - pos));
        }

        size_t endTag = xmlData.find('>', startTag);
        if (endTag == std::string::npos) break;

        std::string tagContent = xmlData.substr(startTag + 1, endTag - startTag - 1);
        pos = endTag + 1;

        bool isClosing = (!tagContent.empty() && tagContent[0] == '/');
        
        if (isClosing) {
            std::string cleanTagName = tagContent.substr(1);
            if (capturing_) {
                accumulator_.append("</" + cleanTagName + ">");
            }
            // If the root vector tag closes, fire the active callback
            if (cleanTagName == active_tag_ && capturing_) {
                auto key = RouteKey(active_tag_, active_vector_name_);
                auto it = routes_.find(key);
                if (it != routes_.end()) {
                    it->second(active_vector_name_, accumulator_);
                } else {
                    // Default fallback triggers handleDefault when no route matches
                    handleDefault(active_vector_name_, accumulator_);
                }
                capturing_ = false;
                accumulator_.clear();
            }
        } 
        else {
            std::string tagName = extractTagName(tagContent);
            std::string nameAttr = extractAttribute(tagContent, "name");

            // INDI root vectors always contain "Vector" (e.g., newNumberVector, newSwitchVector)
            bool isRootVector = (tagName.find("Vector") != std::string::npos || tagName == "message");

            if (isRootVector) {
                // If a previous vector was left open, force-flush it out first
                if (capturing_) {
                    auto key = RouteKey(active_tag_, active_vector_name_);
                    auto it = routes_.find(key);
                    if (it != routes_.end()) {
                        it->second(active_vector_name_, accumulator_);
                    } else {
                        handleDefault(active_vector_name_, accumulator_);
                    }
                }
                
                // Start capturing the new root vector (registered or unregistered)
                active_tag_ = tagName;
                active_vector_name_ = nameAttr;
                capturing_ = true;
                accumulator_.clear();
                accumulator_.append("<" + tagContent + ">");
            } 
            else if (capturing_) {
                // Forward inner child tags (like oneNumber, oneSwitch) into the active buffer
                accumulator_.append("<" + tagContent + ">");
            }
        }
    }
}

std::string VectorStreamRouter::extractTagName(const std::string& tagContent) {
    size_t space = tagContent.find(' ');
    return (space == std::string::npos) ? tagContent : tagContent.substr(0, space);
}

std::string VectorStreamRouter::extractAttribute(const std::string& tagContent, const std::string& attrName) {
    std::string target = attrName + "=\"";
    size_t start = tagContent.find(target);
    if (start == std::string::npos) {
        target = attrName + "='";
        start = tagContent.find(target);
        if (start == std::string::npos) return "";
    }
    start += target.length();
    size_t end = tagContent.find(tagContent[start - 1], start);
    return (end == std::string::npos) ? "" : tagContent.substr(start, end - start);
}

std::string extractOneString(const std::string& xmlChunk, const std::string& targetName) {
    std::string pattern1 = "name='" + targetName + "'";
    std::string pattern2 = "name=\"" + targetName + "\"";
    
    size_t pos = xmlChunk.find(pattern1);
    if (pos == std::string::npos) {
        pos = xmlChunk.find(pattern2);
        if (pos == std::string::npos) return "";
    }

    size_t tagEnd = xmlChunk.find('>', pos);
    if (tagEnd == std::string::npos) return "";

    size_t closeTag = xmlChunk.find("</oneSwitch>", tagEnd);
    if (closeTag == std::string::npos) return "";

    std::string rawValue = xmlChunk.substr(tagEnd + 1, closeTag - (tagEnd + 1));

    size_t first = rawValue.find_first_not_of(" \t\n\r");
    size_t last = rawValue.find_last_not_of(" \t\n\r");
    if (first == std::string::npos || last == std::string::npos) return "";
    
    std::string cleanValue = rawValue.substr(first, (last - first + 1));

    return cleanValue;
}

double extractOneNumber(const std::string& xmlChunk, const std::string& targetName) {
    std::string pattern1 = "name='" + targetName + "'";
    std::string pattern2 = "name=\"" + targetName + "\"";
    
    size_t pos = xmlChunk.find(pattern1);
    if (pos == std::string::npos) {
        pos = xmlChunk.find(pattern2);
        if (pos == std::string::npos) return 0.0;
    }

    size_t tagEnd = xmlChunk.find('>', pos);
    if (tagEnd == std::string::npos) return 0.0;

    size_t closeTag = xmlChunk.find("</oneNumber>", tagEnd);
    if (closeTag == std::string::npos) return 0.0;

    std::string rawValue = xmlChunk.substr(tagEnd + 1, closeTag - (tagEnd + 1));

    size_t first = rawValue.find_first_not_of(" \t\n\r");
    size_t last = rawValue.find_last_not_of(" \t\n\r");
    if (first == std::string::npos || last == std::string::npos) return 0.0;
    
    std::string cleanValue = rawValue.substr(first, (last - first + 1));

    return std::stod(cleanValue);
}

// --- Class Method Routine Handlers ---

void VectorStreamRouter::handleDefault(const std::string& name, const std::string& xmlChunk) {
    std::cout << "[HANDLER: Default] Name: " << name << "\nData:\n" << xmlChunk << "\n\n";
}

void VectorStreamRouter::handleConnect(const std::string& name, const std::string& xmlChunk) {
    std::cout << "[HANDLER: Connect] Name: " << name << "\nData:\n" << xmlChunk << "\n\n";
    std::string CONNECT = extractOneString(xmlChunk, "CONNECT");
    if( CONNECT == "" )
        CONNECT = "Off";
    std::string DISCONNECT = extractOneString(xmlChunk, "DISCONNECT");
    if( DISCONNECT == "" )
        DISCONNECT = "Off";
    if (indiDevice) {
        const std::vector<const char*> elemNames = {"CONNECT", "DISCONNECT"};
        const std::vector<const char*> values = {CONNECT.c_str(), DISCONNECT.c_str()};
        indiDevice->sendSwitchUpdate("HORIZONTAL_COORD", elemNames, values, "Ok");
    }
}

void VectorStreamRouter::handleOnCoord(const std::string& name, const std::string& xmlChunk) {
    std::cout << "[HANDLER: On_Coord] Name: " << name << "\nData:\n" << xmlChunk << "\n\n";
    std::string modeSync = extractOneString(xmlChunk, "SYNC");
    std::string modeTrack = extractOneString(xmlChunk, "TRACK");
    std::string modeSlew = extractOneString(xmlChunk, "SLEW");
    std::cout << "Sync  [" << modeSync << "], Track [" << modeTrack << "], Slew [" << modeSlew << "]\n\n";
    indiDevice->isTracking = false;

    if( modeSync == "On" ) {
        const std::vector<const char*> elemNames = {"SYNC"};
        const std::vector<const char*> values = {"On"};
        indiDevice->sendSwitchUpdate("ON_COORD_SET", elemNames, values, "Ok");
        stopTracking();
    }

    if( modeTrack == "On" ) {
        const std::vector<const char*> elemNames = {"TRACK"};
        const std::vector<const char*> values = {"On"};
        indiDevice->sendSwitchUpdate("ON_COORD_SET", elemNames, values, "Ok");
        indiDevice->isTracking = true;
        startTracking();
    }

    if( modeSlew == "On" ) {
        const std::vector<const char*> elemNames = {"SLEW"};
        const std::vector<const char*> values = {"On"};
        indiDevice->sendSwitchUpdate("ON_COORD_SET", elemNames, values, "Ok");
        stopTracking();
    }
}

void VectorStreamRouter::handleGeoCoords(const std::string& name, const std::string& xmlChunk) {
    std::cout << "[HANDLER: GeoCoords] Name: " << name << "\nData:\n" << xmlChunk << "\n\n";
    LATITUDE = extractOneNumber(xmlChunk, "LAT");
    LONGITUDE = extractOneNumber(xmlChunk, "LONG");
    double elevation = extractOneNumber(xmlChunk, "ELEV");

    if (indiDevice) {
        const std::vector<const char*> elemNames = {"LAT", "LONG", "ELEV"};
        const std::vector<double> values = {LATITUDE, LONGITUDE, elevation};
        indiDevice->sendNumberUpdate("GEOGRAPHIC_COORD", elemNames, values, "Ok");
    }
}
void VectorStreamRouter::handleAltAzCoords(const std::string& name, const std::string& xmlChunk) {
    std::cout << "[HANDLER: HorzCoords] Name: " << name << "\nData:\n" << xmlChunk << "\n\n";
    double Alt = extractOneNumber(xmlChunk, "Alt");
    double Az = extractOneNumber(xmlChunk, "Az");

    Serial.printf("Alt/Az %lf, %lf\r\n", Alt, Az);
    if (indiDevice) {
        const std::vector<const char*> elemNames = {"ALT", "AZ"};
        const std::vector<double> values = {Alt, Az};
        indiDevice->sendNumberUpdate("HORIZONTAL_COORD", elemNames, values, "Ok"); 
    }
}

void VectorStreamRouter::handleEQCoords(const std::string& name, const std::string& xmlChunk) {
    std::cout << "[HANDLER: EQ Coords] Name: " << name << "\nData:\n" << xmlChunk << "\n\n";
    currentRA = extractOneNumber(xmlChunk, "RA");
    currentDec = extractOneNumber(xmlChunk, "DEC");
    double outAlt, outAz;
    
    calcAltAz(currentRA, currentDec, currentAlt, currentAz);
    Serial.printf("Setting RA/DEC %lf, %lf  Alt/Az %lf, %lf\r\n", currentRA, currentDec, currentAlt, currentAz);
    if (indiDevice) {
        const std::vector<const char*> elemNames = {"RA", "DEC"};
        const std::vector<double> values = {currentRA, currentDec};
        indiDevice->sendNumberUpdate("EQUATORIAL_EOD_COORD", elemNames, values, "Ok");
    }
}
void VectorStreamRouter::handleTimeUTC(const std::string& name, const std::string& xmlChunk) {
    std::cout << "[HANDLER: TimeUTC] Name: " << name << "\nData:\n" << xmlChunk << "\n\n";
    std::string UTC = extractOneString(xmlChunk, "UTC");
    std::string UTCOffset = extractOneString(xmlChunk, "OFFSET");
    DateTime dt;

    parseISO8601(UTC.c_str(), dt);
    initialLST = calculateLST(dt, LONGITUDE);
    startTime = millis();

    if (indiDevice) {
        const std::vector<const char*> elemNames = {"UTC", "OFFSET"};
        const std::vector<const char*> values = {UTC.c_str(), UTCOffset.c_str()};
        indiDevice->sendTextUpdate("TIME_UTC", elemNames, values, "Ok");
    }
}