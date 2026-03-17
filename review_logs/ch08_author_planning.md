# Ch08. Advanced TIM 심화 — 기술 저자 기획안

**작성자:** 기술 저자 (Technical Author)
**작성일:** 2026-03-17
**Phase:** 1 (기획 회의)
**프로젝트 버전:** v0.7 → v0.8
**누적 기능:** Complementary PWM + 스톱워치 구현
**상태:** Phase 1 기획 완료

---

## 1. Ch07 선행 조건 분석

### Ch07에서 확립된 기반
- **tim_driver.h/c**: TIM2 기반 Driver 레이어 (init/start/stop/set_period_ms/pwm_start/pwm_stop/pwm_set_duty/register_callback)
- **PSC/ARR/CCR 마스터 공식**: T = (PSC+1) x (ARR+1) / f_clk
- **Edge-Aligned PWM Mode 1**: CNT < CCR → High, CNT >= CCR → Low
- **콜백 패턴**: `tim_callback_t` 함수 포인터 등록 → UEV 시 호출
- **핀 맵**: TIM2_CH1 = PA0(A0), TIM3 핀 맵 소개됨
- **F411 타이머 계층**: GP(TIM2~5) + Advanced(TIM1), Basic 없음
- **TIM1 예고**: Ch07 §7.1 표에서 "Complementary PWM + Dead-time" 언급, Ch07 연습문제 6(c)에서 "Dead-time 필수 시 TIM1" 힌트

### Ch08 진입 시 학습자 상태
- PSC/ARR/CCR 계산 능력 확보
- Edge-Aligned PWM 원리 이해
- tim_driver 패턴(HAL 캡슐화) 체화
- **미경험**: Complementary 출력, Dead-time, Input Capture, Output Compare, One-Pulse, Master-Slave, FSM 실전 구현

---

## 2. 절 단위 구성 (새 개념 3개 이하 규칙 준수)

### §8.0 도입 — "타이머의 상위 기어를 넣다"
- **핵심 메시지**: Ch07의 GP 타이머는 "기본 기어", Ch08의 Advanced TIM1은 "스포츠 기어"
- **감정 곡선**: 성취 확인(Ch07 완주) → 호기심(TIM1은 뭐가 다르지?)
- **새 개념**: 0개 (도입부)
- **분량**: ~800자

### §8.1 TIM1 Advanced Control Timer — GP와의 차이
- **핵심 메시지**: TIM1이 GP 타이머와 다른 3가지: Complementary 출력, Dead-time 삽입, Break 입력
- **비유**: 자동차 엔진의 "흡기-배기 밸브" — 두 밸브가 동시에 열리면 엔진 고장(=관통 전류)
- **새 개념 (3개)**:
  1. Complementary 출력 (CHxN) — 메인 채널의 반전 신호
  2. Dead-time — 두 출력이 동시에 High가 되지 않도록 삽입하는 지연
  3. Break 입력 — 비상 정지 (MOE 비트)
- **NUCLEO-F411RE TIM1 핀 배치 표**:
  - TIM1_CH1: PA8
  - TIM1_CH1N: PA7 (**주의: TIM3_CH2와 충돌!**)
  - TIM1_CH2N: PB0 (**주의: TIM3_CH3와 충돌!**)
  - TIM1_CH3N: PB1
  - TIM1_BKIN: PA6, PB12
- **분량**: ~3000자

### §8.2 Complementary PWM + Dead-time 실습
- **핵심 메시지**: CubeMX에서 TIM1 Complementary PWM 설정 → Dead-time 계산 → 파형 확인
- **새 개념 (2개)**:
  1. Dead-time 계산 공식: DTG 레지스터 → 실제 시간(ns) 변환
  2. HAL_TIMEx_PWMN_Start() — Complementary 채널 시작 API
- **코드**: CubeMX 설정 + HAL 코드 + 오실로스코프 기대 파형 설명
- **분량**: ~3500자

