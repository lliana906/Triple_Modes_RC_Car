#ifndef INC_ULT_H_
#define INC_ULT_H_

#include "main.h"

#define ULT_SENSOR_COUNT 3

void ULT_Init(void);
void ULT_Trigger(uint8_t idx);
void ULT_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);

uint16_t ULT_GetDistance(uint8_t idx);
uint8_t  ULT_IsReady(uint8_t idx);
void     ULT_ClearReady(uint8_t idx);

#endif
