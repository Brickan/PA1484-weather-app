/**
 * @file LV_Helper.h
 * @brief LV_Helper mock for PC simulation
 */

#pragma once

#include "LilyGo_AMOLED.h"
#include <lvgl.h>
#include <string.h>
#include <stdlib.h>

// Label text storage using per-label persistent buffers
#include <map>
#include <string>

static std::map<void*, std::string> label_text_storage;

// Override lv_label_set_text to store strings persistently per label
#define lv_label_set_text(label, text) \
    lv_label_set_text_impl(label, text)

static inline void lv_label_set_text_impl(lv_obj_t* label, const char* text) {
    if (label == NULL || text == NULL) return;

    // Store text persistently for this specific label
    label_text_storage[label] = text;

    // Call original LVGL function with persistent string data
    lv_label_set_text_static(label, label_text_storage[label].c_str());
}

// Mock function to initialize LVGL with the display
static inline void beginLvglHelper(LilyGo_Class& display) {
    printf("[LVGL] Helper initialized for display\n");
}