### §8.3 입력 캡처 (Input Capture) — 외부 신호의 주기 측정
- **핵심 메시지**: 외부 신호의 Rising Edge 시점을 캡처하여 주파수/주기를 측정
- **비유**: 스톱워치로 달리기 선수의 랩 타임 측정 — "출발선을 지날 때마다 시간을 기록"
- **새 개념 (3개)**:
  1. Input Capture 모드 — CCR에 캡처 시점의 CNT 값이 자동 저장
  2. HAL_TIM_IC_Start_IT() + HAL_TIM_IC_CaptureCallback()
  3. 주파수 계산: f = f_cnt / (capture2 - capture1)
- **실습**: 버튼(PC13) 누름 간격 측정 → UART 출력
- **분량**: ~3000자

### §8.4 출력 비교 (Output Compare) + 원샷 (One-Pulse) 모드
- **핵심 메시지**: 정확한 시점에 핀 상태를 바꾸는 OC, 한 번만 펄스를 내는 One-Pulse
- **비유**: 타이머 콘센트 — "30분 후에 한 번만 켜졌다 꺼져" (One-Pulse)
- **새 개념 (3개)**:
  1. Output Compare 모드 — Toggle/Set/Reset on match
  2. One-Pulse Mode (OPM) — 트리거 후 단일 펄스 생성
  3. HAL_TIM_OnePulse_Start() API
- **분량**: ~2500자

### §8.5 타이머 연계 (Master-Slave) — 타이머끼리 대화하기
- **핵심 메시지**: 한 타이머의 UEV가 다른 타이머의 트리거가 되는 연계 구조
- **비유**: 도미노 — "첫 번째 도미노(Master)가 쓰러지면 두 번째(Slave)가 자동 시작"
- **새 개념 (2개)**:
  1. Trigger Output (TRGO) — Master가 내보내는 신호
  2. Slave Mode Controller — ITR 입력으로 타이머 시작/리셋/게이팅
- **실습**: TIM2(Master, 1초 UEV) → TIM3(Slave, 게이팅) — TIM3 PWM이 1초 간격으로 켜졌다 꺼짐
- **분량**: ~2500자

### §8.6 스톱워치 FSM 구현 — Service 레이어 첫 등장
- **핵심 메시지**: STOPWATCH FSM을 Service 레이어에 구현, Driver(tim_driver)와 App(button) 사이를 중재
- **비유**: 실제 스톱워치의 두 버튼 — Start/Stop, Reset
- **새 개념 (3개)**:
  1. Service 레이어의 역할 — Driver 조합 + 비즈니스 로직
  2. FSM 테이블 기반 구현 (Ch03 FSM 기초의 실전 적용)
  3. stopwatch_service.h/c 설계
- **FSM 상태**:
  - IDLE: 초기 상태, 시간 = 00:00.000
  - RUNNING: 타이머 카운팅 중
  - PAUSED: 일시 정지 (현재 시간 유지)
  - (RESET은 IDLE로 전이하는 이벤트)
- **이벤트**: BTN_START_STOP, BTN_RESET
- **상태 전이**:
  - IDLE → RUNNING (BTN_START_STOP)
  - RUNNING → PAUSED (BTN_START_STOP)
  - PAUSED → RUNNING (BTN_START_STOP)
  - PAUSED → IDLE (BTN_RESET)
  - RUNNING → IDLE (BTN_RESET)
- **분량**: ~4000자

### §8.7 누적 통합 + 아키텍처 업데이트 (v0.8)
- **핵심 메시지**: tim_driver 확장 + stopwatch_service 추가 → v0.8 아키텍처 갱신
- **새 개념**: 0개 (통합)
- **v0.7 → v0.8 변경 요약 표**
- **분량**: ~2000자

---

## 3. HAL 코드 예제 목록 (6개)

### ch08_01_complementary_pwm.c
- **목표**: TIM1 CH1 + CH1N Complementary PWM 출력 + Dead-time 설정
- **핵심 구조**: CubeMX 설정값(PSC/ARR/DTG) → HAL_TIM_PWM_Start + HAL_TIMEx_PWMN_Start
- **핵심 코드 스니펫**:
```c
/* TIM1 CH1 + CH1N Complementary PWM 시작 */
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);       /* PA8: 메인 */
HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);    /* PA7: 반전 */
LOG_I("Complementary PWM 시작 (Dead-time=%lu ns)", dead_time_ns);
```

