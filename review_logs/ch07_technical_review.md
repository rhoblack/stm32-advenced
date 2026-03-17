# Ch07 TIM 기초 — 기술 리뷰 보고서

## 작성일: 2026-03-17
## 작성자: 기술 리뷰어 (Technical Reviewer)
## 대상: manuscripts/part2/chapter07.html + 코드 5개 + SVG 7개

---

## 총평

| 등급 | 개수 |
|------|------|
| 🔴 Critical | 1 |
| 🟡 Major | 3 |
| 🟢 Minor | 3 |

**전체 평가**: 조건부 승인 (Critical 1건 + Major 3건 수정 후 승인)

기술적 완성도가 높습니다. HAL API 사용법, PSC/ARR/CCR 공식, 타이머 계층 구조 모두 정확합니다. 코드 품질도 우수하고, uart_driver 패턴과의 일관성이 잘 유지되어 있습니다. 단, TIM1 클럭 표기와 tim_set_period_ms()의 EGR UG 부작용에 대한 수정이 필요합니다.

---

## 🔴 Critical Issues (1건)

### C-01. TIM1 APB2 타이머 클럭 "100MHz" 오류

**위치**: `chapter07.html` 라인 196, `ch07_sec01_timer_hierarchy.svg` 라인 47

**현재 상태**:
```
TIM1 | 16비트 | APB2 (100MHz)
```

**문제**: STM32F411RE의 CubeMX 기본 클럭 설정에서:
- SYSCLK = 100MHz (HSE 25MHz + PLL)일 때: APB2 prescaler = 1 → APB2 peripheral clock = 100MHz → **APB2 timer clock = 100MHz** (prescaler=1이면 x2 규칙 미적용)
- 그러나 NUCLEO-F411RE의 CubeMX 기본 설정은 **SYSCLK = 84MHz** (HSI 16MHz + PLL)가 더 일반적이며, 이 경우 APB2 = 84MHz.
- 교재 전체에서 "84MHz 기반"을 표준으로 사용하므로 (Ch04~06 모두 84MHz), TIM1이 갑자기 100MHz로 표기되면 학생에게 혼란을 줄 수 있음.

**수정안 (2가지 중 택1)**:
1. **(권장)** CubeMX 기본 설정이 SYSCLK=84MHz인 경우: TIM1 클럭도 APB2 = 84MHz로 통일
2. SYSCLK=100MHz 설정을 사용한다면: APB1 타이머 클럭도 84MHz가 아닌 값이 되므로 전체 교재 재검토 필요

**근거**: STM32F411RE Reference Manual (RM0383) Section 6.2 — APB2 prescaler=1일 때 timer clock = APB2 clock (x2 미적용). CubeMX에서 SYSCLK=84MHz이면 APB2=84MHz, SYSCLK=100MHz이면 APB2=100MHz. 교재 일관성을 위해 84MHz 통일 권장.

**심각도 근거**: 학생이 "왜 APB1은 84MHz인데 APB2는 100MHz인가?"라는 질문에 답하려면 PLL 설정 전체를 설명해야 함. Ch07 범위를 벗어남.

---

## 🟡 Major Issues (3건)

### M-01. tim_set_period_ms() — EGR UG 비트의 UIF 부작용 미처리

**위치**: `ch07_04_tim_driver.c` 라인 99, `chapter07.html` 라인 886, 948~951

**현재 상태**:
```c
htim2.Instance->EGR = TIM_EGR_UG;  /* 즉시 반영 */
```

**문제**: EGR UG 비트를 세트하면 소프트웨어 UEV가 발생하고, 이로 인해 **UIF(Update Interrupt Flag)도 세트**됩니다. 인터럽트가 활성화된 상태(HAL_TIM_Base_Start_IT 호출 후)에서 이 코드를 실행하면:
1. 의도하지 않은 콜백이 1회 추가 호출됨
2. LED 토글이 1회 예기치 않게 발생

**수정안**:
```c
/* 인터럽트 플래그 클리어 후 UG 세트 */
__HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
htim2.Instance->EGR = TIM_EGR_UG;
__HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
```
또는 더 안전한 방법:
```c
/* 인터럽트 비활성화 → UG → 플래그 클리어 → 인터럽트 재활성화 */
__HAL_TIM_DISABLE_IT(&htim2, TIM_IT_UPDATE);
htim2.Instance->EGR = TIM_EGR_UG;
__HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
__HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);
```

