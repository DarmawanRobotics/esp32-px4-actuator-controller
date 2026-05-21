/**
 * @file    main.c
 * @brief   ESP32-C3 PX4 actuator controller.
 *
 * Reads two RC-PWM inputs from a PX4 flight controller and decodes them as a
 * 2-bit mode selector (00/01/10/11 -> 4 modes). Each mode maps to a target
 * RPM. A closed-loop PID controller drives a brushed ESC (RC-PWM output) to
 * hold that RPM, using a photoelectric encoder for feedback. A WiFi SoftAP
 * web page allows live PID tuning, per-mode setpoint editing, and RPM
 * statistics for debugging.
 *
 * Arming / autonomous logic lives on the PX4 side; this firmware only follows
 * the mode it is told and regulates motor speed.
 *
 *   PX4 AUX1  --PWM-->  GPIO  (mode bit 0)
 *   PX4 AUX2  --PWM-->  GPIO  (mode bit 1)
 *   Encoder   --pulse-> GPIO  (RPM feedback)
 *   ESC       <--PWM--  GPIO  (motor command)
 */
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "driver/gpio.h"   /* GPIO_NUM_* pin macros */

#include "pwm_read.h"
#include "pwm_out.h"
#include "encoder_read.h"
#include "pid.h"

#include "config_store.h"
#include "motor_stats.h"
#include "web_config.h"

static const char *TAG = "main";

/* ------------------------------------------------------------------ */
/* Pin map  (adjust to your wiring; all GPIOs valid on ESP32-C3).     */
/* ------------------------------------------------------------------ */
#define PIN_MODE_BIT0     GPIO_NUM_2   /* PX4 AUX1 -> mode LSB         */
#define PIN_MODE_BIT1     GPIO_NUM_3   /* PX4 AUX2 -> mode MSB         */
#define PIN_ENCODER       GPIO_NUM_4   /* photoelectric encoder        */
#define PIN_ESC           GPIO_NUM_5   /* brushed ESC PWM out          */

/* ------------------------------------------------------------------ */
/* Tunables                                                            */
/* ------------------------------------------------------------------ */
#define ENCODER_SLOTS_PER_REV   20      /* slots/holes on the disc      */
#define MODE_PWM_THRESHOLD_US   1500    /* >= -> bit 1, < -> bit 0      */
#define PWM_INPUT_TIMEOUT_MS    100     /* PX4 signal stale threshold   */

#define CONTROL_PERIOD_MS       10      /* 100 Hz control loop          */
#define CONTROL_DT_S            (CONTROL_PERIOD_MS / 1000.0f)

/* ESC pulse range. The PID output is in microseconds and is clamped to
   this window. Mode 0 / target 0 forces the disarm pulse. */
#define ESC_MIN_US              1000    /* motor stopped                */
#define ESC_MAX_US              2000    /* full command                 */

/* WiFi access point credentials. */
#define AP_SSID                 "PX4-Actuator"
#define AP_PASSWORD             "actuator123"   /* >= 8 chars, or "" for open */

/* ------------------------------------------------------------------ */
/* Mode decoding                                                       */
/* ------------------------------------------------------------------ */

/* Read one PWM input channel and turn it into a single mode bit. A lost
   signal is treated as 0 so a disconnected PX4 falls back to "off". */
static int read_mode_bit(int channel)
{
    uint32_t us = 0;
    if (!pwm_read_get_us(channel, &us)) {
        return 0;
    }
    return (us >= MODE_PWM_THRESHOLD_US) ? 1 : 0;
}

/* Combine the two PWM channels into a 2-bit mode index (0..3). */
static int read_mode(int ch_bit0, int ch_bit1)
{
    const int b0 = read_mode_bit(ch_bit0);
    const int b1 = read_mode_bit(ch_bit1);
    return (b1 << 1) | b0;
}

/* ------------------------------------------------------------------ */
/* Application                                                         */
/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG, "PX4 actuator controller starting");

    /* Load persisted config (PID gains + per-mode setpoints). */
    app_config_t *cfg = config_store_init();

    /* Statistics start empty. */
    motor_stats_reset();

    /* Peripherals. */
    pwm_read_init();
    const int ch_bit0 = pwm_read_add_channel(PIN_MODE_BIT0, PWM_INPUT_TIMEOUT_MS);
    const int ch_bit1 = pwm_read_add_channel(PIN_MODE_BIT1, PWM_INPUT_TIMEOUT_MS);

    encoder_read_init(PIN_ENCODER, ENCODER_SLOTS_PER_REV);
    pwm_out_init(PIN_ESC);

    /* PID: output is an ESC pulse width in microseconds. */
    pid_t pid;
    pid_init(&pid, cfg->kp, cfg->ki, cfg->kd,
             (float)ESC_MIN_US, (float)ESC_MAX_US);

    /* Bring up WiFi + web server. */
    web_config_start(cfg, AP_SSID, AP_PASSWORD);

    ESP_LOGI(TAG, "running control loop at %d Hz", 1000 / CONTROL_PERIOD_MS);

    int prev_mode = -1;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        /* 1. Decode mode from the two PX4 PWM inputs. */
        const int mode = read_mode(ch_bit0, ch_bit1);
        const float target_rpm = cfg->setpoint_rpm[mode];

        /* 2. Measure current RPM. */
        const float measured_rpm = encoder_read_rpm(CONTROL_DT_S);

        /* 3. Keep PID gains in sync with anything changed over the web. */
        pid_set_gains(&pid, cfg->kp, cfg->ki, cfg->kd);

        /* On a mode change, clear PID history to avoid a derivative kick
           and stale integrator from the previous setpoint. */
        if (mode != prev_mode) {
            pid_reset(&pid);
            prev_mode = mode;
            ESP_LOGI(TAG, "mode -> %d%d (target %.0f RPM)",
                     (mode >> 1) & 1, mode & 1, target_rpm);
        }

        /* 4. Compute the ESC command. A zero target means "stop": skip the
              controller and send the disarm pulse so the integrator can't
              creep the motor. */
        float output_us;
        if (target_rpm <= 0.0f) {
            pid_reset(&pid);
            output_us = (float)ESC_MIN_US;
        } else {
            output_us = pid_update(&pid, target_rpm, measured_rpm, CONTROL_DT_S);
        }

        pwm_out_write_us((uint16_t)output_us);

        /* 5. Statistics + telemetry for the web UI. */
        motor_stats_update(mode, measured_rpm);

        const web_telemetry_t tel = {
            .mode         = mode,
            .target_rpm   = target_rpm,
            .measured_rpm = measured_rpm,
            .output_us    = output_us,
        };
        web_config_publish(&tel);

        /* 6. Fixed-cadence delay for a stable dt. */
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(CONTROL_PERIOD_MS));
    }
}