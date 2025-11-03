/**
 * @file project_wrapper.cpp
 * @brief Universal Arduino .ino file wrapper for PC simulation
 */

// IMPORTANT: Define these FIRST, before ANY includes
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

// Include time headers early
#include <time.h>
#include <sys/time.h>

// Include LVGL first (required by main.c)
#include "lvgl/lvgl.h"

// On Windows, undefine INPUT before including headers that use Windows API
#ifdef _WIN32
    #ifdef INPUT
        #undef INPUT
    #endif
#endif

// Include Arduino compatibility layer - this provides ALL Arduino APIs
#include "arduino_compat/WiFi.h"
#include "arduino_compat/WiFiClientSecure.h"
#include "arduino_compat/HTTPClient.h"
#include "arduino_compat/ArduinoJson.h"
#include "arduino_compat/Arduino.h"
#include "arduino_compat/LilyGo_AMOLED.h"
#include "arduino_compat/LV_Helper.h"
#include "arduino_compat/esp_wifi.h"
#include "arduino_compat/esp_system.h"
#include "arduino_compat/esp_sntp.h"
// time.h is included from system headers directly

// Forward declare the Arduino functions that will be defined in the .ino file
void arduino_setup();
void arduino_loop();

// =============================================================================
// INCLUDE THE ARDUINO SKETCH DIRECTLY
// =============================================================================
// Rename setup/loop during include to avoid conflicts
#define setup arduino_setup
#define loop arduino_loop

// Define compatibility macro for newer LVGL spinner API in simulator
#define lv_spinner_create_compat(parent, time, arc) lv_spinner_create(parent)

// Override the old spinner API for simulator
#define lv_spinner_create(parent, ...) lv_spinner_create(parent)

#include "../../project/project.ino"

#undef setup
#undef loop

// =============================================================================
// C WRAPPERS FOR MAIN.C
// =============================================================================
extern "C" {
    void setup(void) {
        arduino_setup();
    }
    
    void loop(void) {
        arduino_loop();
    }
}
