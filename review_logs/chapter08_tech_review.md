# Ch08 기술 리뷰어 의견서

**리뷰어**: 기술 리뷰어 (STM32 HAL 전문가)
**대상**: `manuscripts/part2/chapter08.html` + `code_examples/ch08_*.c/.h`
**일시**: 2026-03-17

---

## Critical Issues (🔴)

### 🔴 C1. main.c 버튼 이벤트 분기 오류 — BTN_SHORT가 BTN_LONG으로 전달됨

**위치**: `chapter08.html` 1044~1052행 (§8.7 App 레이어 코드)

```c
if (duration >= 2000) {
    stopwatch_handle_event(SW_EVT_BTN_LONG);
} else {
    stopwatch_handle_event(SW_EVT_BTN_LONG);  // ← BUG! SW_EVT_BTN_SHORT이어야 함
}
```

**문제**: `else` 분기에서 `SW_EVT_BTN_SHORT` 대신 `SW_EVT_BTN_LONG`을 호출합니다.
이대로 실행하면 **짧게 누르든 길게 누르든 항상 LONG 이벤트**가 발생합니다.
IDLE 상태에서 버튼을 짧게 눌러도 Start가 되지 않고 무시됩니다 (IDLE+BTN_LONG = 무시).
**스톱워치가 절대 시작되지 않는 치명적 버그**입니다.

**수정**:
```c
} else {
    stopwatch_handle_event(SW_EVT_BTN_SHORT);  // 수정
}
```

### 🔴 C2. 버튼 Long Press 감지 로직 결함 — Press 시점만 기록, Release 미감지

**위치**: `chapter08.html` 1021~1052행 (§8.7 App 레이어)

현재 로직:
1. EXTI Falling Edge → `g_btn_press_ms = HAL_GetTick()` 기록
2. 메인 루프에서 `g_btn_pressed` 확인 → `duration = HAL_GetTick() - g_btn_press_ms` 계산

**문제**: EXTI 콜백이 버튼 **누르는 순간** 호출되고, 메인 루프에서 **거의 즉시** `duration`을 계산합니다. `duration`은 항상 0~수 ms이므로 **BTN_LONG이 절대 발생하지 않습니다**.

Long Press를 올바르게 구현하려면:
- **방법 A**: Falling Edge(누름)에서 시각 기록 → Rising Edge(뗌)에서 duration 계산
- **방법 B**: 누르는 순간 기록 → 메인 루프에서 2초 경과 확인 (polling)

현재 코드는 교육 목적의 간략화로 보이지만, "동작하는 코드"를 표방하므로 수정이 필요합니다. 최소한 주석으로 "이 코드는 Long Press 감지가 불완전하며, 실제 구현에는 Rising Edge 감지 또는 타이머 기반 방식이 필요합니다"라는 안내가 있어야 합니다.

---

## Major Issues (🟡)

### 🟡 M1. TIM1 APB2 클럭 설명 — 84MHz는 조건부 정확

**위치**: `chapter08.html` 156행 (§8.1 비교 표)

> APB2 (84MHz)

STM32F411RE의 APB2 프리스케일러가 /1일 때 타이머 클럭은 84MHz(= HCLK)입니다.
이것은 CubeMX 기본 설정(HCLK=84MHz, APB2 PSC=/1)에서 맞습니다.
하지만 APB1과 달리 APB2는 프리스케일러 ×2 배가 적용되지 않는 경우가 있어, "CubeMX 기본 84MHz 클럭 설정 기준"이라는 전제 조건을 명시하면 좋겠습니다.

**수정 권장**: 표에 각주 추가 — "※ CubeMX 기본 설정(HSE 8MHz, HCLK 84MHz) 기준"

### 🟡 M2. stopwatch_get_time() 경합 조건 (Race Condition)

**위치**: `ch08_05_stopwatch_service.c` 124행

```c
uint32_t total = s_elapsed_ms + s_tick_count;
```

