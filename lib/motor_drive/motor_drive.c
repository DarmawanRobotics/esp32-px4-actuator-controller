/**
 * @file    motor_drive.c
 * @brief   RC PWM motor / ESC output driver using ESP-IDF LEDC.
 */
#include "motor_drive.h"

#include "driver/ledc.h"

#define MOTOR_LEDC_MODE        LEDC_LOW_SPEED_MODE
#define MOTOR_LEDC_TIMER       LEDC_TIMER_0
#define MOTOR_LEDC_CHANNEL     LEDC_CHANNEL_0

#define MOTOR_PWM_FREQUENCY    50
#define MOTOR_PWM_RESOLUTION   LEDC_TIMER_9_BIT

#define MOTOR_PWM_PERIOD_US    20000

#define MOTOR_PWM_MIN_US       1000
#define MOTOR_PWM_NEUTRAL_US   1500
#define MOTOR_PWM_MAX_US       2000

static uint32_t s_max_duty = 0;

void motor_drive_init(int gpio)
{
    s_max_duty = (1u << 16) - 1u;

    const ledc_timer_config_t timer_cfg = {
        .speed_mode      = MOTOR_LEDC_MODE,
        .timer_num       = MOTOR_LEDC_TIMER,
        .duty_resolution = MOTOR_PWM_RESOLUTION,
        .freq_hz         = MOTOR_PWM_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
    };

    ledc_timer_config(&timer_cfg);

    const ledc_channel_config_t channel_cfg = {
        .gpio_num   = gpio,
        .speed_mode = MOTOR_LEDC_MODE,
        .channel    = MOTOR_LEDC_CHANNEL,
        .timer_sel  = MOTOR_LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };

    ledc_channel_config(&channel_cfg);

    motor_drive_stop();
}

void motor_drive_write_us(uint16_t pulse_us)
{
    if (pulse_us < MOTOR_PWM_MIN_US) {
        pulse_us = MOTOR_PWM_MIN_US;
    }

    if (pulse_us > MOTOR_PWM_MAX_US) {
        pulse_us = MOTOR_PWM_MAX_US;
    }

    const uint32_t duty =
        ((uint64_t)pulse_us * s_max_duty) / MOTOR_PWM_PERIOD_US;

    ledc_set_duty(
        MOTOR_LEDC_MODE,
        MOTOR_LEDC_CHANNEL,
        duty
    );

    ledc_update_duty(
        MOTOR_LEDC_MODE,
        MOTOR_LEDC_CHANNEL
    );
}

void motor_drive_stop(void)
{
    motor_drive_write_us(MOTOR_PWM_MIN_US);
}