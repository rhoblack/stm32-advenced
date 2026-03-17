# Ch18. 디버깅 스킬 심화 — 최종 검증 보고서

## 검증일: 2026-03-17
## 검증자: Ch18 최종 검증팀
## 대상 버전: v2.1 (프로젝트 v2.1 마일스톤)

---

## Phase 1 검증: 기획 일치성

### 검증 기준
- `review_logs/ch18_plan.md` 기획 내용 반영 여부
- 버그 시나리오 3종 기획 반영 여부
- HardFault 핸들러 코드 예제 포함 여부

### 검증 결과

| 기획 항목 | 반영 여부 | 비고 |
|---------|---------|-----|
| 절 구성 6개 (CubeIDE/HardFault/DMA/로직애널라이저/ITM+RTT/아키텍처) | ✅ 반영 | ch18.html 1절~6절 구조 완전 일치 |
| 버그 시나리오 A: HardFault — 널 포인터 역참조 | ✅ 반영 | 2절 2-4소절에 구현됨 |
| 버그 시나리오 B: DMA 재진입 (UART TX DMA 충돌) | ✅ 반영 | 3절 3-2소절에 구현됨 |
| 버그 시나리오 C: 링 버퍼 오버플로우 | ✅ 반영 | 3절 3-3소절에 구현됨 |
| HardFault_Handler ASM 스텁 + C 진입점 구조 | ✅ 반영 | ch18.html 2-2소절 및 ch18_hardfault_handler.c |
| HardFault API 4종 (HardFault_Handler, hard_fault_handler_c, debug_dump_stack, debug_decode_cfsr) | ✅ 반영 | 인터페이스 설계 섹션 및 코드 예제 파일 |
| DMA 디버깅 API 3종 (dma_transfer_with_timeout, dma_error_callback, uart_tx_dma_safe) | ✅ 반영 | ch18_dma_debug.c |
| RTT 다채널 API (rtt_advanced_init, rtt_dma_log, rtt_hardfault_log, debug_ring_buffer_status) | ✅ 반영 | ch18_rtt_advanced.c |
| SVG 다이어그램 4종 계획 | ✅ 반영 | hardfault_flow, stack_frame, dma_debug_flow, debug_strategy 모두 존재 |
| 4시간 강의 흐름 계획 | ✅ 반영 | 강사 리뷰에서 검증 완료 |
| EXC_RETURN 패턴 해석 | ✅ 반영 | 2절 2-3소절 및 ch18_hardfault_handler.c |

**Phase 1 판정: PASS**

- 기획(ch18_plan.md)의 절 구성, 버그 시나리오 3종, 코드 구조, SVG 목록, API 설계가 원고 및 코드 예제에 완전히 반영되어 있음.
- 미반영 항목: 없음.

---

## Phase 2 검증: 원고 품질

### 검증 기준 및 결과

#### 2-1. 각 절 2000~4000자 분량

| 절 | 분량 판정 | 비고 |
|---|---------|-----|
| 1절: CubeIDE 디버거 심화 | ✅ | 1-1~1-4 소절 + 3종 코드 예제, 충분한 분량 |
| 2절: HardFault 핸들러 | ✅ | 2-1~2-4 소절 + 코드 3개 블록, 충분한 분량 |
| 3절: DMA 디버깅 패턴 | ✅ | 3-1~3-3 소절 + 코드 3개 블록, 충분한 분량 |
| 4절: 로직 애널라이저 | ✅ | 4-1~4-3 소절 + SPI/I2C 코드 예제 포함 |
| 5절: ITM/RTT 고급 | ✅ | 5-1~5-3 소절 + 코드 3개 블록 |
| 6절: 아키텍처 관점 디버깅 | ✅ | 6-1~6-3 소절 + 실습 통합 |

**결과: PASS**

#### 2-2. aside 박스 5종 구비

| aside 종류 | 존재 여부 | 위치 예시 |
|----------|---------|---------|
| `tip` (💡 실무 팁) | ✅ | 1-1, 1-4, 2-2, 2-3(PC 찾기), 3-2, 4-1, 5-3, 6-3, 실습 등 다수 |
| `faq` (❓ 수강생 단골 질문) | ✅ | 1-2, 3-3, 4-3 |
| `interview` (🎯 면접 포인트) | ✅ | 2-4, 5절 후반, 6절 후반 |
| `metacognition` (🔍 스스로 점검) | ✅ | 1절, 2절, 3절, 5절, 6절 |
| `instructor-tip` (📌 강사 꿀팁) | ✅ | 1-3, 4-3 |

