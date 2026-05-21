/**
 * @file    pwm_out.c
 * @brief   RC PWM output driver using ESP-IDF LEDC.
 */
#include "pwm_out.h"

#include "driver/ledc.h"

#define PWM_OUT_MODE              LEDC_LOW_SPEED_MODE
#define PWM_OUT_TIMER             LEDC_TIMER_0
#define PWM_OUT_CHANNEL           LEDC_CHANNEL_0

#define PWM_OUT_FREQUENCY         50
#define PWM_OUT_RESOLUTION        LEDC_TIMER_9_BIT

#define PWM_OUT_PERIOD_US         20000

#define PWM_OUT_MIN_US            1000
#define PWM_OUT_NEUTRAL_US        1500
#define PWM_OUT_MAX_US            2000

static uint32_t s_max_duty = 0;

void pwm_out_init(int gpio)
{
    s_max_duty =
        (1u << PWM_OUT_RESOLUTION) - 1u;

    const ledc_timer_config_t timer_cfg = {
        .speed_mode      = PWM_OUT_MODE,
        .timer_num       = PWM_OUT_TIMER,
        .duty_resolution = PWM_OUT_RESOLUTION,
        .freq_hz         = PWM_OUT_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
    };

    ledc_timer_config(&timer_cfg);

    const ledc_channel_config_t channel_cfg = {
        .gpio_num   = gpio,
        .speed_mode = PWM_OUT_MODE,
        .channel    = PWM_OUT_CHANNEL,
        .timer_sel  = PWM_OUT_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };

    ledc_channel_config(&channel_cfg);

    pwm_out_stop();
}

void pwm_out_write_us(uint16_t pulse_us)
{
    if (pulse_us < PWM_OUT_MIN_US) {
        pulse_us = PWM_OUT_MIN_US;
    }

    if (pulse_us > PWM_OUT_MAX_US) {
        pulse_us = PWM_OUT_MAX_US;
    }

    const uint32_t duty =
        ((uint64_t)pulse_us * s_max_duty) /
        PWM_OUT_PERIOD_US;

    ledc_set_duty(
        PWM_OUT_MODE,
        PWM_OUT_CHANNEL,
        duty
    );

    ledc_update_duty(
        PWM_OUT_MODE,
        PWM_OUT_CHANNEL
    );
}

void pwm_out_stop(void)
{
    pwm_out_write_us(PWM_OUT_MIN_US);
}