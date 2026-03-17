# Ch09 기획서 — 내부 RTC 활용

## 기술 저자 Phase 1 기획 보고

---

## 1. Ch08 복습 + Ch09 선행 조건

### v0.8 현황 (Ch08 완료 시점)
- **Driver 레이어**: `tim_driver` (TIM2 GP + TIM1 CPWM + TIM3 IC)
- **Service 레이어**: `stopwatch_service` (IDLE/RUNNING/PAUSED FSM) — Service 레이어 첫 실체화
- **App 레이어**: Button EXTI → FSM 이벤트 디스패치, 500ms 주기 시간 표시
- **핵심 패턴 확립**: ISR에서 플래그만 세트 → 메인 루프에서 FSM 처리

### Ch08 → Ch09 연결 포인트
Ch08 마지막 절에서 이미 예고:
> "스톱워치는 **경과 시간**을 측정합니다. 하지만 '지금 몇 시 몇 분인가?'라는 **절대 시각**은 알 수 없습니다. 전원을 끄면 시간이 사라집니다."
> "Alarm FSM도 등장합니다 — Stopwatch FSM과 거의 같은 구조입니다!"

### v0.9에서 추가할 컴포넌트
| 레이어 | 파일 | 역할 |
|--------|------|------|
| Driver | `rtc_driver.c/h` | RTC 하드웨어 추상화 (시간 읽기/쓰기, 알람 설정) |
| Service | `alarm_service.c/h` | Alarm FSM (IDLE/ARMED/TRIGGERED/ACKNOWLEDGED) |
| App | main.c 확장 | RTC 초기화 + 알람 이벤트 처리 |

### 아키텍처 설계 원칙
- `rtc_driver`는 HAL_RTC_* 만 캡슐화 — 알람 판단 로직 넣지 않음
- `alarm_service`는 `rtc_driver` API만 호출 — HAL 직접 참조 금지
- `stopwatch_service`와 `alarm_service`는 독립 — 상호 참조 없음
- 기존 `tim_driver`, `stopwatch_service` API 변경 없음 (인터페이스 불변 원칙)

---

## 2. 절 구성 기획 (8개 절)

### §9.0 도입: "스톱워치의 한계 — 진짜 시계로의 도약"
- **감정 곡선**: 호기심 (v0.8 성취 확인 → "하지만 절대 시각은?")
- Ch08 Stopwatch FSM 성취 확인 → RTC 필요성 자연 도출
- "경과 시간 vs 절대 시각" 구분 — 이것이 RTC의 존재 이유
- 이 챕터 완료 시 기대 결과: 실시간 시각 표시 + 알람 동작
- 스스로 점검: Ch08 FSM 4요소 기억하는가? stopwatch_service 구조 설명 가능한가?

### §9.1 RTC 개요 — "전원이 꺼져도 살아있는 시계"
- **감정 곡선**: 호기심 → 약간의 불안 (새 개념 3개)
- **새 개념 (3개 한정)**:
  1. 백업 도메인 (Backup Domain) — VBAT 핀으로 독립 전원
  2. BCD (Binary-Coded Decimal) — 각 자릿수를 4비트로 표현
  3. LSE vs LSI 클럭 소스 — 정확도와 가용성의 트레이드오프
- **비유**: "벽시계" — 건물 정전에도 건전지로 계속 돌아감
- **NUCLEO-F411RE 특수 상황**: SB48/SB49 미장착 시 LSE 불가 → LSI 32kHz 대체 (±5% 편차)
- RTC 블록 다이어그램 (SVG): 백업 도메인 경계, VBAT, LSE/LSI 선택, Prescaler, Calendar
- BCD 레지스터 구조 (SVG): TR/DR 레지스터 비트 필드 분석
- **aside**: FAQ("LSI로 해도 되나요?" → 교육용은 OK, 제품은 LSE 필수), 면접 포인트(BCD vs Binary 변환)

