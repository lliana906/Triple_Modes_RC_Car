#ifndef INC_CAN_H_
#define INC_CAN_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* 라즈베리파이 -> STM32 : 색 인식 결과 명령 */
typedef enum {
    CAN_CMD_STOP = 0,   // 정지
    CAN_CMD_SLOW = 1,   // 저속
    CAN_CMD_NORMAL = 2  // 정속 (원래 속도)
} CanSpeedCmd_t;

void     CAN_Init(void);

/* 매 루프에서 호출: 새 메시지 있으면 받아서 내부 상태 갱신 */
void     CAN_Poll(void);

/* ✅ 테스트용: CAN 하드웨어 없이 시리얼 키('0'/'1'/'2')로 speed_cmd를 강제 설정.
 * 실제 CAN 메시지가 들어오면 그 값으로 다시 덮어써지므로, 통합 테스트 시
 * 실수신과 충돌하지 않음. */
void     CAN_DebugSetSpeedCmd(uint8_t cmd);

/* 가장 최근 수신된 속도 명령 (0/1/2) */
uint8_t  CAN_GetSpeedCmd(void);

/* 0.0~1.0 배율로 변환된 값 (PWM에 곱하기 좋게) */
float    CAN_GetSpeedScale(void);

/* 마지막 수신 후 경과 시간(ms). 통신 끊김 감지용 */
uint32_t CAN_GetLastRxAgeMs(void);

#endif /* INC_CAN_H_ */
