# Ch14. I2C DMA 심화 — Phase 1 기획 회의 통합 결과

작성일: 2026-03-17
버전: v1.4

---

## Sub-agent 1: 기술 저자 관점

### 각 절 핵심 메시지

| 절 | 제목 | 핵심 메시지 |
|----|------|------------|
| 1절 | I2C DMA 전환 이유와 아키텍처 | 폴링 방식은 CPU가 I2C 완료를 기다리며 블로킹된다. DMA 전환으로 CPU를 해방시켜 동시 작업이 가능해진다. |
| 2절 | HAL I2C DMA API 및 콜백 구조 | HAL_I2C_Master_Transmit_DMA / Receive_DMA API와 완료 콜백 HAL_I2C_MasterTxCpltCallback / RxCpltCallback 구조를 이해한다. |
| 3절 | Repeated Start 미지원 문제 해결 | STM32 HAL의 I2C DMA는 Repeated Start를 지원하지 않으므로, Write→Stop→Read 분리 패턴과 콜백 체인으로 우회한다. |
| 4절 | 다중 DMA 동시 운용 및 우선순위 설정 | STM32F411의 DMA 스트림 리소스는 유한하다. UART/SPI/I2C DMA 동시 운용 시 스트림 충돌과 우선순위를 설계한다. |
| 5절 | 연속 측정 루프 + 실무 검증 | 콜백 기반 자동 재시작 루프로 SHT31 온습도를 주기적으로 비동기 수집하고, 데이터 일관성을 LOG로 검증한다. |

### HAL 코드 예제 목록

1. `ch14_i2c_dma_init.c` — I2C1 DMA 모드 CubeMX 설정 및 HAL 초기화
2. `ch14_i2c_dma_driver.c` — i2c_driver.c DMA 모드 전환 (Transmit/Receive DMA)
3. `ch14_i2c_dma_driver.h` — i2c_driver.h 공개 인터페이스
4. `ch14_sht31_dma.c` — SHT31 드라이버 DMA 비동기 전환 (콜백 체인 패턴)
5. `ch14_multi_dma.c` — UART DMA + SPI DMA + I2C DMA 동시 운용 예제

### SVG 다이어그램 목록

1. `ch14_sec00_architecture.svg` — 레이어 아키텍처 위치도 (Driver 레이어 I2C DMA 강조)
2. `ch14_sec00_sequence.svg` — 3종 DMA 동시 운용 시퀀스 다이어그램
3. `ch14_sec01_polling_vs_dma.svg` — 폴링 vs DMA CPU 타임라인 비교
4. `ch14_sec03_repeated_start.svg` — Repeated Start 미지원 우회 패턴 (Write→Stop→Read 콜백 체인)
5. `ch14_sec04_dma_stream_map.svg` — STM32F411 DMA1/DMA2 스트림-채널 배치표

### STM32F411 DMA 스트림/채널 배치표 (충돌 분석)

#### DMA1 스트림 배치 (I2C, UART, SPI 관련)

| 스트림 | 채널 0 | 채널 1 | 채널 2 | 채널 3 | 채널 4 | 채널 5 | 채널 6 | 채널 7 |
|--------|--------|--------|--------|--------|--------|--------|--------|--------|
| Stream 0 | SPI3_RX | - | SPI3_RX | SPI2_RX | SPI2_RX | SPI3_RX | - | - |
| Stream 1 | - | - | - | SPI2_TX | - | - | - | - |
| Stream 2 | SPI3_RX | - | - | - | I2C3_RX | - | - | I2C2_RX |
| Stream 3 | SPI2_RX | - | - | I2C3_TX | - | SPI2_TX | TIM5_CH4 | I2C2_RX |
| Stream 4 | - | - | - | - | - | I2C3_TX | - | - |
| Stream 5 | - | I2C1_RX | - | - | - | I2C1_RX | - | - |
| Stream 6 | I2C1_TX | - | - | I2C1_TX | - | - | USART2_TX | - |
| Stream 7 | - | - | I2C1_TX | - | - | SPI3_TX | - | USART2_TX |

**Ch14 프로젝트 v1.4 DMA 할당 (권장):**
- **UART2 TX**: DMA1 Stream6 Ch4 (우선순위: Medium)
- **UART2 RX**: DMA1 Stream5 Ch4 (우선순위: Medium)
- **SPI1 TX**: DMA2 Stream3 Ch3 (우선순위: High — LCD 전송 우선)
- **SPI1 RX**: DMA2 Stream0 Ch3 (우선순위: High)
- **I2C1 TX**: DMA1 Stream6 Ch1 → **충돌! UART2 TX와 Stream6 공유**
  - **해결**: I2C1 TX를 DMA1 Stream7 Ch1로 변경
