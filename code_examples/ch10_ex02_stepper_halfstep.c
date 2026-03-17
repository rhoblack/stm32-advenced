/* ============================================================
   Ch10_ex02: 스텝 모터 하프스텝 — 정밀도 향상
   ============================================================
   목적: 하프 스텝으로 360도 = 128 스텝 정밀도 달성
   하드웨어: ULN2003 + 28BYJ-48
   예상 동작: 하프 스텝으로 5바퀴 회전 (매우 부드러운 움직임)
   ============================================================ */

#include "stm32f4xx_hal.h"
#include "log.h"

#define MOTOR_GPIO_PORT GPIOB
#define MOTOR_COIL_A_PIN GPIO_PIN_0
#define MOTOR_COIL_B_PIN GPIO_PIN_1
#define MOTOR_COIL_C_PIN GPIO_PIN_4
#define MOTOR_COIL_D_PIN GPIO_PIN_5

/**
 * @brief 하프 스텝 시퀀스 설정
 * 하프 스텝 = 1개 코일 또는 2개 코일 (교대)
 * 8개 스텝으로 90도만 회전
 */
static void motor_set_halfstep(uint8_t step_idx)
{
    /* 하프 스텝 시퀀스: 8개 스텝 */
    const uint8_t halfstep_sequence[8][4] = {
        {1, 0, 0, 0},  /* A만 */
        {1, 1, 0, 0},  /* A+B */
        {0, 1, 0, 0},  /* B만 */
        {0, 1, 1, 0},  /* B+C */
        {0, 0, 1, 0},  /* C만 */
        {0, 0, 1, 1},  /* C+D */
        {0, 0, 0, 1},  /* D만 */
        {1, 0, 0, 1},  /* D+A */
    };

    uint8_t a_val = halfstep_sequence[step_idx][0] ? GPIO_PIN_RESET : GPIO_PIN_SET;
    uint8_t b_val = halfstep_sequence[step_idx][1] ? GPIO_PIN_RESET : GPIO_PIN_SET;
    uint8_t c_val = halfstep_sequence[step_idx][2] ? GPIO_PIN_RESET : GPIO_PIN_SET;
    uint8_t d_val = halfstep_sequence[step_idx][3] ? GPIO_PIN_RESET : GPIO_PIN_SET;

    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_A_PIN, a_val);
    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_B_PIN, b_val);
    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_C_PIN, c_val);
    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_D_PIN, d_val);
}

/**
 * @brief 메인: 하프 스텝 테스트
 */
void stepper_halfstep_demo(void)
{
    LOG_I("=== Ch10_ex02: Stepper Motor Half-Step ===");

    int total_steps = 128 * 5;  /* 하프 스텝 128 = 360도, 5바퀴 */

    for (int i = 0; i < total_steps; i++) {
        uint8_t step_idx = i % 8;
        motor_set_halfstep(step_idx);
        HAL_Delay(30);  /* 30ms = 약 40 RPM (풀 스텝 대비 느림) */

        if (i % 128 == 0) {
            LOG_D("Completed %d rotations (%d halfsteps)", i / 128, i);
        }
    }

    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_A_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_B_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_C_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_GPIO_PORT, MOTOR_COIL_D_PIN, GPIO_PIN_SET);

    LOG_I("Half-step demo complete (higher precision)");
}
