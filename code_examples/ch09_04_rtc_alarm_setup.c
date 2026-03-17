/* ch09_04_rtc_alarm_setup.c — AlarmA 설정 + 콜백
 *
 * 레이어: App (HAL 직접 호출)
 * 의존: HAL_RTC, log.h
 *
 * AlarmA: 특정 시:분:초에 인터럽트 발생
 * 마스크 비트: ON = 해당 필드 무시 (더 자주 울림)
 */

#include "main.h"
#include "log.h"

extern RTC_HandleTypeDef hrtc;

/* ===== 알람 이벤트 플래그 (ISR → main loop) ===== */
static volatile uint8_t alarm_a_flag = 0;

/* ===== AlarmA 설정: 매일 07시 30분 00초 ===== */

void rtc_alarm_set_daily(void)
{
    RTC_AlarmTypeDef alarm = {0};

    alarm.AlarmTime.Hours   = 7;
    alarm.AlarmTime.Minutes = 30;
    alarm.AlarmTime.Seconds = 0;

    /* 마스크 설정: 날짜/요일만 마스크 → 매일 07:30:00에 트리거 */
    alarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;
    /*
     * 마스크 옵션:
     *   RTC_ALARMMASK_NONE         — 모든 필드 비교 (정확한 1회)
     *   RTC_ALARMMASK_DATEWEEKDAY  — 날짜/요일 무시 (매일)
     *   RTC_ALARMMASK_HOURS        — 시 무시 (매시간)
     *   RTC_ALARMMASK_MINUTES      — 분 무시
     *   RTC_ALARMMASK_SECONDS      — 초 무시
     *   조합 가능: RTC_ALARMMASK_HOURS | RTC_ALARMMASK_DATEWEEKDAY
     */

    alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    alarm.AlarmDateWeekDay    = 1;    /* 마스크되어 무시됨 */
    alarm.Alarm = RTC_ALARM_A;

    /* 인터럽트 모드로 AlarmA 설정 */
    if (HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN) != HAL_OK) {
        LOG_E("AlarmA 설정 실패!");
        return;
    }

    LOG_I("AlarmA 설정 완료: 매일 %02d:%02d:%02d",
          alarm.AlarmTime.Hours,
          alarm.AlarmTime.Minutes,
          alarm.AlarmTime.Seconds);
}

/* ===== AlarmA 설정: 10초 후 (테스트용) ===== */

void rtc_alarm_set_after_seconds(uint8_t seconds)
{
    RTC_TimeTypeDef now;
    RTC_DateTypeDef date;
    RTC_AlarmTypeDef alarm = {0};

    /* 현재 시각 읽기 */
    HAL_RTC_GetTime(&hrtc, &now, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);   /* 필수! */

    /* 현재 시각 + seconds 후 알람 설정 */
    uint8_t target_sec = (now.Seconds + seconds) % 60;
    uint8_t target_min = now.Minutes + ((now.Seconds + seconds) / 60);
    uint8_t target_hour = now.Hours + (target_min / 60);
    target_min %= 60;
    target_hour %= 24;

    alarm.AlarmTime.Hours   = target_hour;
    alarm.AlarmTime.Minutes = target_min;
    alarm.AlarmTime.Seconds = target_sec;
    alarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;   /* 매일 반복 */
    alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    alarm.AlarmDateWeekDay = 1;
    alarm.Alarm = RTC_ALARM_A;

    if (HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN) != HAL_OK) {
        LOG_E("AlarmA 설정 실패!");
        return;
    }

    LOG_I("AlarmA 설정: %d초 후 → %02d:%02d:%02d",
          seconds, target_hour, target_min, target_sec);
}

/* ===== AlarmA 콜백 (ISR 컨텍스트) ===== */

/**
 * @brief RTC AlarmA 이벤트 콜백 — 학습용 단독 예제
 *
 * ⚠️ ISR에서는 플래그만 설정!
 *    LED, 부저 등 실제 처리는 main loop에서 수행합니다.
 *    (Ch08 Stopwatch FSM과 동일 패턴)
 *
 * ⚠️ 최종 프로젝트에는 이 파일의 콜백을 사용하지 마세요!
 *    ch09_06(rtc_driver) + ch09_07(alarm_service)만 포함하세요.
 *    HAL 콜백은 프로젝트당 하나만 정의 가능합니다.
 */
#if 0  /* 학습용 — 최종 프로젝트에서는 ch09_06_rtc_driver.c의 콜백 사용 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc_cb)
{
    alarm_a_flag = 1;
    /* ISR에서는 이것이 전부! 나머지는 main loop에서 */
}
#endif

/* ===== main loop에서 알람 처리 ===== */

void rtc_alarm_process(void)
{
    if (alarm_a_flag) {
        alarm_a_flag = 0;

        LOG_I("*** AlarmA 트리거! ***");

        /* LED 토글 (PA5, 온보드 LD2) */
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        LOG_D("LED 토글 완료");

        /* 현재 시각 확인 */
        RTC_TimeTypeDef time;
        RTC_DateTypeDef date;
        HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);   /* hrtc: 전역 핸들 사용 */
        HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
        LOG_I("알람 발생 시각: %02d:%02d:%02d",
              time.Hours, time.Minutes, time.Seconds);
    }
}

/* ===== 알람 해제 ===== */

void rtc_alarm_deactivate(void)
{
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    LOG_I("AlarmA 해제 완료");
}
