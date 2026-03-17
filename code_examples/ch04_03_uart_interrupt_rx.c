/* ch04_03_uart_interrupt_rx.c — 인터럽트 수신 + 1바이트 재등록 패턴
 *
 * CubeMX 설정:
 *   - USART2: Asynchronous, 115200-8-N-1
 *   - NVIC: USART2 global interrupt → Enable ☑️
 *   - PA5 = GPIO_Output (LED2)
 *
 * 동작:
 *   - PC에서 '1' 전송 → LED ON
 *   - PC에서 '0' 전송 → LED OFF
 *   - 다른 문자 → 에코 + 무시
 *   - main loop에서 LED 500ms 토글도 동시 진행 (인터럽트 덕분!)
 */

#include "main.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart2;

/* ===== 수신 버퍼 (간이 구현, 링 버퍼는 Ch06에서) ===== */
#define RX_BUF_SIZE  64

static volatile uint8_t  g_rx_byte;           /* 1바이트 수신 버퍼 */
static volatile uint8_t  g_rx_buf[RX_BUF_SIZE];
static volatile uint16_t g_rx_head = 0;       /* 쓰기 위치 */
static volatile uint16_t g_rx_tail = 0;       /* 읽기 위치 */
static volatile uint8_t  g_rx_flag = 0;       /* 수신 이벤트 플래그 */

/* ===== 인터럽트 수신 콜백 ===== */

/* HAL이 RXNE 인터럽트 처리 후 호출하는 콜백 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        /* 1) 수신된 바이트를 버퍼에 저장 */
        uint16_t next = (g_rx_head + 1) % RX_BUF_SIZE;
        if (next != g_rx_tail) {  /* 오버플로 방지 */
            g_rx_buf[g_rx_head] = g_rx_byte;
            g_rx_head = next;
        }
        g_rx_flag = 1;

        /* 2) ★ 핵심: 다음 1바이트 수신 재등록! ★
         *    이 줄을 빼면 첫 1바이트만 수신됩니다! */
        HAL_UART_Receive_IT(&huart2, (uint8_t *)&g_rx_byte, 1);
    }
}

/* ===== 유틸리티 함수 ===== */

/* 수신 버퍼에 읽을 데이터가 있는가? */
uint16_t uart_available(void)
{
    return (g_rx_head - g_rx_tail + RX_BUF_SIZE) % RX_BUF_SIZE;
}

/* 수신 버퍼에서 1바이트 읽기 (비블로킹) */
uint8_t uart_read_byte(void)
{
    if (g_rx_head == g_rx_tail) {
        return 0;  /* 버퍼 비어있음 */
    }
    uint8_t byte = g_rx_buf[g_rx_tail];
    g_rx_tail = (g_rx_tail + 1) % RX_BUF_SIZE;
    return byte;
}

/* ===== 메인 애플리케이션 ===== */

void app_interrupt_rx(void)
{
    uint32_t last_led_tick = 0;

    LOG_I("=== 인터럽트 수신 모드 시작 ===");
    printf("\r\nUART Interrupt RX Demo\r\n");
    printf("Send '1' for LED ON, '0' for LED OFF\r\n");

    /* ★ 초기 등록: 첫 1바이트 수신 대기 시작 */
    HAL_UART_Receive_IT(&huart2,
                        (uint8_t *)&g_rx_byte, 1);
    LOG_D("Receive_IT 초기 등록 완료");

    while (1)
    {
        /* 1) 수신 데이터 처리 (비블로킹) */
        if (uart_available() > 0) {
            uint8_t rx = uart_read_byte();
            LOG_D("수신: 0x%02X ('%c')", rx, rx);

            switch (rx) {
            case '1':
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,
                                  GPIO_PIN_SET);
                printf("[CMD] LED ON\r\n");
                break;
            case '0':
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,
                                  GPIO_PIN_RESET);
                printf("[CMD] LED OFF\r\n");
                break;
            default:
                /* 에코 */
                printf("[ECHO] '%c' (0x%02X)\r\n", rx, rx);
                break;
            }
        }

        /* 2) LED 토글 — 인터럽트 덕분에 멈추지 않음! */
        uint32_t now = HAL_GetTick();
        if (now - last_led_tick >= 500) {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            /* 주의: LED 상태를 '1'/'0' 명령이 덮어쓸 수 있음
             * → 실제 프로젝트에서는 모드 분리 필요 */
            last_led_tick = now;
        }
    }
}
