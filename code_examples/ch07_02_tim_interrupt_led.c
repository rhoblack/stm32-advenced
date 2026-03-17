/* ch07_02_tim_interrupt_led.c — TIM2 주기 인터럽트 LED 토글
 *
 * 레이어: App (학습용)
 * 목적: TIM2 UEV 인터럽트로 500ms마다 LED 토글
 *
 * CubeMX 설정:
 *   - TIM2: Internal Clock
 *   - PSC = 8399, ARR = 4999 → 500ms 주기
 *   - NVIC: TIM2 global interrupt → Enable
 *   - PA5 = GPIO_Output (NUCLEO LD2)
 */

#include "main.h"
#include "log.h"

extern TIM_HandleTypeDef htim2;

/* TIM2 인터럽트 시작 */
void tim_interrupt_led_start(void)
{
    /* TIM2 인터럽트 모드 시작 */
    HAL_TIM_Base_Start_IT(&htim2);
    LOG_I("TIM2 인터럽트 LED 토글 시작 (500ms 주기)");
}

/* UEV 콜백 — HAL이 자동 호출 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2) {
        return;  /* 다른 타이머는 무시 */
    }

    /* LED 토글 (PA5 = NUCLEO LD2) */
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    LOG_D("TIM2 UEV: LED 토글");
}
