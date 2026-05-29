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

#include "app/system/taskSensors.h"

static const char *tag = "Sensors";

void taskSensors(void *param)
{
    LOGI(tag, "Started");
    portTick xLastWakeTime = osTaskGetTickCount();

    int i;
    cmd_t *cmd_init;
    cmd_t *cmd_get;
    int nsensors = 5;
    char *init_cmds[] = {"sen_init_dummy", "sen_init_dummy", "sen_init_dummy", "sen_init_dummy", "sen_init_dummy"};
    char *get_cmds[] = {"sen_get_temp", "sen_get_adcs", "sen_get_eps", "sen_get_status", "sen_get_adcs_fss"};


    int action = dat_get_system_var(dat_drp_mach_action);
    int state = dat_get_system_var(dat_drp_mach_state);
    int step = dat_get_system_var(dat_drp_mach_step);
    int active_payloads = dat_get_system_var(dat_drp_mach_payloads);
    int samples_left = dat_get_system_var(dat_drp_mach_left);

    if( action < 0 || state < 0  || active_payloads < 0 || step < 0)
    {
        status_machine = (dat_stmachine_t) {ST_PAUSE, ACT_START, 0, 5, -1, nsensors};
    } else
    {
        status_machine = (dat_stmachine_t) {state, action, active_payloads, step, samples_left, nsensors};
    }

    if(osSemaphoreCreate(&repo_machine_sem) != OS_SEMAPHORE_OK)
    {
        LOGE(tag, "Unable to create system status repository mutex");
    }

    // Run some initialization commands if neccessary
    for(i=0; i < status_machine.total_sensors; i++)
    {
        cmd_init = cmd_build_from_str(init_cmds[i]);
        cmd_send(cmd_init);
    }

    int elapsed_sec = 0;

    while(1)
    {
        osTaskDelayUntil(&xLastWakeTime, 1000); //Suspend task
        status_machine.active_payloads = dat_get_system_var(dat_drp_mach_payloads);
        LOGV(tag, "state: %d, action %d, samples left: %d", status_machine.state, status_machine.action, status_machine.samples_left)

        // Apply action
        if (status_machine.action != ACT_STAND_BY) {
            osSemaphoreTake(&repo_machine_sem, portMAX_DELAY);
            if (status_machine.action == ACT_START) {

                status_machine.state = ST_SAMPLING;
                status_machine.action = ACT_STAND_BY;
            } else if (status_machine.action == ACT_PAUSE) {
                status_machine.state = ST_PAUSE;
                status_machine.action = ACT_STAND_BY;
            }
            osSemaphoreGiven(&repo_machine_sem);
        }

        // States
        if (status_machine.state == ST_SAMPLING) {
            // Check for samples left
            if (status_machine.samples_left == 0) {
                osSemaphoreTake(&repo_machine_sem, portMAX_DELAY);
                status_machine.state = ST_PAUSE;
                status_machine.action = ACT_STAND_BY;
                osSemaphoreGiven(&repo_machine_sem);
            }
            // Check for step
            else if (elapsed_sec % status_machine.step == 0) {
                LOGV(tag, "SAMPLING...");

                for(i=0; i<nsensors; i++)
                {
                    if (dat_stmachine_is_sensor_active(i, status_machine.active_payloads, status_machine.total_sensors))
                    {
                        cmd_get = cmd_build_from_str(get_cmds[i]);
                        cmd_send(cmd_get);
                    }
                }
                if (status_machine.samples_left != -1)
                {
                    osSemaphoreTake(&repo_machine_sem, portMAX_DELAY);
                    status_machine.samples_left -= 1;
                    osSemaphoreGiven(&repo_machine_sem);
                }
            }
        }

        action = (int) status_machine.action;
        state = (int) status_machine.state;
        step = (int) status_machine.step;
        active_payloads = (int) status_machine.active_payloads;
        samples_left = (int) status_machine.samples_left;

        dat_set_system_var(dat_drp_mach_action, action);
        dat_set_system_var(dat_drp_mach_state, state);
        dat_set_system_var(dat_drp_mach_step, step);
        dat_set_system_var(dat_drp_mach_payloads, active_payloads);
        dat_set_system_var(dat_drp_mach_left, samples_left);
        elapsed_sec += 1;
    }
}


