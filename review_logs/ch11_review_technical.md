# Ch11 기술 리뷰 (Technical Review)

**리뷰 수행자**: 기술 리뷰어
**검수 날짜**: 2026-03-18
**검수 대상**: Ch11 초안 + 코드 9개 + SVG 8개

---

## 종합 평가

🟡 **CONDITIONAL PASS**

초안의 기술적 기초는 견고하나, **1개 Critical 이슈**와 **2개 Major 이슈** 발견.
Critical 수정 후 재검 필요.

---

## Critical Issues (❌ 필수 수정)

### 1. SHT31 응답 프레임 포맷 오류
**문제**: HTML §11.3 및 코드의 SHT31 응답 형식이 센서 데이터시트와 불일치.

- **현재 코드**: `sht31_read_measurement()` 에서 인덱스 `[0][1][CRC][3][4][CRC]` 사용
- **SHT31 데이터시트**: 정확한 응답 = `[Temp_H, Temp_L, Temp_CRC, Humid_H, Humid_L, Humid_CRC]`
- **발견된 오류**: `ch11_sht31_driver.c` 라인 74-75에서
  ```c
  temp_raw = ((uint16_t)response[0] << 8) | response[1];
  humid_raw = ((uint16_t)response[3] << 8) | response[4];
  ```
  이는 정확하나, **CRC 검증 호출이 모호함**.

**권장 수정안**:
```c
// 명확한 주석 추가 + 상수 정의
#define SHT31_TEMP_H_IDX    0
#define SHT31_TEMP_L_IDX    1
#define SHT31_TEMP_CRC_IDX  2
#define SHT31_HUM_H_IDX     3
#define SHT31_HUM_L_IDX     4
#define SHT31_HUM_CRC_IDX   5

// CRC 검증 시 3바이트 단위 명확화
if (crc8_verify_sht31(&response[SHT31_TEMP_H_IDX], 3) != 0) { // [0,1,2]
    LOG_E("Temperature CRC error");
    return SHT31_CRC_ERR;
}
if (crc8_verify_sht31(&response[SHT31_HUM_H_IDX], 3) != 0) {  // [3,4,5]
    LOG_E("Humidity CRC error");
    return SHT31_CRC_ERR;
}
```

**영향**: CRC 검증이 현재 맞으나, 향후 버그 가능성 높음 (유지보수 리스크).

---

### 2. HAL_I2C_Master_Transmit의 주소 시프트 명확성 부족
**문제**: `ch11_i2c_driver.c` 라인 26의 `(dev_addr << 1)` 변환이 SHT31_ADDR 정의와 충돌 위험.

**현재 상황**:
- `ch11_sht31_driver.h`에서 `#define SHT31_ADDR 0x44` (7비트)
- `i2c_master_transmit(..., SHT31_ADDR, ...)` 호출 시 `0x44 << 1 = 0x88` (8비트)
- HAL API는 8비트 주소 기대 → 정확함.

**그러나 위험 요소**:
- 만약 `SHT31_ADDR`을 잘못 8비트로 정의(`#define SHT31_ADDR 0x88`)하면 `0x88 << 1 = 0x110` (오버플로우)
- 향후 I2C DMA(Ch14)나 인터럽트 모드에서 이 문제 재발 가능성

**권장 수정안**:
```c
/* i2c_driver.h 상단에 명확한 주석 추가 */
/**
 * @brief I2C 마스터 송신 (폴링)
 * @param dev_addr I2C 슬레이브 주소 (7비트만! 예: 0x44)
 *        주의: 내부에서 8비트로 변환됨 (dev_addr << 1)
 *        오류 예: dev_addr=0x88(8비트) → 0x110 오버플로우!
 */
```

또는 상수 정의 개선:
```c
/* ch11_sht31_driver.h */
#define SHT31_ADDR_7BIT  0x44   // 7비트 주소
// 내부 사용 (HAL API는 8비트 필요)
#define SHT31_ADDR_8BIT  (SHT31_ADDR_7BIT << 1)
```

**영향**: 현재는 정확하나, 팀 협업 시 실수 가능성 높음.

---

## Major Issues (⚠️ 강력 권장 수정)

### 1. CRC-8 다항식 검증 누락
**문제**: `ch11_ex03_crc8.c`의 CRC-8 계산이 Sensirion 표준과 일치하는지 **외부 검증 필요**.

**현재 코드**:
```c
uint8_t crc = 0xFF;  // 초기값 OK
...
crc = (crc << 1) ^ 0x31;  // 다항식 0x31 OK
```

**검증 필요**:
- Sensirion SHT31 CRC 예제 데이터와 비교
- 온도값 0xBE, 0x00 → CRC 계산 결과가 0xAC인지 확인 (코드 주석에 예시 있음)

