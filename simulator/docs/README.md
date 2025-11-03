# Arduino PC Simulator Documentation

This directory contains documentation for the Arduino PC simulator.

## Available Documentation

- **[WIFI_SIMULATION.md](WIFI_SIMULATION.md)** - WiFi network simulation guide
  - Available networks and their credentials
  - Connection behavior and validation
  - Network scanning examples
  - Customization instructions

## Quick Start

1. **Build the simulator:**
   ```bash
   ./build.sh
   ```

2. **Run the simulator:**
   ```bash
   ./bin/main
   ```

3. **Modify your Arduino sketch:**
   - Edit `src/project.ino` with any Arduino code
   - No preprocessing needed - works directly!

## Project Structure

```
lv_port_pc_vscode/
├── src/
│   ├── arduino_compat/     # Arduino compatibility layer
│   │   ├── Arduino.h       # Core Arduino APIs
│   │   ├── WiFi.h          # WiFi simulation
│   │   ├── HTTPClient.h    # HTTP client (real internet via libcurl)
│   │   └── ...             # Other compatibility headers
│   ├── project.ino         # Your Arduino sketch
│   ├── project_wrapper.cpp # C++ wrapper for .ino file
│   ├── main.c              # Simulator entry point
│   └── hardware_mock.h     # Hardware mocking
├── build.sh                # Build script
├── CMakeLists.txt          # CMake configuration
└── docs/                   # Documentation
```

## Features

- ✅ Direct .ino file support (no preprocessing)
- ✅ Real WiFi/HTTP via libcurl
- ✅ Realistic WiFi network simulation with password validation
- ✅ Real ArduinoJson library
- ✅ Full Arduino API compatibility
- ✅ LVGL display simulation via SDL2
- ✅ Cross-platform (Linux/macOS/Windows with WSL)