- **I2C1 RX**: DMA1 Stream0 Ch1 (우선순위: Low — 센서 데이터는 낮음)

**충돌 분석 결론:**
- I2C1 TX(Stream6 Ch1)와 UART2 TX(Stream6 Ch4)는 동일 스트림 → 동시 사용 불가
- I2C1 TX를 Stream7 Ch1로 재배치하면 충돌 해소
- SPI1(DMA2)은 별도 컨트롤러이므로 DMA1과 충돌 없음

### Repeated Start 미지원 우회 패턴 설계

SHT31 온습도 측정 시퀀스:
```
[정상 I2C 프로토콜]
START → ADDR+W → CMD_HIGH → CMD_LOW → REPEATED_START → ADDR+R → DATA(6bytes) → STOP

[HAL DMA 우회 패턴]
Phase 1: HAL_I2C_Master_Transmit_DMA(addr, cmd, 2)
         → HAL_I2C_MasterTxCpltCallback 호출
Phase 2: (콜백에서) HAL_I2C_Master_Receive_DMA(addr, buf, 6)
         → HAL_I2C_MasterRxCpltCallback 호출
Phase 3: (콜백에서) 데이터 파싱 + 다음 측정 타이머 시작
```

이 패턴의 핵심: Repeated Start 대신 Stop→Start로 두 트랜잭션을 분리하되, SHT31은 이를 허용한다 (측정 시작 명령 후 센서가 내부에서 처리 후 응답).

---

## Sub-agent 2: 교육 설계자 관점

### 학습 목표 (4개, "~할 수 있다" 형태)

1. **[이해]** I2C 폴링 방식과 DMA 방식의 CPU 점유율 차이를 설명하고, DMA 전환이 필요한 시나리오를 판단할 수 있다.
2. **[적용]** HAL_I2C_Master_Transmit_DMA / Receive_DMA를 사용하여 SHT31 온습도 센서를 비동기 방식으로 구동하는 드라이버를 작성할 수 있다.
3. **[분석]** STM32F411의 DMA 스트림-채널 배치표를 해석하여 UART/SPI/I2C DMA 동시 운용 시 발생하는 충돌을 진단하고 해소할 수 있다.
4. **[창조]** 콜백 체인 패턴으로 Repeated Start 미지원 문제를 우회하고, 연속 측정 루프를 설계할 수 있다.

### 누적 성장 연결점

#### Ch05(DMA 아키텍처) → Ch14 연결
- Ch05에서 배운 DMA 스트림/채널 개념, Periph-to-Mem 모드, 완료 콜백 구조를 I2C에 적용
- Ch05에서 배운 DMA 우선순위 설정(HAL_NVIC_SetPriority)을 다중 DMA 충돌 해소에 활용

#### Ch11(I2C 기초) → Ch14 연결
- Ch11에서 작성한 HAL_I2C_Master_Transmit/Receive(폴링) 코드를 DMA 버전으로 업그레이드
- Ch11의 SHT31 CRC-8 검증 로직은 그대로 유지 (DMA는 전송 방식만 변경)
- Ch11의 인터페이스 SHT31_Read()를 비동기 버전 SHT31_StartMeasurement() + 콜백으로 분리

#### Ch06(UART DMA) + Ch12(SPI DMA) → Ch14 연결
- Ch06에서 경험한 DMA TX/RX 패턴을 I2C에도 동일하게 적용
- Ch12에서 SPI DMA를 이미 운용 중 → 이 상태에서 I2C DMA 추가 시 스트림 충돌 가능성

### 학습 흐름 설계 (도입→개념→예시→실습→정리)

```
도입 (15분):   폴링 vs DMA CPU 점유율 실측 비교 — "왜 DMA가 필요한가" 동기 부여
개념 (45분):   HAL I2C DMA API + Repeated Start 문제 + DMA 스트림 충돌 분석
예시 (45분):   i2c_driver.c DMA 전환 코드 분석 + SHT31 콜백 체인 패턴 설명
실습 (90분):   CubeMX DMA 설정 → 코드 적용 → 동작 확인 → 충돌 시나리오 재현
정리 (45분):   핵심 정리 + 연습문제 + 다음 챕터 예고
총계: 240분 (4시간)
```

---

## Sub-agent 3: 강사 관점

### 수강생 막힘 Top 5 및 해소 방법

#### 막힘 1: "DMA 설정을 CubeMX에서 했는데 왜 I2C DMA가 동작하지 않나요?"

