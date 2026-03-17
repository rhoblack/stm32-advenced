# Ch11 Phase 1 기획 회의 — I2C 통신 + SHT31 온습도 센서
**날짜**: 2026-03-17
**참여**: 기술 저자, 교육설계자, 교육전문강사
**버전 목표**: v1.1 (누적 기능: 온습도 데이터 수집 + 온도 알람 연동)

---

## 1. 챕터 핵심 정의

### 1.1 기술 저자 관점: 섹션 구조 및 코드 가닥

#### 핵심 메시지
- **I2C 통신**: 동기식 직렬 통신, START/STOP/ACK/NACK 프로토콜
- **SHT31 센서**: 온습도 센서 드라이버 설계, CRC-8 검증 핵심
- **아키텍처**: HAL → I2C Driver → SHT31 Device Driver → Sensor Service 4계층
- **누적 성장**: RTC 알람(Ch09) + 온도 임계값 연동 → 알람 신호 생성

#### 섹션 구조 (9절 예상)
- **§11.1 I2C 프로토콜 개요** (아키텍처 위치 다이어그램)
  - 아이디어: "전화 예약" 비유 (START→주문→응답→END)
  - 목표: I2C 핀(SDA/SCL), 어드레싱, 프레임 구조 이해

- **§11.2 I2C 타이밍 및 클럭 스트레칭**
  - 아이디어: "택시 신호등" 비유 (SCL 풀링, 슬레이브 기다려)
  - 목표: ACK/NACK 메커니즘, 마스터/슬레이브 기다림 이해

- **§11.3 STM32 I2C 초기화 및 폴링 송수신**
  - HAL_I2C_Master_Transmit, HAL_I2C_Master_Receive
  - 목표: NUCLEO-F411RE I2C2 (PB10/PB3) 실습

- **§11.4 SHT31 센서 사양 및 명령어**
  - SHT31 I2C 주소 0x44, 측정 모드, 온습도 계산식
  - 목표: 데이터시트 읽기 능력

- **§11.5 CRC-8 검증 알고리즘**
  - 다항식 0x31, 초기값 0xFF, 반복(reflection) 없음
  - 목표: CRC 계산 구현, 에러 감지

- **§11.6 sht31_driver 구현**
  - 초기화, 측정 요청, 데이터 읽기 + CRC 검증
  - 목표: Device Driver 인터페이스 설계

- **§11.7 Sensor Service 아키텍처 (FSM)**
  - IDLE → MEASURING → DATA_READY / ERROR
  - 온도 임계값 검사, 알람 신호 생성

- **§11.8 온도 알람 연동 (RTC AlarmB)**
  - 온도 > 30°C → RTC AlarmB 트리거
  - 누적 성장: Ch09 RTC + Ch11 센서 통합

- **§11.9 통합 예제 및 테스트**
  - main.c 실습: 1초마다 온습도 읽기, 시리얼 출력, 알람 확인

#### 코드 예제 목록
1. **ch11_i2c_polling.c** — I2C 폴링 송수신 기본
2. **ch11_sht31_init.c** — SHT31 초기화 + 데이터 읽기
3. **ch11_crc8.c** — CRC-8 검증 알고리즘 (다항식 0x31)
4. **ch11_sensor_service.c** — Sensor Service FSM (IDLE/MEASURING/ERROR)
5. **ch11_alarm_integration.c** — 온도 임계값 → RTC AlarmB 연동
6. **ch11_main_integration.c** — 통합 main.c (v0.9 기반)
7. **ch11_debug_crc.c** — CRC 디버깅 유틸리티 (테스트 벡터)

