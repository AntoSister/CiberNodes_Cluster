//
// Created by carlos on 2/29/24.
//

#ifndef SUCHAI_FLIGHT_SOFTWARE_INTERFACE_H
#define SUCHAI_FLIGHT_SOFTWARE_INTERFACE_H

#include <zmq.h>
#include <assert.h>
#include <stdbool.h>
#include <semaphore.h>

#include "drivers-sim/config.h"

/**
 * Initializes I2C driver
 * @return 0 if success, -1 otherwise
 */
int iface_open();

/**
 * De-Initializes I2C driver
 */
void iface_close();

/**
 * I2C write-read transaction
 * @param send Send buffer
 * @param send_len Send buffer length
 * @param recv Receive buffer
 * @param recv_len Receive buffer length
 * @return 0 if Ok, -1 otherwise
 */
int iface_transaction(uint8_t *send, size_t send_len, uint8_t *recv, size_t recv_len);

#endif //SUCHAI_FLIGHT_SOFTWARE_INTERFACE_H