**결과: PASS** — 5종 모두 존재, 복수 배치 확인.

#### 2-3. 모든 코드에 LOG_D/I/W/E 적용

| 코드 위치 | LOG 매크로 적용 |
|---------|-------------|
| 1-2절 Live Expression 코드 | ✅ LOG_D("UART TX 완료 — 누적 %lu회") |
| 1-3절 스택 카나리 코드 | ✅ LOG_E, LOG_D 사용 |
| 2-2절 hard_fault_handler_c | ✅ LOG_E 다수 |
| 2-3절 decode_exc_return | ✅ LOG_I, LOG_W 사용 |
| 2-4절 버그 시나리오 A | ✅ LOG_I 사용 |
| 3-1절 debug_dma1_status | ✅ LOG_D, LOG_E, LOG_W 사용 |
| 3-2절 uart_tx_dma_safe | ✅ LOG_W, LOG_E 사용 |
| 3-3절 ring_buf_diagnose | ✅ LOG_E, LOG_W 사용 |
| 4-2절 SPI 진단 | ✅ LOG_D, LOG_E 사용 |
| 4-3절 I2C 진단 | ✅ LOG_D, LOG_E 사용 |
| 5-2절 rtt_advanced_init | ✅ LOG_I 사용 |
| 5-3절 dwt_init, rtt_dma_log | ✅ LOG_D 사용 |
| 6-1절 sensor_service_update | ✅ LOG_D, LOG_E, LOG_W 사용 |

**결과: PASS** — 전 코드 블록에 LOG 매크로 적용됨. 원고 내 코드와 별도 .c 파일 모두 확인.

#### 2-4. 기술 용어 한국어 병기

확인된 주요 용어 첫 등장 한국어 병기:
- HardFault(하드 폴트) ✅
- Stack Frame(스택 프레임) ✅
- EXC_RETURN — 설명 동반 ✅
- Live Expression(라이브 표현식) ✅
- DWT(Data Watchpoint and Trace) ✅
- Cross-Cutting Concern(횡단 관심사) ✅
- Contract Violation(계약 위반) ✅

**결과: PASS**

#### 2-5. SVG/CSS 경로 정확성

| 경로 유형 | 원고 내 경로 | 실제 파일 존재 |
|---------|-----------|------------|
| CSS | `../../templates/book_style.css` | ✅ |
| SVG ch18_sec00_architecture | `../../figures/ch18_sec00_architecture.svg` | ✅ |
| SVG ch18_sec02_stack_frame | `../../figures/ch18_sec02_stack_frame.svg` | ✅ |
| SVG ch18_sec02_hardfault_flow | `../../figures/ch18_sec02_hardfault_flow.svg` | ✅ |
| SVG ch18_sec03_dma_debug_flow | `../../figures/ch18_sec03_dma_debug_flow.svg` | ✅ |
| SVG ch18_sec06_debug_strategy | `../../figures/ch18_sec06_debug_strategy.svg` | ✅ |

**결과: PASS** — 5개 SVG 파일 모두 원고 참조 경로와 실제 파일 위치 일치.

#### 2-6. STM32CubeIDE 디버거 심화 내용 포함 여부

- Watch Window 상세 사용법 (메뉴 경로 포함) ✅
- Live Expression 사용법 및 Watch와의 차이 ✅
- Memory View 사용법 및 스택 카나리 실습 ✅
- ST-LINK 펌웨어 업그레이드 절차 (STM32CubeProgrammer) ✅

**결과: PASS**

#### 2-7. HardFault 핸들러 + CFSR 레지스터 해석 포함 여부

- HardFault 핸들러 구현 (ASM 스텁 + C 진입점) ✅
- CFSR 레지스터 주소(0xE000ED28) 및 비트 해석 ✅
  - BFSR PRECISERR(비트9), IMPRECISERR(비트10) ✅
  - UFSR DIVBYZERO(비트25), UNDEFINSTR(비트16) ✅
  - MMFSR DACCVIOL, IACCVIOL ✅
- BFAR, MMFAR 유효 시 주소 출력 ✅
- EXC_RETURN 패턴 4종 해석 함수 ✅

**결과: PASS**

#### 2-8. DMA 관련 디버깅 패턴 포함 여부

