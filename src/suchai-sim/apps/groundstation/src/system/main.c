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
#include "app/system/cmdAPP.h"
#include "app/system/cmdAX100.h"
#include "app/system/cmdEPS.h"
#include "app/system/cmdCDH.h"

#if SCH_GND_ADD_PAYLOADS
#include "app/system/cmdMAG.h"
#endif

static char *tag = "app_main";

static csp_iface_t *csp_if_zmqhub;
static csp_iface_t csp_if_kiss;

static csp_kiss_handle_t csp_kiss_driver;
void my_usart_rx(uint8_t * buf, int len, void * pxTaskWoken) {
    csp_kiss_rx(&csp_if_kiss, buf, len, pxTaskWoken);
}


/**
 * App specific initialization routines
 * This function is called by taskInit
 *
 * @param params taskInit params
 */
void initAppHook(void *params)
{
    /** Set loging system */
#ifdef SCH_LOG_ENABLE_MONGODB
    log_set(SCH_LOG_LEVEL, LOG_MODE_MONGO, 0, 0);
#endif

    /** Include app commands */
    cmd_app_init();
    cmd_cdh_init();

#if SCH_GND_ADD_GS
    cmd_ax100_init();
    cmd_eps_init();
#endif

#if SCH_GND_ADD_PAYLOADS
    cmd_mag_init();
#endif

    /** Init CSP **/
    /* KISS INTERFACE */
    struct usart_conf conf;
    conf.device = SCH_KISS_DEVICE;
    conf.baudrate = SCH_KISS_UART_BAUDRATE;
    // usart_init(&conf);
    // csp_kiss_init(&csp_if_kiss, &csp_kiss_driver, usart_putc, usart_insert, "KISS");
    // usart_set_callback(my_usart_rx); // Setup callback from USART RX to KISS RS
    // csp_route_set(SCH_TNC_ADDRESS, &csp_if_kiss, CSP_NODE_MAC);
    // csp_rtable_set(0, 2, &csp_if_kiss, SCH_TNC_ADDRESS); // Traffic to GND (0-7) via KISS node TNC

    /* ANOTHER KISS INTERFACE ? */
//    struct usart_conf conf;
//    conf.device = "/dev/ttyUSB2";
//    conf.baudrate = SCH_KISS_UART_BAUDRATE;
//    usart_init(&conf);
//    csp_kiss_init(&csp_if_kiss, &csp_kiss_driver, usart_putc, usart_insert, "KISS");
//    usart_set_callback(my_usart_rx); // Setup callback from USART RX to KISS RS
//    csp_route_set(SCH_TNC_ADDRESS, &csp_if_kiss, CSP_NODE_MAC);
//    csp_rtable_set(0, 2, &csp_if_kiss, SCH_TNC_ADDRESS); // Traffic to GND (0-7) via KISS node TNC

    /* ZMQ INTERFACE */
    /* Set ZMQ interface as a default route*/
    uint8_t addr = (uint8_t)SCH_COMM_NODE;
    uint8_t *rxfilter = &addr;
    unsigned  int rxfilter_count = 1;
    csp_zmqhub_init_w_name_endpoints_rxfilter(CSP_ZMQHUB_IF_NAME, rxfilter, rxfilter_count,
                                              SCH_COMM_ZMQ_OUT, SCH_COMM_ZMQ_IN, &csp_if_zmqhub);
    csp_route_set(CSP_DEFAULT_ROUTE, csp_if_zmqhub, CSP_NODE_MAC);
    // Add route to TNC (default node is 29)
    // csp_route_set(29, &csp_if_kiss, 255);

    /** Init app tasks */
}

int main(void)
{
    /** Call framework main, shouldn't return */
    suchai_main();


}
