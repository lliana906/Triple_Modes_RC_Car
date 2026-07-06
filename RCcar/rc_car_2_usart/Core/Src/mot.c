///*
// * mot.c
// *
// *  Created on: Feb 24, 2026
// *      Author: kimyujeong
// */
//
//


#include "mot.h"
extern TIM_HandleTypeDef htim3;

/* 모터 A 제어 함수 (방향 및 속도 제어) */
void MotorA_Control(int speed, int direction)
{
    TIM3->CCR1 = speed;   // ✅ A는 CH1(ENA)만 건드림

    if (direction == 1) { // 전진
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_A_IN1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_A_IN2_PIN, GPIO_PIN_RESET);
    } else {              // 후진
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_A_IN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_A_IN2_PIN, GPIO_PIN_SET);
    }
}

/* 모터 B 제어 함수 (방향 및 속도 제어) */
void MotorB_Control(int speed, int direction)
{
    TIM3->CCR2 = speed;   // ✅ B는 CH2(ENB)만 건드림

    // ✅ 방향이 뒤집힌 증상이면 여기서 IN3/IN4를 반대로(아래처럼)
    if (direction == 1) { // 전진
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_B_IN3_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_B_IN4_PIN, GPIO_PIN_SET);
    } else {              // 후진
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_B_IN3_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_B_IN4_PIN, GPIO_PIN_RESET);
    }
}
