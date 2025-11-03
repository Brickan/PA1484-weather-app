#ifndef HARDWARE_MOCK_H
#define HARDWARE_MOCK_H

#include <stdio.h>
#include <stdbool.h>
#include <sys/time.h>

#define LV_SIMULATOR 1

// T4-S3 REAL SPECS: 600×450 AMOLED
#define TFT_WIDTH  600
#define TFT_HEIGHT 450

static inline void initHardware() {
    printf("[MOCK] Hardware initialized\n");
}

static inline int getBatteryPercent() {
    return 85;
}

static inline bool isCharging() {
    return false;
}

static inline bool getTouchPoint(int16_t *x, int16_t *y) {
    return false;
}

static inline void initWiFi() {
    printf("[MOCK] WiFi initialized\n");
}

static inline bool isWiFiConnected() {
    return true;
}

#endif
