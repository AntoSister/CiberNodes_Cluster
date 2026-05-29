/*                                 SUCHAI
 *                      NANOSATELLITE FLIGHT SOFTWARE
 *
 *      Copyright 2020, Carlos Gonzalez Cortes, carlgonz@uchile.cl
 *      Copyright 2022, Elias Obreque Sepulveda, elias.obreque@uchile.cl
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

#include "app/system/taskADCS.h"
#include "app/system/TRIADEFK.h"
#include "app/system/cmdADCS.h"

static const char *tag = "taskADCS";

void taskADCS(void *param)
{
    LOGI(tag, "ADCS Started");
    dat_set_system_var(dat_calc_attitude, 0);
    dat_set_system_var(dat_time_delay_gyro, 200);
    dat_set_system_var(dat_time_delay_quat, 3000);

    value32_t vx;
    value32_t vy;
    value32_t vz;
    vx.f = (float) (-1);
    vy.f = (float) (-1);
    vz.f = (float) (-1);
    dat_set_status_var(dat_mtq_x_axis, vx);
    dat_set_status_var(dat_mtq_y_axis, vy);
    dat_set_status_var(dat_mtq_z_axis, vz);
    dat_set_system_var(dat_activate_ekf, 0);
    dat_set_system_var(dat_activate_ekf, 0);

    portTick delay_ms  = 100;            //Task period in [ms]

//    unsigned int elapsed_sec = 0;           // Seconds counter
    uint32_t elapsed_msec = 0;
    unsigned int _adcs_ctrl_period = 1000;     // ADCS control period in seconds
    unsigned int _10sec_check = 10*1000;         // 10[s] condition
    unsigned int _01min_check = 1*60*1000;       // 05[m] condition
    unsigned int _05min_check = 5*60*1000;       // 05[m] condition
    unsigned int _1hour_check = 60*60*1000;      // 01[h] condition

    portTick xLastWakeTime = osTaskGetTickCount();

    /**
     * Set-up SGP4 propagator
     */
    cmd_t *tle1 = cmd_get_str("tle_set");
    cmd_add_params_str(tle1, "1 42788U 17036Z   22042.64086927  .00012515  00000-0  46931-3 0  9998");
    cmd_send(tle1);
    cmd_t *tle2 = cmd_get_str("tle_set");
    cmd_add_params_str(tle2, "2 42788  97.2207  90.1311 0011706  55.6478 304.5868 15.27309019257963");
    cmd_send(tle2);
    cmd_t *tle_u = cmd_get_str("tle_update");
    cmd_send(tle_u);
    tle_u = cmd_get_str("tle_get");
    cmd_send(tle_u);

    cmd_t *set_inertia = cmd_get_str("set_sc_inertia_matrix");
    cmd_add_params_var(set_inertia, 38910.883, 38478.678, 6873.717, 71.965, -347.697, 332.356);
    cmd_send(set_inertia);

    cmd_t *set_inertia_rw = cmd_get_str("set_rw_inertia_matrix");
    cmd_add_params_var(set_inertia_rw, 1.86);
    cmd_send(set_inertia_rw);

    // dat_set_system_var(dat_obc_opmode, DAT_OBC_OPMODE_DETUMB_MAG);

    // Vectors
    vector3_t current_omega_b = {0.2, -0.2,  0.1};
    vector3_t current_bias_b = {-0.0, 0.0, 0.0};
    vector3_t sat_pos_i;
    vector3_t geod_vect;
    vector3_t current_mag_i;
    int isDark;
    vector3_t  sun_pos_i;
    vector3_t sun_dir_b;
    vector3_t current_mag_b;
    quaternion_t current_q_det = {0.0, 0.0 ,0.0 ,1.0};
    quaternion_t current_q_est_i2b = {0.0, 0.0 ,0.0 ,1.0};
    vector3_t current_omega_b_est = {1.0, -1.0,  -1.0};

    // Set bias in repo
    _set_sat_vector(&current_bias_b, dat_ads_ekf_bias_x);

    double P[6][6];
    _mat_set_diag((double*)P, 1.0,6, 6);

    double yaw_rate = 10.0;
    double pitch_rate = -10.0;
    double roll_rate = 5.0;
    int if_sun_info = 0;
    while(1)
    {
        if (dat_get_system_var(dat_calc_attitude)) {
            /**
             * Estimation and Determination LOOP
             */
            int td_gyro = dat_get_system_var(dat_time_delay_gyro);
            int td_quat = dat_get_system_var(dat_time_delay_quat);
            int act_ekf = dat_get_system_var(dat_activate_ekf);
            int act_ctrl = dat_get_system_var(dat_activate_ctrl);

            if (round(elapsed_msec % td_gyro) == 0) {
                //                                  SENSORS
                // 10 Hz for gyro update
                // Update gyro sensor

                cmd_t *cmd_get_omega = cmd_get_str("get_obc_omega");
                cmd_add_params_str(cmd_get_omega, NULL);
                cmd_send(cmd_get_omega);
                osDelay(30);  // TODO sync. command execution.
                _get_sat_vector(&current_omega_b, dat_ads_omega_x);
                LOGI(tag, "Angular velocity: (%f, %f, %f) [@bodyframe]", current_omega_b.v0, current_omega_b.v1,
                     current_omega_b.v2)
                double dt = (double) elapsed_msec * 0.001;
                // eskf_predict_state((double*)P, dt);

                if (round(elapsed_msec % td_quat) == 0) {
                    // 2 Hz update for Css, mtm, and fss
                    //                                  MODELS

                    calc_adcs_model_parameters(elapsed_msec, &sat_pos_i, &geod_vect,
                                               &current_mag_i, &isDark, &sun_pos_i);

                    //                                  SENSORS
                    // Update magnetic from sensors - Body frame
                    //current_mag_b = test_transform_ypr(current_mag_i, yaw_rate * elapsed_msec * 0.001,
                    //                                             pitch_rate * elapsed_msec * 0.001,
                    //                                             roll_rate * elapsed_msec * 0.001);
                    cmd_t *cmd_get_mag = cmd_get_str("get_obc_mag");
                    cmd_add_params_str(cmd_get_mag, NULL);
                    cmd_send(cmd_get_mag);
                    osDelay(50);  // TODO sync. command execution
                    _get_sat_vector(&current_mag_b, dat_ads_mag_x);
                    LOGI(tag, "Magnetic field: (%f, %f, %f) [@bodyframe]", current_mag_b.v0, current_mag_b.v1,
                         current_mag_b.v2);

                    // Update sun information from sensors - Body frame
                    cmd_t *cmd_get_sun_b = cmd_get_str("get_obc_sun_vec");
                    cmd_send(cmd_get_sun_b);
                    osDelay(1500);

                    // = test_transform_ypr(sun_pos_i, yaw_rate * dt, pitch_rate * dt, roll_rate * dt);
                    _get_sat_vector(&sun_dir_b, dat_sun_vec_b_x);

                    LOGI(tag, "Sun direction: (%f, %f, %f) [@bodyframe]", sun_dir_b.v0, sun_dir_b.v1, sun_dir_b.v2);

                    //                      DETERMINATION AND ESTIMATION
                    vector3_t sun_pos_from_sc_i;
                    vector3_t temp_sc;
                    vec_cons_mult(-1.0, &sat_pos_i, &temp_sc);
                    vec_sum(sun_pos_i, temp_sc, &sun_pos_from_sc_i);
                    double sun_norm = vec_norm(sun_pos_from_sc_i);
                    double sun_norm_sensor = vec_norm(sun_dir_b);
                    if_sun_info = isDark == 0 && sun_norm != 0.0 && sun_norm_sensor != 0.0;
                    LOGI(tag, "if_sun_info: %d \n", if_sun_info);
                    if (if_sun_info) {
                        LOGI(tag, "Calculating quaternion ...");
                        determine_quaternion_triadekf(isDark, current_mag_b, current_mag_i, sun_dir_b,
                                                      sun_pos_from_sc_i, &current_q_det);
                        LOGI(tag, "Quaternion i2b determined: (%f, %f, %f, %f)", current_q_det.q0, current_q_det.q1,
                             current_q_det.q2, current_q_det.q3);
                        _set_sat_quaterion(&current_q_det, dat_ads_q0);
                        //_set_sat_quaterion(&current_q_est_i2b, dat_ads_ekf_q0);
                        osDelay(30);
                    }
                    else{
                        LOGI(tag, "Calculating quaternion ...");
                        determine_quaternion_by_mtt(current_mag_b, current_mag_i, &current_q_det);
                        LOGI(tag, "Quaternion i2b determined: (%f, %f, %f, %f)", current_q_det.q0, current_q_det.q1,
                            current_q_det.q2, current_q_det.q3);
                        _set_sat_quaterion(&current_q_det, dat_ads_q0);
                        //_set_sat_quaterion(&current_q_est_i2b, dat_ads_ekf_q0);
                        osDelay(30);
                    }
                }
                /**
                if (act_ekf) {
                    if (elapsed_msec == 0) {
                        first_adcs_data(current_omega_b, current_q_det);
                    } else {
                        _get_sat_quaterion(&current_q_det, dat_ads_q0);
                        estimate_omega_quat_with_triadekf(current_q_det, current_omega_b, delay_ms, &current_q_est_i2b,
                                                          &current_omega_b_est);
                        _set_sat_quaterion(&current_q_est_i2b, dat_ads_ekf_q0);
                        _set_sat_quaterion(&current_q_det, dat_ads_q0);
                        _set_sat_vector(&current_omega_b_est, dat_ads_ekf_omega_x);
                    }
                } else {
                    current_omega_b_est.v[0] = 0;
                    current_omega_b_est.v[1] = 0;
                    current_omega_b_est.v[2] = 0;
                    current_q_est_i2b.q[0] = 0;
                    current_q_est_i2b.q[1] = 0;
                    current_q_est_i2b.q[2] = 0;
                    current_q_est_i2b.q[3] = 1;
                }**/

                /* Save ADCS data */
                int curr_time = dat_get_time();
                int index_ads = dat_get_system_var(data_map[ekf_sensors].sys_index);
                ekf_data_t data_ads_ekf = {index_ads, curr_time, current_omega_b.v0,
                                           current_omega_b.v1, current_omega_b.v2, current_mag_b.v0,
                                           current_mag_b.v1, current_mag_b.v2,current_q_det.q0,
                                           current_q_det.q1, current_q_det.q2,
                                           current_q_det.q3};
                dat_add_payload_sample(&data_ads_ekf, ekf_sensors);
            }

            /* 1 second actions */
            dat_set_system_var(dat_rtc_date_time, (int) time(NULL));

            /**
             * Control LOOP
             */
            if (act_ctrl){
                if (((elapsed_msec + delay_ms) % (_adcs_ctrl_period + delay_ms)) == 0) {
                    // Set target attitude
                    //cmd_t *cmd_point = cmd_get_str("sim_adcs_set_target");
                    //cmd_add_params_var(cmd_point, 1.0, 1.0, 1.0, 0.01, 0.01, 0.01);
                    int mode;
                    mode = dat_get_system_var(dat_obc_opmode);
                    cmd_t *cmd_point;
                    if (mode == DAT_OBC_OPMODE_REF_POINT) {
                        cmd_point = cmd_get_str("adcs_set_target");
                        cmd_add_params_var(cmd_point, 1.0, 1.0, 1.0, 0.01, 0.01, 0.01);
                    } else if (mode == DAT_OBC_OPMODE_NAD_POINT) {
                        cmd_point = cmd_get_str("adcs_set_to_nadir");
                    } else if (mode == DAT_OBC_OPMODE_DETUMB_MAG) {
                        cmd_point = cmd_get_str("adcs_detumbling_mag");
                    }
                    cmd_send(cmd_point);
                    // Do control loop
                    cmd_t *cmd_ctrl;
                    if (mode == DAT_OBC_OPMODE_DETUMB_MAG) {
                        cmd_ctrl = cmd_get_str("adcs_mag_moment");
                    } else {
                        cmd_ctrl = cmd_get_str("adcs_do_control");
                        cmd_add_params_var(cmd_ctrl, _adcs_ctrl_period * 1000);
                    }
                    cmd_send(cmd_ctrl);
                    // Send telemetry to ADCS subsystem
                    cmd_t *cmd_att = cmd_get_str("adcs_send_attitude");
                    cmd_send(cmd_att);
                }
            }
            /* 1 hours actions */
            if((elapsed_msec % _1hour_check) == 0)
            {
                LOGD(tag, "1 hour check");
                cmd_t *cmd_1h = cmd_get_str("drp_add_hrs_alive");
                cmd_add_params_var(cmd_1h, 1); // Add 1hr
                cmd_send(cmd_1h);
            }
            LOGI(tag, "Condition for save: %u, time: %d \n", if_sun_info, elapsed_msec);
            // Update step of time
            portTick last_ticks = xLastWakeTime;
            osTaskDelayUntil(&xLastWakeTime, delay_ms); //Suspend task
            //LOGI(tag, "Delta time: %lu \n", xLastWakeTime - last_ticks);
            elapsed_msec += delay_ms;
            value32_t lapse_attitude;
            lapse_attitude.i = dat_get_system_var(dat_time_to_attitude);
            if (elapsed_msec > lapse_attitude.i * 1000){
                dat_set_system_var(dat_calc_attitude, 0);
                elapsed_msec = 0;
            }
        }else{
            osDelay(5000);
        }
    }
}

