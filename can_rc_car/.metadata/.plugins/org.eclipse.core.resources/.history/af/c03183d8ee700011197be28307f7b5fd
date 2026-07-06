/*
 * can.c
 * MCP2515 수신 전용 드라이버. 기존 uploaded main.c의 MCP2515 함수들을
 * 그대로 옮기고, RC카 FSM(auto.c)에서 쓰기 쉬운 형태로 감쌈.
 *
 * 핀맵 (MX 설정 기준, 두 번째 캡처로 확정):
 *   PB13 = can_CS   (SPI1 CS, Output, GPIOB)
 *   PB14 = can_INT  (External Interrupt, GPIOB) - 현재는 폴링만 사용, INT는 안 씀
 *   SPI1 SCK=PB3, MISO=PB4, MOSI=PB5 (MX_SPI1_Init()에서 이미 처리되므로 본 파일에서 손댈 것 없음)
 */

#include "can.h"
#include "spi.h"
#include "main.h"   // ✅ can_CS_Pin, can_CS_GPIO_Port (MX gpio.c에서 생성된 매크로 사용)
#include <string.h>

extern SPI_HandleTypeDef hspi1;

/* ===== CS 핀: MX가 gpio.c/main.h에 생성한 매크로를 그대로 사용 (PB13) ===== */
#define CS_LOW()   HAL_GPIO_WritePin(can_CS_GPIO_Port, can_CS_Pin, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(can_CS_GPIO_Port, can_CS_Pin, GPIO_PIN_SET)

/* ===== MCP2515 레지스터 ===== */
#define MCP_CANCTRL   0x0F
#define MCP_CANSTAT   0x0E
#define MCP_CANINTF   0x2C
#define MCP_EFLG      0x2D
#define MCP_RXB0CTRL  0x60
#define MCP_RXB0SIDH  0x61
#define MCP_CNF3      0x28  // CNF3, CNF2, CNF1 연속 주소

#define MCP_CMD_RESET 0xC0
#define MCP_CMD_READ  0x03
#define MCP_CMD_WRITE 0x02
#define MCP_CMD_BITMOD 0x05

static volatile uint8_t  s_speed_cmd = CAN_CMD_NORMAL; // 기본값: 정속(통신 전/끊김 시 안전하게 동작)
static volatile uint32_t s_last_rx_ms = 0;

static uint8_t MCP_ReadReg(uint8_t addr)
{
    uint8_t result = 0;
    uint8_t cmd = MCP_CMD_READ;

    CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_SPI_Transmit(&hspi1, &addr, 1, 100);
    HAL_SPI_Receive(&hspi1, &result, 1, 100);
    CS_HIGH();

    return result;
}

static void MCP_WriteReg(uint8_t addr, uint8_t value)
{
    uint8_t cmd = MCP_CMD_WRITE;

    CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_SPI_Transmit(&hspi1, &addr, 1, 100);
    HAL_SPI_Transmit(&hspi1, &value, 1, 100);
    CS_HIGH();
}

static void MCP_Reset(void)
{
    uint8_t cmd = MCP_CMD_RESET;
    CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    CS_HIGH();
    HAL_Delay(10);
}

static void MCP_SetNormalMode(void)
{
    MCP_WriteReg(MCP_CANCTRL, 0x00);
}

static void MCP_SetReceiveAny(void)
{
    MCP_WriteReg(MCP_RXB0CTRL, 0x60);
}

/* CANINTF RX0IF 비트가 서 있으면 RXB0 읽고 클리어. 데이터 있으면 1 반환 */
static uint8_t MCP_CheckAndReadRX0(uint8_t *rx_data, uint8_t *rx_dlc)
{
    uint8_t canintf = MCP_ReadReg(MCP_CANINTF);

    if (canintf & 0x01) {
        uint8_t cmd = MCP_CMD_READ;
        uint8_t start_addr = MCP_RXB0SIDH;
        uint8_t header[5]; // SIDH, SIDL, EID8, EID0, DLC

        CS_LOW();
        HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
        HAL_SPI_Transmit(&hspi1, &start_addr, 1, 100);
        HAL_SPI_Receive(&hspi1, header, 5, 100);

        uint8_t dlc = header[4] & 0x0F;
        if (dlc > 8) dlc = 8;

        HAL_SPI_Receive(&hspi1, rx_data, dlc, 100);
        CS_HIGH();

        *rx_dlc = dlc;

        /* RX0IF 클리어 (Bit Modify) */
        uint8_t bitmod_cmd[4] = { MCP_CMD_BITMOD, MCP_CANINTF, 0x01, 0x00 };
        CS_LOW();
        HAL_SPI_Transmit(&hspi1, bitmod_cmd, 4, 100);
        CS_HIGH();

        return 1;
    }
    return 0;
}

void CAN_Init(void)
{
    s_speed_cmd = CAN_CMD_NORMAL;
    s_last_rx_ms = HAL_GetTick();

    CS_HIGH();

    MCP_Reset();

    uint8_t canctrl = MCP_ReadReg(MCP_CANCTRL);
    if ((canctrl & 0xE0) != 0x80) {
        /* Config Mode 진입 실패 - 배선/전원 확인 필요. 일단 진행은 계속함 */
    }

    /* 8MHz 크리스탈, 500kbps: CNF3,CNF2,CNF1 연속 쓰기 */
    uint8_t wcmd = MCP_CMD_WRITE;
    uint8_t addr = MCP_CNF3;
    uint8_t cnf_values[3] = { 0x02, 0x90, 0x00 };

    CS_LOW();
    HAL_SPI_Transmit(&hspi1, &wcmd, 1, 100);
    HAL_SPI_Transmit(&hspi1, &addr, 1, 100);
    HAL_SPI_Transmit(&hspi1, cnf_values, 3, 100);
    CS_HIGH();

    MCP_SetReceiveAny();
    MCP_SetNormalMode();
    HAL_Delay(10);
}

void CAN_Poll(void)
{
    uint8_t rx_data[8] = { 0 };
    uint8_t rx_dlc = 0;

    if (MCP_CheckAndReadRX0(rx_data, &rx_dlc)) {
        uint8_t cmd = rx_data[0];
        if (cmd == CAN_CMD_STOP || cmd == CAN_CMD_SLOW || cmd == CAN_CMD_NORMAL) {
            s_speed_cmd = cmd;
        }
        s_last_rx_ms = HAL_GetTick();
    }
}

/* ✅ 테스트용: 시리얼 '0'/'1'/'2' 키 입력으로 speed_cmd를 강제 설정.
 * CAN 하드웨어/라즈베리파이 없이 auto.c의 speed_scale 동작만 먼저 확인할 때 사용. */
void CAN_DebugSetSpeedCmd(uint8_t cmd)
{
    if (cmd == CAN_CMD_STOP || cmd == CAN_CMD_SLOW || cmd == CAN_CMD_NORMAL) {
        s_speed_cmd = cmd;
        s_last_rx_ms = HAL_GetTick();  // age도 갱신해서 "수신 끊김"으로 안 보이게 함
    }
}

uint8_t CAN_GetSpeedCmd(void)
{
    return s_speed_cmd;
}

float CAN_GetSpeedScale(void)
{
    switch (s_speed_cmd) {
        case CAN_CMD_STOP:   return 0.0f;
        case CAN_CMD_SLOW:   return 0.5f;   // 저속: 50% — 필요시 조정
        case CAN_CMD_NORMAL: return 1.0f;
        default:             return 1.0f;
    }
}

uint32_t CAN_GetLastRxAgeMs(void)
{
    return HAL_GetTick() - s_last_rx_ms;
}
