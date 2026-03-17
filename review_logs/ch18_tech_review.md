# Ch18 기술 리뷰 — technical_reviewer.md 기준

## 리뷰어: 기술 리뷰어 (STM32 HAL / NUCLEO-F411RE 전문가)
## 날짜: 2026-03-17
## 대상 파일:
- manuscripts/part6/ch18.html
- code_examples/ch18_hardfault_handler.c
- code_examples/ch18_dma_debug.c
- code_examples/ch18_rtt_advanced.c

---

## 🔴 Critical 이슈 (0건)

없음.

---

## 🟡 Major 이슈 (2건)

### Major 1: CFSR 레지스터 비트 주소 및 조건문 일부 미세 정확도

**위치**: `ch18_hardfault_handler.c` — `debug_decode_cfsr()` 함수
**내용**: 코드에서 `BFSR_PRECISERR` 마스크를 `(1U << 9)`로 정의했습니다.
BFSR(BusFault Status Register)는 CFSR의 비트8~비트15에 해당합니다.
- IBUSERR: CFSR 비트8
- PRECISERR: CFSR 비트9
- IMPRECISERR: CFSR 비트10
- BFARVALID: CFSR 비트15

정의된 값은 정확합니다. 단, IMPRECISERR(비트10) 발생 시 PC 값이 실제 오류 주소보다 몇 사이클 앞을 가리킬 수 있다는 설명이 본문에는 있으나 코드 주석에도 명시하면 더 좋습니다.

**권고**: 코드 주석에 "IMPRECISERR 시 PC는 ±몇 명령어 오프셋 가능, BFAR 우선 확인" 문구 추가.
**심각도**: 기술적 오류는 아님 — 주석 보강 수준.

### Major 2: `uart_tx_dma_safe()` 함수 — `huart2` 외부 참조 방식

**위치**: `code_examples/ch18_dma_debug.c`
**내용**: `extern UART_HandleTypeDef huart2;` 선언으로 CubeMX 생성 전역 핸들에 직접 접근합니다. 이는 STM32 HAL 프로젝트에서 일반적인 패턴이지만, Driver 레이어 캡슐화 원칙(Ch03)과 약간 충돌합니다. 교육 맥락에서는 허용 가능하나 학생들이 "이게 나쁜 패턴인가요?"라고 질문할 수 있습니다.

**권고**: 본문에 "실무에서는 Driver 초기화 시 핸들 포인터를 전달받는 방식(의존성 주입)이 더 클린하지만, 이 예제에서는 교육 목적으로 간소화했습니다" 설명 추가 권고.

---

## 🟢 Minor 이슈 (3건)

### Minor 1: `rtt_advanced_init()`에서 `.noinit` 섹션 링커 스크립트 언급

**위치**: `code_examples/ch18_rtt_advanced.c`
**내용**: `__attribute__((section(".noinit")))` 사용 시 링커 스크립트(STM32F411RETx_FLASH.ld)에 `.noinit` 섹션이 정의되어 있어야 합니다. CubeMX 기본 링커 스크립트에는 없을 수 있어, 학생들이 링크 에러를 마주칠 수 있습니다.
**권고**: 본문 또는 코드 주석에 `.noinit` 섹션 링커 스크립트 추가 방법 간단히 안내.

### Minor 2: `debug_check_stack_canary()`의 RAM 시작 주소 하드코딩

**위치**: `ch18.html` 1-3절 코드
**내용**: `#define STACK_CANARY_ADDR ((volatile uint32_t *)0x20000000)`으로 하드코딩했습니다. F411RE RAM 시작은 0x20000000으로 맞으나, 링커 스크립트의 `_estack` 심볼을 활용하면 더 이식성이 높습니다.
**권고**: 하드코딩임을 주석으로 명시하고, 이식성이 필요하면 링커 심볼 사용 방법 각주 추가.

### Minor 3: DWT_LAR 잠금 해제 코드 — F4 시리즈 일부 실리콘 버전 의존

**위치**: `code_examples/ch18_rtt_advanced.c` — `dwt_init()`
**내용**: `DWT_LAR = 0xC5ACCE55;` 잠금 해제 코드는 CoreSight 구현에 따라 필요한 경우와 그렇지 않은 경우가 있습니다. F411RE에서는 필요하지만, 불필요한 경우 무해합니다.
**권고**: "일부 디바이스에서는 불필요할 수 있으나 추가해도 무해합니다" 주석 추가.

---

## 기술 정확성 총평

| 항목 | 평가 | 비고 |
|-----|-----|-----|
| HardFault 스택 프레임 레이아웃 | ✅ 정확 | R0~xPSR 8개 레지스터 순서 올바름 |
| CFSR 레지스터 비트 정의 | ✅ 정확 | PRECISERR/IMPRECISERR/DIVBYZERO/UNDEFINSTR 위치 정확 |
| EXC_RETURN 패턴 값 | ✅ 정확 | 0xFFFFFFF9/0xFFFFFFFD/0xFFFFFFE9/0xFFFFFFED |
| DMA LISR/HISR 비트 오프셋 | ✅ 정확 | Stream0 기준 TCIF=5, HTIF=4, TEIF=3, DMEIF=2, FEIF=0 |
| HAL_DMA_STATE_BUSY 확인 패턴 | ✅ 실무 표준 패턴 | |
| RTT 채널 모드 선택 근거 | ✅ 정확 | HardFault용 NO_BLOCK_SKIP 선택 이유 명확 |
| DWT 사이클 카운터 마이크로초 변환 | ✅ 정확 | 100MHz 기준 ÷100 올바름 |
| STM32F411RE 핀 배치 | ✅ 정확 | SPI1 PA5/PA6/PA7, I2C SHT31 0x44 올바름 |

**기술 리뷰 결론**: Critical 0건, Major 2건. Major 이슈 모두 교육 문서 보강 수준으로, 기술적 오류 없음. **조건부 승인** — Phase 4에서 주석 보강 후 최종 승인 권고.
