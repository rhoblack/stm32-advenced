# Ch09. 내부 RTC 활용 — 기술 리뷰어 의견서

**작성자:** 기술 리뷰어 (Technical Reviewer)
**작성일:** 2026-03-17
**Phase:** 3 (병렬 리뷰)
**프로젝트 버전:** v0.8 → v0.9
**검토 범위:** HTML 원고 + 코드 7개 + SVG 8개

---

## 1. 검토 요약

Ch09 원고는 RTC 아키텍처, BCD 레지스터, AlarmA/Wake-up Timer, Alarm FSM을 체계적으로 다루고 있으며, HAL API 사용이 전반적으로 정확합니다. Ch08 FSM 패턴의 반복 적용이 자연스럽고, 레이어드 아키텍처(rtc_driver + alarm_service) 설계가 일관성 있습니다.

---

## 2. Critical Issues (🔴) — 0건

없음.

---

## 3. Major Issues (🟡) — 3건

### 🟡 M-1: ch09_04_rtc_alarm_setup.c — rtc_alarm_process()에서 hrtc_cb 잘못 참조

**파일:** `code_examples/ch09_04_rtc_alarm_setup.c`, 라인 122~124

```c
void rtc_alarm_process(void)
{
    if (alarm_a_flag) {
        alarm_a_flag = 0;
        // ...
        HAL_RTC_GetTime(hrtc_cb, &time, RTC_FORMAT_BIN);  // ❌
        HAL_RTC_GetDate(hrtc_cb, &date, RTC_FORMAT_BIN);  // ❌
```

`hrtc_cb`는 `HAL_RTC_AlarmAEventCallback()`의 매개변수로, 해당 함수 스코프 밖에서 사용할 수 없습니다. `rtc_alarm_process()`는 main loop 함수이므로 전역 `hrtc`를 사용해야 합니다.

**수정:** `hrtc_cb` → `&hrtc`

---

### 🟡 M-2: HAL_RTC_AlarmAEventCallback() 중복 정의

**파일:** `ch09_04_rtc_alarm_setup.c` (라인 101)과 `ch09_06_rtc_driver.c` (라인 253) 모두에서 `HAL_RTC_AlarmAEventCallback()`을 정의합니다. 마찬가지로 `HAL_RTCEx_WakeUpTimerEventCallback()`도 `ch09_05_wakeup_timer.c`와 `ch09_06_rtc_driver.c`에서 중복 정의됩니다.

이 파일들이 독립 예제(같은 프로젝트에 동시 링크하지 않음)라면 문제없지만, **교재에서 이 점을 명시해야 합니다.** 학생이 모든 .c 파일을 하나의 프로젝트에 추가하면 링커 에러(`multiple definition`)가 발생합니다.

**권장:** §9.8 실습 섹션에 "ch09_01~05는 개별 학습용 예제이며, 최종 프로젝트에는 ch09_06(rtc_driver) + ch09_07(alarm_service)만 포함합니다" 주의 문구 추가.

---

### 🟡 M-3: rtc_set_wakeup() — seconds=0 전달 시 언더플로우

**파일:** `ch09_06_rtc_driver.c` 라인 216, `ch09_05_wakeup_timer.c` 라인 43

```c
HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, seconds - 1, ...);
```

`seconds`가 0으로 전달되면 `0 - 1 = 0xFFFF` (uint16_t 언더플로우)가 되어 약 18시간 주기의 Wake-up 타이머가 설정됩니다. 입력 검증이 필요합니다.

**수정:**
```c
if (seconds == 0) {
    LOG_W("Wake-up 주기는 1초 이상이어야 합니다");
    return RTC_ERROR;
}
```

---

## 4. Minor Issues (🟢) — 6건

### 🟢 m-1: LSI 오차 설명 — "하루 72분" 표현 재검토

§9.2 및 FAQ에서 "LSI ±5% → 하루 최대 ~72분 오차"라고 기술합니다. 계산: 86,400초 × 5% = 4,320초 = 72분. 이 계산은 정확합니다.

다만 STM32F411 데이터시트(DS10314)에서 LSI 주파수는 **17kHz ~ 47kHz (typical 32kHz)**로, ±5%가 아니라 실제 편차가 훨씬 큽니다. 교재에서는 RM0383 Reference Manual의 "±5%" 수치를 사용했는데, 이는 **25°C 기준 nominal 값**이며 온도 변화를 고려하면 더 클 수 있습니다.

**권장:** "±5% (25°C 기준, 온도 변화 시 더 클 수 있음)" 정도의 부가 설명 추가.

---

### 🟢 m-2: rtc_driver.h 헤더 — RTC_TIMEOUT 열거값 불일치

`ch09_06_rtc_driver.c` 헤더 주석(라인 21)에 `RTC_TIMEOUT = 2`가 정의되어 있지만, 실제 코드에서는 RTC_TIMEOUT을 반환하는 경로가 없습니다. 사용하지 않는 열거값은 제거하거나, HAL_TIMEOUT 발생 시 RTC_TIMEOUT을 반환하는 코드를 추가하는 것이 좋습니다.

