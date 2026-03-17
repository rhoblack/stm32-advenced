# Ch06 기술 저자 기획안

**작성자**: 기술 저자 (Technical Author)
**완료일**: 2026-03-17
**상태**: Phase 1 기획 완료
**챕터**: Ch06. UART DMA 심화 + 링 버퍼
**프로젝트 버전**: v0.5 → v0.6
**누적 기능**: UART → DMA 비동기 전환 + 링 버퍼 로그 출력 완성
**선행 지식**: Ch01~Ch05 전체 (특히 Ch04 UART Driver, Ch05 DMA 원리)
**예상 분량**: 7개 절 × 약 2,500자 = ~17,500자
**강의 시간**: 4시간 (이론 1.5h + 실습 2h + 정리 0.5h)

---

## 1. 학습 목표 (6개, Bloom 분류체계)

- [ ] **목표 1 (기억)**: DMA를 사용한 UART 수신/송신 프로세스(HAL_UART_Transmit_DMA, HAL_UARTEx_ReceiveToIdle_DMA)의 호출 순서와 콜백 흐름을 설명할 수 있다
- [ ] **목표 2 (이해)**: Ch04의 uart_driver.h 인터페이스가 폴링/인터럽트/DMA 중 어느 것을 써도 변경 없이 유지되는 이유를 이해한다 — 레이어드 아키텍처의 캡슐화 원칙 체감
- [ ] **목표 3 (적용)**: Ch05에서 학습한 Mem-to-Mem DMA 콜백 패턴(XferCpltCallback)을 UART DMA 콜백(HAL_UART_TxCpltCallback, HAL_UART_RxCpltCallback)으로 확장 적용할 수 있다
- [ ] **목표 4 (분석)**: 링 버퍼의 head/tail 인덱스 모듈로 연산에서 발생할 수 있는 오버플로우 조건과 빈/가득 참 판별을 분석할 수 있다
- [ ] **목표 5 (평가)**: 주어진 시나리오(데이터 크기, 빈도, CPU 부하)에서 폴링/인터럽트/DMA 중 최적 전략을 판단하고 근거를 제시할 수 있다
- [ ] **목표 6 (창조)**: RX 순환 DMA + TX DMA 큐를 결합한 양방향 비동기 UART 통신 구조를 설계하고 구현할 수 있다

---

## 2. 섹션 구조 (7개)

| § | 주제 | 분량 | 예상 시간 | 핵심 개념 (≤3) |
|---|------|------|-----------|----------------|
| 6.0 | 도입 + 아키텍처 위치 (v0.5→v0.6) | ~1,500자 | 15분 | 아키텍처 연속성, Driver 내부 교체 |
| 6.1 | UART DMA 동작 원리: Stream 매핑과 시퀀스 | ~2,500자 | 30분 | DMA Stream/Channel 매핑, IDLE Line 감지 |
| 6.2 | 링 버퍼 설계: 원형 구조와 인덱스 연산 | ~3,000자 | 40분 | 모듈로 연산, 빈/가득 참 판별, 스레드 안전성 |
| 6.3 | UART RX DMA 구현: 순환 DMA + IDLE 감지 | ~2,500자 | 40분 | Circular DMA, HAL_UARTEx_ReceiveToIdle_DMA |
| 6.4 | UART TX DMA 구현: 논블로킹 송신 | ~2,000자 | 35분 | TX 완료 콜백, 더블 버퍼링 힌트 |
| 6.5 | 통합 드라이버: uart_driver v2 (DMA 기반) | ~3,000자 | 35분 | 인터페이스 불변, 내부 구현 교체 |
| 6.6 | 실습 + 성능 측정 + 정리 | ~3,000자 | 45분 | CPU 점유율 비교, Ch07 예고 |

**합계**: ~17,500자, 4시간

---

## 3. 핵심 개념 (3개, 전 섹션 공통)

### 3.1 링 버퍼 (Ring Buffer / Circular Buffer)
- **정의**: 고정 크기 배열을 원형으로 사용하는 FIFO 데이터 구조
- **핵심 연산**: `head = (head + 1) % SIZE`, `tail = (tail + 1) % SIZE`
- **빈/가득 판별**: 1슬롯 희생 방식 (`(head + 1) % SIZE == tail` → 가득 참)
- **Ch04 연계**: ch04_04_uart_driver.c에서 이미 간이 링 버퍼 사용 (s_rx_buf, 109행 "Ch06 링 버퍼로 개선" 주석) → 이를 정식 구현으로 업그레이드
- **스레드 안전성**: ISR과 메인 루프 간 단일 생산자-단일 소비자(SPSC) 패턴에서 volatile + 배리어로 lock-free 구현

