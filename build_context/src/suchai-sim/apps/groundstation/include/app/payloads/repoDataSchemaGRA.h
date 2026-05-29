/**
 * @file  dataSchema.h
 * @author Camilo Rojas M - camrojas@uchile.cl
 * @date 2020
 * @copyright GNU GPL v3
 *
 * This header contains structs symbolizing the schema of data.
 */


#ifndef REPO_DATA_SCHEMA_GRA_H
#define REPO_DATA_SCHEMA_GRA_H

#include "suchai/log_utils.h"
#include "suchai/storage.h"
#include "app/system/repoDataSchema.h"


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

//typedef enum gra_payload_id {
//    gra_temp_sensors=0,         ///< Temperature sensors
//    gra_last_sensor             ///< Dummy element, the amount of payload variables
//} gra_payload_id_t;

/**
 * Struct for storing temperature data.
 */
typedef struct __attribute__((__packed__)) gra_temp_data {
    uint32_t index;
    uint32_t timestamp;
    float obc_temp_1;
} gra_temp_data_t;

#endif //REPO_DATA_SCHEMA_GRA_H
