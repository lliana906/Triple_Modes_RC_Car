/*
 * mot.c
 *
 *  Created on: Feb 24, 2026
 *      Author: kimyujeong
 */

#include "mot.h"

extern TIM_HandleTypeDef htim3;

/* speed를 0 ~ ARR 범위로 제한 */
static inline uint16_t clamp_pwm(int speed)
{
    int max = (int)__HAL_TIM_GET_AUTORELOAD(&htim3);
    if (speed < 0) return 0;
    if (speed > max) return (uint16_t)max;
    return (uint16_t)speed;
}

/* 모터 A 제어: PWM=TIM3 CH1, DIR=PC9/PC8 */
void MotorA_Control(int speed, int direction)
{
    uint16_t pwm = clamp_pwm(speed);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwm);

    if (direction == DIR_FORWARD) {
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_A_IN1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_A_IN2_PIN, GPIO_PIN_RESET);
    }
    else if (direction == DIR_BACKWARD) {
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_A_IN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_A_IN2_PIN, GPIO_PIN_SET);
    }
    else { // DIR_STOP
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_A_IN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_A_IN2_PIN, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
    }
}

/* 모터 B 제어: PWM=TIM3 CH2, DIR=PC6/PC5 */
void MotorB_Control(int speed, int direction)
{
    uint16_t pwm = clamp_pwm(speed);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwm);

    if (direction == DIR_FORWARD) {
        /* 너가 기존에 B 모터 방향이 뒤집혀서 이렇게 쓰던 패턴 유지 */
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_B_IN3_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_B_IN4_PIN, GPIO_PIN_SET);
    }
    else if (direction == DIR_BACKWARD) {
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_B_IN3_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_B_IN4_PIN, GPIO_PIN_RESET);
    }
    else { // DIR_STOP
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_B_IN3_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_B_IN4_PIN, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
    }
}

/* ===== 자율주행(FSM)용 래퍼 ===== */

void Mot_Stop(void)
{
    MotorA_Control(0, DIR_STOP);
    MotorB_Control(0, DIR_STOP);
}

void Mot_Forward(int speed)
{
    MotorA_Control(speed, DIR_FORWARD);
    MotorB_Control(speed, DIR_FORWARD);
}

void Mot_Backward(int speed)
{
    MotorA_Control(speed, DIR_BACKWARD);
    MotorB_Control(speed, DIR_BACKWARD);
}

/* 제자리 좌회전: A 후진, B 전진 */
void Mot_TurnLeft(int speed)
{
    MotorA_Control(speed, DIR_BACKWARD);
    MotorB_Control(speed, DIR_FORWARD);
}

/* 제자리 우회전: A 전진, B 후진 */
void Mot_TurnRight(int speed)
{
    MotorA_Control(speed, DIR_FORWARD);
    MotorB_Control(speed, DIR_BACKWARD);
}