### ch08_02_input_capture.c
- **목표**: TIM2 Input Capture로 버튼 누름 간격(ms) 측정
- **핵심 구조**: HAL_TIM_IC_Start_IT → CaptureCallback에서 두 캡처 값 차이 계산
- **핵심 코드 스니펫**:
```c
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2 || htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1)
        return;
    uint32_t capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
    uint32_t diff = capture - s_last_capture;
    s_last_capture = capture;
    float period_ms = (float)diff / 10.0f;  /* 10kHz 카운터 기준 */
    LOG_I("캡처 간격: %.1f ms", period_ms);
}
```

### ch08_03_one_pulse.c
- **목표**: TIM1 One-Pulse 모드로 버튼 트리거 시 단일 펄스(예: 10ms) 생성
- **핵심 구조**: OPM 설정 → 외부 트리거 → 단일 펄스 출력 후 자동 정지
- **핵심 코드 스니펫**:
```c
/* One-Pulse 모드 시작 — 트리거 대기 */
HAL_TIM_OnePulse_Start(&htim1, TIM_CHANNEL_1);
LOG_I("One-Pulse 대기 중 (트리거 시 10ms 펄스 출력)");
```

### ch08_04_master_slave.c
- **목표**: TIM2(Master) UEV → TIM3(Slave) 게이팅 — PWM 간헐 출력
- **핵심 구조**: TIM2 TRGO=Update → TIM3 Slave Mode=Gated → TIM3 PWM 1초 간격 on/off
- **핵심 코드 스니펫**:
```c
/* Master: TIM2 TRGO = Update Event */
TIM_MasterConfigTypeDef sMasterConfig = {0};
sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_ENABLE;
HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig);
```

### ch08_05_stopwatch_service.c
- **목표**: STOPWATCH FSM Service 레이어 구현 (IDLE/RUNNING/PAUSED)
- **핵심 구조**: FSM 테이블(상태 x 이벤트 → 다음 상태 + 액션) + TIM 10ms tick + 시간 카운터
- **핵심 코드 스니펫**:
```c
typedef enum {
    SW_STATE_IDLE, SW_STATE_RUNNING, SW_STATE_PAUSED
} sw_state_t;

typedef enum {
    SW_EVT_START_STOP, SW_EVT_RESET, SW_EVT_TICK
} sw_event_t;

typedef struct {
    sw_state_t  state;
    sw_event_t  event;
    sw_state_t  next;
    void        (*action)(void);
} sw_transition_t;

static const sw_transition_t s_table[] = {
    { SW_STATE_IDLE,    SW_EVT_START_STOP, SW_STATE_RUNNING, action_start   },
    { SW_STATE_RUNNING, SW_EVT_START_STOP, SW_STATE_PAUSED,  action_pause   },
    { SW_STATE_RUNNING, SW_EVT_TICK,       SW_STATE_RUNNING, action_tick    },
    { SW_STATE_RUNNING, SW_EVT_RESET,      SW_STATE_IDLE,    action_reset   },
    { SW_STATE_PAUSED,  SW_EVT_START_STOP, SW_STATE_RUNNING, action_resume  },
    { SW_STATE_PAUSED,  SW_EVT_RESET,      SW_STATE_IDLE,    action_reset   },
};
```

### ch08_06_tim_driver_extended.c
- **목표**: tim_driver.h에 TIM1 Advanced 기능 확장 API 추가
- **핵심 구조**: 기존 tim_driver 인터페이스 유지 + Advanced 전용 함수 추가
- **핵심 코드 스니펫**:
```c
/* tim_driver.h 확장 — Ch08 추가 인터페이스 */

/* Complementary PWM (TIM1 전용) */
tim_status_t pwm_complementary_start(uint8_t channel);
tim_status_t pwm_complementary_stop(uint8_t channel);
tim_status_t pwm_set_deadtime_ns(uint32_t ns);

/* Input Capture */
typedef void (*tim_ic_callback_t)(uint32_t capture_value);
tim_status_t tim_ic_start(uint8_t channel);
tim_status_t tim_ic_stop(uint8_t channel);
void         tim_ic_register_callback(tim_ic_callback_t cb);

/* One-Pulse */
tim_status_t tim_one_pulse_start(uint8_t channel);
```

