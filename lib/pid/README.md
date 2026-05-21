# PID Controller Library

Generic reusable PID controller written in pure C.

Features:
- Proportional control
- Integral control
- Derivative-on-measurement
- Anti-windup protection
- Output clamping
- Lightweight and dependency-free

Suitable for:
- Motor speed control
- Drone actuators
- Robotics
- Embedded systems
- Sensor feedback loops

---

## Features

- Pure C implementation
- No hardware dependencies
- Reusable across projects
- Conditional integration anti-windup
- Derivative kick reduction
- Runtime gain updates
- Output saturation limits

---

## Structure

```text
lib/
└── pid/
    ├── pid.h
    ├── pid.c
    └── README.md
```

---

## Example

```c
#include "pid.h"

static pid_t motor_pid;

void app_main(void)
{
    pid_init(
        &motor_pid,
        0.8f,   // kp
        0.2f,   // ki
        0.05f,  // kd
        1000.0f,
        2000.0f
    );

    while (1) {

        float setpoint = 1500.0f;
        float measured = 1420.0f;
        float dt = 0.01f;

        float output =
            pid_update(
                &motor_pid,
                setpoint,
                measured,
                dt
            );

        printf("PID Output: %.2f\n", output);
    }
}
```

---

## API

### Initialize Controller

```c
void pid_init(
    pid_t *pid,
    float kp,
    float ki,
    float kd,
    float out_min,
    float out_max
);
```

---

### Update Gains

```c
void pid_set_gains(
    pid_t *pid,
    float kp,
    float ki,
    float kd
);
```

---

### Reset Controller State

```c
void pid_reset(pid_t *pid);
```

Clears:
- integrator
- derivative history

---

### Run PID Update

```c
float pid_update(
    pid_t *pid,
    float setpoint,
    float measured,
    float dt
);
```

Parameters:
- `setpoint` → target value
- `measured` → current measured value
- `dt` → timestep in seconds

Returns:
- clamped control output

---

## PID Equation

:contentReference[oaicite:0]{index=0}

---

## Notes

- Derivative uses measurement feedback
- Integrator includes anti-windup logic
- Output is clamped to configured limits
- Safe for embedded real-time control loops