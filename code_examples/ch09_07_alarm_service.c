/* ch09_07_alarm_service.c — Alarm FSM 구현
 *
 * 레이어: Service
 * 의존: rtc_driver.h (Driver 레이어)
 *
 * FSM 구현 패턴 (Ch08 stopwatch_service와 동일 구조!):
 *   - enum으로 상태 정의 (alarm_state_t)
 *   - switch-case로 전이 처리
 *   - 이벤트(RTC 인터럽트 + 버튼) → FSM → 액션(LED/부저)
 *   - ISR에서는 플래그만, 메인에서 FSM 처리
 *
 * Ch08과의 차이:
 *   - 상태: 3개 → 4개 (ACKNOWLEDGED 추가)
 *   - 이벤트 소스: 버튼만 → RTC + 버튼
 *   - 나머지 패턴은 동일!
 */

/* ===== ch09_07_alarm_service.h — 공개 인터페이스 ===== */
/*
 * #ifndef ALARM_SERVICE_H
 * #define ALARM_SERVICE_H
 *
 * #include <stdint.h>
 *
 * typedef enum {
 *     ALARM_STATE_IDLE         = 0,  // 알람 미설정
 *     ALARM_STATE_ARMED        = 1,  // 알람 대기 중
 *     ALARM_STATE_TRIGGERED    = 2,  // 알람 울리는 중!
 *     ALARM_STATE_ACKNOWLEDGED = 3   // 사용자 확인 완료
 * } alarm_state_t;
 *
 * typedef enum {
 *     ALARM_EVT_SET_ALARM    = 0,  // 알람 설정 요청
 *     ALARM_EVT_CANCEL       = 1,  // 알람 취소
 *     ALARM_EVT_ALARM_MATCH  = 2,  // RTC 시각 매칭 (ISR에서)
 *     ALARM_EVT_ACK          = 3,  // 사용자 확인 (버튼)
 *     ALARM_EVT_TIMEOUT      = 4   // 자동 복귀 타임아웃
 * } alarm_event_t;
 *
 * void         alarm_service_init(void);
 * void         alarm_service_handle_event(alarm_event_t evt);
 * void         alarm_service_process(void);  // main loop에서 호출
 * alarm_state_t alarm_service_get_state(void);
 * void         alarm_service_set_time(uint8_t h, uint8_t m, uint8_t s);
 *
 * #endif // ALARM_SERVICE_H
 */

/* ===== ch09_07_alarm_service.c — 구현 ===== */

#include "alarm_service.h"
#include "rtc_driver.h"
#include "log.h"

/* ===== 내부 상태 ===== */
static volatile alarm_state_t s_state = ALARM_STATE_IDLE;
static volatile uint8_t s_alarm_flag  = 0;   /* ISR → main 플래그 */
static rtc_time_t s_alarm_time = {0};        /* 설정된 알람 시각 */
static uint32_t s_trigger_tick = 0;          /* TRIGGERED 진입 시각 */
static uint32_t s_ack_tick     = 0;          /* ACKNOWLEDGED 진입 시각 */

#define ALARM_ACK_TIMEOUT_MS  30000   /* 30초 후 자동 타임아웃 (TRIGGERED → ACKNOWLEDGED) */
#define ALARM_IDLE_TIMEOUT_MS 2000    /* 2초 후 ACKNOWLEDGED → IDLE 자동 복귀 */

/* ===== 상태 이름 (로그용) ===== */
static const char * const STATE_NAMES[] = {
    "IDLE", "ARMED", "TRIGGERED", "ACKNOWLEDGED"
};

/* ===== 내부: RTC 알람 콜백 (ISR 컨텍스트) ===== */

static void alarm_isr_callback(void)
{
    /* ISR에서는 플래그만! (Ch08 패턴 동일) */
    s_alarm_flag = 1;
}

/* ===== 내부: 액션 함수들 ===== */

static void action_start_alarm_output(void)
{
    /* LED 점멸 시작 (PA5, 온보드 LD2) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    LOG_I("*** 알람 울림! LED ON ***");
}

static void action_stop_alarm_output(void)
{
    /* LED 끄기 */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    LOG_I("알람 출력 정지");
}

/* ===== 초기화 ===== */

void alarm_service_init(void)
{
    s_state = ALARM_STATE_IDLE;
    s_alarm_flag = 0;

    /* rtc_driver에 알람 콜백 등록 */
    rtc_register_alarm_callback(alarm_isr_callback);

    LOG_I("alarm_service 초기화 완료");
    LOG_D("  FSM 초기 상태: %s", STATE_NAMES[s_state]);
}

/* ===== 알람 시각 설정 ===== */

void alarm_service_set_time(uint8_t h, uint8_t m, uint8_t s)
{
    s_alarm_time.hours   = h;
    s_alarm_time.minutes = m;
    s_alarm_time.seconds = s;
    LOG_D("알람 시각 저장: %02d:%02d:%02d", h, m, s);
}

/* ===== FSM 이벤트 처리 (Ch08 패턴 동일!) ===== */

