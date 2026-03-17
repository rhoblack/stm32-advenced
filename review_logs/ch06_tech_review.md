# Ch06 기술 리뷰 (Technical Review)

**리뷰어**: 기술_리뷰어
**완료일**: 2026-03-17
**상태**: 조건부 통과

## 📊 요약
- Critical: 2건
- Major: 3건
- Minor: 4건
- **최종 판정**: ⚠️ 조건부 통과 (Critical 2건 수정 필요)

---

## 🔍 상세 리뷰

### 1. 코드 정확성

#### ch06_01_ring_buffer.h/c
- **[PASS]** 구조체 설계: `volatile uint16_t head/tail` 올바름 — SPSC lock-free 패턴 준수
- **[PASS]** `ring_buf_write()`: `(head + 1) % size` full 판별 정확
- **[PASS]** `ring_buf_read()`: `head == tail` empty 판별 정확
- **[PASS]** `ring_buf_available()`: `(head - tail + size) % size` 수학적 정확 (uint16_t 언더플로우 시에도 모듈로 연산으로 보정됨)
- **[PASS]** `ring_buf_flush()`: head=tail=0 리셋 정확
- **[PASS]** 오버플로우 시 write 거부 (데이터 유실 방지), LOG_W 출력
- **[PASS]** 코드 스타일: 스네이크 케이스, 4칸 들여쓰기, 한국어 주석, LOG 매크로 적용
- **[PASS]** 각 공개 함수에 @brief/@param/@return 주석 완비

#### ch06_02_uart_rx_dma.c
- **[PASS]** `HAL_UARTEx_ReceiveToIdle_DMA(&huart2, s_dma_rx_buf, DMA_RX_BUF_SIZE)` — 파라미터 순서 정확 (huart, pData, Size)
- **[PASS]** `HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)` — 시그니처 정확 (HAL 표준)
- **[PASS]** `__HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT)` — Half-Transfer 비활성화 올바름
- **[PASS]** DMA Circular 순환 처리: `new_pos > old_pos` (연속) vs `new_pos < old_pos` (순환) 분기 정확
- **[PASS]** `s_rx_old_pos >= DMA_RX_BUF_SIZE` 시 0으로 리셋 — 경계 조건 처리 올바름
- **[PASS]** 콜백 내 블로킹 연산 없음 확인 (ring_buf_write + LOG_D만 사용)
- **[PASS]** CubeMX 주석: DMA1 Stream5 Ch4, Circular, Byte — F411RE RM0383 Table 28 정확

#### ch06_03_uart_tx_dma.c
- **[PASS]** TX FSM 설계: TX_IDLE/TX_BUSY 2-state, volatile 적용
- **[PASS]** `HAL_UART_Transmit_DMA(&huart2, s_tx_dma_buf, count)` — 파라미터 정확
- **[PASS]** `HAL_UART_TxCpltCallback()` 연쇄 전송 패턴 올바름
- **[PASS]** `tx_start_dma()` — 링 버퍼에서 최대 TX_DMA_CHUNK 바이트 추출, DMA 시작
- 🔴 **[Critical-1]** `uart_tx_dma_send()` (line 83~88): **Critical Section 누락**
  - `s_tx_state` 체크와 `tx_start_dma()` 호출 사이에 `__disable_irq()`/`__enable_irq()`가 없음
  - 실제 코드 파일에서는 line 84~88에서 `__disable_irq()` 적용되어 있으나, HTML §6.4의 인라인 코드 (line 748~764)에서는 Critical Section **없이** 표시됨
  - HTML 인라인 코드와 실제 파일의 불일치 — 독자가 HTML 코드를 따라 하면 race condition 발생 가능
- **[PASS]** `_write()` printf 리다이렉트: 반환값 `len` 정확

#### ch06_04_uart_driver_v2.h (인터페이스 불변성 검증)
- **[PASS]** include guard: `UART_DRIVER_H` — Ch04와 동일
- **[PASS]** `uart_status_t` enum: UART_OK=0, UART_ERROR=1, UART_BUSY=2, UART_TIMEOUT=3 — Ch04와 **완전 동일**
- **[PASS]** 함수 시그니처 5개 모두 Ch04와 **완전 동일**:
  - `void uart_driver_init(void)`
  - `uart_status_t uart_send(const uint8_t *buf, uint16_t len)`
  - `uart_status_t uart_send_string(const char *str)`
  - `uint16_t uart_available(void)`
  - `uint8_t uart_read_byte(void)`
