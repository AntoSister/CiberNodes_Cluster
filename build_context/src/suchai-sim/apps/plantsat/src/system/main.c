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

#include "suchai/config.h"
#include "app/system/config.h"
#include "suchai/mainFS.h"
#include "suchai/taskInit.h"
#include "suchai/osThread.h"
#include "suchai/log_utils.h"
#include "app/system/cmdADCS.h"
#include "app/system/cmdAX100.h"
#include "app/system/cmdEPS.h"
#include "app/system/cmdGSSB.h"
#include "app/system/cmdRW.h"
#include "app/system/cmdSensors.h"
#include "app/system/cmdCDH.h"
#include "app/system/cmdCAM.h"
#include "app/system/taskHousekeeping.h"
#include "app/system/taskADCS.h"
#include "app/system/taskSensors.h"
#ifdef SIM
#include "drivers-sim/interface.h"
#endif

static char *tag = "app_main";

#define INIT_DEP_FIRST      0
#define INIT_DEP_DEPLOYING  1
#define INIT_DEP_CONFIRMED  2

int init_setup_libcsp_2(void)
{
    /* Init communications */
    LOGI(tag, "Add CSP interfaces and routes...");
    /**
     * Set interfaces and routes
     *  Platform dependent
     */
#ifdef NANOMIND
    /* Init csp i2c interface with address 1 and 400 kHz clock */
    LOGI(tag, "csp_i2c_init...");
    sch_a3200_init_twi0(GS_AVR_I2C_MULTIMASTER, SCH_COMM_NODE, 400000);
    int t_ok = csp_i2c_init(SCH_COMM_NODE, 0, 400000);
    if(t_ok != CSP_ERR_NONE) LOGE(tag, "\tcsp_i2c_init failed!");

    /**
     * Setting route table
     * Build with options: --enable-if-i2c --with-rtable cidr
     *  csp_rtable_load("8/2 I2C 5");
     *  csp_rtable_load("0/0 I2C");
     */
    csp_rtable_set(8, 2, &csp_if_i2c, SCH_TRX_ADDRESS); // Traffic to GND (8-15) via I2C node TRX
    csp_route_set(CSP_DEFAULT_ROUTE, &csp_if_i2c, CSP_NODE_MAC); // All traffic to I2C using node as i2c address
#else
    csp_add_zmq_iface(SCH_COMM_NODE);
#endif

    LOGI(tag, "Route table");
    csp_route_print_table();
    LOGI(tag, "Interfaces");
    csp_route_print_interfaces();

    return 0;
}

int init_setup_trx(void) {
    LOGD(tag, "\t Init TRX...");

    typedef struct trx_config_s {
        int table;
        char *param;
        int value;
    }trx_config_t;

    // Define the list of commands and parameters
    const int N_CONFIGS = 8;
    trx_config_t trx_configs[] = {
            {0, "tx_inhibit", SCH_TX_INHIBIT},
            {0, "bcn_holdoff", dat_get_status_var(dat_com_bcn_period).i},
            {0, "bcn_interval", dat_get_status_var(dat_com_bcn_period).i},
            {0, "tx_pwr", dat_get_status_var(dat_com_tx_pwr).i},
            {1, "freq", dat_get_status_var(dat_com_freq).i},
            {5, "freq", dat_get_status_var(dat_com_freq).i},
            {1, "baud", dat_get_status_var(dat_com_baud).i},
            {5, "baud", dat_get_status_var(dat_com_baud).i},
        };

    cmd_t *trx_cmd;
    char *trx_str = calloc(SCH_CMD_MAX_STR_PARAMS, 1);

    // Send all commands to config the TRX
    for(int i=0; i<N_CONFIGS; i++)
    {
        trx_config_t trx_config = trx_configs[i];
        memset(trx_str, 0, SCH_CMD_MAX_STR_PARAMS);
        sprintf(trx_str, "com_set_config %d %s %d", trx_config.table, trx_config.param, trx_config.value);
        LOGD(tag, "TRX CMD: %s", trx_str);
        trx_cmd = cmd_build_from_str(trx_str);
        cmd_send(trx_cmd);
        if(log_lvl >= LOG_LVL_DEBUG)
        {
            memset(trx_str, 0, SCH_CMD_MAX_STR_PARAMS);
            sprintf(trx_str, "com_get_config %d %s", trx_config.table, trx_config.param);
            trx_cmd = cmd_build_from_str(trx_str);
            cmd_send(trx_cmd);
        }
    }

    free(trx_str);

}

int init_antenna_deploy(void)
{
    LOGD(tag, "\tAntenna deployment...")
    // Define commands
    const int N_CMDS = 6;
    char *cmds_str[] = {
            "gssb_pwr 1 1",                  // Turn-on istages
            "gssb_update_status",            // Update deployment status variables
            "gssb_antenna_release 16 2000 1 5", // istage 1. On: 2s, off: 1s, rep: 5
            "gssb_antenna_release 17 2000 1 5", // istage 2. On: 2s, off: 1s, rep: 5
            "gssb_antenna_release 18 2000 1 5", // istage 3. On: 2s, off: 1s, rep: 5
            "gssb_antenna_release 19 2000 1 5", // istage 4. On: 2s, off: 1s, rep: 5
    };

    for(int i=0; i<N_CMDS; i++) {
        //Turn on gssb and update antenna deployment status
        cmd_t *cmd_dep;
        cmd_dep = cmd_build_from_str(cmds_str[i]);
        cmd_send(cmd_dep);
        osDelay(2000);
    }

    return 0;
}

