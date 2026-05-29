cmake_minimum_required(VERSION 3.16...3.19)

# Project name and a few useful settings. Other commands can pick up the results
project(
  SUCHAI-Flight-Software
  DESCRIPTION "SUCHAI Nanosatellite Flight Software Framework"
  LANGUAGES C)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(CMAKE_C_STANDARD 99)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

if(NOT EXISTS suchai-flight-software)
    execute_process(COMMAND sh init.sh)
endif()

# Enable -fsanitize
#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

# The compiled library code is here
set(APP simple CACHE STRING "Application name")
message("Building for APP apps/${APP} ...")
include_directories(apps/${APP}/include)
add_subdirectory(apps/${APP})
add_subdirectory(suchai-flight-software/src)
