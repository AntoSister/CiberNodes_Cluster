//
// Created by carlos on 2/29/24.
//

#ifndef SUCHAI_FLIGHT_SOFTWARE_EPS_H
#define SUCHAI_FLIGHT_SOFTWARE_EPS_H

#include "drivers-sim/interface.h"

#define SIM_EPS_ID 0X02
#define SIM_EPS_ADDR_HKP 0X00
#define SIM_EPS_ADDR_SET 0X01
#define SIM_EPS_ADDR_HEATER 0X02
#define SIM_EPS_ADDR_HARD_RESET 0X03

typedef struct eps_hk_read{
    int vbat;
    int current_in;
    int current_out;
    int temp;
} eps_hk_read_t;

typedef struct eps_hk_temp{
    int temp[6];
} eps_hk_temp_t;

/**
 * Read EPS housekeeping data such as voltage, currents, and temperature.
 * @param hk_read Pointer to store HK data.
 * @return 1 Ok, -1 otherwise
 */
int sim_eps_get_hk(eps_hk_read_t *hk_read);

/**
 * Set EPS outputs on/off
 * @param channel Output channel
 * @param mode 1: ON, 0: OFF
 * @return 1 Ok, -1 otherwise
 */
int sim_eps_set_output(uint8_t channel, uint8_t mode);

/**
 * Set battery heaters on/off
 * @param mode 1: ON, 0: OFF
 * @return 1 Ok, -1 otherwise
 */
int sim_eps_set_heater(uint8_t mode);

/**
 * Send the hard reset command to the EPS
 * @return
 */
int sim_eps_hard_reset();

#endif //SUCHAI_FLIGHT_SOFTWARE_EPS_H
