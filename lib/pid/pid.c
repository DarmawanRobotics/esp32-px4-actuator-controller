/**
 * @file    pid.c
 * @brief   Generic PID controller implementation. See pid.h.
 */
#include "pid.h"

void pid_init(pid_t *pid, float kp, float ki, float kd,
              float out_min, float out_max)
{
    pid->kp      = kp;
    pid->ki      = ki;
    pid->kd      = kd;
    pid->out_min = out_min;
    pid->out_max = out_max;
    pid_reset(pid);
}

void pid_set_gains(pid_t *pid, float kp, float ki, float kd)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
}

void pid_reset(pid_t *pid)
{
    pid->integrator   = 0.0f;
    pid->prev_meas    = 0.0f;
    pid->first_update = true;
}

float pid_update(pid_t *pid, float setpoint, float measured, float dt)
{
    if (dt <= 0.0f) {
        dt = 1e-3f;  /* guard against bad timing */
    }

    const float error = setpoint - measured;

    /* Proportional term. */
    const float p_term = pid->kp * error;

    /* Derivative on measurement: -Kd * d(measured)/dt. */
    float d_term = 0.0f;
    if (!pid->first_update) {
        d_term = -pid->kd * (measured - pid->prev_meas) / dt;
    }

    /* Tentative integrator and unclamped output. */
    const float candidate_integ = pid->integrator + pid->ki * error * dt;
    float output = p_term + candidate_integ + d_term;

    /* Clamp output and apply conditional-integration anti-windup. */
    if (output > pid->out_max) {
        if (error < 0.0f) {            /* integral can pull us back down */
            pid->integrator = candidate_integ;
        }
        output = pid->out_max;
    } else if (output < pid->out_min) {
        if (error > 0.0f) {            /* integral can pull us back up */
            pid->integrator = candidate_integ;
        }
        output = pid->out_min;
    } else {
        pid->integrator = candidate_integ;  /* unsaturated: accept */
    }

    pid->prev_meas    = measured;
    pid->first_update = false;
    return output;
}
