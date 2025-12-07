/**
 * @file Preferences.h
 * @brief ESP32 Preferences library mock for simulator with file persistence
 *
 * This implementation stores data in a file (simulator_preferences.txt) so settings
 * survive simulator restarts, just like real ESP32 NVS flash storage.
 */

#pragma once

#include <map>
#include <string>
#include <cstdint>
#include <fstream>
#include <sstream>
#include "Arduino.h"

class Preferences {
private:
    static std::map<std::string, int> intStorage;
    static std::map<std::string, std::string> stringStorage;
    static bool storageLoaded;
    std::string currentNamespace;
    bool readOnly;

    // File path for persistent storage
    static const char* getStorageFilePath() {
        return "simulator_preferences.txt";
    }

    // Load data from file
    static void loadFromFile() {
        if (storageLoaded) return;

        std::ifstream file(getStorageFilePath());
        if (!file.is_open()) {
            printf("[Preferences] No saved data found, starting fresh\n");
            storageLoaded = true;
            return;
        }

        std::string line;
        int intCount = 0, stringCount = 0;

        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            size_t equalPos = line.find('=');
            if (equalPos == std::string::npos) continue;

            std::string key = line.substr(0, equalPos);
            std::string value = line.substr(equalPos + 1);

            // Determine if it's an int or string
            if (value.length() > 0 && value[0] == 'i') {
                // Integer: "i:12345"
                int intValue = std::stoi(value.substr(2));
                intStorage[key] = intValue;
                intCount++;
            } else if (value.length() > 0 && value[0] == 's') {
                // String: "s:Hello World"
                std::string stringValue = value.substr(2);
                stringStorage[key] = stringValue;
                stringCount++;
            }
        }

        file.close();
        printf("[Preferences] Loaded from file: %d integers, %d strings\n", intCount, stringCount);
        storageLoaded = true;
    }

    // Save data to file
    static void saveToFile() {
        std::ofstream file(getStorageFilePath());
        if (!file.is_open()) {
            printf("[Preferences] ERROR: Cannot write to file\n");
            return;
        }

        file << "# ESP32 Simulator Preferences Storage\n";
        file << "# This file persists settings across simulator sessions\n";
        file << "# Format: namespace:key=type:value\n\n";

        // Save integers
        for (const auto& pair : intStorage) {
            file << pair.first << "=i:" << pair.second << "\n";
        }

        // Save strings
        for (const auto& pair : stringStorage) {
            file << pair.first << "=s:" << pair.second << "\n";
        }

        file.close();
        printf("[Preferences] Data saved to file\n");
    }

