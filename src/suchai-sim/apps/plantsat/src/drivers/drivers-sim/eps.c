//
// Created by carlos on 2/29/24.
//

#include "drivers-sim/eps.h"

int sim_eps_get_hk(eps_hk_read_t *hk_read)
{
    uint8_t send[2] = {SIM_EPS_ID, SIM_EPS_ADDR_HKP};
    int rc = iface_transaction(send, 2, (uint8_t*)hk_read, sizeof(eps_hk_read_t));
    return rc;
}

int sim_eps_set_output(uint8_t channel, uint8_t mode)
{
    uint8_t send[4] = {SIM_EPS_ID, SIM_EPS_ADDR_SET, channel, mode};
    int status = -1;
    int rc = iface_transaction(send, 4, (uint8_t*)&status, sizeof(int));
    return rc;
}

int sim_eps_set_heater(uint8_t mode)
{
    uint8_t send[3] = {SIM_EPS_ID, SIM_EPS_ADDR_HEATER, mode};
    int status = -1;
    int rc = iface_transaction(send, 3, (uint8_t*)&status, sizeof(int));
    return rc;
}

int sim_eps_hard_reset()
{
    uint8_t send[2] = {SIM_EPS_ID, SIM_EPS_ADDR_HARD_RESET};
    int status = -1;
    int rc = iface_transaction(send, 2, (uint8_t*)&status, sizeof(int));
    rc = rc == 0 && status > 0 ? 0 : -1;  // Return Ok (0) only if the EPS answered with status 1
    return rc;
}