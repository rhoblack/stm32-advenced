# Ch04. UART 기초 — 기술 저자 Phase 1 기획 제안

**작성자**: 기술 저자
**대상 챕터**: Ch04. UART 기초 (UART Basics)
**프로젝트 버전**: v0.4
**누적 기능**: PC로 시스템 상태 시리얼 출력
**예상 총 분량**: ~18,000자 (5절, 절당 3,000~4,000자)

---

## 학습 목표 (Bloom's Taxonomy)

1. **[기억]** UART 프레임의 5대 구성 요소(Start, Data, Parity, Stop, Baud Rate)를 열거할 수 있다.
2. **[이해]** Baud Rate 설정이 통신 양측의 클럭 동기화를 대체하는 원리를 설명할 수 있다.
3. **[적용]** HAL_UART_Transmit/Receive로 폴링 기반 송수신을 구현할 수 있다.
4. **[적용]** HAL_UART_Receive_IT 1바이트 재등록 패턴으로 인터럽트 기반 수신을 구현할 수 있다.
5. **[분석]** 폴링 vs 인터럽트 방식의 CPU 점유율 차이를 로그로 측정하고 비교할 수 있다.

---

## 절 구성 (5개 절)

### §4.1 UART — MCU의 첫 번째 대화 채널 (~3,200자)

**학습 목표**: [기억/이해] UART 프레임 구조와 Baud Rate의 원리 이해

**핵심 개념** (새 개념 3개):
1. UART(범용 비동기 송수신기, Universal Asynchronous Receiver/Transmitter)
2. Baud Rate(보 레이트) — 초당 비트 전송 속도
3. UART 프레임 구조 (Start/Data/Parity/Stop)

**비유**: "전화 통화 비유"
- UART = 양쪽이 같은 속도로 말하기로 약속한 전화 통화
- Baud Rate = 말하는 속도 약속 (115200bps = 초당 115,200글자)
- Start bit = "여보세요", Stop bit = "끊을게요"
- Parity = "방금 말한 거 맞지?" 확인
- **비유의 한계**: 전화는 양방향 동시 가능(Full Duplex)이지만, 실제 UART도 Full Duplex — 이 비유에서는 잘 맞음. 다만 전화는 아날로그 연속 신호이고 UART는 디지털 이산 신호라는 차이

**다이어그램**:
- `ch04_sec01_uart_frame.svg` — UART 프레임 구조 (타이밍 다이어그램: Idle → Start → D0~D7 → Parity → Stop → Idle)
- `ch04_sec01_baudrate_mismatch.svg` — Baud Rate 불일치 시 데이터 깨짐 시각화

**aside 박스**:
- FAQ: "Q. 왜 115200이 기본인가?" → 역사적 관례 + NUCLEO ST-Link 기본값
- 면접 포인트: "UART vs USART 차이를 설명하시오" → Synchronous 클럭 라인 유무
- 실무 팁: 실무에서 흔한 Baud Rate 불일치 디버깅 (깨진 문자 패턴)

---

### §4.2 아키텍처 위치 — Driver 레이어의 첫 등장 (~3,500자)

**학습 목표**: [이해/적용] Ch03에서 설계한 4계층 구조에 uart_driver를 배치하고 인터페이스를 설계

**핵심 개념** (새 개념 2개):
1. Driver 레이어의 역할 — HAL을 감싸서 상위에 깔끔한 API 제공
2. 인터페이스 설계 원칙 — 상위 레이어는 HAL 함수를 직접 호출하지 않는다

**핵심 내용**:
- Ch03 아키텍처 v0.3 다이어그램에서 Driver 레이어(초록) 영역에 `uart_driver.c/h` 배치
- 공개 인터페이스 설계:
  ```c
  /* uart_driver.h — 공개 인터페이스 */
  typedef enum {
      UART_OK = 0,
      UART_ERROR,
      UART_BUSY,
      UART_TIMEOUT
  } UART_Status;

  UART_Status UART_Init(uint32_t baudrate);
  UART_Status UART_Send(const char *buf, uint16_t len);
  UART_Status UART_SendString(const char *str);
  uint16_t    UART_Available(void);
  uint8_t     UART_ReadByte(void);
  ```
