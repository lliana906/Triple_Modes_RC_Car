/*
 * delay.c
 *
 *  Created on: Feb 4, 2026
 *      Author: kimyujeong
 */

#include "delay.h"

//블로킹을 위해서
void delay_us(uint16_t us)
{
  __HAL_TIM_SET_COUNTER(&htim11, 0);
  while((__HAL_TIM_GET_COUNTER(&htim11)) < us);
}
