/* ch08_05_stopwatch_service.c — Stopwatch Service 구현
 *
 * 레이어: Service
 * 의존: tim_driver.h (Driver 레이어)
 *
 * FSM 구현 패턴:
 *   - enum으로 상태 정의 (sw_state_t)
 *   - switch-case로 전이 처리
 *   - 이벤트(버튼) → FSM → 액션(타이머) 분리
 *   - ISR에서는 플래그만, 메인에서 FSM 처리
 *
 * 이 패턴은 Ch10(clock), Ch13(ui) 등에서 재사용됩니다.
 */

#include "stopwatch_service.h"
#include "tim_driver.h"
#include "log.h"

/* ===== 내부 상태 ===== */
static volatile sw_state_t s_state      = SW_STATE_IDLE;
static volatile uint32_t   s_elapsed_ms = 0;   /* 누적 경과 시간 */
static volatile uint32_t   s_tick_count = 0;    /* 현재 구간 틱 */

/* ===== 상태 이름 (로그용) ===== */
static const char * const STATE_NAMES[] = {
    "IDLE", "RUNNING", "PAUSED"
};

/* ===== 내부: 타이머 틱 콜백 (1ms 주기) ===== */

static void stopwatch_tick_callback(void)
{
    if (s_state == SW_STATE_RUNNING) {
        s_tick_count++;
    }
}

/* ===== 초기화 ===== */

void stopwatch_init(void)
{
    s_state      = SW_STATE_IDLE;
    s_elapsed_ms = 0;
    s_tick_count = 0;

    /* 1ms 주기 타이머 설정 */
    tim_set_period_ms(1);
    tim_register_callback(stopwatch_tick_callback);

    LOG_I("stopwatch_service 초기화 완료");
    LOG_D("  FSM 초기 상태: %s", STATE_NAMES[s_state]);
}

/* ===== FSM 이벤트 처리 ===== */

void stopwatch_handle_event(sw_event_t evt)
{
    sw_state_t prev = s_state;

    switch (s_state) {

    /* ===== IDLE 상태 ===== */
    case SW_STATE_IDLE:
        if (evt == SW_EVT_BTN_SHORT) {
            /* IDLE → RUNNING: 시작 */
            s_elapsed_ms = 0;
            s_tick_count = 0;
            tim_start();
            s_state = SW_STATE_RUNNING;
            LOG_I("스톱워치 시작!");
        }
        /* IDLE에서 BTN_LONG은 무시 (이미 리셋 상태) */
        break;

    /* ===== RUNNING 상태 ===== */
    case SW_STATE_RUNNING:
        if (evt == SW_EVT_BTN_SHORT) {
            /* RUNNING → PAUSED: 일시정지 */
            tim_stop();
            s_elapsed_ms += s_tick_count;
            s_tick_count = 0;
            s_state = SW_STATE_PAUSED;
            LOG_I("스톱워치 일시정지 (%lu ms)", s_elapsed_ms);
        }
        else if (evt == SW_EVT_BTN_LONG) {
            /* RUNNING → IDLE: 즉시 리셋 */
            tim_stop();
            s_elapsed_ms = 0;
            s_tick_count = 0;
            s_state = SW_STATE_IDLE;
            LOG_I("스톱워치 리셋 (RUNNING → IDLE)");
        }
        break;

    /* ===== PAUSED 상태 ===== */
    case SW_STATE_PAUSED:
        if (evt == SW_EVT_BTN_SHORT) {
            /* PAUSED → RUNNING: 재개 (누적 이어가기) */
            s_tick_count = 0;
            tim_start();
            s_state = SW_STATE_RUNNING;
            LOG_I("스톱워치 재개 (누적 %lu ms부터)", s_elapsed_ms);
        }
        else if (evt == SW_EVT_BTN_LONG) {
            /* PAUSED → IDLE: 리셋 */
            s_elapsed_ms = 0;
            s_tick_count = 0;
            s_state = SW_STATE_IDLE;
            LOG_I("스톱워치 리셋 (00:00.000)");
        }
        break;
    }

    /* 상태 변경 로그 */
    if (prev != s_state) {
        LOG_D("FSM 전이: %s → %s", STATE_NAMES[prev], STATE_NAMES[s_state]);
    }
}

/* ===== 시간 조회 ===== */

sw_state_t stopwatch_get_time(sw_time_t *time)
{
    uint32_t total = s_elapsed_ms + s_tick_count;

    if (time != NULL) {
        time->total_ms     = total;
        time->minutes      = (uint8_t)(total / 60000);
        time->seconds      = (uint8_t)((total % 60000) / 1000);
        time->milliseconds = (uint16_t)(total % 1000);
    }

    return s_state;
}

sw_state_t stopwatch_get_state(void)
{
    return s_state;
}
