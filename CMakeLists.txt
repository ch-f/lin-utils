cmake_minimum_required(VERSION 3.25)
project(lin-utils LANGUAGES C)

set(CMAKE_C_STANDARD 23)

add_executable(lin_checksum lin_checksum.c)
add_executable(lin_bcm_config_eventframe lin_bcm_config_eventframe.c)
