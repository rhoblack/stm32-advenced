# 목차 — STM32 고급 실무교육

## 프로젝트 개요
- **메인 프로젝트**: STM32 스마트 시계 & 환경 모니터링 시스템
- **대상 독자**: 중급 임베디드 개발자 (C언어 숙련, HAL 기본 사용 경험)
- **개발 환경**: STM32CubeIDE, HAL 라이브러리, NUCLEO-F411RE, Windows
- **학습 방식**: 누적 성장형 + 소프트웨어 아키텍처 일관 적용
- **챕터당 강의 시간**: 4시간 / **총 강의 시간**: 76시간

> **하드웨어 준비물**: NUCLEO-F411RE, ULN2003 + 28BYJ-48 스텝 모터, SHT31 모듈,
> ILI9341 TFT LCD 모듈, LSE 크리스탈 32.768kHz (SB48/SB49 장착 또는 LSI 대체)

---

## 아키텍처 적용 원칙 (전 챕터 공통)

> **Ch04부터 모든 챕터는 다음 섹션을 도입부에 포함한다:**
>
> **① 아키텍처 위치** — 전체 레이어 다이어그램(SVG)에서 이 챕터 컴포넌트의 위치
> **② 인터페이스 설계** — 상위 레이어에 노출하는 API 정의 (함수 시그니처)
> **③ FSM / 시퀀스 다이어그램** — 해당하는 경우 상태 전이도 또는 시퀀스 작성
> **④ 아키텍처 업데이트** — 챕터 완료 후 전체 아키텍처 다이어그램 갱신

---

## Part 0. 프로젝트 기반 구축

### Ch01. GPIO & EXTI — LED, Button
- 프로젝트 버전: `v0.1`
- 누적 기능: 버튼으로 LED 제어
- 핵심 주제:
  - GPIO 출력: LED 제어 (Push-Pull, Open-Drain)
  - GPIO 입력: Button 폴링 vs 인터럽트 비교
  - EXTI 설정: 엣지 트리거, 디바운싱 처리
  - HAL_GPIO_* API 실무 패턴
  - NVIC 기초: PreemptPriority / SubPriority 개념

### Ch02. 로그 시스템 구축
- 프로젝트 버전: `v0.2`
- 누적 기능: 전체 코드 표준 로그 인프라 확립 (이후 모든 챕터 적용)
- 핵심 주제:
  - SWO/ITM printf 리다이렉트
  - SEGGER RTT 활용
  - 로그 레벨 설계 (ERROR / WARN / INFO / DEBUG)
  - 매크로 설계: `LOG_E` / `LOG_W` / `LOG_I` / `LOG_D`
  - 컴파일 타임 레벨 필터링 (`#define LOG_LEVEL`)
  - ※ 링 버퍼 기반 비동기 로그는 Ch06(UART DMA) 이후 적용

### Ch03. 소프트웨어 아키텍처 설계 원칙 ← 신설
- 프로젝트 버전: `v0.3`
- 누적 기능: 전체 프로젝트 아키텍처 설계도 v1 작성 (이후 매 챕터 갱신)
- 핵심 주제:
  - **레이어드 아키텍처**: HAL → Driver → Service → App 4계층 구조
  - **레이어 설계 원칙**: 상위→하위 단방향 의존, 계층 간 인터페이스 정의
  - **FSM(유한 상태 머신) 기초**: 상태 / 전이 / 이벤트 / 액션 개념
  - **상태 전이 다이어그램 작성법**: SVG로 표현하는 방법
  - **시퀀스 다이어그램 기초**: 컴포넌트 간 메시지 흐름 표현
  - **프로젝트 아키텍처 초안 작성**: 현재 Ch01~02 코드를 레이어에 배치
  - 이후 챕터에서 이 다이어그램에 컴포넌트를 추가하는 방식으로 진행

---

## Part 1. 통신 기초 + DMA

