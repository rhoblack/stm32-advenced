# Ch18. 디버깅 스킬 심화 — Phase 1 기획 회의 통합 결과

## 날짜: 2026-03-17
## 챕터: Ch18 · Part 6 · 프로젝트 v2.1

---

## Sub-agent 1: 기술 저자 관점 — 코드 예제 및 버그 시나리오 설계

### HardFault 핸들러 코드 설계

HardFault는 STM32에서 가장 흔하면서도 가장 당황스러운 예외(Exception)입니다.
설계 목표: HardFault 발생 시 스택 덤프를 UART/RTT로 자동 출력하는 핸들러 구현.

**핵심 구현 요소:**
1. `HardFault_Handler` ASM 스텁 → `hard_fault_handler_c()` C 함수 분리 구조
2. PSP(Process Stack Pointer) vs MSP(Main Stack Pointer) 분기 처리
3. 스택 덤프: R0~R3, R12, LR, PC, xPSR 8개 레지스터 자동 출력
4. CFSR(Configurable Fault Status Register) 디코딩:
   - MMFSR (MemManage Fault Status)
   - BFSR (BusFault Status)
   - UFSR (UsageFault Status)
5. LR 값 해석: EXC_RETURN 패턴 (0xFFFFFFF9 / 0xFFFFFFFD / 0xFFFFFFE9 등)

**주요 API 설계:**
```c
void HardFault_Handler(void);                     // ASM 스텁 (startup에서 override)
void hard_fault_handler_c(uint32_t *stack_frame); // C 진입점
void debug_dump_stack(uint32_t *frame);           // 스택 덤프 출력
void debug_decode_cfsr(uint32_t cfsr);            // CFSR 비트 해석
```

### DMA 디버깅 패턴 설계

**패턴 1: DMA 전송 완료 타임아웃 감시**
- `HAL_DMA_PollForTransfer()` 타임아웃 파라미터 활용
- `HAL_DMA_GetError()` 에러 코드 분석

**패턴 2: DMA Error 인터럽트 핸들러**
- `HAL_DMA_IRQHandler()` → `XferErrorCallback` 분석
- DMA LISR/HISR 레지스터 직접 읽기

**패턴 3: Half-transfer 콜백 기반 더블버퍼 동기화 검증**
- Half-transfer callback에서 버퍼 상태 로그 출력

### 버그 시나리오 3종

**시나리오 A: HardFault — 널 포인터 역참조**
- 원인: `sensor_service.c`에서 미초기화 포인터로 온습도 값 쓰기 시도
- 증상: 센서 읽기 직후 시스템 리셋
- 해결: BFSR PRECISERR 비트 → PC 값으로 원인 라인 특정

**시나리오 B: DMA 충돌 — UART TX DMA 진행 중 재진입**
- 원인: CLI 명령 처리 중 UART TX DMA 완료 전 두 번째 전송 요청
- 증상: 출력 데이터 깨짐, 가끔 HardFault
- 해결: `hdma_usart2_tx.State == HAL_DMA_STATE_BUSY` 상태 확인 패턴

**시나리오 C: 링 버퍼 오버플로우**
- 원인: 로그 출력 폭증(디버그 모드) → 링 버퍼 write pointer가 read pointer 추월
- 증상: 로그 메시지 일부 손실, 나중에 쓰레기값 출력
- 해결: 버퍼 오버플로우 카운터 추가 + LOG_LEVEL 조정

---

## Sub-agent 2: 교육 설계자 관점 — 학습 목표 및 누적 연결점

### 학습 목표 4개 ("~할 수 있다" 형태)

| # | 블룸 수준 | 학습 목표 |
|---|---------|---------|
| 1 | 이해 | HardFault 발생 메커니즘과 스택 덤프에서 R0~R3, PC, LR, xPSR의 의미를 설명할 수 있다. |
| 2 | 적용 | STM32CubeIDE Watch/Live Expression/Memory View를 활용하여 런타임 변수와 메모리 상태를 실시간으로 추적할 수 있다. |
| 3 | 분석 | DMA 전송 오류(Half-transfer 동기화, 에러 인터럽트)를 로그와 레지스터 덤프로 격리(Isolate)하고 원인을 특정할 수 있다. |
| 4 | 평가 | 레이어드 아키텍처(HAL → Driver → Service → App) 관점에서 버그 발생 레이어를 체계적으로 좁혀가는 격리 전략을 적용할 수 있다. |

### Ch02 + Ch05 → Ch18 누적 연결점

**Ch02(로그 시스템) → Ch18**:
- SWO/ITM printf 리다이렉트 → Ch18에서 HardFault 핸들러 내 긴급 출력으로 재활용
- SEGGER RTT → 인터럽트 컨텍스트에서도 안전한 출력 경로로 심화 활용
- LOG_D/I/W/E 매크로 → Ch18에서 디버그 레벨 동적 조정 실습

**Ch05(DMA 아키텍처) → Ch18**:
- DMA Stream/Channel 구조 이해 → LISR/HISR 레지스터 직접 읽기
- DMA 완료/반완료 콜백 → Half-transfer 디버깅 패턴으로 확장
- DMA Error 인터럽트 → Ch18에서 에러 핸들러 구현 실습

**Ch06(UART DMA + 링버퍼) → Ch18**:
- 링 버퍼 구현 → 오버플로우 시나리오 C 기반

### 절 구성별 인지 부하 분석

