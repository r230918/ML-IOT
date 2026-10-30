#include "motor_controller.h"
#include "l293d.h"

void Motor_Init(void) {
    L293D_Init();
}

void Motor_SetDirection(bool forward) {
    L293D_SetDirection(forward);
}

void Motor_SetPWM(uint16_t duty_permille) {
    L293D_SetSpeed(duty_permille);
}

void Motor_EmergencyBrake(void) {
    L293D_Brake();
}
