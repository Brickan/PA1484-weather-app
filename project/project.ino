/*
 * Weather Station v1
 *
 * What it does:
 * - Connects to WiFi
 * - Gets weather from SMHI API
 * - Shows temp, humidity, wind, pressure on screen
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <lvgl.h>

//Api And wifi from/for Final

// ============================================================================
// SECTION 2: CONFIGURATION SETTINGS
// ============================================================================
// Change these settings to match your environment

// WiFi Configuration - CHANGE THESE TO YOUR NETWORK!
const char *WIFI_SSID = "APx";                   // Your WiFi network name
const char *WIFI_PASSWORD = "Password.Password"; // Your WiFi password

// Weather API Configuration
// This URL gets weather data for Karlskrona, Sweden
// To change location, modify the lon (longitude) and lat (latitude) values
const char *API_URL = "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/15.59/lat/56.16/data.json";

// Timezone Configuration (Central European Time with Daylight Saving)
const char *TZ_INFO = "CET-1CEST,M3.5.0,M10.5.0/3";

// ============================================================================
// SECTION 3: DATA STRUCTURES
// ============================================================================
// These structures hold our weather data in an organized way

/**
 * Holds data for a 3-hour forecast period
 */
struct HourlyWeather
{
    char time[8];     // Time label: "Morning", "Noon", or "Evening"
    float temp;       // Temperature in Celsius
    int symbol;       // Weather symbol code (1-27)
    float wind;       // Wind speed in m/s
    int windDir;      // Wind direction in degrees
    float rainChance; // Chance of rain in percentage
    bool valid;       // True if data is available
};

/**
 * Holds current weather data
 */
struct TodayWeather
{
    float temp;     // Current temperature in Celsius
    float humidity; // Humidity percentage
    float wind;     // Wind speed in m/s
    int windDir;    // Wind direction in degrees
    float pressure; // Atmospheric pressure in hPa
    int symbol;     // Weather symbol code
    char desc[64];  // Weather description text
    bool valid;     // True if data is available
};

/**
 * Holds daily forecast data
 */
struct DayWeather
{
    char date[16];    // Date in format "2025-10-22"
    char dayName[12]; // Day name like "Monday"
    float tempMin;    // Minimum temperature
    float tempMax;    // Maximum temperature
    int symbol;       // Weather symbol code
    float rainChance; // Maximum rain chance for the day
    char desc[64];    // Weather description
    bool valid;       // True if data is available
};



// ============================================================================
// SECTION 9: WEATHER DATA FETCHING
// ============================================================================
// Main function for getting weather data from API

// Forward declarations for UI update functions
void updateCurrentWeatherDisplay();
void updateForecastDisplay();

/**
 * Fetches weather data from SMHI API
 * This is the main data fetching function
 */
