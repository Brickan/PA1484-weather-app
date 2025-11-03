#ifndef ESP_TASK_WDT_H
#define ESP_TASK_WDT_H

// Mock ESP32 Task Watchdog Timer for simulator
// This header provides compatibility for ESP32 watchdog functions

#ifdef __cplusplus
extern "C" {
#endif

// Mock watchdog timer functions that do nothing in simulator
inline void esp_task_wdt_reset() {
    // In simulator, we don't need watchdog reset
    // This is a no-op
}

inline void esp_task_wdt_init(uint32_t timeout, bool panic_on_trigger) {
    // Mock implementation
    (void)timeout;
    (void)panic_on_trigger;
}

inline void esp_task_wdt_add(void* handle) {
    // Mock implementation
    (void)handle;
}

inline void esp_task_wdt_delete(void* handle) {
    // Mock implementation
    (void)handle;
}

#ifdef __cplusplus
}
#endif

#endif // ESP_TASK_WDT_H