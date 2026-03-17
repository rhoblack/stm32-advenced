# Ch04 기술 리뷰 보고서

- **리뷰어**: 기술 리뷰어
- **대상**: Ch04. UART 기초
- **일자**: 2026-03-17
- **판정**: ✅ **조건부 승인 (Conditional Approval)**

---

## 1. 🔴 Critical Issues (즉시 수정 필요)

### C1. 다음 챕터 예고 — DMA 챕터 번호 불일치
- **위치**: `chapter04.html` 라인 978, 808, 965
- **문제**: 다음 챕터 예고에서 "Ch05. DMA 아키텍처와 동작 원리"라고 명시하고, figcaption에서 "다음 Ch05에서 DMA가 추가된다"라고 기술. 그러나 본문 여러 곳에서는 "Ch06에서 DMA로 전환", "Ch06 DMA + IDLE Line", "Ch06 링 버퍼로 개선" 등으로 DMA를 Ch06으로 참조.
- **영향**: 독자가 DMA가 Ch05인지 Ch06인지 혼란. TABLE_OF_CONTENTS.md 기준으로 통일 필요.
- **권고**: TABLE_OF_CONTENTS.md의 실제 챕터 번호에 맞춰 모든 참조를 통일.

### C2. `uart_send()` 내 const 캐스팅 문제
- **위치**: `ch04_04_uart_driver.c` 라인 56, `chapter04.html` 라인 707
- **문제**: `HAL_UART_Transmit()`의 두 번째 파라미터는 `uint8_t *`(non-const). `uart_send()`가 `const uint8_t *buf`를 받아서 `(uint8_t *)buf`로 캐스팅하여 전달. 이는 의도적 설계이지만, HAL 함수가 내부적으로 데이터를 수정하지 않음에도 const를 벗기는 캐스트를 명시적으로 설명하지 않으면 학습자가 "const 캐스팅은 안전하다"고 오해할 수 있음.
- **영향**: Minor 수준이지만, 교재 특성상 한 줄 주석으로 "HAL API가 const를 지원하지 않아 캐스팅 필요"를 명시하면 학습 효과 향상.
- **권고**: 코드 주석 또는 본문에서 캐스팅 이유를 한 문장 설명 추가.

---

## 2. 🟡 Major Issues (권장 수정)

### M1. Baud Rate 계산 공식 부재
- **위치**: `chapter04.html` §4.1
- **문제**: 평가 기준 #6에 "115200 = 16MHz / (16 * BRR) 공식이 정확한가?"를 점검하도록 되어 있으나, 본문에 BRR 레지스터 계산 공식 자체가 없음. §4.1에서는 Baud Rate를 "초당 비트 수"로만 설명하고, 내부 레지스터 수준의 설명은 생략.
- **판단**: 이 챕터의 대상(중급 개발자)과 "CubeMX가 자동 설정"이라는 맥락을 고려하면 BRR 생략은 합리적. 단, FAQ로 "CubeMX가 BRR을 어떻게 계산하나요?"를 추가하면 호기심 있는 독자에게 도움.
- **권고**: 선택사항 — BRR 계산을 aside FAQ로 한 단락 추가 고려.

### M2. Overrun Error(ORE) 처리 미언급
- **위치**: `chapter04.html` §4.4, `ch04_04_uart_driver.c`
- **문제**: §4.4에서 Overrun Error를 "발생할 수 있습니다"로 언급하지만, 실제 ORE 플래그 처리 코드는 없음. `HAL_UART_ErrorCallback`이 호출될 수 있다는 점도 미언급. 실무에서 ORE 발생 시 `HAL_UART_Receive_IT()`가 중단되어 재등록 루프가 깨질 수 있음.
- **영향**: 실무 현장에서 "첫 몇 바이트만 수신되고 멈추는" 문제의 원인이 ORE인 경우가 빈번. 재등록 패턴의 약점을 명시해야 함.
- **권고**: §4.4에 aside tip으로 "ORE 발생 시 HAL_UART_ErrorCallback에서 Receive_IT를 재등록해야 한다"는 내용 추가. 코드 예제까지는 불필요하지만, "Ch06에서 DMA로 근본 해결" 연결은 유지.

