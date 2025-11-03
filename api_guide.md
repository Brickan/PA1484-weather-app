# SMHI Weather API Documentation

## API Endpoint
```
https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/{longitude}/lat/{latitude}/data.json
```

## API Response Structure

### Overview
The SMHI API (Swedish Meteorological and Hydrological Institute) provides weather forecast data for approximately **10 days** into the future. The data density varies throughout the forecast period.

### Time Coverage Pattern

| Days from Now | Data Frequency | Entries per Day | Total Entries |
|---------------|----------------|-----------------|---------------|
| Day 0 (Today) | Hourly | 5-24 (partial) | 5 |
| Day 1-2 | Hourly | 24 | 48 |
| Day 3 | Every 3-6 hours | 7 | 7 |
| Day 4-5 | Every 6 hours | 4 | 8 |
| Day 6-7 | Every 8-12 hours | 2-3 | 5 |
| Day 8-10 | Every 12-24 hours | 1-2 | 5 |
| **Total** | | | **~78 entries** |

### JSON Structure
```json
{
    "approvedTime": "2025-10-23T18:31:34Z",
    "referenceTime": "2025-10-23T18:00:00Z",
    "geometry": {
        "type": "Point",
        "coordinates": [[longitude, latitude]]
    },
    "timeSeries": [
        {
            "validTime": "2025-10-23T19:00:00Z",
            "parameters": [
                {
                    "name": "t",        // Temperature
                    "unit": "Cel",
                    "values": [11.7]
                },
                {
                    "name": "ws",       // Wind speed
                    "unit": "m/s",
                    "values": [7.8]
                },
                {
                    "name": "wd",       // Wind direction
                    "unit": "degree",
                    "values": [111]
                },
                {
                    "name": "r",        // Relative humidity
                    "unit": "percent",
                    "values": [94]
                },
                {
                    "name": "msl",      // Sea level pressure
                    "unit": "hPa",
                    "values": [988.8]
                },
                {
                    "name": "tstm",     // Thunder storm probability
                    "unit": "percent",
                    "values": [2]
                },
                {
                    "name": "Wsymb2",   // Weather symbol (1-27)
                    "unit": "category",
                    "values": [8]
                }
            ]
        }
        // ... more time series entries
    ]
}
```

## Complete List of Available Parameters


| Parameter | Description | Unit | Range/Notes |
|-----------|-------------|------|-------------|
| **t** | Temperature | °C | Air temperature at 2m height |
| **ws** | Wind Speed | m/s | Wind speed at 10m height |
| **wd** | Wind Direction | degrees | 0=N, 90=E, 180=S, 270=W |
| **gust** | Wind Gust Speed | m/s | Maximum wind gust speed |
| **r** | Relative Humidity | % | 0-100% at 2m height |
| **msl** | Mean Sea Level Pressure | hPa | Atmospheric pressure |
| **vis** | Visibility | km | Horizontal visibility |
| **tstm** | Thunderstorm Probability | % | 0-100% chance |
| **Wsymb2** | Weather Symbol | code | 1-27 (see table below) |
| **tcc_mean** | Total Cloud Cover | octas | 0-8 (0=clear, 8=overcast) |
| **lcc_mean** | Low Cloud Cover | octas | 0-8 low altitude clouds |
| **mcc_mean** | Medium Cloud Cover | octas | 0-8 medium altitude clouds |
| **hcc_mean** | High Cloud Cover | octas | 0-8 high altitude clouds |
| **pmin** | Minimum Precipitation | mm/h | Minimum expected |
| **pmean** | Mean Precipitation | mm/h | Average expected |
| **pmax** | Maximum Precipitation | mm/h | Maximum expected |
| **pmedian** | Median Precipitation | mm/h | Median expected |
| **pcat** | Precipitation Category | code | 0=None, 1=Snow, 2=Mix, 3=Rain |
| **spp** | Snow Probability | % | 0-100% chance of snow |
| **tp** | Total Precipitation | mm | Accumulated precipitation |


## Weather Symbols (Wsymb2)


| Symbol | Description | Icon Type |
|--------|-------------|-----------|
| 1 | Clear sky | ☀️ Sun |
| 2 | Nearly clear sky | 🌤️ Mostly sunny |
| 3 | Variable cloudiness | ⛅ Partly cloudy |
| 4 | Halfclear sky | ⛅ Partly cloudy |
| 5 | Cloudy sky | ☁️ Cloudy |
| 6 | Overcast | ☁️ Overcast |
| 7 | Fog | 🌫️ Foggy |
| 8 | Light rain showers | 🌦️ Light rain |
| 9 | Moderate rain showers | 🌧️ Rain showers |
| 10 | Heavy rain showers | 🌧️ Heavy rain |
| 11 | Thunderstorm | ⛈️ Thunder |
| 12 | Light sleet showers | 🌨️ Light sleet |
| 13 | Moderate sleet showers | 🌨️ Sleet |
| 14 | Heavy sleet showers | 🌨️ Heavy sleet |
| 15 | Light snow showers | 🌨️ Light snow |
| 16 | Moderate snow showers | ❄️ Snow showers |
| 17 | Heavy snow showers | ❄️ Heavy snow |
| 18 | Light rain | 🌧️ Light rain |
| 19 | Moderate rain | 🌧️ Rain |
| 20 | Heavy rain | 🌧️ Heavy rain |
| 21 | Thunder | ⛈️ Thunder |
| 22 | Light sleet | 🌨️ Light sleet |
| 23 | Moderate sleet | 🌨️ Sleet |
| 24 | Heavy sleet | 🌨️ Heavy sleet |
| 25 | Light snowfall | 🌨️ Light snow |
| 26 | Moderate snowfall | ❄️ Snow |
| 27 | Heavy snowfall | ❄️ Heavy snow |


## Wind Direction Conversion

Wind direction is provided in degrees (0-360). Convert to compass directions:

```cpp
const char* getWindDirection(int degrees) 
{
    const char* directions[] = {
                                 "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                                 "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
                                };
    int index = ((degrees + 11) / 22) % 16;
    return directions[index];
}
```