#### SVG 다이어그램 목록
1. **ch11_sec01_architecture.svg** — 4계층 아키텍처 배치도 (HAL→I2C→SHT31→Service)
2. **ch11_sec02_i2c_timing.svg** — I2C 타이밍 다이어그램 (START/DATA/ACK/STOP)
3. **ch11_sec03_sht31_frame.svg** — SHT31 데이터 프레임 구조 (주소/명령/데이터/CRC)
4. **ch11_sec04_crc8_table.svg** — CRC-8 다항식 및 계산 예시 (비트 반영)
5. **ch11_sec05_sensor_fsm.svg** — Sensor Service FSM (IDLE/MEASURING/DATA_READY/ERROR)
6. **ch11_sec06_temp_calc.svg** — 온도/습도 계산식 및 범위 테이블
7. **ch11_sec07_alarm_sequence.svg** — 알람 연동 시퀀스 (센서→체크→RTC→인터럽트)
8. **ch11_sec08_pinout.svg** — NUCLEO-F411RE I2C2 핀 배치 (PB10/PB3)

### 1.2 교육설계자 관점: 학습 목표 및 인지 부하 설계

#### 학습 목표 (5개, 블룸 분류)
1. **이해**: I2C 프로토콜의 START/STOP/ACK/NACK 시퀀스를 설명할 수 있다.
   - 블룸 수준: 이해
   - 평가: "I2C에서 ACK를 받지 못했을 때 마스터는 어떤 행동을 하는가?"

2. **적용**: HAL_I2C_Master_Transmit/Receive를 사용하여 센서와 통신하는 코드를 작성할 수 있다.
   - 블룸 수준: 적용
   - 평가: "I2C 마스터 초기화 후 SHT31에 측정 명령을 보내는 코드 작성"

3. **분석**: CRC-8 검증 알고리즘을 이해하고, 수신한 데이터의 무결성을 검증할 수 있다.
   - 블룸 수준: 분석
   - 평가: "다항식 0x31로 계산한 CRC와 수신 CRC가 일치하지 않으면 어떤 조치를 취해야 하는가?"

4. **적용**: Device Driver 패턴(SHT31)을 설계하고, 상위 Service 레이어와 인터페이스를 정의할 수 있다.
   - 블룸 수준: 적용
   - 평가: "sht31_read_temp_humid() 함수 시그니처 설계"

5. **평가**: 온도 임계값 알람을 RTC와 통합하고, 감정 곡선 관점에서 이해도를 자가 평가할 수 있다.
   - 블룸 수준: 평가
   - 평가: "온도 > 30°C 검사 로직을 Sensor Service에 추가하는 것이 적절한가? 아니면 App 레이어에 둬야 하는가?"

#### 인지 부하 설계
- **한 절당 새 개념**: 최대 3개 제한
  - §11.1: I2C 프로토콜, START/STOP, 주소 지정 (3개)
  - §11.2: ACK/NACK, 클럭 스트레칭 (2개)
  - §11.3: HAL_I2C_* 폴링, 에러 처리 (2개)
  - §11.4: 센서 명령어, 측정 모드 (2개)
  - §11.5: CRC-8 알고리즘 (1개, 핵심이므로 따로 절 구성)
  - §11.6~9: 구현 + 통합 (복습 주도)

- **누적 성장 연결점**:
  - Ch04 UART Driver 패턴 → Ch11 I2C Driver (비슷한 인터페이스)
  - Ch05 DMA 개념 → Ch14 I2C DMA (향후 심화)
  - Ch09 RTC Alarm FSM → Ch11 Sensor Service FSM (패턴 재활용)

#### 4시간 강의 분량 분배
- §11.1~2: 40분 (프로토콜 이론 + 스트레칭 개념)
- §11.3: 40분 (HAL 폴링 실습)
- §11.4~5: 40분 (센서 사양 + CRC 알고리즘)
- §11.6: 30분 (Device Driver 구현)
- §11.7~8: 30분 (Service FSM + 알람 통합)
- §11.9: 40분 (통합 테스트 + Q&A)
- **합계**: 240분 (4시간)

### 1.3 교육전문강사 관점: 수강생 막힘 포인트 및 사전 해소

#### Top 5 막힘 포인트

**1. I2C ACK/NACK 메커니즘의 혼란**
- 문제: "마스터가 데이터를 보냈는데, ACK가 없으면 뭘 하는 거야?"
- 사전 해소:
  - 비유: "택시 호출 후 신호가 없으면? 손을 내렸다 다시 올렸다 반복하는 거와 같다"
  - 실습: HAL_I2C_Master_Transmit 리턴값 확인 → HAL_I2C_ERROR 처리
  - 면접 연결: "I2C 통신에서 대역폭 결정 요소는?" → "클럭 주파수"