void fetchWeatherData()
{
    // Check if already fetching (prevent multiple simultaneous requests)
    if (isFetching)
    {
        Serial.println("Already fetching data - skipping request");
        return;
    }

    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("No WiFi connection - cannot fetch data");
        return;
    }

    // Set fetching flag to prevent concurrent requests
    isFetching = true;
    Serial.println("Fetching weather data...");

    // Clear old data
    today.valid = false;
    for (int i = 0; i < 3; i++)
    {
        hourly[i].valid = false;
    }
    for (int i = 0; i < 6; i++)
    {
        days[i].valid = false;
        days[i].tempMin = 999;  // Set to high value for minimum comparison
        days[i].tempMax = -999; // Set to low value for maximum comparison
        days[i].symbol = 0;
        days[i].rainChance = 0;
    }

    // Create HTTPS client
    WiFiClientSecure client;
    client.setInsecure();  // Skip certificate verification for simplicity
    client.setTimeout(15); // 15 second timeout

    // Create HTTP client
    HTTPClient http;
    http.setTimeout(30000); // 30 second timeout for T4-S3
    http.setReuse(false);   // Don't reuse connection

    // Try to connect with retry logic
    int retryCount = 3;
    bool connected = false;

    while (retryCount > 0 && !connected)
    {
        if (http.begin(client, API_URL))
        {
            connected = true;
            Serial.printf("Connected to API (attempt %d)\n", 4 - retryCount);
        }
        else
        {
            retryCount--;
            Serial.printf("Connection failed, %d retries left\n", retryCount);
            if (retryCount > 0)
            {
                delay(2000); // Wait 2 seconds before retry
                yield();     // Let system process other tasks
            }
        }
    }

    // Check if connection was successful
    if (!connected)
    {
        Serial.println("Failed to connect to API after all retries");
        isFetching = false;
        return;
    }

    Serial.println("Sending GET request to API...");
    lv_timer_handler(); // Update UI

    // Make HTTP GET request with retry logic
    int responseCode = -1;
    retryCount = 2;

    while (retryCount >= 0 && responseCode != 200)
    {
        responseCode = http.GET();
        Serial.printf("HTTP response code: %d (attempt %d)\n",
                      responseCode, 3 - retryCount);
        lv_timer_handler();

        if (responseCode == 200)
        {
            break; // Success!
        }
        else if (retryCount > 0)
        {
            // Only retry for server errors
            if (responseCode >= 500)
            {
                Serial.println("Server error, retrying...");
                delay(3000);
                yield();
                retryCount--;
            }
            else
            {
                break; // Don't retry for client errors
            }
        }
    }

    // Check if request was successful
    if (responseCode != 200)
    {
        Serial.printf("HTTP error: %d\n", responseCode);
        http.end();
        isFetching = false;
        return;
    }

    // Get response stream
    WiFiClient *stream = http.getStreamPtr();
    if (stream == NULL)
    {
        Serial.println("No data stream available");
        http.end();
        isFetching = false;
        return;
    }

    // Find the start of weather data array
    if (!stream->find("\"timeSeries\":["))
    {
        Serial.println("Could not find weather data in response");
        http.end();
        isFetching = false;
        return;
    }

    Serial.println("Parsing weather data...");

    // Create JSON document for parsing
    // JsonDocument in v7 automatically manages memory
    // It will use heap first, then PSRAM if available
    JsonDocument doc;

    // Variables for tracking parsing progress
    char lastDate[16] = "";
    char firstDate[16] = "";
    int dayIndex = 0;
    int forecastCount = 0;
    bool firstDataProcessed = false;
    bool firstDateSet = false;
    uint32_t parseStartTime = millis();
    int parseErrors = 0;
    const int maxParseErrors = 10;

    // Target hours for hourly forecast
    const int targetHours[3] = {8, 13, 19}; // 8 AM, 1 PM, 7 PM
    const char *hourLabels[3] = {"Morning", "Noon", "Evening"};
    bool hourlyCollected[3] = {false, false, false};

    // Parse JSON data stream
    int daysCollected = 0;
    while (stream->available() &&
           forecastCount < 500 &&
           parseErrors < maxParseErrors &&
           daysCollected < 6)
    {

        // Check for timeout
        if (millis() - parseStartTime > 30000)
        {
            Serial.println("Parsing timeout");
            break;
        }

        // Clear document for next parse
        doc.clear();

        // Let system breathe every 5 items
        if (forecastCount % 5 == 0)
        {
            yield();
        }

        // Try to parse next JSON object
        DeserializationError parseError = deserializeJson(doc, *stream);
        if (parseError)
        {
            Serial.printf("Parse error at entry %d: %s\n", forecastCount, parseError.c_str());
            parseErrors++;
            if (parseErrors >= maxParseErrors)
            {
                Serial.println("Too many parse errors, stopping");
                break;
            }
            stream->find(",");
            continue;
        }

        // Successful parse - just increment counter
        forecastCount++;

        // Update UI periodically
        if (forecastCount % 10 == 0)
        {
            lv_timer_handler();
            yield();
            esp_task_wdt_reset(); // Reset watchdog
        }

        // Get time of this forecast
        const char *validTime = doc["validTime"];
        if (validTime == NULL || strlen(validTime) < 19)
        {
            stream->findUntil(",", "]");
            continue;
        }

        // Extract date and hour
        char date[16] = "";
        strncpy(date, validTime, 10);
        date[10] = '\0';

        int hour = 0;
        if (strlen(validTime) >= 13)
        {
            hour = (validTime[11] - '0') * 10 + (validTime[12] - '0');
        }

        // Capture first date for hourly forecasts
        if (!firstDateSet)
        {
            strncpy(firstDate, date, sizeof(firstDate) - 1);
            firstDate[sizeof(firstDate) - 1] = '\0';
            firstDateSet = true;
            Serial.printf("First date: %s\n", firstDate);
        }

        // Process current weather (first entry)
        if (!firstDataProcessed && !today.valid)
        {
            JsonArray params = doc["parameters"];
            if (!params.isNull())
            {
                // Loop through parameters
                for (JsonObject param : params)
                {
                    const char *name = param["name"];
                    if (name == NULL)
                        continue;

                    JsonArray values = param["values"];
                    if (values.isNull() || values.size() == 0)
                        continue;

                    float value = values[0];

                    // Store values based on parameter name
                    if (strcmp(name, "t") == 0)
                    {
                        today.temp = value; // Temperature
                    }
                    else if (strcmp(name, "r") == 0)
                    {
                        today.humidity = value; // Relative humidity
                    }
                    else if (strcmp(name, "ws") == 0)
                    {
                        today.wind = value; // Wind speed
                    }
                    else if (strcmp(name, "wd") == 0)
                    {
                        today.windDir = (int)value; // Wind direction
                    }
                    else if (strcmp(name, "msl") == 0)
                    {
                        today.pressure = value; // Sea level pressure
                    }
                    else if (strcmp(name, "Wsymb2") == 0)
                    {
                        today.symbol = (int)value; // Weather symbol
                    }
                }

                // Mark as valid if we got weather symbol
                if (today.symbol > 0)
                {
                    strncpy(today.desc, getWeatherDescription(today.symbol),
                            sizeof(today.desc) - 1);
                    today.desc[sizeof(today.desc) - 1] = '\0';
                    today.valid = true;
                    firstDataProcessed = true;
                    Serial.println("Current weather data collected");
                }
            }
        }

        // Collect hourly forecasts
        bool shouldCollectHourly = false;

        // Check if this is today or tomorrow for hourly data
        if (strcmp(date, firstDate) == 0)
        {
            shouldCollectHourly = true; // Today's data
        }
        else if (dayIndex == 1)
        {
            shouldCollectHourly = true; // Tomorrow's data (in case today's hours passed)
        }

        if (shouldCollectHourly)
        {
            // Check each target hour
            for (int h = 0; h < 3; h++)
            {
                if (hour == targetHours[h] && !hourlyCollected[h])
                {
                    Serial.printf("Found %s forecast (%d:00)\n",
                                  hourLabels[h], targetHours[h]);

                    JsonArray params = doc["parameters"];
                    if (!params.isNull())
                    {
                        // Process parameters
                        for (JsonObject param : params)
                        {
                            const char *name = param["name"];
                            if (name == NULL)
                                continue;

                            JsonArray values = param["values"];
                            if (values.isNull() || values.size() == 0)
                                continue;

                            float value = values[0];

                            // Store hourly values
                            if (strcmp(name, "t") == 0)
                            {
                                hourly[h].temp = value;
                            }
                            else if (strcmp(name, "Wsymb2") == 0)
                            {
                                hourly[h].symbol = (int)value;
                            }
                            else if (strcmp(name, "ws") == 0)
                            {
                                hourly[h].wind = value;
                            }
                            else if (strcmp(name, "wd") == 0)
                            {
                                hourly[h].windDir = (int)value;
                            }
                            else if (strcmp(name, "tstm") == 0)
                            {
                                hourly[h].rainChance = value;
                            }
                        }

                        // Store time label and mark as valid
                        strncpy(hourly[h].time, hourLabels[h],
                                sizeof(hourly[h].time) - 1);
                        hourly[h].time[sizeof(hourly[h].time) - 1] = '\0';
                        hourly[h].valid = true;
                        hourlyCollected[h] = true;
                    }
                }
            }
        }

        // Track day changes
        if (strlen(lastDate) > 0 && strcmp(date, lastDate) != 0)
        {
            dayIndex++;
            Serial.printf("Processing day %d: %s\n", dayIndex, date);
        }
        strncpy(lastDate, date, sizeof(lastDate) - 1);
        lastDate[sizeof(lastDate) - 1] = '\0';

        // Collect 6-day forecast (skip today which is dayIndex 0)
        // dayIndex 1 = tomorrow (days[0]), dayIndex 2 = day after (days[1]), etc.
        if (dayIndex >= 1)
        {                            // Start from tomorrow
            int dIdx = dayIndex - 1; // Map to days array index

            // Make sure we don't go out of bounds (we only want 6 days)
            if (dIdx >= 0 && dIdx < 6)
            {

                JsonArray params = doc["parameters"];
                if (!params.isNull())
                {
                    // Process parameters
                    for (JsonObject param : params)
                    {
                        const char *name = param["name"];
                        if (name == NULL)
                            continue;

                        JsonArray values = param["values"];
                        if (values.isNull() || values.size() == 0)
                            continue;

                        float value = values[0];

                        // Update daily min/max values
                        if (strcmp(name, "t") == 0)
                        {
                            if (value < days[dIdx].tempMin)
                            {
                                days[dIdx].tempMin = value;
                            }
                            if (value > days[dIdx].tempMax)
                            {
                                days[dIdx].tempMax = value;
                            }
                        }
                        else if (strcmp(name, "Wsymb2") == 0 && days[dIdx].symbol == 0)
                        {
                            days[dIdx].symbol = (int)value;
                        }
                        else if (strcmp(name, "tstm") == 0)
                        {
                            if (value > days[dIdx].rainChance)
                            {
                                days[dIdx].rainChance = value;
                            }
                        }
                    }

                    // Store date and day name if not already done
                    // Mark day as valid as soon as we have ANY data for it
                    if (!days[dIdx].valid)
                    {
                        // We got here, which means this is a new day with at least some data
                        strncpy(days[dIdx].date, date, sizeof(days[dIdx].date) - 1);
                        days[dIdx].date[sizeof(days[dIdx].date) - 1] = '\0';
                        calculateDayOfWeek(date, days[dIdx].dayName,
                                           sizeof(days[dIdx].dayName));

                        // Check if we have temperature data
                        if (days[dIdx].tempMin < 999)
                        {
                            // We have temperature data, mark as valid
                            days[dIdx].valid = true;
                            daysCollected++;
                            Serial.printf("Day %d: %s\n", dIdx, date);
                        }
                        else if (days[dIdx].symbol > 0)
                        {
                            // No temperature yet but we have a weather symbol
                            // Set default temps and mark as valid
                            days[dIdx].tempMin = 0;
                            days[dIdx].tempMax = 0;
                            days[dIdx].valid = true;
                            daysCollected++;
                        }

#ifdef ARDUINO_ARCH_ESP32
                        // Skip remaining hourly entries for this day on T4-S3 hardware
                        if (days[dIdx].valid && dIdx < 2)
                        {
                            int entriesToSkip = 0;
                            if (dIdx == 0)
                            {
                                entriesToSkip = 48 - forecastCount - 1;
                            }
                            else if (dIdx == 1)
                            {
                                entriesToSkip = 72 - forecastCount - 1;
                            }

                            if (entriesToSkip > 0 && entriesToSkip < 30)
                            {
                                for (int skip = 0; skip < entriesToSkip; skip++)
                                {
                                    if (!stream->findUntil(",", "]"))
                                    {
                                        break;
                                    }
                                    forecastCount++;
                                    if (skip % 5 == 0)
                                    {
                                        delay(1);
                                    }
                                }
                            }
                        }
#endif

                        // Check if we have all 6 days now
                        if (daysCollected >= 6)
                        {
                            break;
                        }
                    }
                }
            } // Close the bounds check if statement
        }

        // Move to next object
        if (!stream->findUntil(",", "]"))
        {
#ifdef ARDUINO_ARCH_ESP32
            // On T4-S3 hardware, buffer might be temporarily empty
            if (daysCollected < 6 && forecastCount < 100)
            {
                bool foundMore = false;
                for (int retry = 0; retry < 5; retry++)
                {
                    delay(200);
                    if (stream->available() > 0)
                    {
                        if (stream->findUntil(",", "]"))
                        {
                            foundMore = true;
                            break;
                        }
                    }
                }
                if (foundMore)
                {
                    continue;
                }
            }
#endif
            break; // End of array
        }

        // Small delay to prevent watchdog issues on T4-S3
        if (forecastCount % 3 == 0)
        {
            delay(1); // Give other tasks a chance to run
        }
    }

    // Add weather descriptions for daily forecasts
    for (int i = 0; i < 6; i++)
    {
        if (days[i].valid && days[i].symbol > 0)
        {
            strncpy(days[i].desc, getWeatherDescription(days[i].symbol),
                    sizeof(days[i].desc) - 1);
            days[i].desc[sizeof(days[i].desc) - 1] = '\0';
        }
    }

    // Clean up HTTP connection
    http.end();

    // Print summary
    Serial.printf("Parsing complete: %d forecasts processed\n", forecastCount);
    Serial.printf("Data collected - Current: %s, Hourly: %d/3, Daily: %d/6\n",
                  today.valid ? "Yes" : "No",
                  hourly[0].valid + hourly[1].valid + hourly[2].valid,
                  days[0].valid + days[1].valid + days[2].valid +
                      days[3].valid + days[4].valid + days[5].valid);

    // Debug: Show detailed info for each day
    Serial.println("Detailed forecast days:");
    for (int i = 0; i < 6; i++)
    {
        if (days[i].valid)
        {
            Serial.printf("  Day %d: %s (%s) - %.0f/%.0f°C, Rain: %.0f%%\n",
                          i + 1, days[i].date, days[i].dayName,
                          days[i].tempMax, days[i].tempMin, days[i].rainChance);
        }
        else
        {
            Serial.printf("  Day %d: NOT COLLECTED\n", i + 1);
        }
    }
    Serial.printf("Last dayIndex reached: %d\n", dayIndex);

    // Update last refresh time
    lastRefresh = millis();

    // Update UI displays
    updateCurrentWeatherDisplay();
    updateForecastDisplay();

    // Clear fetching flag
    isFetching = false;
    Serial.println("====== WEATHER FETCH COMPLETE ======");
}







