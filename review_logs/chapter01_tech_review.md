# Chapter 01 기술 리뷰 보고서

**리뷰어**: 기술 리뷰어 (STM32 HAL / NUCLEO-F411RE / C 임베디드 실무 전문가)
**리뷰 대상**: Ch01. GPIO & EXTI — LED, Button
**리뷰 일자**: 2026-03-16
**리뷰 파일**:
- `manuscripts/part0/chapter01.html`
- `code_examples/ch01_ex01_led_blink.c`
- `code_examples/ch01_ex02_pushpull_vs_opendrain.c`
- `code_examples/ch01_ex03_button_polling.c`
- `code_examples/ch01_ex04_button_exti.c`
- `code_examples/ch01_ex05_button_exti_debounce.c`

---

## 요약

| 등급 | 건수 |
|------|------|
| 🔴 Critical | 2건 |
| 🟡 Major | 4건 |
| 🟢 Minor | 5건 |

**총평**: 전반적으로 HAL API 사용이 정확하고 NUCLEO-F411RE 실동작 패턴에 부합합니다. Critical 이슈 2건은 원고 본문의 기술 설명 오류로, 독자가 잘못된 이해를 형성할 수 있어 반드시 수정이 필요합니다.

---

## 🔴 Critical 이슈

### Critical-01: NVIC SysTick 우선순위 설명 오류 — 원고 §1.5 및 ch01_ex05

**위치**:
- `chapter01.html` §1.5 FAQ 박스 (line ~778-782)
- `chapter01.html` §1.4 면접 포인트 박스 (line ~705-709)
- `ch01_ex05_button_exti_debounce.c` 주석 (line 84-85)

**문제**:

원고 §1.5 FAQ에 다음 문장이 있습니다:

> "만약 현재 ISR의 우선순위가 15 이하(숫자 ≥ 15)라면 SysTick이 선점하지 못해..."

그리고 §1.4 면접 포인트 박스에:

> "HAL 내부 함수(SysTick, HAL_Delay)는 우선순위 15(최저)를 사용"

ex05 주석(line 84-85):

> "(SysTick = Priority 0, EXTI = Priority 1 이상 권장)"

**오류 내용**:

STM32 HAL에서 SysTick의 우선순위는 **0이 아닙니다**. HAL 초기화(`HAL_Init()`) 내부에서 SysTick은 `TICK_INT_PRIORITY`로 설정되며, 이 기본값은 **0x0FU (15)** 입니다(`stm32f4xx_hal.h` 참조). 즉 SysTick은 NVIC_PRIORITYGROUP_4 기준으로 PreemptPriority **15(최저)** 로 설정됩니다.

원고 §1.5 FAQ 설명 "우선순위가 15 이하(숫자 ≥ 15)"라는 조건 서술은 맞지만, 주석 `(SysTick = Priority 0, EXTI = Priority 1 이상 권장)`의 "Priority 0"은 명백한 수치 오류입니다. SysTick = Priority 0이라고 코드 주석에 기술하면 초보 독자가 SysTick이 가장 높은 우선순위라고 오해합니다. 실제로는 SysTick이 Priority 15(최저)이기 때문에, EXTI를 Priority 0~14 중 어떤 값으로 설정해도 SysTick보다 높으며, HAL_GetTick()은 EXTI ISR에서 정상 동작하지 않습니다. (EXTI ISR이 SysTick을 선점하기 때문이 아니라, EXTI ISR이 실행되는 동안 SysTick이 인터럽트를 발생시키지 못하기 때문입니다.)

**수정 방향**:

`ch01_ex05` 주석 line 85를 다음과 같이 수정:
```
(SysTick = Priority 15(최저), EXTI = Priority 1 이상 권장)
```

원고 §1.4 면접 포인트의 "SysTick, HAL_Delay는 우선순위 15(최저)를 사용" 문장은 사실에 부합하므로 유지하되, ex05 주석과 일치시킵니다.

