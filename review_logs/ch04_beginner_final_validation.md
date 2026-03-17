# Ch04 초급 독자 최종 검증

**리뷰어**: 초급 독자
**일시**: 2026-03-17
**대상**: `manuscripts/part1/chapter04.html` (Phase 4 수정 후)

---

## 검증 결과

### 1. §4.3 LED 블로킹 체험 코드 — PASS
- `uint32_t last_led = 0;` 선언 추가 확인 (line 372)
- `HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);` 호출 확인 (line 388)
- "폴링 중 LED가 멈춘다"가 코드로 체감 가능: HAL_UART_Receive 1초 블로킹 → 토글 주기 ~1.5초

### 2. §4.4 인터럽트 main loop — PASS
- `HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);` 호출 확인 (line 576)
- `last_led` 변수 사용 확인 (line 575, 577)
- "인터럽트 덕분에 LED가 계속 토글된다"가 코드로 증명됨: uart_available()은 즉시 반환 → 500ms 토글 정상 동작

### 3. 별점 재평가
- 이전: ⭐⭐⭐⭐½ (코드 누락 2건)
- 현재: **⭐⭐⭐⭐⭐** (모든 코드가 따라 하기 가능, 학습 목표와 코드가 일치)

---

## 최종 판정: **승인**