public:
    Preferences() : readOnly(false) {
        loadFromFile();
    }

    ~Preferences() {}

    /**
     * Open Preferences with namespace
     * @param name Namespace name
     * @param readOnly true = read-only, false = read-write
     * @return true on success
     */
    bool begin(const char* name, bool readOnly = false) {
        loadFromFile(); // Ensure data is loaded
        currentNamespace = std::string(name);
        this->readOnly = readOnly;
        return true;
    }

    /**
     * Close the Preferences
     */
    void end() {
        // Nothing to do
    }

    /**
     * Clear all keys in current namespace
     * @return true on success
     */
    bool clear() {
        if (readOnly) return false;

        // Clear only keys that belong to current namespace
        auto it = intStorage.begin();
        while (it != intStorage.end()) {
            if (it->first.find(currentNamespace + ":") == 0) {
                it = intStorage.erase(it);
            } else {
                ++it;
            }
        }

        auto it2 = stringStorage.begin();
        while (it2 != stringStorage.end()) {
            if (it2->first.find(currentNamespace + ":") == 0) {
                it2 = stringStorage.erase(it2);
            } else {
                ++it2;
            }
        }

        saveToFile();
        return true;
    }

    /**
     * Remove a key
     * @param key Key name
     * @return true on success
     */
    bool remove(const char* key) {
        if (readOnly) return false;

        std::string fullKey = currentNamespace + ":" + std::string(key);
        intStorage.erase(fullKey);
        stringStorage.erase(fullKey);
        saveToFile();
        return true;
    }

    // ========== PUT METHODS (Write) ==========

    /**
     * Store an integer value
     */
    size_t putInt(const char* key, int32_t value) {
        if (readOnly) return 0;

        std::string fullKey = currentNamespace + ":" + std::string(key);
        intStorage[fullKey] = value;

        printf("[Preferences] Saved: %s = %d\n", fullKey.c_str(), value);
        saveToFile(); // Persist to disk
        return sizeof(int32_t);
    }

    /**
     * Store an unsigned integer value
     */
    size_t putUInt(const char* key, uint32_t value) {
        return putInt(key, (int32_t)value);
    }

    /**
     * Store a string value
     */
    size_t putString(const char* key, const char* value) {
        if (readOnly) return 0;

        std::string fullKey = currentNamespace + ":" + std::string(key);
        stringStorage[fullKey] = std::string(value);

        printf("[Preferences] Saved: %s = %s\n", fullKey.c_str(), value);
        saveToFile(); // Persist to disk
        return strlen(value);
    }

    /**
     * Store a string value (String object)
     */
    size_t putString(const char* key, String value) {
        return putString(key, value.c_str());
    }

    // ========== GET METHODS (Read) ==========

    /**
     * Get an integer value
     * @param key Key name
     * @param defaultValue Default value if key doesn't exist
     */
    int32_t getInt(const char* key, int32_t defaultValue = 0) {
        std::string fullKey = currentNamespace + ":" + std::string(key);

        auto it = intStorage.find(fullKey);
        if (it != intStorage.end()) {
            printf("[Preferences] Loaded: %s = %d\n", fullKey.c_str(), it->second);
            return it->second;
        }

        printf("[Preferences] Not found: %s, using default = %d\n", fullKey.c_str(), defaultValue);
        return defaultValue;
    }

    /**
     * Get an unsigned integer value
     */
    uint32_t getUInt(const char* key, uint32_t defaultValue = 0) {
        return (uint32_t)getInt(key, (int32_t)defaultValue);
    }

    /**
     * Get a string value
     * @param key Key name
     * @param defaultValue Default value if key doesn't exist
     */
    String getString(const char* key, const char* defaultValue = "") {
        std::string fullKey = currentNamespace + ":" + std::string(key);

        auto it = stringStorage.find(fullKey);
        if (it != stringStorage.end()) {
            printf("[Preferences] Loaded: %s = %s\n", fullKey.c_str(), it->second.c_str());
            return String(it->second.c_str());
        }

        printf("[Preferences] Not found: %s, using default = %s\n", fullKey.c_str(), defaultValue);
        return String(defaultValue);
    }

    /**
     * Get a string value (String default)
     */
    String getString(const char* key, String defaultValue) {
        return getString(key, defaultValue.c_str());
    }

    /**
     * Check if a key exists
     */
    bool isKey(const char* key) {
        std::string fullKey = currentNamespace + ":" + std::string(key);
        return (intStorage.find(fullKey) != intStorage.end()) ||
               (stringStorage.find(fullKey) != stringStorage.end());
    }

    // Additional methods for compatibility
    size_t putBool(const char* key, bool value) { return putInt(key, value ? 1 : 0); }
    size_t putChar(const char* key, int8_t value) { return putInt(key, value); }
    size_t putUChar(const char* key, uint8_t value) { return putInt(key, value); }
    size_t putShort(const char* key, int16_t value) { return putInt(key, value); }
    size_t putUShort(const char* key, uint16_t value) { return putInt(key, value); }
    size_t putLong(const char* key, int32_t value) { return putInt(key, value); }
    size_t putULong(const char* key, uint32_t value) { return putInt(key, value); }

    bool getBool(const char* key, bool defaultValue = false) { return getInt(key, defaultValue ? 1 : 0) != 0; }
    int8_t getChar(const char* key, int8_t defaultValue = 0) { return (int8_t)getInt(key, defaultValue); }
    uint8_t getUChar(const char* key, uint8_t defaultValue = 0) { return (uint8_t)getInt(key, defaultValue); }
    int16_t getShort(const char* key, int16_t defaultValue = 0) { return (int16_t)getInt(key, defaultValue); }
    uint16_t getUShort(const char* key, uint16_t defaultValue = 0) { return (uint16_t)getInt(key, defaultValue); }
    int32_t getLong(const char* key, int32_t defaultValue = 0) { return getInt(key, defaultValue); }
    uint32_t getULong(const char* key, uint32_t defaultValue = 0) { return getUInt(key, defaultValue); }
};

// Static member initialization
std::map<std::string, int> Preferences::intStorage;
std::map<std::string, std::string> Preferences::stringStorage;
bool Preferences::storageLoaded = false;
