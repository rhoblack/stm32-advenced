/* ============================================================
   ch10_stepper_driver.c: 스텝 모터 Driver 레이어
   ============================================================
   목적: HAL GPIO를 추상화하여 고수준 Motor Control API 제공
   특징: 위치 추적, 풀 스텝 제어, 비블로킹 인터페이스 지원
   ============================================================ */

#include "ch10_stepper_driver.h"
#include "stm32f4xx_hal.h"
#include "log.h"

/* GPIO 포트 정의 (CubeMX에서 할당) */
#define MOTOR_GPIO_PORT GPIOB
#define MOTOR_COIL_A_PIN GPIO_PIN_0
#define MOTOR_COIL_B_PIN GPIO_PIN_1
#define MOTOR_COIL_C_PIN GPIO_PIN_4
#define MOTOR_COIL_D_PIN GPIO_PIN_5

/* 풀 스텝 여자 시퀀스 (4 스텝) */
static const uint8_t full_step_sequence[4][4] = {
    {1, 0, 0, 0},  /* A만 활성 */
    {1, 1, 0, 0},  /* A+B 활성 */
    {0, 1, 0, 0},  /* B만 활성 */
    {0, 1, 1, 0},  /* B+C 활성 */
};

/* 전역 상태 변수 */
static volatile int16_t motor_position = 0;    /* 현재 위치 (스텝) */
static volatile uint8_t motor_step_index = 0;  /* 시퀀스 인덱스 (0~3) */

/**
 * @brief 지정 스텝으로 코일 여자
 */
static void stepper_set_coils(uint8_t coil_a, uint8_t coil_b,
                              uint8_t coil_c, uint8_t coil_d)
{
    /* ULN2003은 active-low이므로 반전 */
    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_A_PIN,
                      coil_a ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_B_PIN,
                      coil_b ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_C_PIN,
                      coil_c ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_D_PIN,
                      coil_d ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

/**
 * @brief 스텝 모터 초기화
 */
void stepper_init(void)
{
    motor_position = 0;
    motor_step_index = 0;
    stepper_set_coils(0, 0, 0, 0);
    LOG_I("Stepper driver initialized");
}

/**
 * @brief 한 스텝 진행 (시계방향)
 */
static void stepper_step_forward(void)
{
    motor_step_index = (motor_step_index + 1) % 4;
    stepper_set_coils(
        full_step_sequence[motor_step_index][0],
        full_step_sequence[motor_step_index][1],
        full_step_sequence[motor_step_index][2],
        full_step_sequence[motor_step_index][3]
    );
    motor_position++;
    if (motor_position >= 4096) motor_position = 0;  /* 360도 순환 */
}

/**
 * @brief 한 스텝 퇴진 (반시계방향)
 */
static void stepper_step_backward(void)
{
    motor_step_index = (motor_step_index + 3) % 4;  /* -1과 동일 */
    stepper_set_coils(
        full_step_sequence[motor_step_index][0],
        full_step_sequence[motor_step_index][1],
        full_step_sequence[motor_step_index][2],
        full_step_sequence[motor_step_index][3]
    );
    motor_position--;
    if (motor_position < 0) motor_position = 4095;
}

/**
 * @brief 지정된 스텝 수만큼 회전
 * @param steps: 이동 스텝 수 (양수=시계방향, 음수=반시계방향)
 * @param speed_rpm: 회전 속도 (RPM, 권장값 10~100)
 */
void stepper_move_steps(int16_t steps, uint16_t speed_rpm)
{
    /* RPM → ms 딜레이 변환
     * 예: 30 RPM = 30 회전/분 = 30*64 스텝/분 (64 스텝 = 1 회전)
     *     = 1920 스텝/분 = 32 스텝/초 = 31.25 ms/스텝
     * 공식: delay_ms = 60000 ms / (RPM * 64 스텝/회전)
     */
    uint16_t delay_ms = 60000 / (speed_rpm * 64);

    if (delay_ms < 5) delay_ms = 5;      /* 최소 5ms */
    if (delay_ms > 500) delay_ms = 500;  /* 최대 500ms */

    LOG_D("Moving %d steps at %d RPM (delay=%u ms)", steps, speed_rpm, delay_ms);

    if (steps > 0) {
        for (int16_t i = 0; i < steps; i++) {
            stepper_step_forward();
            HAL_Delay(delay_ms);
        }
    } else if (steps < 0) {
        for (int16_t i = steps; i < 0; i++) {
            stepper_step_backward();
            HAL_Delay(delay_ms);
        }
    }
}

/**
 * @brief 홈 위치(0°)로 복귀
 */
void stepper_home(void)
{
    LOG_I("Homing motor to 0 degrees");

    if (motor_position > 0) {
        stepper_move_steps(-motor_position, 30);
    }

    motor_position = 0;
    motor_step_index = 0;
    stepper_set_coils(0, 0, 0, 0);
}

/**
 * @brief 현재 모터 위치 반환
 * @return 위치 (0~4095, 0=0도, 4096=360도)
 */
uint16_t stepper_get_position(void)
{
    return motor_position;
}
