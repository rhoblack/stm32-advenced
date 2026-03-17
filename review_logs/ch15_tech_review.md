# Ch15. UART CLI 구현 — 기술 리뷰 (Technical Reviewer)

작성일: 2026-03-17
리뷰어 역할: 기술 리뷰어 (STM32 HAL, NUCLEO-F411RE, C 임베디드 실무 전문가)

---

## 리뷰 요약

| 항목 | 결과 |
|------|------|
| Critical 이슈 | 0건 |
| Major 이슈 | 2건 |
| Minor 이슈 | 3건 |
| 전체 기술 정확성 | ⭐⭐⭐⭐ (4/5) |

---

## Critical 이슈 (컴파일 불가 / HAL API 오용)

없음.

---

## Major 이슈

### 🟡 Major #1: HAL_UART_Transmit 블로킹 사용 — DMA 환경에서 충돌 위험

**위치**: `cli_app.c`, `CLI_Printf()` 함수

**문제**:
```c
HAL_UART_Transmit(s_huart, (uint8_t *)buf, (uint16_t)len, 100);
```
Ch06에서 UART TX를 DMA 모드로 전환했습니다.
`HAL_UART_Transmit()` (폴링)과 `HAL_UART_Transmit_DMA()` (DMA)를 같은 핸들에서 혼용하면
DMA 전송 중에 폴링 전송이 겹쳐 데이터 손상이 발생할 수 있습니다.

**권장 수정**:
```c
/* 방법 1: Ch06 UART Driver의 공개 API 사용 */
UART_Send((const uint8_t *)buf, (uint16_t)len);  /* uart_driver.h 의 DMA 전송 */

/* 방법 2: 폴링 사용 시 DMA 완료 대기 후 호출 */
while (HAL_UART_GetState(s_huart) == HAL_UART_STATE_BUSY_TX) { /* 대기 */ }
HAL_UART_Transmit(s_huart, (uint8_t *)buf, (uint16_t)len, 100);
```

**교재 처리 방안**: 본문에 주석 추가 — "프로젝트가 Ch06 DMA TX를 사용하는 경우 `UART_Send()` Driver API로 교체"
이 이슈는 실제 컴파일/동작에서 문제가 발생할 수 있으므로 **실습 섹션에 명시** 필요.

---

### 🟡 Major #2: `sscanf("%hhu")` — newlib nano 환경 지원 여부 확인 필요

**위치**: `cmd_table.c`, `cmd_time_handler()`, `cmd_alarm_handler()`

**문제**:
```c
if (sscanf(argv[2], "%hhu:%hhu:%hhu", &h, &m, &s) != 3)
```
STM32CubeIDE의 기본 C 런타임은 `newlib-nano`를 사용합니다.
`newlib-nano`는 부동소수점 `printf/scanf`를 기본적으로 비활성화하며,
`%hhu` (unsigned char 스캔) 지원도 라이브러리 버전에 따라 불안정할 수 있습니다.

**권장 수정**:
```c
/* 더 안전한 대안: int로 파싱 후 범위 체크 */
int h_int, m_int, s_int;
if (sscanf(argv[2], "%d:%d:%d", &h_int, &m_int, &s_int) != 3)
{
    /* 에러 처리 */
}
uint8_t h = (uint8_t)h_int;
uint8_t m = (uint8_t)m_int;
uint8_t s = (uint8_t)s_int;
```

**중요도**: 교재에서 반드시 주석으로 언급 — "newlib-nano 환경에서 `%hhu` 동작을 먼저 검증하십시오"

---

## Minor 이슈

### 🟢 Minor #1: `CLI_Printf` 버퍼 크기 256 — 긴 help 출력 잘림 가능성

**위치**: `cli_app.c`, `CLI_Printf()`

```c
char buf[256];
```
`help` 명령어는 각 엔트리를 여러 줄로 출력합니다. 전체 help 텍스트가 256바이트를 초과하면
출력이 잘립니다. 현재 구현에서는 각 엔트리를 개별 `CLI_Printf()` 호출로 출력하므로
실제로는 문제가 없지만, 사용자가 큰 버퍼 텍스트를 한 번에 출력하려 할 때 주의가 필요합니다.

**제안**: `#define CLI_PRINTF_BUF_SIZE 256` 으로 상수화하여 수정이 용이하게 할 것.

---

### 🟢 Minor #2: `atoi()` 에러 처리 미흡 — 본문 언급 있으나 코드는 그대로

**위치**: `cmd_table.c`, `cmd_motor_handler()`

본문 FAQ에서 `atoi()` 의 한계를 언급했으나 실제 코드는 그대로 `atoi()`를 사용합니다.
교육 목적이라면 주석으로 명시하거나, 교재 수준에서는 `strtol()` 예시를 주석으로 제공하는 것이 좋습니다.

```c
/* 실무 권장: strtol 사용 */
/* char *endptr; */
/* steps = strtol(argv[2], &endptr, 10); */
/* if (*endptr != '\0') { return -2; }  // 비숫자 문자 감지 */
```

---

### 🟢 Minor #3: `volatile` 키워드 누락 가능성

**위치**: `cli_app.c`, `s_line_len` 전역 변수

현재 구현은 슈퍼루프 전용이므로 문제가 없습니다.
그러나 교재가 RTOS 또는 다른 컨텍스트에서 사용될 경우를 위해
ISR에서 접근되는 Ring Buffer 포인터나 길이 변수에 `volatile`을 명시하는 것이 좋습니다.
현재 `s_line_len`은 ISR에서 접근하지 않으므로 필수는 아닙니다.

---

## 기술 정확성 체크리스트

| 항목 | 상태 |
|------|------|
| HAL API 시그니처 정확성 | ✅ 정확 |
| strtok_r 사용법 | ✅ 정확 (save_ptr 올바른 사용) |
| Ring Buffer 재사용 패턴 | ✅ 정확 |
| UART DMA/폴링 혼용 경고 | ⚠️ 본문 언급 필요 (Major #1) |
| sscanf 형식 지정자 | ⚠️ 확인 필요 (Major #2) |
| ANSI 이스케이프 코드 | ✅ 정확 |
| NULL 종료 마커 패턴 | ✅ 정확 |
| 코드 들여쓰기 4칸 | ✅ 준수 |
| 스네이크 케이스 | ✅ 준수 |
| 한국어 주석 | ✅ 준수 |
| LOG_D/I/W/E 사용 | ✅ 전 코드 적용 |

---

## 최종 의견

전반적으로 기술적으로 정확하며 HAL 표준을 준수합니다.
Major #1 (DMA/폴링 혼용)은 반드시 본문에 주의사항으로 명시해야 합니다.
Major #2 (sscanf %hhu)는 코드를 `%d`로 수정하는 것이 안전합니다.
두 이슈 모두 코드 수정 + 주석 추가로 해결 가능합니다.