`s_tick_count`는 1ms ISR에서 증가하고, `stopwatch_get_time()`은 메인 루프에서 호출됩니다.
32비트 Cortex-M4에서 단일 word 읽기는 원자적이지만, `s_elapsed_ms + s_tick_count` 두 변수를 더하는 도중 ISR이 `s_tick_count`를 변경하면 **비일관적 합계**가 나올 수 있습니다.

실무에서는 `__disable_irq()` / `__enable_irq()` 또는 임계 구역으로 보호합니다.
교육 코드이므로 당장 수정은 필요 없지만, **주석으로 경합 조건 존재를 언급**하고 "Ch15(RTOS)에서 뮤텍스로 해결" 같은 연결을 하면 교육적 가치가 높아집니다.

### 🟡 M3. Master-Slave 코드에서 Slave 구성 코드 부재

**위치**: `ch08_04_master_slave.c` (코드 예제) / `chapter08.html` 660~678행

Master-Slave 연계를 설명하면서 코드에는 `HAL_TIM_SlaveConfigSynchro()` 호출이 없습니다.
"CubeMX에서 설정" 전제이지만, 교재의 다른 예제들은 HAL API 호출을 명시적으로 보여줍니다.
최소한 주석으로 "CubeMX가 `MX_TIM2_Init()`에서 `HAL_TIM_SlaveConfigSynchro()`를 자동 생성"이라고 명시해야 합니다.

### 🟡 M4. Input Capture 오버플로 카운트 0xFFFF vs 0x10000

**위치**: `ch08_02_input_capture.c` 84행, `ch08_06_tim_driver_extended.c` 138행

```c
diff = (0xFFFF - s_capture_1st) + s_capture_2nd + 1;
```

TIM3는 16비트이므로 ARR=0xFFFF일 때 카운터는 0~65535를 순환합니다.
`(0xFFFF - prev) + curr + 1`은 수학적으로 `(0x10000 - prev) + curr`와 동일하므로 **계산은 정확합니다**.
다만, 다중 오버플로(신호 주기 > 65.535ms) 시에는 이 코드가 부정확합니다.
교재에서 "측정 가능 최저 주파수 ≈ 15Hz (1MHz/65536)" 제한을 명시하면 좋겠습니다.

---

## Minor Issues (🟢)

### 🟢 m1. HTML 엔티티 오류

**위치**: `chapter08.html` 886행 (§8.7 stopwatch_service.h 코드 블록)

```html
SW_EVT_BTN_SHORT = 0,   /* 버튼 짧게 누름 (&lt;2초) */
SW_EVT_BTN_LONG  = 1,   /* 버튼 길게 누름 (&ge;2초) */
```

`<pre><code>` 안에서 `&lt;`와 `&ge;`가 HTML 엔티티로 표시됩니다. 브라우저 렌더링 시 `<`와 `≥`로 보이므로 기능적으로는 문제없으나, `&ge;`는 비표준적입니다. 실제 `.h` 파일(`ch08_05_stopwatch_service.h`)에서는 올바르게 `<`와 `>=`로 되어 있어 코드 예제 파일 자체는 정상입니다.

### 🟢 m2. ch08_03_one_pulse.c — TIM4 사용 시 핀 할당 미명시

**위치**: `chapter08.html` 580~591행 (§8.4)

One-Pulse에서 TIM4 CH1 → PB6을 사용하지만, CubeMX 설정 절차(One-Pulse Mode 활성화, Trigger Source 설정)가 §8.2처럼 상세하게 기술되지 않았습니다.
§8.4가 개념 중심이라 문제는 아니지만, PB6이 다른 주변장치와 충돌하지 않는지 확인 필요합니다. (PB6 = I2C1_SCL — Ch11에서 SHT31에 사용 가능성 있음)

### 🟢 m3. Dead-time 표의 DTG=168 계산

**위치**: `chapter08.html` 226행

> DTG = 168 → (64+40) × 2 × T_DTS → ~2.5 us

