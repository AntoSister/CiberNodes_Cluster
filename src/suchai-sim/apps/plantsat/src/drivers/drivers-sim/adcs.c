//
// Created by carlos on 2/29/24.
//

#include "drivers-sim/adcs.h"

int sim_adcs_get_magnetometer(mag_read_t *mag_read)
{
    assert(sizeof(mag_read_t) == 12);
    uint8_t send[2] = {SIM_ADCS_ID, SIM_ADCS_ADDR_MAG};
    int rc = iface_transaction(send, 2, (uint8_t*)mag_read, sizeof(mag_read_t));
    return rc;
}

int sim_adcs_get_gyroscope(gyro_read_t *gyro_read)
{
    assert(sizeof(gyro_read_t) == 12);
    uint8_t send[2] = {SIM_ADCS_ID, SIM_ADCS_ADDR_GYR};
    int rc = iface_transaction(send, 2, (uint8_t*)gyro_read, sizeof(gyro_read_t));
    return rc;
}

int sim_adcs_get_sunsensor(uint8_t channel, int *sun_read)
{
    uint8_t send[3] = {SIM_ADCS_ID, SIM_ADCS_ADDR_SUN, channel};
    int rc = iface_transaction(send, 3, (uint8_t*)sun_read, sizeof(int));
    return rc;
}

int sim_adcs_set_magnetorquer(uint8_t channel, uint8_t torque)
{
    uint8_t send[4] = {SIM_ADCS_ID, SIM_ADCS_ADDR_MTT, channel, torque};
    int status = -1;
    int rc = iface_transaction(send, 4, (uint8_t*)&status, sizeof(int));
    return rc;
}
