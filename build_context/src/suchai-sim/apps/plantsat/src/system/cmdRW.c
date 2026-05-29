/*                                 SUCHAI
 *                      NANOSATELLITE FLIGHT SOFTWARE
 *
 *      Copyright 2018, Carlos Gonzalez Cortes, carlgonz@ug.uchile.cl
 *      Copyright 2018, Tomas Opazo Toro, tomas.opazo.t@gmail.com
 *      Copyright 2018, Matias Ramirez Martinez, nicoram.mt@gmail.com
 *      Copyright 2018, Camilo Rojas, camrojas@uchile.cl
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

#include "app/system/cmdRW.h"

//#define RW_COMM_DELAY_MS 500

static const char* tag = "cmd_rw";
static const char* tag2 = "cmd_istage";
static int RW_COMM_DELAY_MS = 150;

void cmd_rw_init(void)
{
    /** RW COMMANDS **/
    cmd_add("rw_get_speed", rw_get_speed, "%d", 1);
    cmd_add("rw_get_current", rw_get_current, "%d", 1);
    cmd_add("rw_get_data", rw_get_data, "0", 0);
    cmd_add("rw_set_speed", rw_set_speed, "%d %d", 2);
    cmd_add("rw_set_delay", rw_set_delay, "%d", 1);
    /** UPPER ISTAGE COMMANDS **/
#ifdef SCH_USE_ISTAGE2
    cmd_add("is2_get_temp", istage2_get_temp, "", 0);
    cmd_add("is2_get_state", istage2_get_state_panel, "%d", 1);
    cmd_add("is2_deploy", istage2_deploy_panel, "%d", 1);
    cmd_add("is2_set_deploy", istage2_set_deploy, "%d", 1);
#endif
}

int rw_set_delay(char *fmt, char *params, int nparams)
{
    int delay;
    if(params == NULL || sscanf(params, fmt, &delay) != nparams) {
        return CMD_SYNTAX_ERROR;
    }
    RW_COMM_DELAY_MS = delay;
    return CMD_OK;
}

int rw_get_speed(char *fmt, char *params, int nparams)
{
    int motorid;
    if(params == NULL || sscanf(params, fmt, &motorid) != nparams) {
        motorid = -1;
    }

    if(motorid > 0 && motorid < 4)
    {
        LOGI(tag, "Getting speed %d", motorid);
        uint16_t speed = rwdrv10987_get_speed(motorid);
        LOGR(tag, "Sampled speed%d: %d", motorid, speed);
    }
    else
    {
        LOGI(tag, "Getting all speeds");
        uint16_t speed1 = rwdrv10987_get_speed(RW_MOTOR1_ID);
        osDelay(RW_COMM_DELAY_MS);
        uint16_t speed2 = rwdrv10987_get_speed(RW_MOTOR2_ID);
        osDelay(RW_COMM_DELAY_MS);
        uint16_t speed3 = rwdrv10987_get_speed(RW_MOTOR3_ID);
        osDelay(RW_COMM_DELAY_MS);
        LOGR(tag, "Sampled speed1: %d, speed2: %d, speed3: %d", speed1, speed2, speed3);
    }

    return CMD_OK; //TODO: check error code
}

int rw_get_current(char *fmt, char *params, int nparams)
{
    int motorid;
    if(params == NULL || sscanf(params, fmt, &motorid) != nparams) {
        motorid = -1;
    }

    if(motorid > 0 && motorid < 4)
    {
        LOGI(tag, "Sampling current %d", motorid)
        float current = rwdrv10987_get_current(motorid); //[mA]
        LOGR(tag, "Sampled current%d: %f", motorid, current);
    }
    else
    {
        LOGI(tag, "Sampling all currents");
        float current1 = rwdrv10987_get_current(RW_MOTOR1_ID); //[mA]
        osDelay(RW_COMM_DELAY_MS);
        float current2 = rwdrv10987_get_current(RW_MOTOR2_ID); //[mA]
        osDelay(RW_COMM_DELAY_MS);
        float current3 = rwdrv10987_get_current(RW_MOTOR3_ID); //[mA]
        osDelay(RW_COMM_DELAY_MS);
        LOGR(tag, "Sampled current1: %f, current2: %f, current3: %f", current1, current2, current3);
    }

    return CMD_OK; //TODO: check error code
}

