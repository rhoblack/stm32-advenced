/* ch04_01_uart_polling_echo.c — UART 폴링 에코 + LED 토글
 *
 * CubeMX 설정:
 *   - USART2: Asynchronous, 115200-8-N-1
 *   - PA2 = USART2_TX, PA3 = USART2_RX (ST-Link Virtual COM)
 *   - PA5 = GPIO_Output (LED2)
 *
 * 동작:
 *   1. PC에서 보낸 문자를 그대로 되돌려 보냄 (에코)
 *   2. 동시에 500ms 주기로 LED 토글
 *   3. 폴링 방식이므로 UART 수신 대기 중 LED가 멈추는 것을 관찰
 */

#include "main.h"
#include "log.h"    /* Ch02 로그 시스템 */
#include <string.h>

extern UART_HandleTypeDef huart2;

/* 송신: 문자열을 UART로 전송 (폴링) */
void uart_send_string(const char *str)
{
    uint16_t len = (uint16_t)strlen(str);
    HAL_StatusTypeDef status;

    status = HAL_UART_Transmit(&huart2,
                               (uint8_t *)str, len,
                               100);  /* 타임아웃 100ms */
    if (status != HAL_OK) {
        LOG_W("UART TX 실패: status=%d", status);
    }
}

/* 수신: 1바이트 폴링 수신 (블로킹!) */
uint8_t uart_receive_byte_blocking(void)
{
    uint8_t byte = 0;
    HAL_StatusTypeDef status;

    /* ⚠️ 여기서 CPU가 멈춘다! */
    status = HAL_UART_Receive(&huart2,
                              &byte, 1,
                              1000);  /* 1초 타임아웃 */
    if (status == HAL_TIMEOUT) {
        /* 1초 동안 수신 없음 — 정상 상황 */
        return 0;
    }
    if (status != HAL_OK) {
        LOG_W("UART RX 실패: status=%d", status);
        return 0;
    }
    return byte;
}

/* main 루프에서 호출 */
void app_polling_echo(void)
{
    uint32_t last_led_tick = 0;

    LOG_I("=== 폴링 에코 시작 (115200-8-N-1) ===");
    uart_send_string("Hello UART! Echo ready.\r\n");

    while (1)
    {
        /* 1) UART 수신 대기 (블로킹!) */
        uint8_t rx = uart_receive_byte_blocking();
        if (rx != 0) {
            /* 수신된 바이트를 에코 */
            HAL_UART_Transmit(&huart2, &rx, 1, 10);
            LOG_D("에코: 0x%02X ('%c')", rx, rx);
        }

        /* 2) LED 토글 — 500ms 주기 */
        uint32_t now = HAL_GetTick();
        if (now - last_led_tick >= 500) {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            last_led_tick = now;
        }
        /* ⚠️ 관찰: UART 수신 대기 1초 동안 LED 토글이 안 됨!
         *    → 폴링의 한계. §4.4에서 인터럽트로 해결합니다. */
    }
}
