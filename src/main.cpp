#include <Arduino.h>
#include <Wire.h>
#include <ADXL345.h>
#include <LittleFS.h>

ADXL345 adxl = ADXL345();
const int VIBRATION_SAMPLES = 10;
const int MEASUREMENT_INTERVAL_MS = 1000; // log measure every x ms

const char* BASE_FILENAME = "/vib%04d.log";
const int MAX_FILES = 9999;

void setup() {
    Serial.begin(115200);
    Serial.println("Vibration logger for Raspberry Pi Pico");

    Wire.begin();

    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed");
        return;
    }

    adxl.powerOn();
    adxl.setRangeSetting(2);
    adxl.setSpiBit(0);

    Serial.println("Setup complete");
}

int calculateVibrationLevel() {
    int x, y, z;
    int px, py, pz;
    int intensity = 0;
    adxl.readAccel(&px, &py, &pz);
    for (int i = 0; i < VIBRATION_SAMPLES; i++) {
        adxl.readAccel(&x, &y, &z);
        intensity += abs(x - px) + abs(y - py) + abs(z - pz);
        px = x; py = y; pz = z;
    }
    return intensity;
}

String findNextAvailableFilename() {
    char filename[20];
    for (int i = 1; i <= MAX_FILES; i++) {
        snprintf(filename, sizeof(filename), BASE_FILENAME, i);
        if (!LittleFS.exists(filename)) {
            return String(filename);
        }
    }
    return "";
}

void loop() {
    static String filename = findNextAvailableFilename();
    static unsigned long startTime = millis();
    
    if (filename.isEmpty()) {
        Serial.println("Error: No available filename");
        while(1) delay(1000);
    }

    int vibrationLevel = calculateVibrationLevel();
    unsigned long currentTime = millis() - startTime;

    char dataRow[50];
    snprintf(dataRow, sizeof(dataRow), "%lu,%d", currentTime, vibrationLevel);

    Serial.println(dataRow);

    File dataFile = LittleFS.open(filename.c_str(), "a");
    if (dataFile) {
        dataFile.println(dataRow);
        dataFile.close();
    } else {
        Serial.print("Error opening file - ");
        Serial.println(filename);
    }

    delay(MEASUREMENT_INTERVAL_MS);
}
