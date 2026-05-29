/**
 * @file  cmdCAM.h
 * @author Carlos Gonzalez - contacto.carlosgonzalez@gmail.com
 * @date 2024
 * @copyright GNU GPL v3
 *
 * This header have definitions of commands related to the camera system
 */

#ifndef _CMDCAM_H
#define _CMDCAM_H

#include "app/system/config.h"
#include "config.h"
#include "suchai/globals.h"
#include "suchai/log_utils.h"

#include "suchai/repoCommand.h"
#include "app/system/cmdCDH.h"

#ifdef SIM
#include "drivers-sim/camera.h"
#endif

#define CAM_DEFAULT_GND_NODE 10

/**
 * Register command and data handling (C&DH) commands
 */
void cmd_cam_init(void);

/**
 * Set camera images size
 * @param fmt "%d %d"
 * @param params <sizex> <sizey>
 * @param nparams 2
 * @return CMD_OK if executed correctly, otherwise CMD_ERROR
 */
int cam_set_size(char *fmt, char *params, int nparams);

/**
 * Take a picture
 * @param fmt ""
 * @param params ""
 * @param nparams 0
 * @return CMD_OK if executed correctly, otherwise CMD_ERROR
 */
int cam_take_picture(char *fmt, char *params, int nparams);

#endif //_CMDCAM_H
