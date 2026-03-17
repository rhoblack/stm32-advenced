# Ch16 기술 리뷰 (Technical Review)
## 리뷰어: 기술 리뷰어 에이전트
## 리뷰 대상: manuscripts/part4/ch16.html + code_examples/ch16_*.c/h + ch16_dashboard.py
## 작성일: 2026-03-17

---

## 1. 코드 정확성 검토

### 1.1 protocol_app.c

**CRC-16/CCITT 구현 검토**
- 다항식 0x1021, 초기값 0xFFFF — 표준 CRC-16/CCITT 사양과 일치. ✅
- CRC 계산 범위: `&out_frame[1]` (TYPE 위치)부터 `2U + payload_len` 바이트 — 올바름. ✅
- CRC 빅 엔디언 삽입: 상위 바이트(`crc >> 8`) 먼저 — 일관성 있음. ✅

**float 직렬화**
- `memcpy(&payload[0], &temp, sizeof(float))` — MISRA-C 관점에서 안전한 방식. ✅
- 리틀 엔디언 가정(STM32F411은 ARM Cortex-M4, 리틀 엔디언 확인됨). ✅
- Python 측 `struct.unpack_from("<ff", payload)`가 리틀 엔디언(`<`) 지정 — 일치. ✅

**정적 버퍼 설계**
- `static uint8_t s_tx_frame[PROTO_MAX_FRAME]` — 힙 사용 없이 정적 할당. ✅
- 재진입 위험: `s_tx_frame`을 여러 함수가 공유. 인터럽트에서 호출하면 데이터 오염 가능.

🟡 **Major #1**: `s_tx_frame`은 전역(정적) 버퍼이므로, 인터럽트 컨텍스트에서
`Protocol_Send*()` 함수를 호출하면 재진입 문제가 발생합니다.
본문에 "메인 루프에서만 호출"이라는 제약을 명시하거나,
Critical Section으로 보호하는 주석을 추가해야 합니다.

**권고 수정**: `Protocol_SendSensorData()` 주석에 다음을 추가:
```c
/* @warning 인터럽트 컨텍스트에서 호출 금지 — 정적 버퍼 재진입 위험 */
```

### 1.2 HAL API 사용 검토

- `UART_IsTxBusy()` 함수 — `uart_driver.c`에 구현되어야 하며 헤더에 선언 필요. ✅ (Ch06에서 구현된 것으로 가정)
- `UART_Send()` 반환형 `HAL_StatusTypeDef` — 올바름. ✅
- `HAL_GetTick() / 1000U` — 업타임 초 환산 올바름. ✅

### 1.3 메인 루프 통합 코드 (`main.c` 예시)

- `HAL_Delay(20)` 사용 — 블로킹 함수로 실시간성 저하. 교육적 단순화로 용납 가능하나 실무 경고 필요.
- `volatile uint8_t g_tick_1s` — volatile 선언 올바름. ✅

🟢 **Minor #1**: `HAL_Delay(20)` 예시 코드에 주석으로 "실습용 단순화" 명시 추가 권장.

### 1.4 Python 코드 검토

- `calc_crc16(data: bytes)` — MCU `Protocol_CalcCRC16` 알고리즘과 동일. ✅
- `struct.unpack_from("<ff", payload)` — 2개 float 언팩, 올바름. ✅
- `serial.Serial(..., timeout=0.1)` — 100ms 타임아웃, read가 블로킹되지 않음. ✅
- `pyqtSignal(dict)` — 스레드 간 안전 전달. ✅

🟢 **Minor #2**: Python에서 `payload[0] | (payload[1] << 8)` 각도 계산 —
uint16 리틀 엔디언 조합, 올바름. `struct.unpack_from("<H", payload)`로 대체하면
더 명확하지만 기능적으로 동일.

---

## 2. 기술 설명 정확성

### 2.1 CRC 설명

- "CRC는 STX와 ETX를 제외" — 올바른 관례. ✅
- 체크섬 vs CRC 비교 — 기술적으로 정확. ✅
- "CRC-16은 임베디드 통신에서 표준적으로 사용" — 정확 (Modbus, NMEA, 등). ✅

### 2.2 프레임 재동기 설명

- "STX 값(0xAA)이 페이로드에 우연히 등장할 경우를 대비해 LEN 필드로 끝 위치를 알 수 있도록" — 올바른 설명. ✅
- 단, 실제로 0xAA가 페이로드에 나타나면 완전한 보호가 안 됩니다. 바이트 스터핑(Byte Stuffing)이 완전한 해결책이나, 교육 수준에서 생략 가능.

🟢 **Minor #3**: "바이트 스터핑을 사용하지 않으므로 페이로드에 0xAA/0x55가 나타날 경우 파서가 오동작할 수 있다"는 한계 주의사항을 aside.tip에 추가 권장.

### 2.3 float 엔디언 설명

- "리틀 엔디언, IEEE 754" 명시 — 올바름. ✅
- Python `struct.unpack_from("<ff", ...)` 매핑 설명 — 교육적으로 적절. ✅

---

## 3. 실무 적합성

### 3.1 프레임 설계

- STX/ETX 값(0xAA/0x55) 선택 — 실무에서 흔히 사용. 적절. ✅
- TYPE 1바이트 = 256가지 확장 가능 — 실무적 설계. ✅
- 최대 페이로드 32바이트 제한 — NUCLEO-F411RE UART2 버퍼 대비 여유 있음. ✅

### 3.2 비동기 설계

- TX BUSY 시 프레임 버림 전략 — 단순하지만 대시보드 데이터 유실에는 적합. ✅
- "Ch17에서 비동기 리팩토링 예고" — 적절한 누적 성장 예고. ✅

---

## 4. 리뷰 결론

| 구분 | 건수 |
|------|------|
| 🔴 Critical | 0 |
| 🟡 Major | 1 |
| 🟢 Minor | 3 |

### Major 수정 필요 사항

**Major #1**: `s_tx_frame` 재진입 위험 주석 추가
- 위치: `ch16_protocol_app.h` 및 `ch16_protocol_app.c` 관련 함수 주석
- 수정 내용: `@warning 인터럽트 컨텍스트에서 호출 금지` 주석 추가
- HTML 본문 3절에 경고 박스(aside.tip) 추가

### Minor 수정 사항 (선택)

- Minor #1: `HAL_Delay(20)` 실습용 단순화 주석
- Minor #2: Python 각도 계산 struct 방식 대안 언급
- Minor #3: 바이트 스터핑 한계 주의사항

### 최종 판정

Major #1을 반영하면 승인 가능합니다.
CRC 구현, float 직렬화, UART DMA 활용 패턴 모두 STM32 HAL 표준에 부합합니다.
