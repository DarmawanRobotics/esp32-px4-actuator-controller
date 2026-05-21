/**
 * @file    encoder_read.c
 * @brief   RPM measurement via the ESP-IDF v5 pulse-counter (PCNT) driver.
 *          See encoder_read.h.
 */
#include "encoder_read.h"

#include "driver/pulse_cnt.h"
#include "esp_log.h"

static const char *TAG = "encoder_read";

/* Counter limits: wide enough not to overflow within one sample window. */
#define PCNT_HIGH_LIMIT   10000
#define PCNT_LOW_LIMIT   (-10000)
#define GLITCH_FILTER_NS  1000   /* reject pulses shorter than 1 us */

static pcnt_unit_handle_t    s_unit;
static pcnt_channel_handle_t s_channel;
static int                   s_slots_per_rev = 1;

void encoder_read_init(int gpio, int slots_per_rev)
{
    s_slots_per_rev = (slots_per_rev > 0) ? slots_per_rev : 1;

    const pcnt_unit_config_t unit_cfg = {
        .high_limit = PCNT_HIGH_LIMIT,
        .low_limit  = PCNT_LOW_LIMIT,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_cfg, &s_unit));

    const pcnt_glitch_filter_config_t filter_cfg = {
        .max_glitch_ns = GLITCH_FILTER_NS,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(s_unit, &filter_cfg));

    const pcnt_chan_config_t chan_cfg = {
        .edge_gpio_num  = gpio,
        .level_gpio_num = -1,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(s_unit, &chan_cfg, &s_channel));

    /* Count rising edges; ignore falling edges. */
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        s_channel,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_HOLD));

    ESP_ERROR_CHECK(pcnt_unit_enable(s_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(s_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(s_unit));

    ESP_LOGI(TAG, "PCNT ready: GPIO%d, %d slots/rev", gpio, s_slots_per_rev);
}

int encoder_read_raw_count(void)
{
    int count = 0;
    if (s_unit != NULL) {
        pcnt_unit_get_count(s_unit, &count);
    }
    return count;
}

float encoder_read_rpm(float dt_seconds)
{
    if (s_unit == NULL || dt_seconds <= 0.0f) {
        return 0.0f;
    }

    int pulses = 0;
    pcnt_unit_get_count(s_unit, &pulses);
    pcnt_unit_clear_count(s_unit);

    if (pulses < 0) {
        pulses = -pulses;
    }

    const float revolutions = (float)pulses / (float)s_slots_per_rev;
    return (revolutions / dt_seconds) * 60.0f;
}
