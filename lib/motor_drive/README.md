# Motor Drive Library for ESP32

RC PWM motor / ESC output driver using ESP-IDF LEDC.

Generates standard servo-style PWM signals:

- 50 Hz update rate
- 1000 us = minimum
- 1500 us = neutral
- 2000 us = maximum

Suitable for:

- RC ESC
- PX4 PWM AUX style outputs
- Servo-compatible motor drivers

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
└── motor_drive/
    ├── motor_drive.h
    ├── motor_drive.c
    └── README.md
```

---

## Example

```c
#include "motor_drive.h"

void app_main(void)
{
    motor_drive_init(GPIO_NUM_4);

    while (1) {

        motor_drive_write_us(1000);
        vTaskDelay(pdMS_TO_TICKS(1000));

        motor_drive_write_us(1500);
        vTaskDelay(pdMS_TO_TICKS(1000));

        motor_drive_write_us(2000);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

---

## API

### Initialize Output

```c
void motor_drive_init(int gpio);
```

---

### Write PWM Pulse

```c
void motor_drive_write_us(uint16_t pulse_us);
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
void motor_drive_stop(void);
```

Sends minimum throttle signal.

---

## Notes

- Uses ESP-IDF LEDC driver
- Single channel output
- Servo-style PWM signal
- Designed for RC ESC systems