| 절 | 신규 개념 수 | 인지 부하 | 비고 |
|---|-----------|---------|-----|
| 1절: CubeIDE 디버거 심화 | 2개 (Live Expr, Memory View) | 보통 | 실습 위주 |
| 2절: HardFault 핸들러 | 3개 (스택 프레임, CFSR, EXC_RETURN) | 높음 | 비유로 완화 필요 |
| 3절: DMA 디버깅 패턴 | 2개 (LISR/HISR, Half-transfer 동기화) | 보통 | 시나리오 기반 |
| 4절: 로직 애널라이저 | 1개 (프로토콜 디코딩) | 낮음 | 시각 자료 중심 |
| 5절: ITM/RTT 고급 | 2개 (채널 분리, 타임스탬프) | 보통 | Ch02 복습 연계 |
| 6절: 아키텍처 디버깅 | 2개 (레이어 격리, 계약 위반) | 보통 | 통합 실습 |

---

## Sub-agent 3: 강사 관점 — 수강생 막힘 Top 5 및 해소 방법

### 수강생 막힘 Top 5

**막힘 1: "HardFault가 뜨면 어디서부터 봐야 할지 모르겠어요"**
- 원인: PC 레지스터가 0xXXXX처럼 이상한 값을 보여 막막함
- 해소: "탐정처럼 역추적" 비유. PC → Disassembly → 해당 소스 라인 순서 시연
- 안심 문구: "HardFault는 처음 보면 공포스럽지만, 알고 보면 가장 친절한 예외입니다. 어디서 죽었는지 정확히 알려주니까요."

**막힘 2: "CFSR 레지스터 비트가 너무 많아요"**
- 원인: ARMv7-M 아키텍처 레지스터 문서의 방대한 비트 필드
- 해소: BFSR PRECISERR / UFSR UNDEFINSTR 2가지만 집중. "나머지는 필요할 때 찾아봐도 됩니다"

**막힘 3: "로직 애널라이저를 어떻게 연결하나요? 장비가 없으면 어떡하나요?"**
- 원인: 고가 장비에 대한 진입 장벽
- 해소: 저가 Saleae 호환(~5달러) 소개. 소프트웨어 시뮬레이션 대안(CubeIDE 내장 ST-LINK SWD 파형)

**막힘 4: "DMA 디버깅에서 어느 시점에 어느 레지스터를 봐야 하나요?"**
- 원인: DMA 전송이 비동기라 타이밍 불확실
- 해소: "DMA 전송 시퀀스 체크리스트" 제공. 에러 발생 시 LISR → DMA_SxCR → DMA_SxNDTR 순 확인

**막힘 5: "아키텍처 관점 디버깅이 뭔가요? 어차피 버그는 코드 어딘가에 있는 거 아닌가요?"**
- 원인: 아키텍처 추상화와 디버깅의 연결이 직관적으로 와닿지 않음
- 해소: "소방서 화재 대응" 비유. 어느 구역(레이어)에서 불이 났는지 먼저 파악 → 그 레이어만 집중 조사

### 4시간 강의 흐름 제안

| 시간 | 내용 | 방식 |
|-----|-----|-----|
| 0:00~0:50 | 1절: CubeIDE 디버거 심화 (Watch/Live Expr/Memory View) | 실습 위주 |
| 0:50~1:40 | 2절: HardFault 핸들러 구현 + 스택 덤프 분석 | 코딩 + 시연 |
| 1:40~2:30 | 3절: DMA 디버깅 3종 시나리오 | 버그 재현 실습 |
| 2:30~2:45 | 휴식 | - |
| 2:45~3:20 | 4절: 로직 애널라이저 파형 분석 | 시연 + 이론 |
| 3:20~3:45 | 5절: ITM/RTT 고급 활용 | 실습 |
| 3:45~4:00 | 6절: 아키텍처 디버깅 전략 + 정리 | 통합 토론 |

---

## 통합 결론: 절 구성 최종안

1. **1절**: STM32CubeIDE 디버거 심화 활용법 (Watch / Live Expression / Memory View)
2. **2절**: HardFault 핸들러 구현 및 스택 덤프 분석
3. **3절**: DMA 관련 디버깅 패턴 (시나리오 A/B/C)
4. **4절**: 로직 애널라이저로 SPI/I2C/UART 파형 분석
5. **5절**: ITM/SWO + SEGGER RTT 고급 활용 (Ch02 심화)
6. **6절**: 아키텍처 관점 디버깅 — 레이어 경계 버그 격리 전략

## 주요 SVG 다이어그램
- `ch18_sec02_hardfault_flow.svg`: HardFault 처리 흐름 (예외 발생 → 스택 저장 → 핸들러 → 덤프 출력)
- `ch18_sec02_stack_frame.svg`: 스택 프레임 레이아웃 (R0~xPSR 8개 레지스터 위치)
- `ch18_sec03_dma_debug_flow.svg`: DMA 에러 진단 플로우차트
- `ch18_sec06_debug_strategy.svg`: 레이어 경계 버그 격리 전략 다이어그램

## 주요 코드 예제 파일
- `code_examples/ch18_hardfault_handler.c`: HardFault 핸들러 + CFSR 디코더
- `code_examples/ch18_dma_debug.c`: DMA 디버깅 패턴 3종
- `code_examples/ch18_rtt_advanced.c`: SEGGER RTT 다채널 + 타임스탬프 활용