**2. CRC-8 알고리즘의 복잡성**
- 문제: "다항식 0x31이 뭔데? 초기값이 0xFF인 이유는?"
- 사전 해소:
  - 단계별 계산: 예시 (0xBE, 0x00, 0x99) → CRC 계산 손으로 따라하기
  - 테스트 벡터: SHT31 데이터시트 제공 테스트 데이터로 검증
  - 직관: "체크섬처럼 데이터 손상 감지하는 수학적 방법"
  - 면접: "CRC와 체크섬의 차이는?" → "CRC는 다항식 기반, 에러 감지율 높음"

**3. Device Driver 레이어와 Service 레이어의 경계**
- 문제: "온도 임계값 검사는 Driver에 넣는 거야 Service에 넣는 거야?"
- 사전 해소:
  - 아키텍처 다이어그램: 명확한 경계선 그리기
  - 규칙: "Driver = 하드웨어 제어(읽기/쓰기), Service = 비즈니스 로직(임계값/FSM)"
  - 예시: sht31_read() vs sensor_check_alarm()
  - 면접: "UART와 로그 시스템의 레이어 구분은?" → Ch04/06 복습

**4. RTC AlarmB와 센서 이벤트 통합의 우선순위**
- 문제: "온도 알람이 발생했을 때, RTC 알람과 충돌하면?" (우선순위 역전)
- 사전 해소:
  - Ch09 RTC 알람 복습 (AlarmA / AlarmB 우선순위)
  - 새로운 설계: AlarmA = 시각, AlarmB = 온도 (비충돌)
  - NVIC 우선순위 표 → 명확한 선후 관계
  - 면접: "여러 인터럽트가 동시 발생하면?" → "NVIC 우선순위 결정"

**5. SHT31 데이터시트 읽기의 실무성**
- 문제: "데이터시트에서 어디를 봐야 하는 거야?"
- 사전 해소:
  - 데이터시트 네비게이션 가이드 (1쪽 = 핀/주소, 3쪽 = 명령, 5쪽 = 데이터 포맷)
  - 실습: "다음 세부사항을 찾아보세요" → (i) I2C 주소, (ii) 측정 명령코드, (iii) 데이터 길이
  - 강의 현장: 실제 SHT31을 들고 브레드보드에 연결하며 설명
  - 면접 연결: "임베디드 개발의 80%는 데이터시트 읽기" 메시지 강화

#### 추가 제안사항
- **감정 곡선 설계**:
  - 호기심(§11.1): "온습도 센서가 시계에도 들어간다고?"
  - 약간의 불안(§11.5): "CRC-8 계산 손으로 해본다고? 어렵잖아..."
  - 이해(§11.6~7): "아, 구조가 UART와 같네. 가능할 것 같은데?"
  - 성취(§11.9): "온습도 실제 출력됨! 와, 온도 알람도 RTC와 연동되네!"
  - 자부심(마무리): "나, 이제 센서 드라이버 설계할 수 있다!"

- **강의 현장 대비물**:
  - SHT31 모듈 (직접 보여주기)
  - I2C 타이밍 다이어그램 포스터 (벽에 붙여놓고 계속 참조)
  - CRC 계산 예제 문제지 (3~4개, 점진적 복잡도)
  - 온도 알람 시나리오 (5분 동영상 데모)

---

## 2. 인터페이스 설계 초안

### 2.1 I2C Driver (hal_i2c_driver.c/h)
```c
// 아키텍처: HAL ← I2C Driver
int32_t i2c_master_transmit(uint16_t dev_addr, const uint8_t *data, uint16_t len, uint32_t timeout_ms);
int32_t i2c_master_receive(uint16_t dev_addr, uint8_t *data, uint16_t len, uint32_t timeout_ms);

// 상태 조회
typedef enum { I2C_IDLE, I2C_BUSY, I2C_ERROR } i2c_state_t;
i2c_state_t i2c_get_state(void);
int32_t i2c_clear_error(void);
```

