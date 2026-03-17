/* ch07_03_pwm_led_brightness.c — PWM 4단계 LED 밝기 제어
 *
 * 레이어: App (학습용)
 * 목적: TIM2 CH1 PWM으로 LED 밝기를 4단계로 변경
 *
 * CubeMX 설정:
 *   - TIM2 CH1: PWM Generation CH1
 *   - PA0 = TIM2_CH1 (NUCLEO A0 핀)
 *   - PSC = 83, ARR = 999 → 1kHz PWM 주파수
 *   - 외부 LED를 PA0에 연결 (저항 330옴 직렬)
 *
 * Duty 계산: CCR = (ARR + 1) × duty% / 100
 *   - 25%  → CCR = 1000 × 0.25 = 250
 *   - 50%  → CCR = 1000 × 0.50 = 500
 *   - 75%  → CCR = 1000 × 0.75 = 750
 *   - 100% → CCR = 1000 × 1.00 = 1000
 */

#include "main.h"
#include "log.h"

extern TIM_HandleTypeDef htim2;

/* PWM 시작 */
void pwm_led_brightness_start(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    LOG_I("PWM LED 밝기 제어 시작 (PA0, 1kHz)");
}

/* 듀티 변경 (0~100%) */
void pwm_led_set_duty(uint8_t percent)
{
    if (percent > 100) {
        LOG_W("듀티 범위 초과: %d%% → 100%%로 제한", percent);
        percent = 100;
    }

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim2);
    uint32_t ccr = (arr + 1) * percent / 100;

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, ccr);
    LOG_D("PWM 듀티 변경: %d%% (CCR=%lu)", percent, ccr);
}

/* 4단계 밝기 데모 (main 루프에서 호출) */
void pwm_led_brightness_demo(void)
{
    const uint8_t levels[] = { 25, 50, 75, 100 };
    const uint32_t count = sizeof(levels) / sizeof(levels[0]);

    pwm_led_brightness_start();

    for (uint32_t i = 0; i < count; i++) {
        pwm_led_set_duty(levels[i]);
        LOG_I("밝기 단계 %lu: %d%%", i + 1, levels[i]);
        HAL_Delay(2000);  /* 2초 유지 */
    }

    LOG_I("밝기 데모 완료");
}
