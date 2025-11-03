/**
 * @file esp_wifi.h
 * @brief ESP32 WiFi low-level functions mock
 */

#pragma once

#include "WiFi.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void esp_wifi_set_ps(wifi_ps_type_t type) {
    // Mock - power save mode does nothing on PC
}

#ifdef __cplusplus
}
#endif
