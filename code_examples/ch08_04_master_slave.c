/* ch08_04_master_slave.c — TIM1(Master) → TIM2(Slave) 타이머 연계
 *
 * 레이어: App (실습 예제)
 * 의존: HAL (stm32f4xx_hal_tim.h)
 *
 * CubeMX 설정:
 *
 *   [Master] TIM1:
 *     - Internal Clock, PSC=8399, ARR=9999 → 1초 주기
 *     - Trigger Output (TRGO): Update Event
 *     - Master Mode Selection: MMS=Update
 *
 *   [Slave] TIM2:
 *     - Slave Mode: Trigger Mode
 *     - Trigger Source: ITR0 (TIM1 TRGO)
 *     - PSC=839, ARR=999 → 10ms PWM 주기
 *     - CH1 PWM: PA0 (LED)
 *
 *   결과: TIM1이 1초마다 UEV → TRGO → TIM2 Enable
 *         TIM2는 Enable 후 10ms PWM 1주기 출력
 */

#include "main.h"
#include "log.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;

/**
 * @brief Master-Slave 타이머 연계 시작
 *
 * 순서가 중요합니다:
 *   1. Slave(TIM2)를 먼저 대기 상태로
 *   2. Master(TIM1)를 시작 → TRGO 발생 시 Slave 자동 시작
 */
void master_slave_start(void)
{
    /* Slave: TIM2 PWM 시작 (Trigger 대기 상태) */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    LOG_I("Slave(TIM2) PWM 대기 — Trigger 수신 시 시작");

    /* Master: TIM1 Base 시작 (1초 주기, UEV → TRGO) */
    HAL_TIM_Base_Start(&htim1);
    LOG_I("Master(TIM1) 시작 — 1초마다 TRGO 출력");

    LOG_D("연계 구조: TIM1(1s UEV) → TRGO → ITR0 → TIM2(PWM)");
    LOG_I("PA0 LED가 1초마다 PWM 출력합니다");
}

/**
 * @brief Master-Slave 정지
 */
void master_slave_stop(void)
{
    HAL_TIM_Base_Stop(&htim1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    LOG_I("Master-Slave 타이머 정지");
}
