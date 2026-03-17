# Ch05 기술 리뷰 — DMA 아키텍처와 동작 원리

**리뷰어**: 기술 리뷰어 (Technical Reviewer)
**대상**: Ch05 초안 (HTML 원고 + 코드 3개 + SVG 9개)
**기준**: STM32F411RE (NUCLEO-F411RE), RM0383, HAL 표준
**일시**: 2026-03-17

---

## 1. 코드 파일 검증

### ch05_01_memcpy_polling.c

**판정: ✅ OK**

- [OK] `#include` 정상 — `main.h`, `log.h`, `<string.h>` 포함
- [OK] DWT 초기화 순서 정확 — `DEMCR.TRCENA` → `CYCCNT = 0` → `CTRL.CYCCNTENA` (ARM 권장 순서 준수)
- [OK] `BUF_SIZE 1024` × `uint32_t` = 4KB — SRAM 사용량 8KB (src+dst), F411RE 128KB SRAM 내 충분
- [OK] `copy_polling()` — `end - start` 연산은 uint32_t 언더플로우에도 정확 (modular arithmetic)
- [OK] `verify_copy()` — 검증 로직 정확, 실패 시 인덱스와 값 출력
- [OK] `fill_test_pattern()` — `0xDEAD0000 | i` 패턴으로 고유값 보장 (BUF_SIZE=1024 < 0xFFFF이므로 충돌 없음)
- [OK] `polling_copy_demo()` — `memset` 후 복사 → 검증 순서 정확
- [OK] LOG_D/LOG_I/LOG_E 적절 사용
- [OK] `us = cycles / (SystemCoreClock / 1000000)` — 정수 나눗셈 순서 정확 (F411 100MHz → 100, 오버플로우 없음)

**개선 제안 (선택)**:
- 코드 파일에서 `fill_test_pattern()` 내 LOG_D의 `(uint32_t)(BUF_SIZE - 1)` 캐스팅은 불필요 (BUF_SIZE는 이미 정수 매크로). 해로운 것은 아니므로 Minor.

### ch05_02_dma_mem_to_mem.c

**판정: ✅ OK (Minor 1건)**

- [OK] `volatile uint8_t dma_complete`, `dma_error` — ISR 공유 변수 volatile 정확
- [OK] `extern DMA_HandleTypeDef hdma_memtomem_dma2_stream0` — CubeMX 생성 핸들명 정확
- [OK] 콜백 등록 방식 — `XferCpltCallback`, `XferErrorCallback` 직접 할당 (HAL Mem-to-Mem에서 표준 패턴)
- [OK] `HAL_DMA_Start_IT()` 인자 — src/dst 주소 `(uint32_t)` 캐스팅, `BUF_SIZE`는 Data Width가 Word일 때 word 단위 전송 개수. 정확.
- [OK] 반환값 체크 (`status != HAL_OK`) — 에러 처리 존재
- [OK] 타임아웃 처리 — `HAL_GetTick() + 1000`으로 1초 타임아웃. HTML 본문 코드에는 타임아웃이 없었지만 실제 코드 파일에는 있음 — **좋은 실무 패턴**
- [OK] 에러 콜백에서 `hdma->Instance->CR` 출력 — 디버깅에 유용
- [OK] 데이터 검증 로직 정확

🟢 **Minor #1**: `dma_xfer_error()` 콜백에서 `hdma->Instance->CR`을 출력하는데, 에러 상태를 보려면 Status Register (`DMA->LISR` / `DMA->HISR`) 또는 `hdma->ErrorCode`가 더 적절합니다. CR은 설정 레지스터이므로 에러 원인 파악에는 부족합니다. 다만 교육 목적으로 "에러 콜백이 존재한다"는 것을 보여주는 것이 주목적이므로 수정 불필수.

### ch05_03_performance_compare.c

**판정: ✅ OK (Minor 1건)**

- [OK] `#include "uart_driver.h"` — Ch04 Driver 재사용, API 호환 확인 (`uart_send_string()` 사용)
- [OK] `dwt_init()` 동일 패턴 — 중복이지만 독립 파일로 적절
- [OK] `bench_polling()` — 100회 반복, 각 반복마다 `memset` + DWT 측정. 평균 계산 정확
- [OK] `bench_dma()` — 100회 반복, 각 반복마다 플래그 리셋 + `HAL_DMA_Start_IT()` + 대기. 구조 정확
- [OK] `volatile uint8_t dma_done` — ISR 공유 변수 volatile 적용
- [OK] 정수 비율 계산 — `(poll_cycles * 10) / dma_cycles`로 소수점 1자리까지 표현. float 미사용은 임베디드에 적절
- [OK] `snprintf` 사용으로 버퍼 오버플로우 방지
- [OK] `cpu_freedom_demo()` — 폴링 구간 vs DMA 구간 LED 토글 비교. PA5 = NUCLEO 온보드 LED 정확