/*
 * Weather Station v1
 *
 * What it does:
 * - Connects to WiFi
 * - Gets weather from SMHI API
 * - Shows temp, humidity, wind, pressure on screen
 */


 //Code for demo 1 below
// Your WiFi here
const char *WIFI_SSID = "APx";
const char *WIFI_PASSWORD = "Password.Password";

// Weather API for Karlskrona
const char *API_URL = "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/15.59/lat/56.16/data.json";

// Weather data
struct Weather
{
    float temp;
    float humidity;
    float wind;
    int windDir;
    float pressure;
    int symbol;
    bool valid;
};

Weather weather = {0};
LilyGo_Class amoled;
bool wifiOK = false;

// Screen stuff
lv_obj_t *wifiLabel = NULL;
lv_obj_t *tempLabel = NULL;
lv_obj_t *humidLabel = NULL;
lv_obj_t *windLabel = NULL;
lv_obj_t *pressLabel = NULL;
lv_obj_t *statusLabel = NULL;

// Weather codes to text
const char *weatherText(int code)
{
    if (code < 1 || code > 27)
        return "Unknown";

    const char *txt[] = {
        "Clear", "Nearly clear", "Variable clouds", "Half-clear",
        "Cloudy", "Overcast", "Fog", "Light rain showers",
        "Moderate rain showers", "Heavy rain showers", "Thunder",
        "Light sleet showers", "Moderate sleet showers", "Heavy sleet showers",
        "Light snow showers", "Moderate snow showers", "Heavy snow showers",
        "Light rain", "Moderate rain", "Heavy rain", "Thunder",
        "Light sleet", "Moderate sleet", "Heavy sleet",
        "Light snow", "Moderate snow", "Heavy snow"};

    return txt[code - 1];
}

