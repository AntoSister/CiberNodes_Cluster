set(SCH_SIM_INTERFACE_URI \"tcp://localhost:5555\" CACHE STRING "Drivers interface endpoint")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/drivers-sim/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/drivers-sim/config.h)

set(SOURCE_FILES
        interface.c
        adcs.c
        camera.c
        cdh.c
        eps.c
)

add_library(drivers-sim ${SOURCE_FILES})
target_include_directories(drivers-sim PUBLIC include)
target_link_libraries(drivers-sim PRIVATE -lzmq)