/**
 * @file    ui_service.c
 * @brief   UI Service — DISPLAY FSM + 화면 3종 렌더링 구현 (Ch13, v1.3)
 *
 * 구현 방식: 테이블 기반 FSM (Table-driven FSM)
 *
 * FSM 전이 테이블:
 *   현재 상태       | 이벤트         | 다음 상태        | 액션
 *   TIME_VIEW       | BTN_SHORT      | SENSOR_VIEW      | 화면 지우기 + SENSOR 레이아웃
 *   SENSOR_VIEW     | BTN_SHORT      | STOPWATCH_VIEW   | 화면 지우기 + STOPWATCH 레이아웃
 *   STOPWATCH_VIEW  | BTN_SHORT      | TIME_VIEW        | 화면 지우기 + TIME 레이아웃
 *   (모든 상태)     | BTN_LONG       | (동일 상태)      | 화면 강제 완전 갱신
 *   (모든 상태)     | TICK           | (동일 상태)      | 현재 화면 부분 갱신
 */

#include "ui_service.h"
#include "gfx_service.h"
#include "log.h"
#include <string.h>
#include <stdio.h>

/* ===== 화면 레이아웃 상수 — 좌표 집중 관리 ===== */

/* TIME_VIEW 좌표 */
#define TV_HEADER_Y      0U    /* 헤더 바 Y 시작 */
#define TV_HEADER_H      24U   /* 헤더 높이 */
#define TV_TIME_X        10U   /* 시각 표시 X */
#define TV_TIME_Y        89U   /* 시각 표시 Y */
#define TV_TIME_H        68U   /* 시각 영역 높이 */
#define TV_DATE_Y        177U  /* 날짜 표시 Y */
#define TV_DATE_H        44U   /* 날짜 영역 높이 */
#define TV_MINI_TEMP_Y   221U  /* 소형 온도 표시 Y */
#define TV_MINI_TEMP_H   44U   /* 소형 온도 영역 높이 */
#define TV_HINT_Y        265U  /* 버튼 힌트 Y */
#define TV_HINT_H        55U   /* 버튼 힌트 영역 높이 */

/* SENSOR_VIEW 좌표 */
#define SV_HEADER_Y      0U
#define SV_HEADER_H      24U
#define SV_TEMP_LABEL_Y  40U   /* 온도 레이블 */
#define SV_TEMP_VAL_Y    80U   /* 온도 값 (대형) */
#define SV_TEMP_H        100U  /* 온도 영역 높이 */
#define SV_DIVIDER_Y     189U  /* 구분선 */
#define SV_HUMI_LABEL_Y  205U  /* 습도 레이블 */
#define SV_HUMI_VAL_Y    240U  /* 습도 값 */
#define SV_HUMI_H        100U  /* 습도 영역 높이 */

/* STOPWATCH_VIEW 좌표 */
#define SW_HEADER_Y      0U
#define SW_HEADER_H      24U
#define SW_TIME_Y        60U   /* 경과 시간 표시 */
#define SW_TIME_H        80U   /* 경과 시간 영역 높이 */
#define SW_LAP_Y         160U  /* 랩 기록 표시 시작 */
#define SW_LAP_ROW_H     28U   /* 랩 행 높이 */
#define SW_MAX_LAP_ROWS  4U    /* 표시할 최대 랩 행 수 */

/* 색상 정의 */
#define TV_BG_COLOR      GFX_COLOR_BLACK
#define TV_TIME_COLOR    GFX_RGB(0, 229, 255)   /* 청록 (디지털 시계) */
#define TV_DATE_COLOR    GFX_COLOR_LIGHTGRAY
#define TV_MINI_COLOR    GFX_RGB(105, 240, 174) /* 연초록 */
#define SV_TEMP_COLOR    GFX_RGB(255, 110, 64)  /* 주황 (온도) */
#define SV_HUMI_COLOR    GFX_RGB(64, 196, 255)  /* 파랑 (습도) */
#define SV_LABEL_COLOR   GFX_COLOR_DARKGRAY
#define SW_TIME_COLOR    GFX_RGB(255, 215, 64)  /* 금색 (스톱워치) */
#define SW_LAP_COLOR     GFX_RGB(255, 204, 128) /* 연주황 (랩 기록) */
#define HEADER_BG        GFX_RGB(30, 30, 30)
#define HEADER_FG        GFX_COLOR_WHITE
#define ALARM_COLOR      GFX_COLOR_RED

/* ===== 내부 상태 변수 ===== */

