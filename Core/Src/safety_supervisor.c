#include "safety_supervisor.h"

static MotorSystemState_t currentState = STATE_SAFE_REST;
static AckReasonCode_t lastReason = REASON_ACK_ACCEPTED;
static uint32_t stateEntryTimeMs = 0;
static uint32_t lastHostHeartbeatMs = 0;
static uint16_t currentTargetPWM = 0;
static uint8_t retryCount = 0;

void SafetySupervisor_Init(void) {
    currentState = STATE_SAFE_REST;
    lastReason = REASON_ACK_ACCEPTED;
    stateEntryTimeMs = 0;
    lastHostHeartbeatMs = 0;
    currentTargetPWM = 0;
    retryCount = 0;
}

void SafetySupervisor_ArmStart(void) {
    if (currentState == STATE_SAFE_REST || currentState == STATE_RETRY_BACKOFF || currentState == STATE_TRIP_SHUTDOWN) {
        currentState = STATE_STARTUP_ARMED;
        stateEntryTimeMs = 0;
        currentTargetPWM = 1000; /* Full 100% torque to guarantee startup even on low voltage USB */
        lastReason = REASON_ACK_ACCEPTED;
    }
}

void SafetySupervisor_ManualStop(void) {
    currentState = STATE_SAFE_REST;
    currentTargetPWM = 0;
    lastReason = REASON_ACK_ACCEPTED;
}

void SafetySupervisor_Update(int16_t current_ma, int16_t speed_rpm, int16_t temp_c, uint16_t supply_mv, uint32_t now_ms) {
    if (stateEntryTimeMs == 0 && (currentState == STATE_STARTUP_ARMED || currentState == STATE_STARTUP_RAMPING)) {
        stateEntryTimeMs = now_ms;
    }

    /* 1. HARD FAULT CHECK: Absolute Overcurrent Trip */
    if (current_ma > HARD_LIMIT_MAX_CURRENT_MA) {
        currentState = STATE_TRIP_SHUTDOWN;
        currentTargetPWM = 0;
        lastReason = REASON_REJ_OVERCURRENT;
        return;
    }

    /* 2. HARD FAULT CHECK: Thermal Cutoff */
    if (temp_c > HARD_LIMIT_MAX_TEMP_C) {
        currentState = STATE_TRIP_SHUTDOWN;
        currentTargetPWM = 0;
        lastReason = REASON_REJ_OVERTEMP;
        return;
    }

    /* 3. HARD FAULT CHECK: Undervoltage lockout */
    if (supply_mv < HARD_LIMIT_MIN_SUPPLY_MV && currentState != STATE_SAFE_REST) {
        currentState = STATE_TRIP_SHUTDOWN;
        currentTargetPWM = 0;
        lastReason = REASON_REJ_UNDERVOLTAGE;
        return;
    }

    /* 4. Host Heartbeat Loss (Standalone Inherited Safe State) */
    if (now_ms - lastHostHeartbeatMs > HOST_HEARTBEAT_TIMEOUT_MS && lastHostHeartbeatMs > 0) {
        if (currentState == STATE_RUNNING || currentState == STATE_DERATED) {
            /* Inherited safe state: derate gracefully to deterministic conservative speed */
            currentState = STATE_DERATED;
            currentTargetPWM = 400; /* 40% safe power */
            lastReason = REASON_REJ_HOST_TIMEOUT;
        }
    }

    /* 5. Startup Sequence Transition */
    if (currentState == STATE_STARTUP_ARMED) {
        currentState = STATE_STARTUP_RAMPING;
        stateEntryTimeMs = now_ms;
    } else if (currentState == STATE_STARTUP_RAMPING) {
        uint32_t elapsed = now_ms - stateEntryTimeMs;

        /* Ramp PWM smoothly towards standard target */
        if (currentTargetPWM < 800) {
            currentTargetPWM += 15;
            if (currentTargetPWM > 800) currentTargetPWM = 800;
        }

        /* Transition to RUNNING state once ramped, keeping motor spinning */
        if (elapsed > HARD_LIMIT_STARTUP_TIMEOUT_MS || currentTargetPWM >= 1000) {
            currentState = STATE_RUNNING;
            currentTargetPWM = 1000;
        }
    }
}

bool SafetySupervisor_ProcessHostRecommendation(HostRecommendation_t rec, uint16_t seq, uint16_t confidence_pct, uint16_t expiry_ms, uint32_t now_ms) {
    (void)seq;
    (void)confidence_pct;
    lastHostHeartbeatMs = now_ms;

    if (expiry_ms == 0 || expiry_ms < 50) {
        lastReason = REASON_REJ_COMMAND_EXPIRED;
        return false;
    }

    /* If MCU is currently tripped by hard limits, host cannot un-trip without physical reset */
    if (currentState == STATE_TRIP_SHUTDOWN) {
        lastReason = REASON_REJ_OVERCURRENT;
        return false;
    }

    switch (rec) {
        case HOST_REC_ACCEPT_START:
            if (currentState == STATE_STARTUP_RAMPING || currentState == STATE_RUNNING || currentState == STATE_DERATED) {
                currentState = STATE_RUNNING;
                currentTargetPWM = 1000; /* Full 100% PWM */
                lastReason = REASON_ACK_ACCEPTED;
                return true;
            }
            break;

        case HOST_REC_DERATE:
            if (currentState == STATE_RUNNING || currentState == STATE_STARTUP_RAMPING || currentState == STATE_DERATED) {
                currentState = STATE_DERATED;
                currentTargetPWM = 650; /* Safe derated power */
                lastReason = REASON_ACK_ACCEPTED;
                return true;
            }
            break;

        case HOST_REC_RETRY_SAFE_PROFILE:
            if (retryCount < 2) {
                retryCount++;
                currentState = STATE_RETRY_BACKOFF;
                currentTargetPWM = 250; /* Slower current rise profile */
                lastReason = REASON_ACK_ACCEPTED;
                return true;
            } else {
                currentState = STATE_TRIP_SHUTDOWN;
                currentTargetPWM = 0;
                lastReason = REASON_REJ_OVERCURRENT;
                return false;
            }

        case HOST_REC_STOP:
            currentState = STATE_TRIP_SHUTDOWN;
            currentTargetPWM = 0;
            lastReason = REASON_ACK_ACCEPTED;
            return true;

        default:
            lastReason = REASON_REJ_INVALID_STATE;
            return false;
    }

    lastReason = REASON_REJ_INVALID_STATE;
    return false;
}

MotorSystemState_t SafetySupervisor_GetState(void) {
    return currentState;
}

AckReasonCode_t SafetySupervisor_GetLastReason(void) {
    return lastReason;
}

bool SafetySupervisor_IsTripped(void) {
    return (currentState == STATE_TRIP_SHUTDOWN);
}

uint16_t SafetySupervisor_GetTargetPWM(void) {
    return currentTargetPWM;
}

bool SafetySupervisor_IsHostConnected(uint32_t now_ms) {
    return (lastHostHeartbeatMs > 0 && (now_ms - lastHostHeartbeatMs <= HOST_HEARTBEAT_TIMEOUT_MS));
}
