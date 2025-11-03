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

// Get weather from API
bool getWeather()
{
    if (!wifiOK)
    {
        Serial.println("No WiFi!");
        return false;
    }

    Serial.println("Getting weather...");

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

    weather.valid = true;

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
