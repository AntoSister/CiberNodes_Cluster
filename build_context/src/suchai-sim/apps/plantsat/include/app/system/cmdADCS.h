/**
 * @file  cmdADCS.h
 * @author Carlos Gonzalez C carlgonz@uchile.cl
 * @author Camilo Rojas M - camrojas@uchile.cl
 * @author Gustavo Diaz H - gustavo.diaz@ing.uchile.cl
 * @author Elias Obreque S - elias.obreque@uchile.cl
 * @author Javier Morales R - javiermoralesr95@gmail.com
 * @author Luis Jimenez V - luis.jimenez@ing.uchile.cl
 * @date 2021
 * @copyright GNU Public License.
 *
 * This header contains commands related with the ADCS system
 */

#ifndef _CMDADCS_H
#define _CMDADCS_H

#include <math.h>

//#include "drivers.h"
//#include "csp/csp.h"
#include "suchai/math_utils.h"

#include "suchai/repoData.h"
#include "suchai/repoCommand.h"
#include "suchai/cmdCOM.h"
//#include "suchai/math_utils.h"
#include "suchai/log_utils.h"

/**
 * Register ADCS commands
 */
void cmd_adcs_init(void);

/**
 * Log current TLE lines
 * @param fmt Str. Parameters format ""
 * @param params Str. Parameters as string ""
 * @param nparams Int. Number of parameters 0
 * @return  CMD_OK if executed correctly, CMD_ERROR in case of failures, or CMD_ERROR_SYNTAX in case of parameters errors
 */
int tle_get(char *fmt, char *params, int nparams);

/**
 * Set TLE lines, once a at time. Call this command two times, one for each
 * line to set up the TLE. The command recognizes the line number by the first
 * parameter of each TLE line, the stores the full file.
 * @warning This command do not update the TLE structure. It only saves the TLE
 * lines. Normally it requires to also call the tle_update command.
 *
 * Example 1, from code:
 * @code
 *  cmd_t *tle1 = cmd_get_str("tle_set");
 *  cmd_add_params_str(tle1, "1 42788U 17036Z   20027.14771603  .00000881  00000-0  39896-4 0  9992");
 *  cmd_send(tle1);
 *
 *  cmd_t *tle2 = cmd_get_str("tle_set");
 *  cmd_add_params_str(tle2, "2 42788  97.3234  85.2817 0012095 159.3521 200.8207 15.23399088144212");
 *  cmd_send(tle2);
 *
 *  cmd_t *tle_u = cmd_get_str("tle_update");
    cmd_send(tle_u);
 * @endcode
 *
 * Example 2, from the serial console or telecommand
 * @code
 * tle_set 1 42788U 17036Z   20027.14771603  .00000881  00000-0  39896-4 0  9992
 * tle_set 2 42788  97.3234  85.2817 0012095 159.3521 200.8207 15.23399088144212
 * tle_update
 * @endcode
 *
 * @param fmt Str. Parameters format "%d %n"
 * @param params Str. Parameters as string "<line (69 chars including line number)>"
 * @param nparams Int. Number of parameters 2
 * @return  CMD_OK if executed correctly, CMD_ERROR in case of failures, or CMD_ERROR_SYNTAX in case of parameters errors
 */
int tle_set(char *fmt, char *params, int nparams);