### Ch04. UART 기초
- 프로젝트 버전: `v0.4`
- 누적 기능: PC로 시스템 상태 시리얼 출력
- **아키텍처 위치**: Driver 레이어 — `uart_driver.c/h`
- 핵심 주제:
  - UART 프레임 구조, Baud Rate 설정
  - HAL_UART_Transmit / Receive (폴링)
  - 인터럽트 기반 수신 — HAL_UART_Receive_IT 1바이트 재등록 패턴
  - printf 리다이렉트 (`_write` 오버라이드)
  - **인터페이스 설계**: `UART_Send(const char *buf, uint16_t len)`
  - **시퀀스 다이어그램**: App → UART Driver → HAL → 하드웨어

### Ch05. DMA 아키텍처와 동작 원리
- 프로젝트 버전: `v0.5`
- 누적 기능: Mem-to-Mem DMA 버퍼 복사 성능 측정 (폴링 대비)
- **아키텍처 위치**: HAL 레이어 하위 — 전 Driver에 적용되는 횡단 관심사
- 핵심 주제:
  - DMA 컨트롤러 구조 (Stream, Channel, Priority)
  - 폴링 vs 인터럽트 vs DMA 비교
  - F411 DMA 레지스터 구조, CubeMX 설정
  - Mem-to-Mem / Mem-to-Periph / Periph-to-Mem
  - 순환 모드(Circular), 더블 버퍼링
  - DMA 완료/반완료 콜백
  - **NVIC 심화**: HAL_NVIC_SetPriority, 우선순위 역전 방지, Critical Section

### Ch06. UART DMA 심화 + 링 버퍼
- 프로젝트 버전: `v0.6`
- 누적 기능: UART → DMA 비동기 전환 + 링 버퍼 로그 출력 완성
- **아키텍처 위치**: Driver 레이어 업그레이드 — `uart_driver.c` DMA 모드 전환
- **FSM**: TX_IDLE / TX_BUSY / RX_RECEIVING
- 핵심 주제:
  - TX DMA, RX DMA (IDLE Line 감지)
  - **링 버퍼(Ring Buffer) 직접 구현** — Ch02 로그 시스템에 통합
  - HAL_UART_Transmit_DMA / HAL_UARTEx_ReceiveToIdle_DMA
  - 링 버퍼 기반 비동기 로그 출력 완성 (Ch02 업그레이드)
  - 폴링 UART 대비 CPU 점유율 비교 측정

---

## Part 2. 타이머 + RTC + 스텝 모터

### Ch07. TIM 기초 (General Purpose Timer)
- 프로젝트 버전: `v0.7`
- 누적 기능: 정밀 주기 타이머 + PWM 출력
- **아키텍처 위치**: Driver 레이어 — `tim_driver.c/h`
- 핵심 주제:
  - STM32F411 타이머 계층: Basic / General Purpose / Advanced Control
  - PSC / ARR / CCR 레지스터 및 주기 계산
  - UEV(Update Event), HAL_TIM_Base_Start_IT
  - PWM 기본 (Edge-Aligned, HAL_TIM_PWM_Start)
  - TIM2~TIM5 (GP) vs TIM1 (Advanced) 구분
  - **인터페이스 설계**: `TIM_SetPeriod_ms()`, `PWM_SetDuty()`

### Ch08. Advanced TIM 심화
- 프로젝트 버전: `v0.8`
- 누적 기능: Complementary PWM + 스톱워치 구현
- **아키텍처 위치**: Driver 레이어 — `tim_driver.c` 심화 확장
- **FSM**: STOPWATCH — IDLE / RUNNING / PAUSED / RESET
- 핵심 주제:
  - PWM 고급: Complementary 출력, Dead-time 설정
  - NUCLEO-F411RE TIM1 핀 배치 (PA7/PB0/PB1 충돌 주의)
  - 입력 캡처(Input Capture), 출력 비교(Output Compare)
  - 원샷(One-Pulse) 모드
  - 타이머 연계 (Trigger / Master-Slave)
  - **시퀀스 다이어그램**: Button 이벤트 → Stopwatch FSM 전이