원고 §1.5 FAQ의 핵심 설명("SysTick보다 낮게 설정해야 HAL_GetTick이 정확히 동작한다")은 **논리적으로 잘못**되어 있습니다. EXTI Priority = 1, SysTick Priority = 15이면 EXTI가 SysTick보다 우선순위가 높습니다. EXTI ISR이 실행 중이면 SysTick(Priority 15)은 선점할 수 없어 HAL_GetTick()이 최대 1ms까지 지연될 수 있습니다. 이 오류는 텍스트 설명을 다음과 같이 보완해야 합니다:

> "EXTI ISR 내에서 HAL_GetTick()은 사용 가능하지만, ISR 실행 중 SysTick 인터럽트가 선점하지 못하므로 최대 1ms의 오차가 발생할 수 있습니다. 단순 디바운싱 판정(50ms 기준)에는 이 오차가 문제가 되지 않습니다."

---

### Critical-02: GPIO_PIN_13 비트마스크 값 오류 — 원고 §1.2 FAQ 박스

**위치**: `chapter01.html` §1.2 FAQ 박스 (line ~334-340)

**문제**:

원고 §1.2 FAQ에 다음과 같이 기술되어 있습니다:

> "반면 숫자 13은 0x000D = 0000 0000 0000 1101로, **0번, 2번, 3번 비트**가 1입니다."
> "HAL 함수에 13을 넣으면 전혀 다른 핀들(**PA0, PA2, PA3**)이 영향을 받아..."

**오류 내용**:

`0x000D = 0b0000_0000_0000_1101`이므로 비트 0, 2, 3이 세팅됩니다. 핀 번호 0 = PA0, 핀 번호 2 = PA2, 핀 번호 3 = PA3가 영향을 받는다는 설명은 기술적으로 사실입니다.

그런데 문제는 이 FAQ가 `GPIO_PIN_13`의 올바른 값도 함께 제시하면서 내부적으로 불일치가 발생한다는 점입니다. 원고 본문에서 `GPIO_PIN_13 = ((uint16_t)0x2000)`이라고 정확하게 설명하고, ex04/ex05 코드 주석에서도 `GPIO_PIN_13 = (uint16_t)0x2000 = 0b0010000000000000`라고 올바르게 기술하고 있습니다.

그러나 비트 번호에 대한 서술이 엇갈립니다: ex04 코드 주석(line 69)에 "13번째 비트(0부터 셈)"라고 정확히 기술하고 있으나, 원고 §1.1에서 `GPIO_PIN_5 = 0x0020, 5번째 비트(0부터 시작)`는 올바르게 기술하는 반면, §1.2 FAQ에서는 "13번째 비트"임을 명시하지 않고 바로 0번/2번/3번 비트 예시로 넘어가 독자가 혼란을 겪을 수 있습니다.

더 중요한 오류: 원고 §1.2 FAQ에서 `GPIO_PIN_13`을 설명하면서 "13번째 비트(0부터 시작)가 1"이라는 핵심 정보를 명시하지 않아, 왜 0x2000인지에 대한 설명이 불완전합니다. `0x2000 = 2^13 = 8192`이므로 비트 13이 세팅됨을 명시해야 합니다.

**수정 방향**:

§1.2 FAQ의 GPIO_PIN_13 설명에 다음을 추가:
> "`GPIO_PIN_13 = ((uint16_t)0x2000)`, 즉 2진수 `0010 0000 0000 0000`으로 **13번째 비트(0부터 시작)**가 1입니다. `2^13 = 8192 = 0x2000`이므로 확인할 수 있습니다."

이미 원고 내 다른 곳(§1.2 핵심정리 섹션)에서 `GPIO_PIN_13 = 비트마스크 0x2000`을 올바르게 언급하고 있으므로, FAQ 박스와 일치시키면 됩니다.

---

