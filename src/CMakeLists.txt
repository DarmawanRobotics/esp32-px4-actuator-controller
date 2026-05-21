# Application component.
#   - .c implementation files live here in src/
#   - their headers live in ../include (project public headers)
# Generic, reusable drivers live in lib/ as separate components, each with
# its own header + source.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.c)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "." "${CMAKE_SOURCE_DIR}/include"
    REQUIRES nvs_flash
             esp_wifi
             esp_http_server
             esp_netif
             esp_event
             driver
             esp_timer
)