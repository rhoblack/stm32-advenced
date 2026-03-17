/* ch08_03_one_pulse.c — One-Pulse 모드 데모
 *
 * 레이어: App (실습 예제)
 * 의존: HAL (stm32f4xx_hal_tim.h)
 *
 * CubeMX 설정:
 *   - TIM4: One Pulse Mode → Enable
 *   - TIM4 CH1: Output Compare → PB6
 *   - PSC=8399 (84MHz/8400 = 10kHz 카운터)
 *   - ARR=99 (100카운트 = 10ms 총 주기)
 *   - CCR=30 (3ms 지연 후 펄스 시작)
 *   - Mode: PWM Mode 2
 *   - Trigger: TI1FP1 (외부 트리거)
 *
 * 동작:
 *   PB6에 외부 트리거 → 3ms 지연 → 7ms HIGH → 자동 정지
 *   "카메라 플래시"처럼 한 번만 발생합니다.
 */

#include "main.h"
#include "log.h"

extern TIM_HandleTypeDef htim4;

/**
 * @brief One-Pulse 모드 시작 — 트리거 대기 상태
 */
void one_pulse_start(void)
{
    HAL_TIM_OnePulse_Start(&htim4, TIM_CHANNEL_1);
    LOG_I("One-Pulse 대기 중 (TIM4 CH1, PB6)");
    LOG_I("트리거 입력 시 1회 펄스 출력 후 자동 정지");
    LOG_D("  지연: 3ms, 펄스 폭: 7ms, 총 주기: 10ms");
}

/**
 * @brief One-Pulse 모드 정지
 */
void one_pulse_stop(void)
{
    HAL_TIM_OnePulse_Stop(&htim4, TIM_CHANNEL_1);
    LOG_I("One-Pulse 정지");
}
