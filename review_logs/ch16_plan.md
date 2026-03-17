# Ch16 기획 회의 통합 결과 (Phase 1)

## 작성자: 팀 D (기술 저자 + 교육 설계자 + 강사 관점 통합)
## 작성일: 2026-03-17

---

## Sub-agent 1: 기술 저자 관점

### 시리얼 프레임 구조 설계

```
[STX 1B][TYPE 1B][LEN 1B][PAYLOAD nB][CRC16 2B][ETX 1B]
```

| 필드 | 크기 | 값 | 설명 |
|------|------|----|------|
| STX | 1 byte | 0xAA | Start of Frame |
| TYPE | 1 byte | 0x01~0x05 | 데이터 타입 구분 |
| LEN | 1 byte | 0~32 | 페이로드 바이트 수 |
| PAYLOAD | 0~32 bytes | 가변 | 실제 데이터 |
| CRC16 | 2 bytes | 계산값 | CRC-16/CCITT (다항식 0x1021) |
| ETX | 1 byte | 0x55 | End of Frame |

**타입 정의**:
- 0x01: SENSOR_DATA (온도 4B float + 습도 4B float = 8B)
- 0x02: TIME_DATA (시 1B + 분 1B + 초 1B = 3B)
- 0x03: MOTOR_STATUS (각도 2B uint16 + 상태 1B = 3B)
- 0x04: SYSTEM_STATUS (버전 2B + 업타임 4B = 6B)
- 0x05: ACK/NACK (결과 1B)

### MCU 측 protocol_app.c 인터페이스 설계

```c
/* protocol_app.h — 공개 인터페이스 */

/** 프레임 직렬화 후 UART DMA 전송 */
HAL_StatusTypeDef Protocol_SendSensorData(float temp, float humi);
HAL_StatusTypeDef Protocol_SendTimeData(uint8_t hour, uint8_t min, uint8_t sec);
HAL_StatusTypeDef Protocol_SendMotorStatus(uint16_t angle, uint8_t state);
HAL_StatusTypeDef Protocol_SendSystemStatus(void);

/** 수신 프레임 파싱 (CLI와 동일한 링 버퍼 활용) */
void Protocol_ProcessRxFrame(void);

/** CRC-16/CCITT 계산 */
uint16_t Protocol_CalcCRC16(const uint8_t *data, uint16_t len);
```

### 전체 아키텍처 완성본 다이어그램 구조 기획 (v1.6)

```
[App Layer]
  cli_app.c        — CLI 명령 처리
  protocol_app.c   — PC 대시보드 프레임 직렬화/역직렬화  ← Ch16 신규

[Service Layer]
  clock_service.c  — 시각 관리 + 스텝모터 연동
  sensor_service.c — SHT31 주기적 측정 관리
  gfx_service.c    — LCD 그래픽 서비스
  ui_service.c     — 화면 레이아웃 관리

[Driver Layer]
  uart_driver.c    — UART DMA + 링 버퍼
  tim_driver.c     — TIM2~TIM8 + Advanced TIM1
  rtc_driver.c     — RTC + Alarm
  stepper_driver.c — 28BYJ-48 스텝 모터
  i2c_driver.c     — I2C DMA
  sht31_driver.c   — SHT31 CRC-8 파싱
  spi_driver.c     — SPI DMA
  ili9341_driver.c — ILI9341 커맨드 시퀀스

[HAL Layer]
  STM32 HAL (자동 생성)

[Hardware Layer]
  NUCLEO-F411RE + 주변장치
```

---

## Sub-agent 2: 교육 설계자 관점

### 학습 목표 4개 ("~할 수 있다" 형태, 블룸 분류)

1. **[이해]** 시리얼 통신 프레임의 STX/TYPE/LEN/PAYLOAD/CRC16/ETX 구조와 각 필드의 역할을 설명할 수 있다.
2. **[적용]** STM32 HAL과 UART DMA를 활용하여 `protocol_app.c`의 프레임 직렬화 함수를 구현하고 PC에서 수신을 확인할 수 있다.
3. **[분석]** CRC-16 체크섬 오류 발생 시 로그를 통해 원인을 진단하고 수정할 수 있다.
4. **[평가]** v1.6 전체 아키텍처 완성본을 보고 Ch03 초안 대비 각 레이어가 어떻게 발전했는지 설명할 수 있다.

