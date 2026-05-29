# PlantSat APP

PlantSat Nanosatellite flight software based on the SUCHAI Flight Software Framework

## IMPORTANT NOTE

The `CMakeList.txt` file included in this project IS NOT MADE to build the project,
but to help your IDE to recognize project files and assist you while coding and
configure global variables or macros.

As this app is build for the Nanomind A3200 (AVR32UC3) we have to use the provided
toolchain. PLEASE USE the provided `build_plantsat.sh` to build the project.

## Setup

Configure your CMake (or IDE CMake settings) with following options

```c
-DAPP=plantsat -DSCH_OS=FREERTOS -DSCH_ARCH=NANOMIND -DSCH_ST_MODE=RAM -DSCH_COMM_NODE=1 -DSCH_DEVICE_ID=1 -DSCH_HOOK_COMM=1
```

Its worth to download FreeRTOS source files and custom CMakeList.txt from this
link (https://drive.google.com/file/d/1OMQnXF5BLSVh5SHUz_nV01vUTCoKrEhC/view?usp=sharing)

Extract the folder into `suchai-flight-software/src/os/freertos/`. This will 
help your IDE to find FreeRTOS source files.

## Build

## Install (Program)