### 3.2 순환 DMA (Circular DMA)
- **Ch05 연계**: §5.4에서 Normal vs Circular 개념 소개 완료 → 이번 챕터에서 실전 적용
- **동작**: DMA 전송 완료 시 자동으로 처음부터 재시작 (하드웨어가 자동 처리)
- **UART RX 적용**: DMA가 수신 버퍼를 순환하며 채우고, IDLE Line 인터럽트로 "현재까지 수신된 위치" 파악
- **HAL 함수**: `HAL_UARTEx_ReceiveToIdle_DMA()` — IDLE 감지 시 콜백 호출, 수신 길이 전달
- **Half Transfer 콜백**: 버퍼 절반 채워졌을 때 콜백 → 처리 시간 확보

### 3.3 양방향 DMA 통신 구조
- **TX**: 송신 요청 → 링 버퍼 큐잉 → DMA 전송 시작 → 완료 콜백에서 다음 전송
- **RX**: 순환 DMA가 백그라운드 수신 → IDLE/HT 콜백에서 링 버퍼로 이동 → App 폴링
- **FSM**: `TX_IDLE` → `TX_BUSY` (DMA 전송 중) → `TX_IDLE` (완료 콜백)
- **인터페이스 불변**: uart_driver.h의 `uart_send()`, `uart_read_byte()` 시그니처 변경 없음

---

## 4. 코드 예제 (5개)

### 4.1 `ch06_01_ring_buffer.c` + `ch06_01_ring_buffer.h` (~60줄)
- 링 버퍼 순수 구현: `ring_buf_init()`, `ring_buf_write()`, `ring_buf_read()`, `ring_buf_available()`, `ring_buf_is_full()`
- typedef struct로 캡슐화 (head, tail, buf, size)
- 단위 테스트 성격의 main() 포함 — 넣고 빼고 가득 참/비어 있음 검증
- LOG_I로 각 단계 출력
- **위치**: §6.2에서 등장, 이후 §6.5에서 Driver에 통합

### 4.2 `ch06_02_uart_rx_dma_circular.c` (~50줄)
- CubeMX 설정: USART2 RX → DMA1 Stream5 Channel4, Circular 모드
- `HAL_UARTEx_ReceiveToIdle_DMA()` 호출
- `HAL_UARTEx_RxEventCallback()` 콜백에서 수신 데이터 + 길이 출력
- Half Transfer 인터럽트 비활성화: `__HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT)`
- **포인트**: Ch04의 1바이트씩 수신 → 가변 길이 수신으로 진화

### 4.3 `ch06_03_uart_tx_dma.c` (~40줄)
- `HAL_UART_Transmit_DMA()` 사용한 논블로킹 송신
- `HAL_UART_TxCpltCallback()`에서 완료 플래그 설정
- 전송 중 LED 토글로 CPU 비점유 확인 (Ch05 패턴 재사용)
- printf 리다이렉트를 DMA 기반으로 전환

### 4.4 `ch06_04_uart_driver_v2.c` + `ch06_04_uart_driver_v2.h` (~120줄)
- **핵심 파일**: Ch04의 uart_driver를 DMA + 링 버퍼로 전면 교체
- **헤더 변경 없음**: `uart_driver.h` 인터페이스 100% 동일 유지 (→ 캡슐화 증명)
  - `uart_driver_init()`, `uart_send()`, `uart_send_string()`, `uart_available()`, `uart_read_byte()`
- **내부 구조**:
  - TX: 링 버퍼 큐 + DMA 전송 + 완료 콜백에서 다음 전송
  - RX: 순환 DMA + IDLE 감지 + 링 버퍼 적재
  - FSM: `s_tx_state` (TX_IDLE / TX_BUSY)
- **Ch02 통합**: 링 버퍼 기반 비동기 로그 출력 — LOG_I가 블로킹 없이 동작

### 4.5 `ch06_05_performance_test.c` (~50줄)
- Ch04 인터럽트 드라이버 vs Ch06 DMA 드라이버 성능 비교
- 1KB 데이터 송신: CPU 점유 시간 측정 (DWT Cycle Counter, Ch05 기법 재사용)
- 수신 처리량: 115200bps 연속 수신 시 드롭률 비교
- 결과를 테이블로 UART 출력

### 코드 설계 원칙
- 모든 코드에 `LOG_D`/`LOG_I`/`LOG_W`/`LOG_E` 적용 (Ch02 표준)
- HAL 표준 준수, 들여쓰기 4칸, 스네이크 케이스, 한국어 주석
- 코드 블록 30줄 이하 (분할 설명)
- `#include "uart_driver.h"` — 상위 레이어는 HAL 헤더 직접 참조 금지