static UI_ViewType_t s_current_view = UI_VIEW_TIME;  /* 현재 화면 상태 */
static volatile uint8_t s_refresh_requested = 0;     /* 갱신 요청 플래그 (ISR-safe) */

/* 화면 데이터 캐시 */
static UI_TimeData_t      s_time_data;
static UI_SensorData_t    s_sensor_data;
static UI_StopwatchData_t s_stopwatch_data;

/* ===== FSM 전이 테이블 ===== */

typedef void (*FSM_ActionFunc_t)(void);

typedef struct {
    UI_ViewType_t  current_state;
    UI_EventType_t event;
    UI_ViewType_t  next_state;
    FSM_ActionFunc_t action;
} FSM_Transition_t;

/* 액션 함수 전방 선언 */
static void action_enter_sensor_view(void);
static void action_enter_stopwatch_view(void);
static void action_enter_time_view(void);
static void action_force_refresh(void);
static void action_partial_refresh(void);

/* FSM 전이 테이블 정의 */
static const FSM_Transition_t s_fsm_table[] = {
    /* 현재 상태       | 이벤트              | 다음 상태           | 액션 */
    { UI_VIEW_TIME,      UI_EVENT_BTN_SHORT,  UI_VIEW_SENSOR,      action_enter_sensor_view    },
    { UI_VIEW_SENSOR,    UI_EVENT_BTN_SHORT,  UI_VIEW_STOPWATCH,   action_enter_stopwatch_view },
    { UI_VIEW_STOPWATCH, UI_EVENT_BTN_SHORT,  UI_VIEW_TIME,        action_enter_time_view      },
    { UI_VIEW_TIME,      UI_EVENT_BTN_LONG,   UI_VIEW_TIME,        action_force_refresh        },
    { UI_VIEW_SENSOR,    UI_EVENT_BTN_LONG,   UI_VIEW_SENSOR,      action_force_refresh        },
    { UI_VIEW_STOPWATCH, UI_EVENT_BTN_LONG,   UI_VIEW_STOPWATCH,   action_force_refresh        },
    { UI_VIEW_TIME,      UI_EVENT_TICK,       UI_VIEW_TIME,        action_partial_refresh      },
    { UI_VIEW_SENSOR,    UI_EVENT_TICK,       UI_VIEW_SENSOR,      action_partial_refresh      },
    { UI_VIEW_STOPWATCH, UI_EVENT_TICK,       UI_VIEW_STOPWATCH,   action_partial_refresh      },
};

#define FSM_TABLE_SIZE  (sizeof(s_fsm_table) / sizeof(s_fsm_table[0]))

/* ===== 내부 헬퍼 함수 ===== */

static void draw_header(const char *title)
{
    /* 헤더 배경 */
    GFX_FillRect(0, TV_HEADER_Y, LCD_WIDTH, TV_HEADER_H, HEADER_BG);
    /* 헤더 텍스트 (가운데 정렬) */
    uint16_t title_x = (LCD_WIDTH - (uint16_t)(strlen(title) * 8)) / 2U;
    GFX_DrawString(title_x, TV_HEADER_Y + 4U, title, HEADER_FG, HEADER_BG, 1);
}

static void draw_weekday_str(uint16_t x, uint16_t y, uint8_t weekday,
                              uint16_t fg, uint16_t bg)
{
    static const char * const weekday_str[] = {
        "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
    };
    if (weekday < 7U) {
        GFX_DrawString(x, y, weekday_str[weekday], fg, bg, 1);
    }
}

/* ===== FSM 액션 함수 구현 ===== */

static void action_enter_time_view(void)
{
    LOG_I("FSM: TIME_VIEW 진입");
    GFX_FillScreen(TV_BG_COLOR);
    draw_header("  SMART CLOCK  ");
    UI_Render_TimeView_Full();
}

static void action_enter_sensor_view(void)
{
    LOG_I("FSM: SENSOR_VIEW 진입");
    GFX_FillScreen(TV_BG_COLOR);
    draw_header(" SENSOR / ENV  ");
    UI_Render_SensorView_Full();
}

static void action_enter_stopwatch_view(void)
{
    LOG_I("FSM: STOPWATCH_VIEW 진입");
    GFX_FillScreen(TV_BG_COLOR);
    draw_header("  STOPWATCH    ");
    UI_Render_StopwatchView_Full();
}

static void action_force_refresh(void)
{
    LOG_D("FSM: 강제 완전 갱신 (BTN_LONG)");
    switch (s_current_view) {
    case UI_VIEW_TIME:        action_enter_time_view();       break;
    case UI_VIEW_SENSOR:      action_enter_sensor_view();     break;
    case UI_VIEW_STOPWATCH:   action_enter_stopwatch_view();  break;
    default:                  action_enter_time_view();       break;
    }
}

