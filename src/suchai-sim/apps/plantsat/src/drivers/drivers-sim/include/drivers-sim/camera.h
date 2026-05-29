//
// Created by carlos on 3/15/24.
//

#ifndef SUCHAI_FLIGHT_SOFTWARE_CAMERA_H
#define SUCHAI_FLIGHT_SOFTWARE_CAMERA_H

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "drivers-sim/interface.h"

#define SIM_CAM_ID 0x04
#define SIM_CAM_ADDR_SIZE 0x00
#define SIM_CAM_ADDR_TAKE 0x01
#define SIM_CAM_PATH_LEN 256

/**
 * Take a picture
 * @param path Pointer to a buffer to receive the image path
 * @param len Size of the buffer to receive the image path. See SIM_CAM_PATH_LEN
 * @return 0 if Ok, -1 otherwise
 */
int sim_camera_take(char *path, size_t len);

/**
 * Configure image size
 * @param size_x Image size in pixels x
 * @param size_y Image size in pixels y
 * @return @return 0 if Ok, -1 otherwise
 */
int sim_camera_set_size(int size_x, int size_y);

#endif //SUCHAI_FLIGHT_SOFTWARE_CAMERA_H
