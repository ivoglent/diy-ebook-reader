cmake_minimum_required(VERSION 3.30)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

list(APPEND EXTRA_COMPONENT_DIRS libs/cale-idf/components/Adafruit-GFX)
list(APPEND EXTRA_COMPONENT_DIRS libs/cale-idf/components/CalEPD)
project(diy-ebook-reader)