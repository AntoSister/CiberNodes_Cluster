/*                                 SUCHAI
 *                      NANOSATELLITE FLIGHT SOFTWARE
 *
 *      Copyright 2021, Carlos Gonzalez Cortes, carlgonz@uchile.cl
 *      Copyright 2021, Camilo Rojas Milla, camrojas@uchile.cl
 *      Copyright 2021, Gustavo Diaz Huenupan, gustavo.diaz@ing.uchile.cl
 *      Copyright 2022, Elias Obreque Sepulveda, elias.obreque@uchile.cl
 *      Copyright 2021, Javier Morales Rodriguez, javiermoralesr95@gmail.com
 *      Copyright 2021, Luis Jimenez Verdugo, luis.jimenez@ing.uchile.cl
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

#include "app/system/cmdADCS.h"

static const char* tag = "cmdADCS";

#define ADCS_PORT 7
#define TLE_BUFF_LEN 70

TLE tle;
static char tle1[TLE_BUFF_LEN]; //"1 42788U 17036Z   20054.20928660  .00001463  00000-0  64143-4 0  9996";
static char tle2[TLE_BUFF_LEN]; //"2 42788  97.3188 111.6825 0013081  74.6084 285.6598 15.23469130148339";

// Parametric calibration parameters [h x0 y0 rho]
double cal1[4] = {0.52544,-0.033188,0.022875,-0.0038183}; // uid : 1523358639 - B23 - 0x20
double cal2[4] = {0.53262,0.0053791,-0.015919,-0.0055018}; // uid : 1523289023 - A28 - 0x21
double cal3[4] = {0.52578,-0.020189,-0.0097265,-0.0098635}; // uid : 1523358910 - B24 - 0x22
double cal4[4] = {0.51447,-0.028293,0.0060877,0.0028711}; // uid : 1523359548 - A29 - 0x23
double cal5[4] = {0.52768,0.031435,-0.023118,-0.0060699}; // uid : 1523357329 - C17 - 0x24

double T1[4][4];
double T2[4][4];
double T3[4][4];
double T4[4][4];
double T5[4][4];

// position and orientation of FSS (0x20: +X, 0x21: -Y, 0x22: -X, 0x23: +Y, 0x24: -Z)

quaternion_t q_c2b_fss1 = {1, 0, 0, 0};
quaternion_t q_c2b_fss2 = {0.707106781, -0.707106781, 0, 0};
quaternion_t q_c2b_fss3 = {0, 1 ,0 , 0};
quaternion_t q_c2b_fss4 = {0.707106781, 0.707106781, 0, 0};
quaternion_t q_c2b_fss5 = {0.5, -0.5, -0.5, 0.5};

void cmd_adcs_init(void)
{
    cmd_add("tle_get", tle_get, "", 0);
    cmd_add("tle_set", tle_set, "%d %n", 2);
    cmd_add("tle_update", tle_update, "", 0);
    cmd_add("tle_prop", tle_prop, "%ld", 1);
    cmd_add("get_obc_mag", adcs_get_mag, "", 0);
    cmd_add("get_obc_omega", adcs_get_omega, "", 0);
    cmd_add("get_obc_sun_vec", get_obc_sun_vec, "", 0);
    // Magnetorquers commands
    cmd_add("mtt_set_duty", mtt_set_pwm_duty, "%d %d", 2);
    cmd_add("mtt_set_freq", mtt_set_pwm_freq, "%d %f", 2);
    cmd_add("mtt_set_pwr", mtt_set_pwm_pwr, "%d", 1);

#if SCH_ADCS_ENABLED
    cmd_add("start_attitude", start_attitude, "%u %u %u", 3);
    cmd_add("set_sc_inertia_matrix", set_sc_inertia_matrix, "%lf %lf %lf %lf %lf %lf", 6);
    cmd_add("set_rw_inertia_matrix", set_rw_inertia_matrix, "%lf", 1);
    cmd_add("set_time_delay_gyro", set_time_delay_gyro, "%u", 1);
    cmd_add("set_time_delay_quat", set_time_delay_quat, "%u", 1);
    cmd_add("adcs_detumbling_mag", adcs_detumbling_mag, "", 0);
    cmd_add("adcs_send_attitude", adcs_send_attitude, "", 0);
    cmd_add("adcs_mag_moment", adcs_mag_moment, "", 0);
    cmd_add("set_mtq_axis", set_mtq_axis, "%f %f %f", 3);
    cmd_add("set_quat_fss", set_quat_fss, "%u %u %u %u %u", 5);
    cmd_add("set_bias_omega", set_bias_omega, "%lf %lf %lf", 3);
    cmd_add("adcs_point", adcs_point, "", 0);
    cmd_add("adcs_quat", adcs_get_quaternion, "", 0);
    cmd_add("adcs_do_control", adcs_control_torque, "%lf", 1);
    cmd_add("adcs_set_target", adcs_set_target, "%lf %lf %lf %lf %lf %lf", 6);
    cmd_add("adcs_set_to_nadir", adcs_target_nadir, "", 0);
#endif

    // Matrix of calibration of FSS
    T1[0][0] = cos(cal1[3]); T1[0][1] = sin(cal1[3]);
    T1[1][0] = -sin(cal1[3]); T1[1][1] = cos(cal1[3]);
    T2[0][0] = cos(cal2[3]); T2[0][1] = sin(cal2[3]);
    T2[1][0] = -sin(cal2[3]); T2[1][1] = cos(cal2[3]);
    T3[0][0] = cos(cal3[3]); T3[0][1] = sin(cal3[3]);
    T3[1][0] = -sin(cal3[3]); T3[1][1] = cos(cal3[3]);
    T4[0][0] = cos(cal4[3]); T4[0][1] = sin(cal4[3]);
    T4[1][0] = -sin(cal4[3]); T4[1][1] = cos(cal4[3]);
    T5[0][0] = cos(cal5[3]); T5[0][1] = sin(cal5[3]);
    T5[1][0] = -sin(cal5[3]); T5[1][1] = cos(cal5[3]);
}

int set_bias_omega(char *fmt, char *params, int nparams){
    double wxx, wyy, wzz;
    if (params == NULL || sscanf(params, fmt, &wxx, &wyy, &wzz) != nparams) {
        LOGE(tag, "Error parsing parameters!");
        return CMD_SYNTAX_ERROR;
    }
    vector3_t bias_sensor_omega_b;
    bias_sensor_omega_b.v[0] = wxx;
    bias_sensor_omega_b.v[1] = wyy;
    bias_sensor_omega_b.v[2] = wzz;
    _set_sat_vector(&bias_sensor_omega_b, dat_ads_bias_x);
    return CMD_OK;
}

int set_quat_fss(char *fmt, char *params, int nparams){
    int selected_quat;
    int q0, q1, q2, q3;
    if (params == NULL || sscanf(params, fmt, &selected_quat, &q0, &q1, &q2, &q3) != nparams) {
        LOGE(tag, "Error parsing parameters!");
        return CMD_SYNTAX_ERROR;
    }
    if (selected_quat == 1){
        q_c2b_fss1.q0 = (double) q0 * 0.0001;
        q_c2b_fss1.q1 = (double) q1 * 0.0001;
        q_c2b_fss1.q2 = (double) q2 * 0.0001;
        q_c2b_fss1.q3 = (double) q3 * 0.0001;
    }else if (selected_quat == 2){
        q_c2b_fss2.q0 = (double) q0 * 0.0001;
        q_c2b_fss2.q1 = (double) q1 * 0.0001;
        q_c2b_fss2.q2 = (double) q2 * 0.0001;
        q_c2b_fss2.q3 = (double) q3 * 0.0001;
    }else if (selected_quat == 3){
        q_c2b_fss3.q0 = (double) q0 * 0.0001;
        q_c2b_fss3.q1 = (double) q1 * 0.0001;
        q_c2b_fss3.q2 = (double) q2 * 0.0001;
        q_c2b_fss3.q3 = (double) q3 * 0.0001;
    }else if (selected_quat == 4){
        q_c2b_fss4.q0 = (double) q0 * 0.0001;
        q_c2b_fss4.q1 = (double) q1 * 0.0001;
        q_c2b_fss4.q2 = (double) q2 * 0.0001;
        q_c2b_fss4.q3 = (double) q3 * 0.0001;
    }else{
        q_c2b_fss5.q0 = (double) q0 * 0.0001;
        q_c2b_fss5.q1 = (double) q1 * 0.0001;
        q_c2b_fss5.q2 = (double) q2 * 0.0001;
        q_c2b_fss5.q3 = (double) q3 * 0.0001;
    };
    return CMD_OK;
}

int set_mtq_axis(char *fmt, char *params, int nparams){
    double axisx, axisy, axisz;
    if (params == NULL || sscanf(params, fmt, &axisx, &axisy, &axisz) != nparams) {
        LOGE(tag, "Error parsing parameters!");
        return CMD_SYNTAX_ERROR;
    }
    value32_t vx;
    value32_t vy;
    value32_t vz;
    vx.f = (float) (axisx);
    vy.f = (float) (axisy);
    vz.f = (float) (axisz);


    dat_set_status_var(dat_mtq_x_axis, vx);
    dat_set_status_var(dat_mtq_y_axis, vy);
    dat_set_status_var(dat_mtq_z_axis, vz);
    return CMD_OK;
}

int set_time_delay_gyro(char *fmt, char *params, int nparams){
    int time;
    if(params == NULL || sscanf(params, fmt, &time) != nparams)
    {
        LOGE(tag, "Error parsing parameters!");
        return CMD_SYNTAX_ERROR;
    }
    dat_set_system_var(dat_time_delay_gyro, time);
    return CMD_OK;
}

int set_time_delay_quat(char *fmt, char *params, int nparams){
    int time;
    if(params == NULL || sscanf(params, fmt, &time) != nparams)
    {
        LOGE(tag, "Error parsing parameters!");
        return CMD_SYNTAX_ERROR;
    }
    dat_set_system_var(dat_time_delay_quat, time);
    return CMD_OK;
}

int set_sc_inertia_matrix(char *fmt, char *params, int nparams) {

    double ixx, iyy, izz, ixy, ixz, iyz;
    if (params == NULL || sscanf(params, fmt, &ixx, &iyy, &izz, &ixy, &ixz, &iyz) != nparams) {
        LOGE(tag, "Error parsing parameters!");
        return CMD_SYNTAX_ERROR;
    }

    vector3_t temp1;
    vector3_t temp2;

    temp1.v[0] = ixx, temp1.v[1] = iyy, temp1.v[2] = izz;
    temp2.v[0] = ixy, temp2.v[1] = ixz, temp2.v[2] = iyz;

    _set_sat_vector(&temp1, dat_inertia_xx);
    _set_sat_vector(&temp2, dat_inertia_xy);

    matrix3_t inertia;

    inertia.m[0][0] = temp1.v[0];
    inertia.m[1][1] = temp1.v[1];
    inertia.m[2][2] = temp1.v[2];

    inertia.m[0][1] = temp2.v[0];
    inertia.m[0][2] = temp2.v[1];

    inertia.m[1][0] = temp2.v[0];
    inertia.m[1][2] = temp2.v[2];

    inertia.m[2][0] = temp2.v[1];
    inertia.m[2][1] = temp2.v[2];

    matrix3_t inv_Inertia_ekf;
    mat_inverse(inertia, &inv_Inertia_ekf);

    vector3_t temp3;
    vector3_t temp4;
    temp3.v0 = inv_Inertia_ekf.m[0][0];
    temp3.v1 = inv_Inertia_ekf.m[1][1];
    temp3.v2 = inv_Inertia_ekf.m[2][2];

    temp4.v0 = inv_Inertia_ekf.m[0][1];
    temp4.v1 = inv_Inertia_ekf.m[0][2];
    temp4.v2 = inv_Inertia_ekf.m[1][2];

    _set_sat_vector(&temp3, dat_inv_inertia_xx);
    _set_sat_vector(&temp4, dat_inv_inertia_xy);

    return CMD_OK;
}

int set_rw_inertia_matrix(char *fmt, char *params, int nparams){
    double irw;
    if(params == NULL || sscanf(params, fmt, &irw) != nparams)
    {
        LOGE(tag, "Error parsing parameters!");
        return CMD_SYNTAX_ERROR;
    }
    value32_t v;
    v.f = (float) (irw * 1e-6);
    dat_set_status_var(dat_inertia_rw, v);
    return CMD_OK;
}

int start_attitude(char *fmt, char *params, int nparams){
    printf("Setting start");
    uint32_t lapse_attitude = 0;
    uint32_t activate_ekf = 0;
    uint32_t activate_ctrl = 0;

    if(params != NULL && sscanf(params, fmt, &activate_ekf, &activate_ctrl, &lapse_attitude) != nparams) {
        return CMD_SYNTAX_ERROR;
    }

    if (lapse_attitude == 0){
        dat_set_system_var(dat_calc_attitude, 0);
        lapse_attitude = dat_get_system_var(dat_time_to_attitude);
    }
    dat_set_system_var(dat_calc_attitude, 1);
    dat_set_system_var(dat_time_to_attitude, lapse_attitude);
    dat_set_system_var(dat_activate_ekf, activate_ekf);
    dat_set_system_var(dat_activate_ctrl, activate_ctrl);
    printf("Start Set");
    return CMD_OK;

}

int tle_get(char *fmt, char *params, int nparams)
{
    LOGR(tag, "%s", tle1);
    LOGR(tag, "%s", tle2);
    return CMD_OK;
}

int tle_set(char *fmt, char *params, int nparams)
{
    int line_n, next;

    // fmt: "%d %n", nparams: 2
    // First number is parsed @line_n, but then all the line pointed by @params
    // is copied to the corresponding TLE line. Examples of @params:

    //          1         2         3         4         5         6
    //0123456789012345678901234567890123456789012345678901234567890123456789
    //----------------------------------------------------------------------
    //1 42788U 17036Z   20054.20928660  .00001463  00000-0  64143-4 0  9996
    //2 42788  97.3188 111.6825 0013081  74.6084 285.6598 15.23469130148339
    if(params == NULL || sscanf(params, fmt, &line_n, &next) != nparams-1)
    {
        LOGE(tag, "Error parsing parameters!");
        return CMD_SYNTAX_ERROR;
    }

    if(strlen(params) != 69)
    {
        LOGE(tag, "Invalid TLE line len! (%d)", strlen(params));
        return CMD_SYNTAX_ERROR;
    }

    if(line_n == 1)
    {
        memset(tle1, 0, TLE_BUFF_LEN);
        strncpy(tle1, params, TLE_BUFF_LEN-1);
    }
    else if(line_n == 2)
    {
        memset(tle2, 0, TLE_BUFF_LEN);
        strncpy(tle2, params, TLE_BUFF_LEN-1);
    }
    else
    {
        LOGE(tag, "Invalid TLE Line parameter");
        return CMD_SYNTAX_ERROR;
    }

    return CMD_OK;
}

int tle_update(char *fmt, char *params, int nparams)
{
    parseLines(&tle, tle1, tle2);
    //TODO: Check errors
    if(tle.sgp4Error != 0)
    {
        dat_set_system_var(dat_ads_tle_epoch, 0);
        return CMD_ERROR;
    }

    LOGR(tag, "TLE updated to epoch %.8f (%d)", tle.epoch, (int)(tle.epoch/1000.0));
    dat_set_system_var(dat_ads_tle_epoch, (int)(tle.epoch/1000.0));
    //int epoch_time = (int)(tle.epoch/1000.0);
    //uint32_t curr_time = (uint32_t) time(NULL);
    //if (curr_time < epoch_time){
    //    dat_set_time(epoch_time);
    //}
    return CMD_OK;
}

int tle_prop(char *fmt, char *params, int nparams)
{
    double r[3];  // Sat position in ECI frame
    double v[3];  // Sat velocity in ECI frame
    time_t ts=0;

    if(params != NULL && sscanf(params, fmt, &ts) != nparams){
        return CMD_SYNTAX_ERROR;
    }

    if(ts == 0) {
        ts = dat_get_time();
    }
    double ts_mili = 1000.0 * (double) ts;

    double diff = (double)ts - (double)tle.epoch/1000.0;
    diff /= 60.0;

    getRVForDate(&tle, ts_mili, r, v);

    LOGD(tag, "T : %.8f - %.8f = %.8f", ts_mili/1000.0, tle.epoch/1000.0, diff);
    LOGD(tag, "R : (%.8f, %.8f, %.8f)", r[0], r[1], r[2]);
    LOGD(tag, "V : (%.8f, %.8f, %.8f)", v[0], v[1], v[2]);
    LOGD(tag, "Er: %d", tle.rec.error);
    if(tle.sgp4Error != 0)
        return CMD_ERROR;

    value32_t pos[3] = {{.f=(float)r[0]},{.f=(float)r[1]}, {.f=(float)r[2]}};
    value32_t vel[3] = {{.f=(float)v[0]},{.f=(float)v[1]}, {.f=(float)v[2]}};

    dat_set_status_var(dat_ads_pos_x, pos[0]);
    dat_set_status_var(dat_ads_pos_y, pos[1]);
    dat_set_status_var(dat_ads_pos_z, pos[2]);
    dat_set_status_var(dat_ads_vel_x, vel[0]);
    dat_set_status_var(dat_ads_vel_y, vel[1]);
    dat_set_status_var(dat_ads_vel_z, vel[2]);
    dat_set_system_var(dat_ads_tle_last, (int)ts);

    return CMD_OK;
}

int calc_shadow_zone(vector3_t sun_pos_i, vector3_t sc_pos_i){
    int isDark;
    double radiusearthkm = 6378.137;
    double point_product = vec_inner_product(sun_pos_i, sc_pos_i);
    double r_sun = vec_norm(sun_pos_i);
    double r_sc = vec_norm(sc_pos_i);
    double theta = acos(point_product / (r_sc * r_sun));
    double theta_sun = acos(radiusearthkm / r_sun);
    double theta_sc = acos(radiusearthkm / r_sc);
    if (theta_sc + theta_sun < theta){
        //Shadow
        isDark = 1;
    }
    else {
        isDark = 0;
    }
    return isDark;
}

int mtt_set_pwm_duty(char* fmt, char* params, int nparams)
{
    int channel;
    int duty;
    if(params == NULL || sscanf(params, fmt, &channel, &duty) != nparams)
    {
        LOGW(tag, "set_pwm_duty used with invalid params!");
        return CMD_SYNTAX_ERROR;
    }

    if(channel < 0 || channel > 2 || duty < -100 || duty > 100)
    {
        LOGW(tag, "set_pwm_duty params out of range %d %d!", channel, duty);
        return CMD_SYNTAX_ERROR;
    }

    LOGR(tag, "Setting duty %d to Channel %d", duty, channel);
#if defined(NANOMIND)
    gs_a3200_pwm_enable(channel);
    gs_a3200_pwm_set_duty(channel, duty);
#elif defined(SIM)
    sim_adcs_set_magnetorquer((uint8_t)channel, (uint8_t)duty);
#endif
    return CMD_OK;
}

int mtt_set_pwm_freq(char* fmt, char* params, int nparams)
{
    int channel;
    float freq;

    if(params == NULL || sscanf(params, fmt, &channel, &freq) != nparams)
        return CMD_SYNTAX_ERROR;

    /* The pwm cant handle frequencies above 433 Hz or below 0.1 Hz */
    if(channel > 2 || channel < 0 || freq > 433.0 || freq < 0.1)
    {
        LOGW(tag, "Parameters out of range: 0 <= channel <= 2 (%d), 0.1 <= freq <= 433.0 (%.4f)", channel, freq);
        return CMD_SYNTAX_ERROR;
    }

    float actual_freq = 0.0F;