- **[PASS]** 주석만 변경 (v0.4 → v0.6, DMA 전환 언급) — 기능적 변경 없음
- **[PASS]** "내부 DMA, 외부는 같은 API" 원칙 완벽 준수

#### ch06_04_uart_driver_v2.c
- **[PASS]** RX 구현: ch06_02와 동일 패턴 (Circular DMA + IDLE + ring buffer)
- **[PASS]** TX 구현: ch06_03와 동일 패턴 (FSM + ring buffer + DMA chaining)
- **[PASS]** `uart_send()` line 114~119: `__disable_irq()`/`__enable_irq()` Critical Section 올바르게 적용
- **[PASS]** `uart_send()` line 101~104: NULL/0 파라미터 검증 — Ch04보다 방어적
- **[PASS]** `HAL_UARTEx_RxEventCallback()` + `HAL_UART_TxCpltCallback()` 두 콜백 통합 올바름
- **[PASS]** `_write()` printf 리다이렉트: `uart_send()` 사용 (반환값 무시하지 않음)
- **[PASS]** CubeMX 설정 주석: RX Stream5 Circular, TX Stream6 Normal — 정확
- **[PASS]** 모든 HAL include 경로 유효 (main.h, log.h)

#### ch06_05_performance_test.c
- **[PASS]** DWT CYCCNT 초기화: `CoreDebug->DEMCR` + `DWT->CTRL` 올바른 비트 마스크
- **[PASS]** 폴링 측정: `HAL_UART_Transmit(&huart2, test_buf, TEST_SIZE, 100)` — timeout 100ms 적절
- **[PASS]** DMA 측정: `uart_send(test_buf, TEST_SIZE)` — v0.6 API 사용
- 🟡 **[Major-1]** 인터럽트 방식 테스트 누락: HTML §6.6에서는 "3-way 비교" (폴링/인터럽트/DMA)를 언급하고 인라인 코드에 `HAL_UART_Transmit_IT` 예시가 있지만, 실제 코드 파일 `ch06_05_performance_test.c`에는 **폴링과 DMA만 측정** (2-way). 인터럽트 측정 코드가 빠져 있음.
- **[PASS]** 주석의 main() 예시: `MX_DMA_Init()` → `MX_USART2_UART_Init()` 순서 올바름 (DMA 먼저!)

### 2. HTML 기술 설명

#### §6.0 도입
- **[PASS]** "블로킹 호출", "8.7ms" 계산: 100바이트 x 10비트/바이트 / 115200 = ~8.68ms 정확
- **[PASS]** "11,520번의 인터럽트": 115200 / 10비트 = 11,520 바이트/초 정확
- **[PASS]** v0.5 → v0.6 진화 스토리라인 일관성

#### §6.1 UART DMA 아키텍처
- **[PASS]** DMA 매핑: USART2_RX = DMA1 Stream5 Ch4, TX = DMA1 Stream6 Ch4 — RM0383 Table 28 기준 정확
- **[PASS]** Circular vs Normal 설명 정확
- **[PASS]** IDLE Line 감지: "1프레임(약 87us @115200bps)" — 1/115200 x 10비트 ≈ 86.8us 정확
- **[PASS]** `HAL_UARTEx_ReceiveToIdle_DMA()` vs `HAL_UART_Receive_DMA()` 차이 설명 정확
- **[PASS]** 비유: "반복 재생(Repeat All)" + 한계점 명시 — 교육적으로 우수

#### §6.2a 링 버퍼 개념
- **[PASS]** head/tail 정의: head=쓰기(생산자), tail=읽기(소비자) — 정확
- **[PASS]** 빈 상태: `head == tail`, 가득 참: `(head+1) % size == tail` — 정확
- **[PASS]** "size - 1개 저장 가능" 설명 정확
- **[PASS]** 모듈로 연산 시계 비유 — 수학적 정확

#### §6.2b 링 버퍼 구현
- **[PASS]** 의사코드 → C 코드 전환 — 3단계 스캐폴딩 우수
- **[PASS]** `volatile` 필요성 설명 정확 (컴파일러 최적화 방지)
- **[PASS]** SW 오버플로우 vs HW ORE 표 — 원인/감지/해결 모두 정확
- **[PASS]** 2의 거듭제곱 버퍼 크기 FAQ: `& (size-1)` 비트마스크 최적화 설명 정확

