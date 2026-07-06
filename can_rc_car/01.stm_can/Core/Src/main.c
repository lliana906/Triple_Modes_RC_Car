/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include <string.h>
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN 0 */

/* ------ MCP2515 CAN 통신 전역 변수 및 함수 ------ */

volatile uint8_t mcp2515_status = 0;
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart2;

// 라즈베리파이에서 받은 속도 명령 저장 (0=정지, 1=저속, 2=정속)
volatile uint8_t received_speed_cmd = 0;

// 1. MCP2515 레지스터에서 1바이트 읽기
uint8_t MCP2515_ReadRegister(uint8_t address) {
    uint8_t read_instruction = 0x03;
    uint8_t result = 0;

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET); // CS LOW
    HAL_SPI_Transmit(&hspi1, &read_instruction, 1, 100);
    HAL_SPI_Transmit(&hspi1, &address, 1, 100);
    HAL_SPI_Receive(&hspi1, &result, 1, 100);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);   // CS HIGH

    return result;
}

// 2. MCP2515 레지스터에 1바이트 쓰기
void MCP2515_WriteRegister(uint8_t address, uint8_t value) {
    uint8_t write_instruction = 0x02;

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET); // CS LOW
    HAL_SPI_Transmit(&hspi1, &write_instruction, 1, 100);
    HAL_SPI_Transmit(&hspi1, &address, 1, 100);
    HAL_SPI_Transmit(&hspi1, &value, 1, 100);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);   // CS HIGH
}

// 3. MCP2515 SPI Reset 명령 (0xC0) - Config Mode로 초기화
void MCP2515_Reset(void) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET); // CS LOW
    uint8_t reset_cmd = 0xC0;
    HAL_SPI_Transmit(&hspi1, &reset_cmd, 1, 100);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);   // CS HIGH
    HAL_Delay(10); // 리셋 후 안정화 대기
}

// 4. Normal Mode로 전환
void MCP2515_SetNormalMode(void) {
    MCP2515_WriteRegister(0x0F, 0x00); // CANCTRL = Normal Mode
}

// 5. RXB0가 모든 표준/확장 ID를 받아들이도록 설정 (필터 없이 전부 수신)
void MCP2515_SetReceiveAny(void) {
    // RXB0CTRL(0x60) bit5,6 = 11 → 필터 무시, 모든 메시지 수신
    MCP2515_WriteRegister(0x60, 0x60);
}

// 6. 라즈베리파이로부터 CAN 메시지(RXB0) 수신 확인 및 읽기
uint8_t MCP2515_CheckAndReadRX0(uint8_t *rx_data, uint8_t *rx_dlc) {
    uint8_t canintf = MCP2515_ReadRegister(0x2C); // CANINTF 레지스터

    if (canintf & 0x01) { // RX0IF 비트(bit0) = 1 → 수신 데이터 있음
        uint8_t read_instruction = 0x03;
        uint8_t start_addr = 0x61; // RXB0SIDH

        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET); // CS LOW
        HAL_SPI_Transmit(&hspi1, &read_instruction, 1, 100);
        HAL_SPI_Transmit(&hspi1, &start_addr, 1, 100);

        uint8_t header[5]; // SIDH, SIDL, EID8, EID0, DLC
        HAL_SPI_Receive(&hspi1, header, 5, 100);

        uint8_t dlc = header[4] & 0x0F;
        if (dlc > 8) dlc = 8;

        HAL_SPI_Receive(&hspi1, rx_data, dlc, 100);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET); // CS HIGH

        *rx_dlc = dlc;

        // RX0IF 플래그 클리어 (Bit Modify 명령 0x05)
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
        uint8_t bitmod_cmd[4] = {0x05, 0x2C, 0x01, 0x00};
        HAL_SPI_Transmit(&hspi1, bitmod_cmd, 4, 100);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);

        return 1;
    }
    return 0;
}

// 7. UART로 디버그 메시지 출력
void Debug_Print(char *msg) {
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
}