### 2.2 SHT31 Device Driver (sht31_driver.c/h)
```c
// 아키텍처: I2C Driver ← SHT31 Device Driver
int32_t sht31_init(void);
int32_t sht31_read_temp_humid(float *p_temp, float *p_humid);
int32_t sht31_trigger_measurement(void);  // 비블로킹 모드
int32_t sht31_read_measurement(float *p_temp, float *p_humid);

// 상태
typedef enum { SHT31_READY, SHT31_MEASURING, SHT31_ERROR } sht31_state_t;
sht31_state_t sht31_get_state(void);
```

### 2.3 Sensor Service (sensor_service.c/h)
```c
// 아키텍처: SHT31 Driver ← Sensor Service
typedef enum { SENSOR_IDLE, SENSOR_MEASURING, SENSOR_DATA_READY, SENSOR_ALARM, SENSOR_ERROR } sensor_state_t;

int32_t sensor_init(void);
int32_t sensor_start_measurement(void);
sensor_state_t sensor_get_state(void);
int32_t sensor_get_data(float *p_temp, float *p_humid);
int32_t sensor_set_temp_threshold(float temp_high, float temp_low);
int32_t sensor_check_alarm(uint8_t *p_alarm_flag);  // 온도 임계값 확인
```

---

## 3. FSM 및 시퀀스 다이어그램

### 3.1 Sensor Service FSM
```
         ┌─ IDLE ─┐
         │        ├──→ MEASURING (trigger + wait)
         │        │         │
         │        │    ┌────┴─────┐
         │        │    │           │
         │        │  OK?        CRC_ERROR?
         │        │    │           │
         │        │    ▼           ▼
         │    DATA_READY ──→ ERROR
         │        │            │
         │        └────────────┘
         └────────────────────┘

 - IDLE → MEASURING: 센서 측정 명령 발송
 - MEASURING → DATA_READY: 데이터 수신 + CRC 검증 성공
 - MEASURING → ERROR: I2C 통신 실패 또는 CRC 에러
 - DATA_READY → IDLE: 애플리케이션이 데이터 읽음
 - ERROR → IDLE: 에러 클리어 또는 재시도
```

### 3.2 온도 알람 연동 시퀀스
```
App → Sensor Service
         │ sensor_start_measurement()
         ↓
   Sensor Service → SHT31 Driver
                      │ sht31_trigger_measurement()
                      ↓
                   I2C Driver
                      │ i2c_master_transmit()
                      ↓
                   하드웨어(SHT31)
                      │ [약 50ms 대기]
                      ↓
App → Sensor Service
         │ sensor_check_alarm()
         ↓
   [온도 > 30°C?]
         │ YES
         ↓
   App → RTC Driver
          │ rtc_set_alarm(AlarmB, 초 설정)
          ↓
   하드웨어(RTC)
          │ [매초 검사, 일치하면 인터럽트]
          ↓
   Alarm Callback
          │ LED 점멸 또는 음성
          ↓
   App(IDLE)
```

---

## 4. 아키텍처 배치 (전체 v1.1)

현재 v0.9 (Ch09 RTC) 아키텍처에 Ch11 컴포넌트 추가:

```
┌─────────────────────────────────────────────────────────┐
│                    App Layer (Main)                     │
│  main() → sensor_start() → temp > 30°C? → alarm_set()  │
└────────────────────────────────────────────────────────┬┘
                        ▲                                  │
                        │ start_measurement()              │
┌───────────────────────┴──────────────────────────────────┐
│ Service Layer (Business Logic)                          │
│  ┌─────────────────┬─────────────────┬─────────────────┐│
│  │ Sensor Service  │  Clock Service  │  Alarm Service ││
│  │ (FSM:IDLE/...  │  (RTC 시각)     │  (RTC AlarmB)  ││
│  │  temp_threshold)│                 │                ││
│  └────────┬────────┴─────────────────┴─────────────────┘│
└──────────┼────────────────────────────────────────────────┘
           │ sht31_read()          get_time()    set_alarm()
┌──────────┴─────────────────────────────────────────────────┐
│ Driver Layer (Hardware Abstraction)                        │
│  ┌──────────────┬─────────────────┬──────────────────────┐ │
│  │ SHT31 Driver │ I2C Driver      │ RTC Driver + TIM    │ │
│  │ (CRC check)  │ (polling/DMA)   │ (AlarmA/B, PSC/ARR) │ │
│  │ (0.15s wait) │                 │                     │ │
│  └──────┬───────┴────────┬────────┴──────────────────────┘ │
└─────────┼────────────────┼──────────────────────────────────┘
          │ transmit()     │ receive()
┌─────────┴────────────────┴──────────────────────────────────┐
│ HAL Layer (STM32CubeIDE)                                   │
│  HAL_I2C_* / HAL_RTC_* / HAL_TIM_*                         │
└────────────────────────────────────────────────────────────┘
```

