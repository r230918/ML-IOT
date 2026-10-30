#ifndef TELEMETRY_H
#define TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "safety_supervisor.h"

#define TELEMETRY_SYNC_WORD     0xAA55
#define TELEMETRY_PAYLOAD_SIZE  32

typedef struct __attribute__((packed)) {
    uint16_t sync_word;
    uint8_t  proto_ver;
    uint16_t variant_id;
    uint16_t run_id;
    uint8_t  condition_id;
    uint32_t timestamp_ms;
    uint8_t  motor_state;
    uint16_t pwm_duty;
    int16_t  current_ma;
    int16_t  speed_rpm;
    uint32_t encoder_pulses;
    uint16_t supply_mv;
    int16_t  temp_c_x10;
    uint8_t  last_reason_code;
    uint16_t crc16;
} TelemetryPacket_t;

typedef struct {
    HostRecommendation_t rec;
    uint16_t seq;
    uint16_t confidence_pct;
    uint16_t expiry_ms;
} HostCommandFrame_t;

void Telemetry_Init(void);
void Telemetry_SendFrame(uint16_t run_id, uint8_t condition_id, uint32_t now_ms, 
                         MotorSystemState_t state, uint16_t pwm, int16_t current_ma, 
                         int16_t speed_rpm, uint32_t encoder_pulses, uint16_t supply_mv, 
                         int16_t temp_c, AckReasonCode_t reason);

bool Telemetry_ParseByte(uint8_t ch, HostCommandFrame_t *out_cmd);

#ifdef __cplusplus
}
#endif

#endif /* TELEMETRY_H */
