/**
 * ============================================================================
 * WEATHER STATION APPLICATION FOR ESP32-T4-S3
 * ============================================================================
 *
 * This weather application displays real-time weather data from SMHI.
 * Hardware: LilyGo T4-S3 AMOLED (ESP32-S3 with 600x450 display)
 *
 * Features:
 * - Current weather display with temperature, humidity, wind, and pressure
 * - 3-hour forecast (Morning, Noon, Evening)
 * - 6-day weather forecast
 * - WiFi connectivity with auto-reconnection
 * - Interactive dropdown settings menu
 * - Auto-refresh with configurable intervals
 * - Swipeable pages for different views
 *
 */

// ============================================================================
// SECTION 1: LIBRARY INCLUDES
// ============================================================================
// These libraries provide the functionality we need for the weather station

#include <Arduino.h>          // Basic Arduino functions
#include <WiFi.h>             // WiFi connectivity
#include <WiFiClientSecure.h> // Secure HTTPS connections
#include <HTTPClient.h>       // HTTP requests for API calls
#include <ArduinoJson.h>      // JSON parsing for weather data
#include <LilyGo_AMOLED.h>    // Display driver for T4-S3
#include <LV_Helper.h>        // LVGL helper functions
#include <lvgl.h>             // LVGL graphics library
#include "esp_wifi.h"         // ESP32 WiFi functions
#include "esp_system.h"       // ESP32 system functions
#include "esp_sntp.h"         // Time synchronization
#include "esp_task_wdt.h"     // Watchdog timer
#include "time.h"             // Time functions
#include "icons.h"            // Weather icons

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
// SECTION 4: GLOBAL VARIABLES
// ============================================================================
// Variables that are used throughout the program

// Weather data storage
TodayWeather today = {0};      // Current weather (initialized to zero)
HourlyWeather hourly[3] = {0}; // 3-hour forecasts (Morning, Noon, Evening)
DayWeather days[6] = {0};      // 6-day forecast

// System state flags
volatile bool isFetching = false; // True when fetching data (volatile = can change unexpectedly)
bool timesynced = false;          // True when time is synchronized
bool wifiEnabled = true;          // WiFi on/off state
bool wifiConnecting = false;      // True during WiFi connection
bool autoRefresh = true;          // Auto-refresh on/off
bool dropdownOpen = false;        // Dropdown menu state
bool page1IconsCreated = false;   // Page 1 icons created flag
bool page2IconsCreated = false;   // Page 2 icons created flag

// Timing variables
unsigned long refreshInterval = 15 * 60 * 1000; // 15 minutes in milliseconds
unsigned long lastRefresh = 0;                  // Last refresh timestamp

// Display object
LilyGo_Class amoled; // Display driver instance

// ============================================================================
// SECTION 5: USER INTERFACE OBJECTS
// ============================================================================
// These store references to UI elements so we can update them

// Dropdown menu elements
lv_obj_t *dropdownPanel = NULL;
lv_obj_t *wifiToggleBtn = NULL;
lv_obj_t *autoRefreshToggleBtn = NULL;
lv_obj_t *intervalDropdown = NULL;
lv_obj_t *wifiStatusLabel = NULL;
lv_obj_t *wifiSignalLabel = NULL;
lv_obj_t *wifiDetailsLabel = NULL;
lv_obj_t *wifiIcon = NULL;
lv_obj_t *timeLabel = NULL;

// Page 1 (Current Weather) UI elements
lv_obj_t *cityLabel = NULL;
lv_obj_t *page1DayNameLabel = NULL;
lv_obj_t *page1DateLabel = NULL;
lv_obj_t *page1TempLabel = NULL;
lv_obj_t *page1WeatherIconContainer = NULL;
lv_obj_t *page1StatusLabel = NULL;
lv_obj_t *page1HumLabel = NULL;
lv_obj_t *page1WindLabel = NULL;
lv_obj_t *page1PressLabel = NULL;
lv_obj_t *page1LoadingSpinner = NULL;
lv_obj_t *page1LoadingLabel = NULL;
lv_obj_t *refreshBtn = NULL;
lv_obj_t *updateLabel = NULL;

// Arrays for hourly forecast elements
lv_obj_t *hourlyTempLabels[3] = {NULL};
lv_obj_t *hourlyTimeLabels[3] = {NULL};
lv_obj_t *hourlyWindLabels[3] = {NULL};
lv_obj_t *hourlyRainLabels[3] = {NULL};
lv_obj_t *hourlyStatusLabels[3] = {NULL};
lv_obj_t *hourlyIconContainers[3] = {NULL};

// Page 2 (6-Day Forecast) UI elements
lv_obj_t *dayNameLabels[6] = {NULL};
lv_obj_t *dayDateLabels[6] = {NULL};
lv_obj_t *dayTempLabels[6] = {NULL};
lv_obj_t *dayRainLabels[6] = {NULL};
lv_obj_t *dayStatusLabels[6] = {NULL};
lv_obj_t *dayIconContainers[6] = {NULL};
lv_obj_t *page2LoadingSpinner = NULL;
lv_obj_t *page2LoadingLabel = NULL;

// ============================================================================
// SECTION 6: HELPER FUNCTIONS
// ============================================================================
// Small functions that help with common tasks

/**
 * Converts weather symbol code to description text
 * @param symbolCode Weather symbol code (1-27)
 * @return Description string like "Clear", "Cloudy", etc.
 */
const char *getWeatherDescription(int symbolCode)
{
    // Check if symbol code is valid
    if (symbolCode < 1 || symbolCode > 27)
    {
        return "Unknown";
    }

    // Array of weather descriptions matching SMHI symbol codes
    const char *descriptions[] = {
        "Clear",                  // 1
        "Nearly clear",           // 2
        "Variable clouds",        // 3
        "Half-clear",             // 4
        "Cloudy",                 // 5
        "Overcast",               // 6
        "Fog",                    // 7
        "Light rain showers",     // 8
        "Moderate rain showers",  // 9
        "Heavy rain showers",     // 10
        "Thunder",                // 11
        "Light sleet showers",    // 12
        "Moderate sleet showers", // 13
        "Heavy sleet showers",    // 14
        "Light snow showers",     // 15
        "Moderate snow showers",  // 16
        "Heavy snow showers",     // 17
        "Light rain",             // 18
        "Moderate rain",          // 19
        "Heavy rain",             // 20
        "Thunder",                // 21
        "Light sleet",            // 22
        "Moderate sleet",         // 23
        "Heavy sleet",            // 24
        "Light snow",             // 25
        "Moderate snow",          // 26
        "Heavy snow"              // 27
    };

    // Return the description (subtract 1 because array starts at 0)
    return descriptions[symbolCode - 1];
}

/**
 * Converts wind direction in degrees to compass direction
 * @param degrees Wind direction in degrees (0-359)
 * @return Compass direction like "N", "NE", "E", etc.
 */
const char *getWindDirection(int degrees)
{
    // Array of compass directions
    const char *directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};

    // Calculate which direction (each covers 45 degrees)
    // Add 23 to round to nearest direction
    int index = ((degrees + 23) / 45) % 8;

    return directions[index];
}

/**
 * Converts WiFi signal strength (RSSI) to percentage
 * @param rssi Signal strength in dBm (negative value)
 * @return Signal strength as percentage (0-100)
 */
