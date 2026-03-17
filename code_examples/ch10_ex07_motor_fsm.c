/* ============================================================
   Ch10_ex07: Motor FSM (Finite State Machine)
   ============================================================
   목적: Ch08 Stopwatch FSM과 유사한 패턴으로 모터 제어
   상태: IDLE → MOVING → HOMING → ERROR
   특징: 안정성, 상태 추적, 에러 처리
   ============================================================ */

#include "ch10_stepper_driver.h"
#include "log.h"
#include <stdint.h>

/* Motor FSM 상태 정의 */
typedef enum {
    MOTOR_STATE_IDLE = 0,      /* 대기 (전원 OFF) */
    MOTOR_STATE_MOVING,        /* 이동 중 */
    MOTOR_STATE_HOMING,        /* 홈 위치로 복귀 중 */
    MOTOR_STATE_ERROR          /* 에러 발생 */
} motor_state_t;

/* Motor FSM 컨텍스트 */
typedef struct {
    motor_state_t state;
    uint16_t target_position;
    int16_t remaining_steps;
    uint16_t current_speed_rpm;
} motor_fsm_ctx_t;

/* 전역 상태 머신 인스턴스 */
static motor_fsm_ctx_t motor_fsm = {
    .state = MOTOR_STATE_IDLE,
    .target_position = 0,
    .remaining_steps = 0,
    .current_speed_rpm = 0
};

/**
 * @brief Motor FSM 초기화
 */
void motor_fsm_init(void)
{
    stepper_init();
    motor_fsm.state = MOTOR_STATE_IDLE;
    motor_fsm.target_position = 0;
    motor_fsm.remaining_steps = 0;
    LOG_I("Motor FSM initialized: state=IDLE");
}

/**
 * @brief 모터를 지정된 위치로 이동 (FSM 기반)
 * @param target_pos: 목표 위치 (스텝)
 * @param speed_rpm: 이동 속도 (RPM)
 */
void motor_fsm_move_to(uint16_t target_pos, uint16_t speed_rpm)
{
    if (motor_fsm.state != MOTOR_STATE_IDLE) {
        LOG_W("motor_fsm_move_to: not in IDLE state (current=%u)", motor_fsm.state);
        return;
    }

    /* 이동할 스텝 수 계산 */
    uint16_t current_pos = stepper_get_position();
    int16_t delta = (int16_t)target_pos - (int16_t)current_pos;

    /* 최단 경로 선택 */
    if (delta > 2048) {
        delta -= 4096;
    } else if (delta < -2048) {
        delta += 4096;
    }

    if (delta == 0) {
        LOG_D("motor_fsm_move_to: already at target position %u", target_pos);
        return;
    }

    motor_fsm.state = MOTOR_STATE_MOVING;
    motor_fsm.target_position = target_pos;
    motor_fsm.remaining_steps = delta;
    motor_fsm.current_speed_rpm = speed_rpm;

    LOG_I("Motor FSM: IDLE → MOVING (target=%u, delta=%d, speed=%u RPM)",
          target_pos, delta, speed_rpm);

    /* 실제 이동 */
    stepper_move_steps(delta, speed_rpm);

    /* 이동 완료 후 상태 변경 */
    motor_fsm.state = MOTOR_STATE_IDLE;
    motor_fsm.remaining_steps = 0;
    LOG_I("Motor FSM: MOVING → IDLE (completed)");
}

/**
 * @brief 모터를 홈 위치(0°)로 복귀 (FSM 기반)
 */
void motor_fsm_home(uint16_t speed_rpm)
{
    if (motor_fsm.state != MOTOR_STATE_IDLE) {
        LOG_W("motor_fsm_home: not in IDLE state");
        return;
    }

    motor_fsm.state = MOTOR_STATE_HOMING;
    LOG_I("Motor FSM: IDLE → HOMING");

    stepper_home();

    motor_fsm.state = MOTOR_STATE_IDLE;
    motor_fsm.target_position = 0;
    LOG_I("Motor FSM: HOMING → IDLE (at home)");
}

/**
 * @brief Motor FSM 상태 반환
 */
motor_state_t motor_fsm_get_state(void)
{
    return motor_fsm.state;
}

/**
 * @brief Motor FSM 상태 문자열 반환 (디버깅용)
 */
const char* motor_fsm_get_state_str(void)
{
    switch (motor_fsm.state) {
        case MOTOR_STATE_IDLE:    return "IDLE";
        case MOTOR_STATE_MOVING:  return "MOVING";
        case MOTOR_STATE_HOMING:  return "HOMING";
        case MOTOR_STATE_ERROR:   return "ERROR";
        default:                  return "UNKNOWN";
    }
}

/**
 * @brief Motor FSM 상태 정보 출력
 */
void motor_fsm_print_status(void)
{
    LOG_I("=== Motor FSM Status ===");
    LOG_I("State: %s", motor_fsm_get_state_str());
    LOG_I("Position: %u / Target: %u", stepper_get_position(), motor_fsm.target_position);
    LOG_I("Remaining: %d steps @ %u RPM", motor_fsm.remaining_steps, motor_fsm.current_speed_rpm);
}

/* ===== 테스트 함수 ===== */

/**
 * @brief Motor FSM 시나리오 테스트
 */
void test_motor_fsm_scenarios(void)
{
    LOG_I("=== Ch10_ex07: Motor FSM Test ===");

    /* 시나리오 1: 초기화 */
    motor_fsm_init();
    motor_fsm_print_status();

    HAL_Delay(500);

    /* 시나리오 2: 1024 스텝으로 이동 (90도) */
    LOG_I("Scenario 1: Move to 90 degrees (1024 steps)");
    motor_fsm_move_to(1024, 50);
    motor_fsm_print_status();

    HAL_Delay(500);

    /* 시나리오 3: 2048 스텝으로 이동 (180도) */
    LOG_I("Scenario 2: Move to 180 degrees (2048 steps)");
    motor_fsm_move_to(2048, 50);
    motor_fsm_print_status();

    HAL_Delay(500);

    /* 시나리오 4: 홈으로 복귀 */
    LOG_I("Scenario 3: Return to home (0 degrees)");
    motor_fsm_home(50);
    motor_fsm_print_status();

    HAL_Delay(500);

    /* 시나리오 5: 복잡한 이동 시뮬레이션 */
    LOG_I("Scenario 4: Complex movement (multiple moves)");
    motor_fsm_move_to(512, 40);   /* 45도 */
    HAL_Delay(1000);
    motor_fsm_move_to(1536, 40);  /* 135도 */
    HAL_Delay(1000);
    motor_fsm_move_to(3072, 40);  /* 270도 */
    HAL_Delay(1000);
    motor_fsm_home(40);            /* 홈 복귀 */

    LOG_I("Motor FSM test complete");
}