**근거**: RM0383 Section 13.4.6 — "UG 비트 세트 시 카운터 리셋 + UIF 세트". 본문 실무 팁(라인 948~951)에서 "인터럽트도 발생할 수 있으므로 필요시 비활성화"라고 언급했으나, **코드에는 반영되지 않음**. 학습 코드에서 경고만 하고 처리하지 않으면 혼란 유발.

### M-02. tim_set_period_ms() PSC 값 — 16비트 범위 초과 가능성

**위치**: `ch07_04_tim_driver.c` 라인 87

**현재 상태**:
```c
psc = (TIM2_CLK_HZ / 1000) - 1;  /* 83999 */
```

**문제**: PSC 값 83999는 16비트(65535) 범위를 초과합니다. TIM2는 32비트 카운터이지만, **PSC 레지스터는 모든 타이머에서 16비트**(0~65535)입니다. 83999는 PSC 레지스터에 들어가지 않습니다.

**실제 영향**: `__HAL_TIM_SET_PRESCALER` 매크로는 `htim->Instance->PSC = value`를 수행하는데, TIM2의 PSC는 16비트 레지스터이므로 83999 & 0xFFFF = 18463이 실제로 설정됩니다. 의도한 주기와 완전히 다른 동작.

**수정안**:
```c
if (period_ms >= 10) {
    /* 10ms 이상: PSC로 10kHz 카운터 */
    psc = (TIM2_CLK_HZ / 10000) - 1;  /* 8399 */
    arr = period_ms * 10 - 1;
} else {
    /* 1~9ms: PSC로 1MHz 카운터 */
    psc = (TIM2_CLK_HZ / 1000000) - 1; /* 83 */
    arr = period_ms * 1000 - 1;
}
```

**근거**: RM0383 Section 13.4.11 — TIMx_PSC 레지스터는 [15:0] 비트만 유효. 모든 STM32 타이머의 PSC는 16비트.

### M-03. ch07_01_tim_period_calc.c — PSC 타입 uint16_t인데 값이 정확

**위치**: `ch07_01_tim_period_calc.c` 라인 15~17

**현재 상태**:
```c
typedef struct {
    uint16_t psc;
    uint32_t arr;       /* TIM2는 32비트 */
    const char *desc;
} tim_calc_entry_t;
```

**문제**: PSC를 uint16_t로 올바르게 선언한 점은 좋습니다 (M-02와 일관성 체크). 그러나 tim_driver.c의 PSC 계산(83999)과 이 파일의 타입(uint16_t)이 모순됩니다. 학생이 두 파일을 비교하면 혼란.

**수정안**: M-02 수정 후 자연스럽게 해결됨. tim_driver.c의 PSC가 65535 이하로 변경되면 uint16_t와 일관.

---

## 🟢 Minor Issues (3건)

### m-01. PWM Duty 계산 표 — High 시간 분모 오류

**위치**: `chapter07.html` 라인 634~638 (§7.4 Duty 계산 표)

**현재 상태**:
```
25% → CCR = 250 → High 시간 250/10000 = 25us (1kHz 기준)
50% → CCR = 500 → High 시간 500/10000 = 50us
```

**문제**: ARR = 999이면 주기 = 1000 카운트. f_cnt = 1MHz (PSC=83). 주기 = 1000/1MHz = 1ms = 1000us.
- 25% High 시간 = 250us (맞음), 그러나 **분모가 10000이 아니라 1000000(1MHz)** 또는 **250/1000 × 1ms = 250us**로 표기해야 정확.
- 250/10000 = 0.025로, 단위가 맞지 않습니다. 실제 의미: 250카운트 / 1MHz = 250us.

**수정안**:
```
25% → CCR = 250 → High 시간 = 250 / 1MHz = 250us
50% → CCR = 500 → High 시간 = 500 / 1MHz = 500us
```

### m-02. 시퀀스 다이어그램 — 초기화 순서 불일치

**위치**: `ch07_sec00_sequence.svg` 라인 38~45

**현재 상태**: 시퀀스에서 `tim_driver_init()` 호출 직후 `HAL_TIM_Base_Start_IT()`가 표시됨.

**문제**: 실제 코드에서는 `tim_driver_init()`은 콜백만 NULL로 초기화하고, `HAL_TIM_Base_Start_IT()`는 별도의 `tim_start()` 호출에서 수행됩니다. 시퀀스 다이어그램이 실제 호출 순서와 다릅니다.

