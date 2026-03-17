# Ch05 기획 — 기술 저자 제안

**챕터**: Ch05. DMA 아키텍처와 동작 원리
**프로젝트 버전**: v0.4 → v0.5
**누적 기능**: Mem-to-Mem DMA 버퍼 복사 성능 측정 (폴링 대비)
**선행 지식**: Ch01(GPIO/EXTI), Ch02(로그), Ch03(4계층 아키텍처), Ch04(UART/Driver 레이어)
**예상 분량**: 6개 절 × 약 3,000자 = ~18,000자
**강의 시간**: 4시간 (이론 2h + 실습 1.5h + 정리 0.5h)

---

## 1. 아키텍처 위치

### 레이어 배치
- **DMA는 HAL 레이어 하위** — 특정 Driver에 국한되지 않고 **모든 Driver에 공통 적용되는 횡단 관심사(Cross-cutting Concern)**
- Ch04의 uart_driver는 Driver 레이어에 위치했지만, DMA는 uart/spi/i2c/tim 모두에 걸쳐 동작
- 레스토랑 비유 확장: DMA = **자동 서빙 로봇** (요리사가 직접 나르지 않아도 음식이 테이블에 도착)

### v0.4 → v0.5 아키텍처 갱신
- v0.4: `App → Driver(uart_driver) → HAL(HAL_UART_*) → HW`
- v0.5: `App → Driver(uart_driver) → HAL(HAL_UART_*) → DMA Controller → HW`
- DMA Controller가 HAL과 HW 사이의 **데이터 전송 가속기**로 추가
- 색상: DMA 레이어 = **#FF6D00(주황)** (기존 HAL 파랑, Driver 초록과 구별)

### "왜 Ch05에 DMA가 나오는가?" 근거
- Ch06(UART DMA), Ch09(Timer DMA burst), Ch12(SPI DMA LCD), Ch14(I2C DMA) — **4개 챕터의 공통 기반**
- DMA 원리를 먼저 단독 학습 → 이후 각 주변장치 적용 시 "DMA 부분은 이미 알지?" 전략
- Mem-to-Mem 전송으로 주변장치 복잡성 없이 **DMA 본질만 집중 학습**

---

## 2. 핵심 개념 (3가지 전송 모드 + 부가 개념)

### 2.1 폴링 vs 인터럽트 vs DMA 비교 (Ch04 복습 연계)
- Ch04에서 폴링 vs 인터럽트 학습 완료 → DMA를 세 번째 축으로 추가
- CPU 개입도: 폴링(100%) > 인터럽트(~5%) > DMA(~0%)
- 비유: 택배 수령 — 폴링(현관 앞 대기) / 인터럽트(초인종 반응) / DMA(경비실 보관함)

### 2.2 DMA 컨트롤러 구조 (STM32F411RE 기준)
- **DMA1**: 7 Streams × 8 Channels (주로 APB1 주변장치: USART2, I2C1, TIM2~5)
- **DMA2**: 8 Streams × 8 Channels (주로 APB2 주변장치: SPI1, USART1, TIM1) + **Mem-to-Mem 지원**
- **중요**: Mem-to-Mem은 **DMA2에서만 가능** (DMA1 불가) — 이 제약 강조
- Stream/Channel 매핑: CubeMX에서 자동 설정되지만, 충돌 시 수동 조정 필요
- Priority 4단계: Low / Medium / High / Very High

### 2.3 3가지 DMA 전송 모드
| 모드 | 소스 | 목적지 | 이번 장 | 이후 |
|------|------|--------|---------|------|
| **Mem-to-Mem** | SRAM | SRAM | ★ 핵심 실습 | — |
| **Mem-to-Periph** | SRAM | 주변장치 | 개념 소개 | Ch06 UART TX |
| **Periph-to-Mem** | 주변장치 | SRAM | 개념 소개 | Ch06 UART RX, Ch14 ADC |

### 2.4 순환 모드(Circular) vs 일회(Normal)
- Normal: 전송 완료 → 정지 (이번 장 실습)
- Circular: 전송 완료 → 자동 재시작 (링 버퍼 패턴, Ch06 UART DMA에서 실전 적용)
- 순환 모드는 이번 장에서 **개념만 소개** (SVG 링 버퍼 다이어그램으로 시각화)

### 2.5 더블 버퍼링
- 고급 개념 → 이번 장에서는 **존재만 언급**, Ch06에서 상세
- "왜 필요한가?" 동기 부여만 제공 (데이터 유실 방지)

---

