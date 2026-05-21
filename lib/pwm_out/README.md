# PWM Out Library for ESP32

RC PWM output driver using ESP-IDF LEDC.

Generates standard servo-style PWM signals:

- 50 Hz update rate
- 1000 us = minimum
- 1500 us = neutral
- 2000 us = maximum

Suitable for:

- RC ESC
- PX4 PWM AUX outputs
- Servos
- PWM actuators

---

## Features

- Standard RC PWM output
- ESP-IDF LEDC based
- Configurable GPIO
- Microsecond pulse control
- Lightweight
- Simple API
- Failsafe stop support

---

## Structure

```text
lib/
└── pwm_out/
    ├── pwm_out.h
    ├── pwm_out.c
    └── README.md
```

---

## Example

```c
#include "pwm_out.h"

void app_main(void)
{
    pwm_out_init(GPIO_NUM_4);

    while (1) {

        pwm_out_write_us(1000);
        vTaskDelay(pdMS_TO_TICKS(1000));

        pwm_out_write_us(1500);
        vTaskDelay(pdMS_TO_TICKS(1000));

        pwm_out_write_us(2000);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

---

## API

### Initialize Output

```c
void pwm_out_init(int gpio);
```

---

### Write PWM Pulse

```c
void pwm_out_write_us(uint16_t pulse_us);
```

Valid range:

| Pulse Width | Meaning |
| ------------ | -------- |
| 1000 us | Minimum |
| 1500 us | Neutral |
| 2000 us | Maximum |

---

### Stop Output

```c
void pwm_out_stop(void);
```

Sends minimum PWM signal.

---

## Notes

- Uses ESP-IDF LEDC driver
- Single channel output
- Servo-style PWM signal
- Designed for RC PWM systems