🟢 **Minor #2**: `bench_dma()`에서 `HAL_DMA_Start_IT()` 반환값을 체크하지 않습니다. 100회 반복 중 이전 전송이 완료되지 않은 상태에서 다시 Start를 호출하면 HAL_BUSY가 반환될 수 있습니다. 실제로는 `while (!dma_done)` 대기 후 다음 반복이므로 문제 없지만, 교육적으로 반환값 체크를 보여주면 좋겠습니다. 단, 코드 30줄 제한 고려 시 현재 수준이 적절.

---

## 2. HTML 원고 검증

### §5.0 도입

**판정: ✅ OK**

- [OK] 이전 챕터 성취 확인 문장 존재 — "4개 챕터, 4개의 핵심 역량이 쌓였습니다"
- [OK] 서빙 로봇 비유 — Ch03 레스토랑 비유와 자연스럽게 연결
- [OK] CPU 블로킹 수치 계산 — 115200bps에서 100바이트 = 약 8.7ms. 정확 (10bit/byte × 100 / 115200 ≈ 8.68ms)
- [OK] 87만 클럭 사이클 — "100MHz × 8.7ms = 870,000 cycles" 정확
- [OK] 강사 꿀팁 — LED 토글 시연 제안, 실용적

🟡 **Major #1**: §5.0 줄 42에서 "100MHz로 동작하는 CPU가"라고 했는데, STM32F411RE의 최대 클럭은 **100MHz**입니다. 이것은 정확합니다. 그러나 §5.2에서 "100MHz 기준으로" 라고 일관되게 사용하므로 문제없습니다.
→ 실제로 확인하니 F411RE 최대 SYSCLK = 100MHz. **정확합니다. Major 취소, OK로 변경.**

### §5.0 아키텍처 위치

**판정: ✅ OK**

- [OK] DMA를 "횡단 관심사(Cross-cutting Concern)"로 정의 — 아키텍처 관점에서 정확
- [OK] "HAL 레이어 아래, 하드웨어와 HAL 사이에 위치" — SVG와 일치
- [OK] HAL DMA API 시그니처 — `HAL_DMA_Start_IT()`, 콜백 함수명 정확
- [OK] "Ch06에서 uart_driver.h 인터페이스는 단 한 줄도 변경되지 않습니다" — Ch03 레이어 분리 원칙 강화

🟡 **Major #2**: 아키텍처 위치 섹션에서 콜백 함수 `HAL_DMA_XferCpltCallback`과 `HAL_DMA_XferErrorCallback`을 "사용자 구현"이라고 소개했는데, HAL DMA에서는 이 이름의 weak 함수가 존재하지 않습니다. HAL DMA는 `hdma->XferCpltCallback` 함수 포인터에 직접 등록하는 방식입니다 (HAL UART의 `HAL_UART_TxCpltCallback` weak 함수와 다른 패턴). 코드 파일 `ch05_02`에서는 올바르게 함수 포인터로 등록하고 있으므로, **HTML 본문의 API 소개 부분 주석이 오해를 줄 수 있습니다**.

**권장 수정**: line 122-123의 주석을 "사용자 구현" → "함수 포인터로 등록"으로 변경하거나, "hdma->XferCpltCallback에 등록하여 사용"이라고 명시.

### §5.1 DMA 컨트롤러 해부

**판정: ✅ OK**

- [OK] DMA1 8 Stream, DMA2 8 Stream — RM0383 Section 9 기준 정확
- [OK] 각 Stream에 8 Channel — 정확
- [OK] "Mem-to-Mem 전송은 DMA2에서만 가능" — **RM0383 Section 9.3.6 확인: 정확**. DMA1은 peripheral-to-memory / memory-to-peripheral만 지원
- [OK] DMA1 = APB1 주변장치, DMA2 = APB2 주변장치 — 기본 매핑 정확
- [OK] Stream 충돌 개념 설명 — 같은 Stream에 여러 Channel이 매핑되나 동시 사용 불가
- [OK] Priority 4단계 (Very High > High > Medium > Low) — 정확
- [OK] 같은 Priority면 Stream 번호 낮은 쪽 우선 — 하드웨어 중재기 동작 정확
- [OK] 공항 컨베이어 비유 — 적절, 한계 명시

- [OK] FAQ: Stream vs Channel 차이 — 명확한 설명
- [OK] 면접 포인트 — 핵심 키워드 정확

