set(SCH_TX_FREQ 425000000 CACHE STRING "Nanocom TX freq in Hz")
set(SCH_TX_PWR 0 CACHE STRING "Nanocom TX power")
set(SCH_TX_BAUD 4800 CACHE STRING "Nanocom TX Baudrate")
set(SCH_TX_BCN_PERIOD 600 CACHE STRING "Number of seconds between trx beacon packets")
set(SCH_OBC_BCN_OFFSET 600 CACHE STRING "Number of seconds between obc beacon packets")
set(SCH_GND_ADD_PAYLOADS 0 CACHE BOOL "Enable payloads commands")
set(SCH_GND_ADD_GS 0 CACHE BOOL "Enable GomSpace commands (NANOPOWER, AX100)")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/app/system/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/app/system/config.h)

set(GS_SOURCE_FILES
        #pendiente conf_util.h que se genera al correr ./waf en suchai-drivers-bus, actualmente solo se copio al include de groundstation
        #nanocom-ax_client files
        src/drivers/suchai-drivers-bus/lib/nanocom-ax_client/src/ax100_client.c
        #nanopower files
        src/drivers/suchai-drivers-bus/lib/nanopower_client/src/nanopower2.c
        #libparam_client files
        src/drivers/suchai-drivers-bus/lib/libparam_client/src/serialize.c
        src/drivers/suchai-drivers-bus/lib/libparam_client/src/string.c
        src/drivers/suchai-drivers-bus/lib/libparam_client/src/table.c
        src/drivers/suchai-drivers-bus/lib/libparam_client/src/rparam/deprecated_rparam.c
        src/drivers/suchai-drivers-bus/lib/libparam_client/src/rparam/query.c
        src/drivers/suchai-drivers-bus/lib/libparam_client/src/rparam/rparam.c
        #libutil files
        src/drivers/suchai-drivers-bus/lib/libutil/src/log/log.c
        src/drivers/suchai-drivers-bus/lib/libutil/src/error.c
        src/drivers/suchai-drivers-bus/lib/libutil/src/byteorder.c
        src/drivers/suchai-drivers-bus/lib/libutil/src/bytebuffer.c
        src/drivers/suchai-drivers-bus/lib/libutil/src/fletcher.c
        src/drivers/suchai-drivers-bus/lib/libutil/src/lock.c
        src/drivers/suchai-drivers-bus/lib/libutil/src/strtoint.c
        src/drivers/suchai-drivers-bus/lib/libutil/src/linux/mutex.c
        src/drivers/suchai-drivers-bus/lib/libutil/src/rtc.c
        src/drivers/suchai-drivers-bus/lib/libutil/src/string.c
        src/drivers/suchai-drivers-bus/lib/libutil/src/linux/clock.c
        src/drivers/suchai-drivers-bus/lib/libutil/src/log/appender/console.c
)

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
        include/app/drivers
)

set(SOURCE_FILES
        src/system/main.c
        src/system/cmdAPP.c
        src/system/cmdCDH.c
        src/system/hookCommunications.c
)

if(${SCH_GND_ADD_GS})
    list(APPEND SOURCE_FILES
            src/system/cmdAX100.c
            src/system/cmdEPS.c
            ${GS_SOURCE_FILES}
            )
endif()

if(${SCH_GND_ADD_PAYLOADS})
    list(APPEND SOURCE_FILES
            src/payloads/stt/cmdSTT.c
            src/payloads/stt/cmdCam.c
            )
endif()

add_executable(ground-app ${SOURCE_FILES})
if (${SCH_GND_ADD_GS})
    target_include_directories(ground-app PRIVATE ${GS_INCLUDE_PATH})
endif ()
target_include_directories(ground-app PUBLIC include)
target_link_libraries(ground-app PUBLIC suchai-fs-core)