static void action_partial_refresh(void)
{
    switch (s_current_view) {
    case UI_VIEW_TIME:        UI_Render_TimeView_Partial();       break;
    case UI_VIEW_SENSOR:      UI_Render_SensorView_Partial();     break;
    case UI_VIEW_STOPWATCH:   UI_Render_StopwatchView_Partial();  break;
    default:                                                       break;
    }
}

/* ===== UI Service 공개 API ===== */

void UI_Init(void)
{
    LOG_I("UI_Init: UI Service 초기화 시작");

    /* GFX Service 초기화 (LCD Driver 포함) */
    GFX_Init();

    /* 내부 상태 초기화 */
    s_current_view = UI_VIEW_TIME;
    s_refresh_requested = 0U;
    memset(&s_time_data, 0, sizeof(s_time_data));
    memset(&s_sensor_data, 0, sizeof(s_sensor_data));
    memset(&s_stopwatch_data, 0, sizeof(s_stopwatch_data));

    /* 첫 화면: TIME_VIEW 진입 */
    action_enter_time_view();

    LOG_I("UI_Init: 완료 — TIME_VIEW 표시 중");
}

void UI_HandleEvent(UI_EventType_t event)
{
    uint8_t i;

    LOG_D("UI_HandleEvent: 현재=%d, 이벤트=%d", s_current_view, event);

    /* FSM 전이 테이블 탐색 */
    for (i = 0; i < FSM_TABLE_SIZE; i++) {
        if (s_fsm_table[i].current_state == s_current_view &&
            s_fsm_table[i].event         == event) {
            /* 전이 발견 */
            s_current_view = s_fsm_table[i].next_state;
            if (s_fsm_table[i].action != NULL) {
                s_fsm_table[i].action();
            }
            LOG_D("UI_HandleEvent: 전이 완료 → 상태=%d", s_current_view);
            return;
        }
    }

    LOG_W("UI_HandleEvent: 처리되지 않은 이벤트 (상태=%d, 이벤트=%d)", s_current_view, event);
}

UI_ViewType_t UI_GetCurrentView(void)
{
    return s_current_view;
}

void UI_UpdateTimeData(const UI_TimeData_t *data)
{
    if (data == NULL) return;
    /* 임계 구역 불필요: 구조체 복사는 메인 루프에서 수행 */
    memcpy(&s_time_data, data, sizeof(UI_TimeData_t));
    LOG_D("UI_UpdateTimeData: %02d:%02d:%02d",
          data->hour, data->min, data->sec);
}

void UI_UpdateSensorData(const UI_SensorData_t *data)
{
    if (data == NULL) return;
    memcpy(&s_sensor_data, data, sizeof(UI_SensorData_t));
    LOG_D("UI_UpdateSensorData: %ld.%ld°C %ld.%ld%%",
          (long)data->temp_integer, (long)data->temp_decimal,
          (long)data->humi_integer, (long)data->humi_decimal);
}

void UI_UpdateStopwatchData(const UI_StopwatchData_t *data)
{
    if (data == NULL) return;
    memcpy(&s_stopwatch_data, data, sizeof(UI_StopwatchData_t));
}

void UI_RequestRefresh(void)
{
    /* ISR에서 호출 가능한 원자적 플래그 설정 */
    s_refresh_requested = 1U;
}

void UI_ProcessRefresh(void)
{
    /* 메인 루프에서 폴링 */
    if (s_refresh_requested != 0U) {
        s_refresh_requested = 0U;
        action_partial_refresh();
    }
}

/* ===== 화면 렌더링 구현 ===== */