- 시퀀스 다이어그램: App → UART Driver → HAL_UART → USART2 하드웨어 → PC

**다이어그램**:
- `ch04_sec00_architecture.svg` — 전체 4계층에서 uart_driver 강조 (Driver 레이어 초록색 강조)
- `ch04_sec00_sequence.svg` — App → UART Driver → HAL → HW 시퀀스 다이어그램

**aside 박스**:
- FAQ: "Q. HAL_UART_Transmit을 직접 쓰면 안 되나요?" → 레이어 위반, Ch03 복습
- 강사 꿀팁: "Driver 레이어를 만드는 이유를 Ch06 DMA 전환으로 미리 예고하세요"
- 스스로 점검: "uart_driver.h에 HAL 헤더가 include되어 있다면 인터페이스 설계가 맞는 것인가?"

---

### §4.3 폴링 방식 UART 송수신 — HAL_UART_Transmit/Receive (~3,800자)

**학습 목표**: [적용] HAL 폴링 API로 UART 송수신 구현 + printf 리다이렉트

**핵심 개념** (새 개념 3개):
1. HAL_UART_Transmit (폴링 송신) — 블로킹 방식
2. HAL_UART_Receive (폴링 수신) — 타임아웃 설정
3. printf 리다이렉트 (`_write` 오버라이드)

**비유**: "편의점 택배 비유"
- 폴링 송신 = 택배 기사가 올 때까지 편의점 앞에서 기다리는 것 (블로킹)
- 타임아웃 = "30분 기다려도 안 오면 포기" 설정

**코드 예제**:
- CubeMX USART2 설정 (NUCLEO-F411RE: PA2=TX, PA3=RX, ST-Link Virtual COM Port)
- `uart_driver.c` 폴링 모드 구현 (UART_Init, UART_Send, UART_SendString)
- `_write()` 오버라이드 — printf → UART 리다이렉트
- 테스트 코드: 버튼 누르면 시스템 상태 출력

**다이어그램**:
- `ch04_sec03_polling_timeline.svg` — 폴링 송신 시 CPU 타임라인 (블로킹 구간 강조)

**aside 박스**:
- 실무 팁: "HAL_MAX_DELAY 사용 시 무한 대기 주의 — 실무에서는 반드시 타임아웃 설정"
- FAQ: "Q. printf를 쓰면 코드가 느려지지 않나요?" → Release 빌드에서 LOG 레벨로 제거
- 면접 포인트: "폴링 방식 UART의 단점을 설명하시오" → CPU 블로킹, 실시간성 저하

---

### §4.4 인터럽트 기반 수신 — 1바이트 재등록 패턴 (~4,000자)

**학습 목표**: [적용/분석] HAL_UART_Receive_IT 1바이트 재등록 패턴 구현 및 폴링 대비 장점 분석

**핵심 개념** (새 개념 3개):
1. HAL_UART_Receive_IT — 인터럽트 기반 비동기 수신
2. 1바이트 재등록 패턴 — 콜백에서 다음 1바이트 수신 재등록
3. HAL_UART_RxCpltCallback — 수신 완료 콜백

**비유**: "식당 호출벨 비유"
- 폴링 = 주방에서 계속 홀을 쳐다보며 손님 확인 (다른 일 못함)
- 인터럽트 = 손님이 벨을 누르면 그때 가서 확인 (평소엔 다른 일 가능)
- 1바이트 재등록 = 벨 한 번에 한 명 처리, 처리 후 벨 다시 활성화
- **비유의 한계**: 실제 인터럽트는 현재 작업을 "중단"하고 처리하지만, 식당에서는 요리를 중간에 멈추지 않음