---

## 5. 챕터 작성 일정

### Phase 2: 초안 집필 (1주)
- Day 1: §11.1~3 (프로토콜 이론 + 폴링 구현)
- Day 2: §11.4~5 (센서 사양 + CRC 알고리즘)
- Day 3: §11.6~7 (Device Driver + Service FSM)
- Day 4: §11.8~9 (알람 통합 + 테스트)
- Day 5: SVG 다이어그램 + 코드 최종 정리

### Phase 3: 병렬 리뷰 (2~3일)
- 기술 리뷰어: STM32 HAL 정확성, CRC 알고리즘, 아키텍처 일관성
- 초급자 리뷰: I2C 프로토콜 설명, 용어 명확성
- 심리 리뷰: 감정 곡선, 불안 지점 관리
- 강사 리뷰: 강의 현장 실용성, 비유 검증, 감정 곡선

### Phase 4: 종합 회의 + 수정 (2~3일)
- 수정 우선순위: 정확성(CRC) > 심리적 안전 > 이해도
- 최종 승인 기준:
  - Critical 이슈: 0건
  - Major 이슈: 0건
  - 독자 이해도: ⭐⭐⭐⭐ 이상
  - 교육 설계: ⭐⭐⭐ 이상
  - 강의 적합도: ⭐⭐⭐⭐ 이상

---

## 6. 기타 주의사항

### 6.1 NUCLEO-F411RE I2C2 핀 배치
- I2C2_SDA: **PB3** (확인 필수: PB3이 JTDO 아닌지)
- I2C2_SCL: **PB10** (확인 필수: 다른 SPI 충돌 없는지)
- 클럭 속도: 100kHz (표준 모드) 권장

### 6.2 SHT31 검증
- I2C 주소: 0x44 (고정)
- 측정 명령: 0x24, 0x00 (클럭 스트레칭 없음)
- 응답: 6바이트 (온도 2 + CRC 1 + 습도 2 + CRC 1)
- 측정 시간: 약 15ms

### 6.3 CRC-8 구현 유의
- 다항식: 0x31 (정규형: 0x31, 반사 기반: 0x98)
- 초기값: 0xFF
- 최종 XOR: 0x00 (=생략)
- 입력 반사(Input Reflection): NO
- 출력 반사(Output Reflection): NO

### 6.4 에러 처리 전략
- I2C 통신 실패 → Sensor Service STATE = ERROR, 재시도 카운트
- CRC 검증 실패 → 데이터 폐기, 로그 경고
- 온도 임계값 초과 → 알람 신호(ALARM 상태), RTC AlarmB 설정

---

## 결론

Ch11은 **프로토콜 기반 센서 통신의 첫 번째 심화 사례**입니다.
- **기술적 핵심**: I2C 마스터 통신 + CRC-8 검증
- **아키텍처 핵심**: Device Driver 레이어 정립 (I2C ← SHT31 ← Service)
- **감정적 성과**: "센서 데이터를 직접 읽을 수 있다!"
- **실무 연결**: 산업 센서 대부분이 I2C/SPI로 통신하므로, 이 패턴이 핵심

다음 Ch12(SPI + LCD)로 전진할 때 동일한 Device Driver 패턴을 재활용할 예정입니다.

