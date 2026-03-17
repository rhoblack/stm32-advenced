# Ch05 기술 리뷰 — 최종 재검증

**리뷰어**: 기술 리뷰어 (Technical Reviewer)
**일시**: 2026-03-17
**대상**: 기술 저자 수정본

---

## 수정 항목 검증

### 1. [Major #2] DMA 콜백 설명 방식 — ✅ 수정 확인

**수정 전** (line 121~125):
```c
/* DMA 전송 완료 콜백 (사용자 구현) */
void HAL_DMA_XferCpltCallback(DMA_HandleTypeDef *hdma);
/* DMA 전송 에러 콜백 (사용자 구현) */
void HAL_DMA_XferErrorCallback(DMA_HandleTypeDef *hdma);
```

**수정 후** (line 121~128):
```c
/* DMA 전송 완료 콜백 — 함수 포인터로 등록 */
hdma->XferCpltCallback = my_cplt_handler;
/* DMA 전송 에러 콜백 — 함수 포인터로 등록 */
hdma->XferErrorCallback = my_error_handler;
/* 주의: UART의 __weak 콜백(HAL_UART_RxCpltCallback)과 달리,
 * DMA 콜백은 핸들의 함수 포인터에 직접 등록하는 방식입니다. */
```

**평가**: HAL DMA의 실제 콜백 등록 메커니즘을 정확히 반영. UART weak 함수와의 차이를 명시적으로 설명하여 혼동 방지. **완벽한 수정.**

---

### 2. [Minor #3] Critical Section 주석 — ✅ 수정 확인

**수정 전** (line 916):
```c
/* 방법 2: BASEPRI를 이용한 선택적 마스킹 */
```

**수정 후** (line 916~917):
```c
/* 방법 2: PRIMASK 저장/복원 (중첩 안전) */
/* 이전 인터럽트 상태를 저장하고 복원 */
```

**평가**: 주석이 실제 코드(`__get_PRIMASK()` / `__set_PRIMASK()`)와 정확히 일치. "중첩 안전"이라는 핵심 장점도 명시. **정확한 수정.**

---

### 3. [Minor 추가] 에러 콜백 로그 — ✅ 수정 확인

**수정 전** (ch05_02_dma_mem_to_mem.c line 37~38):
```c
LOG_E("DMA 전송 에러! SR=0x%08lX",
      hdma->Instance->CR);
```

**수정 후**:
```c
LOG_E("DMA 전송 에러! ErrorCode=0x%08lX",
      hdma->ErrorCode);
```

**평가**: `hdma->ErrorCode`는 HAL이 관리하는 에러 코드 필드로, `HAL_DMA_ERROR_TE` (Transfer Error), `HAL_DMA_ERROR_FE` (FIFO Error), `HAL_DMA_ERROR_DME` (Direct Mode Error) 등을 포함. 디버깅에 훨씬 유용. **적절한 수정.**

---

## 최종 판정

| 항목 | 상태 |
|------|------|
| Major #2: DMA 콜백 설명 | ✅ 수정 완료 |
| Minor #3: BASEPRI→PRIMASK 주석 | ✅ 수정 완료 |
| Minor: 에러 콜백 레지스터 | ✅ 수정 완료 |
| 잔여 Critical | 0건 |
| 잔여 Major | 0건 |

### → ✅ **APPROVED (최종 승인)**

코드 3개, HTML 원고, SVG 9개 모두 기술적으로 정확합니다.
STM32F411RE NUCLEO 환경에서 컴파일 및 실행 가능한 수준입니다.