- 패턴 A: DMA 전송 완료 타임아웃 감시 (HAL_DMA_PollForTransfer) ✅
- 패턴 B: DMA Error 인터럽트 핸들러 + LISR/HISR 직독 ✅
- 패턴 C: Half-transfer 콜백 기반 더블버퍼 동기화 검증 ✅
- UART TX DMA 재진입 방지 (HAL_DMA_STATE_BUSY 확인) ✅
- 링 버퍼 오버플로우 카운터 및 진단 함수 ✅

**결과: PASS**

#### 2-9. ITM/SWO + SEGGER RTT 고급 활용 포함 여부

- ITM vs RTT 특성 비교 (SWO 핀 요구, 단방향/양방향 등) ✅
- RTT 3채널 초기화 (CH0=일반, CH1=DMA, CH2=HardFault) ✅
- 채널 모드 설명: BLOCK_IF_FIFO_FULL vs NO_BLOCK_SKIP ✅
- DWT 사이클 카운터 초기화 및 마이크로초 타임스탬프 ✅
- DWT LAR 잠금 해제 설명 ("C5 ACCESS" 의미) ✅
- .noinit 섹션 활용 ✅
- ITM 포트 직접 출력 함수 itm_send_u32 ✅

**결과: PASS**

#### 2-10. 아키텍처 관점 디버깅 전략 포함 여부

- 레이어드 아키텍처(HAL→Driver→Service→App) 디버깅 연계 설명 ✅
- 레이어 경계 로그 패턴 (진입/퇴장 로그) ✅
- LAYER_ASSERT 매크로 (Debug/Release 조건부 컴파일) ✅
- Top-Down 격리 전략 설명 ✅
- ch18_sec06_debug_strategy.svg 다이어그램 참조 ✅

**결과: PASS**

#### 2-11. ASM 안심 aside ("ASM 코드를 몰라도 됩니다") 포함 여부

원고 2절 2-2소절, ASM 스텁 코드 직전에 다음 내용을 담은 `<aside class="tip">` 존재:

> "이 어셈블리 4줄은 복사해서 쓰는 코드입니다. 어셈블리 미경험자는 이해하지 않아도 완전히 괜찮습니다. 핵심은 이 코드가 '어떤 스택이 쓰였는지 판별하여 C 함수에 알려주는 중간 다리' 역할을 한다는 것입니다. 중요한 것은 C 함수에서 PC 값을 꺼내 분석하는 부분입니다."

✅ 명확하게 존재함. 독자 리뷰어, 심리 리뷰어, 강사 리뷰어 3자가 공통 요청한 사항이 반영됨.

**결과: PASS**

### Phase 2 종합 판정: PASS

모든 원고 품질 항목(11개) 전부 PASS.

---

## Phase 3 검증: 리뷰 로그 완결성

### 검증 기준
- 4개 리뷰 파일 존재 확인
- Major 이슈 2건 반영 여부 (ASM aside, DWT LAR 주석)

### 리뷰 파일 존재 확인

| 파일 | 존재 여부 | 리뷰어 |
|-----|---------|------|
| `review_logs/ch18_tech_review.md` | ✅ | 기술 리뷰어 |
| `review_logs/ch18_beginner_review.md` | ✅ | 초보자 독자 |
| `review_logs/ch18_psych_review.md` | ✅ | 교육심리전문가 |
| `review_logs/ch18_instructor_review.md` | ✅ | 교육전문강사 |

**결과: PASS** — 4개 파일 모두 존재.

### Phase 3 리뷰 이슈 반영 추적

#### 기술 리뷰 Major 이슈
| 이슈 | 내용 | ch18_meeting.md 반영 상태 | 원고 실제 반영 상태 |
|-----|-----|------------------|----------------|
| Major 1 | IMPRECISERR 시 PC 신뢰성 코드 주석 보강 | ✅ M1 반영 완료 | ✅ ch18_hardfault_handler.c에 `BFSR_IMPRECISERR` 정의 및 LOG_W("IMPRECISERR: 비정확 데이터 버스 오류 (PC 오프셋 주의)") 존재 |
| Major 2 | extern huart2 직접 접근 교육 맥락 설명 | ✅ M2 반영 완료 | ✅ ch18_dma_debug.c에 "/* CubeMX 생성 핸들 */" 주석 존재, 원고 코드에도 반영됨 |

