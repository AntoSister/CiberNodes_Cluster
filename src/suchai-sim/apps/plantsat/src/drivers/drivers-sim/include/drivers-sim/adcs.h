//
// Created by carlos on 2/29/24.
//

#ifndef SUCHAI_FLIGHT_SOFTWARE_ADCS_H
#define SUCHAI_FLIGHT_SOFTWARE_ADCS_H

#include <stdint.h>
#include "drivers-sim/interface.h"

#define SIM_ADCS_ID 0X03
#define SIM_ADCS_ADDR_MAG 0X00
#define SIM_ADCS_ADDR_GYR 0X01
#define SIM_ADCS_ADDR_SUN 0X02
#define SIM_ADCS_ADDR_MTT 0X03

typedef struct mag_read{
    float x;
    float y;
    float z;
} mag_read_t;

typedef struct gyro_read{
    float gyro_x;
    float gyro_y;
    float gyro_z;
} gyro_read_t;

/**
 * Get magnetometer values
 * @param mag_read Pointer to receive sensor values
 * @return 0 Ok, -1 otherwise
 */
int sim_adcs_get_magnetometer(mag_read_t *mag_read);

/**
 * Get gyroscope values
 * @param gyro_read Pointer to receive sensor values
 * @return 0 Ok, -1 otherwise
 */
int sim_adcs_get_gyroscope(gyro_read_t *gyro_read);

/**
 * Get sun sensor values
 * @param channel Sun sensor channel
 * @param sun_read Pointer to receive sensor values
 * @return 0 Ok, -1 otherwise
 */
int sim_adcs_get_sunsensor(uint8_t channel, int *sun_read);

/**
 * Set magnetorquer value
 * @param channel Magnetorquer channel
 * @param torque Torque value
 * @return 0 Ok, -1 otherwise
 */
int sim_adcs_set_magnetorquer(uint8_t channel, uint8_t torque);

#endif //SUCHAI_FLIGHT_SOFTWARE_ADCS_H
