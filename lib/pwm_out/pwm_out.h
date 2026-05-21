/**
 * @file    pwm_out.h
 * @brief   RC PWM output driver using ESP-IDF LEDC.
 *
 * Generates standard servo-style PWM signals:
 * - 50 Hz update rate
 * - 1000 us = minimum
 * - 1500 us = neutral
 * - 2000 us = maximum
 *
 * Suitable for:
 * - RC ESC
 * - PX4 PWM AUX outputs
 * - Servos
 * - PWM actuators
 */
#ifndef PWM_OUT_H
#define PWM_OUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize PWM output.
 *
 * Configures one LEDC channel for standard RC PWM output.
 *
 * @param gpio Output GPIO pin.
 */
void pwm_out_init(int gpio);

/**
 * @brief Write PWM pulse width.
 *
 * Valid range:
 * - 1000 us = minimum
 * - 1500 us = neutral
 * - 2000 us = maximum
 *
 * Values are automatically clamped.
 *
 * @param pulse_us Pulse width in microseconds.
 */
void pwm_out_write_us(uint16_t pulse_us);

/**
 * @brief Stop / disarm output.
 *
 * Sends minimum PWM signal (1000 us).
 */
void pwm_out_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* PWM_OUT_H */