### M3. `uart_read_byte()` 반환값 0의 모호성
- **위치**: `ch04_04_uart_driver.h` 라인 64, `ch04_04_uart_driver.c` 라인 88
- **문제**: `uart_read_byte()`는 버퍼가 비었을 때 0을 반환. 그러나 실제로 0x00 바이트를 수신할 수도 있음. 이 모호성은 §4.4 본문이나 코드 주석에서 언급되지 않음.
- **영향**: 바이너리 프로토콜에서 0x00을 수신할 경우 "데이터 없음"과 구분 불가.
- **권고**: 코드 주석에 "주의: 0x00 바이트 수신 시 구분 불가. 바이너리 프로토콜에서는 uart_available()로 먼저 확인 필요" 추가. 실제로 본문의 main loop 코드는 `uart_available() > 0` 체크 후 `uart_read_byte()`를 호출하므로 실제 사용 패턴은 안전하지만, 인터페이스 자체의 한계를 명시해야 함.

### M4. `ch04_03_uart_interrupt_rx.c`에서 LED 토글 누락
- **위치**: `ch04_03_uart_interrupt_rx.c` 라인 111-116
- **문제**: "LED 500ms 토글도 동시 진행 (인터럽트 덕분!)"이라고 주석에 명시했으나, 실제 코드에서 `HAL_GPIO_TogglePin()` 호출이 누락됨. `last_led_tick`만 갱신하고 토글 코드가 없음.
- **영향**: 이 예제를 그대로 실행하면 LED가 토글되지 않아, "인터럽트 덕분에 LED가 멈추지 않는다"는 학습 목표를 체감할 수 없음.
- **권고**: `last_led_tick = now;` 위에 `HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);`를 추가.

### M5. 시퀀스 다이어그램 함수명 불일치
- **위치**: `ch04_sec00_sequence.svg`, `ch04_sec00_architecture.svg`
- **문제**: 시퀀스 다이어그램에서 "UART_Send("Hello", 5)"로 표기하고, 아키텍처 다이어그램 의존 화살표에서 "UART_Send()"로 표기. 그러나 실제 코드에서는 `uart_send()` (소문자 스네이크 케이스). 대소문자 불일치.
- **영향**: 독자가 SVG와 코드를 비교할 때 혼란.
- **권고**: SVG 내 텍스트를 `uart_send("Hello", 5)` / `uart_send()`로 수정.

---

## 3. 🟢 Minor Suggestions (선택 수정)

### S1. `ch04_03_uart_interrupt_rx.c`의 `g_rx_flag` 미사용
- **위치**: `ch04_03_uart_interrupt_rx.c` 라인 29, 43
- **문제**: `g_rx_flag` 변수를 선언하고 콜백에서 1로 설정하지만, main loop에서 사용하지 않음.
- **권고**: 삭제하거나, main loop에서 활용하는 예제를 추가.

### S2. `_write()` 함수 중복 정의 가능성
- **위치**: `ch04_02_printf_redirect.c`에서 `_write()` 정의, `ch04_04_uart_driver.c`에서도 `_write()` 정의.
- **문제**: 두 파일을 동시에 프로젝트에 포함하면 링커 에러 발생.
- **권고**: 본문에 "ch04_02는 독립 예제이고, 최종 프로젝트에서는 ch04_04_uart_driver.c의 _write()만 사용" 명시.

### S3. figcaption 번호 vs 실제 배치
- **위치**: `chapter04.html` 라인 44
- **문제**: `ch04_sec01_swo_vs_uart.svg`가 §4.0에 배치되었지만 파일명은 sec01. 그림 번호 "04-1"로 정확하게 매겨졌으므로 기능적 문제는 없으나, 파일 네이밍 규칙과 약간 불일치.
- **권고**: 파일명을 `ch04_sec00_swo_vs_uart.svg`로 변경하면 일관성 향상. 또는 현행 유지(사소한 사항).

### S4. LOG 매크로 일관성
- **위치**: 전체 코드 예제
- **판정**: ✅ 모든 코드에 LOG_D/I/W 적용 확인. Ch02 로그 표준 준수.