void calc_adcs_model_parameters(unsigned int elapsed_msec, vector3_t * sat_pos_i, vector3_t * geod_vect,
                                vector3_t * current_mag_i, int * isdark, vector3_t  * sun_pos_i){
    // GET Time scales
    uint32_t curr_time = (uint32_t) time(NULL);
    double current_jd = unixt_to_jd(curr_time);
    double dec_year = jd_to_dec(current_jd);
    LOGI(tag, "Julian date: %f, Dec year: %f", current_jd, dec_year);

    // update position from TLE
    cmd_t *cmd_tle_prop = cmd_get_str("tle_prop");
    cmd_add_params_str(cmd_tle_prop, "0");
    cmd_send(cmd_tle_prop);
    osDelay(50);
    _get_sat_vector(sat_pos_i, dat_ads_pos_x);
    LOGI(tag, "Satellite position [km] : (%.8f, %.8f, %.8f)", sat_pos_i->v[0], sat_pos_i->v[1], sat_pos_i->v[2]);

    // Update geodetic coordinate
    double current_sideral = gstime(current_jd);;
    eci_to_geodetic(*sat_pos_i, current_sideral, geod_vect);
    LOGI(tag, "Geodetic coordinate (lat-lon-alt): (%f, %f, %f), GMST: %f", geod_vect->v0, geod_vect->v1, geod_vect->v2,
         current_sideral);

    // GET magnetic from model - Inertial frame
    // for test
    //geod_vect->v[0] = 29.1898* deg2rad;
    //geod_vect->v[1] = -81.048 * deg2rad;
    //geod_vect->v[2] = 4 *1e-3;
    //current_sideral = 45 * deg2rad;
    //dec_year = 2022.5;
    calc_magnetic_model(dec_year, geod_vect->v[0], geod_vect->v[1], geod_vect->v[2],
                        current_sideral, current_mag_i);
    //LOGI(tag, "Magnetic field: (%f, %f, %f) [ECI reference frame]", current_mag_i->v[0], current_mag_i->v[1],
    //     current_mag_i->v[2]);

    // Calculate sun direction - Inertial frame
    calc_sun_pos_i(current_jd, sun_pos_i);
    LOGI(tag, "sun position [ECI]: (%f, %f, %f)", sun_pos_i->v0, sun_pos_i->v1, sun_pos_i->v2);

    // Shadow zone
    *isdark = calc_shadow_zone(*sun_pos_i, *sat_pos_i);
    LOGI(tag, "Is dark?: %i", isdark[0]);
}