### §9.2 CubeMX 설정 + 시간 읽기/쓰기
- **감정 곡선**: 불안 해소 → 이해 (CubeMX 따라하면 된다)
- CubeMX RCC → LSE/LSI 선택, RTC 활성화, 초기 시각 설정
- `HAL_RTC_SetTime()` / `HAL_RTC_SetDate()` — 초기값 설정
- `HAL_RTC_GetTime()` / `HAL_RTC_GetDate()` — 현재 시각 읽기
- **중요 주의**: `GetTime()` 후 반드시 `GetDate()` 호출해야 Shadow Register 언락됨
- 1초 주기 출력 실습 (UART로 "HH:MM:SS" 표시)
- **aside**: 실무 팁(GetTime/GetDate 순서), FAQ("왜 GetDate 안 부르면 시간이 멈추나요?")

### §9.3 rtc_driver 설계 — Driver 레이어 캡슐화
- **감정 곡선**: 이해 → 성취감 (Driver 패턴 재활용)
- `rtc_driver.h` 인터페이스 설계:
  ```c
  typedef struct {
      uint8_t hours;
      uint8_t minutes;
      uint8_t seconds;
  } rtc_time_t;

  typedef struct {
      uint8_t year;    /* 0~99 (2000+year) */
      uint8_t month;
      uint8_t date;
      uint8_t weekday;
  } rtc_date_t;

  typedef void (*rtc_alarm_callback_t)(void);

  rtc_status_t rtc_driver_init(void);
  rtc_status_t rtc_get_time(rtc_time_t *time);
  rtc_status_t rtc_get_date(rtc_date_t *date);
  rtc_status_t rtc_set_time(const rtc_time_t *time);
  rtc_status_t rtc_set_date(const rtc_date_t *date);
  rtc_status_t rtc_set_alarm(uint8_t hours, uint8_t minutes, uint8_t seconds);
  rtc_status_t rtc_clear_alarm(void);
  void         rtc_register_alarm_callback(rtc_alarm_callback_t cb);
  ```
- Ch07 tim_driver 패턴 반복: init → get/set → callback 등록
- `rtc_driver.c` 구현: HAL 호출 캡슐화, GetTime/GetDate 순서 보장, LOG 출력
- **aside**: 강사 꿀팁("tim_driver와 구조 비교표를 칠판에 그려주세요")

### §9.4 RTC 알람 — AlarmA/AlarmB 인터럽트
- **감정 곡선**: 호기심 (알람은 직관적) → 이해
- AlarmA vs AlarmB 차이: 마스크 설정으로 "매일 같은 시각" 또는 "특정 날짜" 선택 가능
- `HAL_RTC_SetAlarm_IT()` + `HAL_RTC_AlarmAEventCallback()`
- 알람 발생 시 인터럽트 → EXTI Line 17 → NVIC
- 알람 마스크(Mask) 개념: 어떤 필드를 비교할지 선택
- 알람 타이밍 시뮬레이션 (SVG): 시간 흐름 → 매칭 시점 → 인터럽트 발생
- **aside**: FAQ("AlarmA와 AlarmB 둘 다 쓸 수 있나요?" → 가능, Ch11에서 온도 알람에 AlarmB 사용 예고)

### §9.5 Wake-up 타이머 + 백업 레지스터
- **감정 곡선**: 이해 확장
- RTC Wake-up Timer: 주기적 이벤트 생성 (1초~18시간)
- Wake-up vs Alarm 비교 (SVG): 알람은 "절대 시각 매칭", Wake-up은 "주기적 트리거"
- 백업 레지스터 (BKP): 리셋 시에도 보존되는 20개 32비트 레지스터
  - 활용 예: 부팅 카운터, 마지막 설정 시각 저장, 리셋 원인 기록
