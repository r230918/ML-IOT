#include "telemetry.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

extern UART_HandleTypeDef huart2;

static uint16_t CalculateCRC16(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

void Telemetry_Init(void) {
    /* Ready UART3 at 115200 8N1 */
}

void Telemetry_SendFrame(uint16_t run_id, uint8_t condition_id, uint32_t now_ms, 
                         MotorSystemState_t state, uint16_t pwm, int16_t current_ma, 
                         int16_t speed_rpm, uint32_t encoder_pulses, uint16_t supply_mv, 
                         int16_t temp_c, AckReasonCode_t reason) {
    TelemetryPacket_t pkt;
    pkt.sync_word = TELEMETRY_SYNC_WORD;
    pkt.proto_ver = 1;
    pkt.variant_id = VARIANT_ID_CODE;
    pkt.run_id = run_id;
    pkt.condition_id = condition_id;
    pkt.timestamp_ms = now_ms;
    pkt.motor_state = (uint8_t)state;
    pkt.pwm_duty = pwm;
    pkt.current_ma = current_ma;
    pkt.speed_rpm = speed_rpm;
    pkt.encoder_pulses = encoder_pulses;
    pkt.supply_mv = supply_mv;
    pkt.temp_c_x10 = (int16_t)(temp_c * 10);
    pkt.last_reason_code = (uint8_t)reason;

    /* Calculate CRC excluding CRC field */
    pkt.crc16 = CalculateCRC16((const uint8_t *)&pkt, sizeof(TelemetryPacket_t) - sizeof(uint16_t));

    /* Send binary packet */
    HAL_UART_Transmit(&huart2, (uint8_t *)&pkt, sizeof(TelemetryPacket_t), 10);

    /* Also emit formatted ASCII line for terminal readability */
    char buf[128];
    int len = snprintf(buf, sizeof(buf), 
        "$DAT,%u,%u,%lu,%u,%u,%d,%d,%lu,%u,%d,%u*\r\n",
        (unsigned int)run_id, (unsigned int)condition_id, (unsigned long)now_ms,
        (unsigned int)state, (unsigned int)pwm, (int)current_ma,
        (int)speed_rpm, (unsigned long)encoder_pulses, (unsigned int)supply_mv,
        (int)temp_c, (unsigned int)reason);
    if (len > 0) {
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, (uint16_t)len, 10);
    }
}

static char rxLineBuf[64];
static uint8_t rxLineIdx = 0;

bool Telemetry_ParseByte(uint8_t ch, HostCommandFrame_t *out_cmd) {
    if (ch == '\r' || ch == '\n') {
        if (rxLineIdx > 0) {
            rxLineBuf[rxLineIdx] = '\0';
            rxLineIdx = 0;

            /* Parse line: "CMD,<ACTION>,<SEQ>,<CONF>,<EXP>" */
            char act[24];
            unsigned int seq = 0, conf = 0, exp = 0;
            if (sscanf(rxLineBuf, "CMD,%23[^,],%u,%u,%u", act, &seq, &conf, &exp) == 4) {
                out_cmd->seq = (uint16_t)seq;
                out_cmd->confidence_pct = (uint16_t)conf;
                out_cmd->expiry_ms = (uint16_t)exp;

                if (strcmp(act, "ACCEPT_START") == 0) {
                    out_cmd->rec = HOST_REC_ACCEPT_START;
                    return true;
                } else if (strcmp(act, "RETRY_SAFE_PROFILE") == 0) {
                    out_cmd->rec = HOST_REC_RETRY_SAFE_PROFILE;
                    return true;
                } else if (strcmp(act, "DERATE") == 0) {
                    out_cmd->rec = HOST_REC_DERATE;
                    return true;
                } else if (strcmp(act, "STOP") == 0) {
                    out_cmd->rec = HOST_REC_STOP;
                    return true;
                }
            }
        }
        return false;
    }

    if (rxLineIdx < sizeof(rxLineBuf) - 1) {
        rxLineBuf[rxLineIdx++] = (char)ch;
    } else {
        rxLineIdx = 0;
    }
    return false;
}