#ifdef NANOMIND
    actual_freq = gs_a3200_pwm_set_freq(channel, freq);
#else
    return CMD_ERROR;
#endif
    LOGR(tag, "PWM %d Freq set to: %.4f", channel, actual_freq);
    return CMD_OK;
}

int mtt_set_pwm_pwr(char *fmt, char *params, int nparams)
{
    int enable;
    if(params == NULL || sscanf(params, fmt, &enable) != nparams)
        return CMD_SYNTAX_ERROR;

    /* Turn on/off power channel */
    LOGR(tag, "PWM enabled: %d", enable>0 ? 1:0);
#ifdef NANOMIND
        if(enable > 0)
        gs_a3200_pwr_switch_enable(GS_A3200_PWR_PWM);
    else
        gs_a3200_pwr_switch_disable(GS_A3200_PWR_PWM);
#else
    return CMD_ERROR;
#endif
    return CMD_OK;
}

int adcs_get_mag(char* fmt, char* params, int nparams)
{
    vector3_t mag = {0};
#if defined(NANOMIND)
    gs_error_t result;
    gs_hmc5843_data_t hmc_reading;
    result = gs_hmc5843_read_single(&hmc_reading);

    if (result == GS_OK)
    {
        mag.v0 = hmc_reading.x; // nano Tesla
        mag.v1 = hmc_reading.y; // nT
        mag.v2 = hmc_reading.z; // nT
    }
    else
    {
        return CMD_ERROR;
    }
#elif defined(SIM)
    mag_read_t hmc_reading;
    int rc = sim_adcs_get_magnetometer(&hmc_reading);

    if (rc == 0)
    {
        mag.v0 = (double)hmc_reading.x;
        mag.v1 = (double)hmc_reading.y;
        mag.v2 = (double)hmc_reading.z;
    }
    else
    {
        return CMD_ERROR;
    }
#else
    return CMD_ERROR;
#endif

    _set_sat_vector(&mag, dat_ads_mag_x);
    return CMD_OK;
}