- `HAL_RTCEx_SetWakeUpTimer_IT()` 사용법
- `HAL_RTCEx_BKUPWrite()` / `HAL_RTCEx_BKUPRead()` 사용법
- **aside**: 면접 포인트("백업 레지스터와 플래시 메모리의 차이?"), 실무 팁("BKP에 매직 넘버 저장 → 초기화 여부 판단")

### §9.6 Alarm FSM 설계 + 구현
- **감정 곡선**: 성취감 (Stopwatch FSM과 구조 동일!)
- Alarm FSM 상태 전이표:

  | 현재 상태 | 이벤트 | 다음 상태 | 액션 |
  |-----------|--------|-----------|------|
  | IDLE | SET_ALARM | ARMED | rtc_set_alarm() 호출 |
  | ARMED | ALARM_FIRED | TRIGGERED | 알림 시작 (LED 깜빡임) |
  | ARMED | CANCEL | IDLE | rtc_clear_alarm() |
  | TRIGGERED | ACK | IDLE | 알림 정지 |
  | TRIGGERED | TIMEOUT(30s) | IDLE | 자동 정지 (스누즈 미구현) |

- Alarm FSM 상태 전이도 (SVG)
- `alarm_service.h/c` 구현: Stopwatch FSM switch-case 패턴 재활용
- **핵심 교훈**: "FSM 패턴을 한 번 익히면, 새로운 Service에 바로 적용 가능"
- **aside**: 강사 꿀팁("Stopwatch FSM 코드를 복사 → 상태/이벤트만 교체 → 5분 안에 완성"), 스스로 점검("두 FSM의 구조적 유사성을 설명할 수 있는가?")

### §9.7 누적 통합 — v0.8에서 v0.9로
- **감정 곡선**: 자부심 (v0.9 = 마일스톤 ① 직전!)
- 코드 변경 요약 테이블
- v0.9 최종 아키텍처 (SVG): rtc_driver(Driver) + alarm_service(Service) 추가
- 아키텍처 원칙 확인 체크리스트
- Ch10 예고: "이제 시각을 알고 있으니, 스텝 모터로 시침을 돌릴 차례입니다 — v1.0 마일스톤!"
- **aside**: 실무 팁("Service → Driver → HAL 패턴 3번째 반복 — 이제 몸에 배었을 것")

---

## 3. 코드 예제 목록 (7개)

| 파일명 | 설명 | 줄 수(예상) |
|--------|------|------------|
| `ch09_01_rtc_basic_read.c` | RTC 시간/날짜 읽기 기본 예제 (GetTime→GetDate 순서) | ~25 |
| `ch09_02_rtc_set_time.c` | RTC 시간/날짜 설정 + UART 출력 | ~30 |
| `ch09_03_rtc_driver.h` | rtc_driver 공개 인터페이스 (타입 + 함수 선언) | ~30 |
| `ch09_03_rtc_driver.c` | rtc_driver 구현 (HAL 캡슐화) | ~80 |
| `ch09_04_alarm_interrupt.c` | AlarmA 설정 + 콜백 기본 예제 | ~30 |
| `ch09_05_alarm_service.h` | alarm_service 인터페이스 (상태/이벤트 enum + API) | ~25 |
| `ch09_05_alarm_service.c` | alarm_service FSM 구현 (switch-case) | ~70 |

**총 약 290줄** — 각 코드 블록 30줄 이하 규칙 준수 (분할 게재)

---

## 4. SVG 다이어그램 목록 (8개)

