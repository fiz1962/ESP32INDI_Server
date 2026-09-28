#include "core.h"
#include <cmath> // Required for sin, cos, atan2, and asin

// Define configuration constants
double LATITUDE = 30.6744;       
double LONGITUDE = -96.3704;     
const double STEPS_PER_REV = 20000.0;  
const double DEGS_PER_STEP = 360.0 / STEPS_PER_REV;
const double OMEGA = 360.9856474 / 86400.0 / 1000.;

const int AZ_STEP_PIN = 16;
const int AZ_DIR_PIN  = 17;
const int ALT_STEP_PIN = 18;
const int ALT_DIR_PIN  = 19;

// Define shared state variables
double currentAlt = 45.0;  
double currentAz = 180.0;  
double initialLST = 312.5815; 
unsigned long startTime = 0;

// Calculated outputs to pass back to INDI / KStars
double currentRA = 0.0;
double currentDec = 0.0;

double altStepAccumulator = 0.0;
double azStepAccumulator = 0.0;

portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;
volatile double targetAltRate = 0.0; 
volatile double targetAzRate = 0.0;  
volatile long totalAltSteps = 0;
volatile long totalAzSteps = 0;
volatile bool isTracking = false;

// Converts Horizontal Coordinates (Alt/Az) to Equatorial Coordinates (RA/Dec)
void calcRaDec() {
    portENTER_CRITICAL(&mutex);
    double alt = currentAlt;
    double az = currentAz;
    unsigned long elapsed = millis() - startTime;
    portEXIT_CRITICAL(&mutex);

    // 1. Calculate current Local Sidereal Time (LST) based on elapsed time
    double currentLST = initialLST + (OMEGA * elapsed);
    
    // Normalize LST to 0-360 degrees
    currentLST = fmod(currentLST, 360.0);
    if (currentLST < 0) currentLST += 360.0;

    // 2. Convert degrees to radians for trigonometric calculations
    double altRad = alt * M_PI / 180.0;
    double azRad = az * M_PI / 180.0;
    double latRad = LATITUDE * M_PI / 180.0;

    // 3. Calculate Declination (Dec) using spherical trigonometry
    double sinDec = (sin(altRad) * sin(latRad)) + (cos(altRad) * cos(latRad) * cos(azRad));
    // Clamp values to protect against floating-point edge inaccuracies
    if (sinDec > 1.0) sinDec = 1.0;
    if (sinDec < -1.0) sinDec = -1.0;
    double decRad = asin(sinDec);
    
    // 4. Calculate Hour Angle (HA)
    double y = -sin(azRad) * cos(altRad);
    double x = (sin(altRad) * cos(latRad)) - (cos(altRad) * sin(latRad) * cos(azRad));
    double haRad = atan2(y, x);

    // 5. Convert back to degrees
    double decDeg = decRad * 180.0 / M_PI;
    double haDeg = haRad * 180.0 / M_PI;

    // 6. Calculate Right Ascension (RA = LST - HA)
    double raDeg = currentLST - haDeg;

    // Normalize RA to 0-360 degrees
    raDeg = fmod(raDeg, 360.0);
    if (raDeg < 0) raDeg += 360.0;

    // 7. Update public state variables
    portENTER_CRITICAL(&mutex);
    currentRA = raDeg;
    currentDec = decDeg;
    portEXIT_CRITICAL(&mutex);

    //Serial.printf("LATITUDE %lf, Alt %lf, Az %lf, initLST %lf, start time %lf, Omega %lf, elapsed %d, LST %lf, haDeg %lf, RA %lf, Dec %lf\r\n", LATITUDE, currentAlt, currentAz, initialLST, startTime/1000, OMEGA*1000, elapsed/1000, currentLST, haDeg, currentRA, currentDec);
}