DTG 비트 구간별 공식에 따르면 DTG[7:5] = 10x일 때:
DT = (64 + DTG[5:0]) × 2 × T_DTS
DTG=168 = 0b10101000 → DTG[5:0] = 0b101000 = 40
DT = (64+40) × 2 × (1/84MHz) = 104 × 2 / 84MHz ≈ 2.476us
**계산은 정확합니다.** 다만 "~2.5 us"보다 "≈2.48 us"가 더 정확합니다. (Minor)

### 🟢 m4. 그림 번호 불일치

**위치**: `chapter08.html`

- §8.0 아키텍처 위치의 그림 → "그림 08-1" (OK)
- §8.0 FSM 설계의 그림 → "그림 08-2" (OK)
- §8.1 비교 → "그림 08-3" (OK)
- 핵심 정리의 그림 → "그림 08-10" — 동일 SVG(`ch08_sec8_v08_final.svg`)가 §8.8에서 "그림 08-9"로, 핵심 정리에서 "그림 08-10"으로 이중 번호 부여

**수정**: 핵심 정리의 "그림 08-10"을 "그림 08-9 (재게시)"로 변경하거나 번호를 통일

---

## 아키텍처 검증

| 항목 | 결과 | 비고 |
|------|------|------|
| 레이어 배치 | ✅ O | Service(stopwatch) → Driver(tim) → HAL 정확 |
| API 경계 | ✅ O | Service가 HAL 직접 호출 없음, Driver API만 사용 |
| FSM 표현 | ✅ O | 상태 전이표 + SVG 다이어그램 + switch-case 코드 3중 표현 |
| Ch07 호환성 | ✅ O | 기존 tim_start/stop/pwm_* API 변경 없음, 새 함수만 추가 |
| 인터페이스 불변 | ✅ O | v0.7 코드 재컴파일 가능 |
| ISR 안전 | ✅ O | EXTI에서 플래그만, FSM 전이는 메인 루프 |

---

## 코드 품질 검증

| 항목 | 결과 | 비고 |
|------|------|------|
| HAL API 정확성 | ✅ | PWM_Start, PWMN_Start, IC_Start_IT, ReadCapturedValue 모두 정확 |
| 에러 처리 | ✅ | HAL 반환값 확인, LOG_E 출력 |
| 한국어 주석 | ✅ | 모든 코드에 한국어 주석 |
| LOG 매크로 | ✅ | LOG_I/D/W/E 적절 사용 |
| 들여쓰기 4칸 | ✅ | 일관 |
| 스네이크 케이스 | ✅ | cpwm_start, ic_get_frequency 등 |
| 코드 블록 30줄 이하 | ✅ | 최대 블록 ~30줄 (FSM) |

---

## 최종 평가

| 기준 | 결과 |
|------|------|
| Critical | **2개** (C1: BTN 분기 버그, C2: Long Press 로직 결함) |
| Major | 4개 (M1~M4) |
| Minor | 4개 (m1~m4) |

### 판정: 🔴 **조건부 반려 (Conditional Reject)**

**C1은 반드시 수정 필수** — 복사-붙여넣기 시 스톱워치가 동작하지 않습니다.
**C2는 최소한 주석 보완 필수** — 동작 불가능한 코드를 "동작 예제"로 제시하면 안 됩니다.

Critical 2개 수정 후 재검토하면 **승인 가능**합니다.
Major 항목들은 권장 수정이며, 교육적 가치를 높이는 개선입니다.

---

## 긍정적 평가

1. **아키텍처 구조가 탁월합니다** — Service 레이어 첫 등장을 자연스럽게 도입하고, Driver와의 경계를 명확히 설명합니다.
2. **FSM 설계 → 구현 흐름이 교과서적입니다** — 상태 전이표 → 다이어그램 → switch-case 코드로 이어지는 과정이 체계적입니다.
3. **비유가 적절합니다** — 시소(CPWM), 랩 버튼(IC), 부장-대리(Master-Slave) 모두 직관적입니다.
4. **코드 품질이 높습니다** — 에러 처리, 로그, 네이밍 컨벤션 모두 일관됩니다.
5. **핀 충돌 주의사항이 실무적입니다** — PA7/SPI1 충돌 경고, PB13 대안 제시가 실용적입니다.
