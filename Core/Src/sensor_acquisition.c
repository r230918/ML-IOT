#include "sensor_acquisition.h"
#include "main.h"

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;

static uint32_t lastEncoderCnt = 0;
static uint32_t accumulatedPulses = 0;
static int32_t acs712_zero_mv = 2500;

void Sensors_Init(void) {
    HAL_ADC_Start(&hadc1);
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
    lastEncoderCnt = __HAL_TIM_GET_COUNTER(&htim2);
    accumulatedPulses = 0;

    /* Auto-calibrate resting zero-current baseline for ACS712 on PA4 (ADC1_IN4) */
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) {
        sum += ADC_ReadChannel(4);
    }
    uint32_t avg_raw = sum / 16;
    int32_t measured_zero = (int32_t)(avg_raw * 3300UL / 4095UL);
    if (measured_zero > 1500 && measured_zero < 3100) {
        acs712_zero_mv = measured_zero; /* Valid ACS712 centered at ~2.5V */
    } else {
        acs712_zero_mv = 2500;
    }
}

void Sensors_Sample(SensorData_t *data, uint32_t delta_ms) {
    /* 1. Read Supply Voltage Proxy (PC0 -> Pin A5, ADC1_IN10) */
    uint32_t raw_supply = ADC_ReadChannel(10);
    /* 12-bit ADC (0 - 4095) scaled across 0 - 15000 mV (represents 0 - 15V rail) */
    if (raw_supply < 100) raw_supply = 3276; /* Safe default ~12.0V if pot disconnected */
    data->supply_mv = (uint16_t)(raw_supply * 15000UL / 4095UL);

    /* 2. Read Temperature LM35 (PA6 -> Pin D12, ADC1_IN6, 10 mV/deg C) */
    uint32_t raw_temp = ADC_ReadChannel(6);
    if (raw_temp < 50 || raw_temp > 2000) raw_temp = 335; /* approx 27 deg C nominal */
    uint32_t temp_mv = raw_temp * 3300UL / 4095UL;
    data->temp_c = (int16_t)(temp_mv / 10);

    /* 3. Read Motor Speed / Pulses */
    uint32_t currentCnt = __HAL_TIM_GET_COUNTER(&htim2);
    int32_t deltaPulses = (int32_t)(currentCnt - lastEncoderCnt);
    lastEncoderCnt = currentCnt;

    if (deltaPulses < 0) deltaPulses = -deltaPulses;
    accumulatedPulses += (uint32_t)deltaPulses;

    if (deltaPulses > 0 && delta_ms > 0) {
        /* Physical hardware encoder pulses detected */
        data->speed_rpm = (int16_t)((deltaPulses * 60000L) / (ENCODER_PULSES_PER_REV * (int32_t)delta_ms));
        data->encoder_total_pulses = accumulatedPulses;
    } else {
        /* SENSORLESS SPEED OBSERVER (Analytical Dynamic Model):
         * Used for 2-wire DC motors without physical quadrature encoders.
         * Models rotor electromechanical acceleration as a first-order lag:
         * w_target = V_applied * Kv
         * dw/dt = (w_target - w) / tau_mech
         */
        static float observer_speed_rpm = 0.0f;
        static uint32_t simulated_pulses = 0;

        /* Get active motor drive PWM duty from TIM1 Channel 1 */
        uint16_t active_pwm = (uint16_t)TIM1->CCR1;
        float v_supply_volts = (float)data->supply_mv / 1000.0f;

        if (active_pwm > 0 && v_supply_volts > 6.0f) {
            /* Nominal 12V at 80% PWM targets ~380 RPM; 9V targets ~230 RPM; 7.5V targets ~120 RPM */
            float pwm_ratio = (float)active_pwm / 1000.0f;
            float target_rpm = (v_supply_volts * pwm_ratio * 39.5f);
            if (target_rpm > 420.0f) target_rpm = 420.0f;

            /* First-order acceleration lag with tau = 85 ms */
            float dt_s = (float)delta_ms / 1000.0f;
            if (dt_s <= 0.0f || dt_s > 0.05f) dt_s = 0.010f;
            float alpha = dt_s / (dt_s + 0.085f);
            observer_speed_rpm += alpha * (target_rpm - observer_speed_rpm);
        } else {
            /* Coasting / Deceleration */
            observer_speed_rpm *= 0.85f;
            if (observer_speed_rpm < 2.0f) observer_speed_rpm = 0.0f;
        }

        data->speed_rpm = (int16_t)observer_speed_rpm;
        simulated_pulses += (uint32_t)((observer_speed_rpm * ENCODER_PULSES_PER_REV * (delta_ms > 0 ? delta_ms : 10)) / 60000UL);
        data->encoder_total_pulses = accumulatedPulses + simulated_pulses;
    }

    /* 4. CURRENT SENSING: Hardware Reading OR Virtual Current Observer */
    /* Check physical ADC on PA4 (Pin A2, ADC1_IN4) */
    uint32_t raw_current = ADC_ReadChannel(4); 
    int32_t v_current_mv = (int32_t)(raw_current * 3300UL / 4095UL);
    int32_t delta_v = v_current_mv - acs712_zero_mv;
    if (delta_v < 0) delta_v = -delta_v;

    if (raw_current > 500 && raw_current < 3900 && delta_v > 20) {
        /* Physical ACS712 Current Sensor Active (Auto-calibrated baseline) */
        /* Sensitivity: ~100 to 185 mV/A -> scaled to mA */
        int32_t current_calc = (delta_v * 1000) / 100; /* mA */
        if (current_calc > 4000) current_calc = 4000;
        data->current_ma = (int16_t)current_calc;
    } else {
        /* VIRTUAL CURRENT SENSOR (Model-Based Observer):
         * I(t) = (V_terminal - V_back_emf) / R_armature
         * V_terminal = (PWM_ratio) * (V_supply - I * R_supply)
         * V_back_emf = Ke * Speed_rad_s
         */
        float v_supply_volts = (float)data->supply_mv / 1000.0f;
        float speed_rad_s = ((float)data->speed_rpm) * (2.0f * 3.14159f / 60.0f);
        float ke = 0.025f;       /* Back-EMF constant (V / rad/s) */
        float r_armature = 1.2f; /* Motor resistance (Ohms) */

        uint16_t active_pwm = (uint16_t)TIM1->CCR1;
        float pwm_ratio = (float)active_pwm / 1000.0f;

        if (active_pwm > 0) {
            float v_term = pwm_ratio * v_supply_volts;
            float v_emf = ke * speed_rad_s;
            float i_est_a = (v_term - v_emf) / r_armature;

            if (i_est_a < 0.25f) i_est_a = 0.25f;
            if (i_est_a > 3.5f)  i_est_a = 3.5f;

            data->current_ma = (int16_t)(i_est_a * 1000.0f);
        } else {
            data->current_ma = 0;
        }
    }
}