#### §6.3 RX DMA 구현
- **[PASS]** 3단계 흐름 (DMA 기록 → 콜백 복사 → 메인 읽기) 정확
- **[PASS]** CubeMX 설정: RX Circular, TX Normal — 정확 (역방향 설정 시 무한 송신 버그 경고 포함)
- **[PASS]** Half-Transfer 비활성화 이유 + 고속 통신 시 활용 언급 — 실무적

#### §6.4 TX DMA 구현
- 🔴 **[Critical-1 재확인]** HTML 인라인 코드 (line 748~764)에서 `uart_tx_dma_send()` 함수 내부에 Critical Section (`__disable_irq()`/`__enable_irq()`)이 **빠져 있음**. 실제 파일(ch06_03_uart_tx_dma.c)에는 line 84~88에 올바르게 적용되어 있지만, HTML 원고의 코드가 불완전함.
  - 독자 혼동 위험: §6.5에서 Critical Section을 설명하면서 "uart_send()에서 보호한다"고 했지만, §6.4의 코드에서는 보호가 없는 상태로 제시됨
- **[PASS]** TX FSM 설명 (IDLE/BUSY) 정확
- **[PASS]** 콜백 연쇄 패턴 설명 정확
- **[PASS]** printf DMA 리다이렉트 설명 정확

#### §6.5 통합 드라이버
- **[PASS]** 헤더 불변 원칙 — Ch04 대비 완전 동일 확인
- **[PASS]** SPSC lock-free 설명 정확 (RX: ISR→head, main→tail / TX: main→head, ISR→tail)
- **[PASS]** Critical Section 필요 위치 + `__disable_irq()` 사용법 정확

#### §6.6 실습 + 정리
- 🔴 **[Critical-2]** HTML에서 3-way 비교 (폴링/인터럽트/DMA)를 인라인 코드로 제시하지만, 실제 코드 파일은 2-way (폴링/DMA)만 구현. HTML과 코드 파일 불일치.
  - HTML line 962~965에 `HAL_UART_Transmit_IT` 예시가 있지만, ch06_05_performance_test.c에는 인터럽트 측정이 없음
  - 또한 HTML 결과 테이블에는 인터럽트 행(~10,000 cycles, 1%)이 있으나 코드에서 측정하지 않음
- **[PASS]** v0.3 → v0.6 아키텍처 진화 표 정확
- **[PASS]** 연습문제 6개: 블룸 분류 기억→이해→적용→분석→평가 올바르게 배치
- 🟡 **[Major-2]** 연습문제 3번 검증: head=250, tail=10, size=256일 때 `(250-10+256)%256 = 496%256 = 240`. 답: 240바이트. 이 값이 size-1=255보다 작으므로 유효. 다만 문제 자체에서 "계산하시오"만 있고 기대 답을 제공하지 않는 것은 정상 (연습문제이므로).

### 3. SVG 다이어그램

#### ch06_sec00_architecture.svg
- **[PASS]** 4계층(App/Service/Driver/HAL) 구조 정확
- **[PASS]** uart_driver v0.6 DMA 주황 강조 표시
- **[PASS]** DMA 횡단 영역: Stream5(RX) + Stream6(TX) 표시 정확
- **[PASS]** 의존 화살표: main() → uart_driver → HAL → DMA 올바름
- **[PASS]** 컬러 팔레트: HAL 파랑, Driver 초록, App 빨강 — CLAUDE.md 기준 준수

#### ch06_sec00_sequence.svg
- **[PASS]** 5개 참여자: App, uart_driver, ring_buffer, HAL, DMA+HW — 올바른 레이어 구분
- **[PASS]** TX 시퀀스: send → ring_buf_write → Transmit_DMA → TxCpltCallback 올바름
- **[PASS]** RX 시퀀스: DMA 자동 수신 → IDLE 감지 → RxEventCallback → ring_buf_write → read_byte → ring_buf_read 올바름

