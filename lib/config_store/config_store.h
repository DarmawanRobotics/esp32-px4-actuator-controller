/**
 * @file    config_store.h
 * @brief   Persistent configuration storage in NVS flash.
 *
 * Stores the data that must survive a reboot:
 *   - PID gains (kp, ki, kd)
 *   - The four per-mode RPM setpoints (mode 00/01/10/11)
 *
 * Runtime statistics (min/max/avg RPM) are intentionally NOT stored here;
 * they live in RAM only (see motor_stats).
 *
 * The store keeps an in-RAM copy that the application reads directly; call
 * config_store_save() after a change to persist it. A version field allows
 * future migrations and guards against reading an incompatible layout.
 */
#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Number of selectable modes (2 PWM inputs -> 2 bits -> 4 modes). */
#define CONFIG_MODE_COUNT  4

/**
 * @brief Persistent configuration block.
 */
typedef struct {
    float    kp;                          /* PID proportional gain        */
    float    ki;                          /* PID integral gain            */
    float    kd;                          /* PID derivative gain          */
    float    setpoint_rpm[CONFIG_MODE_COUNT]; /* target RPM per mode      */
} app_config_t;

/**
 * @brief Initialise NVS and load the configuration into RAM.
 *
 * If no valid stored config is found (first boot or layout change), the
 * built-in defaults are applied and written back to flash.
 *
 * @return pointer to the in-RAM configuration (never NULL).
 */
app_config_t *config_store_init(void);

/**
 * @brief Get the in-RAM configuration pointer.
 */
app_config_t *config_store_get(void);

/**
 * @brief Persist the current in-RAM configuration to NVS.
 * @return true on success.
 */
bool config_store_save(void);

/**
 * @brief Reset the in-RAM configuration to defaults and persist it.
 * @return true on success.
 */
bool config_store_reset_defaults(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_STORE_H */
