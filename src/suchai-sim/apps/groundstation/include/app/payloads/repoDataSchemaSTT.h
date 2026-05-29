/**
 * @file  dataSchema.h
 * @author Camilo Rojas M - camrojas@uchile.cl
 * @date 2020
 * @copyright GNU GPL v3
 *
 * This header contains structs symbolizing the schema of data.
 */


#ifndef REPO_DATA_SCHEMA_STT_H
#define REPO_DATA_SCHEMA_STT_H

#include "suchai/log_utils.h"
#include "suchai/storage.h"
#include "app/system/repoDataSchema.h"

/**
 * STT structs
 */

typedef struct __attribute__((__packed__)) stt_stt_data {
    uint32_t index;
    uint32_t timestamp;
    float ra;
    float dec;
    float roll;
    int time;
    float exec_time;
} stt_stt_data_t;

typedef struct __attribute__((__packed__)) stt_exp_time_data{
    uint32_t index;
    uint32_t timestamp;
    int exp_time;
    int n_stars;
}stt_exp_time_data_t;

typedef struct __attribute__((__packed__)) stt_gyro_data{
    uint32_t index;
    uint32_t timestamp;
    float gx;
    float gy;
    float gz;
}stt_gyro_data_t;

/**
 * PAYLOAD DATA DEFINITIONS
 */

/**
 * Struct for storing temperature data.
 */
typedef struct __attribute__((__packed__)) sst_temp_data {
    uint32_t index;
    uint32_t timestamp;
    float obc_temp_1;
} stt_temp_data_t;

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
//typedef enum stt_payload_id {
//    temp_sensors=0,         ///< Temperature sensors
//    stt_sensors,
//    exp_time_sensors,
//    gyro_sensors,
//    last_sensor             ///< Dummy element, the amount of payload variables
//} stt_payload_id_t;

#endif //REPO_DATA_SCHEMA_STT_H