**코드 예제**:
- `uart_driver.c` 인터럽트 모드 추가: 수신 버퍼 + 재등록 패턴
- HAL_UART_RxCpltCallback 구현
- 간이 수신 버퍼 (배열 기반, 링 버퍼는 Ch06에서)
- 테스트: PC에서 문자 전송 → MCU에서 에코(echo) + LED 토글

**다이어그램**:
- `ch04_sec04_interrupt_flow.svg` — 인터럽트 수신 흐름도 (HW RX → NVIC → Callback → 재등록 → 버퍼 저장)
- `ch04_sec04_polling_vs_interrupt.svg` — CPU 타임라인 비교 (폴링: 빨간 블로킹 구간 vs 인터럽트: 짧은 ISR만)

**aside 박스**:
- 실무 팁: "재등록을 깜빡하면 첫 1바이트만 수신된다 — 가장 흔한 실수"
- FAQ: "Q. 왜 한 번에 여러 바이트를 받지 않나요?" → 가변 길이 수신에 유연, Ch06 DMA IDLE로 개선 예고
- 강사 꿀팁: "라이브 코딩에서 일부러 재등록을 빼고 첫 바이트만 수신되는 장면을 보여주세요"
- 면접 포인트: "UART 인터럽트 수신에서 1바이트 재등록 패턴을 설명하시오"

---

### §4.5 통합 실습 — 시스템 상태 시리얼 모니터 (~3,500자)

**학습 목표**: [적용/분석] Ch01~04 코드를 통합하여 시스템 상태를 PC로 출력하는 v0.4 완성

**핵심 개념** (새 개념 1개):
1. 시스템 상태 출력 포맷 설계 — 구조화된 텍스트 출력

**내용**:
- v0.3 → v0.4 업그레이드: uart_driver 추가
- main.c 통합: 버튼 인터럽트(Ch01) + 로그(Ch02) + 아키텍처(Ch03) + UART(Ch04)
- 1초 주기 시스템 상태 출력: LED 상태, 버튼 카운트, 가동 시간(uptime)
- PC에서 명령 수신: 'S' → 상태 출력, 'L' → LED 토글, 'R' → 카운터 리셋
- 아키텍처 다이어그램 갱신: v0.4에 uart_driver 추가

**코드 예제**:
- `ch04_system_monitor.c` — 통합 main 코드 (v0.4)
- `ch04_uart_driver.c` / `ch04_uart_driver.h` — 완성 드라이버

**다이어그램**:
- `ch04_sec05_v04_architecture.svg` — v0.4 아키텍처 (uart_driver 추가된 전체 다이어그램)

**aside 박스**:
- 스스로 점검: "uart_driver.c를 다른 프로젝트에 복사해도 main.c 수정 없이 동작하는가?"
- 실무 팁: "시리얼 모니터 출력 포맷을 정해두면 나중에 파싱이 쉽다 (Ch16 대시보드 복선)"
- 강사 꿀팁: "실습 후 학생들에게 v0.3 → v0.4 아키텍처 변화를 직접 그려보게 하세요"

---

## 코드 예제 파일 목록

| 파일명 | 설명 | 등장 절 |
|--------|------|---------|
| `code_examples/ch04_uart_driver.h` | UART 드라이버 공개 인터페이스 | §4.2 |
| `code_examples/ch04_uart_driver.c` | UART 드라이버 구현 (폴링 + 인터럽트) | §4.3, §4.4 |
| `code_examples/ch04_system_monitor.c` | 통합 main — v0.4 시스템 모니터 | §4.5 |

## SVG 다이어그램 목록

