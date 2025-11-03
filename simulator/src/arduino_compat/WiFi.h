/**
 * @file WiFi.h
 * @brief WiFi functionality mock for PC simulation with realistic network simulation
 */

#pragma once

#include "Arduino.h"
#include "IPAddress.h"
#include <vector>

// WiFi status constants
#define WL_NO_SHIELD        255
#define WL_IDLE_STATUS      0
#define WL_NO_SSID_AVAIL    1
#define WL_SCAN_COMPLETED   2
#define WL_CONNECTED        3
#define WL_CONNECT_FAILED   4
#define WL_CONNECTION_LOST  5
#define WL_DISCONNECTED     6

// WiFi modes
typedef enum {
    WIFI_OFF = 0,
    WIFI_STA = 1,
    WIFI_AP = 2,
    WIFI_AP_STA = 3
} wifi_mode_t;

// WiFi power save modes
typedef enum {
    WIFI_PS_NONE = 0,
    WIFI_PS_MIN_MODEM = 1,
    WIFI_PS_MAX_MODEM = 2
} wifi_ps_type_t;

// WiFi transmit power
typedef enum {
    WIFI_POWER_19_5dBm = 78,
    WIFI_POWER_19dBm = 76,
    WIFI_POWER_18_5dBm = 74,
    WIFI_POWER_17dBm = 68,
    WIFI_POWER_15dBm = 60,
    WIFI_POWER_13dBm = 52,
    WIFI_POWER_11dBm = 44,
    WIFI_POWER_8_5dBm = 34,
    WIFI_POWER_7dBm = 28,
    WIFI_POWER_5dBm = 20,
    WIFI_POWER_2dBm = 8,
    WIFI_POWER_MINUS_1dBm = -4
} wifi_power_t;

// WiFi encryption types
typedef enum {
    WIFI_AUTH_OPEN = 0,
    WIFI_AUTH_WEP,
    WIFI_AUTH_WPA_PSK,
    WIFI_AUTH_WPA2_PSK,
    WIFI_AUTH_WPA_WPA2_PSK,
    WIFI_AUTH_WPA2_ENTERPRISE,
    WIFI_AUTH_WPA3_PSK,
    WIFI_AUTH_WPA2_WPA3_PSK,
    WIFI_AUTH_WAPI_PSK,
    WIFI_AUTH_MAX
} wifi_auth_mode_t;

// Simulated WiFi network
struct SimulatedNetwork {
    String ssid;
    String password;
    int rssi;
    uint8_t channel;
    wifi_auth_mode_t encryption;
};

class WiFiClass {
private:
    bool _connected;
    wifi_mode_t _mode;
    String _ssid;
    String _password;
    int _rssi;
    IPAddress _localIP;
    IPAddress _gatewayIP;
    IPAddress _subnetMask;
    IPAddress _dnsIP;
    String _macAddress;
    int _currentStatus;

    // Simulated available networks
    std::vector<SimulatedNetwork> _networks;
    int _currentNetworkIndex;

    void initializeNetworks() {
        _networks.clear();

        // Network 1: Strong signal, WPA2, with password
        SimulatedNetwork net1;
        net1.ssid = "APx";
        net1.password = "Password.Password";
        net1.rssi = -35;
        net1.channel = 36;
        net1.encryption = WIFI_AUTH_WPA2_PSK;
        _networks.push_back(net1);

        // Network 2: Medium signal, WPA2, with password
        SimulatedNetwork net2;
        net2.ssid = "OfficeWiFi";
        net2.password = "Office2024!";
        net2.rssi = -55;
        net2.channel = 6;
        net2.encryption = WIFI_AUTH_WPA2_PSK;
        _networks.push_back(net2);

        // Network 3: Weak signal, Open (no password)
        SimulatedNetwork net3;
        net3.ssid = "FreePublicWiFi";
        net3.password = "";
        net3.rssi = -72;
        net3.channel = 11;
        net3.encryption = WIFI_AUTH_OPEN;
        _networks.push_back(net3);

        printf("[WiFi] Initialized %d simulated networks\n", (int)_networks.size());
    }

