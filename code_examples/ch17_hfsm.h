/**
 * @file    ch17_hfsm.h
 * @brief   계층적 FSM (HFSM — Hierarchical State Machine) 설계 예시
 * @chapter Ch17. 소프트웨어 아키텍처 심화 & 전체 통합
 *
 * HFSM 핵심 개념:
 *   - 부모 상태(Parent State): 자식 상태들의 공통 동작 처리
 *   - 이벤트 위임(Event Delegation): 자식이 처리 못하면 부모가 처리
 *   - 진입/진출 액션(Entry/Exit Action): 상태 전이 시 자동 실행
 *   - 초기 자식 상태(Initial Substate): 부모 진입 시 기본 자식 상태 지정
 */

#ifndef CH17_HFSM_H
#define CH17_HFSM_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * HFSM 상태 정의 (2계층)
 * ============================================================ */

/**
 * @brief  DISPLAY HFSM 상태 ID
 *
 * 계층 구조:
 *   ROOT
 *   ├── IDLE            (부모 없음 — 최상위)
 *   └── ACTIVE          (부모: ROOT)
 *       ├── TIME_VIEW   (부모: ACTIVE, 초기 자식)
 *       ├── SENSOR_VIEW (부모: ACTIVE)
 *       └── STOPWATCH_VIEW (부모: ACTIVE)
 */
typedef enum {
    HFSM_STATE_IDLE             = 0,
    HFSM_STATE_ACTIVE           = 1,  /* 부모 상태 */
    HFSM_STATE_TIME_VIEW        = 2,  /* ACTIVE의 자식 */
    HFSM_STATE_SENSOR_VIEW      = 3,  /* ACTIVE의 자식 */
    HFSM_STATE_STOPWATCH_VIEW   = 4,  /* ACTIVE의 자식 */
    HFSM_STATE_COUNT
} hfsm_state_id_t;

/** HFSM 이벤트 */
typedef enum {
    HFSM_EVT_BTN_ANY    = 0,  /**< 아무 버튼 */
    HFSM_EVT_BTN_SHORT  = 1,  /**< 짧은 버튼 */
    HFSM_EVT_BTN_LONG   = 2,  /**< 긴 버튼 */
    HFSM_EVT_TIMEOUT    = 3,  /**< 자동 타임아웃 */
    HFSM_EVT_COUNT
} hfsm_event_t;

/* ============================================================
 * HFSM 상태 구조체
 * ============================================================ */

/**
 * @brief  HFSM 상태 노드
 *
 * 각 상태는 다음을 포함:
 *   - parent: 부모 상태 포인터 (NULL = 최상위)
 *   - on_entry: 진입 액션
 *   - on_exit:  진출 액션
 *   - handle_event: 이벤트 처리 (처리 못하면 false 반환 → 부모 위임)
 */
typedef struct hfsm_state_t {
    hfsm_state_id_t         id;
    struct hfsm_state_t    *parent;           /**< 부모 상태 (NULL = root) */
    void                   (*on_entry)(void); /**< 진입 액션 */
    void                   (*on_exit)(void);  /**< 진출 액션 */
    /** 이벤트 처리: 처리했으면 true, 부모에게 위임하면 false */
    bool                   (*handle_event)(hfsm_event_t evt,
                                           hfsm_state_id_t *next_out);
    hfsm_state_id_t         initial_substate; /**< 부모 상태 초기 자식 */
} hfsm_state_t;

/* ============================================================
 * HFSM 인스턴스
 * ============================================================ */

typedef struct {
    hfsm_state_id_t  current;                  /**< 현재 최하위 상태 */
    hfsm_state_t    *state_table[HFSM_STATE_COUNT]; /**< 상태 노드 테이블 */
} hfsm_instance_t;

/* ============================================================
 * 공개 API
 * ============================================================ */

void            hfsm_init(hfsm_instance_t *hfsm);
void            hfsm_dispatch(hfsm_instance_t *hfsm, hfsm_event_t evt);
hfsm_state_id_t hfsm_current_state(const hfsm_instance_t *hfsm);

#endif /* CH17_HFSM_H */
