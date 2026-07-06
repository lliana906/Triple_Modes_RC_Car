#include "bt.h"
#include "usart.h"
#include "mot.h"

uint8_t Rx_data[1];

void BT_Init(void)
{
    HAL_UART_Receive_IT(&huart1, Rx_data, sizeof(Rx_data));
}

/* UART 인터럽트 콜백 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        HAL_UART_Transmit_IT(&huart1, Rx_data, sizeof(Rx_data));
        HAL_UART_Receive_IT(&huart1, Rx_data, sizeof(Rx_data));
    }
}

/* 수신 데이터 처리 */
void BT_Process(void)
{
    if(Rx_data[0])
    {
        switch(Rx_data[0])
        {
            case 'w':
                MotorA_Control(1000, 1);
                MotorB_Control(1000, 1);
                break;

            case 's':
                MotorA_Control(1000, 0);
                MotorB_Control(1000, 0);
                break;

            case 'a':
                MotorA_Control(1000, 0);
                MotorB_Control(1000, 1);
                break;

            case 'd':
                MotorA_Control(1000, 1);
                MotorB_Control(1000, 0);
                break;

            case 'c':
                MotorA_Control(0, 0);
                MotorB_Control(0, 0);
                break;
        }

        Rx_data[0] = 0;
    }
}
