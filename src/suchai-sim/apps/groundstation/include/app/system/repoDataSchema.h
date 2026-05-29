/**
 * @file  dataSchema.h
 * @author Camilo Rojas M - camrojas@uchile.cl
 * @date 2020
 * @copyright GNU GPL v3
 *
 * This header contains structs symbolizing the schema of data.
 */


#ifndef REPO_DATA_SCHEMA_H
#define REPO_DATA_SCHEMA_H

#include "suchai/log_utils.h"
#include "suchai/storage.h"
#include "app/system/config.h"

#include "app/payloads/repoDataSchemaSTT.h"
#include "app/payloads/repoDataSchemaGPS.h"
#include "app/payloads/repoDataSchemaMAG.h"
#include "app/payloads/repoDataSchemaGRA.h"

/**
 * Enum constants for dynamically identifying system status fields at execution time.
 *
 * Also permits adding new status variables cheaply, by generalizing both the
 * dat_set_system_var and dat_get_system_var functions.
 *
 * The dat_status_last_address constant serves only for comparison when looping through all
 * system status values. For example:
 *
 * @code
 * for (dat_status_address_t i = 0; i < dat_status_last_address; i++)
 * {
 * // some code using i.
 * }
 * @endcode
 *
 * @see dat_status_t
 * @seealso dat_set_system_var
 * @seealso dat_get_system_var
 */
typedef enum dat_status_address_enum {
    /// OBC: On board computer related variables.
    dat_obc_opmode = 0,           ///< General operation mode
    dat_obc_last_reset,           ///< Last reset source
    dat_obc_hrs_alive,            ///< Hours since first boot
    dat_obc_hrs_wo_reset,         ///< Hours since last reset
    dat_obc_reset_counter,        ///< Number of reset since first boot
    dat_obc_sw_wdt,               ///< Software watchdog timer counter
    dat_obc_temp_1,               ///< Temperature value of the first sensor
    dat_obc_executed_cmds,        ///< Total number of executed commands
    dat_obc_failed_cmds,          ///< Total number of failed commands

    /// DEP: Deployment related variables.
    dat_dep_deployed,             ///< Was the satellite deployed?
    dat_dep_ant_deployed,         ///< Was the antenna deployed?
    dat_dep_date_time,            ///< Antenna deployment unix time

    /// RTC: Rtc related variables
    dat_rtc_date_time,            ///< RTC current unix time

    /// COM: Communications system variables.
    dat_com_count_tm,             ///< Number of Telemetries sent
    dat_com_count_tc,             ///< Number of received Telecommands
    dat_com_last_tc,              ///< Unix time of the last received Telecommand
    dat_com_freq,                 ///< Communications frequency [Hz]
    dat_com_tx_pwr,               ///< TX power (0: 25dBm, 1: 27dBm, 2: 28dBm, 3: 30dBm)
    dat_com_baud,                 ///< Baudrate [bps]
    dat_com_mode,                 ///< Framing mode (1: RAW, 2: ASM, 3: HDLC, 4: Viterbi, 5: GOLAY, 6: AX25)
    dat_com_bcn_period,           ///< Number of seconds between trx beacon packets
    dat_obc_bcn_offset,           ///< Number of seconds between obc beacon packets

    /// FPL: Flight plan related variables
    dat_fpl_last,                 ///< Last executed flight plan (unix time)
    dat_fpl_queue,                ///< Flight plan queue length

    /// ADS: Attitude determination system
    dat_ads_omega_x,              ///< Gyroscope acceleration value along the x axis
    dat_ads_omega_y,              ///< Gyroscope acceleration value along the y axis
    dat_ads_omega_z,              ///< Gyroscope acceleration value along the z axis
    dat_ads_bias_x,
    dat_ads_bias_y,
    dat_ads_bias_z,
//    dat_ads_ekf_omega_x,              ///< Gyroscope acceleration value along the x axis
//    dat_ads_ekf_omega_y,              ///< Gyroscope acceleration value along the y axis
//    dat_ads_ekf_omega_z,              ///< Gyroscope acceleration value along the z axis
//    dat_ads_ekf_bias_x,              ///< bias Gyroscope acceleration value along the x axis
//    dat_ads_ekf_bias_y,              ///< bias Gyroscope acceleration value along the y axis
//    dat_ads_ekf_bias_z,              ///< bias Gyroscope acceleration value along the z axis
    dat_tgt_omega_x,              ///< Target acceleration value along the x axis
    dat_tgt_omega_y,              ///< Target acceleration value along the y axis
    dat_tgt_omega_z,              ///< Target acceleration value along the z axis
    dat_ads_mag_x,                ///< Magnetometer value along the x axis
    dat_ads_mag_y,                ///< Magnetometer value along the y axis
    dat_ads_mag_z,                ///< Magnetometer value along the z axis
    dat_ads_pos_x,                ///< Satellite orbit position x (ECI)
    dat_ads_pos_y,                ///< Satellite orbit position y (ECI)
    dat_ads_pos_z,                ///< Satellite orbit position z (ECI)
    dat_ads_vel_x,                ///< Satellite orbit velocity x (ECI)
    dat_ads_vel_y,                ///< Satellite orbit velocity y (ECI)
    dat_ads_vel_z,                ///< Satellite orbit velocity z (ECI)
    dat_ads_tle_epoch,            ///< Current TLE epoch, 0 if TLE is invalid
    dat_ads_tle_last,             ///< Las time position was propagated
    dat_ads_q0,                   ///< Attitude quaternion (Inertial to body)
    dat_ads_q1,                   ///< Attitude quaternion (Inertial to body)
    dat_ads_q2,                   ///< Attitude quaternion (Inertial to body)
    dat_ads_q3,                   ///< Attitude quaternion (Inertial to body)
//    dat_ads_ekf_q0,                   ///< Attitude quaternion (Inertial to body)
//    dat_ads_ekf_q1,                   ///< Attitude quaternion (Inertial to body)
//    dat_ads_ekf_q2,                   ///< Attitude quaternion (Inertial to body)
//    dat_ads_ekf_q3,                   ///< Attitude quaternion (Inertial to body)
    dat_tgt_q0,                   ///< Target quaternion (Inertial to body)
    dat_tgt_q1,                   ///< Target quaternion (Inertial to body)
    dat_tgt_q2,                   ///< Target quaternion (Inertial to body)
    dat_tgt_q3,                   ///< Target quaternion (Inertial to body)
//    //dat_css_1,                ///< Coarse sun sensor value
//    dat_css_2,                ///< Coarse sun sensor value
//    dat_css_3,                ///< Coarse sun sensor value
//    dat_css_4,                ///< Coarse sun sensor value
//    //dat_css_5,                ///< Coarse sun sensor value
//    dat_sun_vec_b_x,
//    dat_sun_vec_b_y,
//    dat_sun_vec_b_z,
//    dat_calc_attitude,
//    dat_activate_ekf,
//    dat_activate_ctrl,
//    dat_time_to_attitude,
//    dat_inertia_xx,         ///< Inertia matrix xx
//    dat_inertia_yy,         ///< Inertia matrix yy
//    dat_inertia_zz,         ///< Inertia matrix zz
//    dat_inertia_xy,         ///< Inertia matrix xy
//    dat_inertia_xz,         ///< Inertia matrix xz
//    dat_inertia_yz,         ///< Inertia matrix yz
//    dat_inv_inertia_xx,         ///< Inertia matrix xx
//    dat_inv_inertia_yy,         ///< Inertia matrix yy
//    dat_inv_inertia_zz,         ///< Inertia matrix zz
//    dat_inv_inertia_xy,         ///< Inertia matrix xy
//    dat_inv_inertia_xz,         ///< Inertia matrix xz
//    dat_inv_inertia_yz,         ///< Inertia matrix yz
//    dat_inertia_rw,
//    dat_time_delay_gyro,
//    dat_time_delay_quat,
//    dat_mtq_x_axis,
//    dat_mtq_y_axis,
//    dat_mtq_z_axis,

    /// EPS: Energy power system
    dat_eps_vbatt,                ///< Voltage of the battery [mV]
    dat_eps_cur_sun,              ///< Current from boost converters [mA]
    dat_eps_cur_sys,              ///< Current from the battery [mA]
    dat_eps_temp_bat0,            ///< Battery temperature sensor

    /// Sample Machine: Current state of sample status_machine
    dat_drp_mach_action,          ///< Current action of sampling state machine
    dat_drp_mach_state,           ///< Current state of sampling state machine
    dat_drp_mach_step,            ///< Step in seconds of sampling state machine
    dat_drp_mach_payloads,        ///< Binary data storing active payload being sampled
    dat_drp_mach_left,            ///< Samples left for sampling state machine

    /// Memory: Current payload memory addresses
    dat_drp_idx_temp_2,             ///< Temperature data index
    dat_drp_idx_ads_2,              ///< ADS data index
    dat_drp_idx_eps_2,              ///< EPS data index
    dat_drp_idx_sta_2,              ///< Status data index
    dat_drp_idx_stt_2,              ///< Data from STT index
    dat_drp_idx_rw_2,               ///< Temperature data index
    dat_drp_idx_fss_2,          ///< FSS ads data index
    dat_drp_idx_ekf_2,
    dat_drp_idx_ctrl_2,
    dat_drp_idx_str_2,              ///< String data index

    dat_drp_idx_temp_3,             ///< Temperature data index
    dat_drp_idx_ads_3,              ///< ADS data index
    dat_drp_idx_eps_3,              ///< EPS data index
    dat_drp_idx_sta_3,              ///< Status data index
    dat_drp_idx_stt_3,              ///< Data from STT index
    dat_drp_idx_rw_3,               ///< Temperature data index
    dat_drp_idx_fss_3,          ///< FSS ads data index
    dat_drp_idx_ekf_3,
    dat_drp_idx_ctrl_3,
    dat_drp_idx_str_3,              ///< String data index

    dat_drp_idx_temp_P,             ///< Temperature data index
    dat_drp_idx_ads_P,              ///< ADS data index
    dat_drp_idx_eps_P,              ///< EPS data index
    dat_drp_idx_sta_P,              ///< Status data index
    dat_drp_idx_stt_P,              ///< Data from STT index
    dat_drp_idx_rw_P,               ///< Temperature data index
    dat_drp_idx_fss_P,          ///< FSS ads data index
    dat_drp_idx_ekf_P,
    dat_drp_idx_ctrl_P,
    dat_drp_idx_str_P,              ///< String data index

    /// Memory: Current send acknowledge data
    dat_drp_ack_temp_2,             ///< Temperature data acknowledge
    dat_drp_ack_ads_2,              ///< ADS data index acknowledge
    dat_drp_ack_eps_2,              ///< EPS data index acknowledge
    dat_drp_ack_sta_2,              ///< Status data index acknowledge
    dat_drp_ack_stt_2,              ///< Data from STT index acknowledge
    dat_drp_ack_rw_2,               ///< Temperature data acknowledge
    dat_drp_ack_fss_2,          ///< FSS ads acknowledge
    dat_drp_ack_ekf_2,              ///< ADS EKF data acknowledge
    dat_drp_ack_ctrl_2,             ///< ADS CTRL data acknowledge
    dat_drp_ack_str_2,              ///< String data acknowledge

    dat_drp_ack_temp_3,             ///< Temperature data acknowledge
    dat_drp_ack_ads_3,              ///< ADS data index acknowledge
    dat_drp_ack_eps_3,              ///< EPS data index acknowledge
    dat_drp_ack_sta_3,              ///< Status data index acknowledge
    dat_drp_ack_stt_3,              ///< Data from STT index acknowledge
    dat_drp_ack_rw_3,               ///< Temperature data acknowledge
    dat_drp_ack_fss_3,          ///< FSS ads acknowledge
    dat_drp_ack_ekf_3,              ///< ADS EKF data acknowledge
    dat_drp_ack_ctrl_3,             ///< ADS CTRL data acknowledge
    dat_drp_ack_str_3,              ///< String data acknowledge

    dat_drp_ack_temp_P,             ///< Temperature data acknowledge
    dat_drp_ack_ads_P,              ///< ADS data index acknowledge
    dat_drp_ack_eps_P,              ///< EPS data index acknowledge
    dat_drp_ack_sta_P,              ///< Status data index acknowledge
    dat_drp_ack_stt_P,              ///< Data from STT index acknowledge
    dat_drp_ack_rw_P,               ///< Temperature data acknowledge
    dat_drp_ack_fss_P,          ///< FSS ads acknowledge
    dat_drp_ack_ekf_P,              ///< ADS EKF data acknowledge
    dat_drp_ack_ctrl_P,             ///< ADS CTRL data acknowledge
    dat_drp_ack_str_P,              ///< String data acknowledge

    /// Payloads
    stt_dat_drp_idx_temp_2,
    stt_dat_drp_ack_temp_2,
    stt_dat_drp_idx_stt_2,
    stt_dat_drp_ack_stt_2,
    stt_dat_drp_idx_stt_exp_time_2,
    stt_dat_drp_ack_stt_exp_time_2,
    stt_dat_drp_idx_stt_gyro_2,
    stt_dat_drp_ack_stt_gyro_2,

    stt_dat_drp_idx_temp_3,
    stt_dat_drp_ack_temp_3,
    stt_dat_drp_idx_stt_3,
    stt_dat_drp_ack_stt_3,
    stt_dat_drp_idx_stt_exp_time_3,
    stt_dat_drp_ack_stt_exp_time_3,
    stt_dat_drp_idx_stt_gyro_3,
    stt_dat_drp_ack_stt_gyro_3,

    stt_dat_drp_idx_temp_P,
    stt_dat_drp_ack_temp_P,
    stt_dat_drp_idx_stt_P,
    stt_dat_drp_ack_stt_P,
    stt_dat_drp_idx_stt_exp_time_P,
    stt_dat_drp_ack_stt_exp_time_P,
    stt_dat_drp_idx_stt_gyro_P,
    stt_dat_drp_ack_stt_gyro_P,

    mag_dat_drp_idx_temp_2,
    mag_dat_drp_ack_temp_2,
    mag_dat_drp_idx_fod_2,
    mag_dat_drp_ack_fod_2,
    mag_dat_drp_idx_mag_2,
    mag_dat_drp_ack_mag_2,
    mag_dat_drp_idx_stt_2,
    mag_dat_drp_ack_stt_2,
    mag_dat_drp_idx_stt_exp_time_2,
    mag_dat_drp_ack_stt_exp_time_2,
    mag_dat_drp_idx_stt_gyro_2,
    mag_dat_drp_ack_stt_gyro_2,
    mag_dat_drp_idx_iot_2,
    mag_dat_drp_ack_iot_2,
    mag_dat_drp_idx_aoa_2,
    mag_dat_drp_ack_aoa_2,

    mag_dat_drp_idx_temp_3,
    mag_dat_drp_ack_temp_3,
    mag_dat_drp_idx_fod_3,
    mag_dat_drp_ack_fod_3,
    mag_dat_drp_idx_mag_3,
    mag_dat_drp_ack_mag_3,
    mag_dat_drp_idx_stt_3,
    mag_dat_drp_ack_stt_3,
    mag_dat_drp_idx_stt_exp_time_3,
    mag_dat_drp_ack_stt_exp_time_3,
    mag_dat_drp_idx_stt_gyro_3,
    mag_dat_drp_ack_stt_gyro_3,
    mag_dat_drp_idx_iot_3,
    mag_dat_drp_ack_iot_3,
    mag_dat_drp_idx_aoa_3,
    mag_dat_drp_ack_aoa_3,

    mag_dat_drp_idx_temp_P,
    mag_dat_drp_ack_temp_P,
    mag_dat_drp_idx_fod_P,
    mag_dat_drp_ack_fod_P,
    mag_dat_drp_idx_mag_P,
    mag_dat_drp_ack_mag_P,
    mag_dat_drp_idx_stt_P,
    mag_dat_drp_ack_stt_P,
    mag_dat_drp_idx_stt_exp_time_P,
    mag_dat_drp_ack_stt_exp_time_P,
    mag_dat_drp_idx_stt_gyro_P,
    mag_dat_drp_ack_stt_gyro_P,
    mag_dat_drp_idx_iot_P,
    mag_dat_drp_ack_iot_P,
    mag_dat_drp_idx_aoa_P,
    mag_dat_drp_ack_aoa_P,

    gra_dat_drp_idx_temp_P,
    gra_dat_drp_ack_temp_P,

    gps_dat_drp_idx_temp_2,
    gps_dat_drp_ack_temp_2,
    gps_dat_drp_idx_temp_3,
    gps_dat_drp_ack_temp_3,

    dat_drp_idx_lp_2,
    dat_drp_ack_lp_2,

    dat_drp_idx_lp_3,
    dat_drp_ack_lp_3,

    /// LAST ELEMENT: DO NOT EDIT
    dat_status_last_address           ///< Dummy element, the amount of status variables
} dat_status_address_t;