---

## 5. SVG 다이어그램 (7개)

| # | 파일명 | 내용 | 위치 |
|---|--------|------|------|
| 1 | `ch06_sec00_architecture.svg` | v0.6 아키텍처 — Driver 내부 DMA 강조 (uart_driver ↔ DMA Controller 연결선 추가) | §6.0 |
| 2 | `ch06_sec00_sequence.svg` | App → uart_driver → HAL_UART_*_DMA → DMA Controller → USART2 → 콜백 시퀀스 | §6.0 |
| 3 | `ch06_sec01_dma_stream_mapping.svg` | F411RE USART2 DMA 매핑: TX=DMA1_Stream6_Ch4, RX=DMA1_Stream5_Ch4 (Ch05 매핑 테이블에서 UART 부분 확대) | §6.1 |
| 4 | `ch06_sec02_ring_buffer.svg` | 링 버퍼 head/tail 4단계 진행 그림 (비어있음→쓰기→읽기→가득참) | §6.2 |
| 5 | `ch06_sec03_circular_dma_timeline.svg` | 순환 DMA 타임라인: HT콜백→처리→TC콜백→처리→자동재시작 + CPU 자유 구간 표시 | §6.3 |
| 6 | `ch06_sec04_tx_fsm.svg` | TX FSM 상태 전이: TX_IDLE —(send 요청)→ TX_BUSY —(DMA 완료 콜백)→ TX_IDLE | §6.4 |
| 7 | `ch06_sec05_architecture_v06.svg` | v0.6 최종 아키텍처: App → Service → Driver(uart_driver_v2 + ring_buf) → HAL → DMA → HW | §6.5 |

