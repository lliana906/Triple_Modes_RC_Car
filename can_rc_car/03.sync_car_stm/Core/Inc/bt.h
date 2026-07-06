#ifndef INC_BT_H_
#define INC_BT_H_

#include "main.h"
#include <stdint.h>

typedef enum
{
    MODE_MANUAL  = 0,
    MODE_AUTO    = 1,
    MODE_CAMERA  = 2   // ✅ 추가: 초음파회피 + CAN 색인식 속도조절 동시 작동
} DriveMode_t;

void BT_Init(void);

/* main의 HAL_UART_RxCpltCallback()에서 호출 */
void BT_OnRxByte(uint8_t b);

void BT_Process(void);
DriveMode_t BT_GetMode(void);

#endif
