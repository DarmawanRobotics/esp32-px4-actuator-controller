/**
 * @file    pid.h
 * @brief   Generic PID controller (anti-windup, derivative-on-measurement).
 *
 * Reusable across projects: depends on nothing but the C standard library.
 * Output is clamped to [out_min, out_max]; the integrator only accumulates
 * when doing so does not push the output further into saturation
 * (conditional-integration anti-windup). Derivative is taken on the
 * measurement to avoid "derivative kick" on setpoint changes.
 */
#ifndef PID_H
#define PID_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float kp;
    float ki;
    float kd;

    float integrator;     /* accumulated integral term            */
    float prev_meas;      /* measurement from previous update     */

    float out_min;        /* output lower clamp                   */
    float out_max;        /* output upper clamp                   */

    bool  first_update;   /* skip derivative term on first sample */
} pid_t;

/**
 * @brief Initialise gains, output limits, and reset internal state.
 */
void  pid_init(pid_t *pid, float kp, float ki, float kd,
               float out_min, float out_max);

/**
 * @brief Update gains live without disturbing accumulated state.
 */
void  pid_set_gains(pid_t *pid, float kp, float ki, float kd);

/**
 * @brief Clear the integrator and derivative history.
 *        Call when (re)starting control or after a large setpoint jump.
 */
void  pid_reset(pid_t *pid);

/**
 * @brief Run one control step.
 * @param pid       controller instance
 * @param setpoint  desired value
 * @param measured  measured value
 * @param dt        time step in seconds (> 0)
 * @return control output, clamped to [out_min, out_max]
 */
float pid_update(pid_t *pid, float setpoint, float measured, float dt);

#ifdef __cplusplus
}
#endif

#endif /* PID_H */