## 🟡 Major 이슈

### Major-01: ex04의 EXTI15_10_IRQHandler 주석 코드 — 비표준 패턴 제시

**위치**: `ch01_ex04_button_exti.c` line 47~61

**문제**:

주석으로 제시한 `EXTI15_10_IRQHandler` 예시 코드가 EXTI10~15 모든 핀에 대해 `HAL_GPIO_EXTI_IRQHandler`를 각각 호출하는 패턴을 보여줍니다:

```c
HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
// ...
HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
// ...
```

**문제 설명**: 이는 CubeMX가 실제로 생성하는 코드와 다릅니다. CubeMX는 활성화된 핀만 포함하며, 보통 단일 핀만 사용할 경우 아래와 같이 생성합니다:

```c
void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}
```

모든 PIN_10~15를 나열하는 패턴을 "실제 CubeMX 생성 코드는 사용 중인 핀만 포함한다"는 주석으로 보완하고 있으나, 초보 독자가 이 주석을 쉽게 간과하고 실제로 10~15를 모두 써야 한다고 오해할 수 있습니다.

**수정 방향**: 주석 예시 코드를 CubeMX 실제 생성 패턴(사용 핀만 포함)으로 교체하거나, 현재 예시가 "설명을 위한 확장 예시"임을 더 명확하게 강조하는 경고 문구를 추가합니다.

---

### Major-02: ex02의 Open-Drain HIGH 동작 설명 — PA5 보드 풀업 존재 여부 불명확

**위치**: `ch01_ex02_pushpull_vs_opendrain.c` line 130~138

**문제**:

주석에서 "NUCLEO-F411RE의 PA5는 보드 내부에서 LED와 저항으로 연결되어 있어 미세한 전류가 흐를 수 있으나, 정상적인 HIGH 구동은 불가하다"고 기술합니다.

NUCLEO-F411RE 회로도에 따르면 PA5(LD2)의 연결은 PA5 → 510Ω 저항 → LED → GND 입니다. LED 애노드 쪽에 VCC 풀업은 없습니다. Open-Drain HIGH(High-Z) 상태에서는 LED 경로를 통해 GND로 흐를 수 있는 전류 경로가 없으므로 "미세한 전류가 흐를 수 있다"는 설명은 부정확합니다. 실제로는 Open-Drain HIGH 시 PA5는 완전한 High-Z 상태가 되어 전류가 흐르지 않고 LED도 켜지지 않습니다.

**수정 방향**: "미세한 전류가 흐를 수 있다" 문구를 삭제하고, "PA5에는 외부 VCC 풀업이 없으므로 Open-Drain HIGH 상태에서 핀은 완전한 High-Z(부동) 상태가 되어 LED는 켜지지 않는다"로 수정합니다.

---

### Major-03: ex03의 무한 대기 루프 — 타임아웃 처리 강조 부족

**위치**: `ch01_ex03_button_polling.c` line 120~123

**문제**:

```c
while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
{
    /* 버튼이 떼어질 때까지 대기 (CPU는 여기서 블로킹) */
}
```

주석에서 "버튼이 기계적으로 걸려서 영원히 눌린 채로 있으면 무한 대기한다. 실제 제품에서는 타임아웃 처리가 필요하다"고 언급하고 있습니다. 그러나 이는 교재 코드이므로, 실제 동작에서 이 버그가 발생하면 독자가 원인을 찾기 매우 어렵습니다.

이 코드는 "문제를 보여주는 버전 1"과 "개선된 버전 2"의 구조를 취하고 있지만, 버전 2(DEBOUNCE_ENABLE=1)에도 타임아웃 없는 무한 대기가 포함되어 있어 교육적 완성도가 떨어집니다.

