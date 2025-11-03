# WiFi Simulation - How It Works

## Available Networks

The simulator provides 3 realistic WiFi networks:

### 1. HomeNetwork_5G (Strong Signal, Password Protected)
- **SSID:** `HomeNetwork_5G`
- **Password:** `MySecurePassword123`
- **RSSI:** -35 dBm (Strong signal)
- **Channel:** 36
- **Security:** WPA2-PSK

### 2. OfficeWiFi (Medium Signal, Password Protected)
- **SSID:** `OfficeWiFi`
- **Password:** `Office2024!`
- **RSSI:** -55 dBm (Medium signal)
- **Channel:** 6
- **Security:** WPA2-PSK

### 3. FreePublicWiFi (Weak Signal, Open)
- **SSID:** `FreePublicWiFi`
- **Password:** None (Open network)
- **RSSI:** -72 dBm (Weak signal)
- **Channel:** 11
- **Security:** Open (no encryption)

## Connection Behavior

### ✅ Successful Connection
```cpp
WiFi.begin("HomeNetwork_5G", "MySecurePassword123");  // SUCCESS
WiFi.begin("FreePublicWiFi");                          // SUCCESS (no password needed)
```

### ❌ Failed Connection Scenarios

**Wrong Network Name:**
```cpp
WiFi.begin("NonExistentNetwork", "password");
// Status: WL_NO_SSID_AVAIL
// Console: "Network 'NonExistentNetwork' not found!"
```

**Wrong Password:**
```cpp
WiFi.begin("HomeNetwork_5G", "WrongPassword");
// Status: WL_CONNECT_FAILED
// Console: "Incorrect password for 'HomeNetwork_5G'!"
```

**Missing Password for Secured Network:**
```cpp
WiFi.begin("OfficeWiFi");  // No password provided
// Status: WL_CONNECT_FAILED
// Console: "Network 'OfficeWiFi' requires a password!"
```

## Testing Your .ino File

To test different networks, modify your project.ino:

```cpp
// Change these lines:
const char* WIFI_SSID = "HomeNetwork_5G";
const char* WIFI_PASSWORD = "MySecurePassword123";

// Try different combinations:
// 1. Correct credentials - should connect
// 2. Wrong password - should fail
// 3. Wrong SSID - should fail
// 4. Open network without password - should connect
```

## Scanning Networks

You can scan for available networks:

```cpp
int n = WiFi.scanNetworks();
for (int i = 0; i < n; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.print(" dBm) ");
    Serial.println(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Secured");
}
```

## Customizing Networks

To add/modify networks, edit: `src/arduino_compat/WiFi.h`

Look for the `initializeNetworks()` function and add your own networks:

```cpp
SimulatedNetwork net4;
net4.ssid = "MyCustomNetwork";
net4.password = "MyPassword";
net4.rssi = -50;
net4.channel = 1;
net4.encryption = WIFI_AUTH_WPA2_PSK;
_networks.push_back(net4);
```

## Console Output Examples

When running the simulator, you'll see:

```
[WiFi] Initialized 3 simulated networks
[WiFi] Attempting to connect to 'HomeNetwork_5G'...
[WiFi] ✓ Connected to 'HomeNetwork_5G'!
[WiFi]   IP: 192.168.1.125
[WiFi]   RSSI: -36 dBm
[WiFi]   Channel: 36
[WiFi]   Security: WPA2-PSK
```

Or on failure:

```
[WiFi] Attempting to connect to 'WrongNetwork'...
[WiFi] ERROR: Network 'WrongNetwork' not found!
[WiFi] Available networks:
  - HomeNetwork_5G (Secured)
  - OfficeWiFi (Secured)
  - FreePublicWiFi (Open)
```
