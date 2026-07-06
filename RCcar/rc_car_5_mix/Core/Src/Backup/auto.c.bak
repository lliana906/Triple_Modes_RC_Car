/*
 * auto.c (TRACK RETENTION & PRECISION DISTANCE)
 */

#include "auto.h"
#include "mot.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ===================== TUNING (기록 단축 & 이탈 방지) ===================== */
#define PWM_MAX               1000

// 1. 주행 설정
#define BASE_SPD              530
#define MOTOR_B_BIAS          -3    // 🛑 -5에서 -3으로 조정 (왼쪽 붙음 완화)
#define MIN_RUN_SPD           220

// 2. 조향 스무딩
#define KP_CENTER             10
#define STEER_DEADZONE        6
#define STEER_MAX             380
#define STEER_STEP_MAX        35    // 🛑 더 낮춤 (회전 후 급격한 복귀 방지)

// 3. 인식 거리 최적화 (넓은 길 이탈 방지 핵심)
#define BUF_SIZE              8
#define FRONT_CRITICAL_CM     45    // 🛑 48에서 45로 더 낮춤 (바짝 붙기)
#define SIDE_TURN_CM          15    // 🛑 18에서 15로 낮춤 (벽이 확실히 가까울 때만 턴)
#define WALL_PRESENT_CM       90    // 🛑 100에서 90으로 조정 (먼 벽 무시)

// 4. 고속 회전 설정
#define TURN_DELAY_MS         50
#define TURN_SPD_FWD          580
#define TURN_SPD_BWD          480
#define TURN_MIN_MS           140   // 🛑 턴 유지 시간 확보
#define TURN_MAX_MS           330   // 🛑 오버턴 방지 위해 살짝 단축
/* ================================================= ===================== */

#define DIR_FWD 1
#define DIR_BWD 0

typedef enum { ST_RUN=0, ST_PRE_TURN, ST_TURN_L, ST_TURN_R } AutoState;
static AutoState st;
static uint32_t st_enter_ms;
static int steer_last;

static uint16_t l_buf[BUF_SIZE], f_buf[BUF_SIZE], r_buf[BUF_SIZE];
static uint8_t buf_idx = 0;

static inline int clamp_i(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static uint16_t get_avg(uint16_t *buf) {
    uint32_t sum = 0;
    for(int i=0; i<BUF_SIZE; i++) sum += buf[i];
    return (uint16_t)(sum / BUF_SIZE);
}

static void Enter(AutoState ns) {
    st = ns;
    st_enter_ms = HAL_GetTick();
}

void Auto_Init(void) {
    Enter(ST_RUN);
    steer_last = 0;
    for(int i=0; i<BUF_SIZE; i++) {
        l_buf[i] = r_buf[i] = f_buf[i] = 150;
    }
}

void Auto_Update(uint16_t left_raw, uint16_t front_raw, uint16_t right_raw) {
    uint32_t now = HAL_GetTick();

    if (left_raw == 0 || left_raw > 250) left_raw = l_buf[(buf_idx + BUF_SIZE - 1) % BUF_SIZE];
    if (front_raw == 0 || front_raw > 250) front_raw = f_buf[(buf_idx + BUF_SIZE - 1) % BUF_SIZE];
    if (right_raw == 0 || right_raw > 250) right_raw = r_buf[(buf_idx + BUF_SIZE - 1) % BUF_SIZE];

    l_buf[buf_idx] = left_raw;
    f_buf[buf_idx] = front_raw;
    r_buf[buf_idx] = right_raw;
    buf_idx = (buf_idx + 1) % BUF_SIZE;

    uint16_t lc = get_avg(l_buf);
    uint16_t fc = get_avg(f_buf);
    uint16_t rc = get_avg(r_buf);



    if (st == ST_RUN) {
        // 🛑 넓은 길에서 턴하는 문제 방지: 전방이 45cm 미만이고, 측면 벽이 확실히 15cm 이내일 때만 턴 진입
        bool front_wall = (fc < FRONT_CRITICAL_CM);
        bool side_wall = (lc < SIDE_TURN_CM || rc < SIDE_TURN_CM);

        if ((front_wall && side_wall) || fc < 28) { // 28cm는 절대 충돌 방지 거리
            Enter(ST_PRE_TURN);
        } else {
            int steer = 0;
            // 왼쪽 벽 기준 주행 (왼쪽으로 너무 붙지 않게 35cm 유지)
            if (lc < 75) {
                int error = 35 - (int)lc; // 🛑 32에서 35로 상향
                if (error < -STEER_DEADZONE || error > STEER_DEADZONE) steer = KP_CENTER * error;
            }
            else if (rc < 65) {
                steer = -(KP_CENTER + 5) * (50 - (int)rc);
            }

            // 🛑 조향 급변 방지 (회전 후 지그재그 방지)
            steer = clamp_i(steer, -STEER_MAX, STEER_MAX);
            int d = steer - steer_last;
            d = clamp_i(d, -35, 35); // 🛑 변화 폭을 더 좁혀서 스무스하게
            steer = steer_last + d;
            steer_last = steer;

            int current_base = BASE_SPD;
            if (fc < 75) current_base -= 180;
            else if (fc < 100) current_base -= 50;

            MotorA_Control(clamp_i(current_base + steer, MIN_RUN_SPD, PWM_MAX), DIR_FWD);
            MotorB_Control(clamp_i(current_base - steer + MOTOR_B_BIAS, MIN_RUN_SPD, PWM_MAX), DIR_FWD);
        }
    }
    else if (st == ST_PRE_TURN) {
        MotorA_Control(0, DIR_FWD);
        MotorB_Control(0, DIR_FWD);
        if (now - st_enter_ms > TURN_DELAY_MS) {
            if (rc < lc) Enter(ST_TURN_L);
            else Enter(ST_TURN_R);
        }
    }
    else if (st == ST_TURN_L || st == ST_TURN_R) {
        uint32_t dt = now - st_enter_ms;
        if (st == ST_TURN_L) {
            MotorA_Control(TURN_SPD_BWD, DIR_BWD);
            MotorB_Control(TURN_SPD_FWD, DIR_FWD);
        } else {
            MotorA_Control(TURN_SPD_FWD, DIR_FWD);
            MotorB_Control(TURN_SPD_BWD, DIR_BWD);
        }

        // 🛑 회전 탈출 시 전방 거리를 90cm로 더 넉넉하게 확인
        if ((dt > TURN_MIN_MS && fc > 90) || (dt > TURN_MAX_MS)) {
            Enter(ST_RUN);
            steer_last = 0;
        }
    }
}