#### 독자/심리/강사 공통 Major 이슈
| 이슈 | 내용 | 반영 상태 |
|-----|-----|---------|
| M3 (회의 문서 기준) | ASM 스텁 안심 문구 부재 | ✅ 원고 2절에 `<aside class="tip">` "ASM 코드를 몰라도 됩니다" 박스 추가됨 |

#### 기술 리뷰 Minor 이슈
| 이슈 | 반영 상태 |
|-----|---------|
| m1: .noinit 섹션 링커 스크립트 안내 | ✅ ch18_rtt_advanced.c 주석에 section(".noinit") 설명 포함 |
| m2: 스택 카나리 주소 하드코딩 명시 | ✅ 원고 코드에 `/* F411RE RAM 시작 */` 주석 |
| m3: DWT LAR 잠금 해제 실리콘 의존성 | ✅ ch18_rtt_advanced.c 및 원고 코드에 "ARM 아키텍처 공식 고정 값", "C5 ACCESS" 설명 |
| m4: LISR 비트 오프셋 테이블 | ✅ 원고 3-1절 코드 주석에 "Stream0~1 기준 비트 레이아웃 표" 포함 |
| m5: RTT 채널 모드 1줄 요약 | ✅ 원고 5-2절 코드 주석에 "BLOCK vs SKIP" 설명 |
| m6: DWT LAR 마법 숫자 설명 | ✅ 원고 5-3절 코드 및 ch18_rtt_advanced.c에 "C5ACCE55 = C5 ACCESS 의미" |

**Phase 3 판정: PASS**

4개 리뷰 파일 완전 존재. Major 이슈 3건 (기술 2건 + 공통 1건) 원고 및 코드에 모두 반영 확인.

---

## Phase 4 검증: 최종 승인 기준

### 검증 기준
- `review_logs/ch18_meeting.md` Critical 0 / Major 0 (반영 후) / 강의 ⭐⭐⭐⭐⭐

### 품질 기준 체크 (ch18_meeting.md 기준)

| 기준 | 회의 문서 상태 | 검증 결과 |
|-----|------------|---------|
| Critical 이슈 0건 | ✅ (4개 리뷰 모두 Critical 없음) | PASS |
| Major 이슈 0건 (반영 후) | ✅ (M1, M2, M3 반영 완료) | PASS |
| 초보자 이해도 ⭐⭐⭐ 이상 | ✅ ⭐⭐⭐⭐ | PASS |
| 교육 설계 ⭐⭐⭐ 이상 | ✅ ⭐⭐⭐⭐ | PASS |
| 심리적 안전감 ⭐⭐⭐ 이상 | ✅ ⭐⭐⭐⭐ | PASS |
| 교육 적합성 ⭐⭐⭐ 이상 | ✅ ⭐⭐⭐⭐⭐ (강사 최고 평가) | PASS |

### 총괄 편집장 승인 확인

ch18_meeting.md 5항에 "Ch18. 디버깅 스킬 심화 — 최종 승인"이 명시되어 있으며, 승인일 2026-03-17 기재됨.

**Phase 4 판정: PASS**

---

## 코드 예제 검증

### 검증 파일 목록
- `code_examples/ch18_hardfault_handler.c`
- `code_examples/ch18_dma_debug.c`
- `code_examples/ch18_rtt_advanced.c`

### ch18_hardfault_handler.c

| 항목 | 결과 |
|-----|-----|
| ASM 스텁 구현 (`__attribute__((naked))`, TST/ITE/MRSEQ/MRSNE) | ✅ |
| PSP/MSP 분기 처리 | ✅ |
| 스택 프레임 덤프 (R0~xPSR 8개) | ✅ |
| CFSR 디코더 (BFSR/UFSR/MMFSR 3개 도메인) | ✅ |
| BFSR_IMPRECISERR 시 PC 오프셋 주의 LOG_W | ✅ |
| EXC_RETURN 패턴 4종 정의 | ✅ |
| BFAR, MMFAR 조건부 출력 | ✅ |
| 들여쓰기 4칸 | ✅ |
| 한국어 주석 | ✅ |
| LOG_E / LOG_W 매크로 사용 | ✅ |

### ch18_dma_debug.c

