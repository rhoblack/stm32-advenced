/**
 * @file    ch13_display_fsm.c
 * @brief   DISPLAY FSM 핵심 동작 예제 — 교재 설명용 (Ch13, v1.3)
 *
 * 이 파일은 ui_service.c의 FSM 핵심 로직만 추출한 교육용 예제입니다.
 * 실제 프로젝트에서는 ui_service.c를 사용하세요.
 */

#include "ui_service.h"
#include "gfx_service.h"
#include "log.h"

/* ===== 버튼 이벤트 → FSM 연결: 메인 루프 패턴 ===== */

/* 외부 버튼 드라이버에서 설정되는 플래그 (Ch01 EXTI 활용) */
volatile uint8_t g_btn_short_pressed = 0;  /* 짧게 누름 플래그 */
volatile uint8_t g_btn_long_pressed  = 0;  /* 길게 누름 플래그 */

/* RTC 1초 콜백에서 설정 */
volatile uint8_t g_rtc_1sec_tick = 0;

/**
 * @brief 메인 루프 예시 — 이벤트 폴링 + FSM 처리
 *
 * 실제 main.c에서 무한 루프 내부에 이 코드를 넣습니다.
 */
void ch13_main_loop_example(void)
{
    /* 버튼 짧게 누름 처리 */
    if (g_btn_short_pressed != 0U) {
        g_btn_short_pressed = 0U;
        LOG_I("BTN_SHORT 감지 → FSM 이벤트 전달");
        UI_HandleEvent(UI_EVENT_BTN_SHORT);
    }

    /* 버튼 길게 누름 처리 */
    if (g_btn_long_pressed != 0U) {
        g_btn_long_pressed = 0U;
        LOG_I("BTN_LONG 감지 → 강제 갱신");
        UI_HandleEvent(UI_EVENT_BTN_LONG);
    }

    /* 1초 RTC 틱 처리 */
    if (g_rtc_1sec_tick != 0U) {
        g_rtc_1sec_tick = 0U;
        UI_HandleEvent(UI_EVENT_TICK);
    }
}

/**
 * @brief RTC 알람 콜백 (stm32f4xx_hal_rtc.c에서 오버라이드)
 *        1초마다 호출 → 갱신 요청 플래그 설정
 *
 * @note  ISR에서 호출되므로 LOG_I 사용 금지, 플래그 설정만 허용
 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    (void)hrtc;
    g_rtc_1sec_tick = 1U;  /* 메인 루프에서 처리 */
    UI_RequestRefresh();    /* UI 갱신 요청 */
}

/**
 * @brief EXTI 콜백 (버튼 누름 감지)
 *        Ch01에서 구현한 debouncing 처리 이후 호출
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /* NUCLEO-F411RE: User Button = PC13 */
    if (GPIO_Pin == GPIO_PIN_13) {
        /* 버튼 누름 시간으로 짧게/길게 구분 (500ms 기준) */
        /* 실제 구현: TIM 타이머로 누름 시간 측정 */
        g_btn_short_pressed = 1U;  /* 간략화: 모두 SHORT로 처리 */
    }
}
