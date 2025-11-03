/**
 * @file IPAddress.h
 * @brief IP Address class for Arduino compatibility
 */

#pragma once

#include "Arduino.h"
#include <string>
#include <iostream>

class IPAddress {
private:
    uint8_t _address[4];

public:
    IPAddress() {
        _address[0] = 0;
        _address[1] = 0;
        _address[2] = 0;
        _address[3] = 0;
    }

    IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet) {
        _address[0] = first_octet;
        _address[1] = second_octet;
        _address[2] = third_octet;
        _address[3] = fourth_octet;
    }

    IPAddress(uint32_t address) {
        _address[0] = address & 0xFF;
        _address[1] = (address >> 8) & 0xFF;
        _address[2] = (address >> 16) & 0xFF;
        _address[3] = (address >> 24) & 0xFF;
    }

    IPAddress(const uint8_t* address) {
        memcpy(_address, address, sizeof(_address));
    }

    // Access operators
    uint8_t operator[](int index) const {
        return index >= 0 && index < 4 ? _address[index] : 0;
    }

    uint8_t& operator[](int index) {
        return _address[index];
    }

    // Comparison operators
    bool operator==(const IPAddress& addr) const {
        return memcmp(_address, addr._address, sizeof(_address)) == 0;
    }

    bool operator!=(const IPAddress& addr) const {
        return !(*this == addr);
    }

    // Conversion
    operator uint32_t() const {
        return (_address[0] | (_address[1] << 8) | (_address[2] << 16) | (_address[3] << 24));
    }

    String toString() const {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d.%d.%d.%d",
                 _address[0], _address[1], _address[2], _address[3]);
        return String(buf);
    }

    // Check if address is set
    operator bool() const {
        return _address[0] != 0 || _address[1] != 0 ||
               _address[2] != 0 || _address[3] != 0;
    }
};

// Pre-defined IP addresses
const IPAddress INADDR_NONE(0, 0, 0, 0);