**색상 팔레트**: HAL(#1A73E8 파랑), Driver(#34A853 초록), DMA(#FF6D00 주황), Service(#FBBC04 노랑), App(#EA4335 빨강)
**폰트**: Noto Sans KR, 14px 기준
**규칙**: ASCII art 절대 금지 — SVG만 사용

---

## 6. 비유 및 예시

| 개념 | 비유 | 근거 |
|------|------|------|
| 링 버퍼 | **회전초밥 컨베이어 벨트** — 접시가 끝에 도달하면 처음으로 돌아옴. 손님(tail)이 먹는 속도보다 셰프(head)가 올리는 속도가 빠르면 가득 참 | Ch05 §5.4에서 "회전초밥" 비유로 Circular 모드 소개 → 연속성 유지 |
| 순환 DMA | **음악 플레이리스트 반복 재생** — 마지막 곡이 끝나면 자동으로 첫 곡부터 재시작. 사용자(CPU)는 별도 조작 불필요 | 자동 재시작 특성 직관적 전달 |
| TX DMA 큐 | **편의점 택배 접수** — 보내고 싶은 물건(데이터)을 접수대(링 버퍼)에 맡기면, 택배 기사(DMA)가 순서대로 배달. 접수만 하면 바로 다른 일 가능 | 논블로킹 송신의 핵심 가치 전달 |
| IDLE Line 감지 | **전화 통화의 "여보세요?" 후 침묵** — 상대방이 말을 멈추면(IDLE) "아, 한 문장 끝났구나" 인식. 정확한 길이를 미리 모르는 수신에서 메시지 경계 파악 | 가변 길이 수신의 핵심 메커니즘 |
| 인터페이스 불변 | **자동차 핸들** — 엔진을 가솔린에서 전기로 바꿔도 핸들 조작은 동일. uart_driver.h = 핸들, 내부 DMA 전환 = 엔진 교체 | Ch03 캡슐화 원칙 강화 |

---

## 7. 누적 성장 계획

### Ch04와 연계 (Driver 레이어 업그레이드)
- ch04_04_uart_driver.c의 **인터럽트 기반 구현을 DMA로 교체**
- uart_driver.h 인터페이스 변경 없음 → Ch03 캡슐화 원칙 실증
- Ch04 109행 주석 "Ch06 링 버퍼로 개선" → 이번 챕터에서 해소
- Ch04의 간이 링 버퍼(128바이트, 1바이트 단위) → 정식 ring_buf 모듈(256/512바이트, 가변 블록)

### Ch05 기초 활용 (DMA 지식 적용)
- Mem-to-Mem 콜백 패턴 → UART DMA 콜백으로 자연스러운 확장
- Normal vs Circular 모드: Ch05에서 개념 학습 → Ch06에서 Circular 실전 적용
- DWT Cycle Counter: Ch05 성능 측정 기법을 Ch06 비교 테스트에 재사용
- DMA Stream/Channel 매핑: Ch05 §5.1의 전체 매핑에서 UART 부분만 확대

### Ch02 업그레이드 (비동기 로그 완성)
- Ch02 TABLE_OF_CONTENTS 주석: "링 버퍼 기반 비동기 로그는 Ch06 이후 적용"
- DMA TX + 링 버퍼로 LOG_I/LOG_D가 **블로킹 없이 동작**하도록 개선
- 이후 모든 챕터에서 이 비동기 로그 인프라 활용

### Ch07 예고 (GP Timer + Service 레이어)
- "링 버퍼는 Ch07에서 타이머 이벤트 큐잉에도 활용됩니다"
- "DMA 기반 UART가 완성되었으니, 이제 Service 레이어를 추가할 준비가 되었습니다"
- ring_buf 모듈이 범용으로 설계되어 모터 인코더, 센서 데이터에도 재사용 가능

---

## 8. aside 박스 배치 계획

| 유형 | 개수 | 주요 내용 |
|------|------|-----------|
| 💡 실무 팁 | 3개 | ① IDLE Line 감지가 실무에서 가장 많이 쓰이는 이유 ② 링 버퍼 크기는 2의 거듭제곱으로 (비트마스크 최적화) ③ DMA 전송 중 버퍼 수정 금지 (캐시 일관성) |
| ❓ 단골 질문 | 4개 | ① "HAL_UART_Receive_IT vs HAL_UARTEx_ReceiveToIdle_DMA 차이?" ② "링 버퍼에 mutex가 필요한가요?" (SPSC는 불필요) ③ "DMA 채널 충돌이 나면 어떻게?" ④ "printf를 DMA로 바꾸면 순서가 보장되나요?" |
| 🎯 면접 포인트 | 2개 | ① 폴링/인터럽트/DMA 3종 비교 최종판 (Ch04 연습문제 완성) ② "링 버퍼에서 lock-free가 가능한 조건은?" |
| 📌 강사 꿀팁 | 3개 | ① "Ch04 코드와 Ch06 코드를 나란히 보여주면서 '헤더가 같다'는 것을 확인시키세요" ② "링 버퍼 인덱스를 화이트보드에 직접 그리면 이해도 급상승" ③ "시간 부족 시 §6.4 TX DMA를 축소하고 §6.3 RX에 집중" |
| 🔍 스스로 점검 | 2개 | ① "링 버퍼 크기가 64일 때, 50바이트 쓰고 30바이트 읽으면 head와 tail 값은?" ② "DMA 순환 모드에서 HT 콜백과 TC 콜백이 호출되는 시점은?" |
| 🔍 메타인지 | 1개 | §6.5 통합 완료 후 "Ch04 인터럽트 버전과 Ch06 DMA 버전의 uart_driver_init()을 비교하세요. 어떤 라인이 바뀌었고, 어떤 라인이 동일한가요?" |

---

## 9. 연습문제 구성 (Bloom 분류체계)

| 수준 | # | 내용 |
|------|---|------|
| 기억 | 1 | USART2의 TX/RX DMA Stream과 Channel 번호를 쓰시오 (F411RE 기준) |
| 기억 | 2 | 링 버퍼에서 "가득 참"과 "비어 있음"을 판별하는 조건식을 각각 쓰시오 |
| 이해 | 3 | Circular DMA 모드에서 Half Transfer 콜백이 필요한 이유를 설명하시오 |
| 이해 | 4 | uart_driver.h 인터페이스를 변경하지 않고 내부를 DMA로 전환할 수 있는 이유를 아키텍처 관점에서 설명하시오 |
| 적용 | 5 | 링 버퍼 크기를 128에서 512로 변경하고, 비트마스크 방식으로 모듈로 연산을 최적화하시오 |
| 적용 | 6 | ch06_04_uart_driver_v2.c에 수신 타임아웃 기능을 추가하시오 (마지막 수신 후 500ms 무응답 시 콜백) |
| 분석 | 7 | 115200bps에서 1초간 연속 수신 시, 링 버퍼 크기가 64/128/256/512일 때 오버플로우 발생 여부를 분석하시오 |
| 평가 | 8 | **(면접 대비 최종판)** 폴링/인터럽트/DMA를 CPU 점유율, 구현 복잡도, 최대 처리량, 지연시간, 전력 소모 5가지 관점에서 비교하고, 각 방식이 최적인 시나리오 1가지씩 제시하시오 |

---

## 10. CubeMX 설정 가이드 (§6.1에 포함)

### USART2 DMA 설정 (NUCLEO-F411RE)
```
Connectivity → USART2:
  Mode: Asynchronous
  Baud Rate: 115200
  Word Length: 8 Bits
  Parity: None
  Stop Bits: 1

DMA Settings → Add:
  USART2_RX: DMA1 Stream5, Channel 4, Circular, Byte, Memory Inc
  USART2_TX: DMA1 Stream6, Channel 4, Normal, Byte, Memory Inc

NVIC Settings:
  USART2 global interrupt: Enable
  DMA1 stream5 global interrupt: Enable
  DMA1 stream6 global interrupt: Enable
```

### 핵심 HAL 함수
- `HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_dma_buf, RX_DMA_BUF_SIZE)` — 순환 DMA + IDLE 감지
- `HAL_UART_Transmit_DMA(&huart2, tx_buf, len)` — 논블로킹 DMA 송신
- `HAL_UARTEx_RxEventCallback(huart, Size)` — IDLE/HT/TC 이벤트 콜백
- `HAL_UART_TxCpltCallback(huart)` — TX 완료 콜백
- `__HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT)` — HT 인터럽트 비활성화 (필요 시)

---

## 11. 리스크 및 대응

| 리스크 | 대응 |
|--------|------|
| "링 버퍼가 데이터 구조 수업 같아서 지루" | 회전초밥 비유 + Ch04 간이 링 버퍼 코드에서 출발 → "이미 쓰고 있었다" 깨달음 유도 |
| "HAL_UARTEx_ReceiveToIdle_DMA가 복잡" | 시퀀스 다이어그램으로 콜백 흐름 시각화 + 단계별 코드 분해 |
| "Ch04 uart_driver와 뭐가 달라진 건지 모호" | §6.5에서 Ch04 코드와 Ch06 코드를 나란히 비교하는 테이블 제공 |
| "TX DMA + RX DMA 동시 운용이 혼란" | §6.3(RX만) → §6.4(TX만) → §6.5(통합) 순서로 분리 학습 |
| "IDLE Line 감지 개념이 생소" | 전화 통화 비유("상대방이 침묵하면 한 문장 끝") + 오실로스코프 파형 SVG |
| "4시간 안에 끝나지 않을 수 있음" | §6.4 TX DMA를 축소 가능 (강사 꿀팁으로 안내), §6.6 실습은 숙제 전환 가능 |

---

## 12. Ch04 → Ch06 코드 변경 비교 (§6.5 핵심 콘텐츠)

| 항목 | Ch04 (인터럽트) | Ch06 (DMA + 링 버퍼) |
|------|-----------------|---------------------|
| 헤더 | `uart_driver.h` | **동일** (변경 없음) |
| RX 방식 | `HAL_UART_Receive_IT()` 1바이트씩 | `HAL_UARTEx_ReceiveToIdle_DMA()` 가변 블록 |
| TX 방식 | `HAL_UART_Transmit()` 블로킹 | `HAL_UART_Transmit_DMA()` 논블로킹 |
| RX 버퍼 | `s_rx_buf[128]` 간이 배열 | `ring_buf_t` 정식 링 버퍼 (256바이트) |
| TX 버퍼 | 없음 (즉시 전송) | `ring_buf_t` TX 큐 (512바이트) |
| 콜백 | `HAL_UART_RxCpltCallback` | `HAL_UARTEx_RxEventCallback` + `HAL_UART_TxCpltCallback` |
| CPU 점유 | TX 블로킹 (~8.7ms/100B) | TX/RX 모두 비점유 |
| FSM | 없음 | `TX_IDLE` / `TX_BUSY` |

---

## 최종 판정

- **강의량**: 4시간 예상 (§6.0~6.6, 이론 1.5h + 실습 2h + 정리 0.5h)
- **난이도**: 중급 (DMA 기본 지식 위에 데이터 구조 + 드라이버 설계 추가)
- **산출물**: HTML 원고 1개 + 코드 5개(+헤더 2개) + SVG 7개
- **누적 성장**: v0.5 → v0.6 (uart_driver DMA 전환 + 링 버퍼 정식 도입 + 비동기 로그 완성)
- **연결 강도**: Ch04(인터페이스 불변 증명) + Ch05(DMA 콜백 확장) + Ch02(비동기 로그 완성) → 3개 챕터 동시 연결
- **특이점**: 이 챕터 이후 uart_driver는 "완성형"으로, Ch15(CLI)까지 변경 없이 재사용됨