int adcs_get_omega(char* fmt, char* params, int nparams)
{
    vector3_t omega = {0};
    vector3_t bias_sensor_omega_b = {0};

#if defined(NANOMIND)
    int result;
    gs_mpu3300_gyro_t gyro_reading;
    result = gs_mpu3300_read_gyro(&gyro_reading);

    if(result == 0)
    {
        omega.v0 = gyro_reading.gyro_x;
        omega.v1 = gyro_reading.gyro_y;
        omega.v2 = gyro_reading.gyro_z;
    }
    else
    {
        return CMD_ERROR;
    }
#elif defined(SIM)
    gyro_read_t gyro_reading;
    int result = sim_adcs_get_gyroscope(&gyro_reading);

    if(result == 0)
    {
        omega.v0 = gyro_reading.gyro_x;
        omega.v1 = gyro_reading.gyro_y;
        omega.v2 = gyro_reading.gyro_z;
    }
    else
    {
        return CMD_ERROR;
    }
#else
    return CMD_ERROR;
#endif

    // Add bias and save readings
    _get_sat_vector(&bias_sensor_omega_b, dat_ads_bias_x);
    omega.v0 += bias_sensor_omega_b.v0;
    omega.v1 += bias_sensor_omega_b.v1;
    omega.v2 += bias_sensor_omega_b.v2;
    _set_sat_vector(&omega, dat_ads_omega_x);

    return CMD_OK;
}