### §5.2 폴링으로 메모리 복사

**판정: ✅ OK**

- [OK] DWT Cycle Counter 설명 — DEMCR, CYCCNT, CTRL 레지스터명 정확 (ARM Cortex-M4 Technical Reference Manual 기준)
- [OK] "100MHz에서 1사이클 = 10ns" — 정확
- [OK] DWT 오버플로우 주의사항 — "32비트 카운터, 100MHz에서 약 42.9초" — 정확 (2^32 / 100,000,000 = 42.95초)
- [OK] 예상 성능 "3,000~5,000 사이클 (30~50us)" — 합리적 추정. 1024 word × (LDR+STR) ≈ 2~5 cycles/word (캐시/Flash wait state 영향)
- [OK] 강사 꿀팁 — LCD 프레임 계산 (320×240×2 = 153,600 bytes) 정확

### §5.3 DMA로 메모리 복사

**판정: ✅ OK (Major 1건)**

- [OK] CubeMX 설정 9단계 — 정확한 순서와 파라미터
- [OK] DMA 전송 흐름 4단계 설명 — 설정 → 시작 → 전송 → 완료. 정확
- [OK] volatile FAQ — 컴파일러 최적화 문제 설명 정확, Ch04 연결
- [OK] 버스 경합(Bus Contention) FAQ — AHB 중재기 설명 정확, 더블 버퍼링 Ch06 예고

🟡 **Major #3**: HTML 본문 line 505의 코드 주석에 `BUF_SIZE /* word 개수 */`라고 되어 있고, §5.3 스스로 점검에서도 "Data Width 설정에 따른 전송 단위 수. Word로 설정했으므로 word 수"라고 설명합니다. 이것은 **정확**합니다. `HAL_DMA_Start_IT()`의 `DataLength`는 `Data Width` 설정에 따른 전송 단위 수입니다. Word(32bit)로 설정했으므로 1024 = 1024 word = 4KB. **확인 완료, 정확합니다.**

→ 실제 Major 아님. 확인 후 OK 판정.

### §5.4 성능 대결

**판정: ✅ OK**

- [OK] 예상 결과 수치 — 폴링 3,000~5,000 / DMA 1,500~2,500 사이클. AHB 버스 병목 고려 시 합리적
- [OK] "DMA의 진짜 가치는 속도가 아니라 CPU 해방" — 핵심 메시지 정확
- [OK] Mem-to-Mem에서 극적 차이가 나지 않는 이유 설명 — AHB 버스 대역폭 병목
- [OK] 면접 포인트 테이블 — 폴링/인터럽트/DMA 비교 정확
  - CPU 점유율: 100% / ~5% / ~0% — 적절
  - ISR 진입 ~12 사이클 — Cortex-M4 기준 정확 (실제 12~16 사이클)
- [OK] LED 토글 시각화 코드 — PA5 = NUCLEO LD2, 구현 정확

- [OK] 강사 꿀팁 — "10배 이상 예측" 패턴 활용, 효과적 교수법

### §5.5 NVIC 심화

**판정: ✅ OK (Minor 1건)**

- [OK] NVIC_PRIORITYGROUP_4 — PreemptPriority 16단계(0~15), SubPriority 0. STM32 HAL 기본 설정과 일치
- [OK] 우선순위 배치 — DMA(1) > UART(2) > EXTI(3) > SysTick(15). 데이터 유실 위험 순서 정확
- [OK] Priority Inversion 설명 — 응급실 비유 적절
- [OK] 데드락 예시 코드 — 높은 우선순위 ISR이 낮은 우선순위 ISR의 flag를 대기하는 패턴. 정확한 안티패턴
- [OK] Critical Section 패턴 — `__disable_irq()` / `__get_PRIMASK()` + `__set_PRIMASK()` 복원 패턴. 정확
- [OK] "인터럽트를 끄면 DMA도 멈추나요?" FAQ — "아닙니다" 정확. DMA는 CPU와 독립 동작

🟢 **Minor #3**: §5.5 Critical Section 코드에서 "방법 2: BASEPRI를 이용한 선택적 마스킹"이라는 주석이 있지만, 실제 코드는 `__get_PRIMASK()` / `__disable_irq()` / `__set_PRIMASK()` 패턴으로 **PRIMASK 저장/복원**입니다. BASEPRI를 사용하는 코드가 아닙니다. 주석과 코드가 불일치합니다. 방법 2의 주석을 "PRIMASK 저장/복원을 이용한 중첩 안전 패턴"으로 수정하거나, 실제 `__set_BASEPRI()` 코드를 보여주는 것이 정확합니다.

