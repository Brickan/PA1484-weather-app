/**
 * @file LilyGo_AMOLED.h
 * @brief LilyGo AMOLED display mock for PC simulation
 */

#pragma once

#include "Arduino.h"
#include "../hardware_mock.h"

class LilyGo_Class {
public:
    int width() { return TFT_WIDTH; }
    int height() { return TFT_HEIGHT; }
    bool begin() { return true; }
    
    bool beginAMOLED_147() {
        printf("[Display] LilyGo AMOLED 1.47\" initialized (%dx%d)\n", TFT_WIDTH, TFT_HEIGHT);
        return true;
    }
    
    bool beginAMOLED_191() {
        printf("[Display] LilyGo AMOLED 1.91\" initialized (%dx%d)\n", TFT_WIDTH, TFT_HEIGHT);
        return true;
    }
    
    bool beginAMOLED_241() {
        printf("[Display] LilyGo AMOLED 2.41\" (T4-S3) initialized (%dx%d)\n", TFT_WIDTH, TFT_HEIGHT);
        return true;
    }
    
    void setBrightness(int brightness) {
        // Mock - does nothing on PC
    }
    
    void setRotation(int rotation) {
        // Mock - does nothing on PC
    }
    
    bool hasTouch() { return false; }
    uint8_t getPoint(int16_t* x, int16_t* y, uint8_t get_point = 1) { return 0; }
    bool isPressed() { return false; }
};

// Mock global instance - will be overridden by project.cpp
// This is just a placeholder for header inclusion
extern LilyGo_Class amoled;