void eskf_predict_state(double P[6][6], double dt)
{
    LOGD(tag, "Kalman Estimate")
    // Predict Nominal
    quaternion_t q;
//    vector3_t w = {0.1002623,  -0.10142402,  0.20332787};
    vector3_t w;
    vector3_t wb = {0.0, 0.0, 0.0};
    vector3_t diffw;
    _get_sat_quaterion(&q, dat_ads_q0);
    _get_sat_vector(&w, dat_ads_omega_x);

    quaternion_t q_est;
    vec_cons_mult(-1.0, &wb, NULL);
    vec_sum(w, wb, &diffw);
    eskf_integrate(q, diffw, dt, &q_est);
    _set_sat_quaterion(&q_est, dat_ads_q0);
//    _set_sat_vector(&w, dat_ads_omega_x);

    // Predict Error
    double Q[6][6];
    _mat_set_diag((double *) Q, 1.0, 6, 6);
    eskf_compute_error(diffw, dt, P, Q);
}

void calc_magnetic_model(double decyear, double latrad, double lonrad, double altm, double current_sideral_, vector3_t * mag) {
    IgrfCalc(decyear, latrad, lonrad, altm * 1000, current_sideral_, mag);
}

void calc_sun_pos_i(double jd, vector3_t * sun_dir) {

    // all in degree
//    jd = 2459346.3905324074;
    double au = 149597870.691e3;
    double n = jd - 2451545.0;
    double l = (280.459 + 0.98564736 * n); //% 360
    double m = (357.529 + 0.98560023 * n); //% 360.0
    m *= deg2rad;
    double lam = (l + 1.915 * sin(m) + 0.0200 * sin(2 * m)); // % 360.0
    lam *= deg2rad;
    double e = 23.439 - 3.56e-7 * n;
    e *= deg2rad;

    double r_sun = (1.00014 - 0.01671 * cos(m) - 0.000140 * cos(2 * m)) * au;

    sun_dir->v0 = cos(lam);
    sun_dir->v1 =  cos(e) * sin(lam);
    sun_dir->v2 = sin(lam) * sin(e);
    vec_cons_mult(r_sun, sun_dir, NULL);
}