### Ch15 → Ch16 → 마일스톤 ② 연결성

- **Ch15에서 가져오는 것**: UART DMA 링 버퍼 (uart_driver.c), CLI 파서 구조 (cmd_table 패턴)
- **Ch16에서 추가하는 것**: 바이너리 프레임 직렬화, CRC-16 검증, PC 측 수신 시각화
- **마일스톤 ② 의미**: CLI(텍스트) + 대시보드(바이너리)가 동시에 동작 → 임베디드 시스템의 실무 듀얼 채널 통신 패턴 완성

### 학습 흐름 설계

```
도입: 왜 바이너리 프레임인가? (텍스트 vs 바이너리 비교로 동기 부여)
  ↓
1절: MCU-PC 통신 프로토콜 설계 원칙 [개념 + 비유]
  ↓
2절: 시리얼 프레임 구조 + CRC-16 [핵심 기술]
  ↓
3절: protocol_app.c 구현 [실습 핵심]
  ↓
4절: pyserial 수신 스레드 분석 [코드 읽기]
  ↓
5절: PyQt5 대시보드 커스터마이징 [완성 경험]
  ↓
6절: 마일스톤 ② 전체 아키텍처 완성본 [성취감 절정]
```

### 인지 부하 관리
- 2절에서 CRC 수학 이론을 너무 깊이 다루지 않도록 주의 (계산 함수 제공 + 동작 원리만 설명)
- Python 코드는 "분석" 수준 — 작성 과제 없음, 이미 완성된 코드 읽기

---

## Sub-agent 3: 강사 관점

### 수강생 막힘 Top 5 + 해소 방법

**막힘 1: "바이너리 프레임 vs 텍스트 CLI, 왜 두 가지를?"**
- 해소: 수도 계량기 비유 — 현장에서 사람이 읽는 표시판(CLI)과 원격 모니터링 신호(바이너리)가 공존
- aside.faq에 Q&A 배치

**막힘 2: "CRC-16이 뭔지 모르겠어요. 그냥 합산이랑 다른가요?"**
- 해소: 체크섬 vs CRC 비교표 + "나는 CRC 함수만 호출하면 된다" 안심 멘트
- 다항식 계산은 aside.tip으로 분리

**막힘 3: "pyserial에서 스레드를 써야 한다고요? 스레드 모르는데요"**
- 해소: "분석 수준"임을 명확히 선언 + 코드 흐름도 SVG로 시각화
- Python 스레드는 구현 과제 없음 명시

**막힘 4: "PyQt5 설치가 안 돼요 / pip install 오류"**
- 해소: 환경 설정 aside.tip에 pip 명령어, 가상환경 권장, Python 3.9~3.11 버전 체크
- "PC 환경 문제로 막히면 MCU 측 로그로 먼저 검증" 대안 경로 제시

**막힘 5: "전체 아키텍처 다이어그램이 너무 복잡해요"**
- 해소: 레이어별 색상 구분 + "이미 구현한 것" 체크리스트 방식으로 자신감 부여
- 6절에서 Ch03 초안 SVG와 v1.6 최종 SVG를 나란히 비교

### 4시간 강의 흐름 제안
- 1시간: 1~2절 (프로토콜 설계 + CRC 개념)
- 1시간: 3절 (protocol_app.c 실습 코딩)
- 1시간: 4~5절 (Python 코드 분석 + 대시보드 연동 확인)
- 1시간: 6절 (전체 아키텍처 완성본 리뷰 + 질의응답)

---

## 통합 결론

### 집필 우선순위
1. 3절 (protocol_app.c) — 학습 핵심, 가장 많은 분량
2. 2절 (프레임 구조 + CRC) — 이론적 기반
3. 6절 (전체 아키텍처) — 마일스톤 ② 성취감
4. 1절 (설계 원칙) — 동기 부여
5. 4절 (pyserial 분석) — 코드 읽기
6. 5절 (PyQt5) — 완성 경험

### 특별 주의사항
- Python 코드는 MCU 측 HAL 코드와 동등하게 상세히 설명하지 않도록 균형 조정
- CRC-16 계산 원리는 간략히, HAL 구현에 집중
- 마일스톤 ② 성취감을 극대화하는 감정 곡선 설계