## 3. 성능 측정 시나리오

### 측정 설계
- **데이터**: `uint32_t src[1024]`, `uint32_t dst[1024]` (4KB 각각, 총 8KB SRAM 사용)
- **STM32F411RE SRAM**: 128KB → 8KB는 충분 (Ch03에서 SRAM 오버플로우 주의사항 학습 연계)
- **반복**: 100회 복사 → 총 400KB 전송
- **측정 도구**: `HAL_GetTick()` (ms 단위) + DWT Cycle Counter (us 단위, 정밀 측정)

### 예상 결과
- 폴링(for 루프 memcpy): ~2.5ms (100MHz 클럭, 1024 word 복사)
- DMA Mem-to-Mem: ~0.3ms (버스 직접 접근, CPU 미개입)
- **체감 비율**: DMA가 약 8~10배 빠름 (실제 측정은 버스 경합에 따라 변동)
- **핵심 포인트**: 속도보다 **CPU 해방**이 진짜 이점 (DMA 전송 중 CPU는 다른 작업 가능)

### CPU 사용률 시연
- 폴링 복사 중: LED 깜빡임 정지 (CPU 블로킹)
- DMA 복사 중: LED 깜빡임 유지 (CPU 자유)
- 이 차이를 `LOG_I`로 타임스탬프와 함께 출력

---

## 4. 코드 예제 매핑

### §5.2: 폴링 버전 — `ch05_01_memcpy_polling.c` (~40줄)
```
- uint32_t src[1024], dst[1024] 선언
- for 루프로 word 단위 복사
- DWT Cycle Counter로 시간 측정
- LOG_I로 결과 출력
```

### §5.3: DMA 버전 — `ch05_02_dma_mem_to_mem.c` (~50줄)
```
- CubeMX DMA2 Stream0 설정 (Mem-to-Mem, Word, Normal)
- HAL_DMA_Start_IT() 호출
- HAL_DMA_XferCpltCallback에서 완료 플래그 설정
- 완료 대기 후 데이터 검증 (src == dst 확인)
- LOG_I로 전송 시간 + 검증 결과 출력
```

### §5.4: 성능 비교 — `ch05_03_performance_compare.c` (~50줄)
```
- 폴링 100회 반복 측정
- DMA 100회 반복 측정
- 결과 비교 테이블 UART 출력 (Ch04 uart_send 활용!)
- LED 토글로 CPU 해방 시각화
```

### 코드 설계 원칙
- 모든 코드에 `LOG_D`/`LOG_I`/`LOG_W`/`LOG_E` 적용 (Ch02 표준)
- `uart_send_string()` 활용하여 PC 터미널에 결과 출력 (Ch04 연계)
- HAL 표준 준수, 들여쓰기 4칸, 스네이크 케이스, 한국어 주석
- 코드 블록 30줄 이하 (분할 설명)

---

## 5. SVG 다이어그램 리스트

| 파일명 | 내용 | 위치 |
|--------|------|------|
| `ch05_sec00_architecture.svg` | v0.5 아키텍처 — DMA 레이어 추가 (주황색) | §5.0 |
| `ch05_sec00_sequence.svg` | App → Driver → HAL → DMA → HW 시퀀스 | §5.0 |
| `ch05_sec01_dma_controller.svg` | DMA1/DMA2 Stream/Channel 구조도 | §5.1 |
| `ch05_sec01_stream_channel_map.svg` | STM32F411 DMA 요청 매핑 테이블 (주요 주변장치) | §5.1 |
| `ch05_sec02_polling_vs_dma_timeline.svg` | CPU 타임라인 비교 (폴링: CPU 점유 / DMA: CPU 자유) | §5.2-5.3 |
| `ch05_sec03_mem_to_mem_flow.svg` | Mem-to-Mem 전송 흐름 (Request → Transfer → IRQ → Callback) | §5.3 |
| `ch05_sec04_normal_vs_circular.svg` | Normal 모드 vs Circular 모드 (링 버퍼 미리보기) | §5.4 |
| `ch05_sec05_nvic_priority.svg` | DMA IRQ 우선순위 + Priority Inversion 시나리오 | §5.5 |

