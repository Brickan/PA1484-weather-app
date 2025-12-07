// Weather Station - ESP32-T4-S3
// SMHI Weather Data Display
// Group 4 - V.4
//
// Sections:
//  1. Includes        7. Time Sync       13. Setup/Loop
//  2. Config          8. Loading
//  3. Structures      9. Storage
//  4. Globals        10. API Fetching
//  5. UI Objects     11. UI Updates
//  6. Helpers        12. UI Creation

// ============================================================================
// SECTION 1: LIBRARY INCLUDES
// ============================================================================

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
#include <Preferences.h>      // Non-volatile storage for settings

// ============================================================================
// SECTION 2: CONFIGURATION SETTINGS
// ============================================================================
// WiFi Configuration
const char *WIFI_SSID = "APx";                   // WiFi network name
const char *WIFI_PASSWORD = "Password.Password"; // WiFi password

// Timezone Configuration (Central European Time with Daylight Saving)
const char *TZ_INFO = "CET-1CEST,M3.5.0,M10.5.0/3";

// ============================================================================
// SECTION 3: DATA STRUCTURES
// ============================================================================

// Holds data for a 3-hour forecast period
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

// Holds current weather data
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

// Holds daily forecast data
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

// Holds city information for weather data
struct City
{
    const char *name; // City name
    int stationId;    // SMHI station ID for historical data
    float longitude;  // Longitude coordinate
    float latitude;   // Latitude coordinate
};

// Holds weather parameter information for historical data
struct WeatherParameter
{
    const char *name; // Parameter name
    int id;           // SMHI parameter ID
    const char *unit; // Unit of measurement
};

// ============================================================================
// SECTION 4: GLOBAL VARIABLES
// ============================================================================

// Available cities for weather data
const City cities[] = {
    {"Karlskrona", 65090, 15.59, 56.16},
    {"Stockholm", 97400, 18.06, 59.33},
    {"Gothenburg", 72420, 11.97, 57.71},
    {"Malmo", 53300, 13.00, 55.61},
    {"Kiruna", 180940, 20.22, 67.86}};
const int NUM_CITIES = 5;
int selectedCityIndex = 0; // Currently selected city (default: Karlskrona)

// Available weather parameters for historical data
const WeatherParameter parameters[] = {
    {"Temperature", 1, "°C"},
    {"Humidity", 6, "%"},
    {"Wind Speed", 4, "m/s"},
    {"Air Pressure", 9, "hPa"}};
const int NUM_PARAMETERS = 4;
int selectedParameterIndex = 0; // Currently selected parameter (default: Temperature)

// Weather data storage
TodayWeather today = {0};      // Current weather (initialized to zero)
HourlyWeather hourly[3] = {0}; // 3-hour forecasts (Morning, Noon, Evening)
DayWeather days[6] = {0};      // 6-day forecast

// Historical data storage
#define MAX_HISTORICAL_POINTS 3200
struct HistoricalDataPoint
{
    time_t timestamp; // Unix timestamp
    float value;      // Weather parameter value
    bool valid;       // True if data is available
};
HistoricalDataPoint historicalData[MAX_HISTORICAL_POINTS] = {0};
int historicalDataCount = 0;        // Number of valid data points
int historicalSliderPosition = 100; // Slider position (0=oldest, 100=newest)
bool historicalDataFetched = false; // True when historical data is loaded

// System state flags
volatile bool isFetching = false;
bool timesynced = false;
bool wifiEnabled = true;
bool wifiConnecting = false;
bool autoRefresh = true;
bool page1IconsCreated = false;
bool page2IconsCreated = false;

// Timing variables
unsigned long refreshInterval = 15 * 60 * 1000; // 15 minutes in milliseconds
unsigned long lastRefresh = 0;                  // Last refresh timestamp

// Display object
LilyGo_Class amoled; // Display driver instance

// Preferences object for non-volatile storage
Preferences preferences;
const char *PREFS_NAMESPACE = "weather_app"; // Namespace for storing settings

// ============================================================================
// SECTION 5: USER INTERFACE OBJECTS
// ============================================================================

// Status bar elements
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

// Page 3 (Settings) UI elements
lv_obj_t *cityDropdown = NULL;      // City selection dropdown
lv_obj_t *parameterDropdown = NULL; // Weather parameter dropdown

// Page 4 (Historical Data) UI elements
lv_obj_t *historicalChart = NULL;  // Chart for historical data
lv_obj_t *historicalSlider = NULL; // Slider to scroll through data
lv_obj_t *historicalLabel = NULL;  // Label showing current date/value
lv_obj_t *yAxisMaxLabel = NULL;
lv_obj_t *yAxisMinLabel = NULL;

lv_obj_t *mainTileview = NULL;
lv_obj_t *settingsTile = NULL;
lv_obj_t *historicalTile = NULL;
lv_obj_t *mainPageTile = NULL;
lv_obj_t *forecastTile = NULL;
int previousColumn = 1;

// ============================================================================
// SECTION 6: HELPER FUNCTIONS
// ============================================================================
// Small functions that help with common tasks

// Converts weather symbol code to description text
// @param symbolCode Weather symbol code (1-27)
// @return Description string like "Clear", "Cloudy", etc.
const char *getWeatherDescription(int symbolCode)
{
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

    return descriptions[symbolCode - 1];
}

// Converts wind direction in degrees to compass direction
// @param degrees Wind direction in degrees (0-359)
// @return Compass direction like "N", "NE", "E", etc.
const char *getWindDirection(int degrees)
{
    const char *directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    int index = ((degrees + 23) / 45) % 8;

    return directions[index];
}

