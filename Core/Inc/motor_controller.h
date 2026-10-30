#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

void Motor_Init(void);
void Motor_SetDirection(bool forward);
void Motor_SetPWM(uint16_t duty_permille);
void Motor_EmergencyBrake(void);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_CONTROLLER_H */
