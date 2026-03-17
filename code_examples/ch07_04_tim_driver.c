/* ch07_04_tim_driver.c — TIM Driver 구현
 *
 * 레이어: Driver
 * 의존: HAL (stm32f4xx_hal_tim.h) — 하위 레이어만 참조
 *
 * 이 파일은 HAL TIM 상세를 캡슐화합니다.
 * 상위 레이어(App)는 tim_driver.h의 인터페이스만 사용합니다.
 *
 * CubeMX 설정:
 *   - TIM2: Internal Clock, PSC=8399, ARR=4999 (500ms 기본)
 *   - TIM2 CH1: PWM Generation CH1 → PA0
 *   - NVIC: TIM2 global interrupt → Enable
 */

#include "tim_driver.h"
#include "main.h"       /* HAL, GPIO 정의 */
#include "log.h"        /* LOG_D/I/W/E */

/* ===== HAL 핸들 (CubeMX 생성) ===== */
extern TIM_HandleTypeDef htim2;

/* ===== 내부 상태 ===== */
static volatile tim_callback_t s_uev_callback = NULL;

/* TIM2 클럭 주파수 (APB1 타이머 클럭) */
#define TIM2_CLK_HZ     84000000U

/* ===== 초기화 ===== */

void tim_driver_init(void)
{
    s_uev_callback = NULL;
    LOG_I("tim_driver 초기화 완료 (TIM2, APB1=%luHz)",
          TIM2_CLK_HZ);
}

/* ===== 주기 타이머 ===== */

tim_status_t tim_start(void)
{
    HAL_StatusTypeDef hal_st;
    hal_st = HAL_TIM_Base_Start_IT(&htim2);
    if (hal_st != HAL_OK) {
        LOG_E("tim_start 실패: hal=%d", hal_st);
        return TIM_ERROR;
    }

    LOG_I("TIM2 인터럽트 시작");
    return TIM_OK;
}

tim_status_t tim_stop(void)
{
    HAL_StatusTypeDef hal_st;
    hal_st = HAL_TIM_Base_Stop_IT(&htim2);
    if (hal_st != HAL_OK) {
        LOG_E("tim_stop 실패: hal=%d", hal_st);
        return TIM_ERROR;
    }

    LOG_I("TIM2 인터럽트 정지");
    return TIM_OK;
}

tim_status_t tim_set_period_ms(uint32_t period_ms)
{
    if (period_ms == 0 || period_ms > 60000) {
        LOG_W("tim_set_period_ms: 범위 초과 (%lu ms)", period_ms);
        return TIM_PARAM;
    }

    /*
     * 주기 계산:
     *   T = (PSC+1) × (ARR+1) / f_clk
     *   → (PSC+1) × (ARR+1) = T × f_clk
     *
     * 전략: PSC로 1kHz 카운터를 만들고, ARR로 ms 설정
     *   PSC = (f_clk / 1000) - 1 = 83999
     *   ARR = period_ms - 1
     *
     * 단, period_ms가 짧을 때는 PSC를 줄여 해상도 확보
     */
    uint32_t psc, arr;

    if (period_ms >= 10) {
        /* 10ms 이상: PSC로 1kHz 카운터 */
        psc = (TIM2_CLK_HZ / 1000) - 1;  /* 83999 */
        arr = period_ms - 1;
    } else {
        /* 1~9ms: PSC로 10kHz 카운터 */
        psc = (TIM2_CLK_HZ / 10000) - 1; /* 8399 */
        arr = period_ms * 10 - 1;
    }

    __HAL_TIM_SET_PRESCALER(&htim2, psc);
    __HAL_TIM_SET_AUTORELOAD(&htim2, arr);

    /* 새 설정 즉시 반영 (UG 비트 세트) */
    htim2.Instance->EGR = TIM_EGR_UG;

    LOG_D("주기 변경: %lu ms (PSC=%lu, ARR=%lu)",
          period_ms, psc, arr);
    return TIM_OK;
}

/* ===== PWM ===== */

tim_status_t pwm_start(void)
{
    HAL_StatusTypeDef hal_st;
    hal_st = HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    if (hal_st != HAL_OK) {
        LOG_E("pwm_start 실패: hal=%d", hal_st);
        return TIM_ERROR;
    }

    LOG_I("PWM 출력 시작 (TIM2 CH1, PA0)");
    return TIM_OK;
}

tim_status_t pwm_stop(void)
{
    HAL_StatusTypeDef hal_st;
    hal_st = HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    if (hal_st != HAL_OK) {
        LOG_E("pwm_stop 실패: hal=%d", hal_st);
        return TIM_ERROR;
    }

    LOG_I("PWM 출력 정지");
    return TIM_OK;
}

tim_status_t pwm_set_duty(uint8_t percent)
{
    if (percent > 100) {
        LOG_W("pwm_set_duty: 범위 초과 (%d%%)", percent);
        return TIM_PARAM;
    }

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim2);
    uint32_t ccr = (arr + 1) * percent / 100;

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, ccr);
    LOG_D("PWM 듀티: %d%% (CCR=%lu, ARR=%lu)",
          percent, ccr, arr);
    return TIM_OK;
}

/* ===== 콜백 등록 ===== */

void tim_register_callback(tim_callback_t cb)
{
    s_uev_callback = cb;
    LOG_D("UEV 콜백 %s", cb ? "등록" : "해제");
}

/* ===== HAL 콜백 (UEV 인터럽트) ===== */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2) {
        return;
    }

    if (s_uev_callback != NULL) {
        s_uev_callback();
    }
}
