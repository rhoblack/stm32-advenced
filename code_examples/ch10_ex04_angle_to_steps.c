/* ============================================================
   Ch10_ex04: RTC 시각 → 시침 각도 → 스텝 변환 알고리즘
   ============================================================
   목적: 누적 성장! Ch09 RTC + Ch10 Motor 연결
   기능: 14:30 같은 시각을 시침 위치로 변환
   예상 동작: 시각 입력 → 정확한 스텝 계산
   ============================================================ */

#include "ch10_stepper_driver.h"
#include "log.h"
#include <stdint.h>

/**
 * @brief RTC 시각(시:분)을 시침 각도로 변환
 * @param hour: 시 (0~23, 자동으로 12시간 변환)
 * @param minute: 분 (0~59)
 * @return 각도 (0~360, 단위: 도)
 *
 * 계산 원리:
 * - 아날로그 시계는 12시간 주기 → hour % 12
 * - 시침은 1시간에 30도 (360 / 12)
 * - 시침은 분에도 반응함: 1분에 0.5도 (30 / 60)
 * - 예: 14:30 → 2시 30분 → (2*30) + (30*0.5) = 75°
 */
uint16_t time_to_angle(uint8_t hour, uint8_t minute)
{
    uint16_t angle;

    /* 12시간 형식으로 변환 */
    hour = hour % 12;

    /* 시침 각도 계산 */
    angle = (hour * 30) + (minute / 2);

    LOG_D("time_to_angle: %02d:%02d → %u degrees", hour, minute, angle);

    return angle;
}

/**
 * @brief 각도를 스텝 수로 변환
 * @param angle_deg: 각도 (0~360)
 * @return 스텝 수 (0~4096)
 *
 * 계산 원리:
 * - 360도 = 4096 스텝 (풀 스텝 64개 * 64)
 * - 1도 ≈ 11.378 스텝
 * - 정수 연산: steps = (angle * 1024) / 90
 *   (1024 = 4096/4, 90 = 360/4)
 */
uint16_t angle_to_steps(uint16_t angle_deg)
{
    /* 고정소수점 연산으로 나눗셈 회피 */
    uint16_t steps = (angle_deg * 1024) / 90;

    LOG_D("angle_to_steps: %u degrees → %u steps", angle_deg, steps);

    return steps;
}

/**
 * @brief 현재 시각으로부터 필요한 스텝 수 계산 (누적 성장!)
 * @param hour: 현재 시각(시)
 * @param minute: 현재 시각(분)
 * @param current_steps: 모터 현재 위치
 * @return 이동할 스텝 수 (음수면 반시계방향)
 *
 * 핵심: 최단 경로 선택 (시계방향 vs 반시계방향)
 */
int16_t calculate_steps_to_target(uint8_t hour, uint8_t minute,
                                  uint16_t current_steps)
{
    uint16_t target_angle = time_to_angle(hour, minute);
    uint16_t target_steps = angle_to_steps(target_angle);

    int16_t delta = (int16_t)target_steps - (int16_t)current_steps;

    /* 최단 경로 선택
     * 예: 현재 3900, 목표 100
     *     직진: +200 스텝
     *     역방향: -3800 스텝
     *     → 직진 선택
     */
    if (delta > 2048) {
        delta -= 4096;  /* 반시계방향이 더 짧음 */
    } else if (delta < -2048) {
        delta += 4096;  /* 시계방향이 더 짧음 */
    }

    LOG_D("calculate_steps: target=%u, current=%u, delta=%d",
          target_steps, current_steps, delta);

    return delta;
}

/* ===== 테스트 함수 ===== */

void test_time_to_angle_conversion(void)
{
    LOG_I("=== Ch10_ex04: Time to Angle Conversion ===");

    /* 테스트 케이스 */
    struct {
        uint8_t hour;
        uint8_t minute;
        uint16_t expected_angle;
    } test_cases[] = {
        {0, 0, 0},      /* 자정 → 0도 (12시 위치) */
        {6, 0, 180},    /* 6시 → 180도 */
        {3, 0, 90},     /* 3시 → 90도 */
        {14, 30, 75},   /* 14:30 (2:30) → 75도 */
        {15, 15, 97},   /* 15:15 (3:15) → 97도 */
        {18, 45, 232},  /* 18:45 (6:45) → 232도 */
    };

    for (int i = 0; i < 6; i++) {
        uint16_t angle = time_to_angle(test_cases[i].hour, test_cases[i].minute);
        LOG_I("Test %d: %02d:%02d → %u° (expected %u°) %s",
              i+1,
              test_cases[i].hour, test_cases[i].minute,
              angle,
              test_cases[i].expected_angle,
              angle == test_cases[i].expected_angle ? "PASS" : "FAIL");
    }

    /* 각도 → 스텝 테스트 */
    LOG_I("--- Angle to Steps Conversion ---");
    uint16_t steps_0 = angle_to_steps(0);
    uint16_t steps_90 = angle_to_steps(90);
    uint16_t steps_180 = angle_to_steps(180);
    uint16_t steps_360 = angle_to_steps(360);

    LOG_I("0° → %u steps (expected 0)", steps_0);
    LOG_I("90° → %u steps (expected 1024)", steps_90);
    LOG_I("180° → %u steps (expected 2048)", steps_180);
    LOG_I("360° → %u steps (expected 4096)", steps_360);

    LOG_I("Time conversion test complete");
}
