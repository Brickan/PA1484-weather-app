#!/usr/bin/env python3
"""
Unified SMHI Weather API Test Script
1. Tests API data structure and coverage
2. Simulates Arduino weather data parsing
3. Compares both approaches
"""

import json
import requests
from datetime import datetime, timedelta
from collections import defaultdict
import sys

# API Configuration
API_URL = "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/15.59/lat/56.16/data.json"

# Weather symbol descriptions
WEATHER_SYMBOLS = {
    1: "Clear sky", 2: "Nearly clear sky", 3: "Variable cloudiness",
    4: "Halfclear sky", 5: "Cloudy sky", 6: "Overcast", 7: "Fog",
    8: "Light rain showers", 9: "Moderate rain showers", 10: "Heavy rain showers",
    11: "Thunderstorm", 12: "Light sleet showers", 13: "Moderate sleet showers",
    14: "Heavy sleet showers", 15: "Light snow showers", 16: "Moderate snow showers",
    17: "Heavy snow showers", 18: "Light rain", 19: "Moderate rain",
    20: "Heavy rain", 21: "Thunder", 22: "Light sleet", 23: "Moderate sleet",
    24: "Heavy sleet", 25: "Light snowfall", 26: "Moderate snowfall", 27: "Heavy snowfall"
}

# Parameter descriptions
PARAMETER_INFO = {
    "t": ("Temperature", "°C"), "ws": ("Wind Speed", "m/s"),
    "wd": ("Wind Direction", "°"), "r": ("Relative Humidity", "%"),
    "msl": ("Air Pressure", "hPa"), "vis": ("Visibility", "km"),
    "tstm": ("Thunder Probability", "%"), "tcc_mean": ("Total Cloud Cover", "octas"),
    "lcc_mean": ("Low Cloud Cover", "octas"), "mcc_mean": ("Medium Cloud Cover", "octas"),
    "hcc_mean": ("High Cloud Cover", "octas"), "gust": ("Wind Gust Speed", "m/s"),
    "pmin": ("Min Precipitation", "mm/h"), "pmean": ("Mean Precipitation", "mm/h"),
    "pmax": ("Max Precipitation", "mm/h"), "pmedian": ("Median Precipitation", "mm/h"),
    "pcat": ("Precipitation Category", ""), "spp": ("Snow Probability", "%"),
    "Wsymb2": ("Weather Symbol", ""), "tp": ("Total Precipitation", "mm")
}

def get_wind_direction(degrees):
    """Convert wind direction degrees to compass direction"""
    if degrees is None:
        return "N/A"
    directions = ["N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                  "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"]
    index = int((degrees + 11.25) / 22.5) % 16
    return directions[index]

def fetch_weather_data():
    """Fetch weather data from SMHI API"""
    print("Fetching weather data from SMHI API...")
    print(f"URL: {API_URL}")
    print("-" * 80)

    try:
        response = requests.get(API_URL, timeout=30)
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"❌ Error fetching data: {e}")
        sys.exit(1)

# =============================================================================
# PART 1: API STRUCTURE ANALYSIS (from test_api.py)
# =============================================================================

