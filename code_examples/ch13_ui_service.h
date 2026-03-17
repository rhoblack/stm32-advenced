/**
 * @file    ui_service.h
 * @brief   UI Service — 화면 3종 렌더링 + DISPLAY FSM (Ch13, v1.3)
 *
 * 레이어 위치: Service 레이어 (GFX Service 상위)
 * 의존성: gfx_service.h
 *
 * DISPLAY FSM 상태:
 *   TIME_VIEW → SENSOR_VIEW → STOPWATCH_VIEW → TIME_VIEW (순환)
 */

#ifndef UI_SERVICE_H
#define UI_SERVICE_H

#include <stdint.h>

/* ===== DISPLAY FSM 상태 정의 ===== */
typedef enum {
    UI_VIEW_TIME        = 0,   /* 디지털 시각 화면 (기본값) */
    UI_VIEW_SENSOR      = 1,   /* 온습도 센서 화면 */
    UI_VIEW_STOPWATCH   = 2,   /* 스톱워치 화면 */
    UI_VIEW_MAX
} UI_ViewType_t;

/* ===== FSM 이벤트 정의 ===== */
typedef enum {
    UI_EVENT_BTN_SHORT  = 0,   /* 버튼 짧게 누름: 화면 전환 */
    UI_EVENT_BTN_LONG   = 1,   /* 버튼 길게 누름: 강제 갱신 */
    UI_EVENT_TICK       = 2,   /* 1초 주기 타이머 이벤트 */
    UI_EVENT_MAX
} UI_EventType_t;

/* ===== 화면 데이터 구조체 ===== */

/** 시각 화면용 데이터 */
typedef struct {
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint8_t weekday;  /* 0=일, 1=월, ..., 6=토 */
} UI_TimeData_t;

/** 센서 화면용 데이터 */
typedef struct {
    int32_t temp_integer;   /* 정수부 (예: 23) */
    int32_t temp_decimal;   /* 소수부 1자리 (예: 4 → 23.4°C) */
    int32_t humi_integer;   /* 정수부 (예: 58) */
    int32_t humi_decimal;   /* 소수부 1자리 (예: 3 → 58.3%) */
    uint8_t alarm_active;   /* 온도 알람 활성 여부 */
} UI_SensorData_t;

/** 스톱워치 화면용 데이터 */
typedef struct {
    uint32_t elapsed_ms;        /* 총 경과 시간 (밀리초) */
    uint8_t  is_running;        /* 실행 중 여부 */
    uint8_t  lap_count;         /* 랩 기록 수 */
    uint32_t lap_times[5];      /* 최근 랩 기록 (ms) */
} UI_StopwatchData_t;

/* ===== UI Service API ===== */

/**
 * @brief UI Service 초기화 (GFX 초기화 포함)
 */
void UI_Init(void);

/**
 * @brief FSM 이벤트 처리 — 버튼/타이머 이벤트 입력
 * @param event: UI_EventType_t 이벤트
 */
void UI_HandleEvent(UI_EventType_t event);

/**
 * @brief 현재 활성 View 반환
 */
UI_ViewType_t UI_GetCurrentView(void);

/**
 * @brief 시각 데이터 업데이트 (RTC 콜백에서 호출)
 */
void UI_UpdateTimeData(const UI_TimeData_t *data);

/**
 * @brief 센서 데이터 업데이트 (Sensor Service에서 호출)
 */
void UI_UpdateSensorData(const UI_SensorData_t *data);

/**
 * @brief 스톱워치 데이터 업데이트 (Timer ISR에서 호출)
 */
void UI_UpdateStopwatchData(const UI_StopwatchData_t *data);

/**
 * @brief 현재 화면 갱신 요청 플래그 설정 (ISR-safe)
 * @note  메인 루프에서 UI_ProcessRefresh()를 호출하여 실제 갱신 수행
 */
void UI_RequestRefresh(void);

/**
 * @brief 갱신 요청 처리 (메인 루프에서 호출)
 *        refresh_requested 플래그가 설정된 경우에만 실제 렌더링 수행
 */
void UI_ProcessRefresh(void);

/* ===== 개별 화면 렌더링 함수 (UI Service 내부, 테스트용으로도 공개) ===== */

/**
 * @brief 시각 화면 렌더링 (초기 레이아웃 포함)
 */
void UI_Render_TimeView_Full(void);

/**
 * @brief 시각 화면 데이터 영역만 부분 갱신
 */
void UI_Render_TimeView_Partial(void);

/**
 * @brief 온습도 화면 전체 렌더링
 */
void UI_Render_SensorView_Full(void);

/**
 * @brief 온습도 데이터 영역 부분 갱신
 */
void UI_Render_SensorView_Partial(void);

/**
 * @brief 스톱워치 화면 전체 렌더링
 */
void UI_Render_StopwatchView_Full(void);

/**
 * @brief 스톱워치 경과 시간 부분 갱신
 */
void UI_Render_StopwatchView_Partial(void);

#endif /* UI_SERVICE_H */