// Calculates time since last update
// @return String like "5m", "2h", or "Never"
const char *getTimeSinceUpdate()
{
    static char timeBuf[16];

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

// Calculates day of week from date string using Zeller's algorithm
// @param dateStr Date string in format "YYYY-MM-DD"
// @param dayName Buffer to store the day name
// @param size Size of the buffer
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

// Formats date string to short format
// @param dateStr Date string "YYYY-MM-DD"
// @param shortDate Buffer for short format "Mon DD"
// @param size Buffer size
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

// Callback function called when time is synchronized
// @param tv Time value structure
void onTimeSync(struct timeval *tv)
{
    timesynced = true;
    Serial.println("Time synchronized successfully");
}

// Initializes time synchronization with NTP servers
void initializeTimeSync()
{
    setenv("TZ", TZ_INFO, 1);
    tzset();
    sntp_set_time_sync_notification_cb(onTimeSync);
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "time.google.com");
    sntp_setservername(1, "pool.ntp.org");
    sntp_setservername(2, "time.cloudflare.com");
    sntp_init();
    Serial.println("Time sync initialized");
}

// Updates the time display in the status bar
void updateTimeDisplay()
{
    if (timeLabel == NULL)
        return;

    if (!timesynced)
    {
        lv_label_set_text(timeLabel, "--:--");
        return;
    }
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year < (2024 - 1900))
    {
        lv_label_set_text(timeLabel, "--:--");
        return;
    }

    char timeStr[16];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    lv_label_set_text(timeLabel, timeStr);
}

// ============================================================================
// SECTION 8: LOADING SCREEN MANAGEMENT
// ============================================================================
// Functions to show/hide loading indicators

// Shows or hides loading spinners
// @param show True to show, false to hide
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

// Updates loading message text
// @param message Text to display
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
// SECTION 9: PERSISTENT STORAGE FUNCTIONS
// ============================================================================

// Save current city and parameter to NVS
void saveDefaultSettings()
{
    preferences.begin(PREFS_NAMESPACE, false);
    preferences.putInt("cityIndex", selectedCityIndex);
    preferences.putInt("paramIndex", selectedParameterIndex);
    preferences.end();

    Serial.println("✓ Settings saved to storage");
    Serial.printf("  City: %s (index %d)\n", cities[selectedCityIndex].name, selectedCityIndex);
    Serial.printf("  Parameter: %s (index %d)\n", parameters[selectedParameterIndex].name, selectedParameterIndex);
}

// Load saved city and parameter from NVS
bool loadDefaultSettings()
{
    preferences.begin(PREFS_NAMESPACE, true);
    int savedCity = preferences.getInt("cityIndex", 0);
    int savedParam = preferences.getInt("paramIndex", 0);

    preferences.end();

    if (savedCity >= 0 && savedCity < 5)
    {
        selectedCityIndex = savedCity;
    }
    else
    {
        selectedCityIndex = 0;
    }

    if (savedParam >= 0 && savedParam < 4)
    {
        selectedParameterIndex = savedParam;
    }
    else
    {
        selectedParameterIndex = 0;
    }

    Serial.println("✓ Settings loaded from storage");
    Serial.printf("  City: %s (index %d)\n", cities[selectedCityIndex].name, selectedCityIndex);
    Serial.printf("  Parameter: %s (index %d)\n", parameters[selectedParameterIndex].name, selectedParameterIndex);

    return (savedCity != 0 || savedParam != 0);
}

// Reset to saved defaults
// Loads saved settings from NVS and updates UI dropdowns
void resetToSavedDefaults()
{
    Serial.println("Resetting to saved defaults...");

    // Load saved defaults from storage
    loadDefaultSettings();

    // Update UI dropdowns if they exist
    if (cityDropdown != NULL)
    {
        lv_dropdown_set_selected(cityDropdown, selectedCityIndex);
    }
    if (parameterDropdown != NULL)
    {
        lv_dropdown_set_selected(parameterDropdown, selectedParameterIndex);
    }

    Serial.println("✓ Reset complete");
}

// ============================================================================
// SECTION 10: WEATHER DATA FETCHING
// ============================================================================
// Main function for getting weather data from API

// Forward declarations for UI update functions
void updateCurrentWeatherDisplay();
void updateForecastDisplay();
void updateHistoricalChart();

// Fetch weather data from SMHI API
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

    // Build API URL dynamically based on selected city
    char apiUrl[256];
    snprintf(apiUrl, sizeof(apiUrl),
             "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/%.2f/lat/%.2f/data.json",
             cities[selectedCityIndex].longitude,
             cities[selectedCityIndex].latitude);

    Serial.printf("Fetching weather for %s (lon: %.2f, lat: %.2f)\n",
                  cities[selectedCityIndex].name,
                  cities[selectedCityIndex].longitude,
                  cities[selectedCityIndex].latitude);

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
        if (http.begin(client, apiUrl))
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

                // Mark as valid if weather symbol exists
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

            // Bounds check (max 6 days)
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
                    // Mark day as valid when any data is available
                    if (!days[dIdx].valid)
                    {
                        // New day with at least some data
                        strncpy(days[dIdx].date, date, sizeof(days[dIdx].date) - 1);
                        days[dIdx].date[sizeof(days[dIdx].date) - 1] = '\0';
                        calculateDayOfWeek(date, days[dIdx].dayName,
                                           sizeof(days[dIdx].dayName));

                        // Check for temperature data
                        if (days[dIdx].tempMin < 999)
                        {
                            // Temperature data available, mark as valid
                            days[dIdx].valid = true;
                            daysCollected++;
                            Serial.printf("Day %d: %s\n", dIdx, date);
                        }
                        else if (days[dIdx].symbol > 0)
                        {
                            // No temperature yet but weather symbol exists
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

                        // Check if all 6 days collected
                        if (daysCollected >= 6)
                        {
                            break;
                        }
                    }
                }
            }
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

