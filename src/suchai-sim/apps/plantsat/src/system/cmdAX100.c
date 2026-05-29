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
#ifndef SCH_TRX_ADDRESS
#define SCH_TRX_ADDRESS 5
#endif

static const char *tag = "cmdAX100";
static char trx_node = SCH_TRX_ADDRESS;

static void _com_config_help(void);
static void _com_config_find(char *param_name, int table, const param_table_t **param);

void cmd_ax100_init(void)
{
    cmd_add("com_set_node", com_set_node, "%d", 1);
    cmd_add("com_get_node", com_get_node, "", 0);
    cmd_add("com_reset_wdt", com_reset_wdt, "%d", 1);
    cmd_add("com_get_config", com_get_config, "%d %s", 2);
    cmd_add("com_set_config", com_set_config, "%d %s %s", 3);
    cmd_add("com_set_beacon", com_set_beacon, "%d %d", 2);
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
#ifdef NANOMIND
        rc = rparam_get_single(out, param_i->addr, param_i->type, param_i->size,
                table, trx_node, AX100_PORT_RPARAM, 1000);
#endif

        // Process the answer
        if(rc > 0)
        {
            char param_str[SCH_CMD_MAX_STR_PARAMS];
#ifdef NANOMIND
            param_to_string(param_i, param_str, 0, out, 1, SCH_CMD_MAX_STR_PARAMS) ;
#endif
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
#ifdef NANOMIND
        param_from_string(param_i, value, out);
        rc = rparam_set_single(out, param_i->addr, param_i->type, param_i->size,
                               table, trx_node, AX100_PORT_RPARAM, 1000);
#endif

        // Process the answer
        if(rc > 0)
        {
            char param_str[SCH_CMD_MAX_STR_PARAMS];
#ifdef NANOMIND
            param_to_string(param_i, param_str, 0, out, 1, SCH_CMD_MAX_STR_PARAMS);
#endif
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
