# PA1484 - Software Development: Weather Station Project

## Introduction

This project is a Weather Data Visualization and Interaction System for the ESP32-T4-S3 microcontroller.

- **What the system does:** Fetches and displays real-time weather data from SMHI's open API on an AMOLED touchscreen
- **What problem it solves:** Provides portable access to Swedish weather data without needing a smartphone or computer
- **Technologies used:** ESP32-S3, AMOLED touchscreen, SMHI API, PlatformIO, LVGL graphics library
- **Main functionality:** View current weather, 7-day forecast, historical data, and configure city/parameter preferences

## Getting Started

### Prerequisites

- Visual Studio Code with PlatformIO extension
- LilyGo T-Display-S3 AMOLED board
- WiFi network with internet access

### Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/Brickan/PA1484-weather-app
   cd PA1484-weather-app
   ```

2. Open the project folder in VS Code with PlatformIO

3. Configure WiFi credentials in `project/project.ino`:
   ```cpp
   const char *WIFI_SSID = "Your_WiFi_Name";
   const char *WIFI_PASSWORD = "Your_WiFi_Password";
   ```

## Building and Running

- **Build/Upload:** Connect ESP32-S3 via USB-C, then run `pio run --target upload`
- **Startup:** Device connects to WiFi, syncs time, fetches weather data, shows main display
- **Touchscreen operation:** Swipe to navigate between screens
  - Main screen (center): Current weather
  - Forecast screen (right of main)
  - Historical data screen (left of main)
  - Settings screen (above all screens)

## Features

- [x] US1.1C: As a user, I want to see a starting screen to display the current program version and group number on the first screen.
- [x] US1.3: As a user, I want to have a screen to view weather forecast data.
- [x] US1.2C: As a user, I want to see the weather forecast for the next 7 days for the selected city on the second screen in terms of temperature and weather conditions with symbols (e.g., clear sky, rain, snow, thunder) per day at 12:00.
- [x] US2.1: As a user, I want to be able to navigate between different screens (like forecast screen) by sliding a finger over the touch screen.
- [x] US3.1: As a user, I want to have a screen to view historical weather data.
- [x] US3.2D: As a user, on the third screen I want to view the latest months (SMHI API period: latest-months) of historical hourly data for selected weather parameter in the selected city, using a slider to interact with the historical graph by scrolling where a depleted slider corresponds to the oldest datapoint and a full slider corresponds to the latest datapoint.
- [x] US4.1: As a user, on the fourth screen, I want to access a single settings screen to configure both the city and weather parameter options.
- [x] US4.2B: As a user, I want to select from four weather parameters, namely temperature (1), humidity (6), wind speed (4), and Air pressure (9), using a dropdown list, to customize the historical graph.
- [x] US4.3B: As a user, I want to select from five different cities, namely Karlskrona(65090), Stockholm(97400), Göteborg(72420), Malmö(53300), and Kiruna(180940), using a dropdown, to view their weather data for the historical data and starting screen forecast.
- [x] US4.4: As a user, I want to reset the selected city and weather parameter to default using a button.
- [x] US4.5: As a user, I want to set my default city and weather parameter to the current selection using a button, so they are automatically selected when I start the device.
- [x] US4.6: As a user, I want the device to store my default city and weather parameter so that they are retained even after a restart.