**수정 방향**: `ch01_ex03`은 디바운싱의 한계를 보여주는 예제이므로 코드 수정보다는, 주석에 `/* WARNING: 실제 제품 코드에는 반드시 HAL_GetTick() 기반 타임아웃 추가 필요 */`를 더 눈에 띄게 작성하고, ch01_ex05의 타임스탬프 방식이 이 문제를 근본적으로 해결함을 더 강조합니다.

---

### Major-04: 원고 §1.5의 ISR 황금 법칙 4항 — 원자성 설명에 메모리 배리어 혼동

**위치**: `chapter01.html` §1.5 ISR 황금 법칙 항목 4 (line ~762)

**문제**:

> "공유 데이터 접근 시 원자성(Atomicity) 보장: 32비트보다 큰 변수(예: uint64_t)는 여러 명령어로 처리되어 ISR에 의해 중간에 끊길 수 있습니다."

이 설명은 사실이지만 불완전합니다. ARM Cortex-M4에서 **32비트 이하 정렬된 읽기/쓰기**는 원자적입니다. `uint8_t`, `uint16_t`, `uint32_t` 타입의 aligned 변수는 단일 인스트럭션으로 처리되어 ISR 도중 깨질 수 없습니다. 따라서 이 교재에서 사용하는 `volatile uint8_t g_button_flag`와 `volatile uint32_t s_last_press_tick`은 원자성 문제가 없습니다.

그러나 `volatile`이 멀티코어에서의 메모리 배리어를 보장하지 않는다는 설명(volatile 비유 박스 하단 주석)은 STM32(단일 코어)에서는 불필요한 혼란을 줄 수 있습니다. 단일 코어 Cortex-M에서 `volatile`은 ISR/메인 루프 공유에 충분합니다.

**수정 방향**: 항목 4를 다음과 같이 수정합니다:
> "공유 데이터 접근 시 원자성(Atomicity) 보장: ARM Cortex-M4에서 32비트 이하의 정렬된 변수 읽기/쓰기는 원자적입니다. 단, uint64_t 등 64비트 이상 변수나 read-modify-write 연산(++, |=, &=)은 여러 명령어로 처리될 수 있어 ISR에 의해 중간에 끊길 수 있습니다. 이 경우 __disable_irq()/__enable_irq()로 임계 구간을 보호해야 합니다."

---

## 🟢 Minor 이슈

### Minor-01: ex01 헤더 주석의 NUCLEO-F411RE 최대 클럭 표기

**위치**: `ch01_ex01_led_blink.c` line 14

**문제**:
> "클럭: HSI 16MHz (기본값) 또는 HSE → PLL 100MHz 설정 가능"

STM32F411RE의 최대 시스템 클럭은 100MHz가 맞습니다. 그러나 NUCLEO-F411RE 기본 CubeMX 설정에서 HSE 없이 HSI만 사용 시 기본 PLL 설정으로 84MHz까지 설정되는 경우가 있으며, 100MHz는 HSE(8MHz) + PLL 사용 시에 주로 달성됩니다. 오류는 아니나, "100MHz"에 "최대 클럭" 또는 "HSE+PLL 조합 시" 조건을 명시하면 더 정확합니다.

**수정 방향**: `"클럭: HSI 16MHz (기본값) 또는 HSE + PLL → 최대 100MHz 설정 가능"`으로 수정.

---

### Minor-02: 원고 §1.1의 Open-Drain 내부 구조 설명 — "두 MOS 모두 OFF" 표현

**위치**: `chapter01.html` §1.1 (line ~69) 및 ex02 주석 표 (line 20)

**문제**:
> "HIGH: 두 MOS 모두 OFF → 핀이 플로팅"

Open-Drain 회로에는 P-MOS가 존재하지 않습니다. 회로 자체에 N-MOS 하나만 있습니다. "두 MOS 모두 OFF"라는 표현은 Push-Pull의 P-MOS와 N-MOS 양쪽이 있다는 오해를 줄 수 있습니다. ex02의 주석 표에서도 동일한 표현을 사용합니다.

