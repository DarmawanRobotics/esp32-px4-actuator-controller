/**
 * @file    encoder_read.c
 * @brief   RPM measurement via GPIO interrupt + esp_timer.
 *          Drop-in replacement for the PCNT-based implementation.
 *          ESP32-C3 does not have a hardware PCNT peripheral, so we count
 *          rising edges in a GPIO ISR and snapshot the counter periodically.
 */
#include "encoder_read.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "esp_log.h"

#include <stdint.h>

static const char *TAG = "encoder_read";

static volatile uint32_t s_pulse_count  = 0;
static int               s_slots_per_rev = 1;
static int               s_gpio          = -1;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    s_pulse_count++;
}

void encoder_read_init(int gpio, int slots_per_rev)
{
    s_gpio          = gpio;
    s_slots_per_rev = (slots_per_rev > 0) ? slots_per_rev : 1;
    s_pulse_count   = 0;

    const gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_POSEDGE,   /* count rising edges only */
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(gpio, gpio_isr_handler, NULL);

    ESP_LOGI(TAG, "encoder ready: GPIO%d, %d slots/rev (GPIO ISR mode)",
             gpio, s_slots_per_rev);
}

int encoder_read_raw_count(void)
{
    return (int)s_pulse_count;
}

float encoder_read_rpm(float dt_seconds)
{
    if (s_gpio < 0 || dt_seconds <= 0.0f) {
        return 0.0f;
    }

    /* Atomically snapshot and reset the counter. */
    uint32_t pulses = s_pulse_count;
    s_pulse_count   = 0;

    const float revolutions = (float)pulses / (float)s_slots_per_rev;
    return (revolutions / dt_seconds) * 60.0f;
}