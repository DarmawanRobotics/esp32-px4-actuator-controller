/**
 * @file    web_config.h
 * @brief   WiFi SoftAP + HTTP server for live tuning and debugging.
 *
 * Brings up the ESP32 as its own WiFi access point and serves a small web UI
 * that lets you, without recompiling:
 *   - tune the PID gains (kp, ki, kd) live,
 *   - change the RPM setpoint for each of the four modes,
 *   - watch live telemetry (active mode, target RPM, measured RPM),
 *   - inspect min/max/avg RPM statistics (overall and per mode),
 *   - reset the statistics,
 *   - persist gains + setpoints to flash.
 *
 * Gains and setpoints are owned by config_store; this module edits that
 * shared config in place and calls config_store_save() when asked to persist.
 *
 * Threading: the HTTP handlers run in the server task and the control loop
 * runs in app_main's task. They communicate through a small shared,
 * mutex-protected telemetry block (see web_config_publish / the getters used
 * internally). Config edits made over HTTP take effect on the loop's next
 * iteration because both sides point at the same app_config_t.
 */
#ifndef WEB_CONFIG_H
#define WEB_CONFIG_H

#include <stdbool.h>

#include "config_store.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Live telemetry the control loop publishes for the web UI to display. */
typedef struct {
    int   mode;            /* active mode 0..3                  */
    float target_rpm;      /* current setpoint                  */
    float measured_rpm;    /* latest measured RPM               */
    float output_us;       /* last PWM pulse sent to the ESC    */
} web_telemetry_t;

/**
 * @brief Start WiFi SoftAP and the HTTP server.
 *
 * @param cfg       shared configuration (from config_store_init); the web
 *                  handlers edit this in place. Must remain valid for the
 *                  lifetime of the server.
 * @param ssid      access-point SSID.
 * @param password  access-point password (>= 8 chars), or NULL/"" for an
 *                  open network.
 */
void web_config_start(app_config_t *cfg, const char *ssid, const char *password);

/**
 * @brief Publish the latest telemetry snapshot for the web UI.
 *        Called from the control loop each iteration; cheap and non-blocking.
 */
void web_config_publish(const web_telemetry_t *telemetry);

#ifdef __cplusplus
}
#endif

#endif /* WEB_CONFIG_H */