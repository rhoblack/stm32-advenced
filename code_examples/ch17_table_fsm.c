/**
 * @file    ch17_table_fsm.c
 * @brief   테이블 기반 FSM 엔진 구현 + DISPLAY FSM 전이 테이블
 * @chapter Ch17. 소프트웨어 아키텍처 심화 & 전체 통합
 *
 * 핵심 설계 원칙:
 *   - fsm_run() 엔진은 테이블을 순회하며 매칭 행을 실행
 *   - 새 상태 추가 = 전이 테이블에 행 추가만으로 완결 (OCP)
 *   - 엔진 코드(fsm_run)는 변경 없이 재사용 가능
 */

#include "ch17_table_fsm.h"
#include "log.h"          /* LOG_D / LOG_I / LOG_W / LOG_E */
#include "ui_service.h"   /* show_time_view() 등 */

/* ============================================================
 * DISPLAY FSM 액션 함수 전방 선언
 * ============================================================ */
static void action_show_time(void);
static void action_show_sensor(void);
static void action_show_stopwatch(void);
static void action_show_alarm(void);
static bool guard_alarm_active(void);

/* ============================================================
 * DISPLAY FSM 전이 테이블 (Table-Driven FSM 핵심)
 *
 * 구조: { 현재_상태, 이벤트, 조건함수, 액션함수, 다음_상태 }
 *
 * ※ 상태 추가 시 이 테이블에 행만 추가하면 됨 — 엔진(fsm_run) 수정 불필요
 * ============================================================ */
static const fsm_transition_t g_disp_table[] = {
    /* --- TIME_VIEW 상태에서의 전이 --- */
    { DISP_STATE_TIME_VIEW,       DISP_EVT_BTN_SHORT,     NULL,               action_show_sensor,     DISP_STATE_SENSOR_VIEW      },
    { DISP_STATE_TIME_VIEW,       DISP_EVT_BTN_LONG,      NULL,               action_show_stopwatch,  DISP_STATE_STOPWATCH_VIEW   },
    { DISP_STATE_TIME_VIEW,       DISP_EVT_ALARM_TRIGGER, guard_alarm_active, action_show_alarm,      DISP_STATE_ALARM_VIEW       },

    /* --- SENSOR_VIEW 상태에서의 전이 --- */
    { DISP_STATE_SENSOR_VIEW,     DISP_EVT_BTN_SHORT,     NULL,               action_show_stopwatch,  DISP_STATE_STOPWATCH_VIEW   },
    { DISP_STATE_SENSOR_VIEW,     DISP_EVT_BTN_LONG,      NULL,               action_show_time,       DISP_STATE_TIME_VIEW        },
    { DISP_STATE_SENSOR_VIEW,     DISP_EVT_ALARM_TRIGGER, guard_alarm_active, action_show_alarm,      DISP_STATE_ALARM_VIEW       },

    /* --- STOPWATCH_VIEW 상태에서의 전이 --- */
    { DISP_STATE_STOPWATCH_VIEW,  DISP_EVT_BTN_SHORT,     NULL,               action_show_time,       DISP_STATE_TIME_VIEW        },
    { DISP_STATE_STOPWATCH_VIEW,  DISP_EVT_BTN_LONG,      NULL,               action_show_sensor,     DISP_STATE_SENSOR_VIEW      },
    { DISP_STATE_STOPWATCH_VIEW,  DISP_EVT_ALARM_TRIGGER, guard_alarm_active, action_show_alarm,      DISP_STATE_ALARM_VIEW       },

    /* --- ALARM_VIEW 상태에서의 전이 (v2.0 신규 추가 — 엔진 변경 없음) --- */
    { DISP_STATE_ALARM_VIEW,      DISP_EVT_ALARM_ACK,     NULL,               action_show_time,       DISP_STATE_TIME_VIEW        },
    { DISP_STATE_ALARM_VIEW,      DISP_EVT_BTN_LONG,      NULL,               action_show_time,       DISP_STATE_TIME_VIEW        },
};

#define DISP_TABLE_SIZE  (sizeof(g_disp_table) / sizeof(g_disp_table[0]))

/* ============================================================
 * FSM 인스턴스 (전역)
 * ============================================================ */
