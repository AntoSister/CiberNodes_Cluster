/*                                 SUCHAI
 *                      NANOSATELLITE FLIGHT SOFTWARE
 *
 *      Copyright 2020, Carlos Gonzalez Cortes, carlgonz@uchile.cl
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

#include "app/system/cmdAX100.h"
#include "suchai/taskConsole.h"

#ifndef SCH_TRX_ADDRESS
#define SCH_TRX_ADDRESS 5
#endif

static const char *tag = "cmdAX100";
static char trx_node = 29; //GS100 default 29

static int sat_freqs[3] = {437230000, 437250000, 437240000};  // SCH2, SCH3, PS

static void _com_config_help(void);
static void _com_config_find(char *param_name, int table, const param_table_t **param);

void cmd_ax100_init(void)
{
    cmd_add("com_set_node", com_set_node, "%d", 1);
    cmd_add("com_get_node", com_get_node, "", 0);
    cmd_add("com_reset_wdt", com_reset_wdt, "%d", 1);
    cmd_add("com_get_config", com_get_config, "%d %s", 2);
    cmd_add("com_set_config", com_set_config, "%d %s %s", 3);
    cmd_add("com_update_status", com_update_status_vars, "", 0);
    cmd_add("com_set_beacon", com_set_beacon, "%d %d", 2);
    cmd_add("com_set_uplink", com_set_uplink, "%d", 1);
    cmd_add("com_set_downlink", com_set_downlink, "%d", 1);
    cmd_add("com_set_sat", com_set_satellite, "%s", 1);
}

int com_set_node(char *fmt, char *params, int nparams)
{
    if(params == NULL)
    {
        LOGE(tag, "Null arguments!");
        return CMD_SYNTAX_ERROR;
    }

    int node;
    if(sscanf(params, fmt, &node) == nparams)
    {
        trx_node = node;
        LOGR(tag, "TRX node set to %d", node);
        return CMD_OK;
    }
    return CMD_SYNTAX_ERROR;
}

int com_get_node(char *fmt, char *params, int nparams)
{
    LOGR(tag, "TRX Node: %d\n", trx_node);
    return CMD_OK;
}

int com_reset_wdt(char *fmt, char *params, int nparams)
{

    int rc, node, n_args = 0;

    // If no params received, try to reset the default trx_node node
    if(params == NULL || sscanf(params, fmt, &node) != nparams)
        node = (int)trx_node;

    // Send and empty message to GNDWDT_RESET (9) port
    rc = csp_transaction(CSP_PRIO_CRITICAL, node, AX100_PORT_GNDWDT_RESET, 1000, NULL, 0, NULL, 0);

    if(rc > 0)
    {
        LOGV(tag, "GND Reset sent successfully. (rc: %d)", rc);
        return CMD_OK;
    }
    else
    {
        LOGE(tag, "Error sending GND Reset. (rc: %d)", rc);
        return CMD_ERROR;
    }
}

int com_get_hk(char *fmt, char *params, int nparams)
{
    //TODO: Implement
    return CMD_ERROR;
}

int com_get_config(char *fmt, char *params, int nparams)
{
    int rc, n_args;
    int table;
    char param[SCH_CMD_MAX_STR_PARAMS];
    memset(param, '\0', SCH_CMD_MAX_STR_PARAMS);

    if(params == NULL)
    {
        LOGE(tag, "Null arguments!");
        return CMD_SYNTAX_ERROR;
    }

    // Format: <table> <param_name>
    n_args = sscanf(params, fmt, &table, &param);
    if(n_args == nparams)
    {
        const param_table_t *param_i;

        // If param is 'help' then show the available param names
        if(strcmp(param, "help") == 0)
        {
            _com_config_help();
            return CMD_OK;
        }

        // Find the given parameter by name and get the size, index, type and
        // table; param_i is set to NULL if the parameter is not found.
        _com_config_find(param, table, &param_i);

        // Warning if the parameter name was not found
        if(param_i == NULL)
        {
            LOGW(tag, "Param %s not found in table %d", param, table);
            return CMD_ERROR;
        }

        // Actually get the parameter value
        void *out = malloc(param_i->size);
        rc = rparam_get_single(out, param_i->addr, param_i->type, param_i->size,
                table, trx_node, AX100_PORT_RPARAM, 1000);

        // Process the answer
        if(rc > 0)
        {
            char param_str[SCH_CMD_MAX_STR_PARAMS];
            param_to_string(param_i, param_str, 0, out, 1, SCH_CMD_MAX_STR_PARAMS) ;
            LOGR(tag, "Param %s (table %d): %s", param_i->name, table, param_str);
            free(out);
            return CMD_OK;
        }
        else
        {
            LOGE(tag, "Error getting parameter %s! (rc: %d)", param, rc);
            free(out);
            return CMD_ERROR;
        }
    }
    return CMD_SYNTAX_ERROR;
}

int com_set_config(char *fmt, char *params, int nparams)
{
    int rc, n_args;
    int table;
    char param[SCH_CMD_MAX_STR_PARAMS];
    char value[SCH_CMD_MAX_STR_PARAMS];
    memset(param, '\0', SCH_CMD_MAX_STR_PARAMS);
    memset(value, '\0', SCH_CMD_MAX_STR_PARAMS);

    if(params == NULL)
    {
        LOGE(tag, "Null arguments!");
        return CMD_SYNTAX_ERROR;
    }

    // Format: <param_name> <value>
    n_args = sscanf(params, fmt, &table, &param, &value);
    if(n_args == nparams)
    {
        const param_table_t *param_i;

        // If param is 'help' then show the available param names
        if(strcmp(param, "help") == 0)
        {
            _com_config_help();
            return CMD_OK;
        }

        // Find the given parameter by name and get the size, index, type and
        // table; param_i is set to NULL if the parameter is not found.
        _com_config_find(param, table, &param_i);

        // Warning if the parameter name was not found
        if(param_i == NULL)
        {
            LOGW(tag, "Param %s not found in table %d!", param, table);
            return CMD_ERROR;
        }

        // Actually get the parameter value
        void *out = malloc(param_i->size);
        param_from_string(param_i, value, out);
        rc = rparam_set_single(out, param_i->addr, param_i->type, param_i->size,
                               table, trx_node, AX100_PORT_RPARAM, 1000);

        // Process the answer
        if(rc > 0)
        {
            char param_str[SCH_CMD_MAX_STR_PARAMS];
            param_to_string(param_i, param_str, 0, out, 1, SCH_CMD_MAX_STR_PARAMS);
            LOGR(tag, "Param %s (table %d) set to: %s", param_i->name, table, param_str);
            free(out);
            return CMD_OK;
        }
        else
        {
            LOGE(tag, "Error setting parameter %s! (rc: %d)", param, rc);
            free(out);
            return CMD_ERROR;
        }
    }

    return CMD_SYNTAX_ERROR;
}

int com_update_status_vars(char *fmt, char *params, int nparams)
{
    char *names[5] = {"freq", "tx_pwr", "baud", "mode", "bcn_interval"};
    int tables[5] = {AX100_PARAM_TX(0), AX100_PARAM_RUNNING, AX100_PARAM_TX(0), AX100_PARAM_TX(0), AX100_PARAM_RUNNING};
    dat_status_address_t vars[5] = {dat_com_freq, dat_com_tx_pwr, dat_com_bcn_period,
                             dat_com_mode, dat_com_bcn_period};
    int table = 0;
    const param_table_t *param_i = NULL;
    int rc;

    int i = 0;
    for(i=0; i<5; i++)
    {
        // Find the given parameter by name and get the size, index, type and
        // table; param_i is set to NULL if the parameter is not found.
        table = tables[i];
        _com_config_find(names[i], table, &param_i);

        // Warning if the parameter name was not found
        if(param_i == NULL)
            LOGE(tag, "Parameter (%d) %s not found!", table, names[i]);

        // Actually get the parameter value
        void *out = malloc(param_i->size);
        rc = rparam_get_single(out, param_i->addr, param_i->type, param_i->size,
                               table, trx_node, AX100_PORT_RPARAM, 1000);

        // Process the answer, save value to status variables
        if(rc > 0)
        {
            if(param_i->size == sizeof(int))
                dat_set_system_var(vars[i], *((int *)out));
            else if(param_i->size == sizeof(uint8_t))
                dat_set_system_var(vars[i], *((uint8_t *)out));
            else
                LOGE(tag, "Error casting status variable");

            LOGR(tag, "Param %s (table %d) %d", param_i->name, table, dat_get_system_var(vars[i]));
            free(out);
        }
    }

    return CMD_OK;
}


/* Auxiliary functions */