### Ch09. 내부 RTC 활용
- 프로젝트 버전: `v0.9`
- 누적 기능: 실시간 시각 + RTC 알람 트리거
- **아키텍처 위치**: Driver 레이어 — `rtc_driver.c/h`
- **FSM**: ALARM — IDLE / ARMED / TRIGGERED / ACKNOWLEDGED
- 핵심 주제:
  - RTC 아키텍처 (BCD 레지스터, 백업 도메인)
  - **NUCLEO-F411RE LSE 주의**: 기본 미장착 (SB48/SB49 확인), LSI 대체 시 ±5% 편차
  - CubeMX 설정 (LSE vs LSI 선택 기준)
  - 시간/날짜 설정 및 읽기 (HAL_RTC_Get*)
  - RTC 알람 (AlarmA / AlarmB) 인터럽트
  - RTC Wake-up 타이머, 백업 레지스터 활용
  - **인터페이스 설계**: `RTC_GetTime()`, `RTC_SetAlarm()`

### Ch10. 스텝 모터(ULN2003) 제어
- 프로젝트 버전: `v1.0` ← **마일스톤 ①**
- 누적 기능: 아날로그 시침 구동 (RTC 연동) — 스마트 시계 핵심 기능 완성
- **아키텍처 위치**: Driver + Service 레이어 — `stepper_driver.c`, `clock_service.c`
- **하드웨어 준비**: 28BYJ-48 + ULN2003 드라이버 보드, 사전 배선 확인 필수
- **FSM**: MOTOR — IDLE / MOVING / HOMING / ERROR
- 핵심 주제:
  - 28BYJ-48 사양: 기어비 64:1, 스트라이드 각 5.625°/스텝, 1회전 = 4096 하프 스텝
  - ULN2003 드라이버 회로, 여자 시퀀스 (풀/하프 스텝)
  - 가속/감속 프로파일, 위치 제어
  - RTC 시각 → 침 각도 변환 알고리즘
  - **시퀀스 다이어그램**: RTC 1초 인터럽트 → Clock Service → Motor Driver → 침 이동

---

## Part 3. 센서 + 통신 심화

### Ch11. I2C 통신 + SHT31 온습도 센서
- 프로젝트 버전: `v1.1`
- 누적 기능: 온습도 데이터 수집 + 온도 알람 연동
- **아키텍처 위치**: Driver + Service 레이어 — `i2c_driver.c`, `sht31_driver.c`, `sensor_service.c`
- 핵심 주제:
  - I2C 프로토콜 심화 (ACK/NACK, 클럭 스트레칭)
  - HAL_I2C_Master_Transmit / Receive (폴링)
  - SHT31 드라이버 설계 및 데이터 파싱
  - **SHT31 CRC-8 검증**: 다항식 0x31, 초기값 0xFF
  - 온도 임계값 알람 (RTC AlarmB 연동)
  - **인터페이스 설계**: `SHT31_Read(float *temp, float *humi)`

### Ch12. SPI DMA — ILI9341 TFT LCD 초기화
- 프로젝트 버전: `v1.2`
- 누적 기능: TFT LCD 초기화 + 기본 텍스트/도형 출력
- **아키텍처 위치**: Driver 레이어 — `spi_driver.c`, `ili9341_driver.c`
- 핵심 주제:
  - SPI 프로토콜 기초: CPOL/CPHA, NSS 처리, 클럭 속도 (42MHz 이하 권장)
  - ILI9341 하드웨어 인터페이스: D/C 핀, CS 핀 수동 제어, Reset 시퀀스
  - SPI DMA 초기화 및 전송 구조
  - ILI9341 초기화 커맨드 시퀀스
  - 기본 픽셀/사각형/텍스트 출력 실습
  - DMA 전송 완료 콜백 + CS 핀 타이밍
  - **시퀀스 다이어그램**: App → GFX → LCD Driver → SPI DMA → 하드웨어

