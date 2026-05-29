/*                                 SUCHAI
 *                      NANOSATELLITE FLIGHT SOFTWARE
 *
 *      Copyright 2021, Carlos Gonzalez Cortes, carlgonz@ug.uchile.cl
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

#define TM_TYPE_STRING 104

#include "app/system/cmdCDH.h"

static const char* tag = "cmd_cdh";

void cmd_cdh_init(void)
{
    /** OBC COMMANDS **/
    cmd_add("obc_set_mode", obc_set_mode, "%s", 1);
    cmd_add("obc_cancel_deploy", obc_cancel_deploy, "", 0);
    cmd_add("obc_update_status", obc_update_status, "", 0);
    cmd_add("tm_send_msg", tm_send_msg, "%d %s",  2);
    cmd_add("tm_parse_msg", tm_parse_msg, "", 0);
    cmd_add("tm_send_beacon", tm_send_beacon, "%d", 1);
    cmd_add("tm_parse_beacon", tm_parse_beacon, "", 0);
}

int obc_set_mode(char *fmt, char *params, int nparams)
{
    int opmode = DAT_OBC_OPMODE_NORMAL;
    if(params == NULL)
        return CMD_SYNTAX_ERROR;


    if(strncmp(params, "normal", 6) == 0)
        opmode = DAT_OBC_OPMODE_NORMAL;
    else if(strncmp(params, "deploy", 6) == 0)
        opmode = DAT_OBC_OPMODE_DEPLOYING;
    else if(strncmp(params, "safe", 4) == 0)
        opmode = DAT_OBC_OPMODE_WARN;
    else if(strncmp(params, "point", 5) == 0)
        opmode = DAT_OBC_OPMODE_REF_POINT;
    else if(strncmp(params, "nadir", 5) == 0)
        opmode = DAT_OBC_OPMODE_NAD_POINT;
    else if(strncmp(params, "detumb", 6) == 0)
        opmode = DAT_OBC_OPMODE_DETUMB_MAG;
    else
    {
        LOGE(tag, "Invalid mode %s. Select from normal, deploy, safe, point, nadir, detumb", params)
        return CMD_SYNTAX_ERROR;
    }

    LOGR(tag, "Opmode %s (%d) selected!", params, opmode);
    dat_set_system_var(dat_obc_opmode, opmode);

    return CMD_OK;
}

int obc_cancel_deploy(char *fmt, char *params, int nparams)
{
    int current_opmode = dat_get_system_var(dat_obc_opmode);
    if( current_opmode == DAT_OBC_OPMODE_DEPLOYING)
    {
        LOGR(tag, "Set opmode from deploying (%d) to normal (%d)", DAT_OBC_OPMODE_DEPLOYING, DAT_OBC_OPMODE_NORMAL);
        dat_set_system_var(dat_obc_opmode, DAT_OBC_OPMODE_NORMAL);
        return CMD_OK;
    }
    else
    {
        LOGW(tag, "Opmode not changed because was not deploying (%d)", current_opmode)
        return CMD_ERROR;
    }
}

int obc_update_status(char *fmt, char *params, int nparams)
{
    int rc = 0;
    int t_obc = 0;

#if defined(NANOMIND)
    int16_t t_obc1 = 0, t_obc2 = 0;
    rc += gs_lm71_read_temp(GS_A3200_SPI_SLAVE_LM71_0, 100, &t_obc1);
    rc += gs_lm71_read_temp(GS_A3200_SPI_SLAVE_LM71_1, 100, &t_obc2);
    if(rc != 0)
    {
        LOGE(tag, "Error reading OBC temperatures");
        return CMD_ERROR;
    }
    t_obc = (t_obc1 + t_obc2) * 10 / 2;
#elif defined(SIM)
    rc = sim_obc_get_temp(&t_obc);
    if(rc != 0)
    {
        LOGE(tag, "Error reading OBC temperatures");
        return CMD_ERROR;
    }
#endif
    LOGI(tag, "OBC TEMP: %d ºC", t_obc);
    rc = dat_set_system_var(dat_obc_temp_1, t_obc);
    return rc == 0 ? CMD_OK : CMD_ERROR;
}

int tm_send_msg(char *fmt, char *params, int nparams) {
    int node;
    char msg[SCH_ST_STR_SIZE];

    if(params == NULL || sscanf(params, fmt, &node, msg) != nparams)
    {
        return CMD_SYNTAX_ERROR;
    }

    LOGI(tag, "Message string is: %s", msg);
    return com_send_telemetry(node, SCH_TRX_PORT_CDH, TM_TYPE_STRING, msg, sizeof(msg), 1, 0);
}

