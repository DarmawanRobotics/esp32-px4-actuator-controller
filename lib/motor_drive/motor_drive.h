/**
 * @file    motor_drive.h
 * @brief   RC PWM motor / ESC output driver using ESP-IDF LEDC.
 *
 * Generates standard servo-style PWM signals:
 * - 50 Hz update rate
 * - 1000 us = minimum
 * - 1500 us = neutral
 * - 2000 us = maximum
 *
 * Suitable for:
 * - RC ESC
 * - PX4 PWM AUX output style
 * - Servo-compatible motor drivers
 */
#ifndef MOTOR_DRIVE_H
#define MOTOR_DRIVE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize RC PWM output.
 *
 * Configures one LEDC channel for standard 50 Hz RC PWM output.
 *
 * @param gpio Output GPIO pin.
 */
void motor_drive_init(int gpio);

/**
 * @brief Write RC PWM pulse width.
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
void motor_drive_write_us(uint16_t pulse_us);

/**
 * @brief Stop / disarm output.
 *
 * Sends minimum throttle signal (1000 us).
 */
void motor_drive_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_DRIVE_H */