/**
 * @file    motor_stats.h
 * @brief   Runtime RPM statistics for debugging (RAM only, resettable).
 *
 * Tracks min / max / average measured RPM, both overall and broken down per
 * mode, so you can verify on the web page that the loop actually reaches its
 * targets. Nothing here is persisted; everything clears on reboot or reset.
 *
 * The average is a running mean over all samples fed in since the last reset,
 * computed incrementally so memory use is constant.
 */
#ifndef MOTOR_STATS_H
#define MOTOR_STATS_H

#include <stdint.h>

#include "config_store.h"   /* CONFIG_MODE_COUNT */

#ifdef __cplusplus
extern "C" {
#endif

/** Aggregated statistics for one bucket (overall or a single mode). */
typedef struct {
    float    min_rpm;     /* lowest sample seen     */
    float    max_rpm;     /* highest sample seen    */
    float    avg_rpm;     /* running mean           */
    uint32_t samples;     /* number of samples      */
} stats_bucket_t;

/**
 * @brief Clear all statistics (overall and per-mode).
 */
void motor_stats_reset(void);

/**
 * @brief Feed one RPM sample taken while running in @p mode.
 * @param mode  active mode index (0..CONFIG_MODE_COUNT-1).
 * @param rpm   measured RPM for this sample (>= 0).
 */
void motor_stats_update(int mode, float rpm);

/**
 * @brief Copy out the overall statistics bucket.
 */
void motor_stats_get_overall(stats_bucket_t *out);

/**
 * @brief Copy out the statistics bucket for a single mode.
 * @param mode  mode index (0..CONFIG_MODE_COUNT-1).
 */
void motor_stats_get_mode(int mode, stats_bucket_t *out);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_STATS_H */