#### ch06_sec01_dma_uart_mapping.svg
- **[PASS]** DMA1 컨트롤러 내 Stream5 (RX, Circular) + Stream6 (TX, Normal) 정확
- **[PASS]** Channel 4 할당 — RM0383 기준 정확
- **[PASS]** USART2: 115200-8-N-1, PA2(TX), PA3(RX) 핀 배치 정확
- **[PASS]** Periph→Mem (RX), Mem→Periph (TX) 방향 화살표 정확
- **[PASS]** SRAM 128KB 표시 — F411RE 기준 정확

#### ch06_sec02_ring_buffer.svg
- **[PASS]** 4단계 시각화: ① 초기(head=tail=0) → ② 3바이트 쓰기(head=3,tail=0) → ③ 2바이트 읽기(head=3,tail=2) → ④ 순환(head=1,tail=5)
- **[PASS]** head(초록)/tail(파랑) 화살표 방향 일관성
- **[PASS]** 원형 시각화: size=8 원형 배열, 슬롯 0~7 정확 배치
- **[PASS]** 순환 슬롯(0번) 주황 색상으로 강조
- 🟢 **[Minor-1]** Stage ④에서 available=4 표기: head=1, tail=5, size=8 → (1-5+8)%8 = 4. **정확함**. 다만 슬롯 0에 데이터 "H"로 표시되었는데, 이것은 실제 데이터 값이 아니라 head 위치를 나타내는 것으로 혼동 가능. "H" 대신 데이터 문자(예: "I")가 더 명확함.

#### ch06_sec03_circular_dma_timeline.svg
- **[PASS]** 4개 레인(UART, DMA, ISR, 메인 루프) 시간순 배치
- **[PASS]** IDLE 감지 → 콜백 → 링 버퍼 저장 → 메인 읽기 순서 올바름
- **[PASS]** IDLE 감지 시점에 수직 점선 — 시각적으로 명확
- **[PASS]** ISR 콜백 짧은 박스 (블로킹 없음 암시)

#### ch06_sec04_tx_dma_fsm.svg
- **[PASS]** TX_IDLE → TX_BUSY: `uart_send()` 호출 시 / 링 버퍼 저장 + DMA 시작
- **[PASS]** TX_BUSY → TX_IDLE: `TxCpltCallback` + 링 버퍼 비어있음
- **[PASS]** TX_BUSY → TX_BUSY (자기 전이): TxCplt + 남은 데이터 → 연쇄 전송
- **[PASS]** TX_BUSY에서 uart_send() → 큐잉만 (점선으로 표시)
- **[PASS]** 초기 화살표 → TX_IDLE (init)

#### ch06_sec05_architecture_v06.svg
- **[PASS]** v0.5 대비 변경점 3가지 박스: ① DMA 전환 ② ring_buffer 신규 ③ DMA Stream UART 연결
- **[PASS]** ring_buffer를 Driver 레이어 내 독립 유틸리티로 배치 — 아키텍처적으로 올바름
- **[PASS]** perf_test를 App 레이어에 v0.6 NEW로 표시
- **[PASS]** DMA 횡단 영역: "DMA1 Stream5(USART2_RX, Circular) + Stream6(USART2_TX, Normal)" 정확
- 🟢 **[Minor-2]** ch06_sec05에 ili9341_driver가 없음 (ch06_sec00에는 있음) — 두 SVG 간 미래 드라이버 목록 미세 차이. 실질적 영향 없음.

### 4. 표준 준수 확인

- **[PASS]** HAL 사용 순서: DMA Init → UART Init → driver_init 주석 올바름
- **[PASS]** 코드 스타일: 4칸 들여쓰기, 스네이크 케이스 일관됨
- **[PASS]** 한국어 주석 모든 코드 파일에 포함
- **[PASS]** LOG_D/I/W 매크로 일관 적용 (LOG_E는 에러 경로 없어 미사용 — 정상)
- **[PASS]** 코드 블록 30줄 이하 권장 — 모든 인라인 코드 준수
- 🟡 **[Major-3]** HTML §6.3 인라인 코드 (line 570~571)에서 `#include "uart_driver.h"`, `#include "ring_buffer.h"` 사용하지만, 실제 파일(ch06_02_uart_rx_dma.c)에서는 `#include "ch06_01_ring_buffer.h"`, `#include "main.h"` 사용. 인클루드 경로가 다름. HTML은 "프로젝트 적용 시 이름"을, 파일은 "교재 예제 이름"을 사용하는 것으로 보이나, 독자에게 혼동 유발 가능. §6.4도 동일 패턴.
- 🟢 **[Minor-3]** HTML 내 코드 블록에서 `&lt;stdint.h&gt;` HTML 엔티티 올바르게 사용 — 렌더링 정상
- 🟢 **[Minor-4]** §6.2a "스스로 점검" — "스스로"가 아니라 "스스로" 표기 (전 챕터 동일 패턴이므로 의도적 표기로 판단)

