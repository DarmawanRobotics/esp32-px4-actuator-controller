FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "." "../include"
    REQUIRES nvs_flash
             esp_wifi
             esp_http_server
             esp_netif
             esp_event
             driver
             esp_timer
)