// Wind direction
const char *windDir(int deg)
{
    const char *dirs[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    return dirs[((deg + 23) / 45) % 8];
}
/*
// Validate weather data before accepting it
bool validateWeather(const Weather &w)
{
    if (w.temp < -60 || w.temp > 60) return false;
    if (w.humidity < 0 || w.humidity > 100) return false;
    if (w.wind < 0 || w.wind > 70) return false;
    if (w.symbol < 1 || w.symbol > 27) return false;
    return true;
}
*/

// Connect WiFi
bool connectWiFi()
{
    Serial.println("Connecting WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 20)
    {
        delay(500);
        Serial.print(".");
        tries++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi OK!");
        Serial.println(WiFi.localIP().toString());
        wifiOK = true;
        return true;
    }

    Serial.println("\nWiFi Failed!");
    wifiOK = false;
    return false;
}

// Update WiFi status on screen
void updateWiFi()
{
    if (!wifiLabel)
        return;

    if (wifiOK)
        lv_label_set_text(wifiLabel, "WiFi: Connected");
    else
        lv_label_set_text(wifiLabel, "WiFi: Disconnected");
}
/*
// Try GET request up to 3 times
bool apiGetWithRetry(HTTPClient &http, int &responseCode)
{
    for (int i = 0; i < 3; i++)
    {
        responseCode = http.GET();
        if (responseCode == 200)
            return true;

        // Retry only on server errors (500+)
        if (responseCode >= 500)
        {
            Serial.println("Server error, retrying...");
            delay(2000);
        }
        else
        {
            return false; // Client error — do not retry
        }
    }
    return false;
}*/


// Get weather from API
bool getWeather()
{
    if (!wifiOK)
    {
        Serial.println("No WiFi!");
        return false;
    }

    /*Serial.println("Getting weather...");
    // CACHE CHECK
    if (weather.valid && (millis() - lastWeatherFetch < WEATHER_CACHE_TIME))
    {
        Serial.println("Using cached weather data.");
        return true;
    }*/

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client, API_URL);
    http.setTimeout(15000);

    int code = http.GET();
    if (code != 200)
    {
        Serial.printf("HTTP failed: %d\n", code);
        http.end();
        return false;
    }
    /*int code;
    if (!apiGetWithRetry(http, code))
    {
        Serial.printf("HTTP failed: %d\n", code);
        http.end();
        return false;
    }*/



    Serial.println("Got data!");

    WiFiClient *stream = http.getStreamPtr();

    // Find the start of timeSeries array in the response
    if (!stream->find("\"timeSeries\":["))
    {
        Serial.println("Could not find timeSeries in response!");
        http.end();
        return false;
    }

    // Now parse just the first entry (current weather)
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, *stream);

    if (error)
    {
        Serial.printf("Parse failed: %s\n", error.c_str());
        http.end();
        return false;
    }

    JsonArray params = doc["parameters"];

    // Extract weather values
    for (JsonObject p : params)
    {
        const char *name = p["name"];
        if (strcmp(name, "t") == 0)
            weather.temp = p["values"][0];
        else if (strcmp(name, "r") == 0)
            weather.humidity = p["values"][0];
        else if (strcmp(name, "ws") == 0)
            weather.wind = p["values"][0];
        else if (strcmp(name, "wd") == 0)
            weather.windDir = p["values"][0];
        else if (strcmp(name, "msl") == 0)
            weather.pressure = p["values"][0];
        else if (strcmp(name, "Wsymb2") == 0)
            weather.symbol = p["values"][0];
    }
    /*// VALIDATE WEATHER
    if (!validateWeather(weather))
    {
        Serial.println("Weather data invalid — discarding");
        http.end();
        return false;
    }*/

    weather.valid = true;
    /*// UPDATE CACHE
    lastWeatherFetch = millis();
    Serial.println("Weather data cached.");
    */
    Serial.printf("Temp: %.1f°C\n", weather.temp);
    Serial.printf("Humidity: %.0f%%\n", weather.humidity);
    Serial.printf("Wind: %.1f m/s\n", weather.wind);
    Serial.printf("Pressure: %.0f hPa\n", weather.pressure);

    http.end();
    return true;
}

