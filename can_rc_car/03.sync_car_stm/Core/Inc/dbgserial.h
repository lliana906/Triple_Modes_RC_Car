#ifndef INC_DBGSERIAL_H_
#define INC_DBGSERIAL_H_

#include "main.h"
#include <stdint.h>

/* ===== USART2 = 속도명령 테스트 입력 전용 =====
 * 시리얼 터미널에서 '0'/'1'/'2' 입력 -> CAN_DebugSetSpeedCmd()로 전달.
 * CAN/라즈베리파이 하드웨어 없이 auto.c의 speed_scale 동작만 먼저 검증할 때 사용.
 * 실제 CAN 메시지가 들어오면 그 값이 다시 덮어쓰므로 통합 단계에서도 안전하게 공존.
 */

void DBG_Init(void);

/* main의 HAL_UART_RxCpltCallback()에서 USART2 수신 시 호출 */
void DBG_OnRxByte(uint8_t b);

/* 메인 루프에서 매번 호출: 수신 바이트 있으면 처리 */
void DBG_Process(void);

/* 5초마다 main에서 호출: 현재 속도명령/배율 + 초음파 3개 거리를 USART2로 출력 */
void DBG_PrintStatus(uint16_t left_cm, uint16_t front_cm, uint16_t right_cm);

#endif /* INC_DBGSERIAL_H_ */
