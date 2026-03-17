# Ch05 수정 완료 보고 — 기술 저자

**리뷰어**: 기술 리뷰어
**판정**: 조건부 승인 → 수정 완료

---

## 필수 수정 (Major) — 1건

### [M1] DMA 콜백 설명 방식 (HTML line 121~125)

**문제**: HAL UART의 `__weak` 함수 재정의 패턴으로 설명하여 혼동 유발
- 수정 전: `void HAL_DMA_XferCpltCallback(DMA_HandleTypeDef *hdma);` (사용자 구현)
- 수정 후: `hdma->XferCpltCallback = my_cplt_handler;` (함수 포인터 등록)

**수정 내용**:
- `manuscripts/part1/chapter05.html` 인터페이스 설계 코드 블록 수정
- UART `__weak` 콜백과 DMA 함수 포인터 등록 방식의 차이를 명시하는 주석 추가

**파일**: `manuscripts/part1/chapter05.html`

---

## 권장 수정 (Minor) — 2건

### [m1] §5.5 Critical Section 주석 불일치

**문제**: 주석은 "BASEPRI를 이용한 선택적 마스킹"이라고 했으나, 코드는 PRIMASK(`__get_PRIMASK()` / `__disable_irq()`) 사용
- 수정 전: `/* 방법 2: BASEPRI를 이용한 선택적 마스킹 */`
- 수정 후: `/* 방법 2: PRIMASK 저장/복원 (중첩 안전) */`

**파일**: `manuscripts/part1/chapter05.html`

### [m2] ch05_02 에러 콜백 로그 내용

**문제**: `hdma->Instance->CR` (제어 레지스터)을 출력 — 에러 정보가 아님
- 수정 전: `LOG_E("DMA 전송 에러! SR=0x%08lX", hdma->Instance->CR);`
- 수정 후: `LOG_E("DMA 전송 에러! ErrorCode=0x%08lX", hdma->ErrorCode);`

**파일**: `code_examples/ch05_02_dma_mem_to_mem.c`

---

## 수정 요약

| 구분 | ID | 파일 | 상태 |
|------|-----|------|------|
| Major | M1 | chapter05.html | 완료 |
| Minor | m1 | chapter05.html | 완료 |
| Minor | m2 | ch05_02_dma_mem_to_mem.c | 완료 |

**수정 건수**: 3/3 완료
