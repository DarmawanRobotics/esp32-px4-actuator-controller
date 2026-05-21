/**
 * @file    pwm_read.c
 * @brief   RC-PWM capture via GPIO edge ISR + esp_timer. See pwm_read.h.
 */
#include "pwm_read.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include <string.h>

/* Plausibility window for a captured pulse, in microseconds. Pulses
   outside this range are treated as noise and ignored. */
#define PULSE_MIN_VALID_US   700
#define PULSE_MAX_VALID_US   2500

typedef struct {
    int               gpio;
    uint32_t          timeout_us;     /* stale threshold                  */
    volatile int64_t  rise_us;        /* timestamp of last rising edge    */
    volatile uint32_t width_us;       /* last completed high-time         */
    volatile int64_t  last_pulse_us;  /* timestamp of last valid pulse    */
    bool              in_use;
} pwm_channel_t;

static pwm_channel_t s_channels[PWM_READ_MAX_CHANNELS];
static int           s_channel_count;
static bool          s_isr_installed;

/* Single ISR shared by all pins; the channel index is passed as arg. */
static void IRAM_ATTR pwm_read_isr(void *arg)
{
    const int idx = (int)(intptr_t)arg;
    pwm_channel_t *ch = &s_channels[idx];
    const int64_t now = esp_timer_get_time();

    if (gpio_get_level(ch->gpio)) {
        ch->rise_us = now;                     /* pulse started */
    } else {
        const int64_t width = now - ch->rise_us;
        if (width >= PULSE_MIN_VALID_US && width <= PULSE_MAX_VALID_US) {
            ch->width_us      = (uint32_t)width;
            ch->last_pulse_us = now;
        }
    }
}

void pwm_read_init(void)
{
    memset(s_channels, 0, sizeof(s_channels));
    s_channel_count = 0;

    if (!s_isr_installed) {
        gpio_install_isr_service(0);
        s_isr_installed = true;
    }
}

int pwm_read_add_channel(int gpio, uint32_t timeout_ms)
{
    if (s_channel_count >= PWM_READ_MAX_CHANNELS) {
        return -1;
    }

    const int idx = s_channel_count++;
    s_channels[idx].gpio       = gpio;
    s_channels[idx].timeout_us = timeout_ms * 1000u;
    s_channels[idx].in_use     = true;

    const gpio_config_t io = {
        .pin_bit_mask = (1ULL << gpio),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,   /* idle low between pulses */
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io);
    gpio_isr_handler_add(gpio, pwm_read_isr, (void *)(intptr_t)idx);
    return idx;
}

bool pwm_read_get_us(int channel, uint32_t *out_us)
{
    if (channel < 0 || channel >= s_channel_count ||
        !s_channels[channel].in_use || out_us == NULL) {
        return false;
    }

    const int64_t now = esp_timer_get_time();
    const int64_t age = now - s_channels[channel].last_pulse_us;
    if (age > (int64_t)s_channels[channel].timeout_us) {
        return false;                           /* stale -> failsafe */
    }

    *out_us = s_channels[channel].width_us;
    return true;
}


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