def analyze_api_structure(data):
    """Analyze API data structure and time coverage"""
    print("\n" + "=" * 80)
    print("PART 1: API STRUCTURE ANALYSIS - Understanding SMHI Weather API")
    print("=" * 80)

    # API Endpoint Info
    print("\n🌐 API Endpoint:")
    print(f"  URL: {API_URL}")
    geometry = data.get('geometry', {})
    coords = geometry.get('coordinates', [[None, None]])[0]
    print(f"  Location: Longitude {coords[0]}°, Latitude {coords[1]}°")
    print(f"  → This is Karlskrona, Sweden")
    print(f"  API Type: Point forecast (specific location)")
    print(f"  Version: PMP3G v2 (SMHI's meteorological forecast)")

    # Metadata
    print(f"\n📅 Forecast Metadata:")
    approved = datetime.fromisoformat(data['approvedTime'].replace('Z', '+00:00'))
    reference = datetime.fromisoformat(data['referenceTime'].replace('Z', '+00:00'))
    print(f"  Approved Time:  {approved.strftime('%Y-%m-%d %H:%M UTC')}")
    print(f"  → When SMHI published this forecast")
    print(f"  Reference Time: {reference.strftime('%Y-%m-%d %H:%M UTC')}")
    print(f"  → Forecast calculation base time")
    print(f"  Total Entries:  {len(data['timeSeries'])}")
    print(f"  → Number of hourly forecast data points")

    # Analyze time series
    days_data = defaultdict(list)
    first_time = None
    last_time = None

    for i, entry in enumerate(data['timeSeries']):
        time_str = entry['validTime']
        dt = datetime.fromisoformat(time_str.replace('Z', '+00:00'))
        day = dt.date()

        if i == 0:
            first_time = dt
        if i == len(data['timeSeries']) - 1:
            last_time = dt

        days_data[day].append(dt.hour)

    # Calculate forecast duration
    duration = (last_time - first_time).total_seconds() / 3600
    duration_days = duration / 24

    print(f"\n📊 Time Coverage:")
    print(f"  First Entry:    {first_time.strftime('%Y-%m-%d %H:%M UTC')}")
    print(f"  Last Entry:     {last_time.strftime('%Y-%m-%d %H:%M UTC')}")
    print(f"  Duration:       {duration:.0f} hours ({duration_days:.1f} days)")
    print(f"  Days Covered:   {len(days_data)} days")
    print(f"  → SMHI provides ~10 days of hourly forecast data")

    print(f"\n📈 Daily Breakdown:")
    print(f"  Shows how many forecast entries exist for each day:")
    for i, (day, hours) in enumerate(sorted(days_data.items())):
        day_name = day.strftime("%A")
        is_today = " (Today)" if day == datetime.now().date() else ""
        print(f"  Day {i}: {day} {day_name:9}{is_today:8} - {len(hours):2} entries ({min(hours):02d}:00-{max(hours):02d}:00)")

    # Check intervals
    print(f"\n⏱️  Time Intervals Between Entries:")
    print(f"  How much time between consecutive forecasts:")
    intervals = []
    for i in range(min(10, len(data['timeSeries'])-1)):
        curr = datetime.fromisoformat(data['timeSeries'][i]['validTime'].replace('Z', '+00:00'))
        next_time = datetime.fromisoformat(data['timeSeries'][i+1]['validTime'].replace('Z', '+00:00'))
        diff = (next_time - curr).total_seconds() / 3600
        intervals.append(diff)
        if i < 5:
            print(f"  Entry {i:2} → {i+1:2}: {diff:4.1f} hours")

    avg_interval = sum(intervals) / len(intervals)
    print(f"  Average:       {avg_interval:.1f} hours")
    print(f"  → API provides hourly forecasts (1-hour intervals)")

    # All available parameters
    all_params = set()
    param_examples = {}
    param_counts = defaultdict(int)

    for entry in data.get("timeSeries", []):
        for param in entry.get("parameters", []):
            name = param.get("name")
            if name:
                all_params.add(name)
                param_counts[name] += 1
                if name not in param_examples:
                    param_examples[name] = {
                        'value': param.get("values", [None])[0],
                        'unit': param.get("unit", ""),
                        'level': param.get("level"),
                        'levelType': param.get("levelType")
                    }

    print(f"\n🔧 Available Weather Parameters:")
    print(f"  The API provides {len(all_params)} different weather measurements:")
    print(f"  {'Parameter':12} | {'Description':25} | {'Unit':8} | {'Example Value'}")
    print(f"  {'-'*12}-+-{'-'*25}-+-{'-'*8}-+-{'-'*20}")

    # Group parameters by category
    categories = {
        "Temperature": ["t"],
        "Wind": ["ws", "wd", "gust"],
        "Humidity/Pressure": ["r", "msl"],
        "Visibility": ["vis"],
        "Clouds": ["tcc_mean", "lcc_mean", "mcc_mean", "hcc_mean"],
        "Precipitation": ["pmean", "pmin", "pmax", "pmedian", "pcat", "tp"],
        "Weather Conditions": ["Wsymb2", "tstm", "spp"]
    }

    for category, params_in_cat in categories.items():
        printed_category = False
        for param in params_in_cat:
            if param in all_params:
                if not printed_category:
                    print(f"\n  [{category}]")
                    printed_category = True
                info = PARAMETER_INFO.get(param, (param, ""))
                example = param_examples[param]
                value_str = f"{example['value']}"
                if example['level'] is not None:
                    value_str += f" @{example['level']}{example['levelType']}"
                print(f"  {param:12} | {info[0]:25} | {info[1]:8} | {value_str}")

    print(f"\n💡 Key Insights:")
    print(f"  • API returns JSON with 'timeSeries' array")
    print(f"  • Each entry has 'validTime' and 'parameters' array")
    print(f"  • Covers ~{len(days_data)} days with {len(data['timeSeries'])} hourly forecasts")
    print(f"  • Provides {len(all_params)} different weather parameters per entry")
    print(f"  • Updated regularly (check 'approvedTime' for freshness)")
    print(f"  • Weather Symbol (Wsymb2): 1-27 representing different conditions")

    return days_data

