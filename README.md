# ESP32-C3 PX4 Actuator Controller

Closed-loop brushed-motor speed controller for an ESP32-C3, driven by a PX4
flight controller and tunable live over WiFi.

The PX4 selects one of four operating modes through two PWM lines. Each mode
maps to a target RPM. An onboard PID controller drives a brushed ESC to hold
that RPM, using a photoelectric encoder for feedback. The ESP32 also hosts its
own WiFi access point and a web page for tuning, configuration, and debugging.

Arming and autonomous behaviour stay on the PX4 side. This firmware only
follows the commanded mode and regulates motor speed.

---

## How it works

```
   PX4 AUX1  --PWM-->  GPIO2   mode bit 0 (LSB)
   PX4 AUX2  --PWM-->  GPIO3   mode bit 1 (MSB)
   Encoder   --pulse-> GPIO4   RPM feedback
   ESC       <--PWM--  GPIO5   motor command
```

### Mode selection

The two PWM inputs are each reduced to a single bit (high pulse = 1, low = 0)
and combined into a 2-bit mode index:

| AUX2 | AUX1 | Mode | Default target |
| ---- | ---- | ---- | -------------- |
| 0    | 0    | `00` | 0 RPM (off)    |
| 0    | 1    | `01` | 1000 RPM       |
| 1    | 0    | `10` | 2000 RPM       |
| 1    | 1    | `11` | 3000 RPM       |

The bit threshold is `1500 us` by default. A lost PX4 signal is treated as `0`,
so a disconnected controller falls back to mode `00` (motor off).

### Control loop

Runs at 100 Hz:

1. Decode the mode and look up its target RPM.
2. Measure RPM from the encoder.
3. Sync PID gains (in case they changed over the web).
4. On a mode change, reset the PID to avoid a derivative kick.
5. Compute the ESC pulse width with the PID controller. A zero target sends
   the disarm pulse directly and holds the integrator at zero.
6. Update statistics and publish telemetry to the web UI.

A target of 0 always forces the ESC to its minimum pulse, so mode `00`
reliably stops the motor.

---

## Web interface

On boot the ESP32 starts a WiFi access point:

- **SSID:** `PX4-Actuator`
- **Password:** `actuator123`
- **URL:** `http://192.168.4.1`

Open the page in any browser while connected to that network. It updates twice
per second and lets you:

- **Tune PID gains** (Kp, Ki, Kd) live.
- **Set the RPM target** for each of the four modes.
- **Watch live telemetry:** active mode, target RPM, measured RPM, ESC output.
- **Inspect RPM statistics:** min / max / average, both overall and per mode.
- **Reset statistics** for a clean debug run.
- **Save to flash** so gains and setpoints survive a reboot.
- **Restore defaults** if a tuning session goes wrong.

Changes apply immediately to the running control loop. They are **not**
persisted until you press *Save to flash*.

### HTTP API

The page is backed by a small JSON API, also usable directly:

| Method | Path                  | Body                       | Action                  |
| ------ | --------------------- | -------------------------- | ----------------------- |
| GET    | `/`                   | —                          | Web UI                  |
| GET    | `/api/state`          | —                          | Full state snapshot     |
| POST   | `/api/pid`            | `{"kp":..,"ki":..,"kd":..}`| Update PID gains        |
| POST   | `/api/setpoints`      | `{"sp":[s0,s1,s2,s3]}`     | Update mode setpoints   |
| POST   | `/api/save`           | —                          | Persist config to flash |
| POST   | `/api/reset_stats`    | —                          | Clear statistics        |
| POST   | `/api/reset_defaults` | —                          | Restore default config  |

---

## Persistence

Stored in NVS flash and reloaded on boot:

- PID gains (Kp, Ki, Kd)
- The four per-mode RPM setpoints

Kept in RAM only (cleared on reboot or reset):

- Min / max / average RPM statistics

A version tag guards the stored layout; if it changes in a future build, the
old data is discarded and defaults are applied automatically.

---

## Project structure

```text
.
├── include/                    application public headers (.h)
│   ├── config_store.h
│   ├── motor_stats.h
│   ├── web_config.h
│   └── web_page.h
├── src/                        application implementation (.c)
│   ├── main.c                  control loop + integration
│   ├── config_store.c          NVS persistence (gains + setpoints)
│   ├── motor_stats.c           runtime RPM statistics
│   └── web_config.c            WiFi SoftAP + HTTP server + web UI
├── lib/                        generic, reusable drivers
│   ├── pwm_read/               RC-PWM input capture (mode bits)
│   ├── pwm_out/                RC-PWM ESC output
│   ├── encoder_read/           GPIO interrupt-based RPM measurement
│   └── pid/                    generic PID controller
├── partitions.csv
├── platformio.ini
└── CMakeLists.txt
```

The layout follows two rules:

- **`lib/` holds only generic, reusable modules**, each keeping its own header
  and source together and shipping a `README.md`. Pins and parameters are
  passed in at init, so any of them drops into another project unchanged.
- **Application-specific code is split header/source:** public headers go in
  `include/`, implementation in `src/`. `config_store`, `motor_stats`, and
  `web_config` know about this project's modes, setpoints, and PID, so they
  are application code rather than reusable libraries. They compile as part of
  the `main` component; `src/CMakeLists.txt` adds `include/` to the include
  path and declares the IDF dependencies.

---

## Configuration

All wiring and behaviour constants are at the top of `src/main.c`:

| Constant                | Meaning                              | Default     |
| ----------------------- | ------------------------------------ | ----------- |
| `PIN_MODE_BIT0`         | PX4 AUX1 input (mode LSB)            | GPIO2       |
| `PIN_MODE_BIT1`         | PX4 AUX2 input (mode MSB)            | GPIO3       |
| `PIN_ENCODER`           | Encoder signal input                 | GPIO4       |
| `PIN_ESC`               | ESC PWM output                       | GPIO5       |
| `ENCODER_SLOTS_PER_REV` | Encoder slots per revolution         | 20          |
| `MODE_PWM_THRESHOLD_US` | High/low bit threshold               | 1500 us     |
| `PWM_INPUT_TIMEOUT_MS`  | PX4 signal stale timeout             | 100 ms      |
| `CONTROL_PERIOD_MS`     | Control loop period                  | 10 ms (100 Hz) |
| `ESC_MIN_US`/`ESC_MAX_US` | ESC pulse range                    | 1000–2000 us |
| `AP_SSID` / `AP_PASSWORD` | WiFi access point credentials      | see source  |

Default PID gains and setpoints live in `src/config_store.c`
(`load_defaults`). They only apply on first boot or after *Restore defaults*;
after that the stored values are used.

---

## Build and flash

PlatformIO (ESP-IDF framework, ESP32-C3):

```bash
pio run                   # build
pio run --target upload   # flash
pio device monitor        # serial log @ 115200
```

---

## Notes

- The encoder uses GPIO rising edge interrupt and counts rising edges only; set
  `ENCODER_SLOTS_PER_REV` to match your disc for correct RPM.
  Note: ESP32-C3 does not have hardware PCNT, so GPIO ISR is used instead.
- The PID output is the ESC pulse width directly, clamped to the ESC range,
  with conditional-integration anti-windup.
- If your ESC needs a different pulse range, adjust `ESC_MIN_US` / `ESC_MAX_US`
  and the limits passed to `pid_init`.
- For an open WiFi network, set `AP_PASSWORD` to `""`.