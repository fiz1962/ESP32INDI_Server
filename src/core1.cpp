#include "core.h"

#include <Arduino.h>
#include <cmath>

bool parseISO8601(const char* isoStr, DateTime &dt) {
    int parsedItems;
    float secEx; // Temporary float to parse seconds in case there are decimals

    // sscanf matches the precise positions of '-', 'T', and ':'
    parsedItems = sscanf(isoStr, "%d-%d-%dT%d:%d:%f", 
                         &dt.year, 
                         &dt.month, 
                         &dt.day, 
                         &dt.hour, 
                         &dt.minute, 
                         &secEx);

    // We must successfully parse at least all 6 core components (Y, M, D, H, Min, S)
    if (parsedItems >= 6) {
        dt.second = (double)secEx;
        return true;
    }

    return false; // Return false if the string format was corrupted
}

// Computes Local Sidereal Time in decimal hours
double calculateLST(DateTime dt, double longitude) {
    // 1. Calculate Julian Date
    int y = dt.year;
    int m = dt.month;
    if (m <= 2) {
        y -= 1;
        m += 12;
    }
    
    int A = y / 100;
    int B = A / 4;
    int C = 2 - A + B;
    int E = 365.25 * (y + 4716);
    int F = 30.6001 * (m + 1);
    
    // Julian Day at 0h UTC
    double jd0 = C + dt.day + E + F - 1524.5;
    
    // Day fraction in hours
    double dayFraction = (dt.hour + (dt.minute / 60.0) + (dt.second / 3600.0)) / 24.0;
    double jd = jd0 + dayFraction;

    // 2. Compute GMST (Degrees)
    double T = (jd - 2451545.0) / 36525.0;
    double gmst = 280.46061837 + 360.98564736629 * (jd - 2451545.0) + 0.000387933 * T * T;
    
    // Keep within 0-360 degrees
    gmst = fmod(gmst, 360.0);
    if (gmst < 0) gmst += 360.0;

    // 3. Apply Local Longitude (East positive, West negative)
    double lst_degrees = gmst + longitude;
    
    lst_degrees = fmod(lst_degrees, 360.0);
    if (lst_degrees < 0) lst_degrees += 360.0;

    // 4. Return in hours (0.0 to 24.0)
    return lst_degrees / 15.0;
}


// If you want to keep setup/loop logic out of the .ino file, 
// you can handle Core 1 logic here if called from setup/loop.
void runCore1Logic(bool trackingActive) {
    if (Serial.available() > 0) {
        char cmd = Serial.read();
        if (cmd == 's' || cmd == 'S') {
            startTracking();
        } else if (cmd == 'x' || cmd == 'X') {
            stopTracking();
        }
    }

    static long lastPrintedAltSteps = 0;
    static long lastPrintedAzSteps = 0;

    portENTER_CRITICAL(&mutex);
    long currentAltSteps = totalAltSteps;
    long currentAzSteps = totalAzSteps;
    double lAlt = currentAlt;
    double lAz = currentAz;
    double lAltRate = targetAltRate;
    double lAzRate = targetAzRate;
    //bool trackingActive = isTracking;
    portEXIT_CRITICAL(&mutex);

    //Serial.printf("currentAlt %d, lastAlt %d, currentAz %d, last Az %d\r\n", currentAltSteps, lastPrintedAltSteps, currentAzSteps, lastPrintedAzSteps);
    if (trackingActive) {
        if (currentAltSteps != lastPrintedAltSteps || currentAzSteps != lastPrintedAzSteps) {
            Serial.printf("[ALT STEP] Total: %ld | Current Alt: %.4f° | Rate: %.5f deg/s\r\n", 
                          currentAltSteps, lAlt, lAltRate);
            lastPrintedAltSteps = currentAltSteps;

            Serial.printf("[AZ STEP] Total: %ld | Current Az: %.4f° | Rate: %.5f deg/s\r\n", 
                          currentAzSteps, lAz, lAzRate);
            lastPrintedAzSteps = currentAzSteps;
        }
    }

    static unsigned long lastMathTime = 0;
    unsigned long now = millis();

    if (trackingActive && (now - lastMathTime >= 100)) {
        lastMathTime = now;

        double elapsedSeconds = (now - startTime);
        double currentLST = fmod(initialLST + (OMEGA * elapsedSeconds), 360.0);

        double latRad = radians(LATITUDE);
        double altRad = radians(lAlt);
        double azRad = radians(lAz);

        double dAlt = (OMEGA*1000) * cos(latRad) * sin(azRad);
        double dAz = (OMEGA*1000) * (sin(latRad) - tan(altRad) * cos(latRad) * cos(azRad));
        portENTER_CRITICAL(&mutex);
        targetAltRate = dAlt;
        targetAzRate = dAz;
        portEXIT_CRITICAL(&mutex);
    }
}