int get_obc_sun_vec(char* fmt, char* params, int nparams)
{
    uint16_t sun1 = 300, sun2 = 500, sun3 = 500, sun4 = 500, sun5 = 500;
    int css_ok = 0, fss_ok = 0;
    vector3_t ss_unit_b = {0, 0, 0};
    // FSS
    uint16_t sun_fss1[4] = {2,2,2,2};
    uint16_t sun_fss2[4] = {1,1,1,1};
    uint16_t sun_fss3[4] = {1,1,1,1};
    uint16_t sun_fss4[4] = {2,2,2,2};
    uint16_t sun_fss5[4] = {1,1,1,1};

    double selected_face_axis[3] = {0,0,0}; // 0 if positive face, 1 if negative face of each axis
    vector3_t sun_vec_b_fss = {0, 0, 0};
    uint32_t sum_all_fss[5];
    int rc1 = 0; int rc2 = 0; int rc3 = 0; int rc4 = 0; int rc5 = 0;
    #ifdef NANOMIND
    gs_a3200_pwr_switch_enable(GS_A3200_PWR_GSSB);
    gs_a3200_pwr_switch_enable(GS_A3200_PWR_GSSB2);
    osDelay(30);

    //int rc1 = gs_gssb_istage_get_sun_voltage(0x10, 100, &sun1);
    //int rc2 = gs_gssb_istage_get_sun_voltage(0x11, 100, &sun2);
    //int rc3 = gs_gssb_istage_get_sun_voltage(0x12, 100, &sun3);
    //int rc4 = gs_gssb_istage_get_sun_voltage(0x13, 100, &sun4);
    //int rc5 = 0;

    if (SCH_DEVICE_ID == 2) {
        printf("SUCHAI 2 - FSS\n");
        // position and orientation of FSS (0x20: +X, 0x21: -Y, 0x22: -X, 0x23: +Y, 0x24: -Z)
        //rc5 = gs_gssb_istage_get_sun_voltage(0x14, 100, &sun5);
        int timei2c = 1000;
        if (gs_gssb_sun_sample_sensor(0x20, timei2c) != GS_OK)
            return CMD_ERROR;
        osDelay(30);
        //printf("1 \n");
        int rf1 = gs_gssb_sun_read_sensor_samples(0x20, timei2c, sun_fss1);
        osDelay(30);
        if (gs_gssb_sun_sample_sensor(0x21, timei2c) != GS_OK)
            return CMD_ERROR;
        osDelay(30);
        //printf("1 \n");
        int rf2 = gs_gssb_sun_read_sensor_samples(0x21, timei2c, sun_fss2);
        osDelay(30);
        if (gs_gssb_sun_sample_sensor(0x22, timei2c) != GS_OK)
            return CMD_ERROR;
        osDelay(30);
        //printf("1 \n");
        int rf3 = gs_gssb_sun_read_sensor_samples(0x22, timei2c, sun_fss3);
        osDelay(30);
        if (gs_gssb_sun_sample_sensor(0x23, timei2c) != GS_OK)
            return CMD_ERROR;
        osDelay(30);
        //printf("1 \n");
        int rf4 = gs_gssb_sun_read_sensor_samples(0x23, timei2c, sun_fss4);
        osDelay(30);
        if (gs_gssb_sun_sample_sensor(0x24, timei2c) != GS_OK)
            return CMD_ERROR;
        osDelay(30);
        //printf("1 \n");
        int rf5 = gs_gssb_sun_read_sensor_samples(0x24, timei2c, sun_fss5);
        osDelay(200);

        if(rf1 != 0 || rf2 != 0 || rf3 != 0 || rf4 != 0 || rf5 != 0)
        {
            printf("Error i2c fss \n");
            LOGE(tag, "Error reading fine sun sensors (%d, %d, %d, %d, %d)", rc1, rc2, rc3, rc4, rc5);
            //return CMD_ERROR;
        }else{
            fss_ok = 1;
            // face axis selection
            sum_all_fss[0] = sun_fss1[0] + sun_fss1[1] + sun_fss1[2] + sun_fss1[3];
            sum_all_fss[1] = sun_fss2[0] + sun_fss2[1] + sun_fss2[2] + sun_fss2[3];
            sum_all_fss[2] = sun_fss3[0] + sun_fss3[1] + sun_fss3[2] + sun_fss3[3];
            sum_all_fss[3] = sun_fss4[0] + sun_fss4[1] + sun_fss4[2] + sun_fss4[3];
            sum_all_fss[4] = sun_fss5[0] + sun_fss5[1] + sun_fss5[2] + sun_fss5[3];
            // position and orientation of FSS (0x20: +X, 0x21: -Y, 0x22: -X, 0x23: +Y, 0x24: -Z)
            //printf("Sum all elements: %d, %d, %d, %d, %d \n", sum_all_fss[0], sum_all_fss[1], sum_all_fss[2], sum_all_fss[3],sum_all_fss[4]);
            if (sum_all_fss[0] < sum_all_fss[2]){
                //printf("Axis -X \n");
                selected_face_axis[0] = 0;
            }else{
                selected_face_axis[0] = 0;
                //printf("Axis +X \n");
            }
            if (sum_all_fss[1] < sum_all_fss[3]){
                selected_face_axis[1] = 0;
                //printf("Axis +Y \n");
            }else{
                selected_face_axis[1] = 1;
                //printf("Axis -Y \n");
            }
            selected_face_axis[2] = 1;
            //printf("Axis -Z \n");
        }
    }
    if(rc1 != 0 || rc2 != 0 || rc3 != 0 || rc4 != 0 || rc5 != 0)
    {
        printf("Error i2c FSS \n");
        LOGE(tag, "Error reading coarse sun sensors (%d, %d, %d, %d, %d)", rc1, rc2, rc3, rc4, rc5);
        //return CMD_ERROR;
    }else{css_ok = 1;}
#endif
    if (css_ok)
    {
        uint16_t Ix = sun3;
        uint16_t Iy = sun2;
        uint16_t Iz = sun4;

        uint16_t noise_thr = 10; // [uA]

        if (Ix < noise_thr && Iy < noise_thr && Iz < noise_thr)
        {
            // shadow
            ss_unit_b.v1 = 0, ss_unit_b.v0= 0, ss_unit_b.v2 = 0;
        }else if (Iz < noise_thr) {
            // ignore incomplete observations
            ss_unit_b.v1 = 0, ss_unit_b.v0= 0, ss_unit_b.v2 = 0;
        }
        else{
            ss_unit_b.v0 = -Ix, ss_unit_b.v1 = -Iy, ss_unit_b.v2 = Iz;
            vec_normalize(&ss_unit_b, NULL);
        }
    }
    if (fss_ok && SCH_DEVICE_ID == 2) {
        double vec_d_x_1[2];
        double vec_d_x_2[2];
        double vec_d_y_1[2];
        double vec_d_y_2[2];
        double vec_d_z_2[2];

        // position and orientation of FSS ([0x20: +X], [0x21: -Y], [0x22: -X], [0x23: +Y], [0x24: -Z])
        // FSS 0x20
        double num_1 = sun_fss1[0] + sun_fss1[1] - sun_fss1[2] - sun_fss1[3];
        double num_2 = sun_fss1[0] + sun_fss1[3] - sun_fss1[1] - sun_fss1[2];
        double den = sun_fss1[0] + sun_fss1[1] + sun_fss1[2] + sun_fss1[3];
        vec_d_x_1[0] = num_1 / den + cal1[1];
        vec_d_x_1[1] = num_2 / den + cal1[2];

        // FSS 0x22
        num_1 = sun_fss3[0] + sun_fss3[1] - sun_fss3[2] - sun_fss3[3];
        num_2 = sun_fss3[0] + sun_fss3[3] - sun_fss3[1] - sun_fss3[2];
        den = sun_fss3[0] + sun_fss3[1] + sun_fss3[2] + sun_fss3[3];
        vec_d_x_2[0] = num_1 / den + cal3[1];
        vec_d_x_2[1] = num_2 / den + cal3[2];

        // FSS 0x23
        num_1 = sun_fss4[0] + sun_fss4[1] - sun_fss4[2] - sun_fss4[3];
        num_2 = sun_fss4[0] + sun_fss4[3] - sun_fss4[1] - sun_fss4[2];
        den = sun_fss4[0] + sun_fss4[1] + sun_fss4[2] + sun_fss4[3];
        vec_d_y_1[0] = num_1 / den + cal4[1];
        vec_d_y_1[1] = num_2 / den + cal4[2];

        // FSS 0x21
        num_1 = sun_fss2[0] + sun_fss2[1] - sun_fss2[2] - sun_fss2[3];
        num_2 = sun_fss2[0] + sun_fss2[3] - sun_fss2[1] - sun_fss2[2];
        den = sun_fss2[0] + sun_fss2[1] + sun_fss2[2] + sun_fss2[3];
        vec_d_y_2[0] = num_1 / den + cal2[1];
        vec_d_y_2[1] = num_2 / den + cal2[2];

        // FSS 0x24
        vec_d_z_2[0] = (sun_fss5[0] + sun_fss5[1] - sun_fss5[2] - sun_fss5[3]) / (sun_fss5[0] + sun_fss5[1] + sun_fss5[2] + sun_fss5[3]) + cal5[1];
        vec_d_z_2[1] = (sun_fss5[0] + sun_fss5[3] - sun_fss5[1] - sun_fss5[2]) / (sun_fss5[0] + sun_fss5[1] + sun_fss5[2] + sun_fss5[3]) + cal5[2];

        //printf("Vector X d (+x_b): %f, %f \n", vec_d_x_1[0], vec_d_x_1[1]);
        //printf("Vector X d (-x_b): %f, %f \n", vec_d_x_2[0], vec_d_x_2[1]);
        //printf("Vector Y d (+y_b): %f, %f \n", vec_d_y_1[0], vec_d_y_1[1]);
        //printf("Vector Y d (-y_b): %f, %f \n", vec_d_y_2[0], vec_d_y_2[1]);
        //printf("Vector Z d (-z_b): %f, %f \n", vec_d_z_2[0], vec_d_z_2[1]);

        double vec_dx_1[2], vec_dy_1[2], vec_dz_2[2], vec_dx_2[2], vec_dy_2[2];
        _mat_vec_mult((double *) T1, vec_d_x_1, (double *) &vec_dx_1, 2, 2);
        _mat_vec_mult((double *) T3, vec_d_x_2, (double *) &vec_dx_2, 2, 2);
        _mat_vec_mult((double *) T4, vec_d_y_1, (double *) &vec_dy_1, 2, 2);
        _mat_vec_mult((double *) T2, vec_d_y_2, (double *) &vec_dy_2, 2, 2);
        _mat_vec_mult((double *) T5, vec_d_z_2, (double *) &vec_dz_2, 2, 2);

        double phi_x_1 = atan2(vec_dx_1[0], vec_dx_1[1]); double theta_x_1 = atan(sqrt(vec_dx_1[0] * vec_dx_1[0] + vec_dx_1[1] * vec_dx_1[1]) / cal1[0]);
        double phi_x_2 = atan2(vec_dx_2[0], vec_dx_2[1]); double theta_x_2 = atan(sqrt(vec_dx_2[0] * vec_dx_2[0] + vec_dx_2[1] * vec_dx_2[1]) / cal3[0]);
        double phi_y_1 = atan2(vec_dy_1[0], vec_dy_1[1]); double theta_y_1 = atan(sqrt(vec_dy_1[0] * vec_dy_1[0] + vec_dy_1[1] * vec_dy_1[1]) / cal4[0]);
        double phi_y_2 = atan2(vec_dy_2[0], vec_dy_2[1]); double theta_y_2 = atan(sqrt(vec_dy_2[0] * vec_dy_2[0] + vec_dy_2[1] * vec_dy_2[1]) / cal2[0]);
        double phi_z_2 = atan2(vec_dz_2[0], vec_dz_2[1]); double theta_z_2 = atan(sqrt(vec_dz_2[0] * vec_dz_2[0] + vec_dx_2[1] * vec_dx_2[1]) / cal5[0]);

        vector3_t sun_vec_cx_1, sun_vec_cy_1, sun_vec_cx_2, sun_vec_cy_2, sun_vec_cz_2;
        sun_vec_cx_1.v[0] = cos(theta_x_1); sun_vec_cx_1.v[1] = sin(theta_x_1) * cos(phi_x_1); sun_vec_cx_1.v[2] = sin(theta_x_1) * sin(phi_x_1);
        sun_vec_cx_2.v[0] = cos(theta_x_2); sun_vec_cx_2.v[1] = sin(theta_x_2) * cos(phi_x_2); sun_vec_cx_2.v[2] = sin(theta_x_2) * sin(phi_x_2);
        sun_vec_cy_1.v[0] = cos(theta_y_1); sun_vec_cy_1.v[1] = sin(theta_y_1) * cos(phi_y_1); sun_vec_cy_1.v[2] = sin(theta_y_1) * sin(phi_y_1);
        sun_vec_cy_2.v[0] = cos(theta_y_2); sun_vec_cy_2.v[1] = sin(theta_y_2) * cos(phi_y_2); sun_vec_cy_2.v[2] = sin(theta_y_2) * sin(phi_y_2);
        sun_vec_cz_2.v[0] = cos(theta_z_2); sun_vec_cz_2.v[1] = sin(theta_z_2) * cos(phi_z_2); sun_vec_cz_2.v[2] = sin(theta_z_2) * sin(phi_z_2);

        //printf("SUN VECTOR X+AXIS C: %f, %f, %f \n", sun_vec_cx_1.v[0], sun_vec_cx_1.v[1], sun_vec_cx_1.v[2]);
        //printf("SUN VECTOR Y+AXIS C: %f, %f, %f \n", sun_vec_cy_1.v[0], sun_vec_cy_1.v[1], sun_vec_cy_1.v[2]);
        //printf("SUN VECTOR X-AXIS C: %f, %f, %f \n", sun_vec_cx_2.v[0], sun_vec_cx_2.v[1], sun_vec_cx_2.v[2]);
        //printf("SUN VECTOR Y-AXIS C: %f, %f, %f \n", sun_vec_cy_2.v[0], sun_vec_cy_2.v[1], sun_vec_cy_2.v[2]);
        //printf("SUN VECTOR Z-AXIS C: %f, %f, %f \n", sun_vec_cz_2.v[0], sun_vec_cz_2.v[1], sun_vec_cz_2.v[2]);
        // position and orientation of FSS ([0x20: +X], [0x21: -Y], [0x22: -X], [0x23: +Y], [0x24: -Z])
        vector3_t sun_vec_x1_b = {0,0,0};
        vector3_t sun_vec_y2_b = {0,0,0};
        vector3_t sun_vec_x2_b = {0,0,0};
        vector3_t sun_vec_y1_b = {0,0,0};
        vector3_t sun_vec_z2_b = {0,0,0};
        quat_frame_conv(&q_c2b_fss1, &sun_vec_cx_1, &sun_vec_x1_b);
        quat_frame_conv(&q_c2b_fss2, &sun_vec_cy_2, &sun_vec_y2_b);
        quat_frame_conv(&q_c2b_fss3, &sun_vec_cx_2, &sun_vec_x2_b);
        quat_frame_conv(&q_c2b_fss4, &sun_vec_cy_1, &sun_vec_y1_b);
        quat_frame_conv(&q_c2b_fss5, &sun_vec_cz_2, &sun_vec_z2_b);

        //printf("SUN VECTOR X+AXIS BF: %f, %f, %f \n", sun_vec_x1_b.v[0], sun_vec_x1_b.v[1], sun_vec_x1_b.v[2]);
        //printf("SUN VECTOR Y+AXIS BF: %f, %f, %f \n", sun_vec_y1_b.v[0], sun_vec_y1_b.v[1], sun_vec_y1_b.v[2]);
        //printf("SUN VECTOR X-AXIS BF: %f, %f, %f \n", sun_vec_x2_b.v[0], sun_vec_x2_b.v[1], sun_vec_x2_b.v[2]);
        //printf("SUN VECTOR Y-AXIS BF: %f, %f, %f \n", sun_vec_y2_b.v[0], sun_vec_y2_b.v[1], sun_vec_y2_b.v[2]);
        //printf("SUN VECTOR Z-AXIS BF: %f, %f, %f \n", sun_vec_x2_b.v[0], sun_vec_x2_b.v[1], sun_vec_x2_b.v[2]);

        sun_vec_b_fss.v[0] = sun_vec_x1_b.v[0] + sun_vec_x2_b.v[0] + sun_vec_y1_b.v[0] + sun_vec_y2_b.v[0] + sun_vec_z2_b.v[0];
        sun_vec_b_fss.v[1] = sun_vec_x1_b.v[1] + sun_vec_x2_b.v[1] + sun_vec_y1_b.v[1] + sun_vec_y2_b.v[1] + sun_vec_z2_b.v[1];
        sun_vec_b_fss.v[2] = sun_vec_x1_b.v[2] + sun_vec_x2_b.v[2] + sun_vec_y1_b.v[2] + sun_vec_y2_b.v[2] + sun_vec_z2_b.v[2];
        vec_normalize(&sun_vec_b_fss, NULL);
    }
    vector3_t sun_vec_b;
    vec_sum(ss_unit_b, sun_vec_b_fss, &sun_vec_b);
    vec_normalize(&sun_vec_b, NULL);
    _set_sat_vector(&sun_vec_b, dat_sun_vec_b_x);
    //LOGI(tag, "Sun direction: (%f, %f, %f) [@bodyframe]", sun_vec_b.v0, sun_vec_b.v1, sun_vec_b.v2);
    return CMD_OK;
}

