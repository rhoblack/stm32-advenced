# Ch17 기술 리뷰 — Technical Reviewer

> 작성일: 2026-03-17
> 검토 대상: manuscripts/part5/ch17.html + code_examples/ch17_*.c/.h
> 검토자: 기술 리뷰어 (STM32 HAL, NUCLEO-F411RE, C 임베디드 전문가)

---

## 종합 평가

| 항목 | 평가 |
|------|------|
| 코드 정확성 | ✅ 양호 |
| HAL API 사용 | ✅ 정확 |
| 실무 적합성 | ✅ 우수 |
| Critical 이슈 | 0건 |
| Major 이슈 | 1건 → 수정 필요 |
| Minor 이슈 | 3건 → 권고 |

---

## Critical 이슈 (0건)

없음.

---

## Major 이슈 (1건)

### 🟡 M1: ch17_hfsm.c — 부모 진출/진입 로직의 조건 처리

**위치**: `ch17_hfsm.c`, `hfsm_dispatch()` 함수 내 상태 전이 실행 블록

**문제**:
```c
/* 현재 코드 */
if (cur_s->parent != next_s->parent) {
    if (cur_s->parent && cur_s->parent->on_exit)  cur_s->parent->on_exit();
    if (next_s->parent && next_s->parent->on_entry) next_s->parent->on_entry();
}
```
같은 ACTIVE 부모를 공유하는 자식 간 전이(예: TIME_VIEW → SENSOR_VIEW)에서는
부모의 exit/entry가 호출되지 않는 것이 맞습니다.
그러나 IDLE → TIME_VIEW 전이 시, IDLE의 parent(NULL)와 TIME_VIEW의 parent(ACTIVE)가 달라서
`next_s->parent->on_entry()` 즉 `entry_active()`가 올바르게 호출됩니다.
이 부분은 코드가 올바르게 동작하나, 주석이 부족하여 동작 의도가 불명확합니다.

**권고 수정**: 조건에 설명 주석 추가 및 엣지 케이스(루트 상태) 명시.

---

## Minor 이슈 (3건)

### 🟢 m1: ch17_table_fsm.c — extern 함수 선언 방식

**위치**: `guard_alarm_active()` 내부의 `extern bool alarm_service_is_triggered(void);`

**문제**: 함수 내부에서 extern 선언하는 방식은 C99 표준상 허용되지만 실무에서는 헤더 파일을 include하는 것이 Best Practice입니다.

**권고**: `#include "alarm_service.h"` 헤더를 파일 상단에서 include하는 방식으로 변경.

### 🟢 m2: ch17_table_fsm.h — DISP_STATE_COUNT 사용 설명

**위치**: `display_state_t` enum의 `DISP_STATE_COUNT`

**문제**: 테이블 유효성 검사용이라고 주석이 있으나, 실제 검사 코드가 없습니다. 교재 내용과 불일치.

**권고**: `fsm_init()` 내부에 `assert(table_size <= (size_t)(DISP_STATE_COUNT * DISP_EVT_COUNT))` 추가 또는 주석 제거.

### 🟢 m3: ch17.html — 마스터 Pinout 테이블 핀 충돌 설명

**위치**: 6절 마스터 Pinout 테이블

**내용**: PA5가 SPI1_SCK와 LED LD2 양쪽에 표시되어 있어 충돌이 명확히 드러납니다.
이 충돌에 대한 경고(⚠)가 표에 있고 aside.tip에 설명이 있어 좋습니다.
다만 실제 프로젝트에서 이 충돌의 해결책(별도 핀 배치 예시)을 코드로 보여주면 더 실무적입니다.

---

## 기술 정확성 검증

### FSM 구현

- `fsm_run()` 내 `for` 루프 + `continue` 패턴: 올바른 구현 ✅
- `guard == NULL || guard()` 패턴: Short-circuit evaluation 올바른 활용 ✅
- `fsm_transition_t` 배열을 `const`로 선언: Flash 배치 최적화 ✅

### HFSM 구현

- `while (s != NULL)` 위임 루프: 부모가 없는 루트 상태(NULL)에서 안전하게 종료 ✅
- `on_entry` / `on_exit` NULL 체크: 크래시 방지 ✅
- 자식→부모 위임 시 `LOG_D` 호출: 디버그 추적 가능 ✅

### 코드 예제 컴파일 가능성

- `size_t` 사용: `<stddef.h>` include 필요 — 헤더에 포함됨 ✅
- `LOG_D` / `LOG_I` / `LOG_W` / `LOG_E`: Ch02 표준 준수 ✅
- HAL_StatusTypeDef 반환값 처리: 6절 코드 예제에서 적절히 처리 ✅

---

## 결론

기술적으로 견고한 챕터입니다. Major 이슈 1건은 코드 동작에 영향이 없으나 교육 명확성을 위해 주석 보강을 권고합니다. Critical 이슈 없음.

**기술 리뷰어 승인 여부**: 🟡 **조건부 승인** (M1 주석 보강 후 승인)