static fsm_instance_t g_display_fsm;

/* ============================================================
 * FSM 엔진 구현
 * ============================================================ */

void fsm_init(fsm_instance_t *fsm,
              display_state_t init_state,
              const fsm_transition_t *table,
              size_t table_size)
{
    LOG_D("fsm_init: 초기 상태=%d, 테이블 크기=%u", (int)init_state, (unsigned)table_size);

    fsm->state      = init_state;
    fsm->table      = table;
    fsm->table_size = table_size;
    fsm->on_enter   = NULL;
    fsm->on_exit    = NULL;

    LOG_I("fsm_init: DISPLAY FSM 초기화 완료 — 상태=TIME_VIEW");
}

bool fsm_run(fsm_instance_t *fsm, display_event_t event)
{
    LOG_D("fsm_run: 현재=%d, 이벤트=%d", (int)fsm->state, (int)event);

    /* 전이 테이블 순회 */
    for (size_t i = 0; i < fsm->table_size; i++) {
        const fsm_transition_t *row = &fsm->table[i];

        /* 현재 상태 + 이벤트 매칭 확인 */
        if (row->current_state != fsm->state) continue;
        if (row->event         != event)       continue;

        /* 조건(guard) 검사 — NULL이면 무조건 통과 */
        if (row->guard != NULL && !row->guard()) {
            LOG_D("fsm_run: guard 불충족 — 전이 스킵 (행=%u)", (unsigned)i);
            continue;
        }

        /* 전이 실행 */
        display_state_t prev = fsm->state;

        /* 진출 콜백 */
        if (fsm->on_exit != NULL) {
            fsm->on_exit(prev);
        }

        /* 액션 실행 */
        if (row->action != NULL) {
            row->action();
        }

        /* 상태 전이 */
        fsm->state = row->next_state;

        /* 진입 콜백 */
        if (fsm->on_enter != NULL) {
            fsm->on_enter(fsm->state);
        }

        LOG_I("fsm_run: 전이 완료 %d -> %d (이벤트=%d)",
              (int)prev, (int)fsm->state, (int)event);
        return true;
    }

    /* 매칭 없음 — 현재 상태 유지 */
    LOG_D("fsm_run: 매칭 전이 없음 — 상태 유지 (%d)", (int)fsm->state);
    return false;
}

display_state_t fsm_get_state(const fsm_instance_t *fsm)
{
    return fsm->state;
}

/* ============================================================
 * DISPLAY FSM 외부 진입점
 * ============================================================ */

/**
 * @brief  DISPLAY FSM 초기화 (모듈 초기화 시 1회 호출)
 */
void display_fsm_init(void)
{
    fsm_init(&g_display_fsm,
             DISP_STATE_TIME_VIEW,
             g_disp_table,
             DISP_TABLE_SIZE);
}

/**
 * @brief  DISPLAY FSM 이벤트 전달
 * @param  event  발생한 이벤트
 */
void display_fsm_post_event(display_event_t event)
{
    fsm_run(&g_display_fsm, event);
}

/* ============================================================
 * 액션 함수 구현
 * ============================================================ */

static void action_show_time(void)
{
    LOG_D("action_show_time: TIME_VIEW 화면 전환");
    ui_show_time_view();
}

static void action_show_sensor(void)
{
    LOG_D("action_show_sensor: SENSOR_VIEW 화면 전환");
    ui_show_sensor_view();
}

static void action_show_stopwatch(void)
{
    LOG_D("action_show_stopwatch: STOPWATCH_VIEW 화면 전환");
    ui_show_stopwatch_view();
}

static void action_show_alarm(void)
{
    LOG_W("action_show_alarm: ALARM_VIEW 화면 전환 — 알람 발생!");
    ui_show_alarm_view();
}

/* ============================================================
 * Guard 함수 구현
 * ============================================================ */

static bool guard_alarm_active(void)
{
    /* 알람 서비스에서 활성 알람 여부 조회 */
    extern bool alarm_service_is_triggered(void);
    bool active = alarm_service_is_triggered();
    LOG_D("guard_alarm_active: %s", active ? "true" : "false");
    return active;
}