**수정 방향**: "N-MOS OFF → 핀이 High-Z(고임피던스) 상태. 외부 풀업이 없으면 부동(floating)"으로 수정합니다.

---

### Minor-03: ex05의 s_last_press_tick volatile 누락

**위치**: `ch01_ex05_button_exti_debounce.c` line 65

**문제**:
```c
static uint32_t s_last_press_tick = 0;
```

이 변수는 ISR(`HAL_GPIO_EXTI_Callback`)에서만 접근하고 메인 루프에서는 직접 읽지 않으므로, 기술적으로는 volatile이 없어도 동작합니다. 그러나 교육 맥락에서 ISR 내부에서 읽고 쓰는 변수에 volatile을 붙이지 않으면 컴파일러 최적화 레벨이 높아질 때(예: -O2, -O3) 레지스터 캐싱이 발생할 수 있습니다. 특히 인라인 최적화 환경에서 잠재적 버그가 될 수 있습니다.

**수정 방향**: `static volatile uint32_t s_last_press_tick = 0;`으로 수정을 권장합니다. 교육 코드에서는 ISR에서 사용하는 모든 전역/정적 변수에 volatile을 일관되게 붙이는 습관을 가르치는 것이 좋습니다.

---

### Minor-04: 원고 내부 GPIO_SPEED_FREQ_HIGH 주파수 수치

**위치**: `ch01_ex02_pushpull_vs_opendrain.c` line 79-85 주석

**문제**:
```
GPIO_SPEED_FREQ_LOW       : 저속 (~2MHz)
GPIO_SPEED_FREQ_MEDIUM    : 중속 (~25MHz)
GPIO_SPEED_FREQ_HIGH      : 고속 (~50MHz)
GPIO_SPEED_FREQ_VERY_HIGH : 초고속 (~100MHz)
```

STM32F411RE Reference Manual(RM0383)의 전기적 특성 표에 따르면 속도 등급은 VDD 전압과 부하 용량에 따라 달라집니다. 3.3V, 50pF 부하 조건에서 대략적으로 위 수치에 근접하지만, GPIO_SPEED_FREQ_MEDIUM은 12.5MHz~25MHz 범위로 전압/부하에 따라 차이가 있습니다. "~25MHz"는 최솟값에 가까운 수치입니다.

**수정 방향**: 주석에 "조건에 따라 달라질 수 있음. RM0383 Table 52 참조"를 추가합니다.

---

### Minor-05: 원고 §1.3의 button_detect_press 함수의 last_change_ms 초기값 문제

**위치**: `chapter01.html` §1.3 코드 예제 `debounce_polling.c` (line ~490-513)

**문제**:
```c
static uint32_t last_change_ms = 0;
```

시스템 시작 직후 `HAL_GetTick()`이 0이면 `(now_ms - last_change_ms) = 0`이 되어 DEBOUNCE_TIME_MS(20) 미만이므로 첫 번째 버튼 누름이 무시됩니다. 실제로는 시스템 부팅 후 20ms 이내에 버튼을 누르면 첫 눌림이 무시되는 상황입니다.

이는 매우 드문 엣지 케이스(부팅 20ms 이내 버튼 누름)이므로 실용적으로 큰 문제는 아니지만, 교육 코드에서는 `static uint32_t last_change_ms = 0 - DEBOUNCE_TIME_MS;` 또는 초기값을 충분히 과거 시간으로 설정하는 패턴을 가르치면 더 견고합니다. ex05의 `s_last_press_tick = 0` 초기화도 동일한 이슈를 가지나, 그 코드에서는 별도 주석("시스템 시작 시 0ms 기준")으로 언급하고 있습니다.

**수정 방향**: 주석에 "※ 부팅 후 DEBOUNCE_TIME_MS 이내 첫 누름은 무시될 수 있습니다"를 추가하거나, `last_change_ms`를 적절한 초기값으로 설정합니다.

