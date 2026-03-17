# Ch13 기술 리뷰 (Technical Review)
## 리뷰어: 기술 리뷰어 (Technical Reviewer)
## 작성일: 2026-03-17

---

## 검토 범위
- `manuscripts/part3/ch13.html` — HTML 원고
- `code_examples/ch13_gfx_service.c/.h`
- `code_examples/ch13_ui_service.c/.h`
- `code_examples/ch13_display_fsm.c`

---

## 🔴 Critical 이슈

### [RESOLVED] C99 가변 길이 배열 대신 고정 배열 사용 — 해당 없음
- ch13_gfx_service.c는 고정 크기 배열 `s_font8x16[][16]`을 사용함 — OK

### [CRITICAL-1] `GFX_FillCircle`에서 `sqrtf` 사용 — math.h 미포함
- **위치**: `ch13_gfx_service.c`, `GFX_FillCircle()` 함수
- **문제**: `sqrtf()` 함수를 사용하지만 `#include <math.h>`가 없음
- **수정 방법**: `#include <math.h>` 추가 및 링커 플래그 `-lm` 확인
  ```c
  #include <math.h>  /* sqrtf 사용 시 필요 */
  ```
- **중요도**: 🔴 Critical (컴파일 오류 유발)

**→ Phase 4에서 수정 완료됨**

---

## 🟡 Major 이슈

### [MAJOR-1] `UI_Render_StopwatchView_Partial`에서 unsigned 타입 포맷 지정자 불일치
- **위치**: `ch13_ui_service.c`, `snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", min, sec, csec)`
- **문제**: `min`, `sec`, `csec`가 `uint32_t`이지만 `%lu`(unsigned long)를 사용함.
  STM32 HAL에서 `uint32_t` = `unsigned int` (32-bit)이므로 `%u` 또는 `(unsigned long)` 캐스팅이 필요.
- **수정 방법**: `(unsigned long)` 캐스팅 추가
  ```c
  snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu",
           (unsigned long)min, (unsigned long)sec, (unsigned long)csec);
  ```
- **중요도**: 🟡 Major (경고 발생, 일부 컴파일러에서 오류)

**→ Phase 4에서 수정 완료됨**

### [MAJOR-2] `GFX_DrawChar`에서 scale=0 처리 없음
- **위치**: `ch13_gfx_service.c`, `GFX_DrawChar()`
- **문제**: `scale=0`이 전달되면 `GFX_FillRect(..., 0, 0, ...)` 호출로 무한 루프 위험
- **수정 방법**: scale 최소값 검사 추가
  ```c
  if (scale == 0) scale = 1;  /* 방어적 코딩 */
  ```
- **중요도**: 🟡 Major (잠재적 버그)

**→ Phase 4에서 수정 완료됨**

### [MAJOR-3] `UI_Render_StopwatchView_Partial`에서 `lap_times` 배열 인덱스 범위 초과 가능성
- **위치**: `ch13_ui_service.c`, `i < lap_count && i < 5U`
- **문제**: `lap_count`가 5를 초과하면 `start_lap`이 `lap_count - 4`가 되어야 하는데
  계산식 `start_lap = (lap_count > SW_MAX_LAP_ROWS) ? (lap_count - SW_MAX_LAP_ROWS) : 0U`에서
  `lap_times[i]` 접근 시 i가 최대 4까지만 접근하므로 범위 초과가 없음.
  그러나 `lap_count`가 `uint8_t`이고 255까지 증가할 수 있는 외부 입력이면 문제 발생.
- **수정 방법**: `UI_StopwatchData_t.lap_count`를 5로 상한 제한하는 로직 추가 또는 주석으로 명시
- **중요도**: 🟡 Major (잠재적 버그, 방어 코드 필요)

**→ Phase 4에서 주석으로 명시 및 상한 처리 추가**

---

## 🟢 Minor 이슈

### [MINOR-1] 색상 상수 `GFX_COLOR_ORANGE` 오류
- `0xFD20U`는 실제로 순수 오렌지(R=31, G=41, B=0)가 아님.
  순수 주황에 가까운 값은 `0xFC00U` (R=31, G=32, B=0) 정도이나, 기능적 영향은 없음.
- **중요도**: 🟢 Minor (색상 조금 다를 뿐 오류 아님)

### [MINOR-2] `GFX_DrawFloat1`에서 음수 처리 로직
- 정수부가 음수인 경우 소수부도 음수로 표시되어야 하는 경우 처리 필요.
  예: -3.5°C → `integer=-3, decimal=5` (소수부는 양수로 전달하는 것이 관례)
- 헤더 주석에 "소수부는 항상 양수로 전달할 것" 명시 권장.
- **중요도**: 🟢 Minor (사용 관례 명시 필요)

### [MINOR-3] `draw_weekday_str` 함수에서 문자열 리터럴 배열 static 키워드
- `static const char * const weekday_str[]` → 이미 `static const`로 되어 있음 — OK

### [MINOR-4] `HAL_GPIO_EXTI_Callback` 중복 정의 가능성
- `ch13_display_fsm.c`에 예시로 포함된 `HAL_GPIO_EXTI_Callback`이 프로젝트의 다른 파일에서도
  정의되면 링커 오류 발생.
- **수정**: 교육용 코드임을 명시하는 주석 강화 (현재 충분히 명시되어 있음)
- **중요도**: 🟢 Minor (교육용 파일 특성상 허용)

---

## ✅ 긍정적 평가

1. **레이어 경계 규칙 준수**: `gfx_service.c`만 `ili9341_driver.h`를 include하는 원칙이 코드와 설명에서 일관되게 적용됨. ✅
2. **LOG_D/I/W/E 사용**: 모든 함수에 적절한 로그 레벨 적용됨. ✅
3. **경계 클리핑**: `GFX_FillRect`, `GFX_DrawPixel`에서 LCD 범위 초과 처리 구현됨. ✅
4. **ISR-safe 패턴**: `UI_RequestRefresh()`와 메인 루프 `UI_ProcessRefresh()` 분리 패턴 정확히 구현됨. ✅
5. **테이블 기반 FSM**: 구조체 배열로 전이 테이블 구현, 확장성 우수. ✅
6. **Dirty Flag 패턴**: 5절에서 명확히 설명되고 코드로 구현됨. ✅
7. **비트맵 폰트 테이블**: ASCII 0x20~0x7E 범위를 완전히 커버하는 폰트 테이블 포함됨. ✅
8. **코드 블록 30줄 이하**: 대부분의 함수가 30줄 이하 — UI 렌더링 함수는 레이아웃 특성상 다소 길지만 허용 범위. ✅

---

## 기술 정확성 검증

| 항목 | 검증 결과 |
|------|---------|
| RGB565 포맷 설명 (R5G6B5) | ✅ 정확 |
| ILI9341 Address Window (0x2A, 0x2B) | ✅ 정확 |
| NUCLEO-F411RE User Button = PC13 | ✅ 정확 |
| Bresenham 직선 알고리즘 구현 | ✅ 정확 |
| Midpoint Circle 알고리즘 | ✅ 정확 |
| SPI DMA 전송 시간 계산 (63ms @ 28MHz) | ✅ 계산 정확 (효율 70% 가정 합리적) |
| `HAL_RTC_AlarmAEventCallback` 프로토타입 | ✅ 정확 |
| `HAL_GPIO_EXTI_Callback` 프로토타입 | ✅ 정확 |

---

## 최종 평가

**Critical**: 1건 (math.h 미포함)
**Major**: 3건 (포맷 지정자, scale=0, 배열 상한)
**Minor**: 4건

→ Phase 4 수정 후 승인 가능
