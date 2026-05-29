//
// Created by spel on 15-07-21.
//

#include "suchai/taskCommunications.h"
#include "app/system/cmdCDH.h"

static char *tag = "Communications*";

#define SCH_2_COM_PORT_CDH 16  ///< SUCHAI 2 CDH app port
#define SCH_3_COM_PORT_CDH 17  ///< SUCHAI 3 CDH app port
#define SCH_P_COM_PORT_CDH 18  ///< PLANTSAT CDH app port

#define SCH_2_COM_PORT_STT 19  ///< SUCHAI 2 STT app port
#define SCH_3_COM_PORT_STT 20  ///< SUCHAI 3 STT app port
#define SCH_P_COM_PORT_STT 21  ///< PLANTSAT STT app port

#define SCH_2_COM_PORT_GPS 22  ///< SUCHAI 2 GPS app port
#define SCH_3_COM_PORT_GPS 23  ///< SUCHAI 3 GPS app port
#define SCH_P_COM_PORT_GRA 24  ///< PLANTSAT GRAPHENE app port

#define SCH_2_COM_PORT_MAG 25  ///< SUCHAI 2 MAG app port
#define SCH_3_COM_PORT_MAG 26  ///< SUCHAI 3 MAG app port
#define SCH_P_COM_PORT_MAG 27  ///< PLANTSAT MAG app port

/**
 * This list maps space apps repoDataSchema payloads id to ground app repoDataSchema payloads id
 */
int PAYLOAD_ID_MAP[28] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          temp_sensors_2, temp_sensors_3, temp_sensors_P,
                          stt_temp_sensors_2, stt_temp_sensors_3, stt_temp_sensors_P,
                          gps_temp_sensors_2, gps_temp_sensors_3, gra_temp_sensors_P,
                          mag_temp_sensors_2, mag_temp_sensors_3, mag_temp_sensors_3};


int pay_parse_tm(char *data, int n_structs, int payload, data_map_t *pay_data_map);
void pay_parse_frame(csp_packet_t *packet, int app_id);
void com_receive_cmdh_tm(csp_packet_t *packet, int app_id);

void taskCommunicationsHook(csp_conn_t *conn, csp_packet_t *packet)
{
    switch (csp_conn_dport(conn))
    {
//        case SCH_TRX_PORT_CDH:
        case SCH_2_COM_PORT_CDH:
        case SCH_3_COM_PORT_CDH:
        case SCH_P_COM_PORT_CDH:
            // Process TM packet
            com_receive_cmdh_tm(packet, csp_conn_dport(conn));
            break;
        case SCH_2_COM_PORT_GPS:
        case SCH_3_COM_PORT_GPS:
            pay_parse_frame(packet, csp_conn_dport(conn));
            break;
        case SCH_P_COM_PORT_GRA:
            pay_parse_frame(packet, csp_conn_dport(conn));
            break;
        case SCH_2_COM_PORT_STT:
        case SCH_3_COM_PORT_STT:
        case SCH_P_COM_PORT_STT:
            pay_parse_frame(packet, csp_conn_dport(conn));
            break;
        case SCH_2_COM_PORT_MAG:
        case SCH_3_COM_PORT_MAG:
        case SCH_P_COM_PORT_MAG:
            pay_parse_frame(packet, csp_conn_dport(conn));
            break;
        default:
            break;
    }

}

/**
 * Receives payload telemetry frames
 * @param packet CSP packet with telemetry frame
 */