int adcs_detumbling_mag(char* fmt, char* params, int nparams)
{
    quaternion_t q_i2b_tar;
    q_i2b_tar.q0 = 0;
    q_i2b_tar.q1 = 0;
    q_i2b_tar.q2 = 0;
    q_i2b_tar.q3 = 1;

    vector3_t omega_b_tar;
    omega_b_tar.v[0] = 0.0;
    omega_b_tar.v[1] = 0.0;
    omega_b_tar.v[2] = 0.0;

    _set_sat_quaterion(&q_i2b_tar, dat_tgt_q0);
    _set_sat_vector(&omega_b_tar, dat_tgt_omega_x);
    LOGI(tag, "TGT OMEGA: %lf %lf %lf", omega_b_tar.v0, omega_b_tar.v1, omega_b_tar.v2);
    LOGI(tag, "TGT QUAT: %lf %lf %lf %lf", q_i2b_tar.q0, q_i2b_tar.q1, q_i2b_tar.q2, q_i2b_tar.q3);
    return CMD_OK;
}

int adcs_set_target(char* fmt, char* params, int nparams)
{
    double rot;
    vector3_t i_tar;  // Target vector, inertial frame, read as parameter
    vector3_t b_tar;
    vector3_t b_dir;  // Face to point to, body frame
    vector3_t b_lambda;
    vector3_t omega_tar;  // Target velocity vector, body frame, read as parameter
    quaternion_t q_i2b_est;
    quaternion_t q_b2b_now2tar;
    quaternion_t q_i2b_tar; // Target quaternion, inertial to body frame. Calculate

    if(params == NULL || sscanf(params, fmt, &i_tar.v0, &i_tar.v1, &i_tar.v2, &omega_tar.v0, &omega_tar.v1, &omega_tar.v2) != nparams)
        return CMD_ERROR;
//  int p = sscanf(params, fmt, &i_tar.v0, &i_tar.v1, &i_tar.v2, &omega_tar.v0, &omega_tar.v1, &omega_tar.v2);
    LOGW(tag, fmt, i_tar.v0, i_tar.v1, i_tar.v2, omega_tar.v0, omega_tar.v1, omega_tar.v2);
    // Set Z+ [0, 0, 1] as the face to point to
    b_dir.v0 = 0.0; b_dir.v1 = 0.0; b_dir.v2 = 1.0;
    vec_normalize(&b_dir, NULL);

    // Get target vector in body frame
    _get_sat_quaterion(&q_i2b_est, dat_ads_ekf_q0);
    vec_normalize(&i_tar, NULL);
    quat_frame_conv(&q_i2b_est, &i_tar, &b_tar);
    vec_normalize(&b_tar, NULL);

    // Get I2B target quaternion
    vec_outer_product(b_dir, b_tar, &b_lambda);
    vec_normalize(&b_lambda, NULL);
    rot = acos(vec_inner_product(b_dir, b_tar));
    axis_rotation_to_quat(b_lambda, rot, &q_b2b_now2tar); //Calculate quaternion of shaft rotation
    quat_normalize(&q_b2b_now2tar, NULL);
    quat_mult(&q_i2b_est, &q_b2b_now2tar, &q_i2b_tar); //Calculate quaternion after rotation

    _set_sat_quaterion(&q_i2b_tar, dat_tgt_q0);
    _set_sat_vector(&omega_tar, dat_tgt_omega_x);

    LOGI(tag, "TGT QUAT: %lf %lf %lf %lf", q_i2b_tar.q0, q_i2b_tar.q1, q_i2b_tar.q2, q_i2b_tar.q3);
    return CMD_OK;
}

