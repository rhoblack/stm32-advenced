/**
 * @file    ch17_table_fsm.h
 * @brief   테이블 기반 FSM (Table-Driven FSM) 엔진 — 범용 구현
 * @chapter Ch17. 소프트웨어 아키텍처 심화 & 전체 통합
 *
 * 사용법:
 *   1. fsm_state_t / fsm_event_t 를 enum으로 정의
 *   2. fsm_transition_t 배열로 전이 테이블 작성
 *   3. fsm_run() 호출로 상태 전이 실행
 */

#ifndef CH17_TABLE_FSM_H
#define CH17_TABLE_FSM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  /* size_t */

/* ============================================================
 * 디스플레이 FSM 상태 / 이벤트 정의
 * ============================================================ */

/** 디스플레이 FSM 상태 (Display FSM State) */
typedef enum {
    DISP_STATE_TIME_VIEW        = 0,  /**< 시각 화면 */
    DISP_STATE_SENSOR_VIEW      = 1,  /**< 온습도 화면 */
    DISP_STATE_STOPWATCH_VIEW   = 2,  /**< 스톱워치 화면 */
    DISP_STATE_ALARM_VIEW       = 3,  /**< 알람 화면 (v2.0 신규) */
    DISP_STATE_COUNT                  /**< 상태 개수 (테이블 유효성 검사용) */
} display_state_t;

/** 디스플레이 FSM 이벤트 (Display FSM Event) */
typedef enum {
    DISP_EVT_BTN_SHORT      = 0,  /**< 짧은 버튼 눌림 */
    DISP_EVT_BTN_LONG       = 1,  /**< 긴 버튼 눌림 (2초 이상) */
    DISP_EVT_ALARM_TRIGGER  = 2,  /**< 알람 트리거 */
    DISP_EVT_ALARM_ACK      = 3,  /**< 알람 해제 */
    DISP_EVT_TIMEOUT        = 4,  /**< 화면 타임아웃 */
    DISP_EVT_COUNT                /**< 이벤트 개수 */
} display_event_t;

/* ============================================================
 * 테이블 기반 FSM 엔진 구조체
 * ============================================================ */

/**
 * @brief  FSM 전이 테이블 한 행(Row)
 *
 *   | current_state | event | guard    | action       | next_state      |
 *   |---------------|-------|----------|--------------|-----------------|
 *   | TIME_VIEW     | SHORT | NULL     | show_sensor  | SENSOR_VIEW     |
 *   | TIME_VIEW     | LONG  | NULL     | show_stop    | STOPWATCH_VIEW  |
 *   | ...           | ...   | ...      | ...          | ...             |
 *
 * guard가 NULL이면 조건 없이 전이.
 * guard가 non-NULL이면 guard() == true일 때만 전이.
 */
typedef struct {
    display_state_t  current_state;        /**< 현재 상태 */
    display_event_t  event;                /**< 이벤트 */
    bool             (*guard)(void);       /**< 조건 함수 (NULL = 무조건) */
    void             (*action)(void);      /**< 전이 액션 함수 */
    display_state_t  next_state;           /**< 다음 상태 */
} fsm_transition_t;

/* ============================================================
 * FSM 인스턴스 구조체
 * ============================================================ */

/**
 * @brief  FSM 인스턴스 — 테이블 포인터 + 현재 상태 보유
 */
typedef struct {
    display_state_t         state;         /**< 현재 상태 */
    const fsm_transition_t *table;         /**< 전이 테이블 포인터 */
    size_t                  table_size;    /**< 전이 테이블 행 수 */
    void (*on_enter)(display_state_t s);   /**< 상태 진입 콜백 (선택) */
    void (*on_exit)(display_state_t s);    /**< 상태 진출 콜백 (선택) */
} fsm_instance_t;

/* ============================================================
 * 공개 API
 * ============================================================ */

/**
 * @brief  FSM 초기화
 * @param  fsm         FSM 인스턴스 포인터
 * @param  init_state  초기 상태
 * @param  table       전이 테이블 배열
 * @param  table_size  전이 테이블 행 수
 */
void fsm_init(fsm_instance_t *fsm,
              display_state_t init_state,
              const fsm_transition_t *table,
              size_t table_size);

/**
 * @brief  FSM 이벤트 처리 (전이 실행)
 * @param  fsm    FSM 인스턴스 포인터
 * @param  event  발생한 이벤트
 * @return true   전이가 발생한 경우
 * @return false  매칭 전이 없음 (상태 유지)
 */
bool fsm_run(fsm_instance_t *fsm, display_event_t event);

/**
 * @brief  현재 상태 조회
 * @param  fsm  FSM 인스턴스 포인터
 * @return 현재 상태 값
 */
display_state_t fsm_get_state(const fsm_instance_t *fsm);

#endif /* CH17_TABLE_FSM_H */
