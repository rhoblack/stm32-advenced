/* ch08_02_input_capture.c — Input Capture 주파수 측정
 *
 * 레이어: App (실습 예제)
 * 의존: HAL (stm32f4xx_hal_tim.h)
 *
 * CubeMX 설정:
 *   - TIM3 CH1: Input Capture direct mode → PA6
 *   - TIM3: Internal Clock, PSC=83 (84MHz/84 = 1MHz 카운터)
 *   - ARR=0xFFFF (16비트 최대, 자유 실행)
 *   - Polarity: Rising Edge
 *   - NVIC: TIM3 global interrupt → Enable
 *
 * 동작:
 *   PA6 핀에 Rising Edge가 들어올 때마다 CCR에 CNT 값 캡처.
 *   두 캡처 값의 차이로 주파수를 계산합니다.
 */

#include "main.h"
#include "log.h"

extern TIM_HandleTypeDef htim3;

/* 카운터 클럭 = 84MHz / (83+1) = 1MHz → 1 카운트 = 1us */
#define IC_CNT_FREQ_HZ  1000000U

/* 캡처 데이터 */
static volatile uint32_t s_capture_1st  = 0;
static volatile uint32_t s_capture_2nd  = 0;
static volatile uint8_t  s_capture_done = 0;
static volatile uint8_t  s_capture_idx  = 0;

/**
 * @brief Input Capture 측정 시작
 */
void input_capture_start(void)
{
    s_capture_idx  = 0;
    s_capture_done = 0;

    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
    LOG_I("Input Capture 시작 (TIM3 CH1, PA6)");
    LOG_I("PA6에 신호를 입력하세요 (Rising Edge 2회 필요)");
}

/**
 * @brief HAL Input Capture 콜백
 *
 * Rising Edge가 발생하면 HAL이 이 함수를 호출합니다.
 * CCR에 저장된 캡처 값을 읽어옵니다.
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM3) {
        return;
    }

    uint32_t captured = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

    if (s_capture_idx == 0) {
        s_capture_1st = captured;
        s_capture_idx = 1;
    } else {
        s_capture_2nd = captured;
        s_capture_done = 1;
        HAL_TIM_IC_Stop_IT(htim, TIM_CHANNEL_1);
    }
}

/**
 * @brief 메인 루프에서 호출 — 캡처 완료 시 주파수 출력
 */
void input_capture_poll(void)
{
    if (!s_capture_done) {
        return;
    }
    s_capture_done = 0;

    /* 카운터 오버플로 처리 (16비트 TIM3) */
    uint32_t diff;
    if (s_capture_2nd >= s_capture_1st) {
        diff = s_capture_2nd - s_capture_1st;
    } else {
        diff = (0xFFFF - s_capture_1st) + s_capture_2nd + 1;
    }

    if (diff == 0) {
        LOG_W("캡처 차이 = 0, 측정 불가");
        return;
    }

    uint32_t freq_hz   = IC_CNT_FREQ_HZ / diff;
    uint32_t period_us  = diff;  /* 1MHz 기준이므로 diff = us */

    LOG_I("=== Input Capture 결과 ===");
    LOG_I("  캡처1 = %lu, 캡처2 = %lu", s_capture_1st, s_capture_2nd);
    LOG_I("  차이   = %lu 카운트 (%lu us)", diff, period_us);
    LOG_I("  주파수 = %lu Hz", freq_hz);
    LOG_I("  주기   = %lu us", period_us);
}
