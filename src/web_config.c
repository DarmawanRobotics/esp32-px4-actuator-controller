/**
 * @file    web_config.c
 * @brief   WiFi SoftAP + HTTP server for live tuning/debug. See web_config.h.
 */
#include "web_config.h"
#include "web_page.h"

#include "motor_stats.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_http_server.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "web_config";

#define AP_MAX_CONN     4
#define AP_CHANNEL      1
#define JSON_BUF_SIZE   1024
#define POST_BUF_SIZE   256

static app_config_t     *s_cfg;
static web_telemetry_t   s_tel;
static SemaphoreHandle_t s_tel_lock;

/* ----------------------------------------------------------------------- */
/* Telemetry shared between control loop and HTTP handlers.                */
/* ----------------------------------------------------------------------- */
void web_config_publish(const web_telemetry_t *telemetry)
{
    if (telemetry == NULL || s_tel_lock == NULL) {
        return;
    }
    xSemaphoreTake(s_tel_lock, portMAX_DELAY);
    s_tel = *telemetry;
    xSemaphoreGive(s_tel_lock);
}

static void telemetry_snapshot(web_telemetry_t *out)
{
    xSemaphoreTake(s_tel_lock, portMAX_DELAY);
    *out = s_tel;
    xSemaphoreGive(s_tel_lock);
}

/* ----------------------------------------------------------------------- */
/* Minimal JSON number extraction (no allocator, no full parser).          */
/* Finds "key" then reads the float that follows the next ':'.             */
/* ----------------------------------------------------------------------- */
static bool json_get_float(const char *body, const char *key, float *out)
{
    const char *p = strstr(body, key);
    if (p == NULL) {
        return false;
    }
    p = strchr(p, ':');
    if (p == NULL) {
        return false;
    }
    *out = strtof(p + 1, NULL);
    return true;
}

/* Read an array "key":[a,b,c,d] of up to @p n floats. Returns count read. */
static int json_get_float_array(const char *body, const char *key,
                                float *out, int n)
{
    const char *p = strstr(body, key);
    if (p == NULL) {
        return 0;
    }
    p = strchr(p, '[');
    if (p == NULL) {
        return 0;
    }
    p++;
    int i = 0;
    while (i < n && *p != ']' && *p != '\0') {
        char *end = NULL;
        float v = strtof(p, &end);
        if (end == p) {
            break;          /* no number parsed */
        }
        out[i++] = v;
        p = end;
        while (*p == ',' || *p == ' ') {
            p++;
        }
    }
    return i;
}

/* ----------------------------------------------------------------------- */
/* HTTP handlers                                                           */
/* ----------------------------------------------------------------------- */
static esp_err_t root_get(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, WEB_PAGE_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t state_get(httpd_req_t *req)
{
    web_telemetry_t tel;
    telemetry_snapshot(&tel);

    stats_bucket_t overall;
    motor_stats_get_overall(&overall);

    stats_bucket_t mode[CONFIG_MODE_COUNT];
    for (int i = 0; i < CONFIG_MODE_COUNT; i++) {
        motor_stats_get_mode(i, &mode[i]);
    }

    char buf[JSON_BUF_SIZE];
    int n = snprintf(buf, sizeof(buf),
        "{\"mode\":%d,\"target_rpm\":%.1f,\"measured_rpm\":%.1f,\"output_us\":%.1f,"
        "\"kp\":%.4f,\"ki\":%.4f,\"kd\":%.4f,"
        "\"sp\":[%.1f,%.1f,%.1f,%.1f],"
        "\"stats\":{\"overall\":{\"min\":%.1f,\"max\":%.1f,\"avg\":%.1f,\"samples\":%lu},"
        "\"mode\":["
        "{\"min\":%.1f,\"max\":%.1f,\"avg\":%.1f,\"samples\":%lu},"
        "{\"min\":%.1f,\"max\":%.1f,\"avg\":%.1f,\"samples\":%lu},"
        "{\"min\":%.1f,\"max\":%.1f,\"avg\":%.1f,\"samples\":%lu},"
        "{\"min\":%.1f,\"max\":%.1f,\"avg\":%.1f,\"samples\":%lu}]}}",
        tel.mode, tel.target_rpm, tel.measured_rpm, tel.output_us,
        s_cfg->kp, s_cfg->ki, s_cfg->kd,
        s_cfg->setpoint_rpm[0], s_cfg->setpoint_rpm[1],
        s_cfg->setpoint_rpm[2], s_cfg->setpoint_rpm[3],
        overall.min_rpm, overall.max_rpm, overall.avg_rpm,
        (unsigned long)overall.samples,
        mode[0].min_rpm, mode[0].max_rpm, mode[0].avg_rpm, (unsigned long)mode[0].samples,
        mode[1].min_rpm, mode[1].max_rpm, mode[1].avg_rpm, (unsigned long)mode[1].samples,
        mode[2].min_rpm, mode[2].max_rpm, mode[2].avg_rpm, (unsigned long)mode[2].samples,
        mode[3].min_rpm, mode[3].max_rpm, mode[3].avg_rpm, (unsigned long)mode[3].samples);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, buf, n);
}

