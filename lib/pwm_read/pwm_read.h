/**
 * @file    pwm_read.h
 * @brief   Generic RC-PWM pulse-width capture for ESP32 (multi-channel).
 *
 * Reusable: takes the GPIO and a stale-timeout as parameters, so it has no
 * dependency on any application config. Measures the high-time of standard
 * RC servo/ESC pulses (typically 1000-2000 us) using a GPIO edge ISR plus
 * the esp_timer microsecond clock. Non-blocking; cheap for several channels.
 */
#ifndef PWM_READ_H
#define PWM_READ_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of channels supported by this driver. */
#define PWM_READ_MAX_CHANNELS  4

/**
 * @brief Install the shared GPIO ISR service. Call once before adding
 *        channels. Safe to call if the ISR service is already installed.
 */
void pwm_read_init(void);

/**
 * @brief Register a GPIO as an RC PWM input channel.
 * @param gpio          GPIO number to capture.
 * @param timeout_ms    if no valid pulse arrives within this many ms,
 *                      pwm_read_get_us() reports "no signal".
 * @return channel handle (>= 0) on success, -1 on failure.
 */
int pwm_read_add_channel(int gpio, uint32_t timeout_ms);

/**
 * @brief Get the most recent valid pulse width for a channel.
 * @param channel   handle returned by pwm_read_add_channel().
 * @param out_us    receives the pulse width in microseconds.
 * @return true if a fresh, valid pulse is available; false on timeout.
 */
bool pwm_read_get_us(int channel, uint32_t *out_us);

#ifdef __cplusplus
}
#endif

#endif /* PWM_READ_H */