void alarm_service_handle_event(alarm_event_t evt)
{
    alarm_state_t prev = s_state;

    switch (s_state) {

    /* ===== IDLE 상태: 알람 미설정 ===== */
    case ALARM_STATE_IDLE:
        if (evt == ALARM_EVT_SET_ALARM) {
            /* IDLE → ARMED: 알람 활성화 */
            rtc_set_alarm_a(&s_alarm_time, RTC_ALARMMASK_DATEWEEKDAY);
            s_state = ALARM_STATE_ARMED;
            LOG_I("알람 설정: %02d:%02d:%02d",
                  s_alarm_time.hours,
                  s_alarm_time.minutes,
                  s_alarm_time.seconds);
        }
        break;

    /* ===== ARMED 상태: 알람 대기 중 ===== */
    case ALARM_STATE_ARMED:
        if (evt == ALARM_EVT_ALARM_MATCH) {
            /* ARMED → TRIGGERED: 알람 발생! */
            s_trigger_tick = HAL_GetTick();
            action_start_alarm_output();
            s_state = ALARM_STATE_TRIGGERED;
        }
        else if (evt == ALARM_EVT_CANCEL) {
            /* ARMED → IDLE: 알람 취소 */
            rtc_deactivate_alarm_a();
            s_state = ALARM_STATE_IDLE;
            LOG_I("알람 취소됨");
        }
        break;

    /* ===== TRIGGERED 상태: 알람 울리는 중 ===== */
    case ALARM_STATE_TRIGGERED:
        if (evt == ALARM_EVT_ACK) {
            /* TRIGGERED → ACKNOWLEDGED: 사용자 확인 */
            action_stop_alarm_output();
            s_ack_tick = HAL_GetTick();
            s_state = ALARM_STATE_ACKNOWLEDGED;
            LOG_I("알람 확인 (ACK)");
        }
        else if (evt == ALARM_EVT_TIMEOUT) {
            /* TRIGGERED → ACKNOWLEDGED: 자동 타임아웃 */
            action_stop_alarm_output();
            s_ack_tick = HAL_GetTick();
            s_state = ALARM_STATE_ACKNOWLEDGED;
            LOG_W("알람 자동 타임아웃 (%d초)", ALARM_ACK_TIMEOUT_MS / 1000);
        }
        break;

    /* ===== ACKNOWLEDGED 상태: 확인 완료 ===== */
    case ALARM_STATE_ACKNOWLEDGED:
        if (evt == ALARM_EVT_SET_ALARM) {
            /* ACKNOWLEDGED → ARMED: 새 알람 재설정 */
            rtc_set_alarm_a(&s_alarm_time, RTC_ALARMMASK_DATEWEEKDAY);
            s_state = ALARM_STATE_ARMED;
            LOG_I("알람 재설정: %02d:%02d:%02d",
                  s_alarm_time.hours,
                  s_alarm_time.minutes,
                  s_alarm_time.seconds);
        }
        else if (evt == ALARM_EVT_TIMEOUT) {
            /* ACKNOWLEDGED → IDLE: 자동 복귀 */
            s_state = ALARM_STATE_IDLE;
            LOG_D("ACKNOWLEDGED → IDLE 자동 복귀");
        }
        break;
    }

    /* 상태 변경 로그 (Ch08 패턴 동일) */
    if (prev != s_state) {
        LOG_D("FSM 전이: %s → %s", STATE_NAMES[prev], STATE_NAMES[s_state]);
    }
}

/* ===== main loop 처리 ===== */

/**
 * @brief main loop에서 매 사이클 호출
 *
 * ISR 플래그 확인 + TRIGGERED 타임아웃 체크
 */
void alarm_service_process(void)
{
    /* ISR 플래그 → FSM 이벤트 변환 */
    if (s_alarm_flag) {
        s_alarm_flag = 0;
        alarm_service_handle_event(ALARM_EVT_ALARM_MATCH);
    }

    /* TRIGGERED 상태 타임아웃 체크 */
    if (s_state == ALARM_STATE_TRIGGERED) {
        if ((HAL_GetTick() - s_trigger_tick) > ALARM_ACK_TIMEOUT_MS) {
            alarm_service_handle_event(ALARM_EVT_TIMEOUT);
        }
    }

    /* ACKNOWLEDGED 상태 자동 복귀 */
    if (s_state == ALARM_STATE_ACKNOWLEDGED) {
        if ((HAL_GetTick() - s_ack_tick) > ALARM_IDLE_TIMEOUT_MS) {
            alarm_service_handle_event(ALARM_EVT_TIMEOUT);
        }
    }
}

/* ===== 상태 조회 ===== */

alarm_state_t alarm_service_get_state(void)
{
    return s_state;
}

/* ===== main.c 통합 예제 ===== */
/*
 * int main(void)
 * {
 *     HAL_Init();
 *     SystemClock_Config();
 *     MX_RTC_Init();
 *     MX_GPIO_Init();
 *
 *     rtc_driver_init();
 *     alarm_service_init();
 *     stopwatch_init();         // Ch08 — 유지!
 *
 *     // 알람 설정: 14시 30분 10초
 *     alarm_service_set_time(14, 30, 10);
 *     alarm_service_handle_event(ALARM_EVT_SET_ALARM);
 *
 *     while (1) {
 *         alarm_service_process();    // 알람 FSM
 *         // stopwatch_process();     // v1.0에서 모터 추가 시 통합
 *
 *         // 버튼 B1 → 알람 확인 (ACK)
 *         if (button_short_pressed) {
 *             if (alarm_service_get_state() == ALARM_STATE_TRIGGERED) {
 *                 alarm_service_handle_event(ALARM_EVT_ACK);
 *             }
 *         }
 *     }
 * }
 */
