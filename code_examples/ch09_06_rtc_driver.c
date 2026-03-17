/* ch09_06_rtc_driver.c — RTC Driver 아키텍처
 *
 * 레이어: Driver
 * 의존: HAL_RTC, HAL_RTCEx, log.h
 *
 * tim_driver (Ch07) 패턴을 재활용한 rtc_driver 구현
 * - init → get/set → alarm → wakeup → backup
 * - 상위 레이어(alarm_service)에 범용 인터페이스 제공
 */

/* ===== ch09_06_rtc_driver.h — 공개 인터페이스 ===== */
/*
 * #ifndef RTC_DRIVER_H
 * #define RTC_DRIVER_H
 *
 * #include <stdint.h>
 *
 * typedef enum {
 *     RTC_OK       = 0,
 *     RTC_ERROR    = 1,
 *     RTC_TIMEOUT  = 2
 * } rtc_status_t;
 *
 * typedef struct {
 *     uint8_t hours;
 *     uint8_t minutes;
 *     uint8_t seconds;
 * } rtc_time_t;
 *
 * typedef struct {
 *     uint8_t year;      // 0~99 (2000+year)
 *     uint8_t month;     // 1~12
 *     uint8_t date;      // 1~31
 *     uint8_t weekday;   // 1~7 (Mon~Sun)
 * } rtc_date_t;
 *
 * typedef void (*rtc_alarm_callback_t)(void);
 * typedef void (*rtc_wakeup_callback_t)(void);
 *
 * // 초기화
 * rtc_status_t rtc_driver_init(void);
 *
 * // 시간/날짜 읽기·쓰기
 * rtc_status_t rtc_get_time(rtc_time_t *time);
 * rtc_status_t rtc_get_date(rtc_date_t *date);
 * rtc_status_t rtc_set_time(const rtc_time_t *time);
 * rtc_status_t rtc_set_date(const rtc_date_t *date);
 *
 * // 알람
 * rtc_status_t rtc_set_alarm_a(const rtc_time_t *time, uint32_t mask);
 * rtc_status_t rtc_deactivate_alarm_a(void);
 * void         rtc_register_alarm_callback(rtc_alarm_callback_t cb);
 *
 * // Wake-up 타이머
 * rtc_status_t rtc_set_wakeup(uint16_t seconds);
 * rtc_status_t rtc_stop_wakeup(void);
 * void         rtc_register_wakeup_callback(rtc_wakeup_callback_t cb);
 *
 * // 백업 레지스터
 * uint32_t     rtc_backup_read(uint32_t reg);
 * void         rtc_backup_write(uint32_t reg, uint32_t value);
 *
 * #endif // RTC_DRIVER_H
 */

/* ===== ch09_06_rtc_driver.c — 구현 ===== */

#include "rtc_driver.h"
#include "log.h"

extern RTC_HandleTypeDef hrtc;

/* ===== 내부 상태 ===== */
static rtc_alarm_callback_t  s_alarm_cb  = NULL;
static rtc_wakeup_callback_t s_wakeup_cb = NULL;

/* ===== 초기화 ===== */

rtc_status_t rtc_driver_init(void)
{
    LOG_I("rtc_driver 초기화");

    /* CubeMX에서 RTC 초기화 완료 — 여기서는 콜백 초기화만 */
    s_alarm_cb  = NULL;
    s_wakeup_cb = NULL;

    LOG_D("  RTC 클럭: %s",
          __HAL_RCC_GET_RTC_SOURCE() == RCC_RTCCLKSOURCE_LSE ? "LSE" : "LSI");

    return RTC_OK;
}

/* ===== 시간/날짜 읽기 ===== */

rtc_status_t rtc_get_time(rtc_time_t *time)
{
    RTC_TimeTypeDef hal_time;
    RTC_DateTypeDef hal_date;

    /* ⚠️ GetTime + GetDate 쌍 호출 필수! */
    HAL_RTC_GetTime(&hrtc, &hal_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &hal_date, RTC_FORMAT_BIN);

    if (time != NULL) {
        time->hours   = hal_time.Hours;
        time->minutes = hal_time.Minutes;
        time->seconds = hal_time.Seconds;
    }

    return RTC_OK;
}

rtc_status_t rtc_get_date(rtc_date_t *date)
{
    RTC_TimeTypeDef hal_time;
    RTC_DateTypeDef hal_date;

    HAL_RTC_GetTime(&hrtc, &hal_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &hal_date, RTC_FORMAT_BIN);

    if (date != NULL) {
        date->year    = hal_date.Year;
        date->month   = hal_date.Month;
        date->date    = hal_date.Date;
        date->weekday = hal_date.WeekDay;
    }

    return RTC_OK;
}

