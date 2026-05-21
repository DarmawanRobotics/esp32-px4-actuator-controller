# Encoder Read Library for ESP32

Generic RPM measurement library using GPIO interrupt + esp_timer.
Designed as a drop-in for ESP32 variants without hardware PCNT (e.g. ESP32-C3).

Designed for:
- Slotted disc encoders
- Photoelectric sensors
- Hall-effect sensors
- Motor RPM feedback systems

Counts rising edges via GPIO ISR with minimal CPU overhead.

---

## Features

- GPIO interrupt-based pulse counting
- Compatible with all ESP32 variants including ESP32-C3
- RPM calculation
- Configurable slots-per-revolution
- Simple API
- ESP-IDF v5/v6 compatible

---

## Structure

```text
lib/
└── encoder_read/
    ├── encoder_read.h
    ├── encoder_read.c
    └── README.md
```

---

## Example

```c
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "encoder_read.h"

void app_main(void)
{
    encoder_read_init(
        GPIO_NUM_4,
        20
    );

    while (1) {

        float rpm =
            encoder_read_rpm(0.1f);

        printf("RPM: %.2f\n", rpm);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

---

## API

### Initialize Encoder

```c
void encoder_read_init(
    int gpio,
    int slots_per_rev
);
```

Parameters:
- `gpio` → encoder signal GPIO
- `slots_per_rev` → encoder slots / pulses per revolution

---

### Read RPM

```c
float encoder_read_rpm(float dt_seconds);
```

Parameters:
- `dt_seconds` → elapsed time since previous call

Returns:
- calculated RPM

---

### Read Raw Pulse Count

```c
int encoder_read_raw_count(void);
```

Useful for:
- debugging
- diagnostics
- calibration

---

## RPM Equation

```
RPM = (pulses / slots_per_rev) / dt_seconds * 60
```

---

## Notes

- Uses GPIO rising edge interrupt (POSEDGE)
- No hardware PCNT required, works on ESP32-C3
- Counts rising edges only
- Call `encoder_read_rpm()` periodically for stable readings
- Designed for single-channel encoder input