void pay_parse_frame(csp_packet_t *packet, int app_id)
{
    com_frame_t *frame = (com_frame_t *)packet->data;
    frame->nframe = csp_ntoh16(frame->nframe);
    frame->ndata = csp_ntoh32(frame->ndata);
    // Map sat payload id to ground payload id according to repoDataSchema
    uint8_t prev_type = frame->type;
    frame->type += PAYLOAD_ID_MAP[app_id];

    LOGI(tag, "Node    : %d", frame->node);
    LOGI(tag, "Type    : %d", prev_type);
    LOGI(tag, "Pay id  : %d->%d", prev_type-TM_TYPE_PAYLOAD, frame->type-TM_TYPE_PAYLOAD);
    LOGI(tag, "Frame   : %d", frame->nframe);
    LOGI(tag, "Samples : %d", frame->ndata);

    // Fix message payload endianness
    int pay_id = frame->type-TM_TYPE_PAYLOAD;
    if(pay_id == msg_sensors_2 || pay_id == msg_sensors_3 || pay_id == msg_sensors_P)
        _ntoh32_buff(frame->data.data32, sizeof(frame->data.data32)/ sizeof(uint32_t));

    cmd_t *cmd_parse_tm = cmd_get_str("tm_parse_payload");
    cmd_add_params_raw(cmd_parse_tm, frame, sizeof(com_frame_t));
    cmd_send(cmd_parse_tm);
}


/**
 * Process a TM frame, determine TM type and call corresponding parsing command
 * @param packet a csp buffer containing a com_frame_t structure.
 */
void com_receive_cmdh_tm(csp_packet_t *packet, int app_id) {
    cmd_t *cmd_parse_tm;
    com_frame_t *frame = (com_frame_t *) packet->data;

    if(frame->type >= TM_TYPE_PAYLOAD && frame->type < TM_TYPE_PAYLOAD+last_sensor)
    {
        pay_parse_frame(packet, app_id);
        return;
    }

    frame->nframe = csp_ntoh16(frame->nframe);
    frame->ndata = csp_ntoh32(frame->ndata);

    LOGI(tag, "Received: %d bytes", packet->length);
    LOGI(tag, "Node    : %d", frame->node);
    LOGI(tag, "Frame   : %d", frame->nframe);
    LOGI(tag, "Type    : %d", frame->type);
    LOGI(tag, "Samples : %d", frame->ndata);

    if(frame->type == TM_TYPE_STRING)
    {
        cmd_parse_tm = cmd_get_str("tm_parse_msg");
        cmd_add_params_raw(cmd_parse_tm, frame, sizeof(com_frame_t));
        cmd_send(cmd_parse_tm);
    }
    else if(frame->type == TM_TYPE_PAYLOAD_STA)
    {
        cmd_parse_tm = cmd_get_str("tm_parse_beacon");
        cmd_add_params_raw(cmd_parse_tm, frame, sizeof(com_frame_t));
        cmd_send(cmd_parse_tm);
    }
    else if(frame->type == TM_TYPE_STATUS)
    {
        cmd_parse_tm = cmd_get_str("tm_parse_status");
        cmd_add_params_raw(cmd_parse_tm, frame, sizeof(com_frame_t));
        cmd_send(cmd_parse_tm);
    }
    else if(frame->type == TM_TYPE_HELP)
    {
        cmd_parse_tm = cmd_get_str("tm_parse_string");
        cmd_add_params_raw(cmd_parse_tm, frame, sizeof(com_frame_t));
        cmd_send(cmd_parse_tm);
    }
    else if(frame->type == TM_TYPE_FP)
    {
        cmd_parse_tm = cmd_get_str("tm_print_fp");
        cmd_add_params_raw(cmd_parse_tm, frame, sizeof(com_frame_t));
        cmd_send(cmd_parse_tm);
    }
    else
    {
        LOGW(tag, "Undefined telemetry type %d!", frame->type);
        //Print raw data as bytes, int16, and ascii.
        //Do not use LOG functions after this line
        osSemaphoreTake(&log_mutex, portMAX_DELAY);
        print_buff(packet->data, packet->length);
        print_buff_fmt(packet->data32, packet->length/sizeof(uint32_t), "%d, ");
        print_buff_ascii(packet->data, packet->length);
        osSemaphoreGiven(&log_mutex);
    }

}
