# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.
set(COMPONENT_ADD_INCLUDEDIRS ".")
FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")

idf_component_register(
        SRCS ${app_sources}
        INCLUDE_DIRS "."
        REQUIRES
            main
            nvs_flash
            esp_lvgl_port
            lvgl__lvgl
            spiffs
            fatfs
            button
            esp_event
)

spiffs_create_partition_image(spiffs ../data FLASH_IN_PROJECT)