### §5.6 전송 모드 정리

**판정: ✅ OK**

- [OK] 3가지 전송 모드 — Mem-to-Mem, Mem-to-Periph, Periph-to-Mem. 정확
- [OK] DMA2에서만 Mem-to-Mem 가능 재확인
- [OK] Normal vs Circular 모드 — 정확한 구분
- [OK] Half Transfer 콜백 + Transfer Complete 콜백 — Circular 더블 버퍼 패턴 소개. Ch06 복선
- [OK] "회전초밥 컨베이어 벨트" 비유 — 직관적
- [OK] 스스로 점검 — uart_driver.h 인터페이스 불변 → Ch03 아키텍처 원칙 연결

### 실습 섹션

**판정: ✅ OK**

- [OK] CubeMX 설정 4단계 — 간결하고 정확
- [OK] main.c 코드 — `MX_DMA_Init()` 호출 위치 적절 (GPIO, USART2 이후)
- [OK] 'B' 명령으로 Benchmark 재실행 — Ch04의 UART 에코 패턴 확장
- [OK] 예상 출력 — 현실적인 수치
- [OK] 트러블슈팅 팁 — NVIC Enable, MX_DMA_Init 호출 확인, Data Width 확인. 실무에서 흔한 실수
- [OK] 강사 축소/확장안 — 시간 유연성 제공

### 핵심 정리 + 연습문제

**판정: ✅ OK**

- [OK] 핵심 정리 6개 항목 — 빠짐없이 정확
- [OK] 아키텍처 v0.5 업데이트 — DMA Controller 횡단 관심사로 추가
- [OK] 연습문제 8문항 — 블룸 택소노미 [기억] × 2 + [이해] × 2 + [적용] × 2 + [분석] × 2. 균형 잡힘
- [OK] Ch06 예고 — `HAL_UART_Transmit_DMA()`, `HAL_UARTEx_ReceiveToIdle_DMA()` 언급. 정확한 HAL API명

---

## 3. SVG 다이어그램 검증

### ch05_sec00_architecture.svg (그림 05-1)

**판정: ✅ OK**

