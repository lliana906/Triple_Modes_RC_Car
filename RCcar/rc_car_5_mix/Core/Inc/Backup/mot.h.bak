#ifndef INC_MOT_H_
#define INC_MOT_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* ===== 방향 정의 (기존 유지) ===== */
#define DIR_FORWARD   1
#define DIR_BACKWARD  0
#define DIR_STOP      2

/* ===== 포트 정의 (기존 유지) ===== */
#define MOTOR_GPIO_PORT GPIOC

/* ===== 핀 정의 (기존 유지) ===== */
#define MOTOR_A_IN1_PIN GPIO_PIN_9
#define MOTOR_A_IN2_PIN GPIO_PIN_8
#define MOTOR_B_IN3_PIN GPIO_PIN_6
#define MOTOR_B_IN4_PIN GPIO_PIN_5

/* ===== 기본 제어 함수 (기존 유지) ===== */
void MotorA_Control(int speed, int direction);
void MotorB_Control(int speed, int direction);

/* ===== 자율주행(FSM)용 래퍼 (추가) ===== */
void Mot_Stop(void);
void Mot_Forward(int speed);
void Mot_Backward(int speed);
void Mot_TurnLeft(int speed);
void Mot_TurnRight(int speed);

#endif /* INC_MOT_H_ */