/**
 * Print the list of available parameters
 */
void _com_config_help(void)
{
    int i;
    LOGI(tag, "List of available TRX parameters:")
    LOGR(tag, "TABLE %d\n", AX100_PARAM_RUNNING);
    for(i=0; i<ax100_config_count; i++)
    {
        LOGR(tag, "\t%s\n", ax100_config[i].name);
    }
    LOGR(tag, "TABLE %d\n", AX100_PARAM_TX(0));
    for(i=0; i<ax100_config_tx_count; i++)
    {
        LOGR(tag, "\t%s\n", ax100_tx_config[i].name);
    }
    LOGR(tag, "TABLE %d\n", AX100_PARAM_RX);
    for(i=0; i<ax100_config_tx_count; i++)
    {
        LOGR(tag, "\t%s\n", ax100_rx_config[i].name);
    }
}

/**
 * Find the parameter table structure and table index by name. This function is
 * used before @rparam_get_single and @rparam_set_single to obtain the parameter
 * type, index and size. If not found, the @param is set to NULL.
 *
 * @param param_name Str. Parameter name
 * @param table int *. The parameter table index will be stored here.
 * @param param param_table_t *. The parameter type, size and index will be
 * stored here. If the parameter is not found, this pointer is set to NULL.
 */
void _com_config_find(char *param_name, int table, const param_table_t **param)
{
    int i = 0;
    *param = NULL;


    // Find the given parameter name in the AX100 CONFIG table
    if(table == AX100_PARAM_RUNNING)
    {
        for(i=0; i < ax100_config_count; i++)
        {
            //LOGD(tag, "%d, %s\n", i, ax100_config[i].name);
            if(strcmp(param_name, ax100_config[i].name) == 0)
            {
                *param = &(ax100_config[i]);
                LOGD(tag, "%d, %d, %s\n", i, table, ax100_config[i].name);
                return;
            }
        }
    }

    // Find the given parameter name in the AX100 RX table
    if(table == AX100_PARAM_RX)
    {
        for(i = 0; i < ax100_config_rx_count; i++)
        {
            // LOGD(tag, "(rx) %d, %s\n", i, ax100_rx_config[i].name);
            if(strcmp(param_name, ax100_rx_config[i].name) == 0)
            {
                *param = &(ax100_rx_config[i]);
                LOGD(tag, "%d, %d, %s\n", i, table, ax100_rx_config[i].name);
                return;
            }
        }
    }

    // Find the given parameter name in the AX100 TX table
    if(table == AX100_PARAM_TX(0))
    {
        for(i = 0; i < ax100_config_tx_count; i++)
        {
            // LOGD(tag, "(tx) %d, %s\n", i, ax100_tx_config[i].name);
            if(strcmp(param_name, ax100_tx_config[i].name) == 0)
            {
                *param = &(ax100_tx_config[i]);
                LOGD(tag, "%d, %d, %s\n", i, table, ax100_rx_config[i].name);
                return;
            }
        }
    }

    *param = NULL;
    return;
}