double fmod2p(double x)
{
    /* Returns mod 2PI of argument */

    int i;
    double ret_val;

    ret_val=x;
    i= (int)trunc(ret_val/twopi);
    ret_val-=i*twopi;

    if (ret_val<0.0)
        ret_val+=twopi;

    return ret_val;
}

double jd_to_dec(double jd)
{
    double decyear;
    int leapyrs, year;
    double    days, tu, temp;

    /* --------------- find year and days of the year --------------- */
    temp    = jd - 2415019.5;
    tu      = temp / 365.25;
    year    = 1900 + (int)floor(tu);
    leapyrs = (int)floor((year - 1901) * 0.25);

    // optional nudge by 8.64x10-7 sec to get even outputs
    days    = temp - ((year - 1900) * 365.0 + leapyrs) + 0.00000000001;

    /* ------------ check for case of beginning of a year ----------- */
    if (days < 1.0)
    {
        year    = year - 1;
        leapyrs = (int)floor((year - 1901) * 0.25);
        days    = temp - ((year - 1900) * 365.0 + leapyrs);
    }

    decyear = year + days/365.25;
    return decyear;
}


int eci_to_geodetic(vector3_t sat_pos, double current_side, vector3_t * lat_lon_alt) {
    double radiusearthkm = 6378.137;     // km
    double f = 1.0 / 298.257223563;

    double r, e2, phi, c;
    double theta;

    theta = atan2(sat_pos.v1, sat_pos.v0);


    double lon_rad_ = fmod2p(theta - current_side); /* radians */
    r = sqrt(sat_pos.v0 * sat_pos.v0 + sat_pos.v1 * sat_pos.v1);
    e2 = f*(2 - f);
    double lat_rad_ = atan2(sat_pos.v2, r); /* radians */
    do
    {
        phi = lat_rad_;
        c = 1 / sqrt(1 - e2*sin(phi)*sin(phi));
        lat_rad_ = atan2(sat_pos.v2 + radiusearthkm*c*e2*sin(phi), r);

    } while (fabs(lat_rad_ - phi) >= 1E-10);
    double alt_m_ = r / cos(lat_rad_) - radiusearthkm*c; /* kilometers */

    if (lat_rad_ > pi/2.0)
        lat_rad_ -= twopi;

    lat_lon_alt->v0 = lat_rad_;
    lat_lon_alt->v1 = lon_rad_;
    lat_lon_alt->v2 = alt_m_;
}


double unixt_to_jd(uint32_t unix_time) {
    return ( unix_time / 86400.0 ) + 2440587.5;
}