/* Read the request body into a stack buffer (NUL-terminated). */
static int read_body(httpd_req_t *req, char *buf, size_t cap)
{
    size_t total = req->content_len;
    if (total >= cap) {
        total = cap - 1;
    }
    int received = 0;
    while (received < (int)total) {
        int r = httpd_req_recv(req, buf + received, total - received);
        if (r <= 0) {
            return -1;
        }
        received += r;
    }
    buf[received] = '\0';
    return received;
}

static esp_err_t pid_post(httpd_req_t *req)
{
    char body[POST_BUF_SIZE];
    if (read_body(req, body, sizeof(body)) < 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad body");
        return ESP_FAIL;
    }

    float v;
    if (json_get_float(body, "\"kp\"", &v)) s_cfg->kp = v;
    if (json_get_float(body, "\"ki\"", &v)) s_cfg->ki = v;
    if (json_get_float(body, "\"kd\"", &v)) s_cfg->kd = v;

    ESP_LOGI(TAG, "PID updated: kp=%.4f ki=%.4f kd=%.4f",
             s_cfg->kp, s_cfg->ki, s_cfg->kd);

    httpd_resp_sendstr(req, "ok");
    return ESP_OK;
}

static esp_err_t setpoints_post(httpd_req_t *req)
{
    char body[POST_BUF_SIZE];
    if (read_body(req, body, sizeof(body)) < 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad body");
        return ESP_FAIL;
    }

    float sp[CONFIG_MODE_COUNT];
    int got = json_get_float_array(body, "\"sp\"", sp, CONFIG_MODE_COUNT);
    for (int i = 0; i < got; i++) {
        s_cfg->setpoint_rpm[i] = (sp[i] < 0.0f) ? 0.0f : sp[i];
    }

    ESP_LOGI(TAG, "setpoints updated (%d values)", got);
    httpd_resp_sendstr(req, "ok");
    return ESP_OK;
}

static esp_err_t save_post(httpd_req_t *req)
{
    bool ok = config_store_save();
    httpd_resp_sendstr(req, ok ? "ok" : "err");
    return ESP_OK;
}

static esp_err_t reset_stats_post(httpd_req_t *req)
{
    motor_stats_reset();
    httpd_resp_sendstr(req, "ok");
    return ESP_OK;
}

static esp_err_t reset_defaults_post(httpd_req_t *req)
{
    bool ok = config_store_reset_defaults();
    httpd_resp_sendstr(req, ok ? "ok" : "err");
    return ESP_OK;
}

/* ----------------------------------------------------------------------- */
/* Server + WiFi setup                                                     */
/* ----------------------------------------------------------------------- */
static void register_handlers(httpd_handle_t server)
{
    const httpd_uri_t routes[] = {
        { .uri = "/",                  .method = HTTP_GET,  .handler = root_get },
        { .uri = "/api/state",         .method = HTTP_GET,  .handler = state_get },
        { .uri = "/api/pid",           .method = HTTP_POST, .handler = pid_post },
        { .uri = "/api/setpoints",     .method = HTTP_POST, .handler = setpoints_post },
        { .uri = "/api/save",          .method = HTTP_POST, .handler = save_post },
        { .uri = "/api/reset_stats",   .method = HTTP_POST, .handler = reset_stats_post },
        { .uri = "/api/reset_defaults",.method = HTTP_POST, .handler = reset_defaults_post },
    };
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        httpd_register_uri_handler(server, &routes[i]);
    }
}

static void start_http_server(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.max_uri_handlers = 10;
    cfg.lru_purge_enable = true;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &cfg) == ESP_OK) {
        register_handlers(server);
        ESP_LOGI(TAG, "HTTP server started");
    } else {
        ESP_LOGE(TAG, "failed to start HTTP server");
    }
}

static void wifi_init_softap(const char *ssid, const char *password)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    const wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));

    wifi_config_t wifi_cfg = { 0 };
    strncpy((char *)wifi_cfg.ap.ssid, ssid, sizeof(wifi_cfg.ap.ssid) - 1);
    wifi_cfg.ap.ssid_len = strlen((char *)wifi_cfg.ap.ssid);
    wifi_cfg.ap.channel  = AP_CHANNEL;
    wifi_cfg.ap.max_connection = AP_MAX_CONN;

    if (password == NULL || password[0] == '\0') {
        wifi_cfg.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        strncpy((char *)wifi_cfg.ap.password, password,
                sizeof(wifi_cfg.ap.password) - 1);
        wifi_cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP up: SSID='%s' (default IP 192.168.4.1)", ssid);
}

void web_config_start(app_config_t *cfg, const char *ssid, const char *password)
{
    s_cfg     = cfg;
    s_tel_lock = xSemaphoreCreateMutex();

    wifi_init_softap(ssid, password);
    start_http_server();
}