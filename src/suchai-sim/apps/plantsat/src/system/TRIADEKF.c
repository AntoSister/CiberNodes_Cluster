/*                                 SUCHAI
 *                      NANOSATELLITE FLIGHT SOFTWARE
 *
 *      Copyright 2022, Elias Obreque Sepulveda, elias.obreque@uchile.cl
 *      Copyright 2021, Carlos Gonzalez Cortes, carlgonz@uchile.cl
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

#include "app/system/TRIADEFK.h"

static const char* tag = "triadEFK";
#define PI 3.141592654
#define DEG2RAD	0.017453292519943295769236907684886
#define RAD2DEG (180.0/PI)

matrix10_t P_;
matrix7_t R_;
matrix9_t Q_;
matrix7_10_t H_;
matrix10_t I_x10;

void first_adcs_data(vector3_t current_omega_b, quaternion_t current_q_i2b){
    _set_sat_vector(&current_omega_b, dat_ads_ekf_omega_x);
    _set_sat_quaterion(&current_q_i2b, dat_ads_ekf_q0);
    _mat_set_diag((double *) P_.m, 1.0, 10, 10);
    _mat_set_diag((double *) R_.m, 0.5 * 0.5, 7, 7);
    _mat_set_diag((double *) Q_.m, 1.0, 9, 9);
    _mat_set_diag((double *) I_x10.m, 1.0, 10, 10);

    H_.m[0][0] = 1; H_.m[1][1] = 1; H_.m[2][2] = 1;
    H_.m[0][7] = 1; H_.m[1][8] = 1; H_.m[2][9] = 1;

    H_.m[3][3] = 1; H_.m[4][4] = 1; H_.m[5][5] = 1; H_.m[6][6] = 1;
}

void attitude_discrete_model(quaternion_t current_q_i2b_det, vector3_t current_omega_b,
                             vector3_t torque_b, double dt, quaternion_t *next_q_i2b, vector3_t *next_omega_b,
                             matrix4_t * Omega4x, matrix3_t * sk_matrix) {

    matrix3_t S_omega = skewsymmetricmatrix(current_omega_b);
    matrix4_t Omega = omega4kinematics(current_omega_b);
    vector3_t current_omega_rw = {0, 0, 0};

    mat_cons_mult(1.0, &S_omega, sk_matrix);
    _mat_cons_mult(1.0, (double *) Omega.m, (double *) Omega4x->m, 4, 4);

    matrix3_t Inertia_ekf;
    matrix3_t inv_Inertia_ekf;
    get_sc_inertia_matrix(&Inertia_ekf, &inv_Inertia_ekf);

    matrix3_t rwInertia_ekf = get_rw_inertia_matrix();

    vector3_t h_sc_b;
    vector3_t h_rw_b;
    mat_vec_mult(Inertia_ekf, current_omega_b, &h_sc_b);
    mat_vec_mult(rwInertia_ekf, current_omega_rw, &h_rw_b);
    vector3_t h_total_b;
    vec_sum(h_sc_b, h_rw_b, &h_total_b);

    vector3_t aux_m;
    vector3_t temp1;
    vector3_t temp2;
    mat_vec_mult(S_omega, h_total_b, &aux_m);
    vec_cons_mult(-1.0, &torque_b, NULL);
    vec_sum(aux_m, torque_b, &temp1);
    mat_vec_mult(inv_Inertia_ekf, temp1, &temp2);
    vec_cons_mult(-1 * dt, &temp2, NULL);
    vec_sum(current_omega_b, temp2, next_omega_b);

    quaternion_t dq_temp1;
    mat_quat_mult(Omega, current_q_i2b_det, &dq_temp1);
    quat_cons_mult(0.5 * dt, &dq_temp1, NULL);

    quat_sum(&dq_temp1, &current_q_i2b_det, next_q_i2b);
    quat_normalize(next_q_i2b, NULL);
}

void get_sc_inertia_matrix(matrix3_t * inertia, matrix3_t * inv_inertia){
    vector3_t temp1;
    _get_sat_vector(&temp1, dat_inertia_xx);
    vector3_t temp2;
    _get_sat_vector(&temp2, dat_inertia_xy);

    inertia->m[0][0] = temp1.v[0] * 1e-6;
    inertia->m[1][1] = temp1.v[1] * 1e-6;
    inertia->m[2][2] = temp1.v[2] * 1e-6;

    inertia->m[0][1] = temp2.v[0] * 1e-6;
    inertia->m[0][2] = temp2.v[1] * 1e-6;

    inertia->m[1][0] = temp2.v[0] * 1e-6;
    inertia->m[1][2] = temp2.v[2] * 1e-6;

    inertia->m[2][0] = temp2.v[1] * 1e-6;
    inertia->m[2][1] = temp2.v[2] * 1e-6;

    vector3_t temp3;
    _get_sat_vector(&temp3, dat_inv_inertia_xx);
    vector3_t temp4;
    _get_sat_vector(&temp4, dat_inv_inertia_xy);

    inv_inertia->m[0][0] = temp3.v[0] * 1e6;
    inv_inertia->m[1][1] = temp3.v[1] * 1e6;
    inv_inertia->m[2][2] = temp3.v[2] * 1e6;

    inv_inertia->m[0][1] = temp4.v[0] * 1e6;
    inv_inertia->m[0][2] = temp4.v[1] * 1e6;

    inv_inertia->m[1][0] = temp4.v[0] * 1e6;
    inv_inertia->m[1][2] = temp4.v[2] * 1e6;

    inv_inertia->m[2][0] = temp4.v[1] * 1e6;
    inv_inertia->m[2][1] = temp4.v[2] * 1e6;

}

matrix3_t get_rw_inertia_matrix(){
    matrix3_t inertia;
    double irw = dat_get_status_var(dat_inertia_rw).f;
    mat_set_diag(&inertia, irw, irw, irw);
    return inertia;
}

matrix4_t omega4kinematics(vector3_t x_omega_b){
    matrix4_t Omega;
    Omega.m[1][0] = -x_omega_b.v[2];
    Omega.m[2][0] = x_omega_b.v[1];
    Omega.m[3][0] = -x_omega_b.v[0];

    Omega.m[0][1] = x_omega_b.v[2];
    Omega.m[0][2] = -x_omega_b.v[1];
    Omega.m[0][3] = x_omega_b.v[0];

    Omega.m[1][2] = x_omega_b.v[0];
    Omega.m[1][3] = x_omega_b.v[1];

    Omega.m[2][1] = -x_omega_b.v[0];
    Omega.m[2][3] = x_omega_b.v[2];

    Omega.m[3][1] = -x_omega_b.v[1];
    Omega.m[3][2] = -x_omega_b.v[2];

    Omega.m[0][0] = 0.0;
    Omega.m[1][1] = 0.0;
    Omega.m[2][2] = 0.0;
    Omega.m[3][3] = 0.0;
    return Omega;
}

matrix3_t skewsymmetricmatrix(vector3_t vector_3x3){
    matrix3_t omega_sk;
    omega_sk.m[1][0] = vector_3x3.v[2];
    omega_sk.m[2][0] = -vector_3x3.v[1];

    omega_sk.m[0][1] = -vector_3x3.v[2];
    omega_sk.m[0][2] = vector_3x3.v[1];

    omega_sk.m[2][1] = vector_3x3.v[0];
    omega_sk.m[1][2] = -vector_3x3.v[0];

    omega_sk.m[0][0] = 0;
    omega_sk.m[1][1] = 0;
    omega_sk.m[2][2] = 0;
    return omega_sk;
}

int determine_quaternion_triadekf(int isdark, vector3_t mag_b, vector3_t mag_i, vector3_t sun_b, vector3_t sun_from_sc_i, quaternion_t * quat_i2b)
{
    vector3_t unit_sun_vector_b;
    vec_normalize(&sun_b, &unit_sun_vector_b);
    vector3_t unit_mag_vector_b;
    vec_normalize(&mag_b, &unit_mag_vector_b);

    vector3_t b_times;
    vec_outer_product(unit_sun_vector_b, unit_mag_vector_b, &b_times);
    vec_normalize(&b_times, NULL);

    vector3_t unit_sun_from_sc_i;
    vec_normalize(&sun_from_sc_i, &unit_sun_from_sc_i);
    vector3_t unit_mag_earth_i;
    vec_normalize(&mag_i, &unit_mag_earth_i);

    vector3_t r_times;
    vec_outer_product(unit_sun_from_sc_i, unit_mag_earth_i, &r_times);
    vec_normalize(&r_times, NULL);

    quaternion_t  q_temp;
    //q_temp = calc_with_wahbas_problem(unit_sun_vector_b, unit_mag_vector_b, unit_mag_earth_i, unit_sun_from_sc_i, b_times,
    //                                  r_times);
    q_temp = calc_with_triad(unit_sun_vector_b, unit_mag_vector_b, unit_mag_earth_i, unit_sun_from_sc_i, b_times,
                             r_times);

    quat_i2b->q0 = q_temp.q0;
    quat_i2b->q1 = q_temp.q1;
    quat_i2b->q2 = q_temp.q2;
    quat_i2b->q3 = q_temp.q3;
    return CMD_OK;
}

void estimate_omega_quat_with_triadekf(quaternion_t current_q_det, vector3_t current_omega_b, portTick delay_ms,
                            quaternion_t * current_q_est_i2c, vector3_t * current_omega_b_est){

    vector3_t torque_b = {0 ,0, 0};
    double dt = (double) delay_ms * 0.001;
    quaternion_t pred_q_i2b = {0,0,0,1};
    vector3_t pred_omega_b = {0,0,0};
    quaternion_t last_pred_q_i2b = {0,0,0,1};
    vector3_t last_pred_omega_b = {0,0,0};
    vector3_t pred_bias_b = {0,0,0};
    vector7_t z_observer = {0,0,0,0,0,0,0};
    vector7_t error_state = {0,0,0,0,0,0,0};;
    matrix3_t sk_omega_matrix;
    matrix4_t Omega4x;
    matrix10_t F_j;
    matrix10_9_t L_j;
    matrix10_t P_j;
    matrix7_t S_j;
    matrix10_7_t K_j;

    printf("Init ekf:ok\n");
    _get_sat_vector(&pred_bias_b, dat_ads_ekf_bias_x);
    printf("charge bias_b:ok\n");
    _mat_set_diag((double *) F_j.m, 0, 10, 10);
    printf("Init F\n");
    _mat_set_diag((double *) L_j.m, 0, 10, 9);
    printf("Init L\n");
    _mat_set_diag((double *) P_j.m, 0, 10, 10);
    printf("Init P total\n");
    // init state
    _get_sat_vector(&last_pred_omega_b, dat_ads_ekf_omega_x);
    printf("TRIAD: Omega pred\n");
    _get_sat_quaterion(&last_pred_q_i2b, dat_ads_ekf_q0);
    printf("Init STATE:ok\n");
    // Propagate model
    attitude_discrete_model(last_pred_q_i2b, last_pred_omega_b, torque_b, dt, &pred_q_i2b,
                            &pred_omega_b, &Omega4x, &sk_omega_matrix);
    printf("Propagate model :ok\n");
    // Propagate P
    propagate_P_matrix(last_pred_q_i2b, last_pred_omega_b, Omega4x, sk_omega_matrix, dt, &F_j, &L_j, &P_j);

    // GET observer and measure
    z_observer = get_observer_model(pred_omega_b, pred_q_i2b, pred_bias_b);
    vector7_t measure_vector;
    printf("Observer :ok\n");
    measure_vector = get_measure_as_vector(current_omega_b, current_q_det);

    // Update Covariance matrix S
    S_j = update_covariance_matrix(P_j);
    printf("S_j :ok\n");

    // Calc Kalman Gain and error
    K_j = calc_kalman_gain(P_j, S_j);
    printf("K_j :ok\n");
    vec_sum_v7(measure_vector, z_observer, &error_state);
    printf("error_state :ok\n");

    // update state
    vector3_t omega_ekf;
    quaternion_t q_ekf;
    vector3_t bias_ekf;
    double temp_kf[10];
    _mat_vec_mult((double *) K_j.m, error_state.v, temp_kf, 10, 7);
    printf("NEW VECTOR STATE :ok\n");
    omega_ekf.v[0] = temp_kf[0]; omega_ekf.v[1] = temp_kf[1]; omega_ekf.v[2] = temp_kf[2];
    q_ekf.q[0] = temp_kf[3]; q_ekf.q[1] = temp_kf[4]; q_ekf.q[2] = temp_kf[5]; q_ekf.q[3] = temp_kf[6];
    bias_ekf.v[0] = temp_kf[7]; bias_ekf.v[1] = temp_kf[8]; bias_ekf.v[2] = temp_kf[9];

    // save vector
    vector3_t next_omega_est;
    quaternion_t next_q_i2b_est;
    vector3_t next_bias_est;
    vec_sum(pred_omega_b, omega_ekf, &next_omega_est);
    quat_sum(&pred_q_i2b, &q_ekf, &next_q_i2b_est);
    vec_sum(pred_bias_b, bias_ekf, &next_bias_est);

    // save ESTIMATION
    current_omega_b_est->v[0] = next_omega_est.v[0];
    current_omega_b_est->v[1] = next_omega_est.v[1];
    current_omega_b_est->v[2] = next_omega_est.v[2];

    current_q_est_i2c->q[0] = next_q_i2b_est.q[0];
    current_q_est_i2c->q[1] = next_q_i2b_est.q[1];
    current_q_est_i2c->q[2] = next_q_i2b_est.q[2];
    current_q_est_i2c->q[3] = next_q_i2b_est.q[3];

    _set_sat_vector(&next_bias_est, dat_ads_ekf_bias_x);

    // Update P
    update_covariance_P_matrix(K_j, P_j);
}
void propagate_P_matrix(quaternion_t last_pred_q_i2b, vector3_t last_pred_omega_b, matrix4_t Omega4x, matrix3_t sk_omega_matrix,
                        double dt, matrix10_t * F_j, matrix10_9_t * L_j, matrix10_t * P_j){
    matrix10_t P_j_1;
    matrix10_t P_j_2;
    get_attitude_jacobian_model(last_pred_q_i2b, last_pred_omega_b, Omega4x, sk_omega_matrix, dt, F_j, L_j);
    printf("Propagate P:ok\n");
    matrix10_t tempP;
    _mat_mat_mult((double *) F_j->m, (double *) P_.m, (double *) &tempP.m, 10, 10, 10);
    matrix10_t F_j_T;
    printf("Transpose FJ:ok\n");
    _mat_transpose((double *) F_j->m, (double *) &F_j_T.m, 10, 10);
    _mat_mat_mult((double *) tempP.m, (double *) F_j_T.m, (double *) &P_j_1.m, 10, 10, 10);

    matrix10_9_t tempL;
    matrix9_10_t L_j_T;
    printf("Transpose LJ:ok\n");
    _mat_transpose((double *) L_j->m, (double *) &L_j_T.m, 10, 9);
    _mat_mat_mult((double *) L_j->m, (double *) Q_.m, (double *) &tempL.m, 10, 9, 9);
    _mat_mat_mult((double *) tempL.m, (double *) L_j_T.m, (double *) &P_j_2.m, 10, 9, 10);

    _mat_mat_sum((double *) P_j_1.m, (double *) P_j_2.m, (double *) P_j->m, 10, 10);
}

void update_covariance_P_matrix(matrix10_7_t K_j_, matrix10_t P_j_){
    double temp_kH[10][10];
    matrix10_t next_P;
    _mat_mat_mult((double *) K_j_.m, (double *) H_.m, (double *) temp_kH, 10, 7, 10);
    // mult by -1
    for (int i=0; i<10; i++){
        for (int j=0; j<10; j++){
            temp_kH[i][j] = -1 * temp_kH[i][j];
        }
    }
    double temp1[10][10];
    _mat_mat_sum((double *) I_x10.m, (double *) temp_kH, (double *) temp1, 10, 10);
    _mat_mat_mult((double *) temp1, (double *) P_j_.m, (double *) P_.m, 10, 10, 10);
}

vector7_t get_measure_as_vector(vector3_t current_omega_b_, quaternion_t current_q_det_){
    vector7_t temp1;
    temp1.v[0] = current_omega_b_.v[0];
    temp1.v[1] = current_omega_b_.v[1];
    temp1.v[2] = current_omega_b_.v[2];

    temp1.v[3] = current_q_det_.q[0];
    temp1.v[4] = current_q_det_.q[1];
    temp1.v[5] = current_q_det_.q[2];
    temp1.v[6] = current_q_det_.q[3];
    return temp1;
}

void vec_sum_v7(vector7_t measure_vector, vector7_t z_observer, vector7_t * error_state){
    for (int i = 0; i<7; i++){
        error_state->v[i] = measure_vector.v[i] + z_observer.v[i];
    }
}

matrix10_7_t calc_kalman_gain(matrix10_t P_j, matrix7_t S_j){
    matrix10_7_t k_j;
    double H_T[10][7];
    printf("HT\n");
    _mat_transpose((double *) H_.m, (double *) H_T, 7, 10);
    double temp1[10][7];
    printf("HT: ok\n");
    _mat_mat_mult((double *) P_j.m, (double *) H_T, (double *) temp1, 10, 10, 7);
    matrix7_t S_j_i;
    printf("inverse S_j ....\n");
    calc_inverse_matrix(S_j, &S_j_i);
    printf("S_j_i: ok\n");
    _mat_mat_mult((double *) temp1, (double *) S_j_i.m, (double *) &k_j.m, 10, 7, 7);
    return k_j;
}

matrix7_t update_covariance_matrix(matrix10_t p_j){
    matrix7_t s_temp;
    double H_T[10][7];
    _mat_transpose((double *) H_.m, (double *) H_T, 7, 10);

    double temp1[7][10];
    _mat_mat_mult((double *) H_.m, (double *) p_j.m, (double *) temp1, 7, 10, 10);
    double temp2[7][7];
    _mat_mat_mult((double *) temp1, (double *) H_T, (double *) temp2, 7, 10, 7);
    _mat_mat_sum((double *) temp2, (double *) R_.m, (double *) &s_temp.m, 7, 7);
    return s_temp;
}

vector7_t get_observer_model(vector3_t omega_b, quaternion_t q_i2b, vector3_t bias_b){
    vector7_t temp_z;
    temp_z.v[0] = -1 * (omega_b.v[0] + bias_b.v[0]);
    temp_z.v[1] = -1 * (omega_b.v[1] + bias_b.v[1]);
    temp_z.v[2] = -1 * (omega_b.v[2] + bias_b.v[2]);

    temp_z.v[3] = -1 * q_i2b.q[0];
    temp_z.v[4] = -1 * q_i2b.q[1];
    temp_z.v[5] = -1 * q_i2b.q[2];
    temp_z.v[6] = -1 * q_i2b.q[3];
    return temp_z;
}

void get_attitude_jacobian_model(quaternion_t q_i2c, vector3_t omega_b, matrix4_t Omega4x, matrix3_t sk_omega_matrix,
                                 double dt, matrix10_t * F_j, matrix10_9_t * L_j) {
    matrix3_t f_j_1;
    matrix3_4_t f_j_2_omega;
    matrix4_t f_j_2_quat;
    matrix3_t f_j_3;
    matrix3_t aux_m;
    matrix3_t temp2;
    matrix3_t temp3;
    matrix3_t identity3x;
    matrix3_t Inertia_ekf;
    matrix3_t inv_Inertia_ekf;
    matrix4_t identity4x;
    vector3_t temp1;

    get_sc_inertia_matrix(&Inertia_ekf, &inv_Inertia_ekf);

    mat_set_diag(&identity3x, 1.0, 1.0, 1.0);
    mat_set_diag4x(&identity4x, 1.0, 1.0, 1.0, 1.0);

    mat_mat_mult(sk_omega_matrix, Inertia_ekf, &aux_m);
    mat_vec_mult(Inertia_ekf, omega_b, &temp1);
    matrix3_t sk_h_moment = skewsymmetricmatrix(temp1);
    mat_cons_mult(-1.0, &sk_h_moment, NULL);
    mat_mat_sum(aux_m, sk_h_moment, &temp2);
    mat_mat_mult(inv_Inertia_ekf, temp2, &temp3);
    mat_cons_mult(-1 * dt, &temp3, NULL);
    mat_mat_sum(identity3x, temp3, &f_j_1);

    f_j_2_omega.row0[0] = q_i2c.q[3] * 0.5 * dt; f_j_2_omega.row0[1] = -q_i2c.q[2] * 0.5 * dt; f_j_2_omega.row0[2] = q_i2c.q[1] * 0.5 * dt;
    f_j_2_omega.row1[0] = q_i2c.q[2] * 0.5 * dt; f_j_2_omega.row1[1] = q_i2c.q[3] * 0.5 * dt; f_j_2_omega.row1[2] = -q_i2c.q[0] * 0.5 * dt;
    f_j_2_omega.row2[0] = -q_i2c.q[1] * 0.5 * dt; f_j_2_omega.row2[1] = q_i2c.q[0] * 0.5 * dt; f_j_2_omega.row2[2] = q_i2c.q[3] * 0.5 * dt;
    f_j_2_omega.row3[0] = -q_i2c.q[0] * 0.5 * dt; f_j_2_omega.row3[1] = -q_i2c.q[1] * 0.5 * dt; f_j_2_omega.row3[2] = -q_i2c.q[2] * 0.5 * dt;

    _mat_cons_mult(0.5 * dt, (double *) Omega4x.m, (double *) &f_j_2_quat.m, 4, 4);

    _mat_mat_sum((double *) identity4x.m, (double *) f_j_2_quat.m, (double *) &f_j_2_quat.m, 4, 4);
    mat_cons_mult(1, &identity3x, &f_j_3);

    F_j->m[0][0] = f_j_1.m[0][0]; F_j->m[0][1] = f_j_1.m[0][1]; F_j->m[0][2] = f_j_1.m[0][2];
    F_j->m[1][0] = f_j_1.m[1][0]; F_j->m[1][1] = f_j_1.m[1][1]; F_j->m[1][2] = f_j_1.m[1][2];
    F_j->m[2][0] = f_j_1.m[2][0]; F_j->m[2][1] = f_j_1.m[2][1]; F_j->m[2][2] = f_j_1.m[2][2];

    F_j->m[3][0] = f_j_2_omega.m[0][0]; F_j->m[3][1] = f_j_2_omega.m[0][1]; F_j->m[3][2] = f_j_2_omega.m[0][2];
    F_j->m[4][0] = f_j_2_omega.m[1][0]; F_j->m[4][1] = f_j_2_omega.m[1][1]; F_j->m[4][2] = f_j_2_omega.m[1][2];
    F_j->m[5][0] = f_j_2_omega.m[2][0]; F_j->m[5][1] = f_j_2_omega.m[2][1]; F_j->m[5][2] = f_j_2_omega.m[2][2];
    F_j->m[6][0] = f_j_2_omega.m[3][0]; F_j->m[6][1] = f_j_2_omega.m[3][1]; F_j->m[6][2] = f_j_2_omega.m[3][2];

    F_j->m[3][3] = f_j_2_quat.m[0][0]; F_j->m[3][4] = f_j_2_quat.m[0][1]; F_j->m[3][5] = f_j_2_quat.m[0][2]; F_j->m[3][6] = f_j_2_quat.m[0][3];
    F_j->m[4][3] = f_j_2_quat.m[1][0]; F_j->m[4][4] = f_j_2_quat.m[1][1]; F_j->m[4][5] = f_j_2_quat.m[1][2]; F_j->m[4][6] = f_j_2_quat.m[1][3];
    F_j->m[5][3] = f_j_2_quat.m[2][0]; F_j->m[5][4] = f_j_2_quat.m[2][1]; F_j->m[5][5] = f_j_2_quat.m[2][2]; F_j->m[5][6] = f_j_2_quat.m[2][3];
    F_j->m[6][3] = f_j_2_quat.m[3][0]; F_j->m[6][4] = f_j_2_quat.m[3][1]; F_j->m[6][5] = f_j_2_quat.m[3][2]; F_j->m[6][6] = f_j_2_quat.m[3][3];

    F_j->m[7][7] = 1.0; F_j->m[8][8] = 1.0; F_j->m[9][9] = 1.0;

    L_j->m[0][0] = dt; L_j->m[1][1] = dt; L_j->m[2][2] = dt;

    L_j->m[3][3] = f_j_2_omega.m[0][0]; L_j->m[3][4] = f_j_2_omega.m[0][1]; L_j->m[3][5] = f_j_2_omega.m[0][2];
    L_j->m[4][3] = f_j_2_omega.m[1][0]; L_j->m[4][4] = f_j_2_omega.m[1][1]; L_j->m[4][5] = f_j_2_omega.m[1][2];
    L_j->m[5][3] = f_j_2_omega.m[2][0]; L_j->m[5][4] = f_j_2_omega.m[2][1]; L_j->m[5][5] = f_j_2_omega.m[2][2];
    L_j->m[6][3] = f_j_2_omega.m[3][0]; L_j->m[6][4] = f_j_2_omega.m[3][1]; L_j->m[6][5] = f_j_2_omega.m[3][2];

    L_j->m[7][6] = dt; L_j->m[8][7] = dt; L_j->m[9][8] = dt;
}


quaternion_t calc_with_triad(vector3_t unit_sun_vector_b, vector3_t unit_mag_vector_b,
                             vector3_t unit_mag_earth_i, vector3_t unit_sun_from_sc_i, vector3_t b_times,
                             vector3_t r_times){
    // Triad frame @ body frame
    vector3_t t1b = unit_sun_vector_b;
    vector3_t t2b = b_times;
    vector3_t t3b;
    vec_outer_product(t1b, t2b, &t3b);

    // Triad frame @ inertial frame
    vector3_t t1r = unit_sun_from_sc_i;
    vector3_t t2r = r_times;
    vector3_t t3r;
    vec_outer_product(t1r, t2r, &t3r);

    quaternion_t q_;
    // Matrix construction
    matrix3_t BT;
    BT.m[0][0] = t1b.v0;
    BT.m[1][0] = t1b.v1;
    BT.m[2][0] = t1b.v2;

    BT.m[0][1] = t2b.v0;
    BT.m[1][1] = t2b.v1;
    BT.m[2][1] = t2b.v2;

    BT.m[0][2] = t3b.v0;
    BT.m[1][2] = t3b.v1;
    BT.m[2][2] = t3b.v2;

    matrix3_t NT;
    NT.m[0][0] = t1r.v0;
    NT.m[0][1] = t1r.v1;
    NT.m[0][2] = t1r.v2;

    NT.m[1][0] = t2r.v0;
    NT.m[1][1] = t2r.v1;
    NT.m[1][2] = t2r.v2;

    NT.m[2][0] = t3r.v0;
    NT.m[2][1] = t3r.v1;
    NT.m[2][2] = t3r.v2;

    matrix3_t CM;
    mat_mat_mult(BT, NT, &CM);

    q_ = quat_from_dcm(CM);
    return q_;
}

void estimate_omega_with_triadekf(quaternion_t current_q_det, vector3_t current_omega_b, portTick delay_ms,
                                       quaternion_t * current_q_est_i2c, vector3_t * current_omega_b_est){

    vector3_t torque_b = {0 ,0, 0};
    double dt = (double) delay_ms * 0.001;
    quaternion_t pred_q_i2b = {0,0,0,1};
    vector3_t pred_omega_b = {0,0,0};
    quaternion_t last_pred_q_i2b = {0,0,0,1};
    vector3_t last_pred_omega_b = {0,0,0};
    vector3_t pred_bias_b = {0,0,0};
    vector7_t z_observer = {0,0,0,0,0,0,0};
    vector7_t error_state = {0,0,0,0,0,0,0};;
    matrix3_t sk_omega_matrix;
    matrix4_t Omega4x;
    matrix10_t F_j;
    matrix10_9_t L_j;
    matrix10_t P_j_1;
    matrix10_t P_j_2;
    matrix10_t P_j;
    matrix7_t S_j;
    matrix10_7_t K_j;

    printf("Init ekf:ok\n");
    _get_sat_vector(&pred_bias_b, dat_ads_ekf_bias_x);
    printf("charge bias_b:ok\n");
    _mat_set_diag((double *) F_j.m, 0, 10, 10);
    printf("Init F\n");
    _mat_set_diag((double *) L_j.m, 0, 10, 9);
    printf("Init L\n");
    _mat_set_diag((double *) P_j_1.m, 0, 10, 10);
    printf("Init P1 \n");
    _mat_set_diag((double *) P_j_2.m, 0, 10, 10);
    printf("Init P2\n");
    _mat_set_diag((double *) P_j.m, 0, 10, 10);
    printf("Init P total\n");
    // init state
    _get_sat_vector(&last_pred_omega_b, dat_ads_ekf_omega_x);
    printf("TRIAD: Omega pred\n");
    _get_sat_quaterion(&last_pred_q_i2b, dat_ads_ekf_q0);
    printf("Init STATE:ok\n");
    // Propagate model
    attitude_discrete_model(last_pred_q_i2b, last_pred_omega_b, torque_b, dt, &pred_q_i2b,
                            &pred_omega_b, &Omega4x, &sk_omega_matrix);
    printf("Propagate model :ok\n");
    // Propagate P
    get_attitude_jacobian_model(last_pred_q_i2b, last_pred_omega_b, Omega4x, sk_omega_matrix, dt, &F_j, &L_j);
    printf("Propagate P:ok\n");
    matrix10_t tempP;
    _mat_mat_mult((double *) F_j.m, (double *) P_.m, (double *) &tempP.m, 10, 10, 10);
    matrix10_t F_j_T;
    _mat_transpose((double *) F_j.m, (double *) &F_j_T.m, 10, 10);
    _mat_mat_mult((double *) tempP.m, (double *) F_j_T.m, (double *) &P_j_1.m, 10, 10, 10);

    matrix10_9_t tempL;
    matrix9_10_t L_j_T;

    _mat_transpose((double *) L_j.m, (double *) &L_j_T.m, 10, 9);
    _mat_mat_mult((double *) L_j.m, (double *) Q_.m, (double *) &tempL.m, 10, 9, 9);
    _mat_mat_mult((double *) tempL.m, (double *) L_j_T.m, (double *) &P_j_2.m, 10, 9, 10);

    _mat_mat_sum((double *) P_j_1.m, (double *) P_j_2.m, (double *) &P_j.m, 10, 10);

    // GET observer and measure
    z_observer = get_observer_model(pred_omega_b, pred_q_i2b, pred_bias_b);
    vector7_t measure_vector;
    measure_vector = get_measure_as_vector(current_omega_b, current_q_det);

    // Update Covariance matrix S
    S_j = update_covariance_matrix(P_j);

    // Calc Kalman Gain and error
    K_j = calc_kalman_gain(P_j, S_j);
    vec_sum_v7(measure_vector, z_observer, &error_state);

    // update state
    vector3_t omega_ekf;
    quaternion_t q_ekf;
    vector3_t bias_ekf;
    double temp_kf[10];
    _mat_vec_mult((double *) K_j.m, error_state.v, temp_kf, 10, 7);

    omega_ekf.v[0] = temp_kf[0]; omega_ekf.v[1] = temp_kf[1]; omega_ekf.v[2] = temp_kf[2];
    q_ekf.q[0] = temp_kf[3]; q_ekf.q[1] = temp_kf[4]; q_ekf.q[2] = temp_kf[5]; q_ekf.q[3] = temp_kf[6];
    bias_ekf.v[0] = temp_kf[7]; bias_ekf.v[1] = temp_kf[8]; bias_ekf.v[2] = temp_kf[9];

    // save vector
    vector3_t next_omega_est;
    quaternion_t next_q_i2b_est;
    vector3_t next_bias_est;
    vec_sum(pred_omega_b, omega_ekf, &next_omega_est);
    quat_sum(&pred_q_i2b, &q_ekf, &next_q_i2b_est);
    vec_sum(pred_bias_b, bias_ekf, &next_bias_est);

    // save ESTIMATION
    current_omega_b_est->v[0] = next_omega_est.v[0];
    current_omega_b_est->v[1] = next_omega_est.v[1];
    current_omega_b_est->v[2] = next_omega_est.v[2];

    current_q_est_i2c->q[0] = next_q_i2b_est.q[0];
    current_q_est_i2c->q[1] = next_q_i2b_est.q[1];
    current_q_est_i2c->q[2] = next_q_i2b_est.q[2];
    current_q_est_i2c->q[3] = next_q_i2b_est.q[3];

    _set_sat_vector(&next_bias_est, dat_ads_ekf_bias_x);

    // Update P
    update_covariance_P_matrix(K_j, P_j);
}

quaternion_t calc_with_wahbas_problem(vector3_t unit_sun_vector_b, vector3_t unit_mag_vector_b,
                                      vector3_t unit_mag_earth_i, vector3_t unit_sun_from_sc_i, vector3_t b_times,
                                      vector3_t r_times) {
    vector3_t r_times_temp;
    vec_cons_mult(-1, &r_times, &r_times_temp);
    int is_same = compare_vectors(b_times, r_times_temp);
    double r_times_norm = vec_norm(r_times);

    quaternion_t q_;
    if ((is_same != 3) && (r_times_norm >= 1e-4)) {
        double sigma_css = 5 * 930 * cos(2 * DEG2RAD);
        double sigma_mtm = 1e-3;
        double a1 = 0.6; //sigma_mtm ** 2 / (sigma_css ** 2 + sigma_mtm ** 2)
        double a2 = 1 - a1; // sigma_css ** 2 / (sigma_mtm ** 2 + sigma_css ** 2)

        double sun_b_dot_i = vec_inner_product(unit_sun_vector_b, unit_sun_from_sc_i);
        double mag_b_dot_i = vec_inner_product(unit_mag_vector_b, unit_mag_earth_i);
        //printf("%f", sun_b_dot_i);
        //printf("%f", mag_b_dot_i);

        vector3_t sun_b_cross_i;
        vec_outer_product(unit_sun_vector_b, unit_sun_from_sc_i, &sun_b_cross_i);

        vector3_t mag_b_cross_i;
        vec_outer_product(unit_mag_vector_b, unit_mag_earth_i, &mag_b_cross_i);

        vector3_t b_times_cross_r_times;
        vec_outer_product(b_times, r_times, &b_times_cross_r_times);
        double b_times_dot_r_times = vec_inner_product(b_times, r_times);

        vector3_t temp1;
        vec_cons_mult(a1, &sun_b_cross_i, &temp1);
        vector3_t temp2;
        vec_cons_mult(a2, &mag_b_cross_i, &temp2);
        vector3_t temp3;
        vec_sum(temp1, temp2, &temp3);

        double temp4 = vec_inner_product(b_times_cross_r_times, temp3);

        double alpha = (1 + b_times_dot_r_times) * (a1 * sun_b_dot_i + a2 * mag_b_dot_i) + temp4;
        vector3_t b_sum_r;
        vec_sum(b_times, r_times, &b_sum_r);
        double beta = vec_inner_product(b_sum_r, temp3);

        double gamma = sqrt(alpha * alpha + beta * beta);

        // wp_dcm = np.outer(b1_true, b2_true)
        if (alpha >= 0) {
            //print(alpha, beta, gamma)
            double temp = 1 / (2 * sqrt(gamma * (gamma + alpha) * (1 + b_times_dot_r_times)));
            vector3_t temp_vec;
            vec_cons_mult(beta, &b_sum_r, NULL);
            vec_sum(b_times_cross_r_times, b_sum_r, &temp_vec);

            q_.q1 = temp * (gamma + alpha) * temp_vec.v1;
            q_.q2 = temp * (gamma + alpha) * temp_vec.v2;
            q_.q0 = temp * (gamma + alpha) * temp_vec.v0;
            q_.q3 = temp * (gamma + alpha) * (1 + b_times_dot_r_times);
            quat_normalize(&q_, NULL);
        } else {
            //print(alpha, beta, gamma)
            double temp = 1 / (2 * sqrt(gamma * (gamma - alpha) * (1 + b_times_dot_r_times)));
            vector3_t temp_vec;
            vec_sum(b_times_cross_r_times, b_sum_r, &temp_vec);
            vec_cons_mult(beta, &temp_vec, NULL);

            q_.q1 = temp * (gamma - alpha) * temp_vec.v1;
            q_.q2 = temp * (gamma - alpha) * temp_vec.v2;
            q_.q0 = temp * (gamma - alpha) * temp_vec.v0;
            q_.q3 = temp * (gamma - alpha) * (1 + b_times_dot_r_times);
            quat_normalize(&q_, NULL);
        }
        return q_;
    }
}

void determine_quaternion_by_mtt(vector3_t current_mag_b, vector3_t current_mag_i, quaternion_t * q_det){
    vector3_t temp;
    vec_normalize(&current_mag_b, NULL);
    vector3_t unit_mag_earth_i;
    vec_normalize(&current_mag_i, NULL);
    vec_outer_product(current_mag_i, current_mag_b, &temp);
    vec_normalize(&temp, NULL);
    double rot = acos(vec_inner_product(current_mag_b, current_mag_i));
    printf("rot: %f \n", rot);
    q_det->q0 = temp.v0 * sin(rot * 0.5);
    q_det->q1 = temp.v1 * sin(rot * 0.5);
    q_det->q2 = temp.v2 * sin(rot * 0.5);
    q_det->q0 = cos(rot * 0.5);
    quat_normalize(q_det, NULL);
}

vector3_t test_transform_ypr(vector3_t vec_i, double rot_yaw, double rot_pitch, double rot_roll){
    vector3_t vec_b;
    vector3_t temp1;
    vector3_t temp2;

    rotation_yaw(vec_i, &temp1, rot_yaw);
    rotation_pitch(temp1, &temp2, rot_pitch);
    rotation_roll(temp2, &vec_b, rot_roll);
    //LOGI(tag, "(%f, %f, %f)", temp1.v0, temp1.v1, temp1.v2);
    //LOGI(tag, "(%f, %f, %f)", temp2.v0, temp2.v1, temp2.v2);

    return vec_b;
}

// Rotation function yaw-axis circumference [rad]
int rotation_yaw(vector3_t bfr, vector3_t * aft, double theta)
{
    double temp[3];
    theta = theta * DEG2RAD;
    temp[0] = cos(theta) * bfr.v[0] + sin(theta) * bfr.v[1];
    temp[1] = -sin(theta) * bfr.v[0] + cos(theta) * bfr.v[1];
    temp[2] = bfr.v[2];

    for (int i = 0; i < 3 ;i++) {
        aft->v[i] = temp[i];
    }

    return 0;
}

// Rotation function pitch-axis circumference [rad]
int rotation_pitch(vector3_t bfr, vector3_t * aft, double theta)
{
    double temp[3];
    theta = theta * DEG2RAD;
    temp[0] = cos(theta) * bfr.v[0] - sin(theta) * bfr.v[2];
    temp[1] = bfr.v[1];
    temp[2] = sin(theta) * bfr.v[0] + cos(theta) * bfr.v[2];

    for (int i = 0; i < 3 ;i++) {
        aft->v[i] = temp[i];
    }

    return 0;
}

// Rotation function roll-axis circumference [rad]
int rotation_roll(const vector3_t bfr, vector3_t * aft, double theta)
{
    double temp[3];
    theta = theta * DEG2RAD;
    temp[0] = bfr.v[0];
    temp[1] = cos(theta) * bfr.v[1] + sin(theta) * bfr.v[2];
    temp[2] = -sin(theta) * bfr.v[1] + cos(theta) * bfr.v[2];

    for (int i = 0; i < 3 ;i++) {
        aft->v[i] = temp[i];
    }

    return 0;
}

int compare_vectors(vector3_t v_a, vector3_t v_b){
    int value = 0;
    for (int i=0; i<3; i++){
        if (v_a.v[i] == v_b.v[i]){
            value++;
        }
    }
    return value;

}
