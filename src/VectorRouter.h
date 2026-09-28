#ifndef VECTOR_ROUTER_H
#define VECTOR_ROUTER_H

#include <string>
#include <functional>
#include <map>
#include <utility>

// Define the signature for your vector routines: takes the vector name and the raw XML block
using VectorCallback = std::function<void(const std::string&, const std::string&)>;

class INDI;

class VectorStreamRouter {
private:
    // Route key: pair of (Tag Name, "name" attribute value)
    using RouteKey = std::pair<std::string, std::string>;
    std::map<RouteKey, VectorCallback> routes_;

    std::string active_tag_;
    std::string active_vector_name_;
    std::string accumulator_;
    bool capturing_ = false;
    INDI *indiDevice;

    std::string extractTagName(const std::string& tagContent);
    std::string extractAttribute(const std::string& tagContent, const std::string& attrName);

    // --- Routine Handlers as Class Methods ---
    void handleDefault(const std::string& name, const std::string& xmlChunk);
    void handleConnect(const std::string& name, const std::string& xmlChunk);
    void handleOnCoord(const std::string& name, const std::string& xmlChunk);
    void handleGeoCoords(const std::string& name, const std::string& xmlChunk);
    void handleEQCoords(const std::string& name, const std::string& xmlChunk);
    void handleAltAzCoords(const std::string& name, const std::string& xmlChunk);
    void handleTimeUTC(const std::string& name, const std::string& xmlChunk);

public:
    VectorStreamRouter(INDI *device);
    
    // Register a callback for a specific tag and name attribute[cite: 1]
    void registerRoute(const std::string& tagName, const std::string& nameAttr, VectorCallback cb);

    // Feed a chunk of incoming stream data character by character or line by line[cite: 1]
    void processStream(const std::string& xmlData);
};

double extractOneNumber(const std::string& xmlChunk, const std::string& targetName);
std::string extractOneString(const std::string& xmlChunk, const std::string& targetName);

#endif // VECTOR_ROUTER_H