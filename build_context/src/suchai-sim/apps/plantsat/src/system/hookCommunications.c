//
// Created by spel on 15-07-21.
//

#include "suchai/taskCommunications.h"
#include "app/system/cmdCDH.h"

static char *tag = "Communications*";

#define SCH_TRX_PORT_CDH (SCH_TRX_PORT_APP+0)
#define SCH_TRX_PORT_STT (SCH_TRX_PORT_APP+1)  ///< 17 STT app port
#define STT_TYPE_GYRO 30 ///< STT Gyro telemetry type

#define SCH_2_COM_PORT_CDH 16  ///< SUCHAI 2 CDH app port
#define SCH_3_COM_PORT_CDH 17  ///< SUCHAI 3 CDH app port
#define SCH_P_COM_PORT_CDH 18  ///< PLANTSAT CDH app port

void parse_stt_data(csp_packet_t *packet);
void parse_mag_data(csp_packet_t *packet);
static void com_receive_cmdh_tm(csp_packet_t *packet);

void taskCommunicationsHook(csp_conn_t *conn, csp_packet_t *packet)
{
    switch (csp_conn_dport(conn))
    {
//        case SCH_TRX_PORT_CDH:
        case SCH_2_COM_PORT_CDH:
        case SCH_3_COM_PORT_CDH:
        case SCH_P_COM_PORT_CDH:
            // Process TM packet
            com_receive_cmdh_tm(packet);
            break;
        default:
            break;
    }
}

/**
 * Process a TM frame, determine TM type and call corresponding parsing command
 * @param packet a csp buffer containing a com_frame_t structure.
 */
void com_receive_cmdh_tm(csp_packet_t *packet) {
    cmd_t *cmd_parse_tm;
    com_frame_t *frame = (com_frame_t *) packet->data;

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
    if(frame->type == TM_TYPE_PAYLOAD_STA)
    {
        cmd_parse_tm = cmd_get_str("tm_parse_beacon");
        cmd_add_params_raw(cmd_parse_tm, frame, sizeof(com_frame_t));
        cmd_send(cmd_parse_tm);
    }
}