///< Define opeartion modes
#define DAT_OBC_OPMODE_DEPLOYING     (0) ///< Normal operation
#define DAT_OBC_OPMODE_NORMAL        (1) ///< Normal operation
#define DAT_OBC_OPMODE_WARN          (2) ///< Fail safe operation
#define DAT_OBC_OPMODE_FAIL          (3) ///< Generalized fail operation
#define DAT_OBC_OPMODE_REF_POINT     (4) ///< Point to vector
#define DAT_OBC_OPMODE_NAD_POINT     (5) ///< Point to nadir
#define DAT_OBC_OPMODE_DETUMB_MAG    (6) ///< Detumbling

///< Define is a variable is config or status
#define DAT_IS_CONFIG 0
#define DAT_IS_STATUS 1

/**
 * A system variable (status or config) with an address, name, type and value
 */
#define MAX_VAR_NAME 32

typedef struct __attribute__((packed)) dat_sys_var {
    uint16_t address;   ///< Variable address or index (in the data storage)
    char name[MAX_VAR_NAME];      ///< Variable name (max 24 chars)
    char type;          ///< Variable type (u: uint, i: int, f: float)
    int8_t status;      ///< Variable is status (1), is config (0), or uninitialized (-1)
    value32_t value;    ///< Variable default value
} dat_sys_var_t;

/**
 * A system variable (status or config) with an address, and value
 * A short version to be sent as telemetry
 */
typedef struct __attribute__((packed)) dat_sys_var_short {
    uint16_t address;   ///< Variable address or index (in the data storage)
    value32_t value;    ///< Variable default value
} dat_sys_var_short_t;

/**
 * List of status variables with address, name, type and default values
 * This list is useful to decide how to store and send the status variables
 */
