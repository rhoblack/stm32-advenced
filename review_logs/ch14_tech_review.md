# Ch14. I2C DMA 심화 — 기술 리뷰 (Technical Review)
작성자: 기술 리뷰어 에이전트
작성일: 2026-03-17

---

## 리뷰 요약

| 항목 | 결과 |
|------|------|
| Critical 이슈 | 0건 |
| Major 이슈 | 2건 |
| Minor 이슈 | 4건 |
| 전반적 기술 정확성 | ⭐⭐⭐⭐ (4/5) |
| HAL 표준 준수 | ⭐⭐⭐⭐⭐ (5/5) |
| 실무 적합성 | ⭐⭐⭐⭐ (4/5) |

---

## Critical 이슈 (0건)

없음.

---

## Major 이슈 (2건)

### 🟡 Major-1: ch14_sht31_dma.c — ISR 컨텍스트에서의 float 연산

**위치**: `sht31_parse_raw()` 내부, `on_rx_cplt()` 콜백에서 호출

**문제**:
`on_rx_cplt()`는 DMA 인터럽트 컨텍스트(ISR)에서 실행되며,
여기서 `sht31_parse_raw()`를 호출합니다.
`sht31_parse_raw()` 내부에서 float 연산(나눗셈, 곱셈)이 수행됩니다.
STM32F411에는 FPU(부동소수점 유닛)가 있어 float 연산이 빠르지만,
ISR에서 FPU를 사용하면 FPU 컨텍스트 저장/복원(lazy stacking)에 주의해야 합니다.
HAL이 FPU lazy stacking을 관리하지만, 복잡한 중첩 인터럽트 환경에서 드물게 스택 오버플로우가 발생할 수 있습니다.

**권장 해결책**:
`on_rx_cplt()`에서는 플래그(g_data_ready)만 세우고, 파싱은 메인 루프에서 수행.

```c
static void on_rx_cplt(void)
{
    /* ISR: 플래그만 세우고 즉시 반환 */
    g_rx_done = true;
    g_state   = SHT31_DMA_IDLE;
    LOG_D("SHT31 on_rx_cplt: 수신 완료 플래그 세팅");
}

/* 메인 루프에서 */
if (g_rx_done) {
    g_rx_done = false;
    sht31_parse_raw(g_rx_buf);  /* float 연산은 여기서 */
    g_data_ready = true;
}
```

**현재 코드 허용 여부**: STM32F411에서는 대부분 정상 동작하지만, 실무 안전성을 위해 수정 권장.

---

### 🟡 Major-2: ch14_multi_dma.c — MultiDma_CheckStreamConflict() 런타임 감지 한계

**위치**: `MultiDma_CheckStreamConflict()` 함수

**문제**:
`hdma_i2c_tx->Instance == hdma_uart_tx->Instance` 비교는 런타임에서 올바르게 동작하지 않을 수 있습니다.
DMA 핸들의 `Instance` 포인터는 DMA 스트림 레지스터 기주소를 가리키는데,
DMA1_Stream6과 DMA1_Stream7은 서로 다른 주소이므로 같은 스트림을 쓰지 않는 한 비교가 올바릅니다.

**재확인**: 이 함수의 로직 자체는 올바름. 단, 주석이 "올바르게 동작하지 않을 수 있다"고 표현하여 혼란을 줄 소지가 있음.

**권장 수정**: 주석을 명확히 하여 "DMA1_Stream6(UART TX)와 DMA1_Stream7(I2C TX)은 다른 인스턴스이므로, 올바르게 분리된 경우 false 반환"으로 설명 보강.

---

## Minor 이슈 (4건)

### 🟢 Minor-1: ch14_i2c_dma_driver.c — g_i2c_drv 전역 변수 문제

**위치**: `static I2cDmaHandle *g_i2c_drv = NULL;`

**문제**: 다중 I2C 인스턴스(I2C1 + I2C2 동시 사용) 시 이 전역 포인터 방식은 확장이 불가능합니다.

**현재 교재 범위**: v1.4는 I2C1만 사용하므로 허용 범위. Minor 등록.

**권장**: 코드 주석에 "단일 I2C 인스턴스 기준" 명시 추가 권장.

---

### 🟢 Minor-2: ch14.html 4절 — DMA 우선순위 설명의 숫자 표현

**위치**: 4절 코드 블록