int convertRssiToPercent(int rssi)
{
    // -50 dBm or better = 100%
    if (rssi >= -50)
        return 100;

    // -100 dBm or worse = 0%
    if (rssi <= -100)
        return 0;

    // Linear conversion between -50 and -100
    return 2 * (rssi + 100);
}

/**
 * Gets signal strength description from RSSI value
 * @param rssi Signal strength in dBm
 * @return Description like "Excellent", "Good", etc.
 */
const char *getSignalStrengthText(int rssi)
{
    int percent = convertRssiToPercent(rssi);

    if (percent >= 80)
        return "Excellent";
    if (percent >= 60)
        return "Good";
    if (percent >= 40)
        return "Fair";
    if (percent >= 20)
        return "Weak";
    return "Very Weak";
}

/**
 * Calculates time since last update
 * @return String like "5m", "2h", or "Never"
 */
const char *getTimeSinceUpdate()
{
    static char timeBuf[16]; // Static = keeps value between calls

    // Check if we've never refreshed
    if (lastRefresh == 0)
    {
        return "Never";
    }

    // Calculate seconds since last refresh
    unsigned long seconds = (millis() - lastRefresh) / 1000;

    if (seconds < 60)
    {
        return "Now"; // Less than a minute
    }
    else if (seconds < 3600)
    {
        // Show minutes
        snprintf(timeBuf, sizeof(timeBuf), "%lum", seconds / 60);
    }
    else
    {
        // Show hours
        snprintf(timeBuf, sizeof(timeBuf), "%luh", seconds / 3600);
    }

    return timeBuf;
}

/**
 * Calculates day of week from date string using Zeller's algorithm
 * @param dateStr Date string in format "YYYY-MM-DD"
 * @param dayName Buffer to store the day name
 * @param size Size of the buffer
 */
