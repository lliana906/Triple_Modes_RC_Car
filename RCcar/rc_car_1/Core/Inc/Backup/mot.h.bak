#ifndef INC_MOTOR_H_
#define INC_MOTOR_H_

#include "stm32f4xx_hal.h"

// ===== 방향 정의 =====
#define DIR_FORWARD   1
#define DIR_BACKWARD  0
#define DIR_STOP      2

// ===== 포트 정의 =====
#define MOTOR_GPIO_PORT GPIOC

// ===== 핀 정의 =====
#define MOTOR_A_IN1_PIN GPIO_PIN_9
#define MOTOR_A_IN2_PIN GPIO_PIN_8
#define MOTOR_B_IN3_PIN GPIO_PIN_6
#define MOTOR_B_IN4_PIN GPIO_PIN_5

// ===== 함수 선언 =====
void MotorA_Control(int speed, int direction);
void MotorB_Control(int speed, int direction);

#endif
