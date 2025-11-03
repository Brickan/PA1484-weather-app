/**
 * @file lvgl.h
 * @brief LVGL library wrapper - uses real LVGL with compatibility helpers
 */

#pragma once

// Include the real LVGL library
#include "lvgl/lvgl.h"

// Compatibility macro for lv_event_get_target - add cast for C++
#ifdef __cplusplus
#undef lv_event_get_target
#define lv_event_get_target(e) ((lv_obj_t*)::lv_event_get_target(e))
#endif