/**
 * Update the TLE structure using the received TLE lines
 * @warning This command do not receive the TLE lines. It only updates the TLE
 * structure. Normally it requires to also call the tle_set command.
 *
 * Example 1, from code:
 * @code
 *  cmd_t *tle1 = cmd_get_str("tle_set");
 *  cmd_add_params_str(tle1, "1 42788U 17036Z   20027.14771603  .00000881  00000-0  39896-4 0  9992");
 *  cmd_send(tle1);
 *
 *  cmd_t *tle2 = cmd_get_str("tle_set");
 *  cmd_add_params_str(tle2, "2 42788  97.3234  85.2817 0012095 159.3521 200.8207 15.23399088144212");
 *  cmd_send(tle2);
 *
 *  cmd_t *tle_u = cmd_get_str("tle_update");
    cmd_send(tle_u);
 * @endcode
 *
 * Example 2, from the serial console or telecommand
 * @code
 * tle_set 1 42788U 17036Z   20027.14771603  .00000881  00000-0  39896-4 0  9992
 * tle_set 2 42788  97.3234  85.2817 0012095 159.3521 200.8207 15.23399088144212
 * tle_update
 * @endcode
 *
 * @param fmt Str. Parameters format ""
 * @param params Str. Parameters as string ""
 * @param nparams Int. Number of parameters 0
 * @return  CMD_OK if executed correctly, CMD_ERROR in case of failures, or CMD_ERROR_SYNTAX in case of parameters errors
 */
int tle_update(char *fmt, char *params, int nparams);

/**
 * Propagate the TLE to the given datetime to update satellite position in ECI
 * reference. The result is stored in the system variables. The command receives
 * the datetime to propagate in unix timestamp format. If the parameter is 0
 * then uses the current date and time.
 * @warning This command do not receive the TLE lines, nor update the TLE.
 * It only propagates the TLE. Normally it requires to also call the
 * tle_set and obc_update command.
 *
 * Example 1, from code:
 * @code
 *  // Set and update the TLE previously
 *  cmd_t *cmd_prop = cmd_get_str("tle_prop");
 *  cmd_add_params_var(cmd_prop, dt, 1582643144);
 *  cmd_send(cmd_prop);
 * @endcode
 *
 * Example 2, from the serial console or telecommand
 * @code
 * // Prop. TLE to datetime
 * tle_prop 1582643144
 * // Prop. TLE to current datetime
 * tle_prop 0
 * @endcode
 *
 * @param fmt Str. Parameters format "%ld"
 * @param params Str. Parameters as string "<unix_timestamp | 0>"
 * @param nparams Int. Number of parameters 1
 * @return  CMD_OK if executed correctly, CMD_ERROR in case of failures, or CMD_ERROR_SYNTAX in case of parameters errors
 */
int tle_prop(char *fmt, char *params, int nparams);

/**
 * Change <duty> cycle of pwm <channel>, so use this command carefully.
 * @warning only available in Nanomind
 * <duty> as percentage: 0-100, 10% ~ 1.0V RMS and 90% ~ 3.0V RMS
 * <channel> 0:X, 1:Y, 2:Z
 *
 * @param fmt str. Parameters format: "%d %d"
 * @param params  str. Parameters as string <channel> <duty>,
 * @param nparams int. Number of parameters: 2
 * @return CMD_OK if executed correctly, CMD_ERROR in case of failures, or CMD_ERROR_SYNTAX in case of parameters errors
 */
int mtt_set_pwm_duty(char* fmt, char* params, int nparams);

/**
 * Change pwm <channel> freq to <freq>, so use this command carefully.
 * @warning only available in Nanomind
 * <freq> as hz: 0.1-433.0 Hz
 * <channel> 0:X, 1:Y, 2:Z
 *
 * @param fmt str. Parameters format: "%d %f"
 * @param params  str. Parameters as string <channel> <freq>,
 * @param nparams int. Number of parameters: 2
 * @return CMD_OK if executed correctly, CMD_ERROR in case of failures, or CMD_ERROR_SYNTAX in case of parameters errors
 */
int mtt_set_pwm_freq(char* fmt, char* params, int nparams);

/**
 * Set PWM Switch on/off
 * @warning only available in Nanomind
 * <enable> 1:on, 0:off (>0:on, <=0: off)
 *
 * @param fmt str. Parameters format: "%d"
 * @param params  str. Parameters as string <enable>,
 * @param nparams int. Number of parameters: 1
 * @return CMD_OK if executed correctly, CMD_ERROR in case of failures, or CMD_ERROR_SYNTAX in case of parameters errors
 */
int mtt_set_pwm_pwr(char *fmt, char *params, int nparams);





