/**
 * @file WiFiClientSecure.h
 * @brief WiFi client for secure HTTPS connections using libcurl
 */

#pragma once

#include "Arduino.h"
#include <string>
#include <curl/curl.h>

class WiFiClient {
protected:
    std::string _responseData;
    size_t _readPos;
    bool _connected;

public:
    WiFiClient() : _readPos(0), _connected(false) {}
    virtual ~WiFiClient() {}

    void setResponseData(const std::string& data) {
        _responseData = data;
        _readPos = 0;
    }

    virtual int available() {
        return _readPos < _responseData.length() ?
               (_responseData.length() - _readPos) : 0;
    }

    virtual int read() {
        if (_readPos < _responseData.length()) {
            return _responseData[_readPos++];
        }
        return -1;
    }

    virtual int read(uint8_t* buf, size_t size) {
        size_t avail = available();
        if (avail == 0) return 0;

        size_t toRead = (size < avail) ? size : avail;
        memcpy(buf, _responseData.c_str() + _readPos, toRead);
        _readPos += toRead;
        return toRead;
    }

    virtual size_t write(uint8_t c) { return 1; }
    virtual size_t write(const uint8_t* buf, size_t size) { return size; }

    virtual bool find(const char* target) {
        if (!target) return false;

        size_t targetLen = strlen(target);
        while (available() > 0) {
            size_t remaining = _responseData.length() - _readPos;
            size_t searchLen = remaining < 1024 ? remaining : 1024;

            std::string chunk = _responseData.substr(_readPos, searchLen);
            size_t pos = chunk.find(target);

            if (pos != std::string::npos) {
                _readPos += pos + targetLen;
                return true;
            }

            // Move forward but keep some overlap for pattern matching
            if (remaining > targetLen) {
                _readPos += (searchLen - targetLen + 1);
            } else {
                _readPos = _responseData.length();
                return false;
            }
        }
        return false;
    }

    virtual bool findUntil(const char* target, const char* terminator) {
        if (!target || !terminator) return false;

        size_t startPos = _readPos;
        size_t targetLen = strlen(target);
        size_t termLen = strlen(terminator);

        while (available() > 0) {
            size_t remaining = _responseData.length() - _readPos;
            std::string chunk = _responseData.substr(_readPos, remaining);

            size_t targetPos = chunk.find(target);
            size_t termPos = chunk.find(terminator);

            if (termPos != std::string::npos &&
                (targetPos == std::string::npos || termPos < targetPos)) {
                _readPos = startPos;
                return false;
            }

            if (targetPos != std::string::npos) {
                _readPos += targetPos + targetLen;
                return true;
            }

            if (termPos != std::string::npos) {
                _readPos = startPos;
                return false;
            }

            _readPos++;
        }

        _readPos = startPos;
        return false;
    }

    virtual void stop() {
        _responseData.clear();
        _readPos = 0;
        _connected = false;
    }

    virtual bool connected() { return _connected; }
    operator bool() { return _connected; }
};

class WiFiClientSecure : public WiFiClient {
public:
    WiFiClientSecure() : WiFiClient() {}

    void setInsecure() {
        // For PC simulation, we accept all certificates
    }

    void setCACert(const char* rootCA) {
        // Mock - not needed for libcurl with SSL verification disabled
    }

    void setCertificate(const char* client_ca) {}
    void setPrivateKey(const char* private_key) {}

    // Add missing setTimeout method
    void setTimeout(int timeout_seconds) {
        // Mock - timeout is handled by libcurl internally
        (void)timeout_seconds;
    }
};
