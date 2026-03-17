/* ch09_03_rtc_set_time.c — RTC 시간 설정
 *
 * 레이어: App (HAL 직접 호출)
 * 의존: HAL_RTC, log.h
 *
 * NUCLEO-F411RE에서 VBAT 미연결 시 매 리셋마다 시간 초기화 필요
 */

#include "main.h"
#include "log.h"

extern RTC_HandleTypeDef hrtc;

/* ===== 시간 설정 ===== */

void rtc_set_time_example(void)
{
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};

    LOG_I("RTC 시간 설정 시작");

    /* 시간 설정: 14시 30분 00초 */
    time.Hours   = 14;
    time.Minutes = 30;
    time.Seconds = 0;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK) {
        LOG_E("RTC 시간 설정 실패!");
        return;
    }
    LOG_D("시간 설정: %02d:%02d:%02d", time.Hours, time.Minutes, time.Seconds);

    /* 날짜 설정: 2026년 3월 17일 화요일 */
    date.Year    = 26;       /* 2000 + 26 = 2026 */
    date.Month   = RTC_MONTH_MARCH;
    date.Date    = 17;
    date.WeekDay = RTC_WEEKDAY_TUESDAY;

    if (HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK) {
        LOG_E("RTC 날짜 설정 실패!");
        return;
    }
    LOG_D("날짜 설정: 20%02d-%02d-%02d (요일:%d)",
          date.Year, date.Month, date.Date, date.WeekDay);

    LOG_I("RTC 시간/날짜 설정 완료");
}

/* ===== 백업 레지스터로 초기화 여부 확인 ===== */

/**
 * @brief RTC가 이미 설정되었는지 확인 (리셋 시 중복 초기화 방지)
 *
 * 백업 레지스터에 매직 넘버를 기록해두면,
 * VBAT가 유지되는 한 리셋 후에도 시간 재설정을 건너뛸 수 있습니다.
 */
#define RTC_MAGIC_NUMBER  0xA5A5

void rtc_init_with_backup_check(void)
{
    /* 백업 레지스터 0번에서 매직 넘버 확인 */
    uint32_t magic = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0);

    if (magic == RTC_MAGIC_NUMBER) {
        LOG_I("RTC 이미 설정됨 — 시간 유지 (VBAT 백업)");

        /* 현재 시간 출력 */
        RTC_TimeTypeDef time;
        RTC_DateTypeDef date;
        HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

        LOG_I("유지된 시각: %02d:%02d:%02d",
              time.Hours, time.Minutes, time.Seconds);
    } else {
        LOG_W("RTC 최초 설정 또는 VBAT 소실 — 시간 초기화");

        /* 시간/날짜 설정 */
        rtc_set_time_example();

        /* 매직 넘버 기록 (다음 리셋 시 재설정 건너뛰기) */
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, RTC_MAGIC_NUMBER);
        LOG_D("백업 레지스터에 매직 넘버 기록 완료");
    }
}
