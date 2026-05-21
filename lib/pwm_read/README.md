# PWM Read Library for ESP32

Simple interrupt-based RC PWM input reader for ESP32 using GPIO edge interrupts and `esp_timer`.

## Features

* Multi-channel PWM input
* Interrupt-based
* Lightweight
* Microsecond precision
* Timeout / failsafe support

---

## Example

```c
#include "pwm_read.h"

void app_main(void)
{
    pwm_read_init();

    int ch = pwm_read_add_channel(GPIO_NUM_4, 100);

    while (1) {
        uint32_t pwm;

        if (pwm_read_get_us(ch, &pwm)) {
            printf("PWM: %lu us\n", pwm);
        } else {
            printf("Signal lost\n");
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
```

---

## API

### Initialize

```c
pwm_read_init();
```

### Add Channel

```c
int ch = pwm_read_add_channel(GPIO_NUM_4, 100);
```

### Read PWM

```c
uint32_t pwm;
pwm_read_get_us(ch, &pwm);
```

---

## Typical RC PWM Values

| Pulse Width | Meaning |
| ----------- | ------- |
| 1000 us     | Minimum |
| 1500 us     | Center  |
| 2000 us     | Maximum |