void calculateDayOfWeek(const char *dateStr, char *dayName, size_t size)
{
    int year, month, day;

    // Parse the date string
    if (sscanf(dateStr, "%d-%d-%d", &year, &month, &day) != 3)
    {
        strncpy(dayName, "Unknown", size - 1);
        dayName[size - 1] = '\0'; // Ensure null termination
        return;
    }

    // Zeller's algorithm for day of week calculation
    // Adjust month and year for algorithm
    if (month < 3)
    {
        month = month + 12;
        year = year - 1;
    }

    // Apply Zeller's formula
    int k = year % 100; // Year of century
    int j = year / 100; // Century
    int h = (day + 13 * (month + 1) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;

    // Day names array
    const char *dayNames[] = {
        "Saturday", "Sunday", "Monday", "Tuesday",
        "Wednesday", "Thursday", "Friday"};

    // Copy day name to buffer
    strncpy(dayName, dayNames[h], size - 1);
    dayName[size - 1] = '\0'; // Ensure null termination
}

/**
 * Formats date string to short format
 * @param dateStr Date string "YYYY-MM-DD"
 * @param shortDate Buffer for short format "Mon DD"
 * @param size Buffer size
 */
void formatDateShort(const char *dateStr, char *shortDate, size_t size)
{
    int year, month, day;

    // Parse the date
    if (sscanf(dateStr, "%d-%d-%d", &year, &month, &day) != 3)
    {
        strncpy(shortDate, dateStr, size - 1);
        shortDate[size - 1] = '\0';
        return;
    }

    // Month names array
    const char *months[] = {
        "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    // Format the short date
    if (month >= 1 && month <= 12)
    {
        snprintf(shortDate, size, "%s %d", months[month], day);
    }
    else
    {
        strncpy(shortDate, dateStr, size - 1);
    }
}

// ============================================================================
// SECTION 7: TIME SYNCHRONIZATION FUNCTIONS
// ============================================================================
// Functions for keeping accurate time

/**
 * Callback function called when time is synchronized
 * @param tv Time value structure
 */
void onTimeSync(struct timeval *tv)
{
    timesynced = true;
    Serial.println("Time synchronized successfully");
}

/**
 * Initializes time synchronization with NTP servers
 */
void initializeTimeSync()
{
    // Set timezone
    setenv("TZ", TZ_INFO, 1);
    tzset();

    // Set callback for when time syncs
    sntp_set_time_sync_notification_cb(onTimeSync);

    // Configure NTP client
    sntp_setoperatingmode(SNTP_OPMODE_POLL);

    // Set multiple NTP servers for reliability
    sntp_setservername(0, "time.google.com");
    sntp_setservername(1, "pool.ntp.org");
    sntp_setservername(2, "time.cloudflare.com");

    // Start NTP synchronization
    sntp_init();

    Serial.println("Time sync initialized");
}

/**
 * Updates the time display in the status bar
 */
void updateTimeDisplay()
{
    // Check if we have a time label
    if (timeLabel == NULL)
        return;

    // Check if time is synchronized
    if (!timesynced)
    {
        lv_label_set_text(timeLabel, "--:-- --");
        return;
    }

    // Get current time
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    // Verify we have a valid year (should be 2024 or later)
    if (timeinfo.tm_year < (2024 - 1900))
    {
        lv_label_set_text(timeLabel, "--:-- --");
        return;
    }

    // Convert to 12-hour format
    char timeStr[16];
    int hour = timeinfo.tm_hour;
    const char *period = "AM";

    // Handle 12-hour conversion
    if (hour == 0)
    {
        hour = 12; // Midnight is 12 AM
    }
    else if (hour == 12)
    {
        period = "PM"; // Noon is 12 PM
    }
    else if (hour > 12)
    {
        hour = hour - 12;
        period = "PM";
    }

    // Format time string
    snprintf(timeStr, sizeof(timeStr), "%d:%02d %s",
             hour, timeinfo.tm_min, period);

    // Update label
    lv_label_set_text(timeLabel, timeStr);
}

// ============================================================================
// SECTION 8: LOADING SCREEN MANAGEMENT
// ============================================================================
// Functions to show/hide loading indicators

/**
 * Shows or hides loading spinners
 * @param show True to show, false to hide
 */
void setLoadingVisible(bool show)
{
    // Handle Page 1 loading elements
    if (page1LoadingSpinner != NULL && page1LoadingLabel != NULL)
    {
        if (show)
        {
            lv_obj_clear_flag(page1LoadingSpinner, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(page1LoadingLabel, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(page1LoadingSpinner, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(page1LoadingLabel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Handle Page 2 loading elements
    if (page2LoadingSpinner != NULL && page2LoadingLabel != NULL)
    {
        if (show)
        {
            lv_obj_clear_flag(page2LoadingSpinner, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(page2LoadingLabel, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(page2LoadingSpinner, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(page2LoadingLabel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Update display
    lv_timer_handler();
}

/**
 * Updates loading message text
 * @param message Text to display
 */
void updateLoadingMessage(const char *message)
{
    // Check for null message
    if (message == NULL)
        return;

    // Update page 1 loading text
    if (page1LoadingLabel != NULL)
    {
        lv_label_set_text(page1LoadingLabel, message);
    }

    // Update page 2 loading text
    if (page2LoadingLabel != NULL)
    {
        lv_label_set_text(page2LoadingLabel, message);
    }

    // Update display
    lv_timer_handler();
}

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

// ============================================================================
// SECTION 10: USER INTERFACE UPDATE FUNCTIONS
// ============================================================================
// Functions that update the display with new data

/**
 * Updates Page 1 with current weather and hourly forecast
 */
void updateCurrentWeatherDisplay()
{
    Serial.println("Updating current weather display...");

    // Reset icon flag to allow icon updates on refresh
    page1IconsCreated = false;

    // Hide loading indicators
    setLoadingVisible(false);

    // Show main UI elements
    if (cityLabel)
        lv_obj_clear_flag(cityLabel, LV_OBJ_FLAG_HIDDEN);
    if (page1DayNameLabel)
        lv_obj_clear_flag(page1DayNameLabel, LV_OBJ_FLAG_HIDDEN);
    if (page1DateLabel)
        lv_obj_clear_flag(page1DateLabel, LV_OBJ_FLAG_HIDDEN);
    if (page1WeatherIconContainer)
        lv_obj_clear_flag(page1WeatherIconContainer, LV_OBJ_FLAG_HIDDEN);
    if (page1TempLabel)
        lv_obj_clear_flag(page1TempLabel, LV_OBJ_FLAG_HIDDEN);
    if (page1StatusLabel)
        lv_obj_clear_flag(page1StatusLabel, LV_OBJ_FLAG_HIDDEN);
    if (page1HumLabel)
        lv_obj_clear_flag(lv_obj_get_parent(page1HumLabel), LV_OBJ_FLAG_HIDDEN);
    if (updateLabel)
        lv_obj_clear_flag(updateLabel, LV_OBJ_FLAG_HIDDEN);
    if (refreshBtn)
        lv_obj_clear_flag(refreshBtn, LV_OBJ_FLAG_HIDDEN);

    // Update current date and day name
    if (page1DayNameLabel && page1DateLabel && timesynced)
    {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);

        if (timeinfo.tm_year >= (2024 - 1900))
        {
            // Day names array
            const char *dayNames[] = {
                "Sunday", "Monday", "Tuesday", "Wednesday",
                "Thursday", "Friday", "Saturday"};
            lv_label_set_text(page1DayNameLabel, dayNames[timeinfo.tm_wday]);

            // Month names
            const char *months[] = {
                "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

            char dateStr[16];
            snprintf(dateStr, sizeof(dateStr), "%s %d",
                     months[timeinfo.tm_mon + 1], timeinfo.tm_mday);
            lv_label_set_text(page1DateLabel, dateStr);
        }
    }

    // Update current weather if valid
    if (today.valid && page1TempLabel)
    {
        char buffer[32];

        // Temperature
        snprintf(buffer, sizeof(buffer), "%.1f°C", today.temp);
        lv_label_set_text(page1TempLabel, buffer);

        // Humidity
        if (page1HumLabel)
        {
            snprintf(buffer, sizeof(buffer), "H: %.0f%%", today.humidity);
            lv_label_set_text(page1HumLabel, buffer);
        }

        // Wind
        if (page1WindLabel)
        {
            snprintf(buffer, sizeof(buffer), "W: %.1f m/s %s",
                     today.wind, getWindDirection(today.windDir));
            lv_label_set_text(page1WindLabel, buffer);
        }

        // Pressure
        if (page1PressLabel)
        {
            snprintf(buffer, sizeof(buffer), "P: %.0f hPa", today.pressure);
            lv_label_set_text(page1PressLabel, buffer);
        }

        // Weather icon
        if (page1WeatherIconContainer && !page1IconsCreated)
        {
            lv_obj_clean(page1WeatherIconContainer);
            createWeatherIconBySymbol(page1WeatherIconContainer, today.symbol);
        }

        // Weather description
        if (page1StatusLabel)
        {
            lv_label_set_text(page1StatusLabel, getWeatherDescription(today.symbol));
        }

        // Update time
        if (updateLabel)
        {
            lv_label_set_text(updateLabel, "Updated: Now");
        }
    }

    // Update hourly forecasts
    for (int i = 0; i < 3; i++)
    {
        // Show hourly elements
        if (hourlyIconContainers[i])
            lv_obj_clear_flag(hourlyIconContainers[i], LV_OBJ_FLAG_HIDDEN);
        if (hourlyStatusLabels[i])
            lv_obj_clear_flag(hourlyStatusLabels[i], LV_OBJ_FLAG_HIDDEN);
        if (hourlyTimeLabels[i])
            lv_obj_clear_flag(hourlyTimeLabels[i], LV_OBJ_FLAG_HIDDEN);
        if (hourlyTempLabels[i])
            lv_obj_clear_flag(hourlyTempLabels[i], LV_OBJ_FLAG_HIDDEN);
        if (hourlyWindLabels[i])
            lv_obj_clear_flag(hourlyWindLabels[i], LV_OBJ_FLAG_HIDDEN);
        if (hourlyRainLabels[i])
            lv_obj_clear_flag(hourlyRainLabels[i], LV_OBJ_FLAG_HIDDEN);

        // Update with data if valid
        if (hourly[i].valid)
        {
            char buffer[32];

            // Time label
            if (hourlyTimeLabels[i])
            {
                lv_label_set_text(hourlyTimeLabels[i], hourly[i].time);
            }

            // Temperature
            if (hourlyTempLabels[i])
            {
                snprintf(buffer, sizeof(buffer), "%.0f°C", hourly[i].temp);
                lv_label_set_text(hourlyTempLabels[i], buffer);
            }

            // Wind
            if (hourlyWindLabels[i])
            {
                snprintf(buffer, sizeof(buffer), "%.1f m/s %s",
                         hourly[i].wind, getWindDirection(hourly[i].windDir));
                lv_label_set_text(hourlyWindLabels[i], buffer);
            }

            // Rain chance
            if (hourlyRainLabels[i])
            {
                snprintf(buffer, sizeof(buffer), "R: %.0f%%", hourly[i].rainChance);
                lv_label_set_text(hourlyRainLabels[i], buffer);
            }

            // Description
            if (hourlyStatusLabels[i])
            {
                lv_label_set_text(hourlyStatusLabels[i],
                                  getWeatherDescription(hourly[i].symbol));
            }

            // Weather icon
            if (hourlyIconContainers[i] && !page1IconsCreated)
            {
                lv_obj_clean(hourlyIconContainers[i]);
                createWeatherIconBySymbol(hourlyIconContainers[i], hourly[i].symbol);
            }
        }
    }

    page1IconsCreated = true;
    lv_timer_handler();
}

/**
 * Updates Page 2 with 6-day forecast
 */
void updateForecastDisplay()
{
    Serial.println("Updating forecast display...");

    // Reset icon flag to allow icon updates on refresh
    page2IconsCreated = false;

    // Update each day's forecast
    for (int i = 0; i < 6; i++)
    {
        // Show elements
        if (dayNameLabels[i])
            lv_obj_clear_flag(dayNameLabels[i], LV_OBJ_FLAG_HIDDEN);
        if (dayDateLabels[i])
            lv_obj_clear_flag(dayDateLabels[i], LV_OBJ_FLAG_HIDDEN);
        if (dayTempLabels[i])
            lv_obj_clear_flag(dayTempLabels[i], LV_OBJ_FLAG_HIDDEN);
        if (dayRainLabels[i])
            lv_obj_clear_flag(dayRainLabels[i], LV_OBJ_FLAG_HIDDEN);
        if (dayStatusLabels[i])
            lv_obj_clear_flag(dayStatusLabels[i], LV_OBJ_FLAG_HIDDEN);
        if (dayIconContainers[i])
            lv_obj_clear_flag(dayIconContainers[i], LV_OBJ_FLAG_HIDDEN);

        // Update with data if valid
        if (days[i].valid)
        {
            char buffer[32];
            char shortDate[16];

            // Day name
            if (dayNameLabels[i])
            {
                lv_label_set_text(dayNameLabels[i], days[i].dayName);
            }

            // Date
            if (dayDateLabels[i])
            {
                formatDateShort(days[i].date, shortDate, sizeof(shortDate));
                lv_label_set_text(dayDateLabels[i], shortDate);
            }

            // Temperature range
            if (dayTempLabels[i])
            {
                snprintf(buffer, sizeof(buffer), "%.0f/%.0f°C",
                         days[i].tempMax, days[i].tempMin);
                lv_label_set_text(dayTempLabels[i], buffer);
            }

            // Rain chance
            if (dayRainLabels[i])
            {
                snprintf(buffer, sizeof(buffer), "R: %.0f%%", days[i].rainChance);
                lv_label_set_text(dayRainLabels[i], buffer);
            }

            // Weather icon
            if (dayIconContainers[i] && !page2IconsCreated)
            {
                lv_obj_clean(dayIconContainers[i]);
                createWeatherIconBySymbol(dayIconContainers[i], days[i].symbol);
            }

            // Description
            if (dayStatusLabels[i])
            {
                lv_label_set_text(dayStatusLabels[i],
                                  getWeatherDescription(days[i].symbol));
            }
        }
    }

    page2IconsCreated = true;
    lv_timer_handler();
}

// ============================================================================
// SECTION 11: DROPDOWN MENU FUNCTIONS
// ============================================================================
// Functions for the settings dropdown menu

// Forward declaration
void updateDropdownContent();

/**
 * Toggles dropdown menu visibility with animation
 */
void toggleDropdownMenu()
{
    if (dropdownPanel == NULL)
        return;

    dropdownOpen = !dropdownOpen;

    if (dropdownOpen)
    {
        // Show dropdown with slide animation
        lv_obj_clear_flag(dropdownPanel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_y(dropdownPanel, -300); // Start above screen

        // Create slide down animation
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, dropdownPanel);
        lv_anim_set_values(&anim, -300, 0); // Slide from -300 to 0
        lv_anim_set_time(&anim, 300);       // 300ms duration
        lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_y);
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
        lv_anim_start(&anim);

        updateDropdownContent();
    }
    else
    {
        // Hide dropdown with slide animation
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, dropdownPanel);
        lv_anim_set_values(&anim, 0, -300); // Slide from 0 to -300
        lv_anim_set_time(&anim, 300);
        lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_y);
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_in);

        // Hide after animation completes
        lv_anim_set_ready_cb(&anim, [](lv_anim_t *a)
                             { lv_obj_add_flag((lv_obj_t *)a->var, LV_OBJ_FLAG_HIDDEN); });

        lv_anim_start(&anim);
    }
}

/**
 * Event handler for status bar click
 */
void handleStatusBarClick(lv_event_t *e)
{
    toggleDropdownMenu();
}

/**
 * Event handler for WiFi toggle button
 */
void handleWifiToggle(lv_event_t *e)
{
    wifiEnabled = !wifiEnabled;

    if (wifiEnabled)
    {
        // Turn WiFi ON
        lv_label_set_text(lv_obj_get_child(wifiToggleBtn, 0), "WiFi: ON");
        lv_obj_set_style_bg_color(wifiToggleBtn, lv_color_hex(0x4CAF50), 0);
        wifiConnecting = true;
        updateDropdownContent();

        // Start WiFi
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
    else
    {
        // Turn WiFi OFF
        lv_label_set_text(lv_obj_get_child(wifiToggleBtn, 0), "WiFi: OFF");
        lv_obj_set_style_bg_color(wifiToggleBtn, lv_color_hex(0x666666), 0);
        wifiConnecting = false;

        // Stop WiFi
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }

    updateDropdownContent();
}

/**
 * Event handler for auto-refresh toggle
 */
void handleAutoRefreshToggle(lv_event_t *e)
{
    autoRefresh = !autoRefresh;

    if (autoRefresh)
    {
        lv_label_set_text(lv_obj_get_child(autoRefreshToggleBtn, 0), "Auto: ON");
        lv_obj_set_style_bg_color(autoRefreshToggleBtn, lv_color_hex(0x4CAF50), 0);
    }
    else
    {
        lv_label_set_text(lv_obj_get_child(autoRefreshToggleBtn, 0), "Auto: OFF");
        lv_obj_set_style_bg_color(autoRefreshToggleBtn, lv_color_hex(0x666666), 0);
    }
}

/**
 * Event handler for refresh interval change
 */
void handleIntervalChange(lv_event_t *e)
{
    uint16_t selection = lv_dropdown_get_selected(intervalDropdown);

    // Set interval based on selection
    switch (selection)
    {
    case 0:
        refreshInterval = 5 * 60 * 1000;
        break; // 5 minutes
    case 1:
        refreshInterval = 10 * 60 * 1000;
        break; // 10 minutes
    case 2:
        refreshInterval = 15 * 60 * 1000;
        break; // 15 minutes
    case 3:
        refreshInterval = 30 * 60 * 1000;
        break; // 30 minutes
    case 4:
        refreshInterval = 60 * 60 * 1000;
        break; // 60 minutes
    default:
        refreshInterval = 15 * 60 * 1000;
        break;
    }
}

/**
 * Updates dropdown menu content with current status
 */
void updateDropdownContent()
{
    if (!dropdownPanel || !dropdownOpen)
        return;

    // Update WiFi status
    if (wifiStatusLabel)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            char statusText[64];
            snprintf(statusText, sizeof(statusText), "Connected to: %s", WIFI_SSID);
            lv_label_set_text(wifiStatusLabel, statusText);
        }
        else if (wifiConnecting)
        {
            lv_label_set_text(wifiStatusLabel, "Connecting...");
        }
        else if (wifiEnabled)
        {
            lv_label_set_text(wifiStatusLabel, "Disconnected");
        }
        else
        {
            lv_label_set_text(wifiStatusLabel, "WiFi Off");
        }
    }

    // Update signal strength
    if (wifiSignalLabel)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            int rssi = WiFi.RSSI();
            int percent = convertRssiToPercent(rssi);
            char signalText[64];
            snprintf(signalText, sizeof(signalText), "Signal: %d%% (%s)",
                     percent, getSignalStrengthText(rssi));
            lv_label_set_text(wifiSignalLabel, signalText);
        }
        else
        {
            lv_label_set_text(wifiSignalLabel, "Signal: --");
        }
    }

    // Update connection details
    if (wifiDetailsLabel)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            char detailsText[128];
            IPAddress ip = WiFi.localIP();

            if (ip[0] != 0)
            {
                snprintf(detailsText, sizeof(detailsText),
                         "IP: %d.%d.%d.%d\nMAC: %s\nChannel: %d",
                         ip[0], ip[1], ip[2], ip[3],
                         WiFi.macAddress().c_str(),
                         WiFi.channel());
                lv_label_set_text(wifiDetailsLabel, detailsText);
            }
            else
            {
                lv_label_set_text(wifiDetailsLabel, "Getting IP address...");
            }
        }
        else
        {
            lv_label_set_text(wifiDetailsLabel, "No connection");
        }
    }

    lv_timer_handler();
}

// ============================================================================
// SECTION 12: USER INTERFACE CREATION
// ============================================================================
// Functions to create all UI elements

/**
 * Event handler for refresh button
 */
void handleRefreshButton(lv_event_t *e)
{
    if (e == NULL)
        return;

    // Check if already fetching
    if (isFetching)
    {
        Serial.println("Already fetching - button ignored");
        return;
    }

    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("No WiFi - button ignored");
        return;
    }

    Serial.println("Refresh button clicked");
    fetchWeatherData();
}

/**
 * Creates Page 1 (Current Weather) UI elements
 */
void createCurrentWeatherPage(lv_obj_t *page1)
{
    // City name label
    cityLabel = lv_label_create(page1);
    lv_label_set_text(cityLabel, "Karlskrona");
    lv_obj_set_style_text_font(cityLabel, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(cityLabel, lv_color_hex(0x2196F3), 0);
    lv_obj_align(cityLabel, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_add_flag(cityLabel, LV_OBJ_FLAG_HIDDEN);

    // Day name label
    page1DayNameLabel = lv_label_create(page1);
    lv_label_set_text(page1DayNameLabel, "---");
    lv_obj_set_style_text_color(page1DayNameLabel, lv_color_white(), 0);
    lv_obj_set_style_text_font(page1DayNameLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(page1DayNameLabel, 110, 105);
    lv_obj_add_flag(page1DayNameLabel, LV_OBJ_FLAG_HIDDEN);

    // Date label
    page1DateLabel = lv_label_create(page1);
    lv_label_set_text(page1DateLabel, "--/--");
    lv_obj_set_style_text_color(page1DateLabel, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(page1DateLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(page1DateLabel, 110, 127);
    lv_obj_add_flag(page1DateLabel, LV_OBJ_FLAG_HIDDEN);

    // Weather icon container
    page1WeatherIconContainer = lv_obj_create(page1);
    lv_obj_set_size(page1WeatherIconContainer, 100, 100);
    lv_obj_set_style_bg_opa(page1WeatherIconContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page1WeatherIconContainer, 0, 0);
    lv_obj_set_style_pad_all(page1WeatherIconContainer, 0, 0);
    lv_obj_clear_flag(page1WeatherIconContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(page1WeatherIconContainer, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_clip_corner(page1WeatherIconContainer, false, 0);
    lv_obj_align(page1WeatherIconContainer, LV_ALIGN_TOP_MID, -50, 70);

    // Temperature label
    page1TempLabel = lv_label_create(page1);
    lv_label_set_text(page1TempLabel, "--°C");
    lv_obj_set_style_text_font(page1TempLabel, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(page1TempLabel, lv_color_white(), 0);
    lv_obj_align(page1TempLabel, LV_ALIGN_TOP_MID, 80, 95);

    // Weather status label
    page1StatusLabel = lv_label_create(page1);
    lv_label_set_text(page1StatusLabel, "Connecting...");
    lv_obj_set_style_text_font(page1StatusLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(page1StatusLabel, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_pos(page1StatusLabel, 205, 148);

    // Loading spinner
    page1LoadingSpinner = lv_spinner_create(page1, 1000, 60);
    lv_obj_set_size(page1LoadingSpinner, 80, 80);
    lv_obj_align(page1LoadingSpinner, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_arc_color(page1LoadingSpinner, lv_color_hex(0x2196F3), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(page1LoadingSpinner, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(page1LoadingSpinner, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_arc_width(page1LoadingSpinner, 6, LV_PART_MAIN);

    // Loading message label
    page1LoadingLabel = lv_label_create(page1);
    lv_label_set_text(page1LoadingLabel, "Connecting to WiFi...");
    lv_obj_set_style_text_font(page1LoadingLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(page1LoadingLabel, lv_color_hex(0x2196F3), 0);
    lv_obj_align(page1LoadingLabel, LV_ALIGN_CENTER, 0, 70);

    // Hide main elements initially
    lv_obj_add_flag(page1WeatherIconContainer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(page1TempLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(page1StatusLabel, LV_OBJ_FLAG_HIDDEN);

    // Create container for weather details (humidity, wind, pressure)
    lv_obj_t *detailsRow = lv_obj_create(page1);
    lv_obj_set_size(detailsRow, 500, 40);
    lv_obj_set_style_bg_opa(detailsRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(detailsRow, 0, 0);
    lv_obj_set_flex_flow(detailsRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(detailsRow, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(detailsRow, LV_ALIGN_TOP_MID, 0, 175);

    // Humidity label
    page1HumLabel = lv_label_create(detailsRow);
    lv_label_set_text(page1HumLabel, "H: --%");
    lv_obj_set_style_text_color(page1HumLabel, lv_color_hex(0x64B5F6), 0);
    lv_obj_set_style_text_font(page1HumLabel, &lv_font_montserrat_18, 0);

    // Wind label
    page1WindLabel = lv_label_create(detailsRow);
    lv_label_set_text(page1WindLabel, "W: -- m/s");
    lv_obj_set_style_text_color(page1WindLabel, lv_color_hex(0x81C784), 0);
    lv_obj_set_style_text_font(page1WindLabel, &lv_font_montserrat_18, 0);

    // Pressure label
    page1PressLabel = lv_label_create(detailsRow);
    lv_label_set_text(page1PressLabel, "P: -- hPa");
    lv_obj_set_style_text_color(page1PressLabel, lv_color_hex(0xFFB74D), 0);
    lv_obj_set_style_text_font(page1PressLabel, &lv_font_montserrat_18, 0);

    lv_obj_add_flag(detailsRow, LV_OBJ_FLAG_HIDDEN);

    // Create hourly forecast elements
    for (int i = 0; i < 3; i++)
    {
        int xPosition = 70 + (i * 170);

        // Icon container
        hourlyIconContainers[i] = lv_obj_create(page1);
        lv_obj_set_size(hourlyIconContainers[i], 80, 80);
        lv_obj_set_style_bg_opa(hourlyIconContainers[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(hourlyIconContainers[i], 0, 0);
        lv_obj_set_style_pad_all(hourlyIconContainers[i], 0, 0);
        lv_obj_clear_flag(hourlyIconContainers[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(hourlyIconContainers[i], LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_clip_corner(hourlyIconContainers[i], false, 0);
        lv_obj_remove_style(hourlyIconContainers[i], NULL, LV_PART_SCROLLBAR);
        lv_obj_set_pos(hourlyIconContainers[i], xPosition, 240);

        // Status label
        hourlyStatusLabels[i] = lv_label_create(page1);
        lv_label_set_text(hourlyStatusLabels[i], "---");
        lv_obj_set_style_text_font(hourlyStatusLabels[i], &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(hourlyStatusLabels[i], lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_pos(hourlyStatusLabels[i], xPosition + 5, 323);

        // Time label
        hourlyTimeLabels[i] = lv_label_create(page1);
        lv_label_set_text(hourlyTimeLabels[i], "--:--");
        lv_obj_set_style_text_color(hourlyTimeLabels[i], lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(hourlyTimeLabels[i], &lv_font_montserrat_12, 0);
        lv_obj_set_pos(hourlyTimeLabels[i], xPosition + 85, 245);

        // Temperature label
        hourlyTempLabels[i] = lv_label_create(page1);
        lv_label_set_text(hourlyTempLabels[i], "--°");
        lv_obj_set_style_text_color(hourlyTempLabels[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(hourlyTempLabels[i], &lv_font_montserrat_20, 0);
        lv_obj_set_pos(hourlyTempLabels[i], xPosition + 85, 265);

        // Wind label
        hourlyWindLabels[i] = lv_label_create(page1);
        lv_label_set_text(hourlyWindLabels[i], "-- m/s");
        lv_obj_set_style_text_color(hourlyWindLabels[i], lv_color_hex(0x81C784), 0);
        lv_obj_set_style_text_font(hourlyWindLabels[i], &lv_font_montserrat_12, 0);
        lv_obj_set_pos(hourlyWindLabels[i], xPosition + 85, 293);

        // Rain label
        hourlyRainLabels[i] = lv_label_create(page1);
        lv_label_set_text(hourlyRainLabels[i], "R: --%");
        lv_obj_set_style_text_color(hourlyRainLabels[i], lv_color_hex(0x64B5F6), 0);
        lv_obj_set_style_text_font(hourlyRainLabels[i], &lv_font_montserrat_12, 0);
        lv_obj_set_pos(hourlyRainLabels[i], xPosition + 85, 310);

        // Hide initially
        lv_obj_add_flag(hourlyIconContainers[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(hourlyStatusLabels[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(hourlyTimeLabels[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(hourlyTempLabels[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(hourlyWindLabels[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(hourlyRainLabels[i], LV_OBJ_FLAG_HIDDEN);
    }

    // Update time label
    updateLabel = lv_label_create(page1);
    lv_label_set_text(updateLabel, "Updated: Never");
    lv_obj_set_style_text_color(updateLabel, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(updateLabel, &lv_font_montserrat_12, 0);
    lv_obj_align(updateLabel, LV_ALIGN_BOTTOM_LEFT, 5, -40);
    lv_obj_add_flag(updateLabel, LV_OBJ_FLAG_HIDDEN);

    // Refresh button
    refreshBtn = lv_btn_create(page1);
    lv_obj_set_size(refreshBtn, 180, 35);
    lv_obj_align(refreshBtn, LV_ALIGN_BOTTOM_MID, 0, -3);
    lv_obj_set_style_bg_color(refreshBtn, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_radius(refreshBtn, 10, 0);
    lv_obj_add_event_cb(refreshBtn, handleRefreshButton, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(refreshBtn, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *refreshBtnLabel = lv_label_create(refreshBtn);
    lv_label_set_text(refreshBtnLabel, "Refresh");
    lv_obj_set_style_text_color(refreshBtnLabel, lv_color_white(), 0);
    lv_obj_center(refreshBtnLabel);
}

/**
 * Creates Page 2 (6-Day Forecast) UI elements
 */
void createForecastPage(lv_obj_t *page2)
{
    // Loading spinner
    page2LoadingSpinner = lv_spinner_create(page2, 1000, 60);
    lv_obj_set_size(page2LoadingSpinner, 80, 80);
    lv_obj_align(page2LoadingSpinner, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_arc_color(page2LoadingSpinner, lv_color_hex(0x2196F3), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(page2LoadingSpinner, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(page2LoadingSpinner, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_arc_width(page2LoadingSpinner, 6, LV_PART_MAIN);

    // Loading message
    page2LoadingLabel = lv_label_create(page2);
    lv_label_set_text(page2LoadingLabel, "Connecting to WiFi...");
    lv_obj_set_style_text_font(page2LoadingLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(page2LoadingLabel, lv_color_hex(0x2196F3), 0);
    lv_obj_align(page2LoadingLabel, LV_ALIGN_CENTER, 0, 70);

    // Create 6-day forecast elements
    for (int i = 0; i < 6; i++)
    {
        int yPosition = 70 + (i * 60);

        // Day name label
        dayNameLabels[i] = lv_label_create(page2);
        lv_label_set_text(dayNameLabels[i], "---");
        lv_obj_set_style_text_color(dayNameLabels[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(dayNameLabels[i], &lv_font_montserrat_16, 0);
        lv_obj_set_pos(dayNameLabels[i], 20, yPosition);

        // Date label
        dayDateLabels[i] = lv_label_create(page2);
        lv_label_set_text(dayDateLabels[i], "--/--");
        lv_obj_set_style_text_color(dayDateLabels[i], lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(dayDateLabels[i], &lv_font_montserrat_12, 0);
        lv_obj_set_pos(dayDateLabels[i], 20, yPosition + 22);

        // Temperature label
        dayTempLabels[i] = lv_label_create(page2);
        lv_label_set_text(dayTempLabels[i], "--/--°");
        lv_obj_set_style_text_color(dayTempLabels[i], lv_color_hex(0xFFB74D), 0);
        lv_obj_set_style_text_font(dayTempLabels[i], &lv_font_montserrat_16, 0);
        lv_obj_set_pos(dayTempLabels[i], 150, yPosition + 10);

        // Rain label
        dayRainLabels[i] = lv_label_create(page2);
        lv_label_set_text(dayRainLabels[i], "R: --%");
        lv_obj_set_style_text_color(dayRainLabels[i], lv_color_hex(0x64B5F6), 0);
        lv_obj_set_style_text_font(dayRainLabels[i], &lv_font_montserrat_16, 0);
        lv_obj_set_pos(dayRainLabels[i], 245, yPosition + 10);

        // Status label
        dayStatusLabels[i] = lv_label_create(page2);
        lv_label_set_text(dayStatusLabels[i], "---");
        lv_obj_set_style_text_color(dayStatusLabels[i], lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_text_font(dayStatusLabels[i], &lv_font_montserrat_12, 0);
        lv_obj_set_pos(dayStatusLabels[i], 340, yPosition + 12);

        // Icon container
        dayIconContainers[i] = lv_obj_create(page2);
        lv_obj_set_size(dayIconContainers[i], 80, 80);
        lv_obj_set_style_bg_opa(dayIconContainers[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(dayIconContainers[i], 0, 0);
        lv_obj_set_style_pad_all(dayIconContainers[i], 0, 0);
        lv_obj_clear_flag(dayIconContainers[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(dayIconContainers[i], LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_clip_corner(dayIconContainers[i], false, 0);
        lv_obj_remove_style(dayIconContainers[i], NULL, LV_PART_SCROLLBAR);
        lv_obj_set_pos(dayIconContainers[i], 490, yPosition - 10);

        // Hide initially
        lv_obj_add_flag(dayNameLabels[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(dayDateLabels[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(dayTempLabels[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(dayRainLabels[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(dayStatusLabels[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(dayIconContainers[i], LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * Creates dropdown menu UI
 */
void createDropdownMenu(lv_obj_t *statusBar)
{
    // Create dropdown panel
    dropdownPanel = lv_obj_create(lv_layer_top());
    lv_obj_set_size(dropdownPanel, amoled.width(), 290);
    lv_obj_align(dropdownPanel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(dropdownPanel, lv_color_hex(0x0D1321), 0);
    lv_obj_set_style_bg_opa(dropdownPanel, LV_OPA_80, 0);
    lv_obj_set_style_border_color(dropdownPanel, lv_color_hex(0x2A2F3A), 0);
    lv_obj_set_style_border_width(dropdownPanel, 1, 0);
    lv_obj_set_style_radius(dropdownPanel, 16, 0);
    lv_obj_set_style_shadow_width(dropdownPanel, 20, 0);
    lv_obj_set_style_shadow_color(dropdownPanel, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(dropdownPanel, LV_OPA_40, 0);
    lv_obj_set_style_pad_all(dropdownPanel, 0, 0);
    lv_obj_clear_flag(dropdownPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(dropdownPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(dropdownPanel, LV_OBJ_FLAG_CLICKABLE);

    // Add click handler to close when tapping background
    lv_obj_add_event_cb(dropdownPanel, [](lv_event_t *e)
                        {
        lv_obj_t* target = lv_event_get_target(e);
        if (target == dropdownPanel) toggleDropdownMenu(); }, LV_EVENT_CLICKED, NULL);

    // Panel title
    lv_obj_t *panelTitle = lv_label_create(dropdownPanel);
    lv_label_set_text(panelTitle, "Quick Settings");
    lv_obj_set_style_text_font(panelTitle, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(panelTitle, lv_color_white(), 0);
    lv_obj_align(panelTitle, LV_ALIGN_TOP_LEFT, 10, 10);

    // WiFi toggle button
    wifiToggleBtn = lv_btn_create(dropdownPanel);
    lv_obj_set_size(wifiToggleBtn, 105, 50);
    lv_obj_align(wifiToggleBtn, LV_ALIGN_TOP_LEFT, 10, 40);
    lv_obj_set_style_bg_color(wifiToggleBtn, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_radius(wifiToggleBtn, 10, 0);
    lv_obj_add_event_cb(wifiToggleBtn, handleWifiToggle, LV_EVENT_CLICKED, NULL);
    lv_obj_t *wifiToggleLabel = lv_label_create(wifiToggleBtn);
    lv_label_set_text(wifiToggleLabel, "WiFi: ON");
    lv_obj_center(wifiToggleLabel);

    // Auto refresh toggle button
    autoRefreshToggleBtn = lv_btn_create(dropdownPanel);
    lv_obj_set_size(autoRefreshToggleBtn, 105, 50);
    lv_obj_align(autoRefreshToggleBtn, LV_ALIGN_TOP_RIGHT, -10, 40);
    lv_obj_set_style_bg_color(autoRefreshToggleBtn, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_radius(autoRefreshToggleBtn, 10, 0);
    lv_obj_add_event_cb(autoRefreshToggleBtn, handleAutoRefreshToggle, LV_EVENT_CLICKED, NULL);
    lv_obj_t *autoToggleLabel = lv_label_create(autoRefreshToggleBtn);
    lv_label_set_text(autoToggleLabel, "Auto: ON");
    lv_obj_center(autoToggleLabel);

    // Refresh interval label
    lv_obj_t *intervalLabel = lv_label_create(dropdownPanel);
    lv_label_set_text(intervalLabel, "Refresh Interval:");
    lv_obj_set_style_text_color(intervalLabel, lv_color_white(), 0);
    lv_obj_align(intervalLabel, LV_ALIGN_TOP_LEFT, 10, 100);

    // Interval dropdown
    intervalDropdown = lv_dropdown_create(dropdownPanel);
    lv_dropdown_set_options(intervalDropdown, "5 min\n10 min\n15 min\n30 min\n60 min");
    lv_dropdown_set_selected(intervalDropdown, 2); // Default to 15 min
    lv_obj_set_width(intervalDropdown, 110);
    lv_obj_align(intervalDropdown, LV_ALIGN_TOP_RIGHT, -10, 95);
    lv_obj_add_event_cb(intervalDropdown, handleIntervalChange, LV_EVENT_VALUE_CHANGED, NULL);

    // Separator line
    lv_obj_t *separator = lv_obj_create(dropdownPanel);
    lv_obj_set_size(separator, amoled.width() - 20, 2);
    lv_obj_align(separator, LV_ALIGN_TOP_MID, 0, 135);
    lv_obj_set_style_bg_color(separator, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(separator, 0, 0);

    // WiFi status label
    wifiStatusLabel = lv_label_create(dropdownPanel);
    lv_label_set_text(wifiStatusLabel, "Connecting...");
    lv_obj_set_style_text_font(wifiStatusLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(wifiStatusLabel, lv_color_hex(0x2196F3), 0);
    lv_obj_align(wifiStatusLabel, LV_ALIGN_TOP_LEFT, 10, 145);

    // WiFi signal label
    wifiSignalLabel = lv_label_create(dropdownPanel);
    lv_label_set_text(wifiSignalLabel, "Signal: --");
    lv_obj_set_style_text_color(wifiSignalLabel, lv_color_white(), 0);
    lv_obj_align(wifiSignalLabel, LV_ALIGN_TOP_LEFT, 10, 170);

    // WiFi details label
    wifiDetailsLabel = lv_label_create(dropdownPanel);
    lv_label_set_text(wifiDetailsLabel, "No connection details");
    lv_obj_set_style_text_font(wifiDetailsLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(wifiDetailsLabel, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(wifiDetailsLabel, LV_ALIGN_TOP_LEFT, 10, 195);

    // Close hint
    lv_obj_t *closeHint = lv_label_create(dropdownPanel);
    lv_label_set_text(closeHint, "Tap background to close");
    lv_obj_set_style_text_font(closeHint, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(closeHint, lv_color_hex(0x666666), 0);
    lv_obj_align(closeHint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

/**
 * Creates the main user interface
 */
void createMainUI()
{
    // Create tileview for swipeable pages
    lv_obj_t *tileview = lv_tileview_create(lv_scr_act());
    lv_obj_set_size(tileview, 600, 450);
    lv_obj_set_style_bg_color(tileview, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(tileview, 0, 0);

    // Create status bar
    lv_obj_t *statusBar = lv_obj_create(lv_layer_top());
    lv_obj_set_size(statusBar, amoled.width(), 25);
    lv_obj_align(statusBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(statusBar, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(statusBar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(statusBar, 0, 0);
    lv_obj_set_style_pad_all(statusBar, 0, 0);
    lv_obj_add_flag(statusBar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(statusBar, handleStatusBarClick, LV_EVENT_CLICKED, NULL);

    // WiFi icon in status bar
    wifiIcon = lv_label_create(statusBar);
    lv_label_set_text(wifiIcon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(wifiIcon, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(wifiIcon, lv_color_white(), 0);
    lv_obj_align(wifiIcon, LV_ALIGN_LEFT_MID, 5, 0);

    // Time label in status bar
    timeLabel = lv_label_create(statusBar);
    lv_label_set_text(timeLabel, "--:-- --");
    lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(timeLabel, lv_color_white(), 0);
    lv_obj_align(timeLabel, LV_ALIGN_RIGHT_MID, -5, 0);

    // Create pages
    lv_obj_t *page1 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_RIGHT);
    lv_obj_set_style_bg_color(page1, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(page1, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *page2 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_LEFT);
    lv_obj_set_style_bg_color(page2, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(page2, LV_SCROLLBAR_MODE_OFF);

    // Create page contents
    createCurrentWeatherPage(page1);
    createForecastPage(page2);
    createDropdownMenu(statusBar);

    // Update display
    lv_timer_handler();
}

// ============================================================================
// SECTION 13: MAIN SETUP AND LOOP
// ============================================================================
// Arduino main functions

/**
 * Setup function - runs once at startup
 */
void setup()
{
    // Initialize serial communication for debugging
    Serial.begin(115200);
    delay(1000); // Wait for serial to stabilize
    Serial.println("Weather Station Starting...");

    // Print reset reason for debugging
    esp_reset_reason_t resetReason = esp_reset_reason();
    Serial.printf("Reset reason: %d\n", resetReason);

    // Check PSRAM availability on T4-S3
    if (psramFound())
    {
        Serial.printf("PSRAM found: %d bytes\n", ESP.getPsramSize());
        Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    }
    else
    {
        Serial.println("WARNING: No PSRAM found!");
    }

    // Initialize display with retry logic
    int displayRetries = 3;
    bool displayReady = false;

    while (displayRetries > 0 && !displayReady)
    {
        if (amoled.beginAMOLED_241())
        {
            displayReady = true;
            Serial.println("Display initialized successfully");
        }
        else
        {
            displayRetries--;
            Serial.printf("Display init failed, %d retries left\n", displayRetries);
            delay(500);
        }
    }

    // Restart if display fails
    if (!displayReady)
    {
        Serial.println("Display initialization failed - restarting");
        delay(2000);
        ESP.restart();
    }

    // Initialize LVGL graphics
    beginLvglHelper(amoled);
    delay(100); // Let LVGL stabilize

    // Create user interface
    createMainUI();
    delay(100); // Let UI stabilize

    // Initialize WiFi
    Serial.println("Initializing WiFi...");
    delay(100);

    WiFi.disconnect(true); // Clear any previous connections
    delay(100);

    wifiConnecting = true;
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(WIFI_PS_NONE); // Disable power saving for reliability

    // Show loading screen
    setLoadingVisible(true);
    updateLoadingMessage("Connecting to WiFi...");
    lv_timer_handler();
    delay(100);

    // Start WiFi connection
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Wait for connection (10 seconds max)
    uint32_t wifiStartTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < 10000)
    {
        delay(100);
        lv_timer_handler();

        // Print progress every second
        if ((millis() - wifiStartTime) % 1000 == 0)
        {
            Serial.printf("WiFi connecting... %d seconds\n",
                          (millis() - wifiStartTime) / 1000);
        }
    }

    // Check if connected
    if (WiFi.status() == WL_CONNECTED)
    {
        wifiConnecting = false;
        Serial.println("WiFi connected successfully");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());

        // Sync time
        updateLoadingMessage("Syncing time...");
        lv_timer_handler();
        delay(200);

        initializeTimeSync();
        lv_timer_handler();
    }
    else
    {
        Serial.println("WiFi connection failed - will retry in loop");
        updateLoadingMessage("No WiFi connection\nRetrying...");
        lv_timer_handler();
    }

    Serial.println("Setup complete");
}

/**
 * Loop function - runs continuously after setup
 */
void loop()
{
    // Static variables retain their values between loop iterations
    static uint32_t lastWiFiCheck = 0;
    static uint32_t lastTimeDisplayUpdate = 0;
    static uint32_t lastDropdownRefresh = 0;
    static uint32_t lastUpdateLabelRefresh = 0;
    static bool initialDataFetched = false;
    static bool firstLoop = true;

    uint32_t currentTime = millis();

    // First loop initialization
    if (firstLoop)
    {
        delay(500); // Give system time to stabilize
        firstLoop = false;
        Serial.println("Main loop started");
    }

    // Keep UI responsive - process LVGL events
    lv_timer_handler();

    // Initial data fetch after WiFi and time sync
    if (!initialDataFetched &&
        WiFi.status() == WL_CONNECTED &&
        timesynced &&
        !isFetching)
    {

        // Wait 2 seconds after time sync before first fetch
        static uint32_t fetchWaitStart = 0;
        if (fetchWaitStart == 0)
        {
            fetchWaitStart = currentTime;
        }

        if (currentTime - fetchWaitStart > 2000)
        {
            Serial.println("Performing initial data fetch...");
            updateLoadingMessage("Fetching weather...");
            lv_timer_handler();
            delay(100);
            fetchWeatherData();
            initialDataFetched = true;
        }
    }

    // Update WiFi icon status
    if (wifiIcon)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            lv_label_set_text(wifiIcon, LV_SYMBOL_WIFI);
            lv_obj_set_style_text_color(wifiIcon, lv_color_white(), 0);
        }
        else
        {
            lv_label_set_text(wifiIcon, LV_SYMBOL_WIFI);
            lv_obj_set_style_text_color(wifiIcon, lv_color_hex(0xFF0000), 0);
        }
    }

    // Update time display every second
    if (currentTime - lastTimeDisplayUpdate > 1000)
    {
        lastTimeDisplayUpdate = currentTime;
        updateTimeDisplay();
    }

    // Update dropdown if open (every second)
    if (dropdownOpen && currentTime - lastDropdownRefresh > 1000)
    {
        lastDropdownRefresh = currentTime;
        updateDropdownContent();
    }

    // Update "time ago" label every minute
    if (currentTime - lastUpdateLabelRefresh > 60000 &&
        updateLabel &&
        today.valid &&
        !isFetching &&
        lastRefresh > 0)
    {

        char agoText[64];
        snprintf(agoText, sizeof(agoText), "Updated: %s ago", getTimeSinceUpdate());
        lv_label_set_text(updateLabel, agoText);
        lastUpdateLabelRefresh = currentTime;
    }

    // WiFi reconnection check every 30 seconds
    if (currentTime - lastWiFiCheck > 30000 || wifiConnecting)
    {
        lastWiFiCheck = currentTime;

        if (wifiEnabled && WiFi.status() != WL_CONNECTED && !wifiConnecting)
        {
            Serial.println("WiFi disconnected - attempting reconnection...");
            wifiConnecting = true;

            if (!today.valid)
            {
                setLoadingVisible(true);
                updateLoadingMessage("WiFi disconnected\nReconnecting...");
            }

            WiFi.disconnect();
            lv_timer_handler();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        }
        else if (wifiConnecting && WiFi.status() == WL_CONNECTED)
        {
            wifiConnecting = false;
            Serial.println("WiFi reconnected successfully");

            if (!timesynced)
            {
                updateLoadingMessage("Syncing time...");
                lv_timer_handler();
                initializeTimeSync();
            }

            if (!today.valid && !isFetching)
            {
                Serial.println("Fetching weather data...");
                updateLoadingMessage("Fetching weather...");
                lv_timer_handler();
                fetchWeatherData();
            }

            updateDropdownContent();
        }
    }

    // Auto-refresh check
    if (autoRefresh &&
        WiFi.status() == WL_CONNECTED &&
        !isFetching &&
        today.valid)
    {

        if (currentTime - lastRefresh >= refreshInterval)
        {
            Serial.println("Auto-refresh triggered");
            fetchWeatherData();
        }
    }

    // Small delay to prevent CPU hogging
    delay(5);
}
