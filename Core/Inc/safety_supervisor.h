#ifndef SAFETY_SUPERVISOR_H
#define SAFETY_SUPERVISOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    STATE_SAFE_REST         = 0,
    STATE_STARTUP_ARMED     = 1,
    STATE_STARTUP_RAMPING   = 2,
    STATE_RUNNING           = 3,
    STATE_DERATED           = 4,
    STATE_RETRY_BACKOFF     = 5,
    STATE_TRIP_SHUTDOWN     = 6
} MotorSystemState_t;

typedef enum {
    HOST_REC_NONE               = 0,
    HOST_REC_ACCEPT_START       = 1,
    HOST_REC_RETRY_SAFE_PROFILE = 2,
    HOST_REC_DERATE             = 3,
    HOST_REC_STOP               = 4
} HostRecommendation_t;

typedef enum {
    REASON_ACK_ACCEPTED         = 0,
    REASON_REJ_OVERCURRENT      = 1,
    REASON_REJ_SPEED_TIMEOUT    = 2,
    REASON_REJ_COMMAND_EXPIRED  = 3,
    REASON_REJ_HOST_TIMEOUT     = 4,
    REASON_REJ_UNDERVOLTAGE     = 5,
    REASON_REJ_OVERTEMP         = 6,
    REASON_REJ_INVALID_STATE    = 7
} AckReasonCode_t;

/* Hard Safety Limits (Deterministic Firmware Guard) */
#define HARD_LIMIT_MAX_CURRENT_MA       4000   /* 4.0 A absolute hardware trip (allows normal 3.2-3.5A startup inrush) */
#define HARD_LIMIT_STARTUP_TIMEOUT_MS    450   /* 450 ms time-to-speed limit */
#define HARD_LIMIT_MIN_SPEED_RPM         120   /* Minimum required speed after ramp */
#define HARD_LIMIT_MAX_TEMP_C             60   /* 60 deg C thermal shutdown */
#define HARD_LIMIT_MIN_SUPPLY_MV        6000   /* 6.0 V hard UVLO lockout */
#define HOST_HEARTBEAT_TIMEOUT_MS        350   /* Standalone fallback threshold */

void SafetySupervisor_Init(void);
void SafetySupervisor_ArmStart(void);
void SafetySupervisor_ManualStop(void);
void SafetySupervisor_Update(int16_t current_ma, int16_t speed_rpm, int16_t temp_c, uint16_t supply_mv, uint32_t now_ms);
bool SafetySupervisor_ProcessHostRecommendation(HostRecommendation_t rec, uint16_t seq, uint16_t confidence_pct, uint16_t expiry_ms, uint32_t now_ms);

MotorSystemState_t SafetySupervisor_GetState(void);
AckReasonCode_t SafetySupervisor_GetLastReason(void);
bool SafetySupervisor_IsTripped(void);
uint16_t SafetySupervisor_GetTargetPWM(void);
bool SafetySupervisor_IsHostConnected(uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* SAFETY_SUPERVISOR_H */