**수정안**: 초기화 단계를 두 단계로 분리:
1. `tim_driver_init()` → 내부 상태 초기화
2. `tim_start()` → `HAL_TIM_Base_Start_IT()`

### m-03. 다음 챕터 예고 — Ch08 내용 불일치

**위치**: `chapter07.html` 라인 1170~1174

**현재 상태**: "Ch08. Advanced TIM 심화에서는 TIM1의 Complementary PWM과 Dead-time, Input Capture와 Output Compare, 그리고 스톱워치 FSM을 구현"

**문제**: Ch08의 범위가 매우 넓습니다 (Advanced TIM + IC/OC + FSM). TABLE_OF_CONTENTS.md와의 정합성을 확인하세요. FSM이 Ch08인지 별도 챕터인지 검증 필요.

**수정안**: TABLE_OF_CONTENTS.md 확인 후 예고 내용 조정.

---

## 검증 통과 항목 (이슈 없음)

### HAL API 정확성 ✅
- `HAL_TIM_Base_Start_IT()` — 올바른 사용
- `HAL_TIM_PeriodElapsedCallback()` — weak 함수 올바르게 재정의
- `HAL_TIM_PWM_Start()` / `HAL_TIM_PWM_Stop()` — 올바른 채널 파라미터
- `__HAL_TIM_SET_COMPARE()` — 런타임 CCR 변경 정확
- `__HAL_TIM_GET_AUTORELOAD()` — ARR 읽기 정확

### PSC/ARR 계산 공식 ✅
- 마스터 공식 T = (PSC+1) × (ARR+1) / f_clk 정확
- 계산 표 5개 항목 모두 검증 통과 (1초, 500ms, 100ms, 10ms, 1ms)
- +1 보정 설명 명확

### F411RE 특수 사항 ✅
- Basic 타이머(TIM6/7) 없음 — 정확히 명시
- TIM2 32비트, TIM3/4 16비트 구분 — 정확
- TIM2_CH1 PA0 NUCLEO A0 — 정확 (AF01)
- PA0/PA5 핀 충돌 경고 — 적절

### 코드 품질 ✅
- LOG_D/I/W/E 적절 사용
- 에러 처리 enum 반환 일관적
- 콜백 함수 포인터 패턴 정확
- NVIC 우선순위 — CubeMX 기본값 사용 (문제 없음)
- 들여쓰기 4칸, 스네이크 케이스 준수

### 아키텍처 정합성 ✅
- tim_driver.h 인터페이스가 uart_driver.h 패턴과 일관
- Driver 레이어가 HAL 헤더를 완벽히 숨김 (tim_driver.h에는 stdint.h만)
- v0.7 아키텍처 다이어그램이 코드 구조와 일치
- 색상 팔레트 4색 일관 적용

### SVG 다이어그램 품질 ✅
- 7개 모두 Noto Sans KR, 14px 기준 준수
- 색상 팔레트 일관 (#1A73E8, #34A853, #FBBC04, #EA4335)
- PWM 파형 SVG에서 CCR 지점 명확 (50% 수평선)
- UEV 타임라인 정확 (Up-count → ARR 도달 → 리셋)

---

## 필수 / 권장 분류

### 필수 수정 (승인 전)
1. **C-01**: TIM1 클럭 100MHz → 84MHz 통일 (HTML + SVG)
2. **M-02**: tim_driver.c PSC 83999 → 8399 수정 (16비트 범위 초과)
3. **M-01**: EGR UG 후 UIF 클리어 코드 추가 (코드 + HTML)

### 권장 수정 (승인 후 가능)
4. **M-03**: ch07_01 타입 일관성 — M-02 수정 시 자동 해결
5. **m-01**: PWM Duty 표 분모 표기 수정
6. **m-02**: 시퀀스 다이어그램 초기화 순서 조정
7. **m-03**: Ch08 예고 내용 TABLE_OF_CONTENTS.md와 교차 검증

---

## 최종 판정

**조건부 승인 (Conditionally Approved)**

필수 수정 3건(C-01, M-02, M-01) 완료 후 승인합니다.

특히 M-02(PSC 16비트 초과)는 실제 하드웨어에서 잘못된 동작을 유발하므로 반드시 수정해야 합니다. C-01(TIM1 클럭)은 교재 일관성 차원에서 중요합니다.

전체적으로 기술적 정확성, 코드 품질, 아키텍처 일관성 모두 높은 수준입니다. tim_driver의 uart_driver 패턴 답습이 잘 되어 있으며, PSC/ARR 계산 설명이 매우 직관적입니다.