**원인**: CubeMX에서 I2C1의 DMA Request를 추가하지 않은 경우. DMA 탭에서 "Add"를 눌러 I2C1_RX, I2C1_TX를 명시적으로 추가해야 함.

**해소**: CubeMX 설정 스크린샷을 단계별로 제공. "DMA Settings 탭 → Add 버튼 → I2C1_RX 선택 → Direction: Peripheral to Memory" 순서 명시.

#### 막힘 2: "HAL_I2C_MasterTxCpltCallback에서 Receive를 호출했는데 HAL_BUSY 오류가 납니다"

**원인**: TX 완료 콜백이 DMA 인터럽트 컨텍스트에서 호출된 직후, HAL I2C 상태가 READY로 전환되기 직전에 Receive를 호출하면 HAL_BUSY 반환.

**해소**: 콜백에서 직접 호출하지 않고, 플래그(i2c_tx_done = 1)를 세팅하고 메인 루프에서 폴링하거나, osDelay(1) + 재시도 패턴 사용. 또는 HAL_GetTick() 기반 타임아웃 체크.

#### 막힘 3: "DMA 스트림 충돌이 뭔가요? CubeMX가 자동으로 막아주지 않나요?"

**원인**: CubeMX는 동일 스트림에 다른 채널을 배치하는 것을 경고하지만, 사용자가 무시하면 허용함. 런타임에서 두 요청이 동시에 들어오면 예측 불가 동작.

**해소**: DMA 스트림 배치표를 화이트보드에 직접 그리며 설명. "스트림은 차선, 채널은 차선 번호다. 같은 차선에는 차 한 대만 달릴 수 있다" 비유 사용.

#### 막힘 4: "콜백이 두 번 불리는 것 같아요 (또는 안 불려요)"

**원인**: 재진입(re-entrant) 문제. 콜백 내에서 DMA 전송을 다시 시작하면, 이전 전송의 TC(Transfer Complete) 인터럽트가 중첩될 수 있음. 또는 DMA 인터럽트 우선순위가 잘못 설정되어 마스킹됨.

**해소**: 콜백에 LOG_D 추가하여 호출 횟수 확인. 우선순위 확인: DMA 인터럽트 우선순위 < I2C 이벤트 인터럽트 우선순위 권장.

#### 막힘 5: "Repeated Start가 뭔가요? 폴링에서는 문제없었는데 DMA에서만 문제가 되나요?"

**원인**: 폴링 HAL_I2C_Master_Transmit/Receive는 내부적으로 I2C_FIRST_AND_NEXT_FRAME 옵션을 사용하여 Repeated Start를 자동 처리. DMA 버전은 이 옵션을 지원하지 않아 항상 STOP 후 새 START를 발행.

**해소**: I2C 타이밍 다이어그램 SVG로 Repeated Start vs Stop+Start 차이 시각화. "SHT31은 Stop+Start도 허용하므로 실제로는 문제없다" 안심 메시지 추가.

### 강의 현장 추가 팁

- **복습 포인트**: 수업 시작 전 Ch11 SHT31_Read() 코드 5분 복습 (DMA 전환 전 기준점 확인)
- **데모 시나리오**: 폴링 모드로 SHT31 읽기 → SWO로 CPU 점유 시간 측정 → DMA 전환 후 재측정 → 수치 비교로 효과 체감
- **흔한 오타**: `HAL_I2C_Master_Receive_DMA`의 파라미터 순서 혼동 (hi2c, DevAddress, pData, Size 순서)

---

## Phase 1 통합 결론

### 챕터 구성 확정

| 절 | 분량 목표 | 핵심 aside 박스 |
|----|----------|----------------|
| 도입 + 아키텍처 위치 | - | architecture SVG 2개 |
| 1절: I2C DMA 전환 이유 | 2500자 | tip, faq, metacognition |
| 2절: HAL I2C DMA API | 3000자 | tip, faq, interview |
| 3절: Repeated Start 우회 | 3000자 | tip, faq, instructor-tip |
| 4절: 다중 DMA 동시 운용 | 3000자 | tip, interview, metacognition |
| 5절: 연속 측정 루프 | 2500자 | tip, faq, instructor-tip |
| 실습 | - | tip |

### 품질 리스크 사전 식별

1. **기술 리스크**: DMA 스트림 번호가 RM0383 기준과 다를 경우 오류 → Reference Manual 재확인 필수
2. **교육 리스크**: Repeated Start 개념이 생소할 수 있음 → I2C 타이밍 SVG 충분히 활용
3. **심리 리스크**: HAL_BUSY 오류는 수강생이 막막함을 느끼는 구간 → "흔한 실수" 안심 박스 필수