int adcs_send_attitude(char* fmt, char* params, int nparams)
{
    quaternion_t q_est, q_tgt;
    _get_sat_quaterion(&q_est, dat_ads_ekf_q0);
    _get_sat_quaterion(&q_tgt, dat_tgt_q0);

    csp_packet_t *packet = csp_buffer_get(COM_FRAME_MAX_LEN);
    if(packet == NULL)
        return CMD_ERROR;

    int len = snprintf(packet->data, COM_FRAME_MAX_LEN,
                       "adcs_set_attitude %lf %lf %lf %lf %lf %lf %lf %lf",
                       q_est.q0, q_est.q1, q_est.q2, q_est.q3,
                       q_tgt.q0, q_tgt.q1, q_tgt.q2, q_tgt.q3);
    packet->length = len;
    LOGI(tag, "OBC ATT: (%d) %s", packet->length, packet->data);

    int rc = csp_sendto(CSP_PRIO_NORM, ADCS_PORT, SCH_TRX_PORT_CMD,
                        SCH_TRX_PORT_CMD, CSP_O_NONE, packet, 100);

    if(rc != 0)
    {
        csp_buffer_free((void *)packet);
        return CMD_ERROR;
    }

    return CMD_OK;
}

int adcs_mag_moment(char* fmt, char* params, int nparams)
{
    // GLOBALS
    vector3_t max_mag_am2;
    max_mag_am2.v[0] = 0.35;
    max_mag_am2.v[1] = 0.35;
    max_mag_am2.v[2] = 0.35;
    matrix3_t I_c;
    mat_set_diag(&I_c, 0.035, 0.035, 0.007);
    double nT2T = 1.0e-9;

    // PARAMETERS
    vector3_t mag_earth_b_est;
    _get_sat_vector(&mag_earth_b_est, dat_ads_mag_x);
    vector3_t omega_b_est;  // Current GYRO. Read from ADCS
    _get_sat_vector(&omega_b_est, dat_ads_ekf_omega_x);
    vector3_t omega_b_tar;
    _get_sat_vector(&omega_b_tar, dat_tgt_omega_x);

    vec_cons_mult(-1.0, &omega_b_tar, NULL);
    vector3_t error_angular_vel;
    vec_sum(omega_b_est, omega_b_tar, &error_angular_vel); //dw = w_b_est - w_b_tar
    vector3_t control_torque;
    vec_cons_mult(-1.0, &error_angular_vel, NULL);
    mat_vec_mult(I_c, error_angular_vel, &control_torque); //t = -I * dw
    double inv_norm_torque = 1.0;
    double norm_torque = vec_norm(control_torque);
    if (norm_torque >= pow(10.0, -9.0))
    {
        inv_norm_torque = 1.0 / norm_torque;
    }
    vec_cons_mult(inv_norm_torque, &control_torque, NULL); //t = t/||t|| = -I*dw / ||I*dw||

    vector3_t nT2T_mag_earth_b_est;
    vec_cons_mult(nT2T, &mag_earth_b_est, &nT2T_mag_earth_b_est);
    vector3_t max_torque;
    //bellow method should be named cross product
    vec_outer_product(max_mag_am2, nT2T_mag_earth_b_est, &max_torque);// t_max = m_max x B_est
    printf("max torque %f %f %f \n", max_torque.v0, max_torque.v1, max_torque.v2);
    control_torque.v[0] *= max_torque.v[0];//tx = tx*dirx*|t_max_x|
    control_torque.v[1] *= max_torque.v[1];
    control_torque.v[2] *= max_torque.v[2];

    double inv_b_norm2 = 1.0 / pow(nT2T * vec_norm(mag_earth_b_est), 2.0);//=1/||B_est||**2
    vector3_t control_mag_moment_temp, control_mag_moment;
    vec_outer_product(nT2T_mag_earth_b_est, control_torque, &control_mag_moment_temp);//mc* = Bxt
    vec_cons_mult(inv_b_norm2, &control_mag_moment_temp, &control_mag_moment); //mc =  Bxt / ||B_est||**2

    LOGI(tag, "CTRL_MAG_MOMENT: %f, %f, %f", control_mag_moment.v0, control_mag_moment.v1, control_mag_moment.v2);

    csp_packet_t *packet = csp_buffer_get(COM_FRAME_MAX_LEN);
    if(packet == NULL)
        return CMD_ERROR;

    int len = snprintf(packet->data, COM_FRAME_MAX_LEN,
                       "adcs_set_mag_moment %.06f %.06f %.06f",
                       control_mag_moment.v0, control_mag_moment.v1, control_mag_moment.v2);
    //Calc PWM duty cycle from magnetic moment
    int8_t mtq_duty[3];
    //Todo: calc based on model
    double magMoment_control_norm = vec_norm(control_mag_moment);
    vector3_t magMoment_control_unit;
    if (magMoment_control_norm >= pow(10.0, -9.0))
    {
        vec_cons_mult(100.0 / magMoment_control_norm, &magMoment_control_unit, NULL); //m_hat = m/||m||
    } else
    {
        magMoment_control_unit.v[0] = 0.0;
        magMoment_control_unit.v[1] = 0.0;
        magMoment_control_unit.v[2] = 0.0;
    }
    mtq_duty[0] = (int8_t) control_mag_moment.v[0];
    mtq_duty[1] = (int8_t) control_mag_moment.v[1];
    mtq_duty[2] = (int8_t) control_mag_moment.v[2];

    value32_t vx;
    value32_t vy;
    value32_t vz;

    vx = dat_get_status_var(dat_mtq_x_axis);
    vy = dat_get_status_var(dat_mtq_y_axis);
    vz = dat_get_status_var(dat_mtq_z_axis);

    mtq_duty[0] *= vx.f;
    mtq_duty[1] *= vy.f;
    mtq_duty[2] *= vz.f;

    for (int i=0; i < 3; i++){
        int8_t abs_val = abs(mtq_duty[i]);
        if (abs_val > 100) {
            mtq_duty[i] = 100 * mtq_duty[i] / abs_val;
        }
    }

    //Enable MTQ's through PWM commands
    //Check when the power on cmd should be call
#ifdef NANOMIND
    gs_a3200_pwr_switch_enable(GS_A3200_PWR_GSSB);
    gs_a3200_pwr_switch_enable(GS_A3200_PWR_GSSB2);
#endif
    osDelay(20);
    cmd_t *cmd_set_on_mtt = cmd_get_str("mtt_set_pwr");
    cmd_add_params_var(cmd_set_on_mtt, 1);
    cmd_send(cmd_set_on_mtt)
    osDelay(20);
    cmd_t *cmd_set_pwm_x = cmd_get_str("mtt_set_duty");
    cmd_add_params_var(cmd_set_pwm_x, 0, mtq_duty[0]);
    cmd_send(cmd_set_pwm_x);
    osDelay(20);
    cmd_t *cmd_set_pwm_y = cmd_get_str("mtt_set_duty");
    cmd_add_params_var(cmd_set_pwm_y, 1, mtq_duty[1]);
    cmd_send(cmd_set_pwm_y);
    osDelay(20);
    cmd_t *cmd_set_pwm_z = cmd_get_str("mtt_set_duty");
    cmd_add_params_var(cmd_set_pwm_z, 2, mtq_duty[2]);
    cmd_send(cmd_set_pwm_z);
    osDelay(20);
    packet->length = len;
    LOGI(tag, "ADCS CMD: (%d) %s", packet->length, packet->data);

    /* Save ADCS data */
    int curr_time = dat_get_time();
    int index_ads = dat_get_system_var(data_map[ctrl_data].sys_index);
    ctrl_data_t data_ads_ctrl = {index_ads, curr_time, control_mag_moment.v0, control_mag_moment.v1,
                               control_mag_moment.v2, mtq_duty[0], mtq_duty[1], mtq_duty[2]};
    dat_add_payload_sample(&data_ads_ctrl, ctrl_data);

    int rc = csp_sendto(CSP_PRIO_NORM, ADCS_PORT, SCH_TRX_PORT_CMD,
                        SCH_TRX_PORT_CMD, CSP_O_NONE, packet, 100);

    if(rc != 0)
    {
        csp_buffer_free((void *)packet);
        return CMD_ERROR;
    }

    return CMD_OK;
}