| 파일명 | 설명 | 등장 절 |
|--------|------|---------|
| `figures/ch04_sec00_architecture.svg` | 4계층 아키텍처에서 uart_driver 위치 | §4.2 (도입부) |
| `figures/ch04_sec00_sequence.svg` | App → UART Driver → HAL → HW 시퀀스 | §4.2 (도입부) |
| `figures/ch04_sec01_uart_frame.svg` | UART 프레임 타이밍 다이어그램 | §4.1 |
| `figures/ch04_sec01_baudrate_mismatch.svg` | Baud Rate 불일치 시 데이터 깨짐 | §4.1 |
| `figures/ch04_sec03_polling_timeline.svg` | 폴링 송신 CPU 타임라인 | §4.3 |
| `figures/ch04_sec04_interrupt_flow.svg` | 인터럽트 수신 흐름도 | §4.4 |
| `figures/ch04_sec04_polling_vs_interrupt.svg` | 폴링 vs 인터럽트 CPU 비교 | §4.4 |
| `figures/ch04_sec05_v04_architecture.svg` | v0.4 전체 아키텍처 (최종) | §4.5 |

**총 SVG 8개** (도입부 아키텍처 2개 + 본문 6개)

---

## 교수법 핵심 전략

### 1. 감정 곡선 설계
- **§4.1**: 호기심 — "MCU가 PC와 대화한다고?" + 전화 비유로 친숙하게
- **§4.2**: 약간의 불안 — "Driver 레이어를 직접 만들어야 한다" + Ch03 복습으로 안심
- **§4.3**: 성취감 — "printf로 PC에 메시지 출력 성공!" (시각적 피드백 즉시)
- **§4.4**: 약간의 불안 → 이해 — "인터럽트 재등록 실수" → 원리 이해 후 해결
- **§4.5**: 큰 성취감 — "PC에서 MCU를 원격 제어한다!" (v0.4 완성)

### 2. 누적 성장 연결고리
- **Ch01 연결**: 버튼 인터럽트 → UART로 이벤트 전송
- **Ch02 연결**: LOG_I/D 매크로 → UART 드라이버 디버깅에 활용
- **Ch03 연결**: 4계층 아키텍처 → uart_driver를 Driver 레이어에 배치
- **Ch05 예고**: "폴링의 블로킹 문제 → Ch05 DMA로 해결"
- **Ch06 예고**: "간이 버퍼 → Ch06 링 버퍼로 업그레이드"

### 3. 수강생 예상 막힘 포인트
1. **CubeMX 설정 실수**: USART2 활성화 안 함 / PA2,PA3 핀 확인 안 함
2. **Baud Rate 불일치**: MCU와 PC 터미널의 Baud Rate가 다름 → 깨진 문자
3. **인터럽트 재등록 누락**: 첫 1바이트만 수신되고 이후 무반응
4. **`_write` 오버라이드 위치**: syscalls.c vs main.c 혼동
5. **ST-Link Virtual COM Port 미인식**: 드라이버 설치 안내 필요

### 4. Ch03과의 아키텍처 연결 (중요)
- Ch04는 **Driver 레이어가 처음 실체화**되는 챕터
- Ch03에서 "레스토랑 주방 비유"로 설명한 레이어를 실제 코드로 구현
- 학생이 "아, 이래서 레이어를 나누는구나"를 체감하는 전환점
- 인터페이스 설계 원칙(Ch03)이 실제 .h 파일로 구현됨을 명확히 연결

---

## 특이사항

### NUCLEO-F411RE USART2 핀 배치
- **USART2_TX**: PA2 (CN10 Pin 35) — ST-Link Virtual COM Port 연결
- **USART2_RX**: PA3 (CN10 Pin 37) — ST-Link Virtual COM Port 연결
- CubeMX에서 USART2를 Asynchronous로 설정하면 자동 배정
- **주의**: PA2/PA3는 ST-Link에 하드와이어드이므로 다른 용도로 사용 불가

### Ch06 DMA 전환을 위한 설계 복선
- `uart_driver.h` 인터페이스를 DMA 전환 시에도 변경 없도록 설계
- 내부 구현만 교체 (폴링 → 인터럽트 → DMA) — 레이어 분리의 실질적 가치 입증
- §4.2에서 이 설계 의도를 명시적으로 언급하여 학생의 "왜 이렇게 복잡하게?" 의문 선제 해소