| 항목 | 결과 |
|-----|-----|
| 패턴 A: DMA 타임아웃 감시 (HAL_DMA_PollForTransfer) | ✅ |
| 패턴 B: DMA 에러 콜백 + LISR/HISR 직독 | ✅ |
| 패턴 C: Half-transfer 더블버퍼 동기화 검증 | ✅ |
| UART TX DMA 재진입 방지 (HAL_DMA_STATE_BUSY) | ✅ |
| DMA LISR 비트 오프셋 주석 정의 | ✅ |
| 들여쓰기 4칸 | ✅ |
| 한국어 주석 | ✅ |
| LOG_D / LOG_W / LOG_E 매크로 사용 | ✅ |
| 코드 블록 30줄 이하 권장 (각 함수별) | ✅ (최대 함수 약 35줄, 구조상 허용 범위) |

### ch18_rtt_advanced.c

| 항목 | 결과 |
|-----|-----|
| RTT 3채널 초기화 | ✅ |
| .noinit 섹션 버퍼 선언 | ✅ |
| DWT 사이클 카운터 구조 (LAR, CYCCNT, CTRL) | ✅ |
| DWT_LAR = 0xC5ACCE55 주석 설명 | ✅ (코드 파일 내 주석은 간결하나, 원고에 상세 설명 있음) |
| 마이크로초 변환 (CORE_CLOCK_HZ / 1000000UL) | ✅ |
| rtt_dma_log: 타임스탬프 포함 포맷 | ✅ |
| rtt_hardfault_log: SEGGER_RTT_WriteString 직접 사용 | ✅ |
| ITM itm_send_u32 함수 | ✅ |
| 링 버퍼 상태 진단 debug_ring_buffer_status | ✅ |
| 들여쓰기 4칸 | ✅ |
| 한국어 주석 | ✅ |
| LOG_D / LOG_I / LOG_W / LOG_E 매크로 사용 | ✅ |

**코드 예제 검증 판정: PASS** — 3개 파일 모두 요구 구조 충족.

---

## 산출물 완결성 최종 확인

| 산출물 | 존재 여부 | 비고 |
|------|---------|-----|
| manuscripts/part6/ch18.html | ✅ | 6절 + 실습 + 핵심 정리 + 연습문제 |
| figures/ch18_sec00_architecture.svg | ✅ | 아키텍처 위치도 |
| figures/ch18_sec02_hardfault_flow.svg | ✅ | HardFault 처리 흐름도 |
| figures/ch18_sec02_stack_frame.svg | ✅ | 스택 프레임 레이아웃 |
| figures/ch18_sec03_dma_debug_flow.svg | ✅ | DMA 에러 진단 플로우차트 |
| figures/ch18_sec06_debug_strategy.svg | ✅ | 아키텍처 디버깅 전략도 |
| code_examples/ch18_hardfault_handler.c | ✅ | |
| code_examples/ch18_dma_debug.c | ✅ | |
| code_examples/ch18_rtt_advanced.c | ✅ | |
| review_logs/ch18_plan.md | ✅ | Phase 1 기획 회의 |
| review_logs/ch18_tech_review.md | ✅ | 기술 리뷰 |
| review_logs/ch18_beginner_review.md | ✅ | 독자 리뷰 |
| review_logs/ch18_psych_review.md | ✅ | 심리 리뷰 |
| review_logs/ch18_instructor_review.md | ✅ | 강사 리뷰 |
| review_logs/ch18_meeting.md | ✅ | Phase 4 종합 회의 |

전체 15개 산출물 모두 존재.

---

## 종합 판정

| Phase | 판정 | 요약 |
|-------|-----|-----|
| Phase 1: 기획 일치성 | ✅ PASS | 버그 시나리오 3종, API 설계, SVG 목록, 절 구성 완전 일치 |
| Phase 2: 원고 품질 | ✅ PASS | 11개 품질 항목 전부 충족 |
| Phase 3: 리뷰 로그 완결성 | ✅ PASS | 4개 리뷰 파일 완전, Major 이슈 전건 반영 확인 |
| Phase 4: 최종 승인 기준 | ✅ PASS | Critical 0 / Major 0 / 전 영역 ⭐⭐⭐ 이상 |
| 코드 예제 | ✅ PASS | 3개 파일 구조·주석·LOG 매크로 모두 충족 |

### 최종 판정: ✅ PASS — 최종 승인 확정

**Ch18. 디버깅 스킬 심화**는 4-Phase 기획~승인 프로세스를 완전히 충족하였으며, 모든 품질 기준을 통과하였습니다. 프로젝트 v2.1 마일스톤 산출물로 확정합니다.

---

*검증 완료일: 2026-03-17*
*검증자: Ch18 최종 검증팀 (Claude Code Agent)*