// Fetches historical weather data from SMHI API
// Uses latest-months period for hourly data from past ~4 months
void fetchHistoricalData()
{
    // Check if already fetching
    if (isFetching)
    {
        Serial.println("Already fetching data - skipping historical request");
        return;
    }

    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("No WiFi connection - cannot fetch historical data");
        return;
    }

    isFetching = true;
    Serial.println("Fetching historical data...");

    // Show loading message
    if (historicalLabel != NULL)
    {
        char loadingText[128];
        snprintf(loadingText, sizeof(loadingText), "%s - %s\nLoading data...",
                 cities[selectedCityIndex].name,
                 parameters[selectedParameterIndex].name);
        lv_label_set_text(historicalLabel, loadingText);
        lv_timer_handler(); // Update UI immediately
    }

    // Clear old historical data
    historicalDataCount = 0;
    historicalDataFetched = false;
    for (int i = 0; i < MAX_HISTORICAL_POINTS; i++)
    {
        historicalData[i].valid = false;
    }

    // Build API URL for historical data
    // Format: /api/version/1.0/parameter/{param}/station/{station}/period/latest-months/data.json
    char apiUrl[256];
    snprintf(apiUrl, sizeof(apiUrl),
             "https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/%d/station/%d/period/latest-months/data.json",
             parameters[selectedParameterIndex].id,
             cities[selectedCityIndex].stationId);

    Serial.printf("Fetching historical %s for %s (station: %d, parameter: %d)\n",
                  parameters[selectedParameterIndex].name,
                  cities[selectedCityIndex].name,
                  cities[selectedCityIndex].stationId,
                  parameters[selectedParameterIndex].id);

    // Create HTTPS client
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(30);

    // Create HTTP client
    HTTPClient http;
    http.setTimeout(60000);
    http.setReuse(false);

    // Connect to API
    if (!http.begin(client, apiUrl))
    {
        Serial.println("Failed to connect to historical API");
        isFetching = false;
        return;
    }

    Serial.println("Sending GET request to historical API...");
    int responseCode = http.GET();
    Serial.printf("HTTP response code: %d\n", responseCode);

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

    // Find the start of value array in JSON
    if (!stream->find("\"value\":["))
    {
        Serial.println("Could not find value array in response");
        http.end();
        isFetching = false;
        return;
    }

    Serial.println("Parsing historical data...");

    // Create JSON document for parsing
    JsonDocument doc;

    int dataCount = 0;
    int totalAttempts = 0;
    uint32_t parseStartTime = millis();
    const int maxParseErrors = 100; // Increased to handle more data
    int parseErrors = 0;

    // Parse data points from the value array
    // Format: {"date": timestamp_ms, "value": float_value, "quality": "string"}
    while (stream->available() &&
           dataCount < MAX_HISTORICAL_POINTS &&
           parseErrors < maxParseErrors)
    {
        // Check for timeout (increased to 2 minutes for large datasets)
        if (millis() - parseStartTime > 120000)
        {
            Serial.printf("Parsing timeout after %d data points\n", dataCount);
            break;
        }

        // Let system breathe every 50 items
        if (totalAttempts % 50 == 0)
        {
            yield();
            lv_timer_handler();

            // Progress update every 200 points
            if (dataCount > 0 && dataCount % 200 == 0)
            {
                Serial.printf("Loaded %d data points so far...\n", dataCount);
            }
        }

        // Clear document for next parse
        doc.clear();

        // Try to parse next data point object
        // ArduinoJson automatically skips whitespace and commas
        DeserializationError error = deserializeJson(doc, *stream);
        totalAttempts++;

        if (error)
        {
            if (error == DeserializationError::EmptyInput)
            {
                // End of array reached
                Serial.println("End of data array reached (empty input)");
                break;
            }
            parseErrors++;

            // Log first few errors for debugging
            if (parseErrors <= 5)
            {
                Serial.printf("Parse error #%d: %s (attempt %d, loaded %d points)\n",
                              parseErrors, error.c_str(), totalAttempts, dataCount);
            }
            continue;
        }

        // Extract timestamp and value
        long long timestamp_ms = doc["date"];
        const char *value_str = doc["value"];
        const char *quality = doc["quality"];

        // Debug log first few data points
        if (dataCount < 3)
        {
            Serial.printf("Data point %d: timestamp=%lld, value=%s, quality=%s\n",
                          dataCount, timestamp_ms, value_str ? value_str : "NULL", quality ? quality : "NULL");
        }

        // Parse value and store if valid
        if (value_str != NULL && timestamp_ms > 0)
        {
            float value = atof(value_str);

            // Accept data with good quality (G) or Yellow (Y) quality
            if (quality != NULL && (strcmp(quality, "G") == 0 || strcmp(quality, "Y") == 0))
            {
                historicalData[dataCount].timestamp = timestamp_ms / 1000; // Convert ms to seconds
                historicalData[dataCount].value = value;
                historicalData[dataCount].valid = true;
                dataCount++;
            }
        }

        // Consume the comma or closing bracket after the object
        // This prevents the next parse from failing on the delimiter
        while (stream->available())
        {
            char c = stream->read();
            if (c == ',')
            {
                // Found comma, continue to next object
                break;
            }
            else if (c == ']')
            {
                // End of array
                Serial.println("End of data array reached (found ])");
                goto parsing_done;
            }
            else if (c != ' ' && c != '\n' && c != '\r' && c != '\t')
            {
                // Unexpected character
                break;
            }
        }
    }