---

## 📋 이슈 요약

### 🔴 Critical (반드시 수정)

| # | 위치 | 내용 |
|---|------|------|
| C-1 | HTML §6.4, line 748~764 | `uart_tx_dma_send()` 인라인 코드에 Critical Section (`__disable_irq()`/`__enable_irq()`) 누락. 실제 파일(ch06_03_uart_tx_dma.c)에는 있으나 HTML에 없음. 독자가 HTML을 따라하면 race condition 발생. |
| C-2 | HTML §6.6, line 955~971 vs ch06_05_performance_test.c | HTML은 3-way 비교(폴링/인터럽트/DMA) 제시, 실제 코드 파일은 2-way(폴링/DMA)만 구현. 인터럽트 측정 코드 누락. |

### 🟡 Major (권장 수정)

| # | 위치 | 내용 |
|---|------|------|
| M-1 | ch06_05_performance_test.c | 인터럽트 방식 측정 코드 추가 필요 (HTML과 일치시키기 위해) |
| M-2 | 연습문제 3번 | 문제 자체는 정확하나 해당 없음 (정상) |
| M-3 | HTML §6.3~6.4 인라인 코드 | include 경로가 실제 파일과 다름 (`ring_buffer.h` vs `ch06_01_ring_buffer.h`). 독자 혼동 유발 가능. 일관성 확보 필요. |

### 🟢 Minor (선택 수정)

| # | 위치 | 내용 |
|---|------|------|
| m-1 | ch06_sec02_ring_buffer.svg Stage④ | 슬롯0 데이터 "H" → 실제 데이터 문자로 변경 권장 |
| m-2 | ch06_sec05 vs ch06_sec00 SVG | 미래 드라이버 목록 미세 차이 (ili9341 유무) |
| m-3 | HTML §6.2b | `&lt;stdint.h&gt;` 엔티티 렌더링 정상 |
| m-4 | HTML 전체 | "스스로 점검" 표기 — 전 챕터 동일 패턴 |

---

## ✅ 최종 결론

### 긍정 평가
1. **Ring Buffer 구현 완벽**: SPSC lock-free 패턴, volatile, 모듈로 연산 모두 정확
2. **DMA 매핑 정확**: F411RE USART2 = DMA1 Stream5(RX)/Stream6(TX), Ch4 — RM0383 일치
3. **HAL API 사용 정확**: `HAL_UARTEx_ReceiveToIdle_DMA()`, `HAL_UART_Transmit_DMA()` 파라미터 및 콜백 시그니처 모두 정확
4. **Ch04 인터페이스 불변성 완벽**: `uart_driver_v2.h`와 `ch04_04_uart_driver.h`의 함수 시그니처, 타입, 매크로 100% 동일 확인
5. **SVG 7개 모두 기술적 정확**: DMA 매핑, 링 버퍼 동작, 타이밍, FSM 상태 전이 모두 올바름
6. **Race Condition 대응 우수**: SPSC + volatile + Critical Section 3단계 방어

### 수정 필요 사항
1. **🔴 Critical-1**: HTML §6.4 인라인 코드에 Critical Section 추가 (실제 파일과 일치)
2. **🔴 Critical-2**: ch06_05_performance_test.c에 인터럽트 측정 코드 추가 OR HTML에서 3-way → 2-way로 수정
3. **🟡 Major-3**: HTML 인라인 코드의 include 경로를 실제 파일과 일치시키거나, "프로젝트 적용 시 이름"임을 주석으로 명시

### 판정
- **Critical 2건 수정 후 통과 가능**
- Ring Buffer 알고리즘, DMA 매핑, HAL API, 인터페이스 불변성 등 핵심 기술 요소는 **모두 정확**
- SVG 다이어그램의 기술적 정확성 **우수**
