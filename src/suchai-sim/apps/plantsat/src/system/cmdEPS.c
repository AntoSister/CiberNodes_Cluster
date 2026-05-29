/**
 *                                 SUCHAI
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

#include "app/system/cmdEPS.h"

#ifndef SCH_EPS_ADDRESS
#define SCH_EPS_ADDRESS 2
#endif

static const char *tag = "cmdEPS";

void cmd_eps_init(void)
{
    //EPS io driver requires some initialization
#ifdef NANOMIND
    eps_set_node(SCH_EPS_ADDRESS);
    eps_set_timeout(1000);
#endif

    // Register commands
    cmd_add("eps_set_node", eps_node_set, "%d", 1);
    cmd_add("eps_hard_reset", eps_hard_reset, "", 0);
    cmd_add("eps_get_hk", eps_get_hk, "", 0);
    cmd_add("eps_get_config", eps_get_config, "", 0);
    cmd_add("eps_set_heater", eps_set_heater, "%d %d", 2);
    cmd_add("eps_set_output", eps_set_output, "%d %d", 2);
    cmd_add("eps_set_output_all", eps_set_output_all, "%d", 1);
    cmd_add("eps_set_vboost", eps_set_vboost, "%d", 1);
    cmd_add("eps_set_mppt", eps_set_pptmode, "%d", 1);
    cmd_add("eps_reset_wdt", eps_reset_wdt, "", 0);
    cmd_add("eps_update_status", eps_update_status_vars, "", 0);
}

int eps_node_set(char *fmt, char *params, int nparams)
{
    int node;
    if (params == NULL || sscanf(params, fmt, &node) != nparams) {
        LOGE(tag, "Error parsing parameters!");
        return CMD_SYNTAX_ERROR;
    }
#ifdef NANOMIND
    eps_set_node(node);
#endif
    return CMD_OK;
}

int eps_hard_reset(char *fmt, char *params, int nparams)
{
#if defined(NANOMIND)
    if(eps_hardreset() > 0)
    {
        return CMD_OK;
    }
#elif defined(SIM)
    if (sim_eps_hard_reset() == 0)
    {
        obc_reset(NULL, NULL, 0);  // Never returns
    }
#endif
    LOGE(tag, "Unable to reset the EPS!");
    return CMD_ERROR;
}

int eps_get_hk(char *fmt, char *params, int nparams)
{
    int rc = 0;
#if defined(NANOMIND)
    eps_hk_t hk = {0};
    rc = eps_hk_get(&hk);
    if(rc == 0) return CMD_ERROR;

    eps_hk_print(&hk);
#elif defined(SIM)
    eps_hk_read_t eps_hk_reading;
    rc = sim_eps_get_hk(&eps_hk_reading);
    if(rc != 0)
    {
        LOGE(tag, "Error reading EPS HK data");
        return CMD_ERROR;
    }
    printf("Voltage: %d mV; Input: %d mA; Output: %d mA; Temp: %d ºC\n",
           eps_hk_reading.vbat, eps_hk_reading.current_in, eps_hk_reading.current_out, eps_hk_reading.temp);
#endif

    return CMD_OK;
}

int eps_get_config(char *fmt, char *params, int nparams)
{
#ifdef NANOMIND
    eps_config_t nanopower_config;

    if(eps_config_get(&nanopower_config) > 0)
    {
        eps_config_print(&nanopower_config);
    }
    else
    {
        return CMD_ERROR;
    }
#endif
    return CMD_OK;
}

int eps_set_heater(char *fmt, char *params, int nparams)
{
    if(params == NULL)
    {
        LOGE(tag, "NULL params!");
        return CMD_ERROR;
    }

    int heater, on_off;
    uint8_t state[2];

    if(sscanf(params, fmt, &heater, &on_off) == nparams)
    {
        LOGI(tag, "Setting heater %d to state %d", heater, on_off);
#if defined(NANOMIND)
        eps_heater((uint8_t) heater, (uint8_t) on_off, state);
#elif defined(SIM)
        int rc = sim_eps_set_heater((uint8_t)on_off);
        if (rc == 0)
        {
            state[0] = on_off;
            state[1] = on_off;
        }
#endif
        LOGI(tag, "Heater state is %u %u", (unsigned int)state[0], (unsigned int)state[1]);
        return CMD_OK;
    }
    else
    {
        LOGE(tag, "Invalid params!");
        return CMD_ERROR;
    }
}

int eps_set_output(char *fmt, char *params, int nparams)
{
    unsigned int channel, mode;
    // "<channel> <mode>"
    if(params == NULL || sscanf(params, fmt, &channel, &mode) != nparams)
    {
        LOGE(tag, "Error parsing parameters!");
        return CMD_SYNTAX_ERROR;
    }
    if(channel > 7){
        LOGE(tag, "Error parsing parameters: channel > 7 (%d)!", channel);
        return CMD_SYNTAX_ERROR;
    }
    mode = mode == 0 ? 0 : 1; // Mode is 0 -> OFF or > 0 = 1 -> ON

    int rc = 0;
#if defined(NANOMIND)
    rc = eps_output_set_single((uint8_t)channel, (uint8_t)mode, 0);
#elif defined(SIM)
    rc = sim_eps_set_output((uint8_t)channel, (uint8_t)mode);
#endif
    if(rc > 0)
        return CMD_OK;
    else
        return CMD_ERROR;
}

int eps_set_output_all(char *fmt, char *params, int nparams)
{
    unsigned int mode;
    uint8_t mask;
    // "<on/off>"
    if(params == NULL || sscanf(params, fmt, &mode) != nparams)
    {
        LOGE(tag, "Error parsing parameters!");
        return CMD_SYNTAX_ERROR;
    }

    mode = mode == 0 ? 0 : 1; // Mode is 0 -> OFF or > 0 = 1 -> ON
    mask = mode ? 0xFF : 0x00; // Mode 1 -> All outputs on, else all outputs off

    int rc = 0;
#if defined(NANOMIND)
    eps_output_set(mask);
#elif defined(SIM)
    for(uint8_t i=0; i<7; i++)
    {
        rc += sim_eps_set_output(i, (uint8_t)mode);
    }
#endif
    if(rc > 0)
        return CMD_OK;
    else
        return CMD_ERROR;
}

int eps_set_vboost(char *fmt, char *params, int nparams)
{
    int vboost;
    if(params == NULL || sscanf(params, fmt, &vboost) != nparams)
    {
        LOGE(tag, "Error parsing parameters!");
        return CMD_SYNTAX_ERROR;
    }

    int rc = 0;
#ifdef NANOMIND
    rc = eps_vboost_set(vboost, vboost, vboost);
#endif
    return rc > 0 ? CMD_OK : CMD_ERROR;
}

int eps_set_pptmode(char *fmt, char *params, int nparams)
{
    int pptmode;
    if(params == NULL || sscanf(params, fmt, &pptmode) != nparams)
    {
        LOGE(tag, "Error parsing parameters!");
        return CMD_SYNTAX_ERROR;
    }

    int rc = 0;
#ifdef NANOMIND
    rc = eps_pptmode_set((char)pptmode);
#endif
    return rc > 0 ? CMD_OK : CMD_ERROR;
}

int eps_reset_wdt(char *fmt, char *params, int nparams)
{
    int rc = 0;
#ifdef NANOMIND
    rc = eps_wdt_gnd_reset();
#endif
    return rc > 0 ? CMD_OK : CMD_ERROR;
}

int eps_update_status_vars(char *fmt, char *params, int nparams)
{
    int rc = 0;
    uint32_t cursun = 0;
    uint32_t cursys = 0;
    uint32_t vbatt = 0;
    int32_t tbat = 0;

#if defined(NANOMIND)
    eps_hk_t hk = {0};
    rc = eps_hk_get(&hk);
    cursun = hk.cursun;
    cursys = hk.cursys;
    vbatt = hk.vbatt;
    tbat = (hk.temp[4]+hk.temp[5])*10/2;
#elif defined(SIM)
    eps_hk_read_t eps_hk_reading;
    rc = sim_eps_get_hk(&eps_hk_reading);
    cursun = eps_hk_reading.current_in;
    cursys = eps_hk_reading.current_out;
    vbatt = eps_hk_reading.vbat;
    tbat = eps_hk_reading.temp;
#endif
    if(rc > 0)
    {
        dat_set_system_var(dat_eps_vbatt, vbatt);
        dat_set_system_var(dat_eps_cur_sun, cursun);
        dat_set_system_var(dat_eps_cur_sys, cursys);
        dat_set_system_var(dat_eps_temp_bat0, tbat);
    }
    else
    {
        return CMD_ERROR;
    }
    return CMD_OK;
}