# =============================================================================
# PART 2: ARDUINO-STYLE DATA PARSING (from fetch_all_smhi_data.py)
# =============================================================================

def parse_current_weather(data):
    """Parse current weather (first entry) - Arduino style"""
    if not data.get("timeSeries") or len(data["timeSeries"]) == 0:
        return None

    first_entry = data["timeSeries"][0]
    current = {
        "time": first_entry["validTime"],
        "parameters": {}
    }

    for param in first_entry.get("parameters", []):
        name = param.get("name")
        values = param.get("values", [])
        if name and values:
            current["parameters"][name] = values[0]

    return current

def parse_hourly_forecast(data):
    """Parse 3-hour forecast (Morning/Noon/Evening) - Arduino style"""
    target_hours = [8, 13, 19]  # 8 AM, 1 PM, 7 PM
    hour_labels = ["Morning", "Noon", "Evening"]
    hourly = {label: None for label in hour_labels}

    today = datetime.now().date()
    tomorrow = today + timedelta(days=1)

    for entry in data.get("timeSeries", []):
        time_str = entry["validTime"]
        dt = datetime.fromisoformat(time_str.replace('Z', '+00:00'))

        if dt.date() in [today, tomorrow]:
            hour = dt.hour

            for i, target_hour in enumerate(target_hours):
                if hour == target_hour and not hourly[hour_labels[i]]:
                    params = {}
                    for param in entry.get("parameters", []):
                        name = param.get("name")
                        values = param.get("values", [])
                        if name and values:
                            params[name] = values[0]

                    hourly[hour_labels[i]] = {
                        "time": time_str,
                        "hour": hour,
                        "parameters": params
                    }

    return hourly

