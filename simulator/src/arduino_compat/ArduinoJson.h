/**
 * @file ArduinoJson.h
 * @brief ArduinoJson library wrapper - forwards to real ArduinoJson
 *
 * This wrapper prevents circular includes by directly including
 * the real ArduinoJson.h from the FetchContent directory.
 */

#pragma once

// Prevent circular include - include the real ArduinoJson directly
// The FetchContent ArduinoJson is in the build/_deps directory
// and CMake adds it to the include path

// First, check if we can find it via the standard path
#if __has_include("ArduinoJson.hpp")
#include "ArduinoJson.hpp"
using namespace ArduinoJson;
#elif __has_include(<ArduinoJson.hpp>)
#include <ArduinoJson.hpp>
using namespace ArduinoJson;
#else
// Fallback: include system ArduinoJson
// This works because CMake adds the arduinojson include path
#include <ArduinoJson/ArduinoJson.hpp>
using namespace ArduinoJson;
#endif
