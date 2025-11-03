/**
 * @file esp_system.h
 * @brief ESP32 system functions mock
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Reset reasons
typedef enum {
    ESP_RST_UNKNOWN = 0,
    ESP_RST_POWERON = 1,
    ESP_RST_EXT = 2,
    ESP_RST_SW = 3,
    ESP_RST_PANIC = 4,
    ESP_RST_INT_WDT = 5,
    ESP_RST_TASK_WDT = 6,
    ESP_RST_WDT = 7,
    ESP_RST_DEEPSLEEP = 8,
    ESP_RST_BROWNOUT = 9,
    ESP_RST_SDIO = 10
} esp_reset_reason_t;

static inline esp_reset_reason_t esp_reset_reason() {
    return ESP_RST_POWERON;
}

// PSRAM functions for T4-S3
static inline bool psramFound() {
    return true;  // T4-S3 has 8MB PSRAM
}

// ESP32 mock class - updated for T4-S3 with PSRAM
class ESP32Class {
public:
    uint32_t getFreeHeap() { return 280000 + (rand() % 5000); }  // ~280KB free heap
    uint32_t getHeapSize() { return 327680; }  // 320KB total heap
    uint32_t getPsramSize() { return 8388608; }  // 8MB PSRAM on T4-S3
    uint32_t getFreePsram() { return 8300000 + (rand() % 88608); }  // ~8MB free PSRAM
    uint8_t getChipRevision() { return 1; }
    uint8_t getCpuFreqMHz() { return 240; }
    uint32_t getCycleCount() { return millis() * 240000; }
    String getSketchMD5() { return String("00000000000000000000000000000000"); }
    void restart() { printf("[ESP] Restart requested\n"); exit(0); }
    void deepSleep(uint64_t time_us) { printf("[ESP] Deep sleep for %llu us\n", time_us); delay(time_us / 1000); }
};

static ESP32Class ESP;

#ifdef __cplusplus
}
#endif
