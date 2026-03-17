/* ch09_05_wakeup_timer.c — Wake-up 타이머 설정
 *
 * 레이어: App (HAL 직접 호출)
 * 의존: HAL_RTCEx, log.h
 *
 * Wake-up Timer: 주기적 인터럽트 (주방 타이머)
 *   - TIM 인터럽트와 비슷하지만, 32kHz RTC 클럭으로 동작
 *   - Stop 모드에서도 MCU를 깨울 수 있음 (저전력 핵심!)
 *   - 주기: 약 122us ~ 36시간
 */

#include "main.h"
#include "log.h"

extern RTC_HandleTypeDef hrtc;

/* ===== Wake-up 이벤트 플래그 ===== */
static volatile uint32_t wakeup_count = 0;

/* ===== Wake-up 타이머 설정: 5초 주기 ===== */

void rtc_wakeup_start(uint16_t seconds)
{
    /*
     * Wake-up 클럭 소스 선택:
     *   RTC_WAKEUPCLOCK_RTCCLK_DIV16  — 가장 짧은 주기 (~122us)
     *   RTC_WAKEUPCLOCK_RTCCLK_DIV8
     *   RTC_WAKEUPCLOCK_RTCCLK_DIV4
     *   RTC_WAKEUPCLOCK_RTCCLK_DIV2
     *   RTC_WAKEUPCLOCK_CK_SPRE_16BITS — 1Hz 기반, 1~65536초
     *   RTC_WAKEUPCLOCK_CK_SPRE_17BITS — 1Hz 기반, 0x10000~0x1FFFF초
     *
     * CK_SPRE(1Hz) 사용 시: 카운터 값 = (주기 - 1)
     * 예: 5초 → 카운터 = 4
     */

    /* 입력 검증: 0이면 언더플로우 발생 (0 - 1 = 0xFFFF) */
    if (seconds == 0) {
        LOG_W("Wake-up 주기 0초 — 무시");
        return;
    }

    /* 기존 Wake-up 타이머 해제 */
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

    /* Wake-up 타이머 설정: 1Hz 기반, seconds초 주기 */
    if (HAL_RTCEx_SetWakeUpTimer_IT(
            &hrtc,
            (uint16_t)(seconds - 1),              /* 카운트다운 값 (안전) */
            RTC_WAKEUPCLOCK_CK_SPRE_16BITS        /* 1초 단위 */
        ) != HAL_OK) {
        LOG_E("Wake-up 타이머 설정 실패!");
        return;
    }

    wakeup_count = 0;
    LOG_I("Wake-up 타이머 시작: %d초 주기", seconds);
}

/* ===== Wake-up 타이머 정지 ===== */

void rtc_wakeup_stop(void)
{
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    LOG_I("Wake-up 타이머 정지 (총 %lu회 트리거)", wakeup_count);
}

/* ===== Wake-up 콜백 (ISR 컨텍스트) ===== */

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc_cb)
{
    wakeup_count++;
    /* ISR에서는 카운트만! 실제 처리는 main loop에서 */
}

/* ===== main loop에서 Wake-up 이벤트 처리 ===== */

void rtc_wakeup_process(void)
{
    static uint32_t last_count = 0;

    if (wakeup_count != last_count) {
        last_count = wakeup_count;

        LOG_D("Wake-up #%lu", wakeup_count);

        /* 예: 주기적 센서 읽기 (Ch11에서 활용) */
        /* sensor_service_read(); */

        /* 예: 주기적 시간 표시 갱신 */
        RTC_TimeTypeDef time;
        RTC_DateTypeDef date;
        HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

        LOG_I("[Wake-up] %02d:%02d:%02d",
              time.Hours, time.Minutes, time.Seconds);
    }
}

/* ===== 백업 레지스터 활용 예제 ===== */

void rtc_backup_register_example(void)
{
    LOG_I("===== 백업 레지스터 예제 =====");

    /* 리셋 횟수 카운터 (BKP_DR1 사용) */
    uint32_t reset_count = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
    reset_count++;
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, reset_count);
    LOG_I("시스템 리셋 횟수: %lu (VBAT 유지 기간)", reset_count);

    /* 마지막 에러 코드 저장 (BKP_DR2 사용) */
    uint32_t last_error = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);
    if (last_error != 0) {
        LOG_W("이전 리셋 시 에러 코드: 0x%08lX", last_error);
        /* 에러 코드 클리어 */
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, 0);
    }

    /*
     * 백업 레지스터 용도 정리:
     *   BKP_DR0: RTC 초기화 매직 넘버 (0xA5A5)
     *   BKP_DR1: 리셋 횟수 카운터
     *   BKP_DR2: 마지막 에러 코드
     *   BKP_DR3: 마지막 알람 시각 (packed)
     *   BKP_DR4~19: 사용자 정의
     *
     * 주의: VBAT 미연결 시 전원 차단 → 모두 0으로 초기화
     */

    LOG_D("백업 레지스터: 20개 × 32비트 = 80바이트");
    LOG_D("VBAT 유지 시 전원 차단/리셋 후에도 값 보존");
}