int com_set_beacon(char *fmt, char *params, int nparams)
{
    int period;
    int offset;
    if(params == NULL || sscanf(params, fmt, &period, &offset) != nparams)
    {
        LOGE(tag, "Error parsing params!");
        return CMD_SYNTAX_ERROR;
    }
    dat_set_system_var(dat_com_bcn_period, period);

    char bcn_interval_configuration[32];
    memset(bcn_interval_configuration, 0, 32);
    snprintf(bcn_interval_configuration, 32, "0 bcn_interval %d", period);

    char bcn_offset_configuration[32];
    memset(bcn_offset_configuration, 0,32);
    snprintf(bcn_offset_configuration, 32, "0 bcn_holdoff %d", offset);

    int rc_interval = com_set_config("%d %s %s", bcn_interval_configuration, 3);
    int rc_offset = com_set_config("%d %s %s", bcn_offset_configuration, 3);

    if(rc_interval == CMD_OK && rc_offset == CMD_OK)
    {
        LOGR(tag, "Set beacon period: %d, offset %d", period, offset);
        return CMD_OK;
    }
    else
        return CMD_ERROR;
}

int com_set_downlink(char *fmt, char *params, int nparams)
{
    int baud;
    char trx_node_tmp = trx_node;
    char config[32];

    if(params == NULL || sscanf(params, fmt, &baud) != nparams)
        baud = 4800;

    if(!(baud == 4800 || baud == 9600 || baud == 19200))
    {
        LOGE(tag, "Invalid baud rate %d selected! Please use 4800, 9600 or 19200.", baud);
        return CMD_SYNTAX_ERROR;
    }

    //CONFIGURE NODE TNC (29)
    trx_node = 9;
    memset(config, 0, 32);
    snprintf(config, 32, "1 baud %d", baud);  // TABLE 1 -> RX
    int tnc_baud = com_set_config("%d %s %s", config, 3);
    LOGI(tag, "Setting TNC with: %s (%d)", config, tnc_baud);
    if(tnc_baud != CMD_OK) {
        trx_node = trx_node_tmp;
        return CMD_ERROR;
    }

    //CONFIGURE NODE TRX (5)
    trx_node = 5;
    memset(config, 0, 32);
    snprintf(config, 32, "5 baud %d", baud); // TABLE 5 -> TX
    int trx_baud = com_set_config("%d %s %s", config, 3);
    LOGI(tag, "Setting TRX with: %s (%d)", config, trx_baud);
    if(trx_baud != CMD_OK) {
        trx_node = trx_node_tmp;
        return CMD_ERROR;
    }

    trx_node = trx_node_tmp;
    return CMD_OK;
}

