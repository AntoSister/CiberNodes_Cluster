//
// Created by carlos on 3/15/24.
//

#include "drivers-sim/camera.h"

int sim_camera_take(char *path, size_t len)
{
    int status;
    uint8_t send[2] = {SIM_CAM_ID, SIM_CAM_ADDR_TAKE};
    char recv[SIM_CAM_PATH_LEN] = {0};
    int rc = iface_transaction(send, 2, recv, SIM_CAM_PATH_LEN);
    if (rc == 0)
    {
        // Check if the returned path is valid
        rc = access(recv, F_OK);
        if (rc == 0)
        {
            assert(strlen(recv) < len);
            strncpy(path, recv, len);
        }
    }
    return rc;
}

int sim_camera_set_size(int size_x, int size_y)
{
    int status;
    uint8_t send[2+2*sizeof(int)];
    // Copy codes
    send[0] = SIM_CAM_ID;
    send[1] = SIM_CAM_ADDR_SIZE;
    // Copy parameters
    int sizes[2] = {size_x, size_y};
    memcpy(send+2, sizes, sizeof(sizes));
    // Send command
    int rc = iface_transaction(send, sizeof(send), (uint8_t*)&status, sizeof(int));
    return rc;
}
