/**
 * @file    ch17_hfsm.c
 * @brief   계층적 FSM (HFSM) 구현
 * @chapter Ch17. 소프트웨어 아키텍처 심화 & 전체 통합
 *
 * HFSM 이벤트 처리 흐름:
 *   1. 현재 자식 상태에서 handle_event() 호출
 *   2. 자식이 false 반환 → 부모 상태로 위임
 *   3. 부모가 처리 or 최상위 도달 시 종료
 *
 * 예시: STOPWATCH_VIEW 중 TIMEOUT 이벤트
 *   STOPWATCH_VIEW.handle_event(TIMEOUT) → false (처리 불가)
 *   → ACTIVE.handle_event(TIMEOUT)       → true  (IDLE로 전이)
 *   → ACTIVE.on_exit() 실행 → IDLE.on_entry() 실행
 */

#include "ch17_hfsm.h"
#include "log.h"
#include "ui_service.h"

/* ============================================================
 * 상태 진입/진출 액션 함수 선언
 * ============================================================ */
static void entry_idle(void);
static void exit_idle(void);
static void entry_active(void);       /* lcd_backlight_on() */
static void exit_active(void);        /* lcd_backlight_off() */
static void entry_time_view(void);
static void entry_sensor_view(void);
static void entry_stopwatch_view(void);
static void exit_stopwatch_view(void);

/* ============================================================
 * 이벤트 핸들러 함수 선언
 * ============================================================ */
static bool handle_idle(hfsm_event_t evt, hfsm_state_id_t *next);
static bool handle_active(hfsm_event_t evt, hfsm_state_id_t *next);
static bool handle_time_view(hfsm_event_t evt, hfsm_state_id_t *next);
static bool handle_sensor_view(hfsm_event_t evt, hfsm_state_id_t *next);
static bool handle_stopwatch_view(hfsm_event_t evt, hfsm_state_id_t *next);

/* ============================================================
 * 상태 노드 정의 (전역 정적)
 *
 * { id, parent, on_entry, on_exit, handle_event, initial_substate }
 * ============================================================ */
static hfsm_state_t s_idle = {
    HFSM_STATE_IDLE, NULL, entry_idle, exit_idle,
    handle_idle, HFSM_STATE_IDLE
};

static hfsm_state_t s_active = {
    HFSM_STATE_ACTIVE, NULL,  /* parent는 root (NULL) */
    entry_active, exit_active,
    handle_active,
    HFSM_STATE_TIME_VIEW   /* 초기 자식 상태 */
};

static hfsm_state_t s_time_view = {
    HFSM_STATE_TIME_VIEW, &s_active,  /* 부모: ACTIVE */
    entry_time_view, NULL,
    handle_time_view, HFSM_STATE_TIME_VIEW
};

static hfsm_state_t s_sensor_view = {
    HFSM_STATE_SENSOR_VIEW, &s_active,
    entry_sensor_view, NULL,
    handle_sensor_view, HFSM_STATE_SENSOR_VIEW
};

static hfsm_state_t s_stopwatch_view = {
    HFSM_STATE_STOPWATCH_VIEW, &s_active,
    entry_stopwatch_view, exit_stopwatch_view,
    handle_stopwatch_view, HFSM_STATE_STOPWATCH_VIEW
};

/* ============================================================
 * HFSM 초기화
 * ============================================================ */

void hfsm_init(hfsm_instance_t *hfsm)
{
    LOG_D("hfsm_init: DISPLAY HFSM 초기화");

    /* 상태 테이블 등록 */
    hfsm->state_table[HFSM_STATE_IDLE]           = &s_idle;
    hfsm->state_table[HFSM_STATE_ACTIVE]         = &s_active;
    hfsm->state_table[HFSM_STATE_TIME_VIEW]      = &s_time_view;
    hfsm->state_table[HFSM_STATE_SENSOR_VIEW]    = &s_sensor_view;
    hfsm->state_table[HFSM_STATE_STOPWATCH_VIEW] = &s_stopwatch_view;

    /* 초기 상태: IDLE → entry 실행 */
    hfsm->current = HFSM_STATE_IDLE;
    if (s_idle.on_entry) s_idle.on_entry();

    LOG_I("hfsm_init: 완료 — 현재 상태=IDLE");
}

/* ============================================================
 * HFSM 이벤트 디스패치 (핵심 알고리즘)
 * ============================================================ */