int rw_get_data(char *fmt, char *params, int nparams)
{
    rw_data_t data;
    data.index = dat_get_system_var(data_map[rw_sensors].sys_index);
    data.timestamp = dat_get_time();

    data.speed1 = rwdrv10987_get_speed(RW_MOTOR1_ID);
    osDelay(RW_COMM_DELAY_MS);
    data.speed2 = rwdrv10987_get_speed(RW_MOTOR2_ID);
    osDelay(RW_COMM_DELAY_MS);
    data.speed3 = rwdrv10987_get_speed(RW_MOTOR3_ID);
    osDelay(RW_COMM_DELAY_MS);
    data.current1 = rwdrv10987_get_current(RW_MOTOR1_ID); //[mA]
    osDelay(RW_COMM_DELAY_MS);
    data.current2 = rwdrv10987_get_current(RW_MOTOR2_ID); //[mA]
    osDelay(RW_COMM_DELAY_MS);
    data.current3 = rwdrv10987_get_current(RW_MOTOR3_ID); //[mA]
    osDelay(RW_COMM_DELAY_MS);

    int rc = dat_add_payload_sample(&data, rw_sensors);
    dat_print_payload_struct(&data, rw_sensors);
    return rc;
}

int rw_set_speed(char *fmt, char *params, int nparams)
{
    LOGI(tag, "Speed command");
    int motor_id;
    int speed;
    uint8_t dir;

    if(params == NULL || sscanf(params, fmt, &motor_id, &speed) != nparams)
        return CMD_SYNTAX_ERROR;
    if(speed != 0 && (speed < -511 || speed > 511 || speed > -100 && speed < 100))
    {
        LOGE(tag, "Invalid speed %d. Use values between [-511, -100], [100, 511] or 0");
        return CMD_SYNTAX_ERROR;
    }

    if(speed < 0) {
        dir = RW_DIR_ANTICLOCKWISE;
        speed = -speed;
    }
    else
        dir = RW_DIR_CLOCKWISE;

    if(motor_id == -1)
    {
        int rc = 0;
        rc += (int) rwdrv10987_set_speed(RW_MOTOR1_ID, (int16_t) speed, dir);
        osDelay(RW_COMM_DELAY_MS);
        rc += (int) rwdrv10987_set_speed(RW_MOTOR2_ID, (int16_t) speed, dir);
        osDelay(RW_COMM_DELAY_MS);
        rc += (int) rwdrv10987_set_speed(RW_MOTOR3_ID, (int16_t) speed, dir);
        osDelay(RW_COMM_DELAY_MS);
        LOGR(tag, "Setting motor: %d speed: %d (%d)", 1, speed, rc);
        LOGR(tag, "Setting motor: %d speed: %d (%d)", 2, speed, rc);
        LOGR(tag, "Setting motor: %d speed: %d (%d)", 3, speed, rc);
        return rc != 0 ? CMD_ERROR : CMD_OK;
    }
    else if(motor_id == RW_MOTOR1_ID || motor_id == RW_MOTOR2_ID || motor_id == RW_MOTOR3_ID)
    {
        int rc = (int) rwdrv10987_set_speed((uint8_t) motor_id, (int16_t) speed, dir);
        LOGR(tag, "Setting motor: %d speed: %d (%d)", motor_id, speed, rc);
        return rc != 0 ? CMD_ERROR : CMD_OK;
    }
    else
        return CMD_SYNTAX_ERROR;
}