### Ch13. ILI9341 GFX 레이어 + 화면 완성
- 프로젝트 버전: `v1.3`
- 누적 기능: 디지털 시각 + 온습도 + 스톱워치 화면 구성 완성
- **아키텍처 위치**: Service 레이어 — `gfx_service.c`, `ui_service.c`
- **FSM**: DISPLAY — TIME_VIEW / SENSOR_VIEW / STOPWATCH_VIEW
- 핵심 주제:
  - GFX 드라이버 레이어 설계 (LCD Driver → GFX Service → UI App)
  - 폰트 렌더링, 색상 팔레트
  - 화면 레이아웃 설계: 디지털 시각 / 온습도 / 스톱워치 영역
  - 프레임버퍼 DMA 전송 최적화
  - **아키텍처 다이어그램 갱신**: App → UI Service → GFX → LCD Driver → SPI DMA

### Ch14. I2C DMA 심화
- 프로젝트 버전: `v1.4`
- 누적 기능: SHT31 → DMA 비동기 고속화 (UART/SPI/I2C DMA 동시 운용)
- **아키텍처 위치**: Driver 레이어 업그레이드 — `i2c_driver.c` DMA 모드 전환
- **시퀀스 다이어그램**: 3종 DMA 동시 운용 — 우선순위 충돌 분석
- 핵심 주제:
  - HAL_I2C_Master_Transmit_DMA / Receive_DMA
  - **HAL I2C DMA 주의**: Repeated Start 미지원, Write→Read 별도 구현 패턴
  - 다중 DMA 동시 운용 아키텍처 (우선순위 설정)
  - I2C DMA 완료 콜백 기반 연속 측정 루프

---

## Part 4. CLI + PC 대시보드

### Ch15. UART CLI 구현
- 프로젝트 버전: `v1.5`
- 누적 기능: 명령어로 시스템 전체 제어
- **아키텍처 위치**: App 레이어 — `cli_app.c`, `cmd_table.c`
- **시퀀스 다이어그램**: UART RX → Ring Buffer → CLI Parser → Command Dispatch → Service
- 핵심 주제:
  - 명령어 파서 설계, 토큰화
  - 링 버퍼 기반 비동기 입력 처리 (Ch06 링 버퍼 활용)
  - 커맨드 테이블 패턴 (함수 포인터 배열)
  - 지원 명령어: `time set/get`, `alarm set`, `motor pos`, `sensor read`, `lcd clear`

### Ch16. PC 대시보드 (Python + pyserial)
- 프로젝트 버전: `v1.6` ← **마일스톤 ②**
- 누적 기능: 실시간 데이터 시각화 대시보드
- **아키텍처 위치**: App 레이어 — `protocol_app.c` (MCU 측 프레임 설계)
- **시퀀스 다이어그램**: MCU Protocol Layer → UART DMA → PC pyserial → PyQt5 UI
- ※ Python 코드는 완성 템플릿 제공 — 학습 포인트는 MCU 측 프레임 설계에 집중
- 핵심 주제:
  - 시리얼 프레임 구조 설계 (헤더 / 페이로드 / 체크섬)
  - MCU 측 프레임 직렬화 구현
  - pyserial 수신 스레드 구조 이해 (코드 분석 수준)
  - PyQt5 실시간 데이터 시각화 (완성 템플릿 커스터마이징)
  - **전체 아키텍처 다이어그램 완성본 작성** (Ch03 초안 → 최종)

---

## Part 5. 소프트웨어 아키텍처 심화

