/**
 * @file ESP32.h
 * @brief ESP32-specific function compatibility layer for PC simulation
 *
 * This file provides stubs for ESP32-specific functions that don't exist
 * on PC but are needed to compile and run ESP32 sketches in the simulator.
 */

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32
#include <unistd.h>  // For usleep
#endif

// ESP32 reset reasons enum
typedef enum {
    ESP_RST_UNKNOWN = 0,    // Reset reason can not be determined
    ESP_RST_POWERON = 1,    // Reset due to power-on event
    ESP_RST_EXT = 2,        // Reset by external pin
    ESP_RST_SW = 3,         // Software reset
    ESP_RST_PANIC = 4,      // Software reset due to exception/panic
    ESP_RST_INT_WDT = 5,    // Reset (software or hardware) due to interrupt watchdog
    ESP_RST_TASK_WDT = 6,   // Reset due to task watchdog
    ESP_RST_WDT = 7,        // Reset due to other watchdogs
    ESP_RST_DEEPSLEEP = 8,  // Reset after exiting deep sleep mode
    ESP_RST_BROWNOUT = 9,   // Brownout reset (software or hardware)
    ESP_RST_SDIO = 10       // Reset over SDIO
} esp_reset_reason_t;

// ESP class for system information
class EspClass {
public:
    // Memory functions - return simulated values
    static uint32_t getFreeHeap() {
        // Simulate 280KB free heap (typical for ESP32-S3)
        return 280000 + (rand() % 5000);
    }

    static uint32_t getHeapSize() {
        // Simulate 320KB total heap
        return 327680;
    }

    static uint32_t getFreePsram() {
        // Simulate 8MB PSRAM with some used
        return 8300000 + (rand() % 88608);
    }

    static uint32_t getPsramSize() {
        // Simulate 8MB PSRAM (T4-S3 has 8MB)
        return 8388608;
    }

    static uint32_t getFreeSketchSpace() {
        // Simulate 4MB free sketch space
        return 4194304;
    }

    static uint32_t getSketchSize() {
        // Simulate 1.4MB sketch size
        return 1420000;
    }

    static const char* getSdkVersion() {
        return "ESP-IDF v4.4.0";
    }

    static uint32_t getCpuFreqMHz() {
        return 240;  // ESP32-S3 runs at 240MHz
    }

    static uint32_t getCycleCount() {
        // Return a simulated cycle count
        static uint32_t cycles = 0;
        cycles += 240000;  // Increment by some amount
        return cycles;
    }

    static void restart() {
        printf("[SIMULATOR] ESP.restart() called - would restart ESP32\n");
        // In real implementation, could restart the simulator
        exit(0);
    }

    static uint8_t getChipRevision() {
        return 0;  // Revision 0
    }

    static const char* getChipModel() {
        return "ESP32-S3";
    }

    static uint8_t getChipCores() {
        return 2;  // Dual core
    }

    static uint64_t getEfuseMac() {
        return 0x123456789ABC;  // Simulated MAC
    }
};

// Global ESP object
extern EspClass ESP;

// PSRAM functions
inline bool psramFound() {
    // Always return true to simulate T4-S3 with PSRAM
    return true;
}

inline bool psramInit() {
    return true;
}

inline size_t psramSize() {
    return 8388608;  // 8MB
}

// ESP32 system functions
inline esp_reset_reason_t esp_reset_reason() {
    // Always return power-on reset in simulator
    return ESP_RST_POWERON;
}

// Watchdog functions
inline void esp_task_wdt_init(uint32_t timeout, bool panic) {
    // No-op in simulator
}

inline void esp_task_wdt_reset() {
    // No-op in simulator
}

inline void esp_task_wdt_add(void* task) {
    // No-op in simulator
}

inline void esp_task_wdt_delete(void* task) {
    // No-op in simulator
}

// Yield function for task switching
inline void yield() {
    // Could add a small delay or thread yield here if needed
    #ifdef _WIN32
        // Windows
    #else
        usleep(1);  // 1 microsecond delay
    #endif
}

// Memory allocation from PSRAM
inline void* ps_malloc(size_t size) {
    // In simulator, just use regular malloc
    return malloc(size);
}

inline void* ps_calloc(size_t nmemb, size_t size) {
    // In simulator, just use regular calloc
    return calloc(nmemb, size);
}

inline void* ps_realloc(void* ptr, size_t size) {
    // In simulator, just use regular realloc
    return realloc(ptr, size);
}

// Deep sleep functions
inline void esp_deep_sleep_start() {
    printf("[SIMULATOR] Deep sleep requested\n");
    exit(0);
}

inline void esp_sleep_enable_timer_wakeup(uint64_t time_in_us) {
    printf("[SIMULATOR] Sleep timer set for %llu us\n", time_in_us);
}

// RTC memory functions
inline uint32_t rtc_get_reset_reason(int cpu) {
    return 1;  // POWERON_RESET
}

// CPU frequency functions
inline bool setCpuFrequencyMhz(uint32_t freq) {
    printf("[SIMULATOR] CPU frequency set to %u MHz\n", freq);
    return true;
}

inline uint32_t getCpuFrequencyMhz() {
    return 240;
}