// Converts Equatorial Coordinates (RA/Dec) to Horizontal Coordinates (Alt/Az)
void calcAltAz(double targetRA, double targetDec, double &outAlt, double &outAz) {
    portENTER_CRITICAL(&mutex);
    unsigned long elapsed = millis() - startTime;
    portEXIT_CRITICAL(&mutex);

    // 1. Calculate current Local Sidereal Time (LST)
    double currentLST = initialLST + (OMEGA * elapsed);
    currentLST = fmod(currentLST, 360.0);
    if (currentLST < 0) currentLST += 360.0;

    // 2. Calculate Hour Angle (HA = LST - RA)
    double haDeg = currentLST - targetRA;
    
    // 3. Convert degrees to radians
    double haRad = haDeg * M_PI / 180.0;
    double decRad = targetDec * M_PI / 180.0;
    double latRad = LATITUDE * M_PI / 180.0;

    // 4. Calculate Altitude (Alt)
    double sinAlt = (sin(decRad) * sin(latRad)) + (cos(decRad) * cos(latRad) * cos(haRad));
    // Clamp values to protect against floating-point edge inaccuracies
    if (sinAlt > 1.0) sinAlt = 1.0;
    if (sinAlt < -1.0) sinAlt = -1.0;
    double altRad = asin(sinAlt);

    // 5. Calculate Azimuth (Az)
    double y = -sin(haRad) * cos(decRad);
    double x = (sin(decRad) * cos(latRad)) - (cos(decRad) * sin(latRad) * cos(haRad));
    double azRad = atan2(y, x);

    // 6. Convert back to degrees
    outAlt = altRad * 180.0 / M_PI;
    outAz = azRad * 180.0 / M_PI;

    // Normalize Azimuth to 0-360 degrees
    outAz = fmod(outAz, 360.0);
    if (outAz < 0) outAz += 360.0;
}

void startTracking() {
    portENTER_CRITICAL(&mutex);
    isTracking = true;
    //startTime = millis();       
    altStepAccumulator = 0.0;   
    azStepAccumulator = 0.0;
    portEXIT_CRITICAL(&mutex);
    Serial.println("[TRACKING] Started.");
}

void stopTracking() {
    portENTER_CRITICAL(&mutex);
    isTracking = false;
    targetAltRate = 0.0;        
    targetAzRate = 0.0;
    portEXIT_CRITICAL(&mutex);
    Serial.println("[TRACKING] Stopped.");
}

void stepperTask(void *pvParameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(1);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (true) {
        portENTER_CRITICAL(&mutex);
        bool trackingActive = isTracking;
        double lAltRate = targetAltRate;
        double lAzRate = targetAzRate;
        portEXIT_CRITICAL(&mutex);

        if (!trackingActive) {
            vTaskDelayUntil(&xLastWakeTime, xFrequency);
            continue;
        }

        altStepAccumulator += (lAltRate / 1000.0) / DEGS_PER_STEP;
        azStepAccumulator += (lAzRate / 1000.0) / DEGS_PER_STEP;

        portENTER_CRITICAL(&mutex);
        if (altStepAccumulator >= 1.0) {
            digitalWrite(ALT_DIR_PIN, HIGH);
            digitalWrite(ALT_STEP_PIN, HIGH);
            digitalWrite(ALT_STEP_PIN, LOW);
            altStepAccumulator -= 1.0;
            currentAlt += DEGS_PER_STEP;
            totalAltSteps++;
        } else if (altStepAccumulator <= -1.0) {
            digitalWrite(ALT_DIR_PIN, LOW);
            digitalWrite(ALT_STEP_PIN, HIGH);
            digitalWrite(ALT_STEP_PIN, LOW);
            altStepAccumulator += 1.0;
            currentAlt -= DEGS_PER_STEP;
            totalAltSteps--;
        }

        if (azStepAccumulator >= 1.0) {
            digitalWrite(AZ_DIR_PIN, HIGH);
            digitalWrite(AZ_STEP_PIN, HIGH);
            digitalWrite(AZ_STEP_PIN, LOW);
            azStepAccumulator -= 1.0;
            currentAz += DEGS_PER_STEP;
            totalAzSteps++;
        } else if (azStepAccumulator <= -1.0) {
            digitalWrite(AZ_DIR_PIN, LOW);
            digitalWrite(AZ_STEP_PIN, HIGH);
            digitalWrite(AZ_STEP_PIN, LOW);
            azStepAccumulator += 1.0;
            currentAz -= DEGS_PER_STEP;
            totalAzSteps--;
        }
        portEXIT_CRITICAL(&mutex);

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
