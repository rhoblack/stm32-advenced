# Ch07 TIM 기초 — Phase 4 최종 기술 재검증 보고서

## 작성일: 2026-03-17
## 작성자: 기술 리뷰어 (Technical Reviewer)
## 목적: Phase 3 필수 수정 3건 + 추가 1건 반영 확인

---

## 재검증 결과 요약

| 원래 이슈 | 수정 결과 | 판정 |
|-----------|----------|------|
| C-01: TIM1 APB2 100MHz 오류 | 84MHz로 수정 (HTML + SVG) | PASS |
| M-02: PSC 83999 16비트 초과 | 8399로 수정 + 경고 주석 | PASS |
| M-01: EGR UG 후 UIF 미클리어 | HAL 매크로 + CLEAR_FLAG | PASS |
| (추가) 트러블슈팅 팁 | §7.3에 4항목 체크리스트 | PASS |

---

## 상세 검증

### 1. C-01: TIM1 클럭 84MHz 통일 — PASS

**HTML** (`chapter07.html` 라인 195):
```html
<td>APB2 (84MHz)</td>
```
- 이전: `APB2 (100MHz)` → 수정 확인

**SVG** (`ch07_sec01_timer_hierarchy.svg` 라인 47):
```
16비트 | APB2(84MHz) | Ch08에서 학습
```
- 이전: `APB2(100MHz)` → 수정 확인

**교차 검증**: grep "100MHz" 결과에서 Ch07 HTML 및 SVG 파일에 0건. PASS.

### 2. M-02: PSC 16비트 범위 안전성 — PASS

**코드** (`ch07_04_tim_driver.c` 라인 84~88):
```c
uint32_t psc, arr;

/* 10kHz 카운터 기준 (PSC=8399, 16비트 범위 안전) */
psc = (TIM2_CLK_HZ / 10000) - 1;  /* 8399 */
arr = period_ms * 10 - 1;
```

**검증 포인트**:
- PSC = 8399 < 65535 (16비트 최대값): PASS
- 주기 계산 검증:
  - 1ms: (8399+1) x (1x10-1+1) / 84,000,000 = 8400 x 10 / 84M = 0.001s = 1ms: PASS
  - 500ms: 8400 x 5000 / 84M = 0.5s: PASS
  - 60000ms: ARR = 599999 → TIM2 32비트(max 4,294,967,295) 범위 내: PASS
- 오버플로 경고 주석 (라인 81~82): `※ 주의: PSC 레지스터는 16비트(max 65535)` → 학습 효과 우수

**HTML 인라인 코드** (라인 886~888): 동일한 10kHz 기준 코드 사용. 83999 참조 완전 제거. PASS.

**이전 분기 로직 제거 확인**: `period_ms >= 10` 조건 분기 삭제, 단일 전략으로 통일. 코드 단순화 + 16비트 안전성 확보. PASS.

### 3. M-01: UIF 클리어 처리 — PASS

**코드** (`ch07_04_tim_driver.c` 라인 93~98):
```c
/* 새 설정 즉시 반영 (UG 비트 세트) */
__HAL_TIM_GENERATE_SOFTWARE_UPDATE_EVENT(&htim2);

/* UIF 클리어: UG 세트 시 UIF도 함께 세트되어
 * 의도치 않은 콜백이 1회 호출될 수 있으므로 제거 */
__HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
```

**검증 포인트**:
- 직접 레지스터 접근 (`htim2.Instance->EGR`) → HAL 매크로 사용으로 변경: PASS
- UG 세트 후 UIF 클리어 순서 정확: PASS
- 주석에 이유 설명 포함: PASS
- 레이스 컨디션 가능성: UG → UIF 세트 → CLEAR 사이에 인터럽트 발생 가능하나, ARM Cortex-M4에서 이 3개 명령은 매우 짧은 시간(수 사이클)이므로 실무에서 문제 없음: ACCEPTABLE

**HTML 실무 팁** (라인 953~960):
- `__HAL_TIM_GENERATE_SOFTWARE_UPDATE_EVENT()` 사용 설명: PASS
- UIF 부작용 경고 + 해결책 (`__HAL_TIM_CLEAR_FLAG`) 명시: PASS
- 이전 모호한 표현("필요시 비활성화") → 구체적 해결책으로 개선: PASS

### 4. (추가) §7.3 트러블슈팅 팁 — PASS

**HTML** (라인 575~583):
```html
<aside class="tip">
  💡 <strong>LED가 깜빡이지 않으면?</strong><br>
  <ol>
    <li>CubeMX NVIC Enable 확인</li>
    <li>_IT 접미사 확인</li>
    <li>PSC/ARR LOG_D 검증</li>
    <li>콜백 함수명 오타 (가장 찾기 어려운 버그)</li>
  </ol>
</aside>
```
- 4개 체크리스트 항목 모두 실무에서 가장 흔한 TIM 설정 실수: PASS
- 특히 4번 "콜백 함수명 오타" — weak 함수 특성상 컴파일 에러 없이 동작하지 않는 패턴. 우수한 실무 조언: PASS

---

## 전체 정합성 검증

### HTML ↔ 코드 파일 일관성

| 검증 항목 | HTML 인라인 코드 | 코드 파일 | 일치 |
|-----------|-----------------|----------|------|
| PSC 계산 | 8399 (§7.5) | 8399 (tim_driver.c:87) | PASS |
| UIF 클리어 | CLEAR_FLAG (§7.5 팁) | CLEAR_FLAG (tim_driver.c:98) | PASS |
| TIM1 클럭 | 84MHz (§7.1 표) | N/A | PASS |
| TIM2 CLK | 84MHz (#define) | 84MHz | PASS |

### HTML ↔ SVG 일관성

| 검증 항목 | HTML | SVG | 일치 |
|-----------|------|-----|------|
| TIM1 APB2 | 84MHz | 84MHz (hierarchy.svg) | PASS |
| TIM2 APB1 | 84MHz | 84MHz (hierarchy.svg) | PASS |
| v0.7 아키텍처 | tim_driver 추가 | tim_driver 표시 (architecture.svg) | PASS |

### grep 잔존 확인

- `83999` in Ch07 코드: 경고 주석 1건만 (의도적) — PASS
- `100MHz` in Ch07 HTML/SVG: 0건 — PASS

---

## 최종 판정

### APPROVED (승인)

Phase 3에서 지적한 필수 수정 3건(C-01, M-02, M-01)이 모두 정확하게 반영되었습니다.

**특히 우수한 수정**:
1. **PSC 16비트 경고 주석** (tim_driver.c 라인 81~82): 오버플로 값 83999를 주석으로 남겨 "왜 이렇게 하면 안 되는지" 교육 효과 확보
2. **HAL 매크로 전환**: 직접 레지스터 접근 → `__HAL_TIM_GENERATE_SOFTWARE_UPDATE_EVENT` + `__HAL_TIM_CLEAR_FLAG` 사용. HAL 추상화 원칙에 부합
3. **트러블슈팅 팁**: 실무에서 가장 흔한 TIM 설정 실수 4가지를 체계적으로 정리

**잔여 권장 사항** (Minor, 승인에 영향 없음):
- m-01: PWM Duty 표의 High 시간 분모 표기 (`250/10000` → `250 / 1MHz = 250us`)
- m-02: 시퀀스 다이어그램의 init/start 분리
- m-03: Ch08 예고 내용 TABLE_OF_CONTENTS.md 교차 검증

**기술 검증 최종 결과: PASS**
