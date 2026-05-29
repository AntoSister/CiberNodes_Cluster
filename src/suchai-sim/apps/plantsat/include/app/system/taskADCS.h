//
// Created by Elias Obreque 25-02-2022
//

#ifndef T_ADCS_H
#define T_ADCS_H

#include <stdlib.h>
#include <stdint.h>

#include "suchai/config.h"
#include "suchai/globals.h"

#include "suchai/osQueue.h"
#include "suchai/osDelay.h"

#include "suchai/math_utils.h"
#include "suchai/log_utils.h"

#include "suchai/repoCommand.h"
#include "igrf/igrf13.h"
#include "SGP4.h"

void taskADCS(void *param);

void eskf_predict_state(double P[6][6], double dt);

void calc_sun_pos_i(double jd, vector3_t * sun_dir);

double unixt_to_jd(uint32_t unix_time);

void calc_magnetic_model(double decyear, double latrad, double lonrad, double altm, double current_sideral_, vector3_t * mag);

double jd_to_dec(double jd);

int eci_to_geodetic(vector3_t sat_pos, double current_side, vector3_t * lat_lon_alt);
void calc_adcs_model_parameters(unsigned int elapsed_msec, vector3_t * sat_pos_i, vector3_t * geod_vect,
                                vector3_t * current_mag_i, int * isdark, vector3_t  * sun_pos_i);

#endif //T_ADCS_H
