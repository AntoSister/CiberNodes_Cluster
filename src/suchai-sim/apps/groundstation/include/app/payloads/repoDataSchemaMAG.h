/**
 * @file  dataSchema.h
 * @author Camilo Rojas M - camrojas@uchile.cl
 * @date 2020
 * @copyright GNU GPL v3
 *
 * This header contains structs symbolizing the schema of data.
 */


#ifndef REPO_DATA_SCHEMA_MAG_H
#define REPO_DATA_SCHEMA_MAG_H

#include "suchai/log_utils.h"
#include "suchai/storage.h"

/**
 * PAYLOAD DATA DEFINITIONS
 */

/**
 * Struct for storing temperature data.
 */
typedef struct __attribute__((__packed__)) mag_temp_data {
    uint32_t index;
    uint32_t timestamp;
    float obc_temp_1;
} mag_temp_data_t;

/**
 * Struct for storing the sensor data of the femto-satellite, transmited to the FOD.
 */
typedef struct __attribute__((__packed__)) fod_data {
    uint32_t index;
    uint32_t timestamp;  ///< Timestamp of when the FOD received the data.
    uint32_t node1;      ///< Designates the femto-satellite that measured the sample.
    uint32_t fe_index1;  ///< Index of the femto-satellite's sample.
    uint32_t date;       ///< Date of the sample.
    uint32_t time;       ///< Hour, minute, second and centisecond of the sample.
    int32_t latitude;    ///< The femto-satellite's latitude when it took the sample.
    int32_t longitude;   ///< The femto-satellite's longitude when it took the sample.
    int32_t altitude;    ///< The altitude of the femto-satellite when it took the sample.
    uint32_t num_sats;   ///< The number of satellites the GNSS receiver was able to detect.
    uint32_t node2;      ///< Designates the femto-satellite that measured the 2nd sample.
    uint32_t fe_index2;  ///< Index of the femto-satellite's 2nd sample.
    int32_t fe_mag_x;    ///< The value of the magnetometer in the X axis.
    int32_t fe_mag_y;    ///< The value of the magnetometer in the Y axis.
    int32_t fe_mag_z;    ///< The value of the magnetometer in the Z axis.
} fod_data_t;

/**
 * Magnetometer struct.
 */
typedef struct __attribute__((__packed__)) mag_data {
    uint32_t index;
    uint32_t timestamp;
    int32_t splf;
    int32_t magxf;
    int32_t magyf;
    int32_t magzf;
    int32_t spls;
    int32_t magxs;
    int32_t magys;
    int32_t magzs;
    float tempf;
    float temps;
} mag_data_t;


/**
 * STT structs
 */
typedef struct __attribute__((__packed__)) mag_stt_data {
    uint32_t index;
    uint32_t timestamp;
    float ra;
    float dec;
    float roll;
    int time;
    float exec_time;
} mag_stt_data_t;

typedef struct __attribute__((__packed__)) mag_stt_exp_time_data{
    uint32_t index;
    uint32_t timestamp;
    int exp_time;
    int n_stars;
}mag_stt_exp_time_data_t;

typedef struct __attribute__((__packed__)) mag_stt_gyro_data{
    uint32_t index;
    uint32_t timestamp;
    float gx;
    float gy;
    float gz;
}mag_stt_gyro_data_t;

/**
 * Struct for storing the IoT received data.
 */
typedef struct __attribute__((__packed__)) iot_data {
    uint32_t index;
    uint32_t timestamp;  ///< Timestampt of when the data was received.
    uint32_t module;     ///< The module that received the data.
    uint32_t temp1;      ///< Temperature N°1.
    uint32_t temp2;      ///< Temperature N°2.
    char data[128];      ///< IoT's data and real time clock.
} iot_data_t;

/**
 * Struct for storing AOA magnitude and phase data.
 */
typedef struct __attribute__((__packed__)) aoa_data {
    uint32_t index;
    uint32_t timestamp; ///< Timestamp of the AOA data.
    uint32_t v_mag1;    ///< Voltage related to the magnitude of antennas 1 and 2.
    uint32_t v_phase1;  ///< Voltage related to the phase between the antennas 1 and 2.
    uint32_t v_mag2;    ///< Voltage related to the magnitude of antennas 3 and 4.
    uint32_t v_phase2;  ///< Voltage related to the phase between the antennas 3 and 4.
} aoa_data_t;

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
//typedef enum mag_payload_id {
//    mag_temp_sensors=0,         ///< Temperature sensors
//    mag_fod_sensors,            ///< Data of the femto-satellites recieved at the FOD.
//    mag_mag_sensor,             ///< New mag sensor
//    mag_stt_sensors,            ///< STT sensors
//    mag_stt_exp_time_sensors,   ///< STT exposure time sensors
//    mag_stt_gyro_sensors,       ///< STT gyro sensor
//    mag_iot_sensor,             ///< Data received by the IoT transceiver.
//    mag_aoa_sensors,            ///< Phase and magnitude difference in voltage of the antenna array.
//    mag_last_sensor             ///< Dummy element, the amount of payload variables
//} mag_payload_id_t;

#endif //REPO_DATA_SCHEMA_MAG_H
