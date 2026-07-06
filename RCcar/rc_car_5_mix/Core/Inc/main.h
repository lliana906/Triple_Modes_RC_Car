/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TIM2_CH1_Recho_Pin GPIO_PIN_0
#define TIM2_CH1_Recho_GPIO_Port GPIOA
#define TIM2_CH2_echo_Pin GPIO_PIN_1
#define TIM2_CH2_echo_GPIO_Port GPIOA
#define Rtrig_Pin GPIO_PIN_4
#define Rtrig_GPIO_Port GPIOA
#define TIM3_CH1_PWM_Pin GPIO_PIN_6
#define TIM3_CH1_PWM_GPIO_Port GPIOA
#define TIM3_CH2_PWM_Pin GPIO_PIN_7
#define TIM3_CH2_PWM_GPIO_Port GPIOA
#define mt_D_B2_IN2_Pin GPIO_PIN_5
#define mt_D_B2_IN2_GPIO_Port GPIOC
#define trig_Pin GPIO_PIN_0
#define trig_GPIO_Port GPIOB
#define TIM2_CH3_Lecho_Pin GPIO_PIN_10
#define TIM2_CH3_Lecho_GPIO_Port GPIOB
#define mt_D_B1_IN1_Pin GPIO_PIN_6
#define mt_D_B1_IN1_GPIO_Port GPIOC
#define mt_D_A2_IN2_Pin GPIO_PIN_8
#define mt_D_A2_IN2_GPIO_Port GPIOC
#define mt_D_A1_IN1_Pin GPIO_PIN_9
#define mt_D_A1_IN1_GPIO_Port GPIOC
#define Ltrig_Pin GPIO_PIN_8
#define Ltrig_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