    int findNetwork(const char* ssid) {
        for (size_t i = 0; i < _networks.size(); i++) {
            if (_networks[i].ssid == ssid) {
                return i;
            }
        }
        return -1;
    }

public:
    WiFiClass() : _connected(false), _mode(WIFI_OFF), _rssi(-45),
                  _localIP(192, 168, 1, 100),
                  _gatewayIP(192, 168, 1, 1),
                  _subnetMask(255, 255, 255, 0),
                  _dnsIP(8, 8, 8, 8),
                  _currentStatus(WL_DISCONNECTED),
                  _currentNetworkIndex(-1) {
        // Generate a fixed MAC address once during initialization
        char mac[18];
        snprintf(mac, sizeof(mac), "02:00:00:%02X:%02X:%02X",
                 (rand() % 256), (rand() % 256), (rand() % 256));
        _macAddress = String(mac);

        // Initialize simulated networks
        initializeNetworks();
    }

    // Connection management
    void mode(wifi_mode_t m) {
        _mode = m;
        if (m == WIFI_OFF) {
            _connected = false;
            _currentStatus = WL_DISCONNECTED;
        }
    }

    wifi_mode_t getMode() { return _mode; }

    void begin(const char* ssid, const char* password = NULL) {
        _ssid = ssid;
        _password = password ? password : "";
        _mode = WIFI_STA;
        _currentStatus = WL_IDLE_STATUS;

        printf("[WiFi] Attempting to connect to '%s'...\n", ssid);

        // Find the network
        int networkIndex = findNetwork(ssid);

        if (networkIndex < 0) {
            // Network not found
            printf("[WiFi] ERROR: Network '%s' not found!\n", ssid);
            printf("[WiFi] Available networks:\n");
            for (size_t i = 0; i < _networks.size(); i++) {
                printf("  - %s (%s)\n", _networks[i].ssid.c_str(),
                       _networks[i].encryption == WIFI_AUTH_OPEN ? "Open" : "Secured");
            }
            _connected = false;
            _currentStatus = WL_NO_SSID_AVAIL;
            _currentNetworkIndex = -1;
            return;
        }

        // Check password
        const SimulatedNetwork& network = _networks[networkIndex];

        if (network.encryption != WIFI_AUTH_OPEN) {
            // Network requires password
            if (_password.length() == 0) {
                printf("[WiFi] ERROR: Network '%s' requires a password!\n", ssid);
                _connected = false;
                _currentStatus = WL_CONNECT_FAILED;
                _currentNetworkIndex = -1;
                return;
            }

            if (network.password != _password) {
                printf("[WiFi] ERROR: Incorrect password for '%s'!\n", ssid);
                printf("[WiFi] Hint: Password is '%s'\n", network.password.c_str());
                _connected = false;
                _currentStatus = WL_CONNECT_FAILED;
                _currentNetworkIndex = -1;
                return;
            }
        }

        // Simulate connection delay
        delay(500);

        // Successfully connected
        _connected = true;
        _currentStatus = WL_CONNECTED;
        _currentNetworkIndex = networkIndex;
        _rssi = network.rssi + (rand() % 5 - 2); // Small variation
        _localIP = IPAddress(192, 168, 1, 100 + (rand() % 50));

        printf("[WiFi] ✓ Connected to '%s'!\n", ssid);
        printf("[WiFi]   IP: %s\n", _localIP.toString().c_str());
        printf("[WiFi]   RSSI: %d dBm\n", _rssi);
        printf("[WiFi]   Channel: %d\n", network.channel);
        printf("[WiFi]   Security: %s\n",
               network.encryption == WIFI_AUTH_OPEN ? "Open" : "WPA2-PSK");
    }

    int status() {
        return _currentStatus;
    }

    void disconnect(bool wifiOff = false, bool eraseAp = false) {
        _connected = false;
        _currentStatus = WL_DISCONNECTED;
        _currentNetworkIndex = -1;
        if (wifiOff) _mode = WIFI_OFF;
        printf("[WiFi] Disconnected\n");
    }