int com_set_uplink(char *fmt, char *params, int nparams)
{
    int baud;
    char trx_node_tmp = trx_node;
    char config[32];

    if(params == NULL || sscanf(params, fmt, &baud) != nparams)
        baud = 4800;

    if(!(baud == 4800 || baud == 9600 || baud == 19200))
    {
        LOGE(tag, "Invalid baud rate %d selected! Please use 4800, 9600 or 19200.", baud);
        return CMD_SYNTAX_ERROR;
    }

    //CONFIGURE NODE TRX (5)
    trx_node = 5;
    memset(config, 0, 32);
    snprintf(config, 32, "1 baud %d", baud); // TABLE 1 -> RX
    int trx_baud = com_set_config("%d %s %s", config, 3);
    LOGI(tag, "Setting TRX with: %s (%d)", config, trx_baud);
    if(trx_baud != CMD_OK) {
        trx_node = trx_node_tmp;
        return CMD_ERROR;
    }

    //CONFIGURE NODE TNC (29)
    trx_node = 9;
    memset(config, 0, 32);
    snprintf(config, 32, "5 baud %d", baud);  // TABLE 5 -> TX
    int tnc_baud = com_set_config("%d %s %s", config, 3);
    LOGI(tag, "Setting TNC with: %s (%d)", config, tnc_baud);
    if(tnc_baud != CMD_OK) {
        trx_node = trx_node_tmp;
        return CMD_ERROR;
    }

    trx_node = trx_node_tmp;
    return CMD_OK;
}

int com_set_satellite(char *fmt, char *params, int nparams) {
    if (params == NULL)
        return CMD_SYNTAX_ERROR;

    int freq = dat_get_system_var(dat_com_freq);
    char prompt[12];
    snprintf(prompt, 12, "%dkHz", freq/1000);

    if (strncmp("2", params, 1) == 0 || strncmp("SCH2", params, 4) == 0 || strncmp("SUCHAI2", params, 7) == 0) {
        freq = sat_freqs[0];
        strncpy(prompt, "SUCHAI2", 12);
    }
    else if(strncmp("3", params, 1) == 0 || strncmp("SCH3", params, 4) == 0 || strncmp("SUCHAI3", params, 7) == 0) {
        freq = sat_freqs[1];
        strncpy(prompt, "SUCHAI3", 12);
    }
    else if(strncmp("P", params, 1) == 0 || strncmp("PS", params, 2) == 0 || strncmp("PLANTSAT", params, 8) == 0) {
        freq = sat_freqs[2];
        strncpy(prompt, "PLANTSAT", 12);
    }

    char trx_node_tmp = trx_node;
    trx_node = 29;  // TNC node

    char config[32];
    memset(config, 0, 32);
    snprintf(config, 32, "1 freq %d", freq); // TABLE 1 -> RX
    int ok = com_set_config("%d %s %s", config, 3);
    LOGI(tag, "Setting TRX with: %s (%d)", config, ok);
    if(ok != CMD_OK) {
        LOGE(tag, "Error setting RX Freq %d", freq);
        trx_node = trx_node_tmp;
        return CMD_ERROR;
    }

    memset(config, 0, 32);
    snprintf(config, 32, "5 freq %d", freq); // TABLE 5 -> TX
    ok = com_set_config("%d %s %s", config, 3);
    LOGI(tag, "Setting TRX with: %s (%d)", config, ok);
    if(ok != CMD_OK) {
        LOGE(tag, "Error setting TX Freq %d", freq);
        trx_node = trx_node_tmp;
        return CMD_ERROR;
    }

    trx_node = trx_node_tmp;
    console_set_prompt(prompt);

    return CMD_OK;
}
