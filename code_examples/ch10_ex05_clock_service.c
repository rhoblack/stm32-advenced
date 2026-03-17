/* ============================================================
   Ch10_ex05: Clock Service (Service 레이어 통합)
   ============================================================
   목적: RTC(Ch09) + Motor Driver(Ch10) 조율
         → 완전한 스마트 시계 기능
   특징: FSM 기반, 매초 자동 시침 업데이트, 에러 처리
   ============================================================ */

#include "ch10_stepper_driver.h"
#include "log.h"
#include <stdint.h>

/* 상태 정의 */
typedef enum {
    CLOCK_STATE_IDLE = 0,      /* 정상 상태, 대기 중 */
    CLOCK_STATE_SYNCING,       /* 모터가 이동 중 */
    CLOCK_STATE_ERROR          /* 에러 발생 */
} clock_state_t;

/* Clock Service 상태 변수 */
static clock_state_t clock_state = CLOCK_STATE_IDLE;
static uint8_t last_hour = 0;
static uint8_t last_minute = 0;
static uint16_t motor_target_steps = 0;

/* 외부에서 사용 가능한 상태 변수 (테스트용) */
uint8_t g_last_hour = 0;
uint8_t g_last_minute = 0;

/**
 * @brief 시각(시:분)을 시침 각도로 변환
 * @note ch10_ex04와 동일한 함수 (재사용)
 */
static uint16_t time_to_angle(uint8_t hour, uint8_t minute)
{
    hour = hour % 12;
    uint16_t angle = (hour * 30) + (minute / 2);
    return angle;
}

/**
 * @brief 각도를 스텝으로 변환
 * @note ch10_ex04와 동일한 함수 (재사용)
 */
static uint16_t angle_to_steps(uint16_t angle_deg)
{
    uint16_t steps = (angle_deg * 1024) / 90;
    return steps;
}

/**
 * @brief 현재 위치에서 목표까지 이동할 스텝 수 계산
 */
static int16_t calculate_steps_to_target(uint8_t hour, uint8_t minute,
                                         uint16_t current_steps)
{
    uint16_t target_angle = time_to_angle(hour, minute);
    uint16_t target_steps = angle_to_steps(target_angle);

    int16_t delta = (int16_t)target_steps - (int16_t)current_steps;

    /* 최단 경로 선택 */
    if (delta > 2048) {
        delta -= 4096;
    } else if (delta < -2048) {
        delta += 4096;
    }

    LOG_D("calculate_steps: target=%u, current=%u, delta=%d",
          target_steps, current_steps, delta);

    return delta;
}

/**
 * @brief Clock Service 초기화
 * @note 프로그램 시작 시 한 번만 호출
 */
void clock_service_init(void)
{
    stepper_init();  /* 모터 초기화 */

    /* RTC에서 현재 시각 읽기 (시뮬레이션) */
    /* 실제로는: rtc_get_time(&sTime) 호출 */
    last_hour = 14;
    last_minute = 30;

    motor_target_steps = angle_to_steps(time_to_angle(last_hour, last_minute));
    stepper_move_steps(motor_target_steps, 30);

    clock_state = CLOCK_STATE_IDLE;
    g_last_hour = last_hour;
    g_last_minute = last_minute;

    LOG_I("Clock service initialized, time=%02d:%02d, position=%u",
          last_hour, last_minute, stepper_get_position());
}

/**
 * @brief RTC 1초 인터럽트에서 호출 (매초 시침 업데이트)
 * @note HAL_RTC_AlarmAEventCallback()에서 이 함수를 호출해야 함
 *
 * 워크플로우:
 * 1. 현재 시각 읽기
 * 2. 분이나 시간이 변경되었는가?
 * 3. 변경되었으면 목표 위치 계산 및 모터 이동
 */
void clock_service_on_rtc_tick(void)
{
    if (clock_state == CLOCK_STATE_ERROR) {
        return;
    }

    /* 시뮬레이션: 매 호출마다 분을 1씩 증가
     * 실제로는: rtc_get_time()에서 현재 시각 읽음
     */
    static uint8_t sim_tick = 0;
    uint8_t current_hour = last_hour;
    uint8_t current_minute = last_minute;

    sim_tick++;
    if (sim_tick >= 60) {
        current_minute++;
        sim_tick = 0;
        if (current_minute >= 60) {
            current_hour++;
            current_minute = 0;
            if (current_hour >= 24) {
                current_hour = 0;
            }
        }
    }

    /* 시간이나 분이 변경되었는가? */
    if (current_hour == last_hour && current_minute == last_minute) {
        return;  /* 변화 없음 */
    }

    last_hour = current_hour;
    last_minute = current_minute;
    g_last_hour = current_hour;
    g_last_minute = current_minute;

    /* 목표 위치 계산 */
    int16_t delta_steps = calculate_steps_to_target(
        last_hour, last_minute, stepper_get_position()
    );

    /* 모터 이동 */
    if (delta_steps != 0) {
        clock_state = CLOCK_STATE_SYNCING;
        stepper_move_steps(delta_steps, 50);  /* 50 RPM */
        clock_state = CLOCK_STATE_IDLE;
        LOG_I("Clock updated: %02d:%02d (moved %d steps)",
              last_hour, last_minute, delta_steps);
    }
}

/**
 * @brief 현재 시침 위치(스텝) 반환
 */
uint16_t clock_service_get_hour_position(void)
{
    return stepper_get_position();
}

/**
 * @brief Clock Service 상태 반환
 */
clock_state_t clock_service_get_state(void)
{
    return clock_state;
}

/**
 * @brief Clock Service 상태 문자열 반환 (디버깅용)
 */
const char* clock_service_get_state_str(void)
{
    switch (clock_state) {
        case CLOCK_STATE_IDLE:    return "IDLE";
        case CLOCK_STATE_SYNCING: return "SYNCING";
        case CLOCK_STATE_ERROR:   return "ERROR";
        default:                  return "UNKNOWN";
    }
}

/* ===== 테스트 함수 ===== */

/**
 * @brief Clock Service 통합 테스트
 */
void test_clock_service(void)
{
    LOG_I("=== Ch10_ex05: Clock Service Test ===");

    /* 초기화 */
    clock_service_init();
    LOG_I("Clock service initialized at %02d:%02d", g_last_hour, g_last_minute);

    /* 매초 시뮬레이션 (60초 = 60분 가속) */
    LOG_I("Simulating 60 seconds of operation...");
    for (int i = 0; i < 60; i++) {
        clock_service_on_rtc_tick();

        if (i % 10 == 0) {
            LOG_D("Time: %02d:%02d, State: %s, Motor: %u",
                  g_last_hour, g_last_minute,
                  clock_service_get_state_str(),
                  clock_service_get_hour_position());
        }
        HAL_Delay(100);  /* 100ms sleep (실제로는 1초) */
    }

    LOG_I("Clock service test complete");
    LOG_I("Final time: %02d:%02d, Motor position: %u steps",
          g_last_hour, g_last_minute, clock_service_get_hour_position());
}