def parse_daily_forecast(data):
    """Parse 6-day forecast - Arduino style"""
    daily_data = defaultdict(lambda: {
        "temp_min": 999,
        "temp_max": -999,
        "rain_chance": 0,
        "symbol": 0,
        "entries": []
    })

    for entry in data.get("timeSeries", []):
        time_str = entry["validTime"]
        dt = datetime.fromisoformat(time_str.replace('Z', '+00:00'))
        date = dt.date()

        params = {}
        for param in entry.get("parameters", []):
            name = param.get("name")
            values = param.get("values", [])
            if name and values:
                params[name] = values[0]

        if "t" in params:
            temp = params["t"]
            if temp < daily_data[date]["temp_min"]:
                daily_data[date]["temp_min"] = temp
            if temp > daily_data[date]["temp_max"]:
                daily_data[date]["temp_max"] = temp

        if "tstm" in params:
            if params["tstm"] > daily_data[date]["rain_chance"]:
                daily_data[date]["rain_chance"] = params["tstm"]

        if "Wsymb2" in params and daily_data[date]["symbol"] == 0:
            daily_data[date]["symbol"] = int(params["Wsymb2"])

        daily_data[date]["entries"].append({
            "time": dt.strftime("%H:%M"),
            "params": params
        })

    today = datetime.now().date()
    forecast_days = []

    for i in range(1, 7):
        target_date = today + timedelta(days=i)
        if target_date in daily_data:
            day_data = daily_data[target_date]
            forecast_days.append({
                "date": target_date,
                "day_name": target_date.strftime("%A"),
                "temp_min": day_data["temp_min"] if day_data["temp_min"] < 999 else None,
                "temp_max": day_data["temp_max"] if day_data["temp_max"] > -999 else None,
                "rain_chance": day_data["rain_chance"],
                "symbol": day_data["symbol"],
                "description": WEATHER_SYMBOLS.get(day_data["symbol"], "Unknown"),
                "entries_count": len(day_data["entries"])
            })

    return forecast_days

def display_arduino_style_data(current, hourly, forecast_days):
    """Display data as Arduino would use it"""
    print("\n" + "=" * 80)
    print("PART 2: ARDUINO-STYLE DATA PARSING")
    print("=" * 80)

    # Current Weather
    print("\n🌡️  CURRENT WEATHER:")
    if current:
        dt = datetime.fromisoformat(current["time"].replace('Z', '+00:00'))
        print(f"  Time:           {dt.strftime('%Y-%m-%d %H:%M UTC')}")
        params = current["parameters"]
        print(f"  Temperature:    {params.get('t', 'N/A')}°C")
        print(f"  Humidity:       {params.get('r', 'N/A')}%")
        print(f"  Wind:           {params.get('ws', 'N/A')} m/s {get_wind_direction(params.get('wd'))}")
        print(f"  Pressure:       {params.get('msl', 'N/A')} hPa")
        symbol = int(params.get('Wsymb2', 0))
        print(f"  Weather:        {WEATHER_SYMBOLS.get(symbol, 'Unknown')}")

    # Hourly Forecast
    print("\n🕐 3-HOUR FORECAST:")
    for label in ["Morning", "Noon", "Evening"]:
        data = hourly.get(label)
        if data:
            params = data["parameters"]
            print(f"  {label:8} ({data['hour']:02d}:00): {params.get('t', 'N/A')}°C, Wind {params.get('ws', 'N/A')} m/s, Rain {params.get('tstm', 'N/A')}%")
        else:
            print(f"  {label:8}: No data")

    # Daily Forecast
    print("\n📅 6-DAY FORECAST:")
    for i, day in enumerate(forecast_days, 1):
        temp_range = f"{day['temp_max']:.1f}/{day['temp_min']:.1f}°C" if day['temp_min'] else "N/A"
        print(f"  Day {i} ({day['day_name']:9}): {temp_range:12} Rain {day['rain_chance']:2.0f}% - {day['description']}")


# =============================================================================
# MAIN
# =============================================================================

def main():
    """Main function"""
    print("=" * 80)
    print("SMHI WEATHER API - UNIFIED TEST SCRIPT")
    print("=" * 80)

    # Fetch data
    data = fetch_weather_data()
    print("✅ Data fetched successfully!\n")

    # Part 1: Analyze API structure
    days_data = analyze_api_structure(data)

    # Part 2: Parse Arduino-style
    current = parse_current_weather(data)
    hourly = parse_hourly_forecast(data)
    forecast_days = parse_daily_forecast(data)
    display_arduino_style_data(current, hourly, forecast_days)

    print("\n" + "=" * 80)
    print("✅ TEST COMPLETE")
    print("=" * 80)

if __name__ == "__main__":
    main()
