/**
 * @file  dataSchema.h
 * @author Camilo Rojas M - camrojas@uchile.cl
 * @date 2020
 * @copyright GNU GPL v3
 *
 * This header contains structs symbolizing the schema of data.
 */


#ifndef REPO_DATA_SCHEMA_GPS_H
#define REPO_DATA_SCHEMA_GPS_H

#include "suchai/log_utils.h"
#include "suchai/storage.h"

#define GPS_SPLITJPS_EPOCH 0
#define GPS_SPLITJPS_BYTES 0xFF

#define GPS_SPLITBIN_RINEX 0
#define GPS_SPLITBIN_BYTES 0xFF

/**
 * PAYLOAD DATA DEFINITIONS
 */

/**
 * Struct for storing temperature data.
 */
typedef struct __attribute__((__packed__)) gps_temp_data {
    uint32_t index;
    uint32_t timestamp;
    float obc_temp_1;
} gps_temp_data_t;

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
//typedef enum gps_payload_id {
//    gps_temp_sensors=0,         ///< Temperature sensors
//    gps_last_sensor             ///< Dummy element, the amount of payload variables
//} gps_payload_id_t;

typedef struct __attribute__((__packed__)) lp_data {
    uint32_t lp_index;
    uint32_t lp_timestamp;
    uint32_t lp_unit;
    uint32_t lp_hk_idx;
    uint32_t lp_hk;
    uint32_t lp_hgch;
    uint32_t lp_lgch;
    uint32_t crc;
    uint32_t chk;
} lp_data_t;

typedef enum GPS_STATUS{
    GPS_OFF = 0,
    GPS_ON, // 1
    GPS_CONF, // 2
    GPS_WAIT, // 3
    GPS_GETTIME, // 4
    GPS_LOGGING, // 5
    GPS_PROCESS_BINEX, // 6
    GPS_PROCESS_JPS, // 7
    GPS_NONE // 8
}GPS_STATUS_t;

#endif //REPO_DATA_SCHEMA_GPS_H
