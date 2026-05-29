//
// Created by carlos on 2/29/24.
//

#include "drivers-sim/cdh.h"

int sim_obc_get_temp(int *temp)
{
    uint8_t send[2] = {SIM_OBC_ID, SIM_OBC_ADDR_TEMP};
    int rc = iface_transaction(send, 2, (uint8_t*)temp, sizeof(int));
    return rc;
}
