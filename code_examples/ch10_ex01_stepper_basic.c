/* ============================================================
   Ch10_ex01: 스텝 모터 기초 — 단순 GPIO 풀 스텝 여자
   ============================================================
   목적: 가장 간단한 형태로 스텝 모터의 기본 동작 이해
   하드웨어: ULN2003 + 28BYJ-48, GPIOB 0/1/4/5
   예상 동작: 시계 방향으로 10바퀴 회전
   ============================================================ */

#include "stm32f4xx_hal.h"
#include "log.h"

/* GPIO 포트 정의 */
#define MOTOR_GPIO_PORT GPIOB
#define MOTOR_COIL_A_PIN GPIO_PIN_0
#define MOTOR_COIL_B_PIN GPIO_PIN_1
#define MOTOR_COIL_C_PIN GPIO_PIN_4
#define MOTOR_COIL_D_PIN GPIO_PIN_5

/**
 * @brief 4개 코일에 신호 제공
 * 풀 스텝 시퀀스: A → A+B → B → B+C → C → C+D → D → D+A (반복)
 */
static void motor_set_step(uint8_t step_idx)
{
    /* ULN2003은 active-low이므로 GPIO_PIN_RESET으로 코일을 활성화 */
    const uint8_t sequence[4][4] = {
        {0, 0, 1, 1},  /* A+D 활성 */
        {0, 0, 0, 1},  /* D만 활성 */
        {1, 0, 0, 1},  /* C+D 활성 */
        {1, 0, 0, 0},  /* C만 활성 */
    };

    uint8_t a_val = sequence[step_idx][0] ? GPIO_PIN_SET : GPIO_PIN_RESET;
    uint8_t b_val = sequence[step_idx][1] ? GPIO_PIN_SET : GPIO_PIN_RESET;
    uint8_t c_val = sequence[step_idx][2] ? GPIO_PIN_SET : GPIO_PIN_RESET;
    uint8_t d_val = sequence[step_idx][3] ? GPIO_PIN_SET : GPIO_PIN_RESET;

    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_A_PIN, a_val);
    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_B_PIN, b_val);
    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_C_PIN, c_val);
    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_D_PIN, d_val);
}

/**
 * @brief 메인: 스텝 모터 기초 테스트
 */
void stepper_basic_demo(void)
{
    LOG_I("=== Ch10_ex01: Stepper Motor Basic ===");

    int total_steps = 64 * 10;  /* 360도 = 64 스텝, 10바퀴 */

    for (int i = 0; i < total_steps; i++) {
        uint8_t step_idx = i % 4;
        motor_set_step(step_idx);
        HAL_Delay(50);  /* 50ms = 약 20 RPM */

        if (i % 64 == 0) {
            LOG_D("Completed %d rotations (%d steps)", i / 64, i);
        }
    }

    /* 최종 상태: 모든 코일 OFF */
    motor_set_step(0);
    LOG_I("Stepper demo complete");
}
