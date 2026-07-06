#include "bt.h"
#include "mot.h"
#include "tim.h"   // ✅ htim3 접근
#include <stdint.h>

static volatile uint8_t  g_rx_pending = 0;
static volatile uint8_t  g_rx_byte = 0;

static volatile DriveMode_t g_mode = MODE_MANUAL;

static inline int PWM_MAX(void)
{
    return (int)__HAL_TIM_GET_AUTORELOAD(&htim3);   // ✅ ARR 그대로 사용
}

static void SafeStop(void)
{
    Mot_Stop();
}

void BT_Init(void)
{
    g_rx_pending = 0;
    g_rx_byte = 0;
    g_mode = MODE_MANUAL;
    SafeStop();
}

void BT_OnRxByte(uint8_t b)
{
    g_rx_byte = b;
    g_rx_pending = 1;
}

DriveMode_t BT_GetMode(void)
{
    return g_mode;
}

void BT_Process(void)
{
    if (!g_rx_pending) return;
    g_rx_pending = 0;

    uint8_t c = g_rx_byte;

    /* 모드 전환은 언제든 가능 */
    if (c == 'v') { g_mode = MODE_MANUAL; SafeStop(); return; }
    if (c == 'b') { g_mode = MODE_AUTO;   SafeStop(); return; }
    if (c == 'z') { g_mode = MODE_CAMERA; SafeStop(); return; }  // ✅ 추가: 카메라 모드

    if (g_mode != MODE_MANUAL) return;

    int sp = PWM_MAX(); // ✅ 현재 PWM 최대치로 수동 주행

    switch (c)
    {
        case 'w':
            MotorA_Control(sp, DIR_FORWARD);
            MotorB_Control(sp, DIR_FORWARD);
            break;
        case 's':
            MotorA_Control(sp, DIR_BACKWARD);
            MotorB_Control(sp, DIR_BACKWARD);
            break;
        case 'a':
            MotorA_Control(sp, DIR_BACKWARD);
            MotorB_Control(sp, DIR_FORWARD);
            break;
        case 'd':
            MotorA_Control(sp, DIR_FORWARD);
            MotorB_Control(sp, DIR_BACKWARD);
            break;
        case 'c':
            SafeStop();
            break;
        default:
            break;
    }
}
