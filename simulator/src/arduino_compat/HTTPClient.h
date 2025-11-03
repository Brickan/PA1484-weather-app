/**
 * @file HTTPClient.h
 * @brief HTTP client implementation using libcurl for real internet access
 */

#pragma once

#include "Arduino.h"
#include "WiFiClientSecure.h"
#include <curl/curl.h>
#include <string>
#include <map>

// HTTP error constants
#define HTTPC_ERROR_CONNECTION_REFUSED  (-1)
#define HTTPC_ERROR_SEND_PAYLOAD_FAILED (-2)
#define HTTPC_ERROR_NOT_CONNECTED       (-3)

// Undefine Windows macros if they exist to avoid conflicts
#ifdef ERROR_CONNECTION_REFUSED
#undef ERROR_CONNECTION_REFUSED
#endif
#ifdef ERROR_NOT_CONNECTED
#undef ERROR_NOT_CONNECTED
#endif

#define ERROR_CONNECTION_REFUSED         HTTPC_ERROR_CONNECTION_REFUSED
#define ERROR_NOT_CONNECTED              HTTPC_ERROR_NOT_CONNECTED

// HTTP redirect follow mode constants
#define HTTPC_STRICT_FOLLOW_REDIRECTS    (2)
#define HTTPC_FORCE_FOLLOW_REDIRECTS     (1)
#define HTTPC_DISABLE_FOLLOW_REDIRECTS   (0)

// Callback for libcurl to write response data
static size_t _http_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    std::string* str = (std::string*)userp;
    str->append((char*)contents, total_size);
    return total_size;
}

class HTTPClient {
private:
    WiFiClient* _client;
    std::string _url;
    std::string _responseData;
    std::map<std::string, std::string> _headers;
    int _responseCode;
    CURL* _curl;
    int _timeout;

public:
    HTTPClient() : _client(nullptr), _responseCode(0), _curl(nullptr), _timeout(5000) {}

    ~HTTPClient() {
        end();
    }

    void setTimeout(int milliseconds) {
        _timeout = milliseconds;
    }

    bool begin(WiFiClientSecure& client, const char* url) {
        _client = &client;
        _url = url;
        return true;
    }

    bool begin(WiFiClient& client, const char* url) {
        _client = &client;
        _url = url;
        return true;
    }

    bool begin(const char* url) {
        _url = url;
        return true;
    }

    bool begin(String url) {
        _url = url.c_str();
        return true;
    }

    void addHeader(const String& name, const String& value) {
        _headers[name.c_str()] = value.c_str();
    }

    void addHeader(const char* name, const char* value) {
        _headers[name] = value;
    }

    int GET() {
        if (_url.empty()) return -1;

        printf("[HTTP] GET %s\n", _url.c_str());

        _curl = curl_easy_init();
        if (!_curl) {
            printf("[HTTP] Failed to initialize CURL\n");
            return -1;
        }

        _responseData.clear();

        curl_easy_setopt(_curl, CURLOPT_URL, _url.c_str());
        curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, _http_write_callback);
        curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_responseData);
        curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(_curl, CURLOPT_TIMEOUT, _timeout / 1000);
        curl_easy_setopt(_curl, CURLOPT_USERAGENT, "ESP32-Arduino/1.0");

        // Add custom headers
        struct curl_slist* headers_list = NULL;
        for (const auto& header : _headers) {
            std::string header_str = header.first + ": " + header.second;
            headers_list = curl_slist_append(headers_list, header_str.c_str());
        }
        if (headers_list) {
            curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, headers_list);
        }

        CURLcode res = curl_easy_perform(_curl);

        if (headers_list) {
            curl_slist_free_all(headers_list);
        }

        if (res != CURLE_OK) {
            printf("[HTTP] Request failed: %s\n", curl_easy_strerror(res));
            curl_easy_cleanup(_curl);
            _curl = nullptr;
            return -1;
        }

        long response_code;
        curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &response_code);
        _responseCode = (int)response_code;

        printf("[HTTP] Response code: %d, Size: %zu bytes\n",
               _responseCode, _responseData.length());

        // Pass response data to client for streaming
        if (_client) {
            _client->setResponseData(_responseData);
        }

        return _responseCode;
    }

    int POST(const String& payload) {
        return POST((uint8_t*)payload.c_str(), payload.length());
    }

    int POST(const char* payload) {
        return POST((uint8_t*)payload, strlen(payload));
    }

    int POST(uint8_t* payload, size_t size) {
        if (_url.empty()) return -1;

        printf("[HTTP] POST %s\n", _url.c_str());

        _curl = curl_easy_init();
        if (!_curl) {
            printf("[HTTP] Failed to initialize CURL\n");
            return -1;
        }

        _responseData.clear();

        curl_easy_setopt(_curl, CURLOPT_URL, _url.c_str());
        curl_easy_setopt(_curl, CURLOPT_POST, 1L);
        curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, payload);
        curl_easy_setopt(_curl, CURLOPT_POSTFIELDSIZE, size);
        curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, _http_write_callback);
        curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_responseData);
        curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(_curl, CURLOPT_TIMEOUT, _timeout / 1000);
        curl_easy_setopt(_curl, CURLOPT_USERAGENT, "ESP32-Arduino/1.0");

        // Add custom headers
        struct curl_slist* headers_list = NULL;
        for (const auto& header : _headers) {
            std::string header_str = header.first + ": " + header.second;
            headers_list = curl_slist_append(headers_list, header_str.c_str());
        }
        if (headers_list) {
            curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, headers_list);
        }

        CURLcode res = curl_easy_perform(_curl);

        if (headers_list) {
            curl_slist_free_all(headers_list);
        }

        if (res != CURLE_OK) {
            printf("[HTTP] Request failed: %s\n", curl_easy_strerror(res));
            curl_easy_cleanup(_curl);
            _curl = nullptr;
            return -1;
        }

        long response_code;
        curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &response_code);
        _responseCode = (int)response_code;

        printf("[HTTP] Response code: %d, Size: %zu bytes\n",
               _responseCode, _responseData.length());

        if (_client) {
            _client->setResponseData(_responseData);
        }

        return _responseCode;
    }

    String getString() {
        return String(_responseData.c_str());
    }

    WiFiClient* getStreamPtr() {
        return _client;
    }

    WiFiClient* getStream() {
        return _client;
    }

    int getSize() {
        return _responseData.length();
    }

    void end() {
        if (_curl) {
            curl_easy_cleanup(_curl);
            _curl = nullptr;
        }
        _responseData.clear();
        _responseCode = 0;
        _headers.clear();
    }

    String errorToString(int error) {
        // Return a String object so .c_str() can be called on it in project.ino
        const char* errorMsg = curl_easy_strerror((CURLcode)error);
        return String(errorMsg);
    }

    void setFollowRedirects(bool follow) {}
    void setFollowRedirects(int followMode) {
        // Mock - follow mode is set via CURLOPT_FOLLOWLOCATION
        (void)followMode;
    }
    void setRedirectLimit(int limit) {}
    void setUserAgent(const char* userAgent) {}
    void setAuthorization(const char* user, const char* password) {}
    void setAuthorization(const char* auth) {}

    // Add missing setReuse method
    void setReuse(bool reuse) {
        // Mock - connection reuse is handled by libcurl
        (void)reuse;
    }
};