---

## 긍정 평가 사항

1. **HAL API 사용 정확성**: `HAL_GPIO_WritePin`, `HAL_GPIO_TogglePin`, `HAL_GPIO_ReadPin` 모두 올바른 인자 형태로 사용되었습니다. GPIO_PIN_SET/GPIO_PIN_RESET 매크로를 일관되게 사용하여 가독성이 높습니다.

2. **GPIO_PIN_13 비트마스크 경고 반복 강조**: ex04, ex05에서 `GPIO_Pin == GPIO_PIN_13` vs `GPIO_Pin == 13`의 차이를 코드 주석에서 명확히 경고한 점은 매우 우수합니다. 초보자가 가장 많이 저지르는 오류를 코드 레벨에서 방지합니다.

3. **EXTI 동작 흐름 정확성**: ex04에서 `EXTI15_10_IRQHandler → HAL_GPIO_EXTI_IRQHandler → HAL_GPIO_EXTI_Callback` 호출 체인을 정확하게 설명하고 있습니다. `__weak` 오버라이드 패턴 설명도 정확합니다.

4. **volatile 적용 교육**: ex05에서 `volatile` 키워드의 필요성과 원리를 레지스터 캐시 관점에서 상세하고 정확하게 설명하고 있습니다. ISR-메인 루프 간 공유 변수에 volatile을 붙이는 이유가 초보자가 이해하기 쉽게 서술되어 있습니다.

5. **uint32_t 오버플로우 안전성**: ex05의 `(current_tick - s_last_press_tick)` 패턴이 uint32_t unsigned 산술에서 오버플로우에도 안전함을 명확히 설명하고 수치 예시까지 제시한 점이 우수합니다.

6. **PC13 액티브 로우 설명 정확성**: 원고와 모든 예제 코드에서 PC13(B1) = 액티브 로우, 누름 = GPIO_PIN_RESET, 뗌 = GPIO_PIN_SET 설명이 일관되게 정확합니다.

7. **PA5 액티브 하이 설명 정확성**: PA5(LD2) = 액티브 하이, GPIO_PIN_SET = LED 켜짐 설명이 일관되게 정확하고 NUCLEO-F411RE 하드웨어 사실과 일치합니다.

8. **EXTI15_10_IRQn 공유 설명 정확성**: PC13이 EXTI라인 13에 매핑되고, EXTI10~15가 EXTI15_10_IRQn 하나의 핸들러를 공유한다는 설명이 정확합니다.

9. **ISR 황금 법칙 전반**: HAL_Delay 금지, 블로킹 I/O 금지, 짧게 유지 등 핵심 규칙이 정확하고 논리적 근거와 함께 제시되었습니다.

10. **플래그 클리어 순서 설명**: ex05에서 "처리 전에 플래그를 먼저 클리어"하는 패턴과 그 이유(처리 중 새 인터럽트 유실 방지)를 정확하게 설명합니다.

---

## 종합 의견

Chapter 01은 GPIO 출력/입력, 폴링, EXTI 인터럽트, 디바운싱으로 이어지는 학습 흐름이 논리적이고 완성도가 높습니다. 코드 예제 5개 모두 NUCLEO-F411RE에서 실제 동작하는 패턴을 올바르게 제시하며, HAL API 사용상의 Critical한 오류는 없습니다.

수정이 필요한 Critical 이슈는 코드가 아닌 **원고 텍스트와 주석의 기술 설명 오류**입니다. 특히 SysTick 우선순위 수치 오류(Critical-01: ex05 주석 "Priority 0" → "Priority 15")는 독자가 NVIC 우선순위 체계에 대해 정반대의 이해를 할 수 있으므로 즉각 수정이 필요합니다.

Critical 2건, Major 4건이 수정되면 이 챕터는 STM32 GPIO/EXTI 입문 교재로서 높은 기술적 신뢰도를 갖추게 됩니다.