/** UPPER ISTAGE COMMANDS **/
#ifdef SCH_USE_ISTAGE2
int istage2_get_temp(char *fmt, char *params, int nparams)
{
    int rc;
    char istage_cmd[2] = {0, 0};
    char istage_ans[1];

    istage_cmd[0] = IS2_START_SENSORS_TEMP;
    rc = gs_i2c_master_transaction(ISTAGE_GSSB_TWI_HANDLER, ISTAGE_UPPER_ADD, istage_cmd, 2, istage_ans, 1, 500);
    LOGI(tag2, "START_SENSORS_TEMP %d (%d)", istage_ans[0], rc);
    if(rc != GS_OK)
        return CMD_ERROR;

    osDelay(100);

    istage_cmd[0] = I2S_SAMPLE_TEMP;
    rc = gs_i2c_master_transaction(ISTAGE_GSSB_TWI_HANDLER, ISTAGE_UPPER_ADD, istage_cmd, 2, istage_ans, 1, 500);
    LOGI(tag2, "SAMPLE_TEMP %d (%d)", istage_ans[0], rc);
    if(rc != GS_OK)
        return CMD_ERROR;

    osDelay(500);

    istage_cmd[0] = IS2_GET_TEMP;
    uint32_t temps[8];
    rc = gs_i2c_master_transaction(ISTAGE_GSSB_TWI_HANDLER, ISTAGE_UPPER_ADD, istage_cmd, 2, (char *)temps, sizeof(temps), 500);
    LOGR(tag2, "IS2_GET_TEMP %f, %f, %f, %f, %f, %f, %f, %f (%d)", temps[0], temps[1], temps[2], temps[3], temps[4], temps[5], temps[6], temps[7], rc);
    if(rc != GS_OK)
        return CMD_ERROR;

    istage_cmd[0] = IS2_STOP_SENSORS_TEMP;
    rc = gs_i2c_master_transaction(ISTAGE_GSSB_TWI_HANDLER, ISTAGE_UPPER_ADD, istage_cmd, 2, istage_ans, 1, 500);
    LOGI(tag2, "STOP_SENSORS_TEMP (%d)", rc);
    if(rc != GS_OK)
        return CMD_ERROR;

    return CMD_OK;

}

int istage2_get_state_panel(char *fmt, char *params, int nparams)
{
    int rc, panel;

    if(params == NULL || sscanf(params, fmt, &panel) != nparams)
    {
        LOGW(tag2, "get_state_panel used with invalid params!");
        return CMD_SYNTAX_ERROR;
    }

    char istage_cmd[2] = {IS2_READ_SW_FACE, (char)panel};
    char istage_ans[2] = {-1, -1};
    rc = gs_i2c_master_transaction(ISTAGE_GSSB_TWI_HANDLER, ISTAGE_UPPER_ADD, istage_cmd, 2, istage_ans, 2, 500);
    LOGR(tag2, "IS2_READ_SW_FACE %d=%d (%d)", panel, istage_ans[1], rc);

    if(rc != GS_OK)
        return CMD_ERROR;
    else
        return CMD_OK;
}

int istage2_deploy_panel(char *fmt, char *params, int nparams)
{
    int rc, panel;

    if(params == NULL || sscanf(params, fmt, &panel) != nparams)
    {
        LOGW(tag2, "deploy_panel used with invalid params!");
        return CMD_SYNTAX_ERROR;
    }

    char istage_cmd[2] = {IS2_BURN_FACE, (char)panel};
    rc = gs_i2c_master_transaction(ISTAGE_GSSB_TWI_HANDLER, ISTAGE_UPPER_ADD, istage_cmd, 2, NULL, 0, 500);
    LOGR(tag2, "IS2_BURN_FACE %d (%d)", panel, rc);

    if(rc != GS_OK)
        return CMD_ERROR;
    else
        return CMD_OK;
}

int istage2_set_deploy(char *fmt, char *params, int nparams)
{
    int rc, config;

    if(params == NULL || sscanf(params, fmt, &config) != nparams)
    {
        LOGW(tag2, "deploy_panel used with invalid params!");
        return CMD_SYNTAX_ERROR;
    }

    char istage_cmd[2] = {IS2_SET_BURN, (char)config};
    rc = gs_i2c_master_transaction(ISTAGE_GSSB_TWI_HANDLER, ISTAGE_UPPER_ADD, istage_cmd, 2, NULL, 0, 500);
    LOGR(tag2, "IS2_SET_BURN %d (%d)", config, rc);

    if(rc != GS_OK)
        return CMD_ERROR;
    else
        return CMD_OK;
}
#endif