parsing_done:
    Serial.printf("Parsing complete: %d attempts, %d errors, %d valid data points\n",
                  totalAttempts, parseErrors, dataCount);

    http.end();

    historicalDataCount = dataCount;
    historicalDataFetched = true;

    Serial.printf("Historical data fetch complete - %d data points loaded\n", dataCount);

    // Update historical chart display
    updateHistoricalChart();

    isFetching = false;
}

// ============================================================================
// SECTION 11: USER INTERFACE UPDATE FUNCTIONS
// ============================================================================
// Functions that update the display with new data

// Updates Page 1 with current weather and hourly forecast
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

// Updates Page 2 with 6-day forecast
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
// SECTION 12: USER INTERFACE CREATION
// ============================================================================
// Functions to create all UI elements

// Event handler for refresh button
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

// Event handler for city selection modal button
void handleCitySelection(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    int cityIndex = (int)(intptr_t)lv_event_get_user_data(e);

    // Update selected city
    selectedCityIndex = cityIndex;
    Serial.printf("City changed to: %s\n", cities[selectedCityIndex].name);

    // Update city label
    if (cityLabel)
    {
        lv_label_set_text(cityLabel, cities[selectedCityIndex].name);
    }

    // Close the modal
    lv_obj_t *modal = lv_obj_get_parent(lv_obj_get_parent(btn));
    lv_obj_del(modal);

    // Fetch new weather data for selected city
    if (WiFi.status() == WL_CONNECTED && !isFetching)
    {
        fetchWeatherData();
    }
}

// Event handler for city label click - opens city selection modal
void handleCityLabelClick(lv_event_t *e)
{
    Serial.println("City label clicked - opening city selector");

    // Create modal background
    lv_obj_t *modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(modal, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(modal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(modal, LV_OPA_80, 0);
    lv_obj_set_style_border_width(modal, 0, 0);
    lv_obj_clear_flag(modal, LV_OBJ_FLAG_SCROLLABLE);

    // Create content panel
    lv_obj_t *panel = lv_obj_create(modal);
    lv_obj_set_size(panel, 400, 380);
    lv_obj_center(panel);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_style_radius(panel, 15, 0);

    // Title
    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, "Select City");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x2196F3), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Create city buttons
    for (int i = 0; i < NUM_CITIES; i++)
    {
        lv_obj_t *btn = lv_btn_create(panel);
        lv_obj_set_size(btn, 360, 50);
        lv_obj_set_pos(btn, 20, 50 + (i * 60));

        // Highlight currently selected city
        if (i == selectedCityIndex)
        {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x2196F3), 0);
        }
        else
        {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333), 0);
        }

        lv_obj_set_style_radius(btn, 8, 0);

        // Add city name label
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, cities[i].name);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
        lv_obj_center(label);

        // Add click event
        lv_obj_add_event_cb(btn, handleCitySelection, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}

// Event handler for city dropdown in settings
void handleCityDropdownChange(lv_event_t *e)
{
    uint16_t selection = lv_dropdown_get_selected(cityDropdown);
    selectedCityIndex = selection;
    Serial.printf("City changed to: %s\n", cities[selectedCityIndex].name);

    // Update city label on main page
    if (cityLabel)
    {
        lv_label_set_text(cityLabel, cities[selectedCityIndex].name);
    }

    // Fetch new weather data for selected city
    if (WiFi.status() == WL_CONNECTED && !isFetching)
    {
        fetchWeatherData();
    }

    // Always update historical label and invalidate data
    historicalDataFetched = false;
    historicalDataCount = 0;

    if (historicalLabel != NULL)
    {
        char labelText[128];
        snprintf(labelText, sizeof(labelText), "%s - %s\nClick 'Load Historical Data'",
                 cities[selectedCityIndex].name,
                 parameters[selectedParameterIndex].name);
        lv_label_set_text(historicalLabel, labelText);
    }

    // Clear historical chart data
    if (historicalChart != NULL)
    {
        lv_chart_series_t *series = lv_chart_get_series_next(historicalChart, NULL);
        if (series != NULL)
        {
            for (int i = 0; i < 100; i++)
            {
                lv_chart_set_value_by_id(historicalChart, series, i, 0);
            }
            lv_chart_refresh(historicalChart);
        }
    }
}

// Event handler for parameter dropdown in settings
void handleParameterDropdownChange(lv_event_t *e)
{
    uint16_t selection = lv_dropdown_get_selected(parameterDropdown);
    selectedParameterIndex = selection;
    Serial.printf("Parameter changed to: %s\n", parameters[selectedParameterIndex].name);

    // Always update historical label and invalidate data
    historicalDataFetched = false;
    historicalDataCount = 0;

    if (historicalLabel != NULL)
    {
        char labelText[128];
        snprintf(labelText, sizeof(labelText), "%s - %s\nClick 'Load Historical Data'",
                 cities[selectedCityIndex].name,
                 parameters[selectedParameterIndex].name);
        lv_label_set_text(historicalLabel, labelText);
    }

    // Clear chart data
    if (historicalChart != NULL)
    {
        lv_chart_series_t *series = lv_chart_get_series_next(historicalChart, NULL);
        if (series != NULL)
        {
            for (int i = 0; i < 100; i++)
            {
                lv_chart_set_value_by_id(historicalChart, series, i, 0);
            }
            lv_chart_refresh(historicalChart);
        }
    }
}

// Event handler for "Set as Default" button
// Saves current city and parameter selection to storage
void handleSaveDefaultButton(lv_event_t *e)
{
    saveDefaultSettings();

    // Optional: Show feedback to user (you can add a popup later)
    Serial.println("User clicked 'Set as Default'");
}

