#ifndef CORE_H
#define CORE_H

#include <Arduino.h>

struct DateTime {
    int year, month, day, hour, minute;
    double second;
};

// Configuration & Pins
extern double LATITUDE;
extern double LONGITUDE;
extern const double STEPS_PER_REV;
extern const double DEGS_PER_STEP;
extern const double OMEGA;

extern const int AZ_STEP_PIN;
extern const int AZ_DIR_PIN;
extern const int ALT_STEP_PIN;
extern const int ALT_DIR_PIN;

// Shared State & Synchronization
extern double currentAlt;
extern double currentAz;
extern double initialLST;
extern unsigned long startTime;
extern double currentRA;
extern double currentDec;

extern portMUX_TYPE mutex;
extern volatile double targetAltRate;
extern volatile double targetAzRate;
extern volatile long totalAltSteps;
extern volatile long totalAzSteps;
extern volatile bool isTracking;

// Function Prototypes
void startTracking();
void stopTracking();
void stepperTask(void *pvParameters);
void calcRaDec();
void calcAltAz(double targetRA, double targetDec, double &outAlt, double &outAz);
double calculateLST(DateTime dt, double longitude);
bool parseISO8601(const char* isoStr, DateTime &dt);

#endif