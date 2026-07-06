#ifndef INC_AUTO_H_
#define INC_AUTO_H_

#include <stdint.h>

void Auto_Init(void);

/* speed_scale: 0.0~1.0. CAN(라즈베리파이 색인식)에서 받은 속도 배율.
 * ST_RUN 상태의 최종 모터 PWM에만 곱해짐 (회전 동작은 회피 우선이라 영향 안 받음,
 * 단 speed_scale==0이면 회전 중에도 즉시 정지). */
void Auto_Update(uint16_t left_cm, uint16_t front_cm, uint16_t right_cm, float speed_scale);

#endif