/* ===== 시간/날짜 설정 ===== */

rtc_status_t rtc_set_time(const rtc_time_t *time)
{
    RTC_TimeTypeDef hal_time = {0};

    hal_time.Hours   = time->hours;
    hal_time.Minutes = time->minutes;
    hal_time.Seconds = time->seconds;
    hal_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    hal_time.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(&hrtc, &hal_time, RTC_FORMAT_BIN) != HAL_OK) {
        LOG_E("RTC 시간 설정 실패");
        return RTC_ERROR;
    }

    LOG_I("RTC 시간 설정: %02d:%02d:%02d",
          time->hours, time->minutes, time->seconds);
    return RTC_OK;
}

rtc_status_t rtc_set_date(const rtc_date_t *date)
{
    RTC_DateTypeDef hal_date = {0};

    hal_date.Year    = date->year;
    hal_date.Month   = date->month;
    hal_date.Date    = date->date;
    hal_date.WeekDay = date->weekday;

    if (HAL_RTC_SetDate(&hrtc, &hal_date, RTC_FORMAT_BIN) != HAL_OK) {
        LOG_E("RTC 날짜 설정 실패");
        return RTC_ERROR;
    }

    LOG_I("RTC 날짜 설정: 20%02d-%02d-%02d",
          date->year, date->month, date->date);
    return RTC_OK;
}

/* ===== AlarmA 설정/해제 ===== */

rtc_status_t rtc_set_alarm_a(const rtc_time_t *time, uint32_t mask)
{
    RTC_AlarmTypeDef alarm = {0};

    alarm.AlarmTime.Hours   = time->hours;
    alarm.AlarmTime.Minutes = time->minutes;
    alarm.AlarmTime.Seconds = time->seconds;
    alarm.AlarmMask = mask;
    alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    alarm.AlarmDateWeekDay = 1;
    alarm.Alarm = RTC_ALARM_A;

    if (HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN) != HAL_OK) {
        LOG_E("AlarmA 설정 실패");
        return RTC_ERROR;
    }

    LOG_I("AlarmA 설정: %02d:%02d:%02d (마스크=0x%02lX)",
          time->hours, time->minutes, time->seconds, mask);
    return RTC_OK;
}

rtc_status_t rtc_deactivate_alarm_a(void)
{
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    LOG_I("AlarmA 해제");
    return RTC_OK;
}

void rtc_register_alarm_callback(rtc_alarm_callback_t cb)
{
    s_alarm_cb = cb;
    LOG_D("알람 콜백 등록 완료");
}

/* ===== Wake-up 타이머 ===== */

rtc_status_t rtc_set_wakeup(uint16_t seconds)
{
    /* 입력 검증: 0이면 언더플로우 발생 (0 - 1 = 0xFFFF) */
    if (seconds == 0) {
        LOG_W("Wake-up 주기 0초 — 무시");
        return RTC_ERROR;
    }

    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

    if (HAL_RTCEx_SetWakeUpTimer_IT(
            &hrtc, (uint16_t)(seconds - 1),
            RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK) {
        LOG_E("Wake-up 타이머 설정 실패");
        return RTC_ERROR;
    }

    LOG_I("Wake-up 타이머: %d초 주기", seconds);
    return RTC_OK;
}

rtc_status_t rtc_stop_wakeup(void)
{
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    LOG_I("Wake-up 타이머 정지");
    return RTC_OK;
}

void rtc_register_wakeup_callback(rtc_wakeup_callback_t cb)
{
    s_wakeup_cb = cb;
    LOG_D("Wake-up 콜백 등록 완료");
}

/* ===== 백업 레지스터 ===== */

uint32_t rtc_backup_read(uint32_t reg)
{
    return HAL_RTCEx_BKUPRead(&hrtc, reg);
}

void rtc_backup_write(uint32_t reg, uint32_t value)
{
    HAL_RTCEx_BKUPWrite(&hrtc, reg, value);
}

/* ===== HAL 콜백 → Driver 콜백 전달 ===== */

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc_cb)
{
    if (s_alarm_cb != NULL) {
        s_alarm_cb();   /* alarm_service로 전달 */
    }
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc_cb)
{
    if (s_wakeup_cb != NULL) {
        s_wakeup_cb();
    }
}