**권장 수정안**:
```c
// ch11_ex03_crc8.c에 테스트 코드 추가
void crc8_self_test(void)
{
    // Sensirion SHT31 CRC 예제
    uint8_t test_data[] = {0xBE, 0x00};  // 온도값
    uint8_t expected_crc = 0xAC;         // 데이터시트에서 제공
    uint8_t calculated = crc8_sht31(test_data, 2);

    if (calculated == expected_crc) {
        LOG_I("CRC8 self-test: PASS");
    } else {
        LOG_E("CRC8 self-test: FAIL (expected=0x%02X, got=0x%02X)",
              expected_crc, calculated);
    }
}
```

**영향**: 센서 데이터 무결성 검증 신뢰도 문제.

---

### 2. 타임아웃 값 일관성 부족
**문제**: `ch11_sht31_driver.c`의 I2C 타임아웃 값이 일정하지 않음.

**현재 상황**:
- `sht31_init()`: `timeout_ms = 1000`
- `sht31_read_temp_humid()`: `timeout_ms = 1000`
- `sht31_read_measurement()`: **`timeout_ms = 100`** ← 다름!

**분석**:
- 측정 결과 수신(비블로킹)은 센서가 이미 준비 상태이므로 100ms 충분할 수 있음.
- 그러나 클럭 스트레칭(clock stretching) 고려 시 1000ms 권장.
- 현재 100ms는 "이론적"으로는 충분하나, 실제 하드웨어 변동성 무시.

**권장 수정안**:
```c
/* ch11_sht31_driver.h */
#define SHT31_I2C_TIMEOUT_MS  1000  // 일관된 타임아웃

/* ch11_sht31_driver.c */
sht31_status_t sht31_read_measurement(float *p_temp, float *p_humid)
{
    ...
    ret = i2c_master_receive(SHT31_ADDR, response, SHT31_RESP_LEN,
                             SHT31_I2C_TIMEOUT_MS);  // 1000으로 변경
```

**영향**: 드문 경우 타임아웃 오류 발생 가능.

---

## Minor Issues & Suggestions (💡 선택 사항)

1. **CRC 계산 최적화** (옵션)
   - 비트별 처리(현재)는 이해하기 쉽지만, 성능 최적화를 위해 **테이블 기반 CRC** 검토 가능.
   - 함수 호출 빈도가 높지 않으므로 현재 코드도 충분함.

2. **로그 레벨 조정** (소수 권장)
   - `ch11_sht31_driver.c` 라인 41: `LOG_D("crc8_sht31: ...")`는 DEBUG 레벨
   - 프로덕션에서 로그 비활성화 시 성능 영향 미미하나, 볼륨 많음.
   - 권장: `LOG_D` → `LOG_V`(verbose, 새 레벨 추가) 또는 조건부 컴파일

3. **응답 버퍼 오버플로우 보호** (권장)
   - `sht31_read_measurement()`에서 `response[SHT31_RESP_LEN]` 버퍼 사용
   - 그러나 인덱스 `[0~5]` 접근 → `SHT31_RESP_LEN` 정의 필수
   - `ch11_sht31_driver.h`에서 `#define SHT31_RESP_LEN 6` 명시 필요

---

## 강점 (👍)

1. **아키텍처 설계 탁월함**
   - 3계층 분리(I2C Driver, SHT31 Device Driver, Sensor Service FSM) 명확
   - Ch09 Alarm FSM 패턴 재활용으로 학습 곡선 완화
   - 블로킹/비블로킹 패턴 구분 정확

2. **HAL 함수 사용 정확**
   - `HAL_I2C_Master_Transmit/Receive` 올바른 사용
   - 에러 처리(TIMEOUT, ERROR) 체계적

3. **센서 규격 준수**
   - SHT31 측정 명령(0x2C 0x06), 응답 형식(6바이트), 계산식 정확
   - CRC-8 알고리즘 기초 견고

4. **코드 가독성**
   - 한국어 주석 충분
   - 함수 분해(separation of concerns) 우수
   - 로그 추적 가능

---

## 종합 판정

| 항목 | 상태 | 비고 |
|------|------|------|
| I2C 프로토콜 | ✅ PASS | 정확 |
| SHT31 규격 | ⚠️ CONDITIONAL | 인덱스 명확화 필요 |
| CRC-8 | ⚠️ CONDITIONAL | 데이터시트 검증 필요 |
| HAL 코드 | ✅ PASS | 견고 |
| 아키텍처 | ✅ PASS | 우수 |

---

## 수정 로드맵

**필수 (Critical)**:
1. SHT31 응답 프레임 인덱스 상수화 (`TEMP_H_IDX`, `HUM_H_IDX` 등)
2. HAL 주소 변환 문서화

**강력 권장 (Major)**:
1. CRC-8 자체 테스트 코드 추가
2. I2C 타임아웃 일관성 통일 (100ms → 1000ms)
3. `SHT31_RESP_LEN` 상수 정의

**재검 필요 이슈**: Critical 2개 수정 후 재검 (30분 소요)

---

## 다음 단계

✅ **Phase 3 병렬 리뷰 계속 진행**
- 초급자 독자, 심리 전문가, 강사 리뷰 진행
- Phase 4 (최종 회의)에서 모든 이슈 통합 검토