#if 1
int adcs_point(char* fmt, char* params, int nparams)
{
    csp_packet_t *packet = csp_buffer_get(COM_FRAME_MAX_LEN);
    if(packet == NULL)
        return CMD_SYNTAX_ERROR;
    memset(packet->data, 0, COM_FRAME_MAX_LEN);

    vector3_t r;
    _get_sat_vector(&r, dat_ads_pos_x);

    int len = snprintf(packet->data, COM_FRAME_MAX_LEN,
                       "adcs_point_to %lf %lf %lf", r.v0, r.v1, r.v2);
    packet->length = len;
    LOGI(tag, "ADCS CMD: (%d) %s", packet->length, packet->data);

    int rc = csp_sendto(CSP_PRIO_NORM, ADCS_PORT, SCH_TRX_PORT_CMD,
                        SCH_TRX_PORT_CMD, CSP_O_NONE, packet, 1000);

    if(rc != 0)
    {
        csp_buffer_free((void *)packet);
        return CMD_ERROR;
    }

    return CMD_OK;
}

int adcs_get_quaternion(char* fmt, char* params, int nparams)
{
    char *out_buff = (char *)malloc(COM_FRAME_MAX_LEN);
    char *in_buff = (char *)malloc(COM_FRAME_MAX_LEN);
    memset(out_buff, 0, COM_FRAME_MAX_LEN);
    memset(in_buff, 0, COM_FRAME_MAX_LEN);

    int len = snprintf(out_buff, COM_FRAME_MAX_LEN, "adcs_get_quaternion 0");
    int rc = csp_transaction(CSP_PRIO_NORM, ADCS_PORT, SCH_TRX_PORT_CMD, 100,
                             out_buff, COM_FRAME_MAX_LEN, in_buff, COM_FRAME_MAX_LEN);

    if(rc == COM_FRAME_MAX_LEN)
    {
        LOGI(tag, "QUAT: %s", in_buff);
        quaternion_t q;
        rc = sscanf(in_buff, "%lf %lf %lf %lf", &q.q0, &q.q1, &q.q2, &q.q3);
        if(rc == 4)
        {
            _set_sat_quaterion(&q, dat_ads_q0);
            quaternion_t tmp;
            _get_sat_quaterion(&tmp, dat_ads_q0);
            LOGI(tag, "SAT_QUAT: %.04f, %.04f, %.04f, %.04f", tmp.q0, tmp.q1, tmp.q2, tmp.q3);
            free(out_buff);
            free(in_buff);
            return CMD_OK;
        }
        LOGE(tag, "Error reading values!")
    }
    LOGE(tag, "csp_transaction failed! (%d)", rc);

    free(out_buff);
    free(in_buff);
    return CMD_SYNTAX_ERROR;
}

