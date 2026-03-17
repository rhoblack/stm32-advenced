# Ch07 TIM 기초 — Phase 4 수정 완료 보고서

## 작성일: 2026-03-17
## 작성자: 기술 저자

---

## 수정 4건 완료 요약

### 1. TIM1 클럭: 100MHz → 84MHz

**수정 파일 2개:**

#### (a) `manuscripts/part2/chapter07.html` (§7.1 타이머 계층 표)
- **변경 전**: `<td>APB2 (100MHz)</td>`
- **변경 후**: `<td>APB2 (84MHz)</td>`
- **위치**: §7.1 타이머 계층 표, TIM1 행

#### (b) `figures/ch07_sec01_timer_hierarchy.svg`
- **변경 전**: `16비트 | APB2(100MHz) | Ch08에서 학습`
- **변경 후**: `16비트 | APB2(84MHz) | Ch08에서 학습`

**근거**: STM32F411RE HCLK=84MHz, APB2 프리스케일러=1 → APB2 타이머 클럭 = 84MHz

---

### 2. PSC 오버플로우: 83999 → 8399

**수정 파일 2개:**

#### (a) `code_examples/ch07_04_tim_driver.c` — tim_set_period_ms()
- **변경 전**:
```c
if (period_ms >= 10) {
    psc = (TIM2_CLK_HZ / 1000) - 1;  /* 83999 ← 16비트 오버플로! */
    arr = period_ms - 1;
} else {
    psc = (TIM2_CLK_HZ / 10000) - 1; /* 8399 */
    arr = period_ms * 10 - 1;
}
```
- **변경 후**:
```c
/* 10kHz 카운터 기준 (PSC=8399, 16비트 범위 안전) */
psc = (TIM2_CLK_HZ / 10000) - 1;  /* 8399 */
arr = period_ms * 10 - 1;
```
- **주석에 오버플로 경고 추가**: `※ 주의: PSC 레지스터는 16비트(max 65535)`

#### (b) `manuscripts/part2/chapter07.html` — §7.5 인라인 코드
- 동일하게 10kHz 카운터 기준으로 통일
- 83999 참조 완전 제거

**검증**:
- 1ms: PSC=8399, ARR=9 → (8400 × 10) / 84,000,000 = 0.001s = 1ms ✓
- 500ms: PSC=8399, ARR=4999 → (8400 × 5000) / 84,000,000 = 0.5s ✓
- 60000ms: PSC=8399, ARR=599999 → TIM2 32비트이므로 범위 내 ✓

---

### 3. UIF 클리어 미처리

**수정 파일 2개:**

#### (a) `code_examples/ch07_04_tim_driver.c` — tim_set_period_ms()
- **변경 전**:
```c
htim2.Instance->EGR = TIM_EGR_UG;
```
- **변경 후**:
```c
__HAL_TIM_GENERATE_SOFTWARE_UPDATE_EVENT(&htim2);
__HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
```

#### (b) `manuscripts/part2/chapter07.html` — §7.5 실무 팁
- EGR UG 직접 접근 → HAL 매크로 사용으로 변경
- UIF 클리어 필요성 설명 추가
- 변경 전: "필요시 인터럽트를 잠시 비활성화하세요"
- 변경 후: "`__HAL_TIM_CLEAR_FLAG()`로 UIF를 클리어하세요" (구체적 해결책)

---

### 4. §7.3 트러블슈팅 팁 추가

**수정 파일 1개:**

#### `manuscripts/part2/chapter07.html` — §7.3 끝 (면접 포인트 다음)
- **추가 내용**: `<aside class="tip">` 형태의 체크리스트 4항목:
  1. CubeMX NVIC Enable 확인
  2. `_IT` 접미사 확인
  3. PSC/ARR 값 LOG_D 검증
  4. 콜백 함수명 오타 확인 (가장 찾기 어려운 버그 경고)

---

## 수정 파일 목록 (총 4개)

| 파일 | 수정 건 |
|------|---------|
| `manuscripts/part2/chapter07.html` | 1,2,3,4 (4건 모두) |
| `figures/ch07_sec01_timer_hierarchy.svg` | 1 (TIM1 클럭) |
| `code_examples/ch07_04_tim_driver.c` | 2,3 (PSC + UIF) |
| `review_logs/ch07_author_corrections_completed.md` | 본 보고서 |

## 검증 완료
- [x] 83999 참조 완전 제거 (grep 확인: 0건)
- [x] 100MHz 참조 완전 제거 (grep 확인: 0건)
- [x] HAL API 정확성: `__HAL_TIM_GENERATE_SOFTWARE_UPDATE_EVENT`, `__HAL_TIM_CLEAR_FLAG` 사용
- [x] PSC 16비트 범위: 8399 < 65535 ✓
- [x] 주기 계산 검증: 1ms~60000ms 범위 정상
