###
# PLANTSAT APP CMAKE FILE
##
if(SCH_NAME STREQUAL "SUCHAI2" OR SCH_NAME STREQUAL "SUCHAI3" OR SCH_NAME STREQUAL "PLANTSAT")
    set(${SCH_NAME} ON)
else()
    message(FATAL_ERROR "Invalid SCH_NAME=${SCH_NAME} selected. Please use SUCHAI2, SUCHAI3, or PLANTSAT.")
endif()
set(SCH_TX_INHIBIT 10 CACHE STRING "Default silent time in seconds [0, 1800 (30min)]")
set(SCH_TX_FREQ 437250000 CACHE STRING "Nanocom TX freq in Hz")
set(SCH_TX_PWR 0 CACHE STRING "Nanocom TX power")
set(SCH_TX_BAUD 4800 CACHE STRING "Nanocom TX Baudrate")
set(SCH_TX_BCN_PERIOD 600 CACHE STRING "Number of seconds between trx beacon packets")
set(SCH_OBC_BCN_OFFSET 600 CACHE STRING "Number of seconds between obc beacon packets")
set(SCH_HK_ENABLED 1 CACHE BOOL "Enable task housekeeping")
set(SCH_SEN_ENABLED 1 CACHE BOOL "Enable task sensors")
set(SCH_ADCS_ENABLED 0 CACHE BOOL "Enable task adcs")
set(SCH_EPS_OUT_ENABLED 1 CACHE BOOL "Set EPS output (on/off)")

add_definitions(-DSIM)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/app/system/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/app/system/config.h)


set(SOURCE_FILES
        src/system/main.c
        src/system/cmdADCS.c
        src/system/cmdEPS.c
        src/system/cmdRW.c
        src/system/cmdSensors.c
        src/system/cmdCDH.c
        src/system/cmdCAM.c
        src/system/hookCommunications.c
        src/system/taskHousekeeping.c
        src/system/taskSensors.c
        src/system/taskADCS.c
        src/drivers/rwdrv10987_2.c
        src/system/TRIADEKF.c
)

if("${SCH_ARCH}" STREQUAL "NANOMIND")
    # Set NanoMind specific sources
    list(APPEND SOURCE_FILES src/system/cmdAX100.c src/system/cmdGSSB.c)
endif ()

set(GS_INCLUDE_PATH
        src/drivers/suchai-drivers-bus/lib/libparam_client/src
        src/drivers/suchai-drivers-bus/lib/libparam_client/include/deprecated
        src/drivers/suchai-drivers-bus/lib/libparam_client/include
        src/drivers/suchai-drivers-bus/lib/libparam_client/include/gs
        src/drivers/suchai-drivers-bus/lib/libgscsp/include
        src/drivers/suchai-drivers-bus/lib/nanocom-ax_client/include
        src/drivers/suchai-drivers-bus/lib/libutil/include/deprecated/gs/gosh
        src/drivers/suchai-drivers-bus/lib/libutil/include
        src/drivers/suchai-drivers-bus/lib/libutil/include/gs
        src/drivers/suchai-drivers-bus/lib/libutil
        src/drivers/suchai-drivers-bus/lib/nanopower_client
        src/drivers/suchai-drivers-bus/lib/nanopower-bpx_client/include
        src/drivers/suchai-drivers-bus/lib/libgssb_client/include

        src/drivers/suchai-drivers-obc/lib/liba3200/include
        src/drivers/suchai-drivers-obc/lib/liba3200/lib/liba3200_client/include
        src/drivers/suchai-drivers-obc/lib/libasf/avr32/drivers/wdt
        src/drivers/suchai-drivers-obc/lib/libasf/avr32/drivers/gpio
        src/drivers/suchai-drivers-obc/lib/libasf/avr32/drivers/spi
        src/drivers/suchai-drivers-obc/lib/libasf/avr32/drivers/scif
        src/drivers/suchai-drivers-obc/lib/libasf/common/boards
        src/drivers/suchai-drivers-obc/lib/libasf/common/services/clock
        src/drivers/suchai-drivers-obc/lib/libasf/avr32/drivers/ebi/sdramc
        src/drivers/suchai-drivers-obc/lib/libthirdparty/include
        src/drivers/suchai-drivers-obc/lib/libembed/include
)

include_directories(include/app/drivers ${GS_INCLUDE_PATH})
add_executable(suchai-app ${SOURCE_FILES})
target_include_directories(suchai-app PUBLIC include)
# Set NanoMind include directories
if("${SCH_ARCH}" STREQUAL "NANOMIND")
    target_include_directories(suchai-app PUBLIC ${GS_INCLUDE_PATH})
endif ()
target_link_libraries(suchai-app PUBLIC suchai-fs-core)

add_subdirectory(src/drivers/drivers-sim)
target_include_directories(suchai-app PUBLIC src/drivers/drivers-sim/include)
target_link_libraries(suchai-app PUBLIC drivers-sim)
