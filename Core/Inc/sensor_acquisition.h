#ifndef SENSOR_ACQUISITION_H
#define SENSOR_ACQUISITION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define ENCODER_PULSES_PER_REV   48   /* Typical 12 CPR quad decoded x4 = 48 */

typedef struct {
    int16_t  current_ma;
    int16_t  speed_rpm;
    uint32_t encoder_total_pulses;
    uint16_t supply_mv;
    int16_t  temp_c;
} SensorData_t;

void Sensors_Init(void);
void Sensors_Sample(SensorData_t *data, uint32_t delta_ms);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_ACQUISITION_H */
