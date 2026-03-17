# Ch19 기술 리뷰 결과

**리뷰어**: 기술 리뷰어 (Technical Reviewer)
**날짜**: 2026-03-17
**대상 파일**: manuscripts/part6/ch19.html, code_examples/ch19_*.c

---

## 종합 평가

| 항목 | 평가 |
|------|------|
| 코드 정확성 | ✅ 통과 |
| HAL API 사용 | ✅ 통과 |
| 기술 설명 정확성 | ✅ 통과 (1개 Minor 보완) |
| STM32 실무 적합성 | ✅ 통과 |
| MISRA-C 규칙 정확성 | ✅ 통과 |

---

## 🔴 Critical 이슈 — 0건

없음.

---

## 🟡 Major 이슈 — 0건

없음.

---

## 🟢 Minor 이슈 — 3건

### M01: HAL_I2C_Master_Receive 주소 표현
**위치**: ch19_refactoring_after.c, line ~70
**현황**: `HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(0x44U << 1U | 0x01U), ...)` 형태로 주소 작성
**의견**: STM32 HAL의 I2C Receive는 DevAddress를 `(addr << 1)` 형태(Write 주소)로 전달해도 HAL이 자동으로 Read 비트를 설정합니다. `| 0x01U`를 명시하면 오해를 줄 수 있습니다.
**권장**: `(uint16_t)(0x44U << 1U)`만 사용하고, 주석으로 "HAL이 R/W 비트를 자동 처리"를 명시하면 더 명확합니다.
**중요도**: Minor — 동작에 영향 없음 (HAL 내부에서 덮어씀)

### M02: __get_PRIMASK / __set_PRIMASK CMSIS 함수 설명 보완
**위치**: 3절 재진입 코드 예제
**의견**: `__get_PRIMASK()`와 `__set_PRIMASK()`가 CMSIS 코어 함수임을 명시하면 더 좋습니다. STM32CubeIDE 기본 포함이지만 다른 툴체인에서는 헤더 포함이 필요할 수 있습니다.
**권장**: 주석으로 `/* CMSIS 코어 함수 — core_cmInstr.h 포함 시 사용 가능 */` 추가
**중요도**: Minor — 교육적 명확성

### M03: Stack_FillCanary 함수의 절대 주소 사용
**위치**: 4절 스택 모니터링 코드
**현황**: `(uint32_t)&_estack - 1024U` 형태의 고정 오프셋 사용
**의견**: 스택 크기가 링커 스크립트에서 변경되면 이 오프셋도 변경해야 합니다. `_Min_Stack_Size` 심볼을 사용하면 더 안전합니다.
**권장**: `extern uint32_t _Min_Stack_Size; uint32_t stack_size = (uint32_t)&_Min_Stack_Size;` 활용 추가 설명
**중요도**: Minor — 개념 예시이므로 주석으로 보완 가능

---

## ✅ 긍정적 평가

1. **MISRA-C 규칙의 Before/After 패턴**: 추상적 규칙을 구체적 코드로 설명 — 실무 이해도 극대화
2. **volatile 설명**: 컴파일러 최적화 관점 설명이 기술적으로 정확하고 명확함
3. **HAL 반환값 처리**: 모든 코드 예제에서 HAL_StatusTypeDef ret 확인 패턴 일관되게 적용
4. **정수 오버플로우 수정**: 나눗셈 순서 조정으로 중간값 범위 제한 — 실무 적용 가능한 정확한 패턴
5. **HAL Mock 코드**: HAL_StatusTypeDef, I2C_HandleTypeDef 타입 정의가 실제 HAL과 동일 시그니처 유지 — 테스트 빌드/실제 빌드 전환 가능
6. **PRIMASK/BASEPRI 면접 질문 Q19**: 기술 내용 정확, FreeRTOS 연결도 정확
7. **DMA Stream/Channel Q3**: UART2 TX = DMA1 Stream6 Channel4 — STM32F411 RM0383 참조하여 정확
8. **LOG_D/I/W/E 일관 적용**: 모든 코드 예제에서 Ch02 표준 로그 시스템 사용

---

## 기술 정확성 최종 판정

**Critical: 0건 / Major: 0건 / Minor: 3건 (코드 동작에 영향 없음)**

v2.2 기술 리뷰 ✅ **통과**
