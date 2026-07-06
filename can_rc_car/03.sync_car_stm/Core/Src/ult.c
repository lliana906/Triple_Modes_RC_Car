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

/* ===== 타임아웃(에코 미수신) 처리 ===== */
#define ULT_TIMEOUT_MS  30   // 4m 왕복~23ms, 여유로 30ms 권장

/* ===== 기존 변수(크기만 32bit로 안정화) ===== */
static uint32_t IC_Value1[3];
static uint32_t IC_Value2[3];
static uint32_t echoTime[3];
static uint8_t  captureFlag[3];
static uint16_t distance[3];
static uint8_t  dist_ready[3];

/* 측정 진행 상태 */
static uint8_t  waiting[3];
static uint32_t trig_ms[3];

/* 내부: idx -> channel/it */
static void ULT_Map(uint8_t idx, GPIO_TypeDef **port, uint16_t *pin, uint32_t *ch, uint32_t *it, uint32_t *flag)
{
    if(idx==0){ *port=TRIG1_PORT; *pin=TRIG1_PIN; *ch=TIM_CHANNEL_1; *it=TIM_IT_CC1; *flag=TIM_FLAG_CC1; }
    else if(idx==1){ *port=TRIG2_PORT; *pin=TRIG2_PIN; *ch=TIM_CHANNEL_2; *it=TIM_IT_CC2; *flag=TIM_FLAG_CC2; }
    else { *port=TRIG3_PORT; *pin=TRIG3_PIN; *ch=TIM_CHANNEL_3; *it=TIM_IT_CC3; *flag=TIM_FLAG_CC3; }
}

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
        waiting[i]=0;
        trig_ms[i]=0;
    }

    /* ✅ 채널은 시작하되, 인터럽트는 평소엔 꺼둔다(잡음 캡처 꼬임 방지) */
    HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_3);

    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC2);
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC3);
}

void ULT_Trigger(uint8_t idx)
{
    GPIO_TypeDef *port;
    uint16_t pin;
    uint32_t ch;
    uint32_t it;
    uint32_t flag;

    if(idx > 2) return;

    /* ✅ 측정 중이면 재트리거 금지(꼬임 방지) */
    if(waiting[idx]) return;

    ULT_Map(idx, &port, &pin, &ch, &it, &flag);

    /* 트리거 펄스 */
    HAL_GPIO_WritePin(port,pin,GPIO_PIN_RESET);
    delay_us(2);
    HAL_GPIO_WritePin(port,pin,GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(port,pin,GPIO_PIN_RESET);

    captureFlag[idx]=0;
    dist_ready[idx]=0;
    waiting[idx]=1;
    trig_ms[idx]=HAL_GetTick();

    __HAL_TIM_SET_CAPTUREPOLARITY(&htim2,ch,TIM_INPUTCHANNELPOLARITY_RISING);
    __HAL_TIM_SET_COUNTER(&htim2,0);

    /* ✅ 이전 CC 플래그 남아있으면 "가짜 인터럽트"가 바로 터질 수 있음 */
    __HAL_TIM_CLEAR_FLAG(&htim2, flag);

    __HAL_TIM_ENABLE_IT(&htim2,it);
}

/* ✅ 에코 미수신(측정 실패) 타임아웃 처리: 실패도 dist_ready=1로 내보냄 */
void ULT_Update(void)
{
    uint32_t now = HAL_GetTick();

    for (uint8_t idx=0; idx<3; idx++)
    {
        if(waiting[idx] && (now - trig_ms[idx] > ULT_TIMEOUT_MS))
        {
            GPIO_TypeDef *port;
            uint16_t pin;
            uint32_t ch;
            uint32_t it;
            uint32_t flag;
            ULT_Map(idx, &port, &pin, &ch, &it, &flag);

            distance[idx]   = 0;   // 실패값
            dist_ready[idx] = 1;
            waiting[idx]    = 0;
            captureFlag[idx]= 0;

            __HAL_TIM_SET_CAPTUREPOLARITY(&htim2,ch,TIM_INPUTCHANNELPOLARITY_RISING);
            __HAL_TIM_DISABLE_IT(&htim2,it);
            __HAL_TIM_CLEAR_FLAG(&htim2, flag);
        }
    }
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

        uint32_t max = __HAL_TIM_GET_AUTORELOAD(htim);
        if(IC_Value2[idx]>=IC_Value1[idx])
            echoTime[idx]=IC_Value2[idx]-IC_Value1[idx];
        else
            echoTime[idx]=(max - IC_Value1[idx]) + IC_Value2[idx] + 1;

        /* echoTime이 us 단위(1MHz tick)일 때: cm = us / 58 */
        distance[idx]=(uint16_t)(echoTime[idx]/58);

        dist_ready[idx]=1;
        waiting[idx]=0;

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
