/* ch08_06_tim_driver_extended.c — tim_driver 확장 (TIM1 Complementary PWM + Input Capture)
 *
 * 레이어: Driver
 * 의존: HAL (stm32f4xx_hal_tim_ex.h)
 *
 * v0.7 tim_driver에 다음 기능을 추가합니다:
 *   ① Complementary PWM (TIM1 CH1 + CH1N)
 *   ② Input Capture (TIM3 CH1)
 *
 * 기존 API(tim_start/stop, pwm_start/stop 등)는 변경 없이 유지됩니다.
 * Ch04 uart_driver 확장 패턴과 동일합니다:
 *   "기존 인터페이스 불변, 새 기능만 추가"
 *
 * CubeMX 추가 설정:
 *   - TIM1: CH1 PWM(PA8), CH1N PWM(PB13), Dead-time=84(1us)
 *   - TIM3: CH1 Input Capture(PA6), PSC=83(1MHz), ARR=0xFFFF
 */

#include "tim_driver.h"
#include "main.h"
#include "log.h"

/* ===== HAL 핸들 (CubeMX 생성) ===== */
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;

#define TIM1_CLK_HZ     84000000U
#define IC_CNT_FREQ_HZ  1000000U   /* TIM3: 84MHz / 84 = 1MHz */

/* ===== 내부 상태 ===== */
static volatile uint32_t s_ic_capture_prev = 0;
static volatile uint32_t s_ic_period_us    = 0;
static volatile uint8_t  s_ic_ready        = 0;

/* ==============================
 * Complementary PWM (TIM1)
 * ============================== */

tim_status_t cpwm_start(void)
{
    HAL_StatusTypeDef st;

    /* CH1 (주 출력, PA8) */
    st = HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    if (st != HAL_OK) {
        LOG_E("cpwm CH1 시작 실패: %d", st);
        return TIM_ERROR;
    }

    /* CH1N (보상 출력, PB13) */
    st = HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
    if (st != HAL_OK) {
        LOG_E("cpwm CH1N 시작 실패: %d", st);
        return TIM_ERROR;
    }

    LOG_I("Complementary PWM 시작 (PA8 + PB13)");
    return TIM_OK;
}

tim_status_t cpwm_stop(void)
{
    HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    LOG_I("Complementary PWM 정지");
    return TIM_OK;
}

tim_status_t cpwm_set_duty(uint8_t percent)
{
    if (percent > 100) {
        LOG_W("cpwm_set_duty: 범위 초과 (%d%%)", percent);
        return TIM_PARAM;
    }

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);
    uint32_t ccr = (arr + 1) * percent / 100;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);
    LOG_D("CPWM 듀티: %d%% (CCR=%lu)", percent, ccr);
    return TIM_OK;
}

/* ==============================
 * Input Capture (TIM3)
 * ============================== */

tim_status_t ic_start(void)
{
    s_ic_ready = 0;
    s_ic_capture_prev = 0;

    HAL_StatusTypeDef st;
    st = HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
    if (st != HAL_OK) {
        LOG_E("IC 시작 실패: %d", st);
        return TIM_ERROR;
    }

    LOG_I("Input Capture 시작 (TIM3 CH1, PA6)");
    return TIM_OK;
}

tim_status_t ic_stop(void)
{
    HAL_TIM_IC_Stop_IT(&htim3, TIM_CHANNEL_1);
    LOG_I("Input Capture 정지");
    return TIM_OK;
}

uint32_t ic_get_frequency(void)
{
    if (!s_ic_ready || s_ic_period_us == 0) {
        return 0;
    }
    return IC_CNT_FREQ_HZ / s_ic_period_us;
}

uint32_t ic_get_period_us(void)
{
    return s_ic_ready ? s_ic_period_us : 0;
}

/* ===== HAL IC 콜백 ===== */

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM3) {
        return;
    }

    uint32_t captured = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

    if (s_ic_capture_prev != 0) {
        /* 오버플로 처리 (16비트) */
        if (captured >= s_ic_capture_prev) {
            s_ic_period_us = captured - s_ic_capture_prev;
        } else {
            s_ic_period_us = (0xFFFF - s_ic_capture_prev) + captured + 1;
        }
        s_ic_ready = 1;
    }
    s_ic_capture_prev = captured;
}
