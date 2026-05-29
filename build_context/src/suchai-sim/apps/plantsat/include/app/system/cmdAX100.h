/**
 * @file  cmdCOM.h
 * @author Carlos Gonzalez C - carlgonz@uchile.cl
 * @date 2020
 * @copyright GNU Public License.
 *
 * This header contains commands related with the communication system
 */

#ifndef CMD_AX100_H
#define CMD_AX100_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "suchai/config.h"

#include "app/drivers/drivers.h"
#include "suchai/repoCommand.h"

/**
 * Registers communications commands in the system
 */
void cmd_ax100_init(void);

/**
 * Set module global variable trx_node. Future command calls will use this node
 *
 * @param fmt Str. Parameters format: "%d"
 * @param params Str. Parameters: <node>, the TRX node number
 * @param nparams Str. Number of parameters: 1
 * @return CMD_OK if executed correctly, CMD_ERROR in case of failures, or CMD_ERROR_SYNTAX in case of parameters errors.
 */
int com_set_node(char *fmt, char *params, int nparams);

/**
 * Send (and set) current time to node
 * @param fmt Str. Parameters format: "%d"
 * @param params  Str. Parameters: <node>
 * @param nparams Str. Number of parameters: 1
 * @return CMD_OK if executed correctly, CMD_ERROR in case of failures, or CMD_ERROR_SYNTAX in case of parameters errors.
 */
int com_get_node(char *fmt, char *params, int nparams);

/**
 * Reset the TRX GND Watchdog timer at @node node by sending a CSP command to the
 * AX100_PORT_GNDWDT_RESET (9) port. This command targets the AX100 TRX.
 * If the <node> param is given, then the message is send to that node, if no
 * parameters given then the message is sent to SCH_TRX_ADDRESS node.
 *
 * @param fmt Str. Parameters format: "%d"
 * @param params Str. Parameters: [node], the TRX node number
 * @param nparams Str. Number of parameters: 0|1
 * @return CMD_OK if executed correctly, CMD_ERROR in case of failures, or CMD_ERROR_SYNTAX in case of parameters errors.
 *
 * @code
 *      // Function usage
 *      com_reset_gnd_wdt("%d", "5", 1);  // Reset gnd wdt of node 5
 *      com_reset_gnd_wdt("", "", 0);     // Reset gnd wdt of default TRX node
 *
 *      // Send reset gnd wdt to node 5
 *      cmd_t *send_cmd = cmd_get_str("com_reset_wdt"); // Get the command
 *      cmd_add_params_var(send_cmd, 5)  // Set param node to 5
 *      cmd_send(send_cmd);
 *
 *      // Send reset gnd_wdt to default SCH_TRX_ADDRESS node
 *      cmd_t *send_cmd = cmd_get_str("com_reset_wdt"); // Get the command
 *      cmd_send(send_cmd);  // Send command without parameters;
 *
 * @endcode
 */
int com_reset_wdt(char *fmt, char *params, int nparams);

/**
 * Print TRX housekeeping information
 * @warning not implemented yet
 *
 * @param fmt
 * @param params
 * @param nparams
 * @return
 */
int com_get_hk(char *fmt, char *params, int nparams);

/**
 * Get TRX settings values. The TRX has a list of parameters to set and
 * get (@see ax100_param.h and ax100_param_radio.h). Use this command to get
 * any parameter value by name. The special argument 'help' can be
 * used to print the list of available parameters.
 *
 * TABLES:  0 Running parameters
 *          1 RX parameters
 *          5 TX parameters
 *
 * @param fmt Str. Parameters format: "%d %s"
 * @param params Str. Parameters: <table> <param_name>, the parameter name
 * @param nparams Str. Number of parameters: 2
 * @return CMD_OK if executed correctly, CMD_ERROR in case of failures, or CMD_ERROR_SYNTAX in case of parameters errors.
 *
 * @code
 *      // Function usage
 *      com_get_config("%d %s", "0 help", 1);     // Print the parameter list
 *      com_get_config("%d %s", "0 csp_node", 1); // Read and print the TRX node
 *
 *      // Command usage to get a TRX parameter
 *      cmd_t *send_cmd = cmd_get_str("com_get_config"); // Get the command
 *      cmd_add_params(send_cmd, "0 tx_pwr")  // Read param "tx_pwr"
 *      cmd_send(send_cmd);
 * @endcode
 *
 */
int com_get_config(char *fmt, char *params, int nparams);

/**
 * Set TRX settings values. The TRX has a list of parameters to set and
 * get (@see ax100_param.h and ax100_param_radio.h). Use this command to set
 * any parameter value by name. The special argument 'help 0' can be
 * used to print the list of available parameters.
 *
 * TABLES:  0 Running parameters
 *          1 RX parameters
 *          5 TX parameters
 *
 * @param fmt Str. Parameters format: "%d %s %s"
 * @param params Str. Parameters: <table> <param_name> <param_value>, the parameter name
 * and value as strings.
 * @param nparams Str. Number of parameters: 3
 * @return CMD_OK if executed correctly, CMD_ERROR in case of failures, or CMD_ERROR_SYNTAX in case of parameters errors.
 *
 * @code
 *      // Function usage
 *      com_set_config("%d %s %s", "0 help 0", 3);     // Print the parameter list
 *      com_set_config("%d %s %s", "0 csp_node 5", 1); // Set and print the TRX node
 *
 *      // Command usage to set a TRX parameter
 *      cmd_t *send_cmd = cmd_get_str("com_set_config"); // Get the command
 *      cmd_add_params(send_cmd, "0 tx_pwr 0")  // Set param "tx_pwr" to 0
 *      cmd_send(send_cmd);
 * @endcode
 *
 */
int com_set_config(char *fmt, char *params, int nparams);

/* TODO: Add documentation */
int com_update_status_vars(char *fmt, char *params, int nparams);

/* TODO: ADD documentation */
int com_set_beacon(char *fmt, char *params, int nparams);

#endif /* CMD_AX100_H */
