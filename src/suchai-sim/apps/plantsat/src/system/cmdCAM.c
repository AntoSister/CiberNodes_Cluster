/*                                 SUCHAI
 *                      NANOSATELLITE FLIGHT SOFTWARE
 *
 *      Copyright 2024, Carlos Gonzalez Cortes, contacto.carlosgonzalez@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "app/system/cmdCAM.h"

static const char* tag = "cmd_cam";

void cmd_cam_init(void)
{
    /** OBC COMMANDS **/
    cmd_add("cam_set_size", cam_set_size, "%d %d",  2);
    cmd_add("cam_take_img", cam_take_picture, "%d", 1);
}

int cam_set_size(char *fmt, char *params, int nparams) {
    int size_x, size_y;
    int rc = -1;

    if(params == NULL || sscanf(params, fmt, &size_x, &size_y) != nparams)
    {
        return CMD_SYNTAX_ERROR;
    }

#ifdef SIM
    rc = sim_camera_set_size(size_x, size_y);
#endif
    return rc == 0 ? CMD_OK : CMD_ERROR;

}

int cam_take_picture(char *fmt, char *params, int nparams) {
    if(params == NULL)
        return CMD_SYNTAX_ERROR;

    int gnd_node;
    if(sscanf(params, fmt, &gnd_node) != nparams)
    {
        gnd_node = CAM_DEFAULT_GND_NODE;
        LOGW(tag, "cam_take_picture using default node %d", gnd_node);
    }

    int rc = -1;

#ifdef SIM
    char path[SIM_CAM_PATH_LEN] = {0};
    rc = sim_camera_take(path, SIM_CAM_PATH_LEN);
    if (rc == 0)
    {
        // Send the file name to the ground station
        _com_send_data(gnd_node, path, strlen(path), TM_TYPE_STRING, 1, 0);
    }
#endif

    return rc != -1 ? CMD_OK : CMD_ERROR;
}