| 파일명 | 설명 | 배치 절 |
|--------|------|---------|
| `ch09_sec00_architecture.svg` | v0.8 → v0.9 아키텍처 비교 (rtc_driver + alarm_service 강조) | 아키텍처 위치 |
| `ch09_sec00_alarm_fsm.svg` | Alarm FSM 상태 전이 다이어그램 | 아키텍처 위치 |
| `ch09_sec01_rtc_block.svg` | RTC 블록 다이어그램 (백업 도메인, VBAT, LSE/LSI, Prescaler) | §9.1 |
| `ch09_sec01_bcd_register.svg` | BCD 레지스터 비트 필드 (TR/DR) | §9.1 |
| `ch09_sec01_lse_vs_lsi.svg` | LSE vs LSI 비교 (정확도, 가용성, 용도) | §9.1 |
| `ch09_sec04_alarm_timing.svg` | 알람 매칭 타이밍 시뮬레이션 | §9.4 |
| `ch09_sec05_wakeup_vs_alarm.svg` | Wake-up 타이머 vs 알람 비교 | §9.5 |
| `ch09_sec07_v09_final.svg` | v0.9 최종 아키텍처 | §9.7 |

**색상 규칙 준수**: HAL(#1A73E8), Driver(#34A853), Service(#FBBC04), App(#EA4335)

---

## 5. 비유/실생활 예시

| 대상 개념 | 비유 | 한계 명시 |
|-----------|------|-----------|
| RTC + 백업 도메인 | **벽시계** — 건물 정전에도 건전지(VBAT)로 계속 돌아감 | 실제 RTC는 디지털이며 BCD로 저장, 벽시계는 아날로그 |
| BCD 인코딩 | **게시판 숫자판** — 각 자릿수가 독립 카드로 교체 가능 (시:분 각각 2자리) | BCD는 4비트 단위, 게시판은 10진수 단위 |
| LSE vs LSI | **석영(쿼츠) 시계 vs 태엽 시계** — 석영이 훨씬 정확하지만, 태엽은 어디서든 작동 | LSI는 RC 발진기로 태엽보다는 정확하지만 LSE 대비 ±5% 편차 |
| Alarm FSM | **실제 알람 시계** — 설정 → 울림 → 확인 버튼 → 정지 | 스누즈는 미구현 (향후 확장 가능) |
| Wake-up 타이머 | **학교 종** — 매 시간마다 울리는 주기적 알림 | 학교 종은 고정 주기, Wake-up은 가변 주기 |
| 백업 레지스터 | **냉장고 메모판** — 리셋(정전)에도 사라지지 않는 메모 20장 | 백업 레지스터는 VBAT 제거 시 소실, 냉장고 메모는 영구 |

---

## 6. Aside 박스 계획 (최소 15개)

| 유형 | 위치 | 핵심 내용 |
|------|------|-----------|
| 💡 실무 팁 | §9.1 | NUCLEO-F411RE SB48/SB49 납땜 확인 — 미장착 시 LSI 사용 |
| 💡 실무 팁 | §9.2 | GetTime() 후 반드시 GetDate() 호출 — Shadow Register 언락 |
| 💡 실무 팁 | §9.5 | BKP에 매직 넘버 저장 → 초기화 판단 패턴 |
| 💡 실무 팁 | §9.7 | Service → Driver → HAL 패턴 3번째 반복 |
| ❓ FAQ | §9.1 | "LSI로 해도 되나요?" → 교육용 OK, 제품은 LSE |
| ❓ FAQ | §9.2 | "왜 GetDate 안 부르면 시간 멈추나요?" → Shadow Register 메커니즘 |
| ❓ FAQ | §9.4 | "AlarmA와 AlarmB 둘 다 쓸 수 있나요?" → Yes, Ch11에서 AlarmB 사용 예고 |
| ❓ FAQ | §9.5 | "Wake-up이랑 TIM 주기 인터럽트랑 뭐가 다르나요?" → 저전력 + 백업 도메인 |
| 🎯 면접 | §9.1 | BCD vs Binary 변환, RTC가 백업 도메인에 있는 이유 |
| 🎯 면접 | §9.4 | "RTC 알람 인터럽트 경로를 설명하시오" (EXTI Line 17) |
| 🎯 면접 | §9.5 | "백업 레지스터와 플래시 메모리의 차이?" |
| 📌 강사 꿀팁 | §9.0 | 핸드폰 알람 앱 시연 → FSM 상태 식별 유도 |
| 📌 강사 꿀팁 | §9.3 | tim_driver vs rtc_driver 구조 비교표 칠판에 그리기 |
| 📌 강사 꿀팁 | §9.6 | Stopwatch FSM 복사 → 상태/이벤트 교체 → 5분 완성 시연 |
| 🔍 스스로 점검 | §9.0 | Ch08 FSM 4요소 기억? stopwatch_service 구조 설명 가능? |
| 🔍 스스로 점검 | §9.6 | 두 FSM의 구조적 유사성, 왜 Service 레이어에 FSM을 넣는가? |
| 🔍 스스로 점검 | §9.7 | v0.9에서 추가된 파일 4개 나열, 각각의 레이어 구분 |

---

## 7. 감정 곡선 설계

```
§9.0  호기심     ── "스톱워치는 됐는데, 진짜 시계는?"
§9.1  약간의 불안 ── "백업 도메인? BCD? LSE? 새 개념이 3개나..."
§9.2  불안 해소   ── "CubeMX 따라하면 되는구나, HAL API도 간단하네"
§9.3  이해+성취   ── "tim_driver 패턴 그대로! 이건 할 수 있다"
§9.4  호기심      ── "알람도 설정할 수 있다고? 재밌겠다"
§9.5  이해 확장   ── "Wake-up은 주기적, 알람은 절대 시각, 백업 레지스터는 메모"
§9.6  성취감      ── "FSM 패턴 재활용! 5분 만에 새 Service 완성!"
§9.7  자부심      ── "v0.9 완성! 다음은 v1.0 마일스톤 — 진짜 시계!"
```

---

## 8. 학습 목표 (Bloom 분류)

1. **[기억/이해]** RTC의 백업 도메인, BCD 레지스터, LSE/LSI 클럭 소스 개념을 설명할 수 있다.
2. **[적용]** CubeMX에서 RTC를 설정하고, HAL API로 시간을 읽기/쓰기할 수 있다.
3. **[적용]** RTC 알람 인터럽트를 설정하고, 콜백으로 이벤트를 처리할 수 있다.
4. **[분석]** rtc_driver를 tim_driver 패턴과 비교하여 Driver 레이어 설계 원칙을 분석할 수 있다.
5. **[적용/분석]** Alarm FSM을 설계하고 Service 레이어에 구현하여, Stopwatch FSM과의 구조적 유사성을 설명할 수 있다.

---

## 9. 특별 주의사항

### NUCLEO-F411RE LSE 문제
- **기본 상태**: SB48, SB49 미납땜 → LSE 32.768kHz 크리스탈 미연결
- **해결 방안 2가지**:
  1. SB48/SB49 납땜 + 32.768kHz 크리스탈 장착 → LSE 사용 (정확도 ±20ppm)
  2. LSI 32kHz 사용 (납땜 불필요, 정확도 ±5% — 교육용으로 충분)
- **교재 접근**: 두 방법 모두 설명, LSI를 기본으로 실습 진행 (하드웨어 수정 부담 제거)

### HAL_RTC_GetTime/GetDate 순서 제약
- **STM32 RM0383**: "GetTime 후 GetDate를 호출해야 Shadow Register가 unlock됨"
- 이 순서를 지키지 않으면 다음 GetTime 호출 시 이전 값이 반환됨
- rtc_driver 내부에서 이 순서를 강제하여 사용자 실수 방지

### Alarm FSM vs Stopwatch FSM 병렬 구조
- 두 FSM이 독립 실행 — 알람이 울리는 중에도 스톱워치 동작 가능
- 메인 루프에서 두 Service를 순차 호출 (협력적 멀티태스킹)
- 이 패턴은 Ch10 clock_service, Ch13 ui_service에서 확장됨

---

## 10. 인터페이스 설계 확정

### rtc_driver.h
```c
/* rtc_driver.h — RTC Driver 공개 인터페이스 */

#ifndef RTC_DRIVER_H
#define RTC_DRIVER_H

#include <stdint.h>

typedef enum {
    RTC_OK    = 0,
    RTC_ERROR = 1,
    RTC_PARAM = 2
} rtc_status_t;

typedef struct {
    uint8_t hours;      /* 0~23 */
    uint8_t minutes;    /* 0~59 */
    uint8_t seconds;    /* 0~59 */
} rtc_time_t;

typedef struct {
    uint8_t year;       /* 0~99 (2000 + year) */
    uint8_t month;      /* 1~12 */
    uint8_t date;       /* 1~31 */
    uint8_t weekday;    /* 1=Mon ~ 7=Sun */
} rtc_date_t;

typedef void (*rtc_alarm_callback_t)(void);

rtc_status_t rtc_driver_init(void);
rtc_status_t rtc_get_time(rtc_time_t *time);
rtc_status_t rtc_get_date(rtc_date_t *date);
rtc_status_t rtc_set_time(const rtc_time_t *time);
rtc_status_t rtc_set_date(const rtc_date_t *date);
rtc_status_t rtc_set_alarm(uint8_t hours, uint8_t minutes, uint8_t seconds);
rtc_status_t rtc_clear_alarm(void);
void         rtc_register_alarm_callback(rtc_alarm_callback_t cb);

#endif /* RTC_DRIVER_H */
```

### alarm_service.h
```c
/* alarm_service.h — Alarm Service 공개 인터페이스 */

#ifndef ALARM_SERVICE_H
#define ALARM_SERVICE_H

#include <stdint.h>

typedef enum {
    ALARM_STATE_IDLE         = 0,
    ALARM_STATE_ARMED        = 1,
    ALARM_STATE_TRIGGERED    = 2,
    ALARM_STATE_ACKNOWLEDGED = 3
} alarm_state_t;

typedef enum {
    ALARM_EVT_SET     = 0,   /* 알람 시각 설정 */
    ALARM_EVT_CANCEL  = 1,   /* 알람 취소 */
    ALARM_EVT_FIRED   = 2,   /* RTC 알람 발생 (콜백에서) */
    ALARM_EVT_ACK     = 3,   /* 사용자 확인 (버튼) */
    ALARM_EVT_TIMEOUT = 4    /* 30초 무응답 자동 정지 */
} alarm_event_t;

void          alarm_service_init(void);
void          alarm_service_handle_event(alarm_event_t evt);
alarm_state_t alarm_service_get_state(void);
void          alarm_service_set_target(uint8_t hours, uint8_t minutes, uint8_t seconds);

#endif /* ALARM_SERVICE_H */
```

---

## 11. 연습문제 구성 (6문제)

1. **[기억]** RTC 백업 도메인의 3가지 구성 요소와 VBAT의 역할을 설명하시오.
2. **[이해]** BCD 인코딩에서 0x23이 십진수 몇을 의미하는지, 왜 RTC가 BCD를 사용하는지 설명하시오.
3. **[적용]** LSI 클럭 소스로 RTC를 설정하고, 매 초 UART에 "HH:MM:SS" 형식으로 출력하는 코드를 작성하시오.
4. **[적용]** AlarmA를 15:30:00에 발생하도록 설정하고, 콜백에서 LED를 토글하는 코드를 작성하시오.
5. **[분석]** Alarm FSM에서 TRIGGERED 상태가 30초 이상 지속될 때 자동으로 IDLE로 전이하는 타임아웃 로직을 구현하시오. HAL_GetTick()을 활용할 것.
6. **[평가]** Stopwatch FSM(Ch08)과 Alarm FSM(Ch09)의 상태 전이표를 비교하고, 공통 패턴을 추출하여 "범용 FSM 프레임워크"의 가능성을 논하시오. (힌트: Ch17 아키텍처 심화 예고)

---

**기획 완료. Phase 2(초안 집필) 준비 완료.**