/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */

  HAL_Delay(100); // 전원 안정화 대기
  Debug_Print("=== STM32 CAN 수신 테스트 시작 ===\r\n");

  // 1. MCP2515 소프트웨어 리셋 -> Config Mode로 진입
  MCP2515_Reset();

  // 2. CANCTRL 레지스터 확인
  mcp2515_status = MCP2515_ReadRegister(0x0F);

  char dbg_buf[64];
  sprintf(dbg_buf, "Reset 후 CANCTRL: 0x%02X\r\n", mcp2515_status);
  Debug_Print(dbg_buf);

  // 상위 3비트(모드 비트)만 확인 (0x80 = Config Mode)
  if ((mcp2515_status & 0xE0) == 0x80) {
      Debug_Print("MCP2515 응답 확인! (Config Mode) Bitrate 설정 시작...\r\n");

      // 3. CNF3, CNF2, CNF1 (0x28번지부터 연속 쓰기) - 8MHz 크리스탈, 500kbps
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
      uint8_t write_cmd = 0x02;
      uint8_t cnf_start_addr = 0x28;
      HAL_SPI_Transmit(&hspi1, &write_cmd, 1, 100);
      HAL_SPI_Transmit(&hspi1, &cnf_start_addr, 1, 100);
      uint8_t cnf_values[3] = {0x02, 0x90, 0x00}; // CNF3, CNF2, CNF1
      HAL_SPI_Transmit(&hspi1, cnf_values, 3, 100);
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);

      // 4. 필터 없이 모든 메시지 수신하도록 설정
      MCP2515_SetReceiveAny();

      // 5. Normal Mode로 전환
      MCP2515_SetNormalMode();
      HAL_Delay(10);

      // 6. Normal Mode 전환 확인 (상위 3비트가 000이어야 Normal Mode)
      uint8_t check_mode = MCP2515_ReadRegister(0x0F);
      sprintf(dbg_buf, "전환 후 CANCTRL: 0x%02X (상위3비트=000 이면 Normal Mode 성공)\r\n", check_mode);
      Debug_Print(dbg_buf);

      if ((check_mode & 0xE0) == 0x00) {
          Debug_Print(">>> CAN 수신 준비 완료! 대기 중...\r\n");
      } else {
          Debug_Print(">>> 경고: Normal Mode 전환 실패\r\n");
      }
  } else {
      Debug_Print("경고: MCP2515 응답 없음! SPI 연결/전원 확인 필요\r\n");
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint8_t rx_data[8] = {0};
    uint8_t rx_dlc = 0;

    // ===== 진단용: CANINTF 레지스터 매번 출력 =====
    // RX0IF(bit0)뿐 아니라 에러 관련 비트도 같이 확인
    uint8_t canintf_debug = MCP2515_ReadRegister(0x2C); // CANINTF
    uint8_t eflg_debug = MCP2515_ReadRegister(0x2D);    // EFLG (에러 플래그)
    uint8_t canstat_debug = MCP2515_ReadRegister(0x0E); // CANSTAT (현재 모드)

    char dbg[80];
    sprintf(dbg, "CANINTF=0x%02X EFLG=0x%02X CANSTAT=0x%02X\r\n",
            canintf_debug, eflg_debug, canstat_debug);
    Debug_Print(dbg);
    // ================================================

    // 라즈베리파이로부터 CAN 메시지 수신 확인
    if (MCP2515_CheckAndReadRX0(rx_data, &rx_dlc)) {
        received_speed_cmd = rx_data[0];

        char buf[80];
        sprintf(buf, "수신: cmd=%d, DLC=%d, data=[%d,%d,%d,%d]\r\n",
                received_speed_cmd, rx_dlc, rx_data[0], rx_data[1], rx_data[2], rx_data[3]);
        Debug_Print(buf);

        if (received_speed_cmd == 0) {
            Debug_Print(">> 정지 명령 수신\r\n");
        } else if (received_speed_cmd == 1) {
            Debug_Print(">> 저속 명령 수신\r\n");
        } else if (received_speed_cmd == 2) {
            Debug_Print(">> 정속 명령 수신\r\n");
        }
    }

    HAL_Delay(300); // 로그가 너무 빨리 지나가지 않도록
  }
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = HAL_RCC_GetPCLK1Freq() > 42000000 ? RCC_HCLK_DIV2 : RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