int init_deployment_routine(void)
{
    LOGI(tag, "DEPLOYMENT...");
    dat_set_system_var(dat_obc_opmode, DAT_OBC_OPMODE_DEPLOYING);
    int deployed = dat_get_system_var(dat_dep_deployed);
    LOGI(tag, "dat_dep_deployed: %d...", deployed);
    if(deployed == INIT_DEP_FIRST) // First deploy
    {
        LOGI(tag, "FIRST DEPLOY");
        /* First deploy - 30min TRX Silence */
        LOGI(tag, "Setting TRX Inhibit to: %d seconds...", 1860);
        cmd_t *tx_silence = cmd_build_from_str("com_set_config 0 tx_inhibit 1860");
        cmd_send(tx_silence);

        /* Wait 30 minutes before antenna deployment */
        int seconds = 0;
        portTick xLastWakeTime = osTaskGetTickCount();
        while(seconds < 1800)
        {
            LOGI(tag, "Deployment delay: %d/%d seconds...", seconds, 1800);
            osTaskDelayUntil(&xLastWakeTime, 1000); //Suspend task
            seconds ++;
            if(dat_get_system_var(dat_obc_opmode) != DAT_OBC_OPMODE_DEPLOYING)
                goto cancel;
        }
        dat_set_system_var(dat_dep_deployed, INIT_DEP_DEPLOYING);
    }

    deployed = dat_get_system_var(dat_dep_deployed);
    LOGI(tag, "dat_dep_deployed: %d...", deployed);
    if(deployed == INIT_DEP_DEPLOYING) // Deployed not confirmed, but silence time
    {
        LOGI(tag, "ANTENNA DEPLOYMENT");
        eps_update_status_vars(NULL, NULL, 0);
        int vbat_mV = dat_get_system_var(dat_eps_vbatt);

        // Deploy antenna
        LOGI(tag, "Deploying antennas: %d (Battery Voltage: %.04f)", vbat_mV>7000, vbat_mV/1000.0);
        if(vbat_mV > 7000)
            init_antenna_deploy();

        //Update antenna deployment status
#ifdef NANOMIND
        gssb_update_status(NULL, NULL, 0);
#endif
    }

    if(deployed == INIT_DEP_CONFIRMED)
    {
        LOGI(tag, "Deployment confirmed!");
    }

cancel:
    dat_set_system_var(dat_obc_opmode, DAT_OBC_OPMODE_NORMAL);
    LOGI(tag, "Restore TRX Inhibit to: %d seconds", 0);
    cmd_t *tx_silence = cmd_build_from_str("com_set_config 0 tx_inhibit 0");
    cmd_send(tx_silence);
}

/**
 * App specific initialization routines
 * This function is called by taskInit
 *
 * @param params taskInit params
 */
void initAppHook(void *params)
{
    int rc;

    /** Set loging system */
#ifdef SCH_LOG_ENABLE_MONGODB
    log_set(SCH_LOG_LEVEL, LOG_MODE_MONGO, 0, 0);
#endif

    /** Include app commands */
    cmd_adcs_init();
    cmd_eps_init();
    cmd_sensors_init();
    cmd_cdh_init();
    cmd_cam_init();
#ifdef NANOMIND
    cmd_rw_init();
    cmd_ax100_init();
    cmd_gssb_init();
#endif

    /** Finish CSP setup */
    init_setup_libcsp_2();

    /** Finish drivers setup*/
#ifdef SIM
    iface_open();
#endif


#if !SCH_EPS_OUT_ENABLED
    LOGI(tag, "POWERING OFF ALL EPS OUTPUT")
    cmd_t *cmd_eps_set_output_all;
    cmd_eps_set_output_all = cmd_build_from_str("eps_set_output_all 0");
    cmd_send(cmd_eps_set_output_all);
#endif

    /** Init TRX */
    LOGI(tag, "SETUP TRX...");
    rc = init_setup_trx();

    /** Init app tasks */
    int t_ok;
#if SCH_HK_ENABLED
    t_ok = osCreateTask(taskHousekeeping, "housekeeping", 2*SCH_TASK_DEF_STACK, NULL, 2, NULL);
    if(t_ok != 0) LOGE(tag, "Task housekeeping not created!");
#endif
#if SCH_SEN_ENABLED
    t_ok = osCreateTask(taskSensors,"sensors", 2*SCH_TASK_DEF_STACK, NULL, 2, NULL);
    if(t_ok != 0) LOGE(tag, "Task sensors not created!");
#endif
#if SCH_ADCS_ENABLED
    t_ok = osCreateTask(taskADCS,"adcs", 3*SCH_TASK_DEF_STACK, NULL, 2, NULL);
    if(t_ok != 0) LOGE(tag, "Task ADCS not created!");
#endif

    /** DEPLOYMENT */
    init_deployment_routine();
}

int main(void)
{
    /** Call framework main, shouldn't return */
    suchai_main();
}