**문제**: `Priority=3 (기대값: DMA_PRIORITY_VERY_HIGH=3)` 로그 출력 예시에서
`DMA_PRIORITY_VERY_HIGH`의 숫자 값이 `3`으로 표시되어 있습니다.
HAL에서 `DMA_PRIORITY_VERY_HIGH`는 `0x00030000UL`인데,
Init.Priority 필드에 저장되는 값(매크로 내부 비트 위치)과
HAL_NVIC_SetPriority의 PreemptPriority(0~15)는 다른 개념입니다.
독자가 혼동할 수 있으므로 설명 보강 필요.

---

### 🟢 Minor-3: ch14_sht31_dma.h — SHT31_DMA_OnTimerElapsed 네이밍

**문제**: 함수명이 `SHT31_DMA_OnTimerElapsed`인데, CLAUDE.md 코딩 규칙에 따르면
스네이크 케이스(`sht31_dma_on_timer_elapsed`)가 권장됩니다.
챕터 내에서 일관성을 위해 HAL 콜백 규칙(PascalCase 접두사)과의 충돌 주의.

---

### 🟢 Minor-4: ch14_i2c_dma_driver.c — I2cDma_Reset() HAL_I2C_Init 호출 시 설정 소실 위험

**문제**: `HAL_I2C_DeInit` 후 `HAL_I2C_Init`을 호출할 때,
`hi2c->Init` 구조체가 이미 채워져 있는 경우(MX_I2C1_Init에서 설정됨)에만 올바르게 동작합니다.
코드 자체는 문제없으나, 독자에게 이 전제 조건을 주석으로 명시하면 좋습니다.

---

## 기술 정확성 검증 항목

| 항목 | 결과 |
|------|------|
| HAL_I2C_Master_Transmit_DMA API 시그니처 | ✅ 정확 |
| HAL_I2C_Master_Receive_DMA API 시그니처 | ✅ 정확 |
| HAL_I2C_MasterTxCpltCallback 콜백명 | ✅ 정확 |
| STM32F411 DMA1 스트림-채널 배치 (RM0383 기준) | ✅ 정확 |
| SHT31 CRC-8 (다항식 0x31, 초기값 0xFF) | ✅ 정확 (Ch11 로직 재사용) |
| SHT31 I2C 주소 (0x44) | ✅ 정확 |
| SHT31 고정밀 단발 측정 명령 (0x24, 0x00) | ✅ 정확 |
| SHT31 측정 대기 시간 (~15ms) | ✅ 정확 (데이터시트: 최대 15.5ms) |
| TIM6 PSC=15999, ARR=14 → 15ms | ✅ 정확 (84MHz PCLK1 기준: 84M/16000 = 5250Hz, 15/5250 ≈ 2.86ms → 수정 필요!) |

**주의**: TIM6 설정 계산 오류 발견!

STM32F411 TIM6는 APB1 버스에 연결됩니다.
PCLK1 = 42MHz (APB1), APB1 배수기 = 2이므로 TIM6 클럭 = 84MHz.

`PSC=15999, ARR=14`:
- 타이머 클럭 = 84MHz / (15999+1) = 84M/16000 = 5250 Hz
- 주기 = 14+1 = 15 틱 → 15/5250 = 2.857ms

**이것은 15ms가 아닌 약 2.86ms입니다!**

15ms를 만들려면:
- PSC=839, ARR=1499 → 84M/840 = 100kHz, 1500 틱 = 15ms ✅
- 또는 PSC=8399, ARR=149 → 84M/8400 = 10kHz, 150 틱 = 15ms ✅

**이 오류는 기술 저자가 수정해야 합니다.**

---

## 코드 품질 평가

| 기준 | 평가 |
|------|------|
| LOG_D/I/W/E 사용 | ✅ 전 함수에 적용됨 |
| 들여쓰기 4칸 | ✅ 준수 |
| 스네이크 케이스 | ⚠ 일부 PascalCase 혼용 (Minor-3) |
| 한국어 주석 | ✅ 적절히 사용됨 |
| 코드 블록 30줄 이하 | ✅ 대부분 준수 |

---

## 수정 요청 우선순위

1. **즉시 수정**: TIM6 타이머 설정 계산 오류 (PSC/ARR 값 수정)
2. **권장 수정**: on_rx_cplt()에서 float 연산을 메인 루프로 이동 (Major-1)
3. **선택 수정**: Minor 이슈들