- 색상 팔레트: HAL(#1A73E8 파랑), Driver(#34A853 초록), DMA(#FF6D00 주황), App(#EA4335 빨강)
- 폰트: Noto Sans KR, 14px 기준
- ASCII art 절대 금지 — SVG만 사용

---

## 6. 챕터 구조 (7개 절)

### §5.0 도입 + 아키텍처 위치 (~2,500자)
- "CPU가 택배를 직접 나르고 있다" — Ch04 폴링의 한계 상기
- 아키텍처 위치: DMA = 횡단 관심사 (모든 Driver 공통)
- v0.4 → v0.5 아키텍처 다이어그램 갱신
- 인터페이스 설계: DMA는 HAL이 감싸므로 별도 Driver 인터페이스 불필요
- 시퀀스 다이어그램: App → Driver → HAL → DMA Controller → Memory/Periph
- aside: 📌 강사 꿀팁 — "DMA 없이 UART 100KB 전송하면?" 시연 제안

### §5.1 DMA 컨트롤러 해부 (~3,000자)
- STM32F411 DMA1/DMA2 구조
- Stream, Channel, Priority 개념
- DMA 요청 매핑 (USART2 = DMA1, SPI1 = DMA2 등)
- CubeMX에서 DMA 설정 방법 (스크린샷 대신 텍스트 설명)
- aside: ❓ "Stream과 Channel 차이가 뭔가요?" FAQ
- aside: 🎯 면접 포인트 — DMA Stream/Channel 매핑 질문

### §5.2 폴링으로 메모리 복사 — 기준선 측정 (~2,500자)
- for 루프 memcpy 구현
- DWT Cycle Counter 활용한 정밀 시간 측정
- CPU가 복사 중 다른 일을 못하는 문제 시각화
- 코드 예제: `ch05_01_memcpy_polling.c`
- aside: 💡 실무 팁 — DWT Cycle Counter 초기화 방법

### §5.3 DMA로 메모리 복사 — CPU 해방 (~3,500자)
- CubeMX DMA2 Stream0 설정 (Mem-to-Mem)
- `HAL_DMA_Start_IT()` 사용법
- 콜백 함수: `XferCpltCallback`, `XferErrorCallback`
- 전송 완료 후 데이터 검증 (src vs dst 비교)
- 코드 예제: `ch05_02_dma_mem_to_mem.c`
- aside: ❓ "DMA 전송 중 CPU가 같은 메모리에 접근하면?" FAQ (버스 경합 개념)
- aside: 🔍 스스로 점검 — 콜백이 호출되지 않는다면 어디를 확인?

### §5.4 성능 대결 — 폴링 vs DMA (~3,000자)
- 100회 반복 측정 코드
- 결과 테이블: 시간, CPU 점유율, 처리량
- LED 깜빡임으로 CPU 해방 체감
- Normal 모드 vs Circular 모드 개념 소개
- 코드 예제: `ch05_03_performance_compare.c`
- aside: 📌 강사 꿀팁 — 측정 결과를 학생에게 예측하게 하면 효과적
- aside: 🎯 면접 포인트 — "폴링/인터럽트/DMA 비교" (Ch04 연습문제 8번 정답 완성)

### §5.5 NVIC 심화 — 우선순위와 Critical Section (~3,000자)
- Ch01 NVIC 기초 복습 → DMA 인터럽트 우선순위 설정
- `HAL_NVIC_SetPriority()` 상세 (PreemptPriority / SubPriority)
- Priority Inversion(우선순위 역전) 시나리오
  - 높은 우선순위 태스크가 낮은 우선순위 DMA 완료를 기다리는 상황
- Critical Section: `__disable_irq()` / `__enable_irq()` 패턴
- aside: 💡 실무 팁 — NVIC 우선순위 배치 전략 (DMA > UART > TIM)
- aside: ❓ "인터럽트를 끄면 DMA도 멈추나요?" FAQ (정답: 아니오, DMA는 독립 동작)

### §5.6 정리 + Ch06 예고 (~1,500자)
- 핵심 3줄 요약
- 아키텍처 v0.5 최종 다이어그램
- Ch06 예고: uart_driver.c 내부를 DMA로 교체 — 인터페이스는 변경 없음!
- aside: 🔍 스스로 점검 — 3가지 전송 모드, 각각 어떤 챕터에서 사용되는지

---

## 7. 비유 전략

| 개념 | 비유 | 근거 |
|------|------|------|
| DMA 전체 | 자동 서빙 로봇 (Ch03 레스토랑 비유 확장) | 기존 비유 체계 유지 |
| 폴링 복사 | 요리사가 직접 접시를 나름 | CPU가 모든 데이터 이동 담당 |
| DMA 복사 | 서빙 로봇이 나르고, 요리사는 다음 요리 시작 | CPU 해방 핵심 메시지 |
| Stream | 서빙 로봇의 이동 경로 (복도) | 물리적 전송 채널 |
| Channel | 주문 번호 (어느 테이블 음식인지) | 어느 주변장치 요청인지 |
| Priority | 긴급 주문 vs 일반 주문 | VIP 테이블 우선 서빙 |
| Circular 모드 | 회전초밥 컨베이어 벨트 | 끝나면 자동으로 처음부터 반복 |
| Normal 모드 | 일회성 배달 | 한 번 전송하고 정지 |

---

## 8. aside 박스 배치 계획

| 유형 | 개수 | 주요 내용 |
|------|------|-----------|
| 💡 실무 팁 | 3개 | DWT Counter, NVIC 전략, DMA 디버깅 |
| ❓ 단골 질문 | 4개 | Stream vs Channel, 버스 경합, 인터럽트 vs DMA, Circular 언제 쓰나 |
| 🎯 면접 포인트 | 2개 | DMA 매핑, 폴링/인터럽트/DMA 3종 비교 |
| 📌 강사 꿀팁 | 3개 | 시연 제안, 예측 퀴즈, 시간 부족 축소안 |
| 🔍 스스로 점검 | 2개 | 콜백 미호출 디버깅, 전송 모드-챕터 매핑 |

---

## 9. 연습문제 구성 (블룸 분류체계)

| 수준 | 문제 | 내용 |
|------|------|------|
| 기억 | 1 | DMA 3가지 전송 모드 나열 + 각 모드의 소스/목적지 |
| 기억 | 2 | STM32F411에서 Mem-to-Mem이 가능한 DMA 컨트롤러는? |
| 이해 | 3 | 폴링 복사와 DMA 복사의 CPU 점유율 차이 설명 |
| 이해 | 4 | Normal 모드와 Circular 모드 차이 + 각각 적합한 사용 사례 |
| 적용 | 5 | DMA 전송 크기를 2048 word로 변경하고 성능 재측정 |
| 적용 | 6 | DMA 전송 완료 콜백에서 검증 코드 추가 (src[i] == dst[i]) |
| 분석 | 7 | DMA 우선순위를 Low로 설정하면 성능에 어떤 영향? |
| 분석 | 8 | (면접 대비) "폴링/인터럽트/DMA를 CPU 점유율, 구현 복잡도, 최대 처리량, 지연시간 관점에서 비교" — Ch04 연습문제 8번 완전판 |

---

## 10. Ch04 → Ch05 연계 포인트

1. **Ch04 연습문제 8번 완성**: Ch04에서 "DMA는 Ch06에서 학습 예정"이라고 보류한 답을 이 장에서 완성
2. **uart_send_string() 활용**: 성능 측정 결과를 PC 터미널에 출력 (Ch04 Driver 재사용)
3. **폴링 vs 인터럽트 복습**: Ch04 §4.3-4.4에서 학습한 비교에 DMA 축 추가
4. **아키텍처 연속성**: v0.3(설계도) → v0.4(uart_driver 추가) → v0.5(DMA 레이어 추가)
5. **Ch06 예고 강화**: "uart_driver.h 인터페이스 변경 없이 DMA 전환" — Ch04에서 이미 복선

---

## 11. 리스크 및 주의사항

| 리스크 | 대응 |
|--------|------|
| "DMA가 왜 필요한지 체감 못함" | 성능 측정 + LED 시각화로 체감 유도 |
| "Stream/Channel 매핑이 복잡" | CubeMX가 자동 처리한다는 점 강조, 수동 설정은 참고 수준 |
| "Mem-to-Mem이 실무에서 쓸모있나?" | 실무 사례 제시 (프레임버퍼 복사, 로그 버퍼 백업) + Ch06 연계 동기 부여 |
| "NVIC 심화가 갑자기 어려움" | Ch01 복습 + 단계적 확장, Priority Inversion은 시나리오 중심 설명 |
| "DWT Cycle Counter 생소" | 별도 aside로 초기화 코드 제공, 이후 챕터에서 반복 사용 |

---

## 12. 예상 산출물

- **HTML**: `manuscripts/part1/chapter05.html` (~18,000자, 7개 절)
- **코드**: 3개 파일
  - `code_examples/ch05_01_memcpy_polling.c`
  - `code_examples/ch05_02_dma_mem_to_mem.c`
  - `code_examples/ch05_03_performance_compare.c`
- **SVG**: 8개 다이어그램 (§5.0~5.5)
- **예상 작업 시간**: Phase 2 초안 집필 약 10시간
