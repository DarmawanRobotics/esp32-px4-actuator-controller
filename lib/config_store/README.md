# Config Store Library for ESP32

Persistent application configuration storage using ESP-IDF NVS (Non-Volatile Storage).

Stores configuration data that must survive reboot or power loss.

Designed for:
- PID controller gains
- Motor RPM setpoints
- Runtime configuration
- Embedded robotics systems

---

## Features

- NVS flash storage
- Automatic default configuration
- Versioned configuration layout
- In-RAM configuration cache
- Persistent save support
- Factory reset support
- Lightweight API

---

## Structure

```text
lib/
└── config_store/
    ├── config_store.h
    ├── config_store.c
    └── README.md
```

---

## Stored Configuration

```c
typedef struct {
    float kp;
    float ki;
    float kd;

    float setpoint_rpm[4];
} app_config_t;
```

Includes:
- PID gains
- Per-mode RPM setpoints

---

## Example

```c
#include "config_store.h"

void app_main(void)
{
    app_config_t *cfg =
        config_store_init();

    printf("KP: %.2f\n", cfg->kp);

    cfg->kp = 1.2f;
    cfg->setpoint_rpm[1] = 2500.0f;

    config_store_save();
}
```

---

## API

### Initialize Store

```c
app_config_t *config_store_init(void);
```

Initializes:
- NVS flash
- configuration cache
- default config if needed

Returns:
- pointer to in-RAM configuration

---

### Get Configuration

```c
app_config_t *config_store_get(void);
```

Returns:
- current in-RAM configuration

---

### Save Configuration

```c
bool config_store_save(void);
```

Writes current configuration to NVS.

Returns:
- `true` on success
- `false` on failure

---

### Reset Defaults

```c
bool config_store_reset_defaults(void);
```

Restores default configuration and saves to flash.

---

## Default Configuration

| Parameter | Default |
| ---------- | ------- |
| KP | 0.8 |
| KI | 0.2 |
| KD | 0.05 |
| Mode 0 RPM | 0 |
| Mode 1 RPM | 1000 |
| Mode 2 RPM | 2000 |
| Mode 3 RPM | 3000 |

---

## Configuration Versioning

The library includes a configuration version field.

If the configuration layout changes:

```c
#define CONFIG_VERSION  1
```

increment the version number to invalidate incompatible stored data automatically.

---

## Notes

- Uses ESP-IDF NVS
- Stores configuration as a binary blob
- Keeps active configuration cached in RAM
- Automatically restores defaults on invalid data
- Suitable for embedded persistent settings