**권장:** 사용하지 않으면 제거하여 혼란 방지. 또는 HAL_RTC_SetTime() 등의 반환값 HAL_TIMEOUT을 매핑.

---

### 🟢 m-3: alarm_service.c — ACKNOWLEDGED 자동 복귀 미구현

`ch09_07_alarm_service.c` 라인 216~217:
```c
/* ACKNOWLEDGED 상태 자동 복귀 (5초 후) */
/* 실제 구현에서는 별도 타이머 필요 — 여기서는 간략화 */
```

FSM 전이표(§9.6)에서 ACKNOWLEDGED → IDLE(TIMEOUT)은 정의되어 있지만, `alarm_service_process()`에서 이 전이를 트리거하는 타이머 코드가 없습니다. TRIGGERED 타임아웃은 구현되어 있으나 ACKNOWLEDGED 타임아웃은 주석으로만 남아 있습니다.

**권장:** TRIGGERED 타임아웃과 같은 패턴으로 ACKNOWLEDGED 자동 복귀를 구현하거나, 교재에서 "연습문제"로 남기는 것을 명시.

---

### 🟢 m-4: BCD 변환 함수명 — 교재와 코드 불일치

§9.1 교재 본문에서 BCD 변환 함수를:
```c
uint8_t bcd_to_bin(uint8_t bcd);
uint8_t bin_to_bcd(uint8_t bin);
```
로 설명하지만, `ch09_01_bcd_read_write.c`에서는:
```c
static uint8_t bcd_to_binary(uint8_t bcd);
static uint8_t binary_to_bcd(uint8_t bin);
```
로 정의합니다. 함수명이 다릅니다(`bin` vs `binary`).

**권장:** 교재 본문과 코드 예제의 함수명 통일.

---

### 🟢 m-5: HAL_RTC_GetTime() 반환값 미확인

`ch09_06_rtc_driver.c`의 `rtc_get_time()`, `rtc_get_date()` 함수에서 HAL_RTC_GetTime/GetDate의 반환값(HAL_StatusTypeDef)을 확인하지 않고 무조건 `RTC_OK`를 반환합니다.

```c
HAL_RTC_GetTime(&hrtc, &hal_time, RTC_FORMAT_BIN);  // 반환값 무시
HAL_RTC_GetDate(&hrtc, &hal_date, RTC_FORMAT_BIN);  // 반환값 무시
return RTC_OK;
```

실무에서 HAL_RTC_GetTime이 실패하는 경우는 극히 드물지만, `rtc_set_time()`에서는 반환값을 확인하고 있으므로 일관성을 위해 `rtc_get_time()`에서도 확인하는 것이 좋습니다.

**권장:** 반환값 확인 추가 또는, 교재에서 "GetTime은 실패하지 않으므로 생략"이라는 주석 추가.

---

### 🟢 m-6: v0.9 main.c 통합 코드 — 주석 오류

§9.8 main.c 통합 코드(라인 1074):
```c
/* stopwatch_process();       /* 스톱워치 FSM (병행) */
```

중첩 주석(`/* ... /* ... */`)은 C 표준에서 정의되지 않은 동작이며, 대부분의 컴파일러에서 경고가 발생합니다. 주석 처리를 `//`로 변경하거나 올바르게 닫아야 합니다.

**수정:**
```c
// stopwatch_process();       /* 스톱워치 FSM (병행) */
```

---

## 5. HAL API 정확성 검증

| API | 사용 방법 | 정확성 |
|-----|----------|--------|
| HAL_RTC_GetTime() | RTC_FORMAT_BIN, 구조체 전달 | ✅ 정확 |
| HAL_RTC_GetDate() | GetTime 후 반드시 호출 | ✅ 정확, 3회 반복 강조 우수 |
| HAL_RTC_SetTime() | RTC_FORMAT_BIN, DayLightSaving/StoreOperation 설정 | ✅ 정확 |
| HAL_RTC_SetDate() | Year(0~99), Month(RTC_MONTH_*), WeekDay(RTC_WEEKDAY_*) | ✅ 정확 |
| HAL_RTC_SetAlarm_IT() | RTC_FORMAT_BIN, AlarmMask, RTC_ALARM_A | ✅ 정확 |
| HAL_RTC_DeactivateAlarm() | RTC_ALARM_A | ✅ 정확 |
| HAL_RTCEx_SetWakeUpTimer_IT() | CK_SPRE_16BITS, (seconds-1) | ✅ 정확 (M-3 입력 검증 제외) |
| HAL_RTCEx_BKUPRead/Write() | RTC_BKP_DR0~DR19 | ✅ 정확 |
| RTC_ByteToBcd2() / RTC_Bcd2ToByte() | HAL 매크로 | ✅ 정확 |
| HAL_RTC_AlarmAEventCallback() | weak 오버라이드 | ✅ 정확 |
| HAL_RTCEx_WakeUpTimerEventCallback() | weak 오버라이드 | ✅ 정확 |

---

## 6. NUCLEO-F411RE RTC 특성 검증

