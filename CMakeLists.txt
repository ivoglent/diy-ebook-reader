cmake_minimum_required(VERSION 3.30)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(LV_CONF_PATH "${CMAKE_SOURCE_DIR}/lv_conf.h")

add_compile_definitions(LV_COLOR_DEPTH=1)

project(diy-ebook-reader)