/**
 * @file    motor_stats.c
 * @brief   Runtime RPM statistics. See motor_stats.h.
 *
 * The control loop writes samples while the web server reads snapshots from
 * another task, so all access is guarded by a mutex. Operations are tiny
 * (a few comparisons), so lock contention is negligible.
 */
#include "motor_stats.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <string.h>
#include <math.h>

static stats_bucket_t    s_overall;
static stats_bucket_t    s_mode[CONFIG_MODE_COUNT];
static SemaphoreHandle_t s_lock;

static void bucket_clear(stats_bucket_t *b)
{
    b->min_rpm = 0.0f;
    b->max_rpm = 0.0f;
    b->avg_rpm = 0.0f;
    b->samples = 0;
}

static void bucket_add(stats_bucket_t *b, float rpm)
{
    if (b->samples == 0) {
        b->min_rpm = rpm;
        b->max_rpm = rpm;
        b->avg_rpm = rpm;
        b->samples = 1;
        return;
    }

    if (rpm < b->min_rpm) {
        b->min_rpm = rpm;
    }
    if (rpm > b->max_rpm) {
        b->max_rpm = rpm;
    }

    /* Incremental running mean: avg += (x - avg) / n. */
    b->samples += 1;
    b->avg_rpm += (rpm - b->avg_rpm) / (float)b->samples;
}

static void ensure_lock(void)
{
    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateMutex();
    }
}

void motor_stats_reset(void)
{
    ensure_lock();
    xSemaphoreTake(s_lock, portMAX_DELAY);

    bucket_clear(&s_overall);
    for (int i = 0; i < CONFIG_MODE_COUNT; i++) {
        bucket_clear(&s_mode[i]);
    }

    xSemaphoreGive(s_lock);
}

void motor_stats_update(int mode, float rpm)
{
    if (rpm < 0.0f || isnan(rpm)) {
        return;
    }

    ensure_lock();
    xSemaphoreTake(s_lock, portMAX_DELAY);

    bucket_add(&s_overall, rpm);
    if (mode >= 0 && mode < CONFIG_MODE_COUNT) {
        bucket_add(&s_mode[mode], rpm);
    }

    xSemaphoreGive(s_lock);
}

void motor_stats_get_overall(stats_bucket_t *out)
{
    if (out == NULL) {
        return;
    }
    ensure_lock();
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_overall;
    xSemaphoreGive(s_lock);
}

void motor_stats_get_mode(int mode, stats_bucket_t *out)
{
    if (out == NULL) {
        return;
    }
    ensure_lock();
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (mode >= 0 && mode < CONFIG_MODE_COUNT) {
        *out = s_mode[mode];
    } else {
        memset(out, 0, sizeof(*out));
    }
    xSemaphoreGive(s_lock);
}