int tm_parse_msg(char *fmt, char *params, int nparams) {
    if(params == NULL)
        return CMD_SYNTAX_ERROR;

    com_frame_t *frame = (com_frame_t *)params;
    char msg[SCH_ST_STR_SIZE];
    strncpy(msg, (char *)frame->data.data8, SCH_ST_STR_SIZE);

    string_data_t message;
    message.index = dat_get_system_var(data_map[msg_sensors].sys_index);
    //message.index = dat_get_system_var(dat_drp_idx_str);
    message.timestamp = dat_get_time();
    memset(message.msg, 0, SCH_ST_STR_SIZE);
    strcpy(message.msg, (char *)frame->data.data8);

    int rc = dat_add_payload_sample(&message, msg_sensors);
    LOGI(tag, "String message is %s", message.msg);
    return rc != -1 ? CMD_OK : CMD_ERROR;
}

int tm_send_beacon(char *fmt, char *params, int nparams)
{
    int node;
    if(params == NULL || sscanf(params, fmt, &node) != nparams)
    {
        return CMD_SYNTAX_ERROR;
    }

    status_data_t status;
    obc_read_status_basic(&status);
    _hton32_buff((uint32_t *)&status, sizeof(status_data_t)/sizeof(uint32_t));
    return com_send_telemetry(node, SCH_TRX_PORT_CDH, TM_TYPE_PAYLOAD_STA, &status, sizeof(status_data_t), 1, 0);
}

int tm_parse_beacon(char *fmt, char *params, int nparams)
{
    if(params == NULL)
        return CMD_SYNTAX_ERROR;

    com_frame_t *frame = (com_frame_t *)params;
    status_data_t sta_data;
    memcpy(&sta_data, frame->data.data8, sizeof(status_data_t));
    _ntoh32_buff((uint32_t *)&sta_data, sizeof(status_data_t)/sizeof(uint32_t));
    dat_print_payload_struct(&sta_data, status_sensors);
    return CMD_OK;
}

int obc_read_status_basic(status_data_t *status)
{
    status->timestamp = dat_get_time();
    status->index = dat_get_system_var(data_map[temp_sensors].sys_index);
    status->dat_obc_opmode = dat_get_system_var(dat_obc_opmode);
    status->dat_rtc_date_time = dat_get_system_var(dat_rtc_date_time);
    status->dat_obc_last_reset = dat_get_system_var(dat_obc_last_reset);
    status->dat_obc_hrs_alive = dat_get_system_var(dat_obc_hrs_alive);
    status->dat_obc_hrs_wo_reset = dat_get_system_var(dat_obc_hrs_wo_reset);
    status->dat_obc_reset_counter = dat_get_system_var(dat_obc_reset_counter);
    status->dat_obc_executed_cmds = dat_get_system_var(dat_obc_executed_cmds);
    status->dat_obc_failed_cmds = dat_get_system_var(dat_obc_failed_cmds);
    status->dat_com_count_tm = dat_get_system_var(dat_com_count_tm);
    status->dat_com_count_tc = dat_get_system_var(dat_com_count_tc);
    status->dat_com_last_tc = dat_get_system_var(dat_com_last_tc);
    status->dat_fpl_last = dat_get_system_var(dat_fpl_last);
    status->dat_fpl_queue = dat_get_system_var(dat_fpl_queue);
    status->dat_ads_tle_epoch = dat_get_system_var(dat_ads_tle_epoch);
    status->dat_eps_vbatt = dat_get_system_var(dat_eps_vbatt);
    status->dat_eps_cur_sun = dat_get_system_var(dat_eps_cur_sun);
    status->dat_eps_cur_sys = dat_get_system_var(dat_eps_cur_sys);
    status->dat_obc_temp_1 = dat_get_system_var(dat_obc_temp_1);
    status->dat_eps_temp_bat0 = dat_get_system_var(dat_eps_temp_bat0);
    status->dat_drp_mach_action = dat_get_system_var(dat_drp_mach_action);
    status->dat_drp_mach_state = dat_get_system_var(dat_drp_mach_state);
    status->dat_drp_mach_payloads = dat_get_system_var(dat_drp_mach_payloads);
    status->dat_drp_mach_step = dat_get_system_var(dat_drp_mach_step);
    return CMD_OK;
}