| 항목 | 교재 설명 | 검증 결과 |
|------|----------|----------|
| LSE 기본 미장착 | ✅ SB48/SB49 열림 명시 | 정확 — UM1724 참조 |
| LSI 대체 사용 | ✅ ±5% 편차 명시 | 정확 (m-1 부가설명 권장) |
| VBAT = VDD 연결 | ✅ 전원 끄면 시간 소실 명시 | 정확 |
| 백업 레지스터 20개 | ✅ RTC_BKP_DR0~19 | 정확 |
| RTC 프리스케일러 | ✅ PREDIV_A=127, PREDIV_S=255 → 1Hz | 정확 |
| Alarm EXTI line 17 | ✅ CubeMX 설정에서 명시 | 정확 |

---

## 7. 아키텍처 검증

| 항목 | 검증 결과 |
|------|----------|
| rtc_driver → Driver 레이어 | ✅ HAL 직접 호출, 상위에 추상 인터페이스 제공 |
| alarm_service → Service 레이어 | ✅ 비즈니스 로직(FSM), rtc_driver만 호출 |
| rtc_driver ↔ tim_driver 독립성 | ✅ 의존성 없음 |
| alarm_service ↔ stopwatch_service 독립성 | ✅ 의존성 없음, main.c에서 병렬 process() |
| HAL 콜백 → Driver 콜백 패턴 | ✅ Ch07/Ch08 패턴과 동일 |
| v0.8 기존 코드 변경 없음 | ✅ 추가만, 교체 없음 |

---

## 8. 코드 품질 검증

| 항목 | 검증 결과 |
|------|----------|
| 30줄 이하 | ✅ 모든 함수 30줄 이내 |
| LOG_D/I/W/E 적용 | ✅ 전 코드에서 일관 적용 |
| 스네이크 케이스 | ✅ rtc_get_time, alarm_service_init 등 |
| 한국어 주석 | ✅ 모든 코드에 한국어 주석 |
| GetTime+GetDate 쌍 호출 | ✅ 모든 GetTime 사용처에서 GetDate 동반 |
| ISR 안전성 (플래그만 설정) | ✅ Ch08 패턴 일관 |
| volatile 사용 | ✅ s_alarm_flag, s_state에 volatile 적용 |

---

## 9. 콘텐츠 품질 검증

| 항목 | 검증 결과 |
|------|----------|
| 절당 2000~4000자 | ✅ 추정 범위 내 |
| 새 개념 시 비유 | ✅ 벽걸이 시계, 전광판, 알람시계 vs 주방타이머, 석영 vs 기계식 |
| 기술 용어 첫 등장 시 설명 | ✅ RTC(Real-Time Clock), BCD(Binary-Coded Decimal) 등 |
| 감정 곡선 | ✅ 호기심→불안→이해→성취→자부심 |
| Ch08 패턴 재활용 강조 | ✅ 1:1 대응표 포함 (§9.7) |
| Aside 박스 | ✅ tip(3), faq(5), interview(3), metacognition(4), instructor-tip(3) = 18개 |

---

## 10. SVG 다이어그램 검증

8개 SVG 파일 존재 확인:
- ch09_sec0_architecture.svg — v0.8→v0.9 아키텍처 ✅
- ch09_sec1_rtc_backup.svg — RTC 블록 다이어그램 ✅
- ch09_sec1_bcd_structure.svg — BCD 레지스터 구조 ✅
- ch09_sec1_lse_vs_lsi.svg — LSE/LSI 비교 ✅
- ch09_sec4_alarm_timing.svg — 알람 마스크 동작 ✅
- ch09_sec5_wakeup_vs_alarm.svg — Alarm vs Wake-up ✅
- ch09_sec6_alarm_fsm.svg — FSM 상태 전이도 ✅
- ch09_sec8_v09_final.svg — v0.9 최종 아키텍처 ✅

---

## 11. 최종 평가

| 항목 | 결과 |
|------|------|
| Critical (🔴) | **0건** |
| Major (🟡) | **3건** (M-1 hrtc_cb 참조 오류, M-2 콜백 중복 주의 문구, M-3 언더플로우) |
| Minor (🟢) | **6건** |
| HAL API 정확성 | ✅ PASS |
| NUCLEO-F411RE 특성 | ✅ PASS |
| 아키텍처 일관성 | ✅ PASS |
| 코드 품질 | ✅ PASS |

### 판정: ✅ 조건부 승인 (Conditional Approval)

**필수 수정 (Major 3건):**
1. M-1: `ch09_04_rtc_alarm_setup.c` — `hrtc_cb` → `&hrtc` 수정
2. M-2: 교재 또는 코드에 "개별 예제 vs 통합 프로젝트" 구분 주의 문구 추가
3. M-3: `rtc_set_wakeup()`에 seconds=0 입력 검증 추가

**권장 수정 (Minor 6건):** 가능하면 반영, 필수는 아님.

Major 3건 수정 후 최종 승인 가능합니다.

---

*기술 리뷰어 (Technical Reviewer) — Phase 3 리뷰 완료*