static const dat_sys_var_t dat_status_list[] = {
        {dat_rtc_date_time,     "rtc_date_time",     'd', DAT_IS_CONFIG, 0},          ///< RTC current unix time
        {dat_obc_last_reset,    "obc_last_reset",    'u', DAT_IS_STATUS, 0},          ///< Last reset source
        {dat_obc_opmode,        "obc_opmode",        'd', DAT_IS_CONFIG, DAT_OBC_OPMODE_DEPLOYING}, ///< General operation mode
        {dat_obc_hrs_alive,     "obc_hrs_alive",     'u', DAT_IS_STATUS, 0},          ///< Hours since first boot
        {dat_obc_hrs_wo_reset,  "obc_hrs_wo_reset",  'u', DAT_IS_STATUS, 0},          ///< Hours since last reset
        {dat_obc_reset_counter, "obc_reset_counter", 'u', DAT_IS_STATUS, 0},          ///< Number of reset since first boot
        {dat_obc_sw_wdt,        "obc_sw_wdt",        'u', DAT_IS_STATUS, 0},          ///< Software watchdog timer counter
        {dat_obc_temp_1,        "obc_temp_1",        'f', DAT_IS_STATUS, -1},         ///< Temperature value of the first sensor
        {dat_obc_executed_cmds, "obc_executed_cmds", 'u', DAT_IS_STATUS, 0},          ///< Execute commands counter
        {dat_obc_failed_cmds,   "obc_failed_cmds",   'u', DAT_IS_STATUS, 0},          ///< Commands execute with errors counter
        {dat_dep_deployed,      "dep_deployed",      'u', DAT_IS_STATUS, 2},          ///< Was the satellite deployed?
        {dat_dep_ant_deployed,  "dep_ant_deployed",  'u', DAT_IS_STATUS, 1},          ///< Was the antenna deployed?
        {dat_dep_date_time,     "dep_date_time",     'u', DAT_IS_STATUS, 0},          ///< Antenna deployment unix time
        {dat_com_count_tm,      "com_count_tm",      'u', DAT_IS_STATUS, 0},          ///< Number of Telemetries sent
        {dat_com_count_tc,      "com_count_tc",      'u', DAT_IS_STATUS, 0},          ///< Number of received Telecommands
        {dat_com_last_tc,       "com_last_tc",       'd', DAT_IS_STATUS, 0},          ///< Unix time of the last received Telecommand
        {dat_com_freq,          "com_freq",          'u', DAT_IS_CONFIG, SCH_TX_FREQ},       ///< Communications frequency [Hz]
        {dat_com_tx_pwr,        "com_tx_pwr",        'u', DAT_IS_CONFIG, SCH_TX_PWR},        ///< TX power (0: 25dBm, 1: 27dBm, 2: 28dBm, 3: 30dBm)
        {dat_com_baud,          "com_baud",          'u', DAT_IS_CONFIG, SCH_TX_BAUD},       ///< Baudrate [bps]
        {dat_com_mode,          "com_mode",          'u', DAT_IS_CONFIG, 0},          ///< Framing mode (1: RAW, 2: ASM, 3: HDLC, 4: Viterbi, 5: GOLAY, 6: AX25)
        {dat_com_bcn_period,    "com_bcn_period",    'u', DAT_IS_CONFIG, SCH_TX_BCN_PERIOD},  ///< Number of seconds between trx beacon packets
        {dat_obc_bcn_offset,    "obc_bcn_offset",    'u', DAT_IS_CONFIG, SCH_OBC_BCN_OFFSET}, ///< Number of seconds between obc beacon packets
        {dat_fpl_last,          "fpl_last",          'd', DAT_IS_STATUS, 0},          ///< Last executed flight plan (unix time)
        {dat_fpl_queue,         "fpl_queue",         'u', DAT_IS_STATUS, 0},          ///< Flight plan queue length
        {dat_ads_omega_x,       "ads_omega_x",       'f', DAT_IS_STATUS, 0.0},         ///< Gyroscope acceleration value along the x axis
        {dat_ads_omega_y,       "ads_omega_y",       'f', DAT_IS_STATUS, 0.0},         ///< Gyroscope acceleration value along the y axis
        {dat_ads_omega_z,       "ads_omega_z",       'f', DAT_IS_STATUS, 0.0},         ///< Gyroscope acceleration value along the z axis
        {dat_ads_bias_x,       "ads_bias_x",       'f', DAT_IS_STATUS, 0.0},         ///< Gyroscope bias value along the x axis
        {dat_ads_bias_y,       "ads_bias_y",       'f', DAT_IS_STATUS, 0.0},         ///< Gyroscope bias value along the y axis
        {dat_ads_bias_z,       "ads_bias_z",       'f', DAT_IS_STATUS, 0.0},         ///< Gyroscope bias value along the z axis
        {dat_ads_mag_x,         "ads_mag_x",         'f', DAT_IS_STATUS, 0.0},         ///< Magnetometer value along the x axis
        {dat_ads_mag_y,         "ads_mag_y",         'f', DAT_IS_STATUS, 0.0},         ///< Magnetometer value along the y axis
        {dat_ads_mag_z,         "ads_mag_z",         'f', DAT_IS_STATUS, 0.0},         ///< Magnetometer value along the z axis
        {dat_ads_pos_x,         "ads_pos_x",         'f', DAT_IS_STATUS, 0.0},         ///< Satellite orbit position x (ECI)
        {dat_ads_pos_y,         "ads_pos_y",         'f', DAT_IS_STATUS, 0.0},         ///< Satellite orbit position y (ECI)
        {dat_ads_pos_z,         "ads_pos_z",         'f', DAT_IS_STATUS, 0.0},         ///< Satellite orbit position z (ECI)
        {dat_ads_vel_x,         "ads_pos_x",         'f', DAT_IS_STATUS, 0.0},         ///< Satellite orbit Velocity x (ECI)
        {dat_ads_vel_y,         "ads_pos_y",         'f', DAT_IS_STATUS, 0.0},         ///< Satellite orbit Velocity y (ECI)
        {dat_ads_vel_z,         "ads_pos_z",         'f', DAT_IS_STATUS, 0.0},         ///< Satellite orbit Velocity z (ECI)
        {dat_ads_tle_epoch,     "ads_tle_epoch",     'd', DAT_IS_STATUS, 0},          ///< Current TLE epoch, 0 if TLE is invalid
        {dat_ads_tle_last,      "ads_tle_last",      'u', DAT_IS_STATUS, 0},          ///< Last time position was propagated
        {dat_ads_q0,            "ads_q0",            'f', DAT_IS_STATUS, 0.0},          ///< Attitude quaternion (Inertial to body)
        {dat_ads_q1,            "ads_q1",            'f', DAT_IS_STATUS, 0.0},          ///< Attitude quaternion (Inertial to body)
        {dat_ads_q2,            "ads_q2",            'f', DAT_IS_STATUS, 0.0},          ///< Attitude quaternion (Inertial to body)
        {dat_ads_q3,            "ads_q3",            'f', DAT_IS_STATUS, 1.0},          ///< Attitude quaternion (Inertial to body)
        {dat_tgt_omega_x,       "tgt_omega_x",       'f', DAT_IS_CONFIG, 0.0},          ///< Target acceleration value along the x axis
        {dat_tgt_omega_y,       "tgt_omega_y",       'f', DAT_IS_CONFIG, 0.0},          ///< Target acceleration value along the y axis
        {dat_tgt_omega_z,       "tgt_omega_z",       'f', DAT_IS_CONFIG, 0.0},          ///< Target acceleration value along the z axis
        {dat_tgt_q0,            "tgt_q0",            'f', DAT_IS_CONFIG, 0.0},          ///< Target quaternion (Inertial to body)
        {dat_tgt_q1,            "tgt_q1",            'f', DAT_IS_CONFIG, 0.0},          ///< Target quaternion (Inertial to body)
        {dat_tgt_q2,            "tgt_q2",            'f', DAT_IS_CONFIG, 0.0},          ///< Target quaternion (Inertial to body)
        {dat_tgt_q3,            "tgt_q3",            'f', DAT_IS_CONFIG, 1.0},          ///< Target quaternion (Inertial to body)
        {dat_eps_vbatt,       "eps_vbatt",       'u', DAT_IS_STATUS, 0},          ///< Voltage of the battery [mV]
        {dat_eps_cur_sun,     "eps_cur_sun",     'u', DAT_IS_STATUS, 0},          ///< Current from boost converters [mA]
        {dat_eps_cur_sys,     "eps_cur_sys",     'u', DAT_IS_STATUS, 0},          ///< Current from the battery [mA]
        {dat_eps_temp_bat0,   "eps_temp_bat0",   'd', DAT_IS_STATUS, 0},          ///< Battery temperature sensor
        {dat_drp_mach_action,   "drp_mach_action",   'u', DAT_IS_STATUS, 0},          ///<
        {dat_drp_mach_state,    "drp_mach_state",    'u', DAT_IS_STATUS, 0},          ///<
        {dat_drp_mach_left,     "drp_mach_left",     'u', DAT_IS_STATUS, 0},          ///<
        {dat_drp_mach_step,     "drp_mach_step",     'd', DAT_IS_CONFIG, 0},          ///<
        {dat_drp_mach_payloads, "drp_mach_payloads", 'u', DAT_IS_CONFIG, 0},          ///<

        {dat_drp_idx_temp_2,      "drp_temp_2",          'u', DAT_IS_STATUS, 0},          ///< Temperature data index
        {dat_drp_idx_ads_2,       "drp_ads_2",           'u', DAT_IS_STATUS, 0},          ///< ADS data index
        {dat_drp_idx_eps_2,       "drp_eps_2",           'u', DAT_IS_STATUS, 0},          ///< EPS data index
        {dat_drp_idx_sta_2,       "drp_sta_2",           'u', DAT_IS_STATUS, 0},          ///< Status data index
        {dat_drp_idx_stt_2,       "drp_stt_2",           'u', DAT_IS_STATUS, 0},          ///< STT data index
        {dat_drp_idx_rw_2,        "drp_idx_rw_2",        'u', DAT_IS_STATUS, 0},          ///< RW data index
        {dat_drp_idx_fss_2,   "drp_idx_fss_2",           'u', DAT_IS_STATUS, 0},          ///< ADS data index
        {dat_drp_idx_ekf_2,       "drp_idx_ekf_2",       'u', DAT_IS_STATUS, 0},          ///< ADS ekf data index
        {dat_drp_idx_ctrl_2,      "drp_idx_ctrl_2", 'u', DAT_IS_STATUS, 0},
        {dat_drp_idx_str_2,       "drp_idx_str_2",       'u', DAT_IS_STATUS, 0},          ///< String data index
        {dat_drp_ack_temp_2,      "drp_ack_temp_2",      'u', DAT_IS_CONFIG, 0},          ///< Temperature data acknowledge
        {dat_drp_ack_ads_2,       "drp_ack_ads_2",       'u', DAT_IS_CONFIG, 0},          ///< ADS data index acknowledge
        {dat_drp_ack_eps_2,       "drp_ack_eps_2",       'u', DAT_IS_CONFIG, 0},          ///< EPS data index acknowledge
        {dat_drp_ack_sta_2,       "drp_ack_sta_2",       'u', DAT_IS_CONFIG, 0},          ///< Status data index acknowledge
        {dat_drp_ack_stt_2,       "drp_ack_stt_2",       'u', DAT_IS_CONFIG, 0},          ///< Stt data index acknowledge
        {dat_drp_ack_rw_2,        "drp_ack_rw_2",        'u', DAT_IS_CONFIG, 0},          ///< RW data acknowledge
        {dat_drp_ack_fss_2,   "drp_ack_fss_2",   'u', DAT_IS_CONFIG, 0},          ///< ADS FSS data index acknowledge
        {dat_drp_ack_fss_2,       "drp_ack_fss_2",   'u', DAT_IS_CONFIG, 0},          ///< ADS FSS data index acknowledge
        {dat_drp_ack_ekf_2,       "drp_ack_ekf_2",   'u', DAT_IS_CONFIG, 0},          ///< ADS EKF data index acknowledge
        {dat_drp_ack_ctrl_2, "drp_ack_ctrl_2", 'u', DAT_IS_CONFIG, 0},
        {dat_drp_ack_str_2,       "drp_ack_str_2",       'u', DAT_IS_CONFIG, 0},          ///< String data acknowledge

        {dat_drp_idx_temp_3,      "drp_temp_3",          'u', DAT_IS_STATUS, 0},          ///< Temperature data index
        {dat_drp_idx_ads_3,       "drp_ads_3",           'u', DAT_IS_STATUS, 0},          ///< ADS data index
        {dat_drp_idx_eps_3,       "drp_eps_3",           'u', DAT_IS_STATUS, 0},          ///< EPS data index
        {dat_drp_idx_sta_3,       "drp_sta_3",           'u', DAT_IS_STATUS, 0},          ///< Status data index
        {dat_drp_idx_stt_3,       "drp_stt_3",           'u', DAT_IS_STATUS, 0},          ///< STT data index
        {dat_drp_idx_rw_3,        "drp_idx_rw_3",        'u', DAT_IS_STATUS, 0},          ///< RW data index
        {dat_drp_idx_fss_3,   "drp_idx_fss_3",           'u', DAT_IS_STATUS, 0},          ///< ADS data index
        {dat_drp_idx_ekf_3,       "drp_idx_ekf_3",       'u', DAT_IS_STATUS, 0},          ///< ADS ekf data index
        {dat_drp_idx_ctrl_3,      "drp_idx_ctrl_3", 'u', DAT_IS_STATUS, 0},
        {dat_drp_idx_str_3,       "drp_idx_str_3",       'u', DAT_IS_STATUS, 0},          ///< String data index
        {dat_drp_ack_temp_3,      "drp_ack_temp_3",      'u', DAT_IS_CONFIG, 0},          ///< Temperature data acknowledge
        {dat_drp_ack_ads_3,       "drp_ack_ads_3",       'u', DAT_IS_CONFIG, 0},          ///< ADS data index acknowledge
        {dat_drp_ack_eps_3,       "drp_ack_eps_3",       'u', DAT_IS_CONFIG, 0},          ///< EPS data index acknowledge
        {dat_drp_ack_sta_3,       "drp_ack_sta_3",       'u', DAT_IS_CONFIG, 0},          ///< Status data index acknowledge
        {dat_drp_ack_stt_3,       "drp_ack_stt_3",       'u', DAT_IS_CONFIG, 0},          ///< Stt data index acknowledge
        {dat_drp_ack_rw_3,        "drp_ack_rw_3",        'u', DAT_IS_CONFIG, 0},          ///< RW data acknowledge
        {dat_drp_ack_fss_3,   "drp_ack_fss_3",   'u', DAT_IS_CONFIG, 0},          ///< ADS FSS data index acknowledge
        {dat_drp_ack_ekf_3,       "drp_ack_ekf_3",   'u', DAT_IS_CONFIG, 0},          ///< ADS EKF data index acknowledge
        {dat_drp_ack_ctrl_3, "drp_ack_ctrl_3", 'u', DAT_IS_CONFIG, 0},
        {dat_drp_ack_str_3,       "drp_ack_str_3",       'u', DAT_IS_CONFIG, 0},          ///< String data acknowledge

        {dat_drp_idx_temp_P,      "drp_temp_P",          'u', DAT_IS_STATUS, 0},          ///< Temperature data index
        {dat_drp_idx_ads_P,       "drp_ads_P",           'u', DAT_IS_STATUS, 0},          ///< ADS data index
        {dat_drp_idx_eps_P,       "drp_eps_P",           'u', DAT_IS_STATUS, 0},          ///< EPS data index
        {dat_drp_idx_sta_P,       "drp_sta_P",           'u', DAT_IS_STATUS, 0},          ///< Status data index
        {dat_drp_idx_stt_P,       "drp_stt_P",           'u', DAT_IS_STATUS, 0},          ///< STT data index
        {dat_drp_idx_rw_P,        "drp_idx_rw_P",        'u', DAT_IS_STATUS, 0},          ///< RW data index
        {dat_drp_idx_fss_P,   "drp_ads_fss_P",           'u', DAT_IS_STATUS, 0},          ///< ADS data index
        {dat_drp_idx_ekf_P,       "drp_idx_ekf_P",       'u', DAT_IS_STATUS, 0},          ///< ADS ekf data index
        {dat_drp_idx_ctrl_P,      "drp_idx_ctrl_P", 'u', DAT_IS_STATUS, 0},
        {dat_drp_idx_str_P,       "drp_idx_str_P",       'u', DAT_IS_STATUS, 0},          ///< String data index
        {dat_drp_ack_temp_P,      "drp_ack_temp_P",      'u', DAT_IS_CONFIG, 0},          ///< Temperature data acknowledge
        {dat_drp_ack_ads_P,       "drp_ack_ads_P",       'u', DAT_IS_CONFIG, 0},          ///< ADS data index acknowledge
        {dat_drp_ack_eps_P,       "drp_ack_eps_P",       'u', DAT_IS_CONFIG, 0},          ///< EPS data index acknowledge
        {dat_drp_ack_sta_P,       "drp_ack_sta_P",       'u', DAT_IS_CONFIG, 0},          ///< Status data index acknowledge
        {dat_drp_ack_stt_P,       "drp_ack_stt_P",       'u', DAT_IS_CONFIG, 0},          ///< Stt data index acknowledge
        {dat_drp_ack_rw_P,        "drp_ack_rw_P",        'u', DAT_IS_CONFIG, 0},          ///< RW data acknowledge
        {dat_drp_ack_fss_P,   "drp_ack_fss_P",   'u', DAT_IS_CONFIG, 0},          ///< ADS FSS data index acknowledge
        {dat_drp_ack_ekf_P,       "drp_ack_ekf_P",   'u', DAT_IS_CONFIG, 0},          ///< ADS EKF data index acknowledge
        {dat_drp_ack_ctrl_P, "drp_ack_ctrl_P", 'u', DAT_IS_CONFIG, 0},
        {dat_drp_ack_str_P,       "drp_ack_str_P",       'u', DAT_IS_CONFIG, 0},          ///< String data acknowledge

        {stt_dat_drp_idx_temp_2, "stt_dat_drp_idx_temp_2", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_ack_temp_2, "stt_dat_drp_ack_temp_2", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_idx_stt_2, "stt_dat_drp_idx_stt_2", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_ack_stt_2, "stt_dat_drp_ack_stt_2", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_idx_stt_exp_time_2, "stt_dat_drp_idx_stt_exp_time_2", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_ack_stt_exp_time_2, "stt_dat_drp_ack_stt_exp_time_2", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_idx_stt_gyro_2, "stt_dat_drp_idx_stt_gyro_2", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_ack_stt_gyro_2, "stt_dat_drp_ack_stt_gyro_2", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_idx_temp_3, "stt_dat_drp_idx_temp_3", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_ack_temp_3, "stt_dat_drp_ack_temp_3", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_idx_stt_3, "stt_dat_drp_idx_stt_3", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_ack_stt_3, "stt_dat_drp_ack_stt_3", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_idx_stt_exp_time_3, "stt_dat_drp_idx_stt_exp_time_3", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_ack_stt_exp_time_3, "stt_dat_drp_ack_stt_exp_time_3", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_idx_stt_gyro_3, "stt_dat_drp_idx_stt_gyro_3", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_ack_stt_gyro_3, "stt_dat_drp_ack_stt_gyro_3", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_idx_temp_P, "stt_dat_drp_idx_temp_P", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_ack_temp_P, "stt_dat_drp_ack_temp_P", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_idx_stt_P, "stt_dat_drp_idx_stt_P", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_ack_stt_P, "stt_dat_drp_ack_stt_P", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_idx_stt_exp_time_P, "stt_dat_drp_idx_stt_exp_time_P", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_ack_stt_exp_time_P, "stt_dat_drp_ack_stt_exp_time_P", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_idx_stt_gyro_P, "stt_dat_drp_idx_stt_gyro_P", 'u', DAT_IS_STATUS, 0},
        {stt_dat_drp_ack_stt_gyro_P, "stt_dat_drp_ack_stt_gyro_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_temp_2, "mag_dat_drp_idx_temp_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_temp_2, "mag_dat_drp_ack_temp_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_fod_2, "mag_dat_drp_idx_fod_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_fod_2, "mag_dat_drp_ack_fod_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_mag_2, "mag_dat_drp_idx_mag_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_mag_2, "mag_dat_drp_ack_mag_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_stt_2, "mag_dat_drp_idx_stt_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_stt_2, "mag_dat_drp_ack_stt_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_stt_exp_time_2, "mag_dat_drp_idx_stt_exp_time_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_stt_exp_time_2, "mag_dat_drp_ack_stt_exp_time_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_stt_gyro_2, "mag_dat_drp_idx_stt_gyro_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_stt_gyro_2, "mag_dat_drp_ack_stt_gyro_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_iot_2, "mag_dat_drp_idx_iot_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_iot_2, "mag_dat_drp_ack_iot_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_aoa_2, "mag_dat_drp_idx_aoa_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_aoa_2, "mag_dat_drp_ack_aoa_2", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_temp_3, "mag_dat_drp_idx_temp_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_temp_3, "mag_dat_drp_ack_temp_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_fod_3, "mag_dat_drp_idx_fod_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_fod_3, "mag_dat_drp_ack_fod_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_mag_3, "mag_dat_drp_idx_mag_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_mag_3, "mag_dat_drp_ack_mag_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_stt_3, "mag_dat_drp_idx_stt_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_stt_3, "mag_dat_drp_ack_stt_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_stt_exp_time_3, "mag_dat_drp_idx_stt_exp_time_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_stt_exp_time_3, "mag_dat_drp_ack_stt_exp_time_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_stt_gyro_3, "mag_dat_drp_idx_stt_gyro_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_stt_gyro_3, "mag_dat_drp_ack_stt_gyro_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_iot_3, "mag_dat_drp_idx_iot_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_iot_3, "mag_dat_drp_ack_iot_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_aoa_3, "mag_dat_drp_idx_aoa_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_aoa_3, "mag_dat_drp_ack_aoa_3", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_temp_P, "mag_dat_drp_idx_temp_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_temp_P, "mag_dat_drp_ack_temp_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_fod_P, "mag_dat_drp_idx_fod_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_fod_P, "mag_dat_drp_ack_fod_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_mag_P, "mag_dat_drp_idx_mag_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_mag_P, "mag_dat_drp_ack_mag_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_stt_P, "mag_dat_drp_idx_stt_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_stt_P, "mag_dat_drp_ack_stt_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_stt_exp_time_P, "mag_dat_drp_idx_stt_exp_time_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_stt_exp_time_P, "mag_dat_drp_ack_stt_exp_time_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_stt_gyro_P, "mag_dat_drp_idx_stt_gyro_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_stt_gyro_P, "mag_dat_drp_ack_stt_gyro_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_iot_P, "mag_dat_drp_idx_iot_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_iot_P, "mag_dat_drp_ack_iot_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_idx_aoa_P, "mag_dat_drp_idx_aoa_P", 'u', DAT_IS_STATUS, 0},
        {mag_dat_drp_ack_aoa_P, "mag_dat_drp_ack_aoa_P", 'u', DAT_IS_STATUS, 0},
        {gra_dat_drp_idx_temp_P, "gra_dat_drp_idx_temp_P", 'u', DAT_IS_STATUS, 0},
        {gra_dat_drp_ack_temp_P, "gra_dat_drp_ack_temp_P", 'u', DAT_IS_STATUS, 0},
        {gps_dat_drp_idx_temp_2, "gps_dat_drp_idx_temp_2", 'u', DAT_IS_STATUS, 0},
        {gps_dat_drp_ack_temp_2, "gps_dat_drp_ack_temp_2", 'u', DAT_IS_STATUS, 0},
        {gps_dat_drp_idx_temp_3, "gps_dat_drp_idx_temp_3", 'u', DAT_IS_STATUS, 0},
        {gps_dat_drp_ack_temp_3, "gps_dat_drp_ack_temp_3", 'u', DAT_IS_STATUS, 0},
        {dat_drp_idx_lp_2,       "drp_idx_lp_2",           'u', DAT_IS_STATUS, 0},
        {dat_drp_ack_lp_2,       "drp_ack_lp_2",           'u', DAT_IS_CONFIG, 0},
        {dat_drp_idx_lp_3,       "drp_idx_lp_3",           'u', DAT_IS_STATUS, 0},
        {dat_drp_ack_lp_3,       "drp_ack_lp_3",           'u', DAT_IS_CONFIG, 0},

};
///< The dat_status_last_var constant serves for looping through all status variables
static const int dat_status_last_var = sizeof(dat_status_list) / sizeof(dat_status_list[0]);

/**
 * Enum constants for dynamically identifying payload fields at execution time.
 *
 * Also permits adding payload fields cheaply.
 *
 * The last_sensor constant serves only for comparison when looping through all
 * payload values. For example:
 *
 * @code
 * for (payload_id_t i = 0; i < last_sensor; i++)
 * {
 * // some code using i.
 * }
 * @endcode
 */

typedef enum payload_id {
    temp_sensors_2=0,           ///< 0: Temperature sensors
    ads_sensors_2,              ///< Ads sensors
    eps_sensors_2,              ///< Eps sensors
    status_sensors_2,           ///< Status Variables
    stt_sensors_2,              ///< STT sensors
    rw_sensors_2,               ///< RW Speed and current sensor
    fss_sensors_2,              ///< 6: Ads sensors fss
    ekf_sensors_2,
    ctrl_sensors_2,
    msg_sensors_2,              ///< Store and forward messages payloads

    temp_sensors_3,             ///< 10: Temperature sensors
    ads_sensors_3,              ///< Ads sensors
    eps_sensors_3,              ///< Eps sensors
    status_sensors_3,           ///< Status Variables
    stt_sensors_3,              ///< STT sensors
    rw_sensors_3,               ///< RW Speed and current sensor
    fss_sensors_3,              ///< 16: Ads sensors fss
    ekf_sensors_3,
    ctrl_sensors_3,
    msg_sensors_3,              ///< Store and forward messages payloads

    temp_sensors_P,             ///< 20: Temperature sensors
    ads_sensors_P,              ///< Ads sensors
    eps_sensors_P,              ///< Eps sensors
    status_sensors_P,           ///< Status Variables
    stt_sensors_P,              ///< STT sensors
    rw_sensors_P,               ///< RW Speed and current sensor
    fss_sensors_P,              ///< Ads sensors fss
    ekf_sensors_P,
    ctrl_sensors_P,
    msg_sensors_P,              ///< Store and forward messages payloads
    ///< STT sensors
    stt_temp_sensors_2,         ///< 30: STT Temperature sensors
    stt_stt_sensors_2,
    stt_exp_time_sensors_2,
    stt_gyro_sensors_2,
    stt_temp_sensors_3,         ///< 34: STT Temperature sensors
    stt_stt_sensors_3,
    stt_exp_time_sensors_3,
    stt_gyro_sensors_3,
    stt_temp_sensors_P,         ///< 38: STT Temperature sensors
    stt_stt_sensors_P,
    stt_exp_time_sensors_P,
    stt_gyro_sensors_P,
    ///< MAG sensors
    mag_temp_sensors_2,         ///< 42: Temperature sensors
    mag_fod_sensors_2,          ///< Data of the femto-satellites received at the FOD.
    mag_mag_sensor_2,           ///< New mag sensor
    mag_stt_sensors_2,          ///< STT sensors
    mag_stt_exp_time_sensors_2, ///< STT exposure time sensors
    mag_stt_gyro_sensors_2,     ///< STT gyro sensor
    mag_iot_sensor_2,           ///< Data received by the IoT transceiver.
    mag_aoa_sensors_2,          ///< Phase and magnitude difference in voltage of the antenna array.
    mag_temp_sensors_3,         ///< 50: Temperature sensors
    mag_fod_sensors_3,          ///< Data of the femto-satellites received at the FOD.
    mag_mag_sensor_3,           ///< New mag sensor
    mag_stt_sensors_3,          ///< STT sensors
    mag_stt_exp_time_sensors_3, ///< STT exposure time sensors
    mag_stt_gyro_sensors_3,     ///< STT gyro sensor
    mag_iot_sensor_3,           ///< Data received by the IoT transceiver.
    mag_aoa_sensors_3,          ///< Phase and magnitude difference in voltage of the antenna array.
    mag_temp_sensors_P,         ///< 58: Temperature sensors
    mag_fod_sensors_P,          ///< Data of the femto-satellites received at the FOD.
    mag_mag_sensor_P,           ///< New mag sensor
    mag_stt_sensors_P,          ///< STT sensors
    mag_stt_exp_time_sensors_P, ///< STT exposure time sensors
    mag_stt_gyro_sensors_P,     ///< STT gyro sensor
    mag_iot_sensor_P,           ///< Data received by the IoT transceiver.
    mag_aoa_sensors_P,          ///< Phase and magnitude difference in voltage of the antenna array.
    ///< GRA sensors
    gra_temp_sensors_P,         ///< 66: Temperature sensors
    ///< GPS sensors
    gps_temp_sensors_2,         ///< 67: Temperature sensors
    gps_temp_sensors_3,         ///< 68: Temperature sensors
    lp_sensors_2,               ///< 69: Temperature sensors
    lp_sensors_3,                 ///< 70: Temperature sensors
    ///< Last
    last_sensor               ///< Dummy element, the amount of payload variables
} payload_id_t;

/**
 * Struct for storing temperature data.
 */
typedef struct __attribute__((__packed__)) temp_data {
    uint32_t index;
    uint32_t timestamp;
    int16_t obc_temp_1;
    int16_t obc_temp_2;
    int16_t obc_temp_3;
    int16_t eps_temp1;
    int16_t eps_temp2;
    int16_t eps_temp3;
    int16_t eps_temp4;
    int16_t bat_temp1;
    int16_t bat_temp2;
    int16_t istage_temp1;
    int16_t istage_temp2;
    int16_t istage_temp3;
    int16_t istage_temp4;
    int16_t spanel_temp1;
    int16_t spanel_temp2;
    int16_t spanel_temp3;
    int16_t spanel_temp4;
    /**int16_t is2_int_temp1;
    int16_t is2_int_temp2;
    int16_t is2_int_temp3;
    int16_t is2_int_temp4;
    int16_t is2_ext_temp1;
    int16_t is2_ext_temp2;
    int16_t is2_ext_temp3;
    int16_t is2_ext_temp4;**/
    int16_t dummy;
} temp_data_t; //2*4+26*2 bytes = 60

static char temp_var_string[] = "sat_index timestamp obc_temp_1 obc_temp_2 obc_temp_3 eps_temp1 eps_temp2 eps_temp3 "
                                  "eps_temp4 bat_temp1 bat_temp2 istage_temp1 istage_temp2 istage_temp3 istage_temp4 "
                                  "spanel_temp1 spanel_temp2 spanel_temp3 spanel_temp4 dummy";

static char temp_var_types[] = "%u %u %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h";

/**
 * Struct for storing data collected by ads sensors.
 */
typedef struct __attribute__((__packed__)) ads_data {
    uint32_t index;
    uint32_t timestamp;
    float acc_x;            ///< Gyroscope acceleration value along the x axis
    float acc_y;            ///< Gyroscope acceleration value along the y axis
    float acc_z;            ///< Gyroscope acceleration value along the z axis
    float mag_x;            ///< Magnetometer x axis
    float mag_y;            ///< Magnetometer y axis
    float mag_z;            ///< Magnetometer z axis
    //int32_t sun1;           ///< Coarse sun sensor 1
    int32_t sun2;           ///< Coarse sun sensor 2
    int32_t sun3;           ///< Coarse sun sensor 3
    int32_t sun4;           ///< Coarse sun sensor 4
    //int32_t sun5;           ///< Coarse sun sensor 5
    //int32_t sun6;           ///< Coarse sun sensor 6
} ads_data_t; //2*4 + 6*4 + 3*4 = 44 bytes

/**
 * Struct for storing data collected by ads ekf sensors.
 */
typedef struct __attribute__((__packed__)) ads_ekf_data {
    uint32_t index;
    uint32_t timestamp;
    float gyro_x;            ///< Gyroscope acceleration value along the x axis
    float gyro_y;            ///< Gyroscope acceleration value along the y axis
    float gyro_z;            ///< Gyroscope acceleration value along the z axis
    float mag_x;            ///< Magnetometer x axis
    float mag_y;            ///< Magnetometer y axis
    float mag_z;            ///< Magnetometer z axis
    float q0_det;
    float q1_det;
    float q2_det;
    float q3_det;
} ekf_data_t; //2*4 + 6*4 + 4*2 +3*4= 52

/**
 * Struct for storing data collected by ads ekf sensors.
 */
typedef struct __attribute__((__packed__)) ads_ctrl_data {
    uint32_t index;
    uint32_t timestamp;
    float ctrl_torque_x;
    float ctrl_torque_y;
    float ctrl_torque_z;
    float ctrl_hardware_x;
    float ctrl_hardware_y;
    float ctrl_hardware_z;
} ctrl_data_t; //2*4 + 6*4 + 4*2 +3*4= 52

/**
 * Struct for storing data collected by ads fss sensors.
 */
typedef struct __attribute__((__packed__)) ads_data_fss {
    uint32_t index;
    uint32_t timestamp;
    float acc_x;            ///< Gyroscope acceleration value along the x axis
    float acc_y;            ///< Gyroscope acceleration value along the y axis
    float acc_z;            ///< Gyroscope acceleration value along the z axis
    uint16_t fss1_a;
    uint16_t fss1_b;
    uint16_t fss1_c;
    uint16_t fss1_d;
    uint16_t fss2_a;
    uint16_t fss2_b;
    uint16_t fss2_c;
    uint16_t fss2_d;
    uint16_t fss3_a;
    uint16_t fss3_b;
    uint16_t fss3_c;
    uint16_t fss3_d;
    uint16_t fss4_a;
    uint16_t fss4_b;
    uint16_t fss4_c;
    uint16_t fss4_d;
    uint16_t fss5_a;
    uint16_t fss5_b;
    uint16_t fss5_c;
    uint16_t fss5_d;
} fss_data_t; // 2*4 + 3*4 + 20*2 = 60 bytes

/**
 * Struct for storing data collected by eps housekeeping.
 */
typedef struct __attribute__((__packed__)) eps_data {
    uint32_t index;
    uint32_t timestamp;
    uint32_t cursun; ///< Current from boost converters [mA]
    uint32_t cursys; ///< Current out of battery [mA]
    uint32_t vbatt; ///< Voltage of battery [mV]
    int32_t temp1; ///< EPS temperature = 10*(TEMP1+TEMP2+TEMP3+TEMP4)/4
    int32_t temp2; ///< BAT temperature = 10*(BATT0 + BATT1)/2
} eps_data_t;

/**
 * Struct for storing data collected by stt.
 */
typedef struct __attribute__((__packed__)) stt_data {
    uint32_t index;
    uint32_t timestamp;
    float ra;
    float dec;
    float roll;
    int time;
    float exec_time;
} stt_data_t;

/**
 * Struct for storing data collected by status variables.
 */
typedef struct __attribute__((__packed__)) sta_data {
    uint32_t index;
    uint32_t timestamp;
    uint32_t dat_obc_opmode;
    uint32_t dat_rtc_date_time;
    uint32_t dat_obc_last_reset;
    uint32_t dat_obc_hrs_alive;
    uint32_t dat_obc_hrs_wo_reset;
    uint32_t dat_obc_reset_counter;
    uint32_t dat_obc_executed_cmds;
    uint32_t dat_obc_failed_cmds;
    uint32_t dat_com_count_tm;
    uint32_t dat_com_count_tc;
    uint32_t dat_com_last_tc;
    uint32_t dat_fpl_last;
    uint32_t dat_fpl_queue;
    uint32_t dat_ads_tle_epoch;
    uint32_t dat_eps_vbatt;
    uint32_t dat_eps_cur_sun;
    uint32_t dat_eps_cur_sys;
    uint32_t dat_obc_temp_1;
    uint32_t dat_eps_temp_bat0;
    uint32_t dat_drp_mach_action;
    uint32_t dat_drp_mach_state;
    uint32_t dat_drp_mach_payloads;
    uint32_t dat_drp_mach_step;
} status_data_t;

static char status_var_string[] = "sat_index timestamp dat_obc_opmode rtc_date_time obc_last_reset obc_hrs_alive "
                                  "obc_hrs_wo_reset obc_reset_counter obc_executed_cmds obc_failed_cmds com_count_tm "
                                  "com_count_tc com_last_tc fpl_last fpl_queue ads_tle_epoch eps_vbatt eps_cur_sun "
                                  "eps_cur_sys obc_temp_1 eps_temp_bat0 drp_mach_action drp_mach_state drp_mach_payloads "
                                  "drp_mach_step";

static char status_var_types[] = "%u %u %u %d %u %u %u %u %u %u %u %u %d %d %u %d %d %u %u %f %d %u %u %u %u";

/**
 * Struct for storing rw data.
 */
typedef struct __attribute__((__packed__)) rw_data {
    uint32_t index;
    uint32_t timestamp;
    float current1;
    float current2;
    float current3;
    int32_t speed1;
    int32_t speed2;
    int32_t speed3;
} rw_data_t;

/**
 * Struct for storing string data.
 */
typedef struct __attribute__((__packed__)) string_data {
    uint32_t index;
    uint32_t timestamp;
    char msg[SCH_ST_STR_SIZE];
} string_data_t;

static data_map_t data_map[last_sensor] = {
        ///< CDH data
        {"dat_temp_data_2",    (uint16_t) (sizeof(temp_data_t)),   dat_drp_idx_temp_2, dat_drp_ack_temp_2, temp_var_types, temp_var_string},
        {"dat_ads_data_2",     (uint16_t) (sizeof(ads_data_t)),    dat_drp_idx_ads_2,  dat_drp_ack_ads_2,  "%u %u %f %f %f %f %f %f %d %d %d", "sat_index timestamp acc_x acc_y acc_z mag_x mag_y mag_z sun2 sun3 sun4"},
        {"dat_eps_data_2",     (uint16_t) (sizeof(eps_data_t)),    dat_drp_idx_eps_2,  dat_drp_ack_eps_2,  "%u %u %u %u %u %d %d",                "sat_index timestamp cursun cursys vbatt temp_eps temp_bat"},
        {"dat_sta_data_2",     (uint16_t) (sizeof(status_data_t)), dat_drp_idx_sta_2,  dat_drp_ack_sta_2,  status_var_types, status_var_string},
        {"dat_stt_data_2",     (uint16_t) (sizeof(stt_data_t)),    dat_drp_idx_stt_2,  dat_drp_ack_stt_2,  "%u %u %f %f %f %d %f",    "sat_index timestamp ra dec roll time exec_time"},
        {"dat_rw_data_2",      (uint16_t) (sizeof(temp_data_t)),   dat_drp_idx_rw_2,   dat_drp_ack_rw_2,   "%u %u %f %f %f %d %d %d", "sat_index timestamp current1 current2 current3 speed1 speed2 speed3"},
        {"dat_fss_data_2",     (uint16_t) (sizeof(fss_data_t)),    dat_drp_idx_fss_2,  dat_drp_ack_fss_2,
                "%u %u %f %f %f %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h",
                "sat_index timestamp acc_x acc_y acc_z fss1_a fss1_b fss1_c fss1_d fss2_a fss2_b fss2_c fss2_d fss3_a fss3_b fss3_c fss3_d fss4_a fss4_b fss4_c fss4_d fss5_a fss5_b fss5_c fss5_d"},
        {"dat_ekf_data_2",     (uint16_t) (sizeof(ekf_data_t)),     dat_drp_idx_ekf_2,  dat_drp_ack_ekf_2,  "%u %u %f %f %f %f %f %f %f %f %f %f",
                "sat_index timestamp gyro_x gyro_y gyro_z mag_x mag_y mag_z q0 q1 q2 q3"},
        {"dat_ctrl_data_2", (uint16_t) (sizeof(ctrl_data_t)), dat_drp_idx_ctrl_2, dat_drp_ack_ctrl_2, "%u %u %f %f %f %f %f %f",
                "sat_index timestamp ctrl_x ctrl_y ctrl_z ctrl_hw_x ctrl_hw_y ctrl_hw_z"},
        {"dat_msg_data_2",     (uint16_t) (sizeof(string_data_t)), dat_drp_idx_str_2, dat_drp_ack_str_2,   "%u %u %s",                "sat_index timestamp string_data"},
        {"dat_temp_data_3",    (uint16_t) (sizeof(temp_data_t)),   dat_drp_idx_temp_3, dat_drp_ack_temp_3, temp_var_types, temp_var_string},
        {"dat_ads_data_3",     (uint16_t) (sizeof(ads_data_t)),    dat_drp_idx_ads_3,  dat_drp_ack_ads_3,  "%u %u %f %f %f %f %f %f %d %d %d", "sat_index timestamp acc_x acc_y acc_z mag_x mag_y mag_z sun1 sun2 sun3"},
        {"dat_eps_data_3",     (uint16_t) (sizeof(eps_data_t)),    dat_drp_idx_eps_3,  dat_drp_ack_eps_3,  "%u %u %u %u %u %d %d",                "sat_index timestamp cursun cursys vbatt temp_eps temp_bat"},
        {"dat_sta_data_3",     (uint16_t) (sizeof(status_data_t)), dat_drp_idx_sta_3,  dat_drp_ack_sta_3,  status_var_types, status_var_string},
        {"dat_stt_data_3",     (uint16_t) (sizeof(stt_data_t)),    dat_drp_idx_stt_3,  dat_drp_ack_stt_3,  "%u %u %f %f %f %d %f",    "sat_index timestamp ra dec roll time exec_time"},
        {"dat_rw_data_3",      (uint16_t) (sizeof(temp_data_t)),   dat_drp_idx_rw_3,   dat_drp_ack_rw_3,   "%u %u %f %f %f %d %d %d", "sat_index timestamp current1 current2 current3 speed1 speed2 speed3"},
        {"dat_fss_data_3",     (uint16_t) (sizeof(fss_data_t)),    dat_drp_idx_fss_3,  dat_drp_ack_fss_3,
                                                                                                            "%u %u %f %f %f %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h",
                "sat_index timestamp acc_x acc_y acc_z fss1_a fss1_b fss1_c fss1_d fss2_a fss2_b fss2_c fss2_d fss3_a fss3_b fss3_c fss3_d fss4_a fss4_b fss4_c fss4_d fss5_a fss5_b fss5_c fss5_d"},
        {"dat_ekf_data_3",     (uint16_t) (sizeof(ekf_data_t)),     dat_drp_idx_ekf_3,  dat_drp_ack_ekf_3,  "%u %u %f %f %f %f %f %f %f %f %f %f",
                "sat_index timestamp gyro_x gyro_y gyro_z mag_x mag_y mag_z q0 q1 q2 q3"},
        {"dat_ctrl_data_3", (uint16_t) (sizeof(ctrl_data_t)), dat_drp_idx_ctrl_3, dat_drp_ack_ctrl_3, "%u %u %f %f %f %f %f %f",
                "sat_index timestamp ctrl_x ctrl_y ctrl_z ctrl_hw_x ctrl_hw_y ctrl_hw_z"},
        {"dat_msg_data_3",     (uint16_t) (sizeof(string_data_t)), dat_drp_idx_str_3, dat_drp_ack_str_3,   "%u %u %s",                "sat_index timestamp string_data"},
        {"dat_temp_data_P",    (uint16_t) (sizeof(temp_data_t)),   dat_drp_idx_temp_P, dat_drp_ack_temp_P, temp_var_types, temp_var_string},
        {"dat_ads_data_P",     (uint16_t) (sizeof(ads_data_t)),    dat_drp_idx_ads_P,  dat_drp_ack_ads_P,  "%u %u %f %f %f %f %f %f %d %d %d", "sat_index timestamp acc_x acc_y acc_z mag_x mag_y mag_z sun1 sun2 sun3"},
        {"dat_eps_data_P",     (uint16_t) (sizeof(eps_data_t)),    dat_drp_idx_eps_P,  dat_drp_ack_eps_P,  "%u %u %u %u %u %d %d",                "sat_index timestamp cursun cursys vbatt temp_eps temp_bat"},
        {"dat_sta_data_P",     (uint16_t) (sizeof(status_data_t)), dat_drp_idx_sta_P,  dat_drp_ack_sta_P,  status_var_types, status_var_string},
        {"dat_stt_data_P",     (uint16_t) (sizeof(stt_data_t)),    dat_drp_idx_stt_P,  dat_drp_ack_stt_P,  "%u %u %f %f %f %d %f",    "sat_index timestamp ra dec roll time exec_time"},
        {"dat_rw_data_P",      (uint16_t) (sizeof(temp_data_t)),   dat_drp_idx_rw_P,   dat_drp_ack_rw_P,   "%u %u %f %f %f %d %d %d", "sat_index timestamp current1 current2 current3 speed1 speed2 speed3"},
        {"dat_fss_data_P",     (uint16_t) (sizeof(fss_data_t)),    dat_drp_idx_fss_P,  dat_drp_ack_fss_P,
                                                                                                            "%u %u %f %f %f %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h %h",
                "sat_index timestamp acc_x acc_y acc_z fss1_a fss1_b fss1_c fss1_d fss2_a fss2_b fss2_c fss2_d fss3_a fss3_b fss3_c fss3_d fss4_a fss4_b fss4_c fss4_d fss5_a fss5_b fss5_c fss5_d"},
        {"dat_ekf_data_P",     (uint16_t) (sizeof(ekf_data_t)),     dat_drp_idx_ekf_P,  dat_drp_ack_ekf_P,  "%u %u %f %f %f %f %f %f %f %f %f %f",
                "sat_index timestamp gyro_x gyro_y gyro_z mag_x mag_y mag_z q0 q1 q2 q3"},
        {"dat_ctrl_data_P", (uint16_t) (sizeof(ctrl_data_t)), dat_drp_idx_ctrl_P, dat_drp_ack_ctrl_P, "%u %u %f %f %f %f %f %f",
                "sat_index timestamp ctrl_x ctrl_y ctrl_z ctrl_hw_x ctrl_hw_y ctrl_hw_z"},
        {"dat_msg_data_P",     (uint16_t) (sizeof(string_data_t)), dat_drp_idx_str_P, dat_drp_ack_str_P,   "%u %u %s",                "sat_index timestamp string_data"},
        ///< STT data
        {"stt_temp_data_2",    (uint16_t) (sizeof(stt_temp_data_t)),     stt_dat_drp_idx_temp_2,     stt_dat_drp_ack_temp_2,         "%u %u %f",             "sat_index timestamp obc_temp_1"},
        {"stt_data_2",         (uint16_t) (sizeof(stt_stt_data_t)),      stt_dat_drp_idx_stt_2,          stt_dat_drp_ack_stt_2,          "%u %u %f %f %f %d %f", "sat_index timestamp ra dec roll time exec_time"},
        {"stt_exp_time_2",     (uint16_t) (sizeof(stt_exp_time_data_t)), stt_dat_drp_idx_stt_exp_time_2, stt_dat_drp_ack_stt_exp_time_2, "%u %u %d %d",          "sat_index timestamp exp_time n_stars"},
        {"stt_gyro_data_2",    (uint16_t) (sizeof(stt_gyro_data_t)),     stt_dat_drp_idx_stt_gyro_2,     stt_dat_drp_ack_stt_gyro_2,     "%u %u %f %f %f",       "sat_index timestamp gx gy gz"},
        {"stt_temp_data_3",    (uint16_t) (sizeof(stt_temp_data_t)),     stt_dat_drp_idx_temp_3,     stt_dat_drp_ack_temp_3,         "%u %u %f",             "sat_index timestamp obc_temp_1"},
        {"stt_data_3",         (uint16_t) (sizeof(stt_stt_data_t)),      stt_dat_drp_idx_stt_3,          stt_dat_drp_ack_stt_3,          "%u %u %f %f %f %d %f", "sat_index timestamp ra dec roll time exec_time"},
        {"stt_exp_time_3",     (uint16_t) (sizeof(stt_exp_time_data_t)), stt_dat_drp_idx_stt_exp_time_3, stt_dat_drp_ack_stt_exp_time_3, "%u %u %d %d",          "sat_index timestamp exp_time n_stars"},
        {"stt_gyro_data_3",    (uint16_t) (sizeof(stt_gyro_data_t)),     stt_dat_drp_idx_stt_gyro_3,     stt_dat_drp_ack_stt_gyro_3,     "%u %u %f %f %f",       "sat_index timestamp gx gy gz"},
        {"stt_temp_data_P",    (uint16_t) (sizeof(stt_temp_data_t)),     stt_dat_drp_idx_temp_P,     stt_dat_drp_ack_temp_P,         "%u %u %f",             "sat_index timestamp obc_temp_1"},
        {"stt_data_P",         (uint16_t) (sizeof(stt_stt_data_t)),      stt_dat_drp_idx_stt_P,          stt_dat_drp_ack_stt_P,          "%u %u %f %f %f %d %f", "sat_index timestamp ra dec roll time exec_time"},
        {"stt_exp_time_P",     (uint16_t) (sizeof(stt_exp_time_data_t)), stt_dat_drp_idx_stt_exp_time_P, stt_dat_drp_ack_stt_exp_time_P, "%u %u %d %d",          "sat_index timestamp exp_time n_stars"},
        {"stt_gyro_data_P",    (uint16_t) (sizeof(stt_gyro_data_t)),     stt_dat_drp_idx_stt_gyro_P,     stt_dat_drp_ack_stt_gyro_P,     "%u %u %f %f %f",       "sat_index timestamp gx gy gz"},
        ///< MAG DATA
        {"mag_temp_data_2",    (uint16_t) (sizeof(mag_temp_data_t)), mag_dat_drp_idx_temp_2, mag_dat_drp_ack_temp_2, "%u %u %f", "sat_index timestamp obc_temp_1"},
        {"mag_fod_data_2",     (uint16_t) (sizeof(fod_data_t)), mag_dat_drp_idx_fod_2,  mag_dat_drp_ack_fod_2,  "%u %u %u %u %u %u %d %d %d %u %u %u %d %d %d", "sat_index timestamp node1 fe_index1 date time latitude longitude altitude num_sats node2 fe_index2 fe_mag_x fe_mag_y fe_mag_z"},
        {"mag_mag_data_2",     (uint16_t) (sizeof(mag_data_t)), mag_dat_drp_idx_mag_2,  mag_dat_drp_ack_mag_2,  "%u %u %d %d %d %d %d %d %d %d %f %f", "sat_index timestamp splf magxf magyf magzf spls magxs magys magzs tmpf tmps"},
        {"mag_stt_data_2",     (uint16_t) (sizeof(mag_stt_data_t)), mag_dat_drp_idx_stt_2, mag_dat_drp_ack_stt_2, "%u %u %f %f %f %d %f", "sat_index timestamp ra dec roll time exec_time"},
        {"mag_stt_exp_time_2", (uint16_t) (sizeof(mag_stt_exp_time_data_t)), mag_dat_drp_idx_stt_exp_time_2, mag_dat_drp_ack_stt_exp_time_2, "%u %u %d %d", "sat_index timestamp exp_time n_stars"},
        {"mag_stt_gyro_data_2",(uint16_t) (sizeof(mag_stt_gyro_data_t)), mag_dat_drp_idx_stt_gyro_2, mag_dat_drp_ack_stt_gyro_2, "%u %u %f %f %f", "sat_index timestamp gx gy gz"},
        {"mag_iot_data_2",     (uint16_t) (sizeof(iot_data_t)), mag_dat_drp_idx_iot_2,  mag_dat_drp_ack_iot_2, "%u %u %u %u %u %s", "sat_index timestamp module temp1 temp2 iot_data"},
        {"mag_aoa_data_2",     (uint16_t) (sizeof(aoa_data_t)), mag_dat_drp_idx_aoa_2, mag_dat_drp_ack_aoa_2,   "%u %u %u %u %u %u", "sat_index timestamp vmag1 vphase1 vmag2 vphase2"},
        {"mag_temp_data_3",    (uint16_t) (sizeof(mag_temp_data_t)), mag_dat_drp_idx_temp_3, mag_dat_drp_ack_temp_3, "%u %u %f", "sat_index timestamp obc_temp_1"},
        {"mag_fod_data_3",     (uint16_t) (sizeof(fod_data_t)), mag_dat_drp_idx_fod_3,  mag_dat_drp_ack_fod_3,  "%u %u %u %u %u %u %d %d %d %u %u %u %d %d %d", "sat_index timestamp node1 fe_index1 date time latitude longitude altitude num_sats node2 fe_index2 fe_mag_x fe_mag_y fe_mag_z"},
        {"mag_mag_data_3",     (uint16_t) (sizeof(mag_data_t)), mag_dat_drp_idx_mag_3,  mag_dat_drp_ack_mag_3,  "%u %u %d %d %d %d %d %d %d %d %f %f", "sat_index timestamp splf magxf magyf magzf spls magxs magys magzs tmpf tmps"},
        {"mag_stt_data_3",     (uint16_t) (sizeof(mag_stt_data_t)), mag_dat_drp_idx_stt_3, mag_dat_drp_ack_stt_3, "%u %u %f %f %f %d %f", "sat_index timestamp ra dec roll time exec_time"},
        {"mag_stt_exp_time_3", (uint16_t) (sizeof(mag_stt_exp_time_data_t)), mag_dat_drp_idx_stt_exp_time_3, mag_dat_drp_ack_stt_exp_time_3, "%u %u %d %d", "sat_index timestamp exp_time n_stars"},
        {"mag_stt_gyro_data_3",(uint16_t) (sizeof(mag_stt_gyro_data_t)), mag_dat_drp_idx_stt_gyro_3, mag_dat_drp_ack_stt_gyro_3, "%u %u %f %f %f", "sat_index timestamp gx gy gz"},
        {"mag_iot_data_3",     (uint16_t) (sizeof(iot_data_t)), mag_dat_drp_idx_iot_3,  mag_dat_drp_ack_iot_3, "%u %u %u %u %u %s", "sat_index timestamp module temp1 temp2 iot_data"},
        {"mag_aoa_data_3",     (uint16_t) (sizeof(aoa_data_t)), mag_dat_drp_idx_aoa_3, mag_dat_drp_ack_aoa_3,   "%u %u %u %u %u %u", "sat_index timestamp vmag1 vphase1 vmag2 vphase2"},
        {"mag_temp_data_P",    (uint16_t) (sizeof(mag_temp_data_t)), mag_dat_drp_idx_temp_P, mag_dat_drp_ack_temp_P, "%u %u %f", "sat_index timestamp obc_temp_1"},
        {"mag_fod_data_P",     (uint16_t) (sizeof(fod_data_t)), mag_dat_drp_idx_fod_P,  mag_dat_drp_ack_fod_P,  "%u %u %u %u %u %u %d %d %d %u %u %u %d %d %d", "sat_index timestamp node1 fe_index1 date time latitude longitude altitude num_sats node2 fe_index2 fe_mag_x fe_mag_y fe_mag_z"},
        {"mag_mag_data_P",     (uint16_t) (sizeof(mag_data_t)), mag_dat_drp_idx_mag_P,  mag_dat_drp_ack_mag_P,  "%u %u %d %d %d %d %d %d %d %d %f %f", "sat_index timestamp splf magxf magyf magzf spls magxs magys magzs tmpf tmps"},
        {"mag_stt_data_P",     (uint16_t) (sizeof(mag_stt_data_t)), mag_dat_drp_idx_stt_P, mag_dat_drp_ack_stt_P, "%u %u %f %f %f %d %f", "sat_index timestamp ra dec roll time exec_time"},
        {"mag_stt_exp_time_P", (uint16_t) (sizeof(mag_stt_exp_time_data_t)), mag_dat_drp_idx_stt_exp_time_P, mag_dat_drp_ack_stt_exp_time_P, "%u %u %d %d", "sat_index timestamp exp_time n_stars"},
        {"mag_stt_gyro_data_P",(uint16_t) (sizeof(mag_stt_gyro_data_t)), mag_dat_drp_idx_stt_gyro_P, mag_dat_drp_ack_stt_gyro_P, "%u %u %f %f %f", "sat_index timestamp gx gy gz"},
        {"mag_iot_data_P",     (uint16_t) (sizeof(iot_data_t)), mag_dat_drp_idx_iot_P,  mag_dat_drp_ack_iot_P, "%u %u %u %u %u %s", "sat_index timestamp module temp1 temp2 iot_data"},
        {"mag_aoa_data_P",     (uint16_t) (sizeof(aoa_data_t)), mag_dat_drp_idx_aoa_P, mag_dat_drp_ack_aoa_P,   "%u %u %u %u %u %u", "sat_index timestamp vmag1 vphase1 vmag2 vphase2"},
        ///< GRA DATA
        {"gra_temp_data_P",    (uint16_t) (sizeof(gra_temp_data_t)),gra_dat_drp_idx_temp_P,gra_dat_drp_ack_temp_P, "%u %u %f", "sat_index timestamp obc_temp_1"},
        ///< GPS DATA
        {"gps_temp_data_2",    (uint16_t) (sizeof(gps_temp_data_t)), gps_dat_drp_idx_temp_2, gps_dat_drp_ack_temp_2, "%u %u %f", "sat_index timestamp obc_temp_1"},
        {"gps_temp_data_3",    (uint16_t) (sizeof(gps_temp_data_t)), gps_dat_drp_idx_temp_3, gps_dat_drp_ack_temp_3, "%u %u %f", "sat_index timestamp obc_temp_1"},
        {"lp_data_2",            (uint16_t) (sizeof(lp_data_t)), dat_drp_idx_lp_2, dat_drp_ack_lp_2, "%u %u %u %u %u %u %u %u %u", "lp_index lp_timestamp lp_unit hk_idx lp_hk lp_hgchn lp_lgchn crc chk"},
        {"lp_data_3",            (uint16_t) (sizeof(lp_data_t)), dat_drp_idx_lp_3, dat_drp_ack_lp_3, "%u %u %u %u %u %u %u %u %u", "lp_index lp_timestamp lp_unit hk_idx lp_hk lp_hgchn lp_lgchn crc chk"},
};

/** The repository's name */
#define DAT_TABLE_STATUS "dat_status"      ///< Status variables table name
#define DAT_TABLE_DATA   "dat_data"        ///< Data storage table name
#define DAT_TABLE_FP     "dat_flightplan"  ///< Flight plan table name

/**
 * Search and return a status variable definition from dat_status_list by index or by name
 * @param address Variable index
 * @param name Variable name
 * @return dat_sys_var_t or 0 if not found.
 */
dat_sys_var_t dat_get_status_var_def(dat_status_address_t address);
dat_sys_var_t dat_get_status_var_def_name(char *name);

/**
 * Print the names and values of a system status variable list.
 * @param status Pointer to a status variables list
 */
void dat_print_system_var(dat_sys_var_t *status);

#endif //REPO_DATA_SCHEMA_H
