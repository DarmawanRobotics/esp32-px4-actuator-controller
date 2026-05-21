/**
 * @file    encoder_read.h
 * @brief   Generic speed (RPM) measurement from a single-channel encoder
 *          (e.g. slotted disc + photoelectric sensor) using PCNT.
 *
 * Reusable: the GPIO and slots-per-revolution are passed at init, so the
 * module has no application dependency. The PCNT peripheral counts pulses
 * in hardware with no CPU cost; call encoder_read_rpm() periodically.
 */
#ifndef ENCODER_READ_H
#define ENCODER_READ_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise the pulse counter.
 * @param gpio            sensor digital-output pin.
 * @param slots_per_rev   slots/holes on the disc (>= 1).
 */
void  encoder_read_init(int gpio, int slots_per_rev);

/**
 * @brief Compute RPM since the previous call, then reset the counter.
 * @param dt_seconds  elapsed time since the previous call (> 0).
 * @return measured RPM (>= 0).
 *
 * Call at a fixed cadence for a stable reading.
 */
float encoder_read_rpm(float dt_seconds);

/**
 * @brief Current raw pulse count (for debugging/diagnostics).
 */
int   encoder_read_raw_count(void);

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_READ_H */