/**
 * Send "adcs_point_to <x> <y> <z>" command to the ADCS system with current
 * position vector.
 *
 * @param fmt ""
 * @param params ""
 * @param nparams 0
 * @return CMD_OK | CMD_ERROR_SYNTAX | CMD_ERROR
 */
int adcs_point(char* fmt, char* params, int nparams);

/**
 * Read current spacecraft quaternion from the ADCS/STT
 * @param fmt ""
 * @param params ""
 * @param nparams 0
 * @return CMD_OK | CMD_ERROR_SYNTAX | CMD_ERROR
 */
int adcs_get_quaternion(char* fmt, char* params, int nparams);

/**
 * Read gyroscopes. Update values in status variables.
 * @param fmt ""
 * @param params ""
 * @param nparams 0
 * @return CMD_OK | CMD_ERROR_SYNTAX | CMD_ERROR
 */
int adcs_get_omega(char* fmt, char* params, int nparams);

/**
 * Read magnetic sensors. Update values in status variables.
 * @param fmt ""
 * @param params ""
 * @param nparams 0
 * @return CMD_OK | CMD_ERROR_SYNTAX | CMD_ERROR
 */
int adcs_get_mag(char* fmt, char* params, int nparams);

/**
 *
 * @param fmt Str. Parameters format "%lf"
 * @param params Str. Parameters as string: "<control cycle>"
 * @param nparams Int. Number of parameters 1
 * @return CMD_OK if executed correctly, CMD_ERROR in case of failures, or CMD_ERROR_SYNTAX in case of parameters errors
 */
int adcs_control_torque(char* fmt, char* params, int nparams);

/**
 *
 * @param fmt ""
 * @param params ""
 * @param nparams 0
 * @return CMD_OK if executed correctly, CMD_ERROR in case of failures, or CMD_ERROR_SYNTAX in case of parameters errors
 */
int adcs_mag_moment(char* fmt, char* params, int nparams);

/**
 * Set ADCS vector (Intertial frame) and velocity (body frame) targets
 * @param fmt "%lf lf lf lf lf lf"
 * @param params "<x y z> <wx wy wz>"
 * @param nparams 6
 * @return CMD_OK | CMD_ERROR | CMD_ERROR_SYNTAX
 */
int adcs_set_target(char* fmt, char* params, int nparams);

/**
 * Set ADCS target to Nadir based on current quaternion and position
 * Uses sim_adcs_set_target
 * @param fmt ""
 * @param params ""
 * @param nparams 0
 * @return CMD_OK | CMD_ERROR | CMD_ERROR_SYNTAX
 */
int adcs_target_nadir(char* fmt, char* params, int nparams);

/**
 * Set ADCS in detumbling mode
 * @param fmt ""
 * @param params ""
 * @param nparams 0
 * @return CMD_OK | CMD_ERROR | CMD_ERROR_SYNTAX
 */
int adcs_detumbling_mag(char* fmt, char* params, int nparams);

/**
 * Send current attitude variables to ADCS system. For testing purposes.
 * @param fmt ""
 * @param params ""
 * @param nparams 0
 * @return CMD_OK | CMD_ERROR | CMD_ERROR_SYNTAX
 */
int adcs_send_attitude(char* fmt, char* params, int nparams);
int calc_shadow_zone(vector3_t sun_pos_i, vector3_t sc_pos_i);
int get_obc_sun_vec(char* fmt, char* params, int nparams);
int start_attitude(char *fmt, char *params, int nparams);
int set_sc_inertia_matrix(char *fmt, char *params, int nparams);
int set_rw_inertia_matrix(char *fmt, char *params, int nparams);
int set_time_delay_gyro(char *fmt, char *params, int nparams);
int set_time_delay_quat(char *fmt, char *params, int nparams);
int set_mtq_axis(char *fmt, char *params, int nparams);
int set_quat_fss(char *fmt, char *params, int nparams);
int set_bias_omega(char *fmt, char *params, int nparams);
#endif //_CMDADCS_H