void hfsm_dispatch(hfsm_instance_t *hfsm, hfsm_event_t evt)
{
    LOG_D("hfsm_dispatch: 상태=%d, 이벤트=%d", (int)hfsm->current, (int)evt);

    hfsm_state_t    *s    = hfsm->state_table[hfsm->current];
    hfsm_state_id_t  next = hfsm->current;
    bool             handled = false;

    /*
     * 이벤트 위임 루프:
     *   현재 상태에서 시작하여 처리될 때까지 부모로 올라감
     */
    while (s != NULL) {
        if (s->handle_event != NULL) {
            handled = s->handle_event(evt, &next);
            if (handled) break;
        }
        /* 부모로 위임 */
        LOG_D("hfsm_dispatch: 상태=%d 처리 불가 → 부모 위임", (int)s->id);
        s = s->parent;
    }

    if (!handled) {
        LOG_D("hfsm_dispatch: 이벤트 %d 처리 안됨 — 무시", (int)evt);
        return;
    }

    if (next == hfsm->current) {
        /* 상태 변화 없음 (내부 전이) */
        return;
    }

    /* ---- 상태 전이 실행 ---- */
    hfsm_state_t *cur_s  = hfsm->state_table[hfsm->current];
    hfsm_state_t *next_s = hfsm->state_table[next];

    /* 진출 액션 (현재 상태) */
    if (cur_s->on_exit != NULL)  cur_s->on_exit();

    /* 현재 상태의 부모가 다음 상태의 부모와 다르면 부모 진출/진입도 실행 */
    if (cur_s->parent != next_s->parent) {
        if (cur_s->parent && cur_s->parent->on_exit)  cur_s->parent->on_exit();
        if (next_s->parent && next_s->parent->on_entry) next_s->parent->on_entry();
    }

    /* 진입 액션 (다음 상태) */
    if (next_s->on_entry != NULL) next_s->on_entry();

    LOG_I("hfsm_dispatch: 전이 %d → %d", (int)hfsm->current, (int)next);
    hfsm->current = next;
}

hfsm_state_id_t hfsm_current_state(const hfsm_instance_t *hfsm)
{
    return hfsm->current;
}

/* ============================================================
 * 이벤트 핸들러 구현
 * ============================================================ */

static bool handle_idle(hfsm_event_t evt, hfsm_state_id_t *next)
{
    if (evt == HFSM_EVT_BTN_ANY || evt == HFSM_EVT_BTN_SHORT || evt == HFSM_EVT_BTN_LONG) {
        /* IDLE → ACTIVE (초기 자식 = TIME_VIEW) */
        *next = HFSM_STATE_TIME_VIEW;
        return true;
    }
    return false;
}

static bool handle_active(hfsm_event_t evt, hfsm_state_id_t *next)
{
    /* TIMEOUT 이벤트는 부모(ACTIVE)가 공통 처리 — 모든 자식 상태에서 작동 */
    if (evt == HFSM_EVT_TIMEOUT) {
        LOG_W("handle_active: TIMEOUT → IDLE 전이 (백라이트 꺼짐)");
        *next = HFSM_STATE_IDLE;
        return true;
    }
    return false;
}

static bool handle_time_view(hfsm_event_t evt, hfsm_state_id_t *next)
{
    if (evt == HFSM_EVT_BTN_SHORT) { *next = HFSM_STATE_SENSOR_VIEW;    return true; }
    if (evt == HFSM_EVT_BTN_LONG)  { *next = HFSM_STATE_STOPWATCH_VIEW; return true; }
    return false;  /* 나머지 이벤트(TIMEOUT 등) → 부모(ACTIVE)가 처리 */
}

static bool handle_sensor_view(hfsm_event_t evt, hfsm_state_id_t *next)
{
    if (evt == HFSM_EVT_BTN_SHORT) { *next = HFSM_STATE_STOPWATCH_VIEW; return true; }
    if (evt == HFSM_EVT_BTN_LONG)  { *next = HFSM_STATE_TIME_VIEW;      return true; }
    return false;
}

static bool handle_stopwatch_view(hfsm_event_t evt, hfsm_state_id_t *next)
{
    if (evt == HFSM_EVT_BTN_SHORT) { *next = HFSM_STATE_TIME_VIEW;   return true; }
    if (evt == HFSM_EVT_BTN_LONG)  { *next = HFSM_STATE_SENSOR_VIEW; return true; }
    return false;  /* TIMEOUT → 부모(ACTIVE)가 처리 */
}

/* ============================================================
 * 진입/진출 액션 구현
 * ============================================================ */

static void entry_idle(void)
{
    LOG_D("entry_idle: 화면 꺼짐 상태 진입");
}

static void exit_idle(void)
{
    LOG_D("exit_idle: IDLE 진출");
}

static void entry_active(void)
{
    /* 부모 상태 진입 액션: 모든 자식 상태 진입 전 반드시 실행 */
    LOG_I("entry_active: LCD 백라이트 ON");
    ui_set_backlight(true);  /* HAL_GPIO_WritePin(LCD_BL_GPIO...) */
}

static void exit_active(void)
{
    /* 부모 상태 진출 액션: 모든 자식 상태 진출 후 반드시 실행 */
    LOG_I("exit_active: LCD 백라이트 OFF");
    ui_set_backlight(false);
}

static void entry_time_view(void)
{
    LOG_D("entry_time_view: 시각 화면 표시");
    ui_show_time_view();
}

static void entry_sensor_view(void)
{
    LOG_D("entry_sensor_view: 온습도 화면 표시");
    ui_show_sensor_view();
}

static void entry_stopwatch_view(void)
{
    LOG_D("entry_stopwatch_view: 스톱워치 화면 표시");
    ui_show_stopwatch_view();
}

static void exit_stopwatch_view(void)
{
    /* 스톱워치 타이머 정지 */
    LOG_D("exit_stopwatch_view: 스톱워치 타이머 정지");
    extern void stopwatch_pause(void);
    stopwatch_pause();
}