// Event handler for "Reset to Default" button
// Resets to user's saved default settings
void handleResetDefaultButton(lv_event_t *e)
{
    resetToSavedDefaults();

    // Refresh weather data with new defaults
    if (WiFi.status() == WL_CONNECTED && !isFetching)
    {
        fetchWeatherData();
    }

    // Update historical label
    historicalDataFetched = false;
    historicalDataCount = 0;

    if (historicalLabel != NULL)
    {
        char labelText[128];
        snprintf(labelText, sizeof(labelText), "%s - %s\nClick 'Load Historical Data'",
                 cities[selectedCityIndex].name,
                 parameters[selectedParameterIndex].name);
        lv_label_set_text(historicalLabel, labelText);
    }

    // Clear historical chart
    if (historicalChart != NULL)
    {
        lv_chart_series_t *series = lv_chart_get_series_next(historicalChart, NULL);
        if (series != NULL)
        {
            for (int i = 0; i < 100; i++)
            {
                lv_chart_set_value_by_id(historicalChart, series, i, 0);
            }
            lv_chart_refresh(historicalChart);
        }
    }

    // Update city label on main page
    if (cityLabel)
    {
        lv_label_set_text(cityLabel, cities[selectedCityIndex].name);
    }

    Serial.println("User clicked 'Reset to Default'");
}

// Event handler for historical data slider
// Updates chart display when slider is moved
void handleHistoricalSlider(lv_event_t *e)
{
    int32_t value = lv_slider_get_value(historicalSlider);
    historicalSliderPosition = value;

    // Update chart with new window of data
    updateHistoricalChart();
}

// Updates historical chart with data based on slider position
// Slider at 0 = oldest data, slider at 100 = newest data
void updateHistoricalChart()
{
    if (!historicalDataFetched || historicalDataCount == 0 || historicalChart == NULL)
    {
        Serial.printf("updateHistoricalChart: Cannot update - fetched=%d, count=%d, chart=%p\n",
                      historicalDataFetched, historicalDataCount, historicalChart);
        return;
    }

    // Get the first series from the chart
    lv_chart_series_t *series = lv_chart_get_series_next(historicalChart, NULL);
    if (series == NULL)
    {
        Serial.println("updateHistoricalChart: No chart series found");
        return;
    }

    // Calculate which data points to show based on slider position
    // Slider 0-100 maps to oldest-newest data
    const int pointsToShow = 168;
    int dataWindowSize = min(historicalDataCount, pointsToShow);

    // Calculate start index based on slider position (0-100)
    // slider at 100 = show latest data (end of array)
    // slider at 0 = show oldest data (start of array)
    int maxStartIndex = max(0, historicalDataCount - dataWindowSize);
    int startIndex = (maxStartIndex * historicalSliderPosition) / 100;

    Serial.printf("updateHistoricalChart: slider=%d, dataCount=%d, startIndex=%d, windowSize=%d\n",
                  historicalSliderPosition, historicalDataCount, startIndex, dataWindowSize);

    // Find min and max values for y-axis scaling - with safety checks
    float minVal = 999999.0;
    float maxVal = -999999.0;
    bool foundValidData = false;

    for (int i = 0; i < dataWindowSize; i++)
    {
        int dataIndex = startIndex + i;
        if (dataIndex < historicalDataCount && historicalData[dataIndex].valid)
        {
            float val = historicalData[dataIndex].value;
            if (val < minVal)
                minVal = val;
            if (val > maxVal)
                maxVal = val;
            foundValidData = true;
        }
    }

    // If no valid data found, use default range
    if (!foundValidData)
    {
        Serial.println("updateHistoricalChart: No valid data in window");
        minVal = 0;
        maxVal = 100;
    }

    // Add some padding to the range
    float range = maxVal - minVal;
    if (range < 1.0)
        range = 1.0; // Minimum range
    minVal -= range * 0.1;
    maxVal += range * 0.1;

    // Update chart range
    lv_chart_set_range(historicalChart, LV_CHART_AXIS_PRIMARY_Y,
                       (int)(minVal * 10), (int)(maxVal * 10));

    if (yAxisMaxLabel != NULL && yAxisMinLabel != NULL)
    {
        char maxStr[32], minStr[32];
        snprintf(maxStr, sizeof(maxStr), "%.1f%s", maxVal, parameters[selectedParameterIndex].unit);
        snprintf(minStr, sizeof(minStr), "%.1f%s", minVal, parameters[selectedParameterIndex].unit);
        lv_label_set_text(yAxisMaxLabel, maxStr);
        lv_label_set_text(yAxisMinLabel, minStr);
    }

    // Fill chart with data using LVGL API
    for (int i = 0; i < pointsToShow; i++)
    {
        int value = 0;
        if (i < dataWindowSize)
        {
            int dataIndex = startIndex + i;
            if (dataIndex < historicalDataCount && historicalData[dataIndex].valid)
            {
                // Scale value to fit in chart (multiply by 10 to match range)
                value = (int)(historicalData[dataIndex].value * 10);
            }
        }
        lv_chart_set_value_by_id(historicalChart, series, i, value);
    }

    lv_chart_refresh(historicalChart);

    // Update label with date range
    if (historicalLabel != NULL && startIndex < historicalDataCount)
    {
        struct tm timeinfo_start;
        struct tm timeinfo_end;
        time_t start_time = historicalData[startIndex].timestamp;
        time_t end_time = historicalData[min(startIndex + dataWindowSize - 1, historicalDataCount - 1)].timestamp;

        localtime_r(&start_time, &timeinfo_start);
        localtime_r(&end_time, &timeinfo_end);

        char dateStr[128];
        snprintf(dateStr, sizeof(dateStr), "%s - %s\n%02d/%02d %02d:%02d - %02d/%02d %02d:%02d",
                 cities[selectedCityIndex].name,
                 parameters[selectedParameterIndex].name,
                 timeinfo_start.tm_mon + 1, timeinfo_start.tm_mday,
                 timeinfo_start.tm_hour, timeinfo_start.tm_min,
                 timeinfo_end.tm_mon + 1, timeinfo_end.tm_mday,
                 timeinfo_end.tm_hour, timeinfo_end.tm_min);

        lv_label_set_text(historicalLabel, dateStr);
    }
}