// Build the UI
void buildUI()
{
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFFFFFF), 0); // White background

    // Top bar
    lv_obj_t *topBar = lv_obj_create(screen);
    lv_obj_set_size(topBar, LV_PCT(100), 40);
    lv_obj_align(topBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(topBar, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_border_width(topBar, 0, 0);
    lv_obj_set_style_radius(topBar, 0, 0);

    wifiLabel = lv_label_create(topBar);
    lv_label_set_text(wifiLabel, "WiFi: Connecting...");
    lv_obj_set_style_text_font(wifiLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(wifiLabel, lv_color_hex(0x000000), 0);
    lv_obj_align(wifiLabel, LV_ALIGN_LEFT_MID, 20, 0);

    lv_obj_t *title = lv_label_create(topBar);
    lv_label_set_text(title, "Weather Station V1");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
    lv_obj_align(title, LV_ALIGN_RIGHT_MID, -10, 0);

    // Main box
    lv_obj_t *box = lv_obj_create(screen);
    lv_obj_set_size(box, LV_PCT(90), LV_PCT(80));
    lv_obj_align(box, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_color(box, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_border_color(box, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_radius(box, 15, 0);

    // City
    lv_obj_t *city = lv_label_create(box);
    lv_label_set_text(city, "Karlskrona, Sweden");
    lv_obj_set_style_text_font(city, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(city, lv_color_hex(0x000000), 0);
    lv_obj_align(city, LV_ALIGN_TOP_MID, 0, 20);

    // Big temp
    tempLabel = lv_label_create(box);
    lv_label_set_text(tempLabel, "--°C");
    lv_obj_set_style_text_font(tempLabel, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(tempLabel, lv_color_hex(0x000000), 0);
    lv_obj_align(tempLabel, LV_ALIGN_TOP_MID, 0, 60);

    // Status text
    statusLabel = lv_label_create(box);
    lv_label_set_text(statusLabel, "Loading...");
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x666666), 0);
    lv_obj_align(statusLabel, LV_ALIGN_TOP_MID, 0, 130);

    // Bottom info
    humidLabel = lv_label_create(box);
    lv_label_set_text(humidLabel, "Humidity: --%");
    lv_obj_set_style_text_font(humidLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(humidLabel, lv_color_hex(0x000000), 0);
    lv_obj_align(humidLabel, LV_ALIGN_BOTTOM_LEFT, 20, -10);

    windLabel = lv_label_create(box);
    lv_label_set_text(windLabel, "Wind: -- m/s");
    lv_obj_set_style_text_font(windLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(windLabel, lv_color_hex(0x000000), 0);
    lv_obj_align(windLabel, LV_ALIGN_BOTTOM_MID, 0, -10);

    pressLabel = lv_label_create(box);
    lv_label_set_text(pressLabel, "Pressure: ---- hPa");
    lv_obj_set_style_text_font(pressLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(pressLabel, lv_color_hex(0x000000), 0);
    lv_obj_align(pressLabel, LV_ALIGN_BOTTOM_RIGHT, -20, -10);

    Serial.println("UI ready");
}

// Update screen with weather
void updateScreen()
{
    if (!weather.valid)
        return;

    char txt[64];

    // Temp
    snprintf(txt, 64, "%.1f°C", weather.temp);
    lv_label_set_text(tempLabel, txt);

    // Description
    lv_label_set_text(statusLabel, weatherText(weather.symbol));

    // Humidity
    snprintf(txt, 64, "Humidity: %.0f%%", weather.humidity);
    lv_label_set_text(humidLabel, txt);

    // Wind
    snprintf(txt, 64, "Wind: %.1f m/s %s", weather.wind, windDir(weather.windDir));
    lv_label_set_text(windLabel, txt);

    // Pressure
    snprintf(txt, 64, "Pressure: %.0f hPa", weather.pressure);
    lv_label_set_text(pressLabel, txt);

    Serial.println("Screen updated");
}

void setup()
{
    Serial.begin(115200);
    Serial.println("\n=== Weather Station V1 ===\n");

    // Start display
    if (!amoled.begin())
    {
        Serial.println("Display failed!");
        while (1)
            delay(1000);
    }
    Serial.println("Display OK");

    // Start graphics
    beginLvglHelper(amoled);
    Serial.println("Graphics OK");

    // Build UI
    buildUI();
    updateWiFi();

    // Connect
    if (connectWiFi())
    {
        updateWiFi();

        // Get weather
        if (getWeather())
        {
            updateScreen();
            Serial.println("\n=== Ready! ===\n");
        }
        else
        {
            Serial.println("Weather fetch failed!");
            lv_label_set_text(statusLabel, "Failed to get data");
        }
    }
    else
    {
        updateWiFi();
        Serial.println("WiFi failed - check SSID/password");
    }
}

void loop()
{
    lv_task_handler();
    delay(5);
}

/*void loop()
{
    // Auto-reconnect WiFi if lost
    if (WiFi.status() != WL_CONNECTED)
    {
        connectWiFi();
        updateWiFi();
    }

    lv_task_handler();
    delay(5);
}
*/