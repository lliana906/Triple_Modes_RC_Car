/*
 * auto.c
 *
 *  Created on: Feb 27, 2026
 *      Author: kimyujeong
 */

#include "auto.h"
#include "mot.h"

/* ====== 튜닝 파라미터(트랙마다 여기만 만지면 됨) ====== */
#define SPD_FWD     450
#define SPD_TURN    430
#define SPD_BACK    380

#define STOP_CM     10   // 너무 가까우면 정지
#define TURN_CM     25   // 회피 시작(앞이 가까움)
#define CLEAR_CM    35   // 안전하면 전진 복귀(히스테리시스)

#define SIDE_CM     18   // ✅ (추가) 옆 벽이 이 값 이하면 옆 회피
#define BIAS_MS     90   // ✅ (추가) 살짝 꺾는 시간(짧게)

#define STOP_MS     120
#define BACK_MS     260
#define TURN_MS     260

/* ====== 방향 정의(너 mot.h 기준) ====== */
#define DIR_FWD  1
#define DIR_BWD  0

/* ====== 좌/우 모터 매핑 ======
   - 좌우가 반대로 움직이면 여기만 바꿔.
*/
static inline void LMotor(int spd, int dir) { MotorA_Control(spd, dir); } // LEFT
static inline void RMotor(int spd, int dir) { MotorB_Control(spd, dir); } // RIGHT

static inline void Drive_Stop(void)
{
  Mot_Stop(); // mot.c에 추가한 정지 함수 사용
}
static inline void Drive_Forward(void)
{
  LMotor(SPD_FWD, DIR_FWD);
  RMotor(SPD_FWD, DIR_FWD);
}
static inline void Drive_Back(void)
{
  LMotor(SPD_BACK, DIR_BWD);
  RMotor(SPD_BACK, DIR_BWD);
}
static inline void Drive_TurnLeft(void)
{
  // 제자리 좌회전(왼쪽 후진, 오른쪽 전진)
  LMotor(SPD_TURN, DIR_BWD);
  RMotor(SPD_TURN, DIR_FWD);
}
static inline void Drive_TurnRight(void)
{
  // 제자리 우회전(왼쪽 전진, 오른쪽 후진)
  LMotor(SPD_TURN, DIR_FWD);
  RMotor(SPD_TURN, DIR_BWD);
}

/* ====== FSM ====== */
typedef enum {
  ST_FWD = 0,
  ST_STOP,
  ST_BACK,
  ST_TURN_L,
  ST_TURN_R,
  ST_BIAS_L,   // ✅ (추가) 살짝 좌로
  ST_BIAS_R    // ✅ (추가) 살짝 우로
} AutoState;

static AutoState st;
static uint32_t  st_enter_ms;

static void Enter(AutoState ns)
{
  st = ns;
  st_enter_ms = HAL_GetTick();
}

void Auto_Init(void)
{
  Enter(ST_FWD);
}

void Auto_Update(uint16_t left_cm, uint16_t front_cm, uint16_t right_cm)
{
  uint32_t now = HAL_GetTick();

  /* ---------- 전이(상태 결정) ---------- */

  /* 1) 최우선: 너무 가까우면 정지 */
  if (front_cm > 0 && front_cm <= STOP_CM)
  {
    if (st != ST_STOP) Enter(ST_STOP);
  }
  /* 2) 앞이 막히면: 후진 -> (좌/우 넓은쪽으로) 회전 */
  else if (front_cm > 0 && front_cm <= TURN_CM)
  {
    // 전진 중에만 후진 진입
    if (st == ST_FWD || st == ST_BIAS_L || st == ST_BIAS_R) Enter(ST_BACK);
  }
  /* 3) 앞이 안전한 구간: 옆 벽 회피(BIAS) */
  else if (front_cm >= CLEAR_CM || front_cm == 0)
  {
    // ✅ 옆이 너무 가까우면 반대 방향으로 짧게 꺾기
    // (0은 “측정 실패” 가능성이 있으니 제외하고 판단)
    if (left_cm > 0 && left_cm <= SIDE_CM)
    {
      if (st == ST_FWD) Enter(ST_BIAS_R); // 왼쪽 가까움 -> 오른쪽으로
    }
    else if (right_cm > 0 && right_cm <= SIDE_CM)
    {
      if (st == ST_FWD) Enter(ST_BIAS_L); // 오른쪽 가까움 -> 왼쪽으로
    }
    else
    {
      // STOP 상태면 다시 전진으로 복귀
      if (st == ST_STOP) Enter(ST_FWD);
      // 나머지는 현재 상태 유지(각 state의 타이머가 끝나면 FWD로)
    }
  }

  /* ---------- 상태별 행동 ---------- */
  switch (st)
  {
    case ST_FWD:
      Drive_Forward();
      break;

    case ST_STOP:
      Drive_Stop();
      if (now - st_enter_ms >= STOP_MS) Enter(ST_FWD);
      break;

    case ST_BACK:
      Drive_Back();
      if (now - st_enter_ms >= BACK_MS)
      {
        // 후진 후 더 넓은 쪽으로 회전
        if (left_cm >= right_cm) Enter(ST_TURN_L);
        else                    Enter(ST_TURN_R);
      }
      break;

    case ST_TURN_L:
      Drive_TurnLeft();
      if (now - st_enter_ms >= TURN_MS) Enter(ST_FWD);
      break;

    case ST_TURN_R:
      Drive_TurnRight();
      if (now - st_enter_ms >= TURN_MS) Enter(ST_FWD);
      break;

    case ST_BIAS_L:
      Drive_TurnLeft();
      if (now - st_enter_ms >= BIAS_MS) Enter(ST_FWD);
      break;

    case ST_BIAS_R:
      Drive_TurnRight();
      if (now - st_enter_ms >= BIAS_MS) Enter(ST_FWD);
      break;

    default:
      Enter(ST_FWD);
      break;
  }
}
