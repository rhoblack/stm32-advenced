/* ch09_02_rtc_get_time.c — HAL_RTC API로 시간 읽기
 *
 * 레이어: App (HAL 직접 호출 — 이후 rtc_driver로 래핑)
 * 의존: HAL_RTC, log.h
 *
 * 핵심 주의사항:
 *   HAL_RTC_GetTime() 호출 후 반드시 HAL_RTC_GetDate()를 호출해야 함!
 *   GetDate를 호출하지 않으면 Shadow Register가 잠금 해제되지 않아
 *   다음 GetTime에서 값이 갱신되지 않습니다.
 */

#include "main.h"
#include "log.h"

extern RTC_HandleTypeDef hrtc;   /* CubeMX 자동 생성 */

/* ===== RTC 시간 읽기 (기본 패턴) ===== */

void rtc_read_time_example(void)
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    /* ⚠️ 반드시 GetTime → GetDate 순서로 호출!
     *    GetDate를 호출해야 Shadow Register 잠금이 해제됩니다.
     *    날짜가 필요 없더라도 GetDate를 호출해야 합니다! */
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

    LOG_I("현재 시각: %02d:%02d:%02d",
          time.Hours, time.Minutes, time.Seconds);

    LOG_D("현재 날짜: 20%02d-%02d-%02d (요일:%d)",
          date.Year, date.Month, date.Date, date.WeekDay);
}

/* ===== BCD 형식으로 읽기 (디버거 확인용) ===== */

void rtc_read_time_bcd(void)
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    /* RTC_FORMAT_BCD: 레지스터 원본 BCD 값 그대로 반환 */
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BCD);

    LOG_D("BCD 시간: 0x%02X:0x%02X:0x%02X",
          time.Hours, time.Minutes, time.Seconds);
    LOG_D("BCD 날짜: 0x%02X-0x%02X-0x%02X",
          date.Year, date.Month, date.Date);
}

/* ===== 주기적 시간 출력 (1초마다) ===== */

void rtc_periodic_display(void)
{
    static uint8_t last_sec = 0xFF;

    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);   /* 필수! */

    /* 초가 바뀌었을 때만 출력 (매 루프 출력 방지) */
    if (time.Seconds != last_sec) {
        last_sec = time.Seconds;

        LOG_I("[RTC] %02d:%02d:%02d  20%02d-%02d-%02d",
              time.Hours, time.Minutes, time.Seconds,
              date.Year, date.Month, date.Date);
    }
}

/* ===== 흔한 실수: GetDate 누락 ===== */

/*
 * ❌ 잘못된 코드 (절대 이렇게 하지 마세요!)
 *
 * void wrong_read(void)
 * {
 *     RTC_TimeTypeDef time;
 *     HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
 *     // GetDate 호출 누락!
 *     // → Shadow Register 잠금 해제 안 됨
 *     // → 다음 GetTime에서 값이 갱신되지 않음
 *     // → 시간이 멈춘 것처럼 보임!
 * }
 *
 * 이것은 RM0383 Reference Manual에 명시된 동작입니다.
 * "시간이 멈추면 GetDate를 빠뜨렸는지 확인하세요!"
 */
