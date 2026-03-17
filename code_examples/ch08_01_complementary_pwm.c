/* ch08_01_complementary_pwm.c — TIM1 Complementary PWM + Dead-time 설정
 *
 * 레이어: App (실습 예제)
 * 의존: HAL (stm32f4xx_hal_tim_ex.h)
 *
 * CubeMX 설정:
 *   - TIM1: Internal Clock, PSC=83, ARR=999 (84MHz/84/1000 = 1kHz)
 *   - TIM1 CH1: PWM Generation CH1 → PA8
 *   - TIM1 CH1N: PWM Generation CH1N → PB13
 *   - Dead-time: 84 (= 1us @ 84MHz)
 *   - BDTR: MOE=Enable, OSSI/OSSR=Enable
 *   - NVIC: TIM1 불필요 (PWM은 인터럽트 없이 동작)
 */

#include "main.h"
#include "log.h"

extern TIM_HandleTypeDef htim1;

/* TIM1 클럭 주파수 (APB2 타이머 클럭) */
#define TIM1_CLK_HZ     84000000U

/**
 * @brief Complementary PWM 시작 (CH1 + CH1N)
 *
 * 핵심 차이:
 *   - 일반 PWM:  HAL_TIM_PWM_Start()
 *   - 보상 PWM:  HAL_TIMEx_PWMN_Start()  ← 'N' 주의!
 *   - 둘 다 호출해야 CH1 + CH1N 동시 출력
 */
void complementary_pwm_demo(void)
{
    HAL_StatusTypeDef hal_st;

    /* CH1 (주 출력) 시작 — PA8 */
    hal_st = HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    if (hal_st != HAL_OK) {
        LOG_E("TIM1 CH1 PWM 시작 실패: %d", hal_st);
        return;
    }
    LOG_I("TIM1 CH1 PWM 시작 (PA8)");

    /* CH1N (보상 출력) 시작 — PB13 */
    hal_st = HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
    if (hal_st != HAL_OK) {
        LOG_E("TIM1 CH1N PWM 시작 실패: %d", hal_st);
        return;
    }
    LOG_I("TIM1 CH1N PWM 시작 (PB13)");

    /* 듀티 50% 설정 */
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);
    uint32_t ccr = (arr + 1) * 50 / 100;  /* 500 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);
    LOG_D("CPWM 듀티 50%% (CCR=%lu, ARR=%lu)", ccr, arr);

    /* 결과 관찰:
     * - PA8:  ‾‾‾_____ (50% HIGH)
     * - PB13: _____‾‾‾ (50% HIGH, 반전)
     * - Dead-time 1us 구간에서 둘 다 LOW
     * - 오실로스코프 2채널로 확인
     */
    LOG_I("오실로스코프로 PA8/PB13 파형 확인하세요");
    LOG_I("Dead-time = %lu ns",
          (uint32_t)(1000000000ULL * 84 / TIM1_CLK_HZ));
}
