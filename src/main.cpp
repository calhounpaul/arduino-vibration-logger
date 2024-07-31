#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADXL345_U.h>
#include <hardware/flash.h>
#include <hardware/sync.h>

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
const int VIBRATION_SAMPLES = 10;
const int MEASUREMENT_INTERVAL_MS = 1000; // log measure every x ms

const uint32_t FLASH_TARGET_OFFSET = 256 * 1024; // Use the last 256KB of flash
const int MAX_LOG_ENTRIES = (256 * 1024 - sizeof(int)) / (sizeof(unsigned long) + sizeof(int));
int currentLogIndex = 0;

void readFromFlash(void* data, size_t len, uint32_t offset = 0) {
    noInterrupts();
    memcpy(data, (void *)(XIP_BASE + FLASH_TARGET_OFFSET + offset), len);
    interrupts();
}

void writeToFlash(const void* data, size_t len) {
    noInterrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t*)data, len);
    interrupts();
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    Serial.println("Vibration logger for Raspberry Pi Pico");

    Wire.begin();

    if (!accel.begin()) {
        Serial.println("Could not find a valid ADXL345 sensor, check wiring!");
        while (1);
    }

    accel.setRange(ADXL345_RANGE_2_G);

    // Read the current log index from flash
    readFromFlash(&currentLogIndex, sizeof(int));

    if (currentLogIndex >= MAX_LOG_ENTRIES || currentLogIndex < 0) {
        currentLogIndex = 0;
    }

    Serial.print("Starting at log index: ");
    Serial.println(currentLogIndex);

    Serial.println("Setup complete");
}

int calculateVibrationLevel() {
    sensors_event_t event;
    float px, py, pz;
    float intensity = 0;
    
    accel.getEvent(&event);
    px = event.acceleration.x;
    py = event.acceleration.y;
    pz = event.acceleration.z;

    for (int i = 0; i < VIBRATION_SAMPLES; i++) {
        accel.getEvent(&event);
        intensity += abs(event.acceleration.x - px) + 
                     abs(event.acceleration.y - py) + 
                     abs(event.acceleration.z - pz);
        px = event.acceleration.x;
        py = event.acceleration.y;
        pz = event.acceleration.z;
    }
    return static_cast<int>(intensity * 100); // Scale up for integer storage
}

void dumpData() {
    int storedIndex;
    readFromFlash(&storedIndex, sizeof(int));
    
    Serial.println("Timestamp,VibrationLevel");
    
    for (int i = 0; i < storedIndex; i++) {
        unsigned long timestamp;
        int vibrationLevel;
        uint32_t dataOffset = sizeof(int) + i * (sizeof(unsigned long) + sizeof(int));
        
        readFromFlash(&timestamp, sizeof(unsigned long), dataOffset);
        readFromFlash(&vibrationLevel, sizeof(int), dataOffset + sizeof(unsigned long));
        
        Serial.print(timestamp);
        Serial.print(",");
        Serial.println(vibrationLevel);
    }
    
    Serial.println("End of data");
}

void processCommand(String command) {
    if (command == "dump") {
        dumpData();
    } else if (command == "reset") {
        currentLogIndex = 0;
        writeToFlash(&currentLogIndex, sizeof(int));
        Serial.println("Log reset");
    } else {
        Serial.println("Unknown command. Available commands: dump, reset");
    }
}

void loop() {
    int vibrationLevel = calculateVibrationLevel();
    unsigned long currentTime = millis();

    // Prepare data for flash
    uint8_t buffer[FLASH_PAGE_SIZE];
    memcpy(buffer, &currentLogIndex, sizeof(int));
    uint32_t dataOffset = sizeof(int) + currentLogIndex * (sizeof(unsigned long) + sizeof(int));
    memcpy(buffer + dataOffset, &currentTime, sizeof(unsigned long));
    memcpy(buffer + dataOffset + sizeof(unsigned long), &vibrationLevel, sizeof(int));

    // Write to flash
    writeToFlash(buffer, FLASH_PAGE_SIZE);

    currentLogIndex++;
    if (currentLogIndex >= MAX_LOG_ENTRIES) {
        currentLogIndex = 0;
    }

    char dataRow[50];
    snprintf(dataRow, sizeof(dataRow), "%lu,%d", currentTime, vibrationLevel);
    Serial.println(dataRow);

    // Check for serial commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        processCommand(command);
    }

    delay(MEASUREMENT_INTERVAL_MS);
}
