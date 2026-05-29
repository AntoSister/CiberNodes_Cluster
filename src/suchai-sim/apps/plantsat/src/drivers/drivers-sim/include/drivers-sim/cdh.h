//
// Created by carlos on 2/29/24.
//

#ifndef SUCHAI_FLIGHT_SOFTWARE_CDH_H
#define SUCHAI_FLIGHT_SOFTWARE_CDH_H

#include "drivers-sim/interface.h"

#define SIM_OBC_ID 0X01
#define SIM_OBC_ADDR_TEMP 0X00

/**
 * Read OBC temperature sensor
 * @param temp Pointer to receive temperature value
 * @return Temperature in C
 */
int sim_obc_get_temp(int *temp);

#endif //SUCHAI_FLIGHT_SOFTWARE_CDH_H