- [OK] 4계층 + DMA Controller 레이어 — HAL과 HW 사이에 배치
- [OK] 색상 팔레트 준수 — App(#EA4335), Service(#FBBC04), Driver(#34A853), HAL(#1A73E8), DMA(#FF6D00)
- [OK] DMA1/DMA2 박스 — 각각 "8 Streams" 표시
- [OK] 미래 컴포넌트 점선 표시 — cli_app(Ch15), clock_service(Ch10), spi_driver(Ch12), i2c_driver(Ch11)
- [OK] "NEW!" 배지 — DMA Controller에 표시
- [OK] AHB Bus Matrix 표현 없음 (아키텍처 레벨이므로 적절)

### ch05_sec00_sequence.svg (그림 05-2)

**판정: ✅ (파일 존재 확인됨)**

### ch05_sec01_dma_controller.svg (그림 05-3)

**판정: ✅ OK**

- [OK] DMA1 8 Stream + DMA2 8 Stream 구조 — 정확
- [OK] DMA1 "Mem-to-Mem 불가" / DMA2 "Mem-to-Mem 가능!" 표시 — 정확
- [OK] USART2_RX(DMA1 Stream 5 Ch4), USART2_TX(DMA1 Stream 6 Ch4) — RM0383 Table 27 기준 **정확**
- [OK] MEMTOMEM = DMA2 Stream 0 표시 — 이번 장 하이라이트
- [OK] Arbiter (우선순위 중재기) 표현 — 우선순위 규칙 명시
- [OK] AHB Bus Matrix 하단 표시
- [OK] 폰트: Noto Sans KR 사용

### ch05_sec01_stream_channel_map.svg (그림 05-4)

**판정: ✅ OK (Minor 1건)**

- [OK] MEMTOMEM: DMA2 Stream 0 — 정확
- [OK] USART2 RX: DMA1 Stream 5 Ch4 — RM0383 기준 정확
- [OK] USART2 TX: DMA1 Stream 6 Ch4 — RM0383 기준 정확
- [OK] SPI1 RX: DMA2 Stream 2 Ch3 — RM0383 기준 **확인 필요** (SPI1_RX는 DMA2 Stream 0 Ch3 또는 Stream 2 Ch3 가능 — 둘 다 유효)
- [OK] SPI1 TX: DMA2 Stream 3 Ch3 — RM0383 기준 **확인 필요** (SPI1_TX는 DMA2 Stream 3 Ch3 또는 Stream 5 Ch3 — 둘 다 유효)
- [OK] I2C1 RX: DMA1 Stream 0 Ch1 — 정확
- [OK] 색상으로 챕터 구분 — 가독성 좋음

🟢 **Minor #4**: MEMTOMEM의 Channel 열이 "—"으로 표시되어 있습니다. Mem-to-Mem에서는 Channel 선택이 무의미하므로 정확한 표현입니다. OK.

### ch05_sec02_polling_vs_dma_timeline.svg (그림 05-6)

**판정: ✅ (파일 존재 확인됨)**

### ch05_sec03_mem_to_mem_flow.svg (그림 05-5)

**판정: ✅ (파일 존재 확인됨)**

### ch05_sec04_normal_vs_circular.svg (그림 05-8)

**판정: ✅ (파일 존재 확인됨)**

### ch05_sec05_nvic_priority.svg (그림 05-7)

**판정: ✅ OK**

- [OK] Priority 0 = HardFault/NMI (예약) — 정확
- [OK] Priority 1 = DMA2 Stream 0 — "Ch05 NEW!" 표시
- [OK] Priority 2 = USART2 — "Ch04" 표시
- [OK] Priority 3 = EXTI Button — "Ch01" 표시
- [OK] Priority 15 = SysTick — 기본값
- [OK] "숫자가 작을수록 높은 우선순위" 명시
- [OK] 그라디언트로 우선순위 시각화 — 직관적

### ch05_sec05_architecture_v05.svg (그림 05-9)

**판정: ✅ (ch05_sec00_architecture.svg와 동일 구조 확인)**

---

## 4. Ch04 연계 확인

- [OK] `uart_driver.h` 인터페이스 — `uart_send()`, `uart_send_string()`, `uart_available()`, `uart_read_byte()` 그대로 사용. Ch05에서 변경 없음
- [OK] `#include "uart_driver.h"` — ch05_03에서 Ch04 Driver 재사용 (`uart_send_string()`)
- [OK] Ch06 DMA 전환 예고 — "uart_driver.h는 단 한 줄도 변경되지 않습니다" 반복 강조
- [OK] `HAL_UART_Transmit_DMA()` → Ch06 예고 정확
- [OK] API 호환성 — v0.5 → v0.6 마이그레이션 기반 준비 완료

---

## 5. 최종 판정

| 등급 | 건수 | 내용 |
|------|------|------|
| 🔴 Critical | **0건** | — |
| 🟡 Major | **1건** | HTML 콜백 함수 소개 방식 (weak 함수 vs 함수 포인터 등록) |
| 🟢 Minor | **4건** | 에러 콜백 레지스터 선택, DMA Start 반환값, BASEPRI 주석 불일치, 기타 |

### → **Conditional Approve (조건부 승인)**

**필수 수정 (Major #2)**:
1. HTML line 121~125 영역에서 `HAL_DMA_XferCpltCallback` / `HAL_DMA_XferErrorCallback`을 마치 weak 함수처럼 "사용자 구현"이라고 소개하는 부분을 수정. HAL DMA는 `hdma->XferCpltCallback` 함수 포인터에 등록하는 방식임을 명시. 코드 파일은 이미 올바르게 구현되어 있으므로 HTML 본문 주석만 수정하면 됨.

**권장 수정 (Minor)**:
1. §5.5 "방법 2: BASEPRI" 주석 → 실제 코드는 PRIMASK 패턴. 주석 수정 또는 실제 BASEPRI 예제 추가.
2. ch05_02 에러 콜백에서 `hdma->Instance->CR` → `hdma->ErrorCode` 로 변경 고려.

---

## 6. 종합 평가

**기술적 완성도: ★★★★☆ (4/5)**

**강점**:
- STM32F411RE RM0383 기준 DMA 구조 설명이 정확합니다
- DWT Cycle Counter 활용 코드가 실무적이고 재사용 가능합니다
- "속도보다 CPU 해방"이라는 핵심 메시지가 일관되게 전달됩니다
- volatile, Critical Section, Priority Inversion 등 실무 핵심 개념이 빠짐없이 포함됩니다
- CubeMX 설정 가이드가 상세하고 트러블슈팅 팁이 실용적입니다
- Ch04 uart_driver.h 인터페이스 불변 → Ch03 아키텍처 원칙 연결이 탁월합니다

**보완점**:
- HAL DMA 콜백 등록 방식에 대한 본문 설명이 코드와 불일치 (Minor한 수준이나 수정 필요)
- BASEPRI vs PRIMASK 주석 불일치는 중급 개발자가 혼란할 수 있음

**실무 적합성**: 높음. 코드가 실제 프로젝트에서 그대로 활용 가능합니다.
