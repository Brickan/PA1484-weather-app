/**
 * @file esp_sntp.h
 * @brief SNTP (time synchronization) mock using system time
 */

#pragma once

#include <sys/time.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SNTP_OPMODE_POLL 0
#define SNTP_OPMODE_LISTENONLY 1

typedef void (*sntp_sync_time_cb_t)(struct timeval *tv);

static inline void sntp_set_time_sync_notification_cb(sntp_sync_time_cb_t callback) {
    if (callback) {
        // Immediately simulate time sync on PC
        struct timeval tv;
        gettimeofday(&tv, NULL);
        callback(&tv);
    }
}

static inline void sntp_setoperatingmode(int mode) {
    // Mock
}

static inline void sntp_setservername(int idx, const char* server) {
    printf("[SNTP] Server %d: %s\n", idx, server);
}

static inline void sntp_init() {
    printf("[SNTP] Initialized (using system time on PC)\n");
}

static inline void sntp_stop() {
    printf("[SNTP] Stopped\n");
}

// Override setenv/tzset to ignore TZ on simulator - use PC's local time
#define setenv(name, value, overwrite) \
    ((strcmp(name, "TZ") == 0) ? (printf("[Simulator] Ignoring TZ setting, using PC local time\n"), 0) : 0)

#define tzset() do { /* No-op on simulator */ } while(0)

#ifdef __cplusplus
}
#endif
