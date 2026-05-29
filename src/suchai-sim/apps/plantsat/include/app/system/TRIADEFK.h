/**
 * This header have definitions of commands related to user application.
 */

#ifndef TRIAD_H
#define TRIAD_H

#include "suchai/config.h"

#include "suchai/repoCommand.h"
#include "suchai/repoData.h"
#include "suchai/math_utils.h"

/**
 * Calculate the quaternion from 2 vectors measured
 */
int determine_quaternion_triadekf(int isdark, vector3_t mag_b, vector3_t mag_i, vector3_t sun_b, vector3_t sun_from_sc_i, quaternion_t * quat_i2b);
vector3_t test_transform_ypr(vector3_t vec_i, double rot_yaw, double rot_pitch, double rot_roll);
int rotation_yaw(vector3_t bfr, vector3_t * aft, double theta);
int rotation_pitch(vector3_t bfr, vector3_t * aft, double theta);
int rotation_roll(vector3_t bfr, vector3_t * aft, double theta);
int compare_vectors(vector3_t vec_a, vector3_t v_b);
void propagate_P_matrix(quaternion_t last_pred_q_i2b, vector3_t last_pred_omega_b, matrix4_t Omega4x, matrix3_t sk_omega_matrix,
                        double dt, matrix10_t * F_j, matrix10_9_t * L_j, matrix10_t * P_j);
quaternion_t calc_with_wahbas_problem(vector3_t unit_sun_vector_b, vector3_t unit_mag_vector_b,
                                      vector3_t unit_mag_earth_i, vector3_t unit_sun_from_sc_i, vector3_t b_times,
                                      vector3_t r_times);
quaternion_t calc_with_triad(vector3_t unit_sun_vector_b, vector3_t unit_mag_vector_b,
                             vector3_t unit_mag_earth_i, vector3_t unit_sun_from_sc_i, vector3_t b_times,
                             vector3_t r_times);
matrix3_t skewsymmetricmatrix(vector3_t vector_3x3);
void estimate_omega_quat_with_triadekf(quaternion_t current_q_det, vector3_t current_omega_b, portTick delay_ms,
                                       quaternion_t * current_q_est_i2c, vector3_t * current_omega_b_est);
matrix3_t skewsymmetricmatrix(vector3_t vector_3x3);
void estimate_omega_with_triadekf(quaternion_t current_q_det, vector3_t current_omega_b, portTick delay_ms,
                                       quaternion_t * current_q_est_i2c, vector3_t * current_omega_b_est);
matrix4_t omega4kinematics(vector3_t x_omega_b);
void attitude_discrete_model(quaternion_t current_q_i2b_det, vector3_t current_omega_b, vector3_t torque_b,
                             double dt, quaternion_t *next_q_i2b, vector3_t *next_omega_b,
                             matrix4_t * Omega4x, matrix3_t * sk_matrix);
void get_sc_inertia_matrix(matrix3_t * Inertia_ekf, matrix3_t * inv_Inertia_ekf);
void get_attitude_jacobian_model(quaternion_t q_i2c, vector3_t omega_b, matrix4_t Omega4x, matrix3_t sk_omega_matrix,
                                 double dt, matrix10_t * F_j, matrix10_9_t * L_j);
matrix3_t get_rw_inertia_matrix();
void first_adcs_data(vector3_t current_omega_b, quaternion_t current_q_i2b);
vector7_t get_observer_model(vector3_t omega_b, quaternion_t q_i2b, vector3_t bias_b);
matrix7_t update_covariance_matrix(matrix10_t P_j);
matrix10_7_t calc_kalman_gain(matrix10_t P_j, matrix7_t S_j);

//void calc_inverse_matrix(matrix7_t S_j, matrix7_t * S_j_i);
//void calc_cofactor(matrix7_t lhs, matrix7_t * res, int f);
//double calc_determinant(matrix7_t a, int k);
//matrix7_t calc_transpose(matrix7_t num, matrix7_t fac, int r);
vector7_t get_measure_as_vector(vector3_t current_omega_b_, quaternion_t current_q_det_);
void vec_sum_v7(vector7_t measure_vector, vector7_t z_observer, vector7_t * error_state);
void update_covariance_P_matrix(matrix10_7_t K_j_, matrix10_t P_j_);
void determine_quaternion_by_mtt(vector3_t current_mag_b, vector3_t current_mag_i, quaternion_t * current_q_det);
#endif /* TRIAD_H */