### S5. 레스토랑 비유 연속성
- **위치**: `chapter04.html` §4.0 아키텍처 위치, §4.5
- **판정**: ✅ Ch03의 레스토랑 비유(요리사=Driver, 가스레인지=HAL, 메뉴판=인터페이스)를 일관되게 확장. "요리사가 가스레인지를 인덕션으로 바꿔도 주문서는 변하지 않는다"는 DMA 전환 예고도 적절.

---

## 4. 종합 평가

### HAL API 정확성: ✅ 우수
- `HAL_UART_Transmit()`, `HAL_UART_Receive()`, `HAL_UART_Receive_IT()`, `HAL_UART_RxCpltCallback()` — 모두 표준 HAL 사용법 준수.
- USART2 PA2/PA3, ST-Link Virtual COM Port 구성 정확.
- CubeMX 설정 (115200-8-N-1, NVIC Enable) 실무 적합.

### NUCLEO-F411RE 호환성: ✅ 문제없음
- USART2 핀 배치, ST-Link VCP 연결, PA5 LED2 — 모두 정확.

### 컴파일 가능성: ✅ 양호
- 필요한 헤더 include 완비. 문법 오류 없음.
- `const` 캐스팅은 의도적이며 HAL 제약에 의한 것 (C2 참조).

### 재진입 안전성: ✅ 양호
- ISR 내 Receive_IT 재등록은 HAL 내부 상태 머신과 호환. HAL이 huart->RxState를 HAL_UART_STATE_READY로 전환한 후 콜백을 호출하므로, 콜백 내 재등록은 안전.

### Ch03 아키텍처 연결: ✅ 우수
- 4계층 레이어 다이어그램, 인터페이스 설계, 시퀀스 다이어그램 — Ch04 필수 구조 모두 포함.
- v0.3 → v0.4 변화 명확 (uart_driver 추가).
- Driver 레이어 분리 체크리스트 제공.

### SVG 다이어그램 품질: ✅ 양호 (M5 수정 필요)
- 색상 팔레트: #1A73E8(HAL), #34A853(Driver), #FBBC04(Service), #EA4335(App) — 정확 사용.
- 9개 SVG 모두 Noto Sans KR 14px 기준 준수.
- UART 프레임 다이어그램: 0x41('A')의 LSB-first 비트 배열 정확 (D0=1, D1=0, ..., D6=1, D7=0).
- 함수명 대소문자 불일치만 수정 필요 (M5).

### 학습 효과: ✅ 우수
- 편지 봉투, 호텔 모닝콜, 택배 기사 — 비유 3개 모두 기술적으로 정확하며 한계도 명시.
- 막힘 포인트(Baud Rate 불일치, 재등록 누락)에 FAQ/팁 즉시 배치.
- 감정 곡선: 호기심(SWO vs UART 차이) → 불안(폴링 블로킹) → 이해(인터럽트 해결) → 성취(Driver 분리 완성).

---

## 5. 수정 우선순위

| 우선순위 | ID | 내용 | 예상 작업량 |
|---------|-----|------|-----------|
| 필수 | C1 | DMA 챕터 번호 통일 | 5분 |
| 필수 | M4 | LED 토글 코드 누락 수정 | 1분 |
| 필수 | M5 | SVG 함수명 대소문자 수정 | 3분 |
| 권장 | C2 | const 캐스팅 설명 추가 | 2분 |
| 권장 | M2 | ORE 처리 aside 추가 | 5분 |
| 권장 | M3 | uart_read_byte() 0 모호성 주석 | 2분 |
| 선택 | M1 | BRR FAQ 추가 | 5분 |
| 선택 | S1~S3 | 미사용 변수, 중복 정의 명시 등 | 5분 |

**총 필수 수정 예상 시간: ~10분**

---

## 6. 최종 판정

**조건부 승인 (Conditional Approval)**

C1(챕터 번호 통일), M4(LED 토글 누락), M5(SVG 함수명)를 수정하면 출판 가능.
나머지 Major/Minor는 권장 사항이며, 기술 저자의 판단에 따라 반영.

전반적으로 HAL API 사용, 아키텍처 연결, 학습 설계 모두 높은 수준입니다.
"첫 Driver 레이어 실전"이라는 Ch04의 교육 목표를 충실히 달성했습니다.
