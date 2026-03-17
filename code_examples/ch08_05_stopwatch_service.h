/* ch08_05_stopwatch_service.h — Stopwatch Service 공개 인터페이스
 *
 * 레이어: Service
 * 위치: 프로젝트 아키텍처 v0.8
 *
 * Service 레이어의 첫 구현입니다.
 * Driver(tim_driver)를 사용하여 FSM 기반 스톱워치를 구현합니다.
 *
 * 설계 원칙 (Ch03 복습):
 *   - Service = 비즈니스 로직 (FSM, 데이터 가공)
 *   - Driver  = 하드웨어 추상화 (타이머 제어)
 *   - Service는 Driver API만 호출, HAL 직접 접근 금지
 */

#ifndef STOPWATCH_SERVICE_H
#define STOPWATCH_SERVICE_H

#include <stdint.h>

/* ===== FSM 상태 ===== */
typedef enum {
    SW_STATE_IDLE    = 0,   /* 초기 상태, 00:00.000 */
    SW_STATE_RUNNING = 1,   /* 시간 측정 중 */
    SW_STATE_PAUSED  = 2    /* 일시 정지 */
} sw_state_t;

/* ===== 이벤트 ===== */
typedef enum {
    SW_EVT_BTN_SHORT = 0,   /* 버튼 짧게 누름 (<2초) */
    SW_EVT_BTN_LONG  = 1,   /* 버튼 길게 누름 (>=2초) */
    SW_EVT_TICK      = 2    /* 타이머 1ms 틱 (내부용) */
} sw_event_t;

/* ===== 시간 구조체 ===== */
typedef struct {
    uint32_t total_ms;       /* 총 경과 밀리초 */
    uint8_t  minutes;        /* 분 (0~59) */
    uint8_t  seconds;        /* 초 (0~59) */
    uint16_t milliseconds;   /* 밀리초 (0~999) */
} sw_time_t;

/* ===== 공개 API ===== */

/**
 * @brief 스톱워치 서비스 초기화
 * @note  tim_driver_init() 이후에 호출
 *        내부적으로 1ms 타이머 설정
 */
void stopwatch_init(void);

/**
 * @brief FSM 이벤트 처리
 * @param evt  발생한 이벤트 (BTN_SHORT / BTN_LONG)
 * @note  메인 루프에서 호출 (ISR 컨텍스트 금지)
 *
 * 상태 전이:
 *   IDLE    + BTN_SHORT → RUNNING (시작)
 *   RUNNING + BTN_SHORT → PAUSED  (일시정지)
 *   PAUSED  + BTN_SHORT → RUNNING (재개)
 *   PAUSED  + BTN_LONG  → IDLE    (리셋)
 *   RUNNING + BTN_LONG  → IDLE    (리셋)
 */
void stopwatch_handle_event(sw_event_t evt);

/**
 * @brief 현재 경과 시간 조회
 * @param[out] time  시간 구조체 (분:초.밀리초)
 * @return 현재 FSM 상태
 */
sw_state_t stopwatch_get_time(sw_time_t *time);

/**
 * @brief 현재 FSM 상태 조회
 * @return 현재 상태 (IDLE / RUNNING / PAUSED)
 */
sw_state_t stopwatch_get_state(void);

#endif /* STOPWATCH_SERVICE_H */
