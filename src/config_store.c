/**
 * @file    config_store.c
 * @brief   Persistent configuration storage in NVS flash. See config_store.h.
 */
#include "config_store.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "config_store";

#define NVS_NAMESPACE   "appcfg"
#define NVS_KEY_BLOB    "cfg"

/* Bump this whenever the layout of app_config_t changes so that an old,
   incompatible blob in flash is discarded rather than misread. */
#define CONFIG_VERSION  1

/* On-flash wrapper: a version tag followed by the config payload. */
typedef struct {
    uint32_t     version;
    app_config_t cfg;
} stored_config_t;

static app_config_t s_config;
static bool         s_ready;

/* ---- Defaults: applied on first boot or after a layout change. ---------- */
static void load_defaults(app_config_t *c)
{
    c->kp = 0.8f;
    c->ki = 0.2f;
    c->kd = 0.05f;

    /* Mode 00 is "off"; the rest are example targets. All editable later. */
    c->setpoint_rpm[0] = 0.0f;
    c->setpoint_rpm[1] = 1000.0f;
    c->setpoint_rpm[2] = 2000.0f;
    c->setpoint_rpm[3] = 3000.0f;
}

static bool write_blob(const app_config_t *c)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return false;
    }

    stored_config_t blob = {
        .version = CONFIG_VERSION,
        .cfg     = *c,
    };

    bool ok = (nvs_set_blob(h, NVS_KEY_BLOB, &blob, sizeof(blob)) == ESP_OK) &&
              (nvs_commit(h) == ESP_OK);

    nvs_close(h);

    if (!ok) {
        ESP_LOGE(TAG, "failed to write config blob");
    }
    return ok;
}

app_config_t *config_store_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "erasing NVS partition (%s)", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    bool loaded = false;
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK) {
        stored_config_t blob;
        size_t len = sizeof(blob);
        if (nvs_get_blob(h, NVS_KEY_BLOB, &blob, &len) == ESP_OK &&
            len == sizeof(blob) &&
            blob.version == CONFIG_VERSION) {
            s_config = blob.cfg;
            loaded   = true;
        }
        nvs_close(h);
    }

    if (!loaded) {
        ESP_LOGI(TAG, "no valid config found; applying defaults");
        load_defaults(&s_config);
        write_blob(&s_config);
    } else {
        ESP_LOGI(TAG, "config loaded from NVS");
    }

    s_ready = true;
    return &s_config;
}

app_config_t *config_store_get(void)
{
    return s_ready ? &s_config : NULL;
}

bool config_store_save(void)
{
    if (!s_ready) {
        return false;
    }
    return write_blob(&s_config);
}

bool config_store_reset_defaults(void)
{
    if (!s_ready) {
        return false;
    }
    load_defaults(&s_config);
    return write_blob(&s_config);
}