int adcs_control_torque(char* fmt, char* params, int nparams)
{
    // GLOBALS
    double ctrl_cycle;
    matrix3_t I_quat;
    mat_set_diag(&I_quat, 0.00, 0.00, 0.00);
    matrix3_t P_quat;
    mat_set_diag(&P_quat, 0.001, 0.001, 0.001);
    matrix3_t P_omega;
    mat_set_diag(&P_omega, 0.003, 0.003, 0.003);

    if(params == NULL || sscanf(params, fmt, &ctrl_cycle) != nparams)
        return CMD_SYNTAX_ERROR;

    // PARAMETERS
    quaternion_t q_i2b_est; // Current quaternion. Read as from ADCS
    quaternion_t q_i2b_tar; // Target quaternion. Read as parameter
    _get_sat_quaterion(&q_i2b_est, dat_ads_q0);
    _get_sat_quaterion(&q_i2b_tar, dat_tgt_q0);
    vector3_t omega_b_est;  // Current GYRO. Read from ADCS
    _get_sat_vector(&omega_b_est, dat_ads_omega_x);
    vector3_t omega_b_tar;
    _get_sat_vector(&omega_b_tar, dat_tgt_omega_x);

//    libra::Quaternion q_b2i_est = q_i2b_est_.conjugate(); //body frame to inertial frame
//    libra::Quaternion q_i2b_now2tar = q_b2i_est * q_i2b_tar_;//q_i2b_tar_ = qi2b_est * qi2b_now2tar：クオータニオンによる2回転は積であらわされる。
//    q_i2b_now2tar.normalize();
    quaternion_t q_b2i_est;
    quat_conjugate(&q_i2b_est, &q_b2i_est);
    quaternion_t q_i2b_now2tar;
    quat_mult(&q_b2i_est, &q_i2b_tar, &q_i2b_now2tar);
    quat_normalize(&q_i2b_now2tar, NULL);

//    Vector<3> TorqueDirection;
//    TorqueDirection[0] = q_i2b_now2tar[0];
//    TorqueDirection[1] = q_i2b_now2tar[1];
//    TorqueDirection[2] = q_i2b_now2tar[2];
//    TorqueDirection = normalize(TorqueDirection);//q1,q2,q3を標準化している。つまり、p=(l,m,n)Tを求めている。
    vector3_t torq_dir;
    memcpy(torq_dir.v, q_i2b_now2tar.vec, sizeof(vector3_t));
    assert(torq_dir.v0 == q_i2b_now2tar.q0 && torq_dir.v1 == q_i2b_now2tar.q1 && torq_dir.v2 == q_i2b_now2tar.q2);
    vec_normalize(&torq_dir, NULL);

//    double AttitudeRotation = 2 * acos(q_i2b_now2tar[3]) * 180 / M_PI; //回転角θ。q_i2b_now2tar[3]は授業ではq0として扱った。
//    error_integral = ctrl_cycle_ * AttitudeRotation * TorqueDirection;
    double att_rot = 2*acos(q_i2b_now2tar.q[3]);
    vector3_t error_integral;
    vec_cons_mult(att_rot*ctrl_cycle, &torq_dir, &error_integral);

//    Vector<3> ControlTorque = (P_quat_ * (AttitudeRotation * TorqueDirection)) + (I_quat_ * error_integral) + (P_omega_ * (omega_b_tar_ - omega_b_est_));
    vector3_t torque_rot;
    vec_cons_mult(att_rot, &torq_dir, &torque_rot); //(AttitudeRotation * TorqueDirection)
    vector3_t P;
    mat_vec_mult(P_quat, torque_rot, &P);  //P_quat_ * (AttitudeRotation * TorqueDirection)
    vector3_t I;
    mat_vec_mult(I_quat, error_integral, &I); //I_quat_ * error_integral
    vector3_t omega_b;
    vector3_t P_o;
    vec_cons_mult(-1.0, &omega_b_est, NULL);
    vec_sum(omega_b_tar, omega_b_est, &omega_b);
    mat_vec_mult(P_omega, omega_b, &P_o); //P_omega_ * (omega_b_tar_ - omega_b_est_);

    vector3_t control_torque_tmp, control_torque;
    vec_sum(P, I, &control_torque_tmp);
    vec_sum(P_o, control_torque_tmp, &control_torque);

    LOGI(tag, "CTRL_TORQUE: %f, %f, %f", control_torque.v0, control_torque.v1, control_torque.v2);

    csp_packet_t *packet = csp_buffer_get(COM_FRAME_MAX_LEN);
    if(packet == NULL)
        return CMD_ERROR;

    int len = snprintf(packet->data, COM_FRAME_MAX_LEN,
                       "adcs_set_torque %.06f %.06f %.06f",
                       control_torque.v0, control_torque.v1, control_torque.v2);
    packet->length = len;
    LOGI(tag, "ADCS CMD: (%d) %s", packet->length, packet->data);

    int rc = csp_sendto(CSP_PRIO_NORM, ADCS_PORT, SCH_TRX_PORT_CMD,
                        SCH_TRX_PORT_CMD, CSP_O_NONE, packet, 100);

    if(rc != 0)
    {
        csp_buffer_free((void *)packet);
        return CMD_ERROR;
    }

    return CMD_OK;
}

int adcs_target_nadir(char* fmt, char* params, int nparams)
{
    // Get Nadir vector
    vector3_t i_tar;
    _get_sat_vector(&i_tar, dat_ads_pos_x);
    vec_cons_mult(-1.0, &i_tar, NULL);
    vector3_t v_tar;
    _get_sat_vector(&v_tar, dat_ads_pos_x);
    vec_cons_mult(-1.0, &v_tar, NULL);

    vector3_t temp;
    vec_outer_product(i_tar, v_tar, &temp);

    double rr = vec_norm(i_tar);
    rr *= rr;

    vec_normalize(&i_tar, NULL);
    vec_normalize(&v_tar, NULL);

    // Get required Nadir velocity
    // Target GYRO. ECI frame. For LEO sat -> nadir
    vector3_t omega_i_tar;
    vec_cons_mult(rr, &temp, &omega_i_tar);

    vector3_t omega_b_tar;
    quaternion_t q_i2b_est;
    _get_sat_quaterion(&q_i2b_est, dat_ads_q0);
    quat_frame_conv(&q_i2b_est, &omega_i_tar, &omega_b_tar);

    char *_fmt = "%lf %lf %lf %lf %lf %lf";
    char _params[SCH_CMD_MAX_STR_PARAMS];
    memset(_params, 0, SCH_CMD_MAX_STR_PARAMS);
    snprintf(_params, SCH_CMD_MAX_STR_PARAMS, _fmt, i_tar.v0, i_tar.v1, i_tar.v2,
             omega_b_tar.v0, omega_b_tar.v1, omega_b_tar.v2);
    int ret = adcs_set_target(_fmt, _params, 6);

    return ret;
}

#endif