### Ch17. 소프트웨어 아키텍처 심화 & 전체 통합
- 프로젝트 버전: `v2.0`
- 누적 기능: Ch01~Ch16 전체 코드 → 레이어드 아키텍처 정합성 검증 및 리팩토링
- 핵심 주제:
  - **Ch03 초안 → v2.0 최종 아키텍처 진화 과정 리뷰**
  - 레이어 의존성 위반 사례 발견 및 수정
  - **FSM 심화**: 테이블 기반 FSM, 계층적 FSM (HFSM)
  - **시퀀스 다이어그램 심화**: 전체 시스템 시나리오 3종 (알람 / 스톱워치 / 대시보드 갱신)
  - 전체 시스템 마스터 Pinout 테이블 정리
  - 설계 원칙 점검: 단일 책임, 의존성 역전, 인터페이스 분리

---

## Part 6. 실무 품질 향상

### Ch18. 디버깅 스킬 심화
- 프로젝트 버전: `v2.1`
- 누적 기능: 전체 프로젝트 디버깅 기법 적용, 버그 시나리오 해결 실습
- 핵심 주제:
  - STM32CubeIDE 디버거 심화: Watch, Live Expression, Memory View
  - **HardFault 핸들러 해석**: 스택 덤프 분석, LR / CFSR 레지스터
  - 로직 애널라이저 / 오실로스코프 파형 읽기 (SPI / I2C / UART)
  - ITM/SWO + SEGGER RTT 고급 활용 (Ch02 로그 시스템 심화)
  - ST-LINK 펌웨어 업그레이드 (STM32CubeProgrammer)
  - DMA 관련 디버깅 패턴
  - **아키텍처 관점 디버깅**: 레이어 경계에서 버그 격리 전략

### Ch19. 코드 리팩토링 & 실무 품질
- 프로젝트 버전: `v2.2` ← **마일스톤 ③**
- 누적 기능: 전체 프로젝트 최종 완성 + 코드 리뷰 + 회고
- 핵심 주제:
  - 임베디드 C 코딩 표준 (MISRA-C 핵심 규칙)
  - 레이어 분리 리팩토링 실습
  - 공통 버그 패턴: `volatile` 누락, 정수 오버플로, 재진입 문제
  - 메모리 최적화: 스택/힙 분석, `static` 활용
  - 단위 테스트 전략 (HAL Mock 기법)
  - 코드 리뷰 체크리스트
  - **실무 면접 예상 질문 20선** (DMA / RTC / FSM / 아키텍처 / 인터럽트)
  - 전체 프로젝트 마무리 회고

---

## 누적 성장 버전 로드맵

```
v0.1  Ch01  LED/Button 기본 I/O
v0.2  Ch02  로그 인프라 (SWO/ITM)
v0.3  Ch03  프로젝트 아키텍처 설계도 v1            ← 아키텍처 시작
v0.4  Ch04  UART + Driver 레이어 추가
v0.5  Ch05  DMA 아키텍처 이해
v0.6  Ch06  UART DMA + 링버퍼 로그 완성
v0.7  Ch07  GP TIM + Service 레이어 추가
v0.8  Ch08  Advanced TIM + Stopwatch FSM
v0.9  Ch09  RTC + Alarm FSM
v1.0  Ch10  스텝 모터 + Clock Service          ← 마일스톤 ①
v1.1  Ch11  I2C + SHT31 + Sensor Service
v1.2  Ch12  SPI DMA + LCD Driver
v1.3  Ch13  GFX Service + UI Service + Display FSM
v1.4  Ch14  I2C DMA 고속화
v1.5  Ch15  CLI + App 레이어 완성
v1.6  Ch16  PC 대시보드 + 전체 아키텍처 완성본   ← 마일스톤 ②
v2.0  Ch17  아키텍처 심화 & 전체 통합 리팩토링
v2.1  Ch18  디버깅 인프라 강화
v2.2  Ch19  품질 인증 최종 완성                 ← 마일스톤 ③
```

**총 7 Part / 19 Chapter / 76시간**
