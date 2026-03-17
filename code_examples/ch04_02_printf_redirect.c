/* ch04_02_printf_redirect.c — printf UART 리다이렉트 + LED 블로킹 체험
 *
 * CubeMX 설정: ch04_01과 동일
 *
 * _write() 함수를 오버라이드하여 printf 출력을 UART로 전환합니다.
 * Ch02에서는 SWO(ITM)로 출력했지만, 이제 UART로 출력합니다.
 *
 * 주의: 이 파일은 syscalls.c 또는 main.c에 배치합니다.
 *       STM32CubeIDE 기본 프로젝트에서는 syscalls.c에
 *       __weak _write()가 이미 있으므로 main.c에서 재정의하면 됩니다.
 */

#include "main.h"
#include "log.h"
#include <stdio.h>

extern UART_HandleTypeDef huart2;

/* ===== printf → UART 리다이렉트 ===== */

/* _write: newlib의 저수준 출력 함수
 * printf() → _write() → HAL_UART_Transmit()
 *
 * file=1 → stdout, file=2 → stderr
 */
int _write(int file, char *ptr, int len)
{
    (void)file;  /* stdout/stderr 구분 없이 모두 UART로 출력 */

    HAL_UART_Transmit(&huart2,
                      (uint8_t *)ptr, (uint16_t)len,
                      100);
    return len;
}

/* ===== LED 블로킹 체험 코드 ===== */

void app_printf_demo(void)
{
    uint32_t last_led_tick = 0;
    uint32_t count = 0;

    LOG_I("=== printf UART 리다이렉트 테스트 ===");

    /* printf가 UART로 출력되는지 확인 */
    printf("\r\n[printf] UART 출력 테스트 시작\r\n");
    printf("[printf] MCU: STM32F411RE @ %luMHz\r\n",
           SystemCoreClock / 1000000UL);

    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* 1초마다 시스템 상태 출력 (printf 사용) */
        if (now - last_led_tick >= 1000) {
            count++;

            /* ⚠️ printf → _write → HAL_UART_Transmit (폴링)
             *    긴 문자열일수록 블로킹 시간이 늘어남 */
            printf("[%5lu] uptime=%lus  LED=%s  count=%lu\r\n",
                   now,
                   now / 1000,
                   (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5)
                    == GPIO_PIN_SET) ? "ON" : "OFF",
                   count);

            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            last_led_tick = now;
            LOG_D("LED 토글 완료, count=%lu", count);
        }
    }
}
