/*                                 SUCHAI
 *                      NANOSATELLITE FLIGHT SOFTWARE
 *
 *      Copyright 2021, Carlos Gonzalez Cortes, carlgonz@uchile.cl
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

#include "suchai/mainFS.h"
#include "suchai/taskInit.h"
#include "suchai/osThread.h"
#include "suchai/log_utils.h"
#include "app/system/taskHousekeeping.h"
#include "app/system/cmdAPP.h"
#ifdef RPI
#include "csp_if_i2c_uart.h"
#endif

static char *tag = "app_main";

/**
 * App specific initialization routines
 * This function is called by taskInit
 *
 * @param params taskInit params
 */
void initAppHook(void *params)
{
    /** Include app commands */
    cmd_app_init();

#ifdef LINUX
    csp_add_zmq_iface(SCH_COMM_NODE);
#endif
#ifdef RPI
    csp_i2c_uart_init(SCH_COMM_NODE, 0, 19200);
    csp_rtable_set(8, 2, &csp_if_i2c_uart, 5); // Traffic to GND (8-15) via I2C to TRX node
    csp_route_set(CSP_DEFAULT_ROUTE, &csp_if_i2c_uart, CSP_NODE_MAC); // Rest of the traffic to I2C using node i2c address
#endif

    /** Init app tasks */
    int t_ok = osCreateTask(taskHousekeeping, "housekeeping", 1024, NULL, 2, NULL);
    if(t_ok != 0) LOGE("simple-app", "Task housekeeping not created!");
}

int main(void)
{
    /** Call framework main, shouldn't return */
    suchai_main();
}
