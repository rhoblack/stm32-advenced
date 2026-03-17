/* ============================================================
   Ch10_ex06: 가속/감속 프로파일 (Acceleration Profile)
   ============================================================
   목적: 모터 안정성 향상, 소음/진동 감소, 수명 연장
   기능: 선형 가속 → 등속 → 감속 (부드러운 제어)
   효과: 급격한 전류 변화 방지 → 드라이버 보호
   ============================================================ */

#include "ch10_stepper_driver.h"
#include "log.h"
#include <stdint.h>

/**
 * @brief 가속 프로파일 구조체
 */
typedef struct {
    uint16_t target_rpm;    /* 목표 속도 */
    uint16_t accel_steps;   /* 가속 구간 스텝 수 */
    uint16_t decel_steps;   /* 감속 구간 스텝 수 */
} accel_profile_t;

/**
 * @brief 선형 가속 프로파일이 적용된 모터 회전
 * @param steps: 이동 스텝 수
 * @param target_rpm: 목표 속도 (RPM)
 * @param accel_steps: 가속 구간 스텝 수 (0이면 가속 없음)
 *
 * 3개 구간:
 * 1. 가속 구간: max_delay → target_delay로 선형 감소
 * 2. 등속 구간: target_delay 유지
 * 3. 감속 구간: target_delay → max_delay로 선형 증가
 */
void stepper_move_with_acceleration(int16_t steps, uint16_t target_rpm,
                                    uint16_t accel_steps)
{
    if (steps == 0) {
        LOG_W("stepper_move_with_acceleration: steps=0");
        return;
    }

    /* 목표 딜레이 계산 */
    uint16_t target_delay = 60000 / (target_rpm * 64);
    uint16_t min_delay = 5;    /* 최소 딜레이 (안정성 한계) */
    uint16_t max_delay = 500;  /* 최대 딜레이 (느린 시작) */

    if (target_delay < min_delay) target_delay = min_delay;
    if (target_delay > max_delay) target_delay = max_delay;

    uint16_t current_delay = max_delay;
    uint16_t abs_steps = steps > 0 ? steps : -steps;

    LOG_I("Accel move: %d steps, target=%d RPM, accel=%u steps",
          steps, target_rpm, accel_steps);
    LOG_D("Delays: target=%u ms, max=%u ms", target_delay, max_delay);

    /* 가속 구간 */
    uint16_t i = 0;
    for (; i < accel_steps && i < abs_steps; i++) {
        /* 선형 보간: max_delay → target_delay */
        current_delay = max_delay -
                       ((max_delay - target_delay) * i) / accel_steps;
        if (current_delay < target_delay) current_delay = target_delay;

        if (steps > 0) {
            stepper_step_forward();
        } else {
            stepper_step_backward();
        }
        HAL_Delay(current_delay);
    }

    /* 등속 구간 */
    uint16_t remaining = abs_steps - i;
    uint16_t decel_steps = accel_steps;  /* 대칭 가정 */
    uint16_t constant_steps = remaining > decel_steps ?
                             remaining - decel_steps : 0;

    for (uint16_t j = 0; j < constant_steps; j++) {
        if (steps > 0) {
            stepper_step_forward();
        } else {
            stepper_step_backward();
        }
        HAL_Delay(target_delay);
    }

    /* 감속 구간 */
    for (uint16_t j = 0; j < decel_steps && (i + constant_steps + j) < abs_steps; j++) {
        /* 선형 보간: target_delay → max_delay */
        current_delay = target_delay +
                       ((max_delay - target_delay) * j) / decel_steps;
        if (current_delay > max_delay) current_delay = max_delay;

        if (steps > 0) {
            stepper_step_forward();
        } else {
            stepper_step_backward();
        }
        HAL_Delay(current_delay);
    }

    LOG_I("Acceleration complete, position=%u", stepper_get_position());
}

/**
 * @brief S-커브 가속 (더 매끄러운 가속, 고급)
 * @note 현재는 선형 가속만 사용. 실무에서는 sine/exponential 곡선 권장
 */
void stepper_move_with_scurve(int16_t steps, uint16_t target_rpm,
                              uint16_t accel_steps)
{
    /* TODO: 실제 구현에서는 sine 곡선 적용
     * delay(t) = target_delay + (max-target) * sin(π*t/(2*T))
     * 이는 매우 부드러운 가속을 제공합니다.
     */
    stepper_move_with_acceleration(steps, target_rpm, accel_steps);
}

/* ===== 프로파일 비교 데모 ===== */

/**
 * @brief 가속 프로파일 적용 전/후 비교
 */
void test_acceleration_profiles(void)
{
    LOG_I("=== Ch10_ex06: Acceleration Profile Test ===");

    /* 테스트 1: 가속 없음 (충격) */
    LOG_I("--- Test 1: No acceleration (immediate start) ---");
    stepper_init();
    stepper_move_steps(64, 50);  /* 1회전, 50 RPM, 즉시 시작 */
    LOG_I("Completed without acceleration");

    HAL_Delay(1000);

    /* 테스트 2: 가속 포함 (부드러움) */
    LOG_I("--- Test 2: With acceleration (smooth start) ---");
    stepper_init();
    stepper_move_with_acceleration(64, 50, 16);  /* 1회전, 50 RPM, 16스텝 가속 */
    LOG_I("Completed with acceleration");

    /* 테스트 3: 장거리 이동 */
    LOG_I("--- Test 3: Long distance (10 rotations) ---");
    stepper_init();
    stepper_move_with_acceleration(640, 50, 32);  /* 10회전 */
    LOG_I("Long move complete");
}