void UI_Render_TimeView_Full(void)
{
    char buf[24];

    LOG_D("UI_Render_TimeView_Full");

    /* 시:분:초 (대형, scale=3: 24×48px) */
    GFX_FillRect(0, TV_TIME_Y, LCD_WIDTH, TV_TIME_H, TV_BG_COLOR);
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             s_time_data.hour, s_time_data.min, s_time_data.sec);
    GFX_DrawString(TV_TIME_X, TV_TIME_Y + 10U, buf, TV_TIME_COLOR, TV_BG_COLOR, 3);

    /* 날짜 (scale=2) */
    GFX_FillRect(0, TV_DATE_Y, LCD_WIDTH, TV_DATE_H, TV_BG_COLOR);
    snprintf(buf, sizeof(buf), "%04d.%02d.%02d",
             s_time_data.year, s_time_data.month, s_time_data.day);
    GFX_DrawString(TV_TIME_X, TV_DATE_Y + 8U, buf, TV_DATE_COLOR, TV_BG_COLOR, 2);
    draw_weekday_str(LCD_WIDTH - 40U, TV_DATE_Y + 12U,
                     s_time_data.weekday, TV_DATE_COLOR, TV_BG_COLOR);

    /* 소형 온도 힌트 */
    GFX_FillRect(0, TV_MINI_TEMP_Y, LCD_WIDTH, TV_MINI_TEMP_H, GFX_RGB(0, 30, 10));
    GFX_DrawFloat1(TV_TIME_X, TV_MINI_TEMP_Y + 12U,
                   s_sensor_data.temp_integer, s_sensor_data.temp_decimal,
                   TV_MINI_COLOR, GFX_RGB(0, 30, 10), 2);
    GFX_DrawString(TV_TIME_X + 80U, TV_MINI_TEMP_Y + 14U, "C",
                   TV_MINI_COLOR, GFX_RGB(0, 30, 10), 1);

    /* 버튼 힌트 */
    GFX_FillRect(0, TV_HINT_Y, LCD_WIDTH, TV_HINT_H, GFX_RGB(10, 10, 50));
    GFX_DrawString(TV_TIME_X, TV_HINT_Y + 10U, "BTN: Next Screen",
                   GFX_COLOR_DARKGRAY, GFX_RGB(10, 10, 50), 1);
}

void UI_Render_TimeView_Partial(void)
{
    char buf[24];

    /* 시:분:초 영역만 갱신 */
    GFX_FillRect(0, TV_TIME_Y, LCD_WIDTH, TV_TIME_H, TV_BG_COLOR);
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             s_time_data.hour, s_time_data.min, s_time_data.sec);
    GFX_DrawString(TV_TIME_X, TV_TIME_Y + 10U, buf, TV_TIME_COLOR, TV_BG_COLOR, 3);
}

void UI_Render_SensorView_Full(void)
{
    char buf[20];
    uint16_t temp_bg = (s_sensor_data.alarm_active) ? GFX_RGB(50, 0, 0) : GFX_RGB(0, 10, 30);

    LOG_D("UI_Render_SensorView_Full");

    /* 온도 레이블 */
    GFX_FillRect(0, SV_HEADER_H, LCD_WIDTH, (uint16_t)(SV_TEMP_LABEL_Y - SV_HEADER_H), TV_BG_COLOR);
    GFX_DrawString(TV_TIME_X, SV_TEMP_LABEL_Y, "TEMPERATURE", SV_LABEL_COLOR, TV_BG_COLOR, 1);

    /* 온도 값 (대형) */
    GFX_FillRect(0, SV_TEMP_VAL_Y, LCD_WIDTH, SV_TEMP_H, temp_bg);
    GFX_DrawFloat1(TV_TIME_X, SV_TEMP_VAL_Y + 15U,
                   s_sensor_data.temp_integer, s_sensor_data.temp_decimal,
                   SV_TEMP_COLOR, temp_bg, 3);
    GFX_DrawString(TV_TIME_X + 120U, SV_TEMP_VAL_Y + 18U,
                   "C", SV_TEMP_COLOR, temp_bg, 2);

    /* 알람 표시 */
    if (s_sensor_data.alarm_active) {
        GFX_DrawString(TV_TIME_X, SV_TEMP_VAL_Y + 70U,
                       "! ALARM !", ALARM_COLOR, temp_bg, 2);
    }

    /* 구분선 */
    GFX_DrawHLine(20U, SV_DIVIDER_Y, LCD_WIDTH - 40U, GFX_COLOR_DARKGRAY);

    /* 습도 레이블 */
    GFX_DrawString(TV_TIME_X, SV_HUMI_LABEL_Y, "HUMIDITY", SV_LABEL_COLOR, TV_BG_COLOR, 1);

    /* 습도 값 (대형) */
    GFX_FillRect(0, SV_HUMI_VAL_Y, LCD_WIDTH, SV_HUMI_H, GFX_RGB(0, 10, 30));
    GFX_DrawFloat1(TV_TIME_X, SV_HUMI_VAL_Y + 15U,
                   s_sensor_data.humi_integer, s_sensor_data.humi_decimal,
                   SV_HUMI_COLOR, GFX_RGB(0, 10, 30), 3);
    GFX_DrawString(TV_TIME_X + 120U, SV_HUMI_VAL_Y + 18U,
                   "%", SV_HUMI_COLOR, GFX_RGB(0, 10, 30), 2);

    /* 업데이트 주기 안내 */
    snprintf(buf, sizeof(buf), "Upd: 2sec");
    GFX_DrawString(TV_TIME_X, 295U, buf, GFX_COLOR_DARKGRAY, TV_BG_COLOR, 1);
}