    void reconnect() {
        if (_ssid.length() == 0) return;
        begin(_ssid.c_str(), _password.c_str());
    }

    // Network scanning
    int16_t scanNetworks(bool async = false, bool show_hidden = false, bool passive = false, uint32_t max_ms_per_chan = 300) {
        (void)async;
        (void)show_hidden;
        (void)passive;
        (void)max_ms_per_chan;

        printf("[WiFi] Scanning for networks...\n");
        delay(100); // Simulate scan time

        printf("[WiFi] Found %d networks:\n", (int)_networks.size());
        for (size_t i = 0; i < _networks.size(); i++) {
            printf("  %d: %s (RSSI: %d, Ch: %d, %s)\n",
                   (int)i,
                   _networks[i].ssid.c_str(),
                   _networks[i].rssi,
                   _networks[i].channel,
                   _networks[i].encryption == WIFI_AUTH_OPEN ? "Open" : "Secured");
        }

        return _networks.size();
    }

    String SSID(uint8_t networkItem) {
        if (networkItem < _networks.size()) {
            return _networks[networkItem].ssid;
        }
        return String("");
    }

    int32_t RSSI(uint8_t networkItem) {
        if (networkItem < _networks.size()) {
            return _networks[networkItem].rssi;
        }
        return 0;
    }

    wifi_auth_mode_t encryptionType(uint8_t networkItem) {
        if (networkItem < _networks.size()) {
            return _networks[networkItem].encryption;
        }
        return WIFI_AUTH_OPEN;
    }

    uint8_t* BSSID(uint8_t networkItem) {
        static uint8_t bssid[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
        if (networkItem < _networks.size()) {
            bssid[3] = networkItem;
            bssid[4] = rand() % 256;
            bssid[5] = rand() % 256;
        }
        return bssid;
    }

    uint8_t channel(uint8_t networkItem) {
        if (networkItem < _networks.size()) {
            return _networks[networkItem].channel;
        }
        return 0;
    }

    void scanDelete() {
        // Networks are persistent in simulation
    }

    // Network information
    IPAddress localIP() { return _localIP; }
    IPAddress gatewayIP() { return _gatewayIP; }
    IPAddress subnetMask() { return _subnetMask; }
    IPAddress dnsIP(uint8_t dns_no = 0) { return _dnsIP; }

    String localIPStr() { return _localIP.toString(); }
    String SSID() { return _ssid; }

    int RSSI() {
        if (!_connected || _currentNetworkIndex < 0) return 0;
        // Return current network RSSI with slight variation
        _rssi = _networks[_currentNetworkIndex].rssi + (rand() % 5 - 2);
        return _rssi;
    }

    String macAddress() {
        // Return the fixed MAC address that was generated during initialization
        return _macAddress;
    }

    uint8_t channel() {
        if (_connected && _currentNetworkIndex >= 0) {
            return _networks[_currentNetworkIndex].channel;
        }
        return 0;
    }

    // Advanced configuration
    bool config(IPAddress local_ip, IPAddress gateway, IPAddress subnet,
                IPAddress dns1 = IPAddress(0,0,0,0), IPAddress dns2 = IPAddress(0,0,0,0)) {
        _localIP = local_ip;
        _gatewayIP = gateway;
        _subnetMask = subnet;
        if (dns1[0] != 0) _dnsIP = dns1;
        return true;
    }

    // Power management
    void setSleep(bool enable) {}
    void setTxPower(wifi_power_t power) {}
    void setAutoConnect(bool autoConnect) {}
    void setAutoReconnect(bool autoReconnect) {}
    void persistent(bool persistent) {}

    // Hostname
    bool setHostname(const char* hostname) { return true; }
    String getHostname() { return String("esp32-sim"); }

    // Operators
    operator bool() { return _connected; }
};

// Global WiFi object
static WiFiClass WiFi;