// Creates Page 1 (Current Weather) UI elements
void createCurrentWeatherPage(lv_obj_t *page1)
{
    // City name label (display only - change in Settings page)
    cityLabel = lv_label_create(page1);
    lv_label_set_text(cityLabel, cities[selectedCityIndex].name);
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
    lv_obj_align(updateLabel, LV_ALIGN_BOTTOM_LEFT, 5, -5);
    lv_obj_add_flag(updateLabel, LV_OBJ_FLAG_HIDDEN);

    // Group number label
    lv_obj_t *groupLabel = lv_label_create(page1);
    lv_label_set_text(groupLabel, "Group 4");
    lv_obj_set_style_text_color(groupLabel, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(groupLabel, &lv_font_montserrat_12, 0);
    lv_obj_align(groupLabel, LV_ALIGN_BOTTOM_RIGHT, -5, -20);

    // Version label
    lv_obj_t *versionLabel = lv_label_create(page1);
    lv_label_set_text(versionLabel, "v4");
    lv_obj_set_style_text_color(versionLabel, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(versionLabel, &lv_font_montserrat_12, 0);
    lv_obj_align(versionLabel, LV_ALIGN_BOTTOM_RIGHT, -5, -5);

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

// Creates Page 2 (6-Day Forecast) UI elements
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

// Creates Page 3 (Settings) UI elements
void createSettingsPage(lv_obj_t *page3)
{
    // Page title
    lv_obj_t *title = lv_label_create(page3);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x2196F3), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    // ============= CITY SELECTION SECTION =============
    lv_obj_t *citySection = lv_label_create(page3);
    lv_label_set_text(citySection, "City");
    lv_obj_set_style_text_font(citySection, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(citySection, lv_color_white(), 0);
    lv_obj_set_pos(citySection, 30, 80);

    // City dropdown
    cityDropdown = lv_dropdown_create(page3);
    lv_dropdown_set_options(cityDropdown,
                            "Karlskrona\n"
                            "Stockholm\n"
                            "Gothenburg\n"
                            "Malmo\n"
                            "Kiruna");
    lv_dropdown_set_selected(cityDropdown, selectedCityIndex);
    lv_obj_set_size(cityDropdown, 540, 45);
    lv_obj_set_pos(cityDropdown, 30, 110);
    lv_obj_set_style_bg_color(cityDropdown, lv_color_white(), 0);
    lv_obj_set_style_text_color(cityDropdown, lv_color_black(), 0);
    lv_obj_set_style_radius(cityDropdown, 8, 0);
    lv_obj_add_event_cb(cityDropdown, handleCityDropdownChange, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(cityDropdown, [](lv_event_t *e)
                        {
        lv_obj_t *list = lv_dropdown_get_list(lv_event_get_target(e));
        if (list) {
            lv_obj_set_style_bg_color(list, lv_color_white(), 0);
            lv_obj_set_style_text_color(list, lv_color_black(), 0);
            lv_obj_set_style_bg_color(list, lv_color_hex(0x2196F3), LV_PART_SELECTED);
            lv_obj_set_style_text_color(list, lv_color_white(), LV_PART_SELECTED);
        } }, LV_EVENT_CLICKED, NULL);

    // ============= WEATHER PARAMETER SELECTION SECTION =============
    lv_obj_t *paramSection = lv_label_create(page3);
    lv_label_set_text(paramSection, "Weather Parameter");
    lv_obj_set_style_text_font(paramSection, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(paramSection, lv_color_white(), 0);
    lv_obj_set_pos(paramSection, 30, 180);

    // Parameter dropdown
    parameterDropdown = lv_dropdown_create(page3);
    lv_dropdown_set_options(parameterDropdown,
                            "Temperature\n"
                            "Humidity\n"
                            "Wind Speed\n"
                            "Air Pressure");
    lv_dropdown_set_selected(parameterDropdown, selectedParameterIndex);
    lv_obj_set_size(parameterDropdown, 540, 45);
    lv_obj_set_pos(parameterDropdown, 30, 210);
    lv_obj_set_style_bg_color(parameterDropdown, lv_color_white(), 0);
    lv_obj_set_style_text_color(parameterDropdown, lv_color_black(), 0);
    lv_obj_set_style_radius(parameterDropdown, 8, 0);
    lv_obj_add_event_cb(parameterDropdown, handleParameterDropdownChange, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(parameterDropdown, [](lv_event_t *e)
                        {
        lv_obj_t *list = lv_dropdown_get_list(lv_event_get_target(e));
        if (list) {
            lv_obj_set_style_bg_color(list, lv_color_white(), 0);
            lv_obj_set_style_text_color(list, lv_color_black(), 0);
            lv_obj_set_style_bg_color(list, lv_color_hex(0x2196F3), LV_PART_SELECTED);
            lv_obj_set_style_text_color(list, lv_color_white(), LV_PART_SELECTED);
        } }, LV_EVENT_CLICKED, NULL);

    // ============= DEFAULT SETTINGS BUTTONS SECTION =============
    lv_obj_t *defaultsSection = lv_label_create(page3);
    lv_label_set_text(defaultsSection, "Default Settings");
    lv_obj_set_style_text_font(defaultsSection, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(defaultsSection, lv_color_white(), 0);
    lv_obj_set_pos(defaultsSection, 30, 270);

    // "Set as Default" button
    lv_obj_t *saveDefaultBtn = lv_btn_create(page3);
    lv_obj_set_size(saveDefaultBtn, 540, 50);
    lv_obj_set_pos(saveDefaultBtn, 30, 305);
    lv_obj_set_style_bg_color(saveDefaultBtn, lv_color_hex(0x4CAF50), 0); // Green color
    lv_obj_set_style_bg_color(saveDefaultBtn, lv_color_hex(0x45a049), LV_STATE_PRESSED);
    lv_obj_set_style_radius(saveDefaultBtn, 8, 0);
    lv_obj_add_event_cb(saveDefaultBtn, handleSaveDefaultButton, LV_EVENT_CLICKED, NULL);

    lv_obj_t *saveDefaultLabel = lv_label_create(saveDefaultBtn);
    lv_label_set_text(saveDefaultLabel, "Set as Default");
    lv_obj_set_style_text_font(saveDefaultLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(saveDefaultLabel, lv_color_white(), 0);
    lv_obj_center(saveDefaultLabel);

    // "Reset to Default" button
    lv_obj_t *resetDefaultBtn = lv_btn_create(page3);
    lv_obj_set_size(resetDefaultBtn, 540, 50);
    lv_obj_set_pos(resetDefaultBtn, 30, 370);
    lv_obj_set_style_bg_color(resetDefaultBtn, lv_color_hex(0xFF5722), 0); // Orange/Red color
    lv_obj_set_style_bg_color(resetDefaultBtn, lv_color_hex(0xe64a19), LV_STATE_PRESSED);
    lv_obj_set_style_radius(resetDefaultBtn, 8, 0);
    lv_obj_add_event_cb(resetDefaultBtn, handleResetDefaultButton, LV_EVENT_CLICKED, NULL);

    lv_obj_t *resetDefaultLabel = lv_label_create(resetDefaultBtn);
    lv_label_set_text(resetDefaultLabel, "Reset to Default");
    lv_obj_set_style_text_font(resetDefaultLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(resetDefaultLabel, lv_color_white(), 0);
    lv_obj_center(resetDefaultLabel);
}

// Creates Page 4 (Historical Data) UI elements
// API: https://opendata-download-metobs.smhi.se/api/version/1.0/parameter/{param}/station/{station}/period/latest-months/data.json
void createHistoricalPage(lv_obj_t *page4)
{
    // Page title
    lv_obj_t *title = lv_label_create(page4);
    lv_label_set_text(title, "Historical Data");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x2196F3), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    // Info label showing selected parameter
    historicalLabel = lv_label_create(page4);
    lv_label_set_text_fmt(historicalLabel, "%s - %s",
                          cities[selectedCityIndex].name,
                          parameters[selectedParameterIndex].name);
    lv_obj_set_style_text_font(historicalLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(historicalLabel, lv_color_white(), 0);
    lv_obj_align(historicalLabel, LV_ALIGN_TOP_MID, 0, 70);

    // Create chart for historical data
    historicalChart = lv_chart_create(page4);
    lv_obj_set_size(historicalChart, 500, 250);
    lv_obj_set_pos(historicalChart, 70, 110);
    lv_obj_set_style_bg_color(historicalChart, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_color(historicalChart, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_border_width(historicalChart, 2, 0);
    lv_obj_set_style_radius(historicalChart, 10, 0);

    yAxisMaxLabel = lv_label_create(page4);
    lv_label_set_text(yAxisMaxLabel, "---");
    lv_obj_set_style_text_font(yAxisMaxLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(yAxisMaxLabel, lv_color_hex(0x888888), 0);
    lv_obj_set_pos(yAxisMaxLabel, 5, 115);

    yAxisMinLabel = lv_label_create(page4);
    lv_label_set_text(yAxisMinLabel, "---");
    lv_obj_set_style_text_font(yAxisMinLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(yAxisMinLabel, lv_color_hex(0x888888), 0);
    lv_obj_set_pos(yAxisMinLabel, 5, 340);

    // Configure chart
    lv_chart_set_type(historicalChart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(historicalChart, 168);
    lv_chart_set_range(historicalChart, LV_CHART_AXIS_PRIMARY_Y, 0, 100); // Will be adjusted based on data
    lv_chart_set_div_line_count(historicalChart, 5, 10);

    // Style the chart
    lv_obj_set_style_line_width(historicalChart, 3, LV_PART_ITEMS);
    lv_obj_set_style_line_color(historicalChart, lv_color_hex(0x2196F3), LV_PART_ITEMS);
    lv_obj_set_style_width(historicalChart, 0, LV_PART_INDICATOR);  // Hide points
    lv_obj_set_style_height(historicalChart, 0, LV_PART_INDICATOR); // Hide points

    // Add data series
    lv_chart_series_t *series = lv_chart_add_series(historicalChart, lv_color_hex(0x2196F3), LV_CHART_AXIS_PRIMARY_Y);

    // Initialize with empty data using LVGL API
    for (int i = 0; i < 168; i++)
    {
        lv_chart_set_value_by_id(historicalChart, series, i, 0);
    }

    lv_chart_refresh(historicalChart);

    // Slider for scrolling through historical data
    // Depleted (0) = oldest data, Full (100) = latest data
    historicalSlider = lv_slider_create(page4);
    lv_obj_set_size(historicalSlider, 500, 15);
    lv_obj_set_pos(historicalSlider, 70, 375);
    lv_slider_set_range(historicalSlider, 0, 100);
    lv_slider_set_value(historicalSlider, 100, LV_ANIM_OFF); // Start at newest data
    lv_obj_set_style_bg_color(historicalSlider, lv_color_hex(0x2196F3), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(historicalSlider, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_add_event_cb(historicalSlider, handleHistoricalSlider, LV_EVENT_VALUE_CHANGED, NULL);

    // Slider labels
    lv_obj_t *oldestLabel = lv_label_create(page4);
    lv_label_set_text(oldestLabel, "Oldest");
    lv_obj_set_style_text_font(oldestLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(oldestLabel, lv_color_hex(0x888888), 0);
    lv_obj_set_pos(oldestLabel, 70, 395);

    lv_obj_t *latestLabel = lv_label_create(page4);
    lv_label_set_text(latestLabel, "Latest");
    lv_obj_set_style_text_font(latestLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(latestLabel, lv_color_hex(0x888888), 0);
    lv_obj_set_pos(latestLabel, 520, 395);

    // Load data button
    lv_obj_t *loadBtn = lv_btn_create(page4);
    lv_obj_set_size(loadBtn, 200, 45);
    lv_obj_align(loadBtn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(loadBtn, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_radius(loadBtn, 8, 0);

    lv_obj_t *btnLabel = lv_label_create(loadBtn);
    lv_label_set_text(btnLabel, "Load Historical Data");
    lv_obj_set_style_text_font(btnLabel, &lv_font_montserrat_14, 0);
    lv_obj_center(btnLabel);

    // Add event handler for load button
    lv_obj_add_event_cb(loadBtn, [](lv_event_t *e)
                        {
        if (WiFi.status() == WL_CONNECTED && !isFetching)
        {
            fetchHistoricalData();
        } }, LV_EVENT_CLICKED, NULL);
}

// Creates the main user interface
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

    // Store tileview reference
    mainTileview = tileview;

    // Settings row (row 0) - ABOVE main content
    // Only center (1,0) has actual settings, left/right redirect to center
    lv_obj_t *settingsLeft = lv_tileview_add_tile(tileview, 0, 0, (lv_dir_t)(LV_DIR_BOTTOM | LV_DIR_RIGHT));
    lv_obj_set_style_bg_color(settingsLeft, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(settingsLeft, LV_SCROLLBAR_MODE_OFF);

    settingsTile = lv_tileview_add_tile(tileview, 1, 0, (lv_dir_t)(LV_DIR_BOTTOM | LV_DIR_LEFT | LV_DIR_RIGHT));
    lv_obj_set_style_bg_color(settingsTile, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(settingsTile, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *settingsRight = lv_tileview_add_tile(tileview, 2, 0, (lv_dir_t)(LV_DIR_BOTTOM | LV_DIR_LEFT));
    lv_obj_set_style_bg_color(settingsRight, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(settingsRight, LV_SCROLLBAR_MODE_OFF);

    // Main content row (row 1) - all can swipe DOWN (pull down) to settings above
    historicalTile = lv_tileview_add_tile(tileview, 0, 1, (lv_dir_t)(LV_DIR_RIGHT | LV_DIR_TOP));
    lv_obj_set_style_bg_color(historicalTile, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(historicalTile, LV_SCROLLBAR_MODE_OFF);

    mainPageTile = lv_tileview_add_tile(tileview, 1, 1, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT | LV_DIR_TOP));
    lv_obj_set_style_bg_color(mainPageTile, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(mainPageTile, LV_SCROLLBAR_MODE_OFF);

    forecastTile = lv_tileview_add_tile(tileview, 2, 1, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_TOP));
    lv_obj_set_style_bg_color(forecastTile, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(forecastTile, LV_SCROLLBAR_MODE_OFF);

    // Create main page contents
    createHistoricalPage(historicalTile);
    createCurrentWeatherPage(mainPageTile);
    createForecastPage(forecastTile);

    createSettingsPage(settingsTile);

    lv_obj_add_event_cb(tileview, [](lv_event_t *e)
                        {
        static bool onSettings = false;
        static bool redirecting = false;

        if (redirecting) {
            redirecting = false;
            return;
        }

        lv_obj_t *tile = lv_tileview_get_tile_act(mainTileview);
        lv_coord_t col = lv_obj_get_x(tile) / lv_obj_get_width(tile);
        lv_coord_t row = lv_obj_get_y(tile) / lv_obj_get_height(tile);

        if (row == 0) {
            if (col != 1) {
                redirecting = true;
                lv_obj_set_tile(mainTileview, settingsTile, LV_ANIM_ON);
            }
            onSettings = true;
        }
        else if (row == 1) {
            if (onSettings) {
                onSettings = false;
                if (col != previousColumn) {
                    redirecting = true;
                    lv_obj_t *targetTile = historicalTile;
                    if (previousColumn == 1) targetTile = mainPageTile;
                    else if (previousColumn == 2) targetTile = forecastTile;
                    lv_obj_set_tile(mainTileview, targetTile, LV_ANIM_ON);
                }
            } else {
                previousColumn = col;
            }
        } }, LV_EVENT_VALUE_CHANGED, NULL);

    // Set initial page to Main (center)
    lv_obj_set_tile(tileview, mainPageTile, LV_ANIM_OFF);

    // Update display
    lv_timer_handler();
}

// ============================================================================
// SECTION 13: MAIN SETUP AND LOOP
// ============================================================================
// Arduino main functions

// Setup function - runs once at startup
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

    // Load saved settings from non-volatile storage
    Serial.println("Loading saved settings...");
    loadDefaultSettings();

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

// Loop function - runs continuously after setup
void loop()
{
    // Static variables retain their values between loop iterations
    static uint32_t lastWiFiCheck = 0;
    static uint32_t lastTimeDisplayUpdate = 0;
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