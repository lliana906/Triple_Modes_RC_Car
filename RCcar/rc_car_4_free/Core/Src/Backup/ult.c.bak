#include "ult.h"
#include "delay.h"

extern TIM_HandleTypeDef htim2;

/* ===== TRIG 핀 ===== */
#define TRIG1_PORT GPIOA
#define TRIG1_PIN  GPIO_PIN_4

#define TRIG2_PORT GPIOB
#define TRIG2_PIN  GPIO_PIN_0

#define TRIG3_PORT GPIOA
#define TRIG3_PIN  GPIO_PIN_8

/* ===== 기존 변수 그대로 ===== */
static uint16_t IC_Value1[3];
static uint16_t IC_Value2[3];
static uint16_t echoTime[3];
static uint8_t  captureFlag[3];
static uint16_t distance[3];
static uint8_t  dist_ready[3];

void ULT_Init(void)
{
    for(int i=0;i<3;i++)
    {
        IC_Value1[i]=0;
        IC_Value2[i]=0;
        echoTime[i]=0;
        captureFlag[i]=0;
        distance[i]=0;
        dist_ready[i]=0;
    }

    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_3);
}

void ULT_Trigger(uint8_t idx)
{
    GPIO_TypeDef *port;
    uint16_t pin;
    uint32_t ch;
    uint32_t it;

    if(idx==0){ port=TRIG1_PORT; pin=TRIG1_PIN; ch=TIM_CHANNEL_1; it=TIM_IT_CC1; }
    else if(idx==1){ port=TRIG2_PORT; pin=TRIG2_PIN; ch=TIM_CHANNEL_2; it=TIM_IT_CC2; }
    else if(idx==2){ port=TRIG3_PORT; pin=TRIG3_PIN; ch=TIM_CHANNEL_3; it=TIM_IT_CC3; }
    else return;

    HAL_GPIO_WritePin(port,pin,GPIO_PIN_RESET);
    delay_us(2);
    HAL_GPIO_WritePin(port,pin,GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(port,pin,GPIO_PIN_RESET);

    captureFlag[idx]=0;
    dist_ready[idx]=0;

    __HAL_TIM_SET_CAPTUREPOLARITY(&htim2,ch,TIM_INPUTCHANNELPOLARITY_RISING);
    __HAL_TIM_SET_COUNTER(&htim2,0);
    __HAL_TIM_ENABLE_IT(&htim2,it);
}

void ULT_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    uint8_t idx;
    uint32_t ch;
    uint32_t it;

    if(htim->Instance!=TIM2) return;

    if(htim->Channel==HAL_TIM_ACTIVE_CHANNEL_1)
    { idx=0; ch=TIM_CHANNEL_1; it=TIM_IT_CC1; }
    else if(htim->Channel==HAL_TIM_ACTIVE_CHANNEL_2)
    { idx=1; ch=TIM_CHANNEL_2; it=TIM_IT_CC2; }
    else if(htim->Channel==HAL_TIM_ACTIVE_CHANNEL_3)
    { idx=2; ch=TIM_CHANNEL_3; it=TIM_IT_CC3; }
    else return;

    if(captureFlag[idx]==0)
    {
        IC_Value1[idx]=HAL_TIM_ReadCapturedValue(htim,ch);
        captureFlag[idx]=1;
        __HAL_TIM_SET_CAPTUREPOLARITY(htim,ch,TIM_INPUTCHANNELPOLARITY_FALLING);
    }
    else
    {
        IC_Value2[idx]=HAL_TIM_ReadCapturedValue(htim,ch);

        if(IC_Value2[idx]>=IC_Value1[idx])
            echoTime[idx]=IC_Value2[idx]-IC_Value1[idx];
        else
            echoTime[idx]=(0xFFFF-IC_Value1[idx])+IC_Value2[idx];

        distance[idx]=echoTime[idx]/58;
        dist_ready[idx]=1;

        captureFlag[idx]=0;
        __HAL_TIM_SET_CAPTUREPOLARITY(htim,ch,TIM_INPUTCHANNELPOLARITY_RISING);
        __HAL_TIM_DISABLE_IT(&htim2,it);
    }
}

uint16_t ULT_GetDistance(uint8_t idx)
{
    return distance[idx];
}

uint8_t ULT_IsReady(uint8_t idx)
{
    return dist_ready[idx];
}

void ULT_ClearReady(uint8_t idx)
{
    dist_ready[idx]=0;
}