void UI_Render_SensorView_Partial(void)
{
    uint16_t temp_bg = (s_sensor_data.alarm_active) ? GFX_RGB(50, 0, 0) : GFX_RGB(0, 10, 30);

    /* 온도 데이터 영역만 갱신 */
    GFX_FillRect(0, SV_TEMP_VAL_Y, LCD_WIDTH, SV_TEMP_H, temp_bg);
    GFX_DrawFloat1(TV_TIME_X, SV_TEMP_VAL_Y + 15U,
                   s_sensor_data.temp_integer, s_sensor_data.temp_decimal,
                   SV_TEMP_COLOR, temp_bg, 3);

    /* 습도 데이터 영역만 갱신 */
    GFX_FillRect(0, SV_HUMI_VAL_Y, LCD_WIDTH, SV_HUMI_H, GFX_RGB(0, 10, 30));
    GFX_DrawFloat1(TV_TIME_X, SV_HUMI_VAL_Y + 15U,
                   s_sensor_data.humi_integer, s_sensor_data.humi_decimal,
                   SV_HUMI_COLOR, GFX_RGB(0, 10, 30), 3);
}

void UI_Render_StopwatchView_Full(void)
{
    LOG_D("UI_Render_StopwatchView_Full");
    UI_Render_StopwatchView_Partial();
}

void UI_Render_StopwatchView_Partial(void)
{
    char buf[24];
    uint32_t ms    = s_stopwatch_data.elapsed_ms;
    uint32_t min   = ms / 60000U;
    uint32_t sec   = (ms % 60000U) / 1000U;
    uint32_t csec  = (ms % 1000U) / 10U;  /* 센티초 */
    uint8_t  i;
    uint16_t sw_bg = GFX_RGB(20, 10, 0);

    /* 경과 시간 표시: MM:SS.cc */
    GFX_FillRect(0, SW_TIME_Y, LCD_WIDTH, SW_TIME_H, sw_bg);
    snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu",
             (unsigned long)min, (unsigned long)sec, (unsigned long)csec);
    GFX_DrawString(TV_TIME_X, SW_TIME_Y + 15U, buf, SW_TIME_COLOR, sw_bg, 3);

    /* 실행 상태 표시 */
    GFX_FillRect(0, SW_TIME_Y + SW_TIME_H, LCD_WIDTH, 20U, TV_BG_COLOR);
    if (s_stopwatch_data.is_running) {
        GFX_DrawString(TV_TIME_X, SW_TIME_Y + SW_TIME_H + 2U,
                       "RUNNING", GFX_COLOR_GREEN, TV_BG_COLOR, 1);
    } else {
        GFX_DrawString(TV_TIME_X, SW_TIME_Y + SW_TIME_H + 2U,
                       "PAUSED ", GFX_COLOR_YELLOW, TV_BG_COLOR, 1);
    }

    /* 랩 기록 표시 (최근 4개) */
    /* lap_count 상한 처리: UI_StopwatchData_t.lap_times 배열은 최대 5개 */
    uint8_t lap_count = (s_stopwatch_data.lap_count > 5U) ? 5U : s_stopwatch_data.lap_count;
    uint8_t start_lap = (lap_count > SW_MAX_LAP_ROWS) ? (uint8_t)(lap_count - SW_MAX_LAP_ROWS) : 0U;

    GFX_FillRect(0, SW_LAP_Y, LCD_WIDTH, SW_MAX_LAP_ROWS * SW_LAP_ROW_H + 4U, TV_BG_COLOR);

    for (i = start_lap; i < lap_count && i < 5U; i++) {
        uint32_t lap_ms  = s_stopwatch_data.lap_times[i];
        uint32_t lap_min = lap_ms / 60000U;
        uint32_t lap_sec = (lap_ms % 60000U) / 1000U;
        uint32_t lap_cs  = (lap_ms % 1000U) / 10U;
        uint16_t row_y   = SW_LAP_Y + (uint16_t)((i - start_lap) * SW_LAP_ROW_H);

        snprintf(buf, sizeof(buf), "Lap%d  %02lu:%02lu.%02lu",
                 i + 1, lap_min, lap_sec, lap_cs);
        GFX_DrawString(TV_TIME_X, row_y, buf, SW_LAP_COLOR, TV_BG_COLOR, 1);
    }
}