---

## 4. SVG 다이어그램 목록 (8개)

| # | 파일명 | 제목 | 범위 | 주요 색상 |
|---|--------|------|------|-----------|
| 1 | ch08_sec00_architecture.svg | v0.8 아키텍처 위치도 | 4계층, stopwatch_service 강조 | Service(#FBBC04) 강조 |
| 2 | ch08_sec00_fsm.svg | STOPWATCH FSM 상태 전이 다이어그램 | IDLE ↔ RUNNING ↔ PAUSED | 상태(#34A853), 전이(#1A73E8) |
| 3 | ch08_sec00_sequence.svg | Button → Service → Driver 시퀀스 | User → App → stopwatch_service → tim_driver → HW | 레이어별 색상 |
| 4 | ch08_sec01_tim1_vs_gp.svg | TIM1 vs GP Timer 기능 비교 | GP 기능 + Advanced 전용 기능 | GP(#34A853), Adv(#EA4335) |
| 5 | ch08_sec02_complementary_waveform.svg | Complementary PWM 파형 | CH1, CH1N, Dead-time 구간 | CH1(#1A73E8), CH1N(#EA4335), DT(#FBBC04) |
| 6 | ch08_sec03_input_capture.svg | Input Capture 동작 원리 | 외부 신호 + 카운터 + 캡처 포인트 | 신호(#EA4335), CNT(#34A853), 캡처(#1A73E8) |
| 7 | ch08_sec05_master_slave.svg | Master-Slave 타이머 연계 | TIM2 TRGO → TIM3 Gated | Master(#1A73E8), Slave(#34A853), TRGO(#FBBC04) |
| 8 | ch08_sec07_architecture_v08.svg | v0.8 아키텍처 최종 현황 | 4계층 전체, 신규 컴포넌트 강조 | 표준 4색 팔레트 |

---

## 5. 비유/실생활 예시

### Complementary PWM
- **1순위 — "흡기-배기 밸브"**: 자동차 엔진의 두 밸브가 번갈아 열림. 동시에 열리면 엔진 고장(=관통 전류). Dead-time은 두 밸브가 모두 닫혀있는 "안전 구간".
- **2순위 — "시소 놀이"**: 한쪽이 올라가면 반대쪽이 내려감. 시소가 수평인 순간 = Dead-time.
- **한계 명시**: 실제 Complementary PWM은 MOSFET H-Bridge 제어에 사용. 비유는 "반전 + 안전 구간" 개념만 전달.

### Stopwatch FSM
- **"실제 스톱워치"**: 디지털 스톱워치의 두 버튼(Start/Stop, Reset)이 FSM 이벤트에 정확히 대응.
- **한계 명시**: 실제 스톱워치는 SPLIT(랩 타임) 기능도 있지만 기본 3상태만 구현.

### Input Capture
- **"랩 타임 측정"**: 육상 경기에서 선수가 출발선을 지날 때마다 시간 기록.

### One-Pulse
- **"호텔 복도 조명"**: 버튼 한 번 누르면 30초 동안만 켜지고 자동 꺼짐.

### Master-Slave
- **"도미노 체인"**: 첫 번째 도미노(Master)가 쓰러지면 연결된 두 번째(Slave)가 자동 시작.

---

## 6. 아키텍처 섹션 기획

### 6.1 아키텍처 위치
```
┌─────────────────────────────────────────────┐
│ App Layer (#EA4335)                         │
│   main.c (버튼 이벤트 디스패치)              │
├─────────────────────────────────────────────┤
│ Service Layer (#FBBC04) ← Ch08 신규!        │
│   stopwatch_service.c (FSM + 시간 관리)      │
├─────────────────────────────────────────────┤
│ Driver Layer (#34A853)                      │
│   uart_driver │ logger │ tim_driver(확장)    │
├─────────────────────────────────────────────┤
│ HAL Layer (#1A73E8)                         │
│   GPIO │ UART │ DMA │ TIM(TIM1+TIM2+TIM3)  │
└─────────────────────────────────────────────┘
```

### 6.2 인터페이스 설계 — Ch07 인터페이스 확장

**기존 유지 (Ch07, 변경 없음)**:
```c
void         tim_driver_init(void);
tim_status_t tim_start(void);
tim_status_t tim_stop(void);
tim_status_t tim_set_period_ms(uint32_t period_ms);
tim_status_t pwm_start(void);
tim_status_t pwm_stop(void);
tim_status_t pwm_set_duty(uint8_t percent);
void         tim_register_callback(tim_callback_t cb);
```

**Ch08 확장 API (추가만, 기존 불변)**:
```c
/* === TIM1 Advanced 기능 === */

/* Complementary PWM (TIM1 전용) */
tim_status_t pwm_complementary_start(uint8_t channel);
tim_status_t pwm_complementary_stop(uint8_t channel);
tim_status_t pwm_set_deadtime_ns(uint32_t ns);

/* Input Capture */
typedef void (*tim_ic_callback_t)(uint32_t capture_value);
tim_status_t tim_ic_start(uint8_t channel);
tim_status_t tim_ic_stop(uint8_t channel);
void         tim_ic_register_callback(tim_ic_callback_t cb);

/* One-Pulse */
tim_status_t tim_one_pulse_start(uint8_t channel);
```

**stopwatch_service.h (신규 Service 레이어)**:
```c
typedef enum {
    SW_STATE_IDLE, SW_STATE_RUNNING, SW_STATE_PAUSED
} sw_state_t;

void        stopwatch_init(void);
void        stopwatch_start_stop(void);   /* 버튼 1: 시작/일시정지 토글 */
void        stopwatch_reset(void);        /* 버튼 2: 리셋 → IDLE */
sw_state_t  stopwatch_get_state(void);
uint32_t    stopwatch_get_time_ms(void);  /* 현재 경과 시간 (ms) */
```

### 6.3 FSM 설계 — STOPWATCH

**상태 전이 테이블**:

| 현재 상태 | 이벤트 | 다음 상태 | 액션 |
|-----------|--------|-----------|------|
| IDLE | BTN_START_STOP | RUNNING | tim_start(), elapsed=0 |
| RUNNING | BTN_START_STOP | PAUSED | tim_stop(), 시간 저장 |
| RUNNING | TICK | RUNNING | elapsed += 10ms |
| RUNNING | BTN_RESET | IDLE | tim_stop(), elapsed=0 |
| PAUSED | BTN_START_STOP | RUNNING | tim_start(), 이어서 카운트 |
| PAUSED | BTN_RESET | IDLE | elapsed=0 |

### 6.4 시퀀스 다이어그램 흐름

```
User → [BTN Press] → App(EXTI Callback) → stopwatch_service.start_stop()
  → FSM 전이(IDLE→RUNNING) → tim_driver.tim_start() → TIM HW Start
  → [10ms tick] → tim_driver UEV callback → stopwatch_service.tick()
  → elapsed_ms += 10 → LOG 출력
```

---

## 7. 감정 곡선 설계

```
§8.0  호기심     — "TIM1은 GP랑 뭐가 다르지?" (Ch07 완주 성취감)
§8.1  약간 불안  — "Complementary, Dead-time... 용어가 많다"
§8.2  이해       — "아, 반전 신호 + 안전 구간이구나" (파형 시각화로 해소)
§8.3  호기심     — "외부 신호를 측정할 수 있다고?" (Input Capture)
§8.4  이해       — "One-Pulse는 간단하네" (타이머 콘센트 비유)
§8.5  호기심     — "타이머끼리 대화?" (Master-Slave, 도미노)
§8.6  성취감     — "FSM으로 스톱워치를 만들었다!" (Service 레이어 첫 구현)
§8.7  자부심     — "v0.8 아키텍처에 Service 레이어가 생겼다"
```

---

## 8. NUCLEO-F411RE TIM1 핀 충돌 주의사항

| TIM1 채널 | 핀 | 충돌 | 대처 |
|-----------|-----|------|------|
| CH1 | PA8 | 없음 (안전) | 권장 사용 |
| CH1N | PA7 | **TIM3_CH2** | TIM3 CH2 미사용 시 OK |
| CH2N | PB0 | **TIM3_CH3** | TIM3 CH3 미사용 시 OK |
| CH3N | PB1 | 없음 | 사용 가능 |
| BKIN | PA6 | **TIM3_CH1** | PB12 대체 가능 |

**Ch08 실습 권장 핀 배치**: TIM1_CH1 = PA8 (메인), TIM1_CH1N = PA7 (Complementary, TIM3 미사용 전제)

---

## 9. Aside 박스 계획 (18~20개)

| 절 | 종류 | 주제 |
|----|------|------|
| §8.0 | instructor-tip | "Ch07 연습문제 6(c) 답이 오늘 내용" |
| §8.1 | tip | TIM1 APB2 클럭 = 84MHz (GP와 동일) |
| §8.1 | faq | "Complementary가 뭔가요? 왜 필요한가요?" |
| §8.1 | interview | "H-Bridge에서 Dead-time이 필요한 이유" |
| §8.1 | tip | NUCLEO-F411RE TIM1 핀 충돌 주의 |
| §8.2 | faq | "Dead-time을 얼마로 설정해야 하나요?" |
| §8.2 | instructor-tip | 오실로스코프 없이 확인하는 법 (LED 2개) |
| §8.2 | metacognition | Complementary 파형 그려보기 자가점검 |
| §8.3 | tip | Input Capture로 RPM 측정 실무 패턴 |
| §8.3 | faq | "캡처 값이 오버플로하면?" (32비트 TIM2 vs 16비트) |
| §8.3 | interview | "외부 주파수 측정 방법 2가지" |
| §8.4 | faq | "One-Pulse는 어디에 쓰나요?" |
| §8.4 | metacognition | OC vs PWM 차이 자가점검 |
| §8.5 | tip | Master-Slave로 ADC 자동 트리거 (실무) |
| §8.5 | faq | "ITR0~3은 어떤 타이머에 연결되나요?" |
| §8.6 | instructor-tip | FSM: 칠판에 상태도 먼저 → 코드 순서 |
| §8.6 | faq | "왜 Service 레이어가 필요한가요?" |
| §8.6 | interview | "FSM을 if-else vs 테이블로 구현하는 차이" |
| §8.7 | tip | v0.8 아키텍처 — Service 레이어의 의미 |
| §8.7 | metacognition | 전체 챕터 자가점검 (5문항) |

---

## 10. 연습문제 계획 (6문항)

1. **[기억]** TIM1의 Advanced 기능 3가지를 열거하고, GP 타이머와의 차이를 설명하시오.
2. **[이해]** Dead-time = 500ns일 때, 84MHz 클럭 기준 DTG 레지스터 값을 계산하시오.
3. **[적용]** Input Capture로 1kHz 외부 신호의 주파수를 측정하는 코드를 작성하시오.
4. **[적용]** STOPWATCH FSM의 상태 전이 테이블을 완성하고, `stopwatch_process()` 함수를 구현하시오.
5. **[분석]** if-else 기반 FSM과 테이블 기반 FSM의 장단점을 코드 가독성, 확장성, 메모리 관점에서 비교하시오.
6. **[평가]** 시나리오별 적합한 타이머 모드 선택: (a) 초음파 echo 측정 (b) 3상 BLDC 6-step (c) 인젝터 정밀 펄스

---

## 11. 집필 시 주의사항

1. **Ch07 인터페이스 불변**: 기존 tim_driver.h API 시그니처 변경 금지. 확장만 추가.
2. **Service 레이어 첫 등장**: Ch03 "맛보기" → §8.6에서 첫 실전 구현. 교육적 의미 강조.
3. **핀 충돌 표**: §8.1에서 반드시 NUCLEO-F411RE TIM1 핀 충돌 표 제시.
4. **FSM 상태도 먼저**: §8.6에서 코드 전에 SVG 상태 전이도 먼저 제시.
5. **DTG 레지스터**: 4구간 중 실습에 필요한 범위만 설명, CubeMX 활용 권장.
6. **코드 30줄 이하**: 모든 코드 블록 분할, 긴 함수는 "핵심 발췌" 표시.
7. **Ch09 예고**: 다음 챕터 RTC + Alarm FSM으로 자연스러운 연결.
