# Ch04 기술 최종 검증 보고서

- **검증자**: 기술 리뷰어
- **일자**: 2026-03-17
- **판정**: ❌ **추가 수정 필요** (1건)

---

## 검증 항목 1: 코드 오류 수정 확인

### 1-1. LED 토글 호출 추가 — ❌ 미수정

**파일**: `ch04_03_uart_interrupt_rx.c` 라인 110~116

```c
/* 2) LED 토글 — 인터럽트 덕분에 멈추지 않음! */
uint32_t now = HAL_GetTick();
if (now - last_led_tick >= 500) {
    /* 주의: LED 상태를 '1'/'0' 명령이 덮어쓸 수 있음
     * → 실제 프로젝트에서는 모드 분리 필요 */
    last_led_tick = now;
}
```

**문제**: `HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);` 호출이 여전히 없음. `last_led_tick`만 갱신하고 LED를 실제로 토글하지 않음. 이 코드를 그대로 실행하면 LED가 전혀 깜빡이지 않아 "인터럽트 덕분에 멈추지 않음!"이라는 주석과 모순됨.

**수정 필요**:
```c
if (now - last_led_tick >= 500) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);  /* ← 이 줄 추가 */
    last_led_tick = now;
}
```

### 1-2. last_led 변수 선언 (HTML 본문) — ✅ OK

`chapter04.html` 라인 372에 `uint32_t last_led = 0;`이 선언되어 있음. HTML 본문의 코드 스니펫에서는 정상.

### 1-3. DMA 챕터 번호 통일 — ✅ OK

검증 결과:
- 본문에서 DMA 원리를 Ch05로, UART DMA 전환을 Ch06으로 명확 구분
- 라인 823 figcaption: "Ch05에서 DMA 원리를, Ch06에서 UART DMA 전환을 다룬다" — 명확
- 라인 1014: "Ch05. DMA 아키텍처와 동작 원리" — 정확
- 라인 1020: "Ch06에서는 Ch04의 uart_driver.c 내부 구현을 DMA로 교체" — 정확
- 기존 "Ch06 DMA" 참조들도 UART DMA 전환 맥락에서 정확하게 사용됨

Ch05=DMA 원리, Ch06=UART DMA 적용으로 일관성 확보. **통일 완료**.

### 1-4. SVG 함수명 대소문자 — ✅ OK

- `ch04_sec00_sequence.svg` 라인 37: `uart_send("Hello", 5)` — 소문자 스네이크 케이스 ✅
- `ch04_sec00_architecture.svg` 라인 95: `uart_send()` — 소문자 스네이크 케이스 ✅

코드의 `uart_send()` 함수명과 일치. **수정 완료**.

---

## 검증 항목 2: 코드 컴파일 가능성

### `ch04_03_uart_interrupt_rx.c` — ⚠️ 컴파일은 가능하나 동작 오류

- 문법적으로 컴파일 가능 (오류 없음)
- 그러나 LED 토글 코드 누락으로 **런타임 동작이 기대와 다름**
- 수정 후 재검증 필요

### `ch04_04_uart_driver.c` — ✅ OK

- 모든 include 정상 (`uart_driver.h`, `main.h`, `log.h`, `<string.h>`)
- `extern UART_HandleTypeDef huart2` 선언 정상
- 모든 함수 시그니처가 `uart_driver.h`와 일치
- `volatile` 키워드 적절 사용
- 링 버퍼 인덱스 연산 정확 (`(head + 1) % SIZE`)
- 컴파일 문제 없음

---

## 검증 항목 3: HAL API 정확성

### `HAL_GPIO_TogglePin()` — ✅ (추가되면 정확)

- `HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5)` — NUCLEO-F411RE LED2 핀 정확
- HAL 표준 API 사용법 준수
- 단, 현재 코드에는 이 호출이 없으므로 추가 필요

---

## 최종 판정

| 항목 | 결과 |
|------|------|
| DMA 챕터 번호 통일 | ✅ OK |
| SVG 함수명 대소문자 | ✅ OK |
| last_led 변수 선언 (HTML) | ✅ OK |
| LED 토글 코드 추가 (ch04_03) | ❌ 미수정 |
| ch04_04_uart_driver.c 컴파일 | ✅ OK |
| HAL API 정확성 | ✅ OK |

**판정: 추가 수정 필요 (1건)**

`ch04_03_uart_interrupt_rx.c` 라인 114에 `HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);`를 추가하면 최종 승인.

이 수정은 1줄, 30초 작업입니다.
