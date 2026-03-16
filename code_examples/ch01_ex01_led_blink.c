/**
 * @file    ch01_ex01_led_blink.c
 * @brief   LED 점멸 Hello World — GPIO 출력의 가장 단순한 형태
 *
 * @board   NUCLEO-F411RE (STM32F411RE)
 * @핀연결  LD2 (녹색 LED) = PA5, 액티브 하이 (GPIO HIGH → LED ON)
 *
 * @예상동작
 *   - 프로그램 시작 후 LD2가 500ms 간격으로 계속 점멸한다.
 *   - ON 500ms → OFF 500ms → ON 500ms → ... 반복
 *
 * @CubeMX_설정
 *   - PA5: GPIO_OUTPUT, Push-Pull, No Pull-up/Pull-down, Speed=Low
 *   - 클럭: HSI 16MHz (기본값) 또는 HSE → PLL 100MHz 설정 가능
 *
 * @컴파일환경
 *   - STM32CubeIDE 또는 Keil MDK
 *   - STM32Cube HAL 드라이버 포함 필요
 *
 * TODO: Ch02에서 LOG 시스템 추가
 */

#include "main.h"

/* =========================================================
 * 주의: 아래 함수 선언 및 전역 변수는 CubeMX가 자동 생성합니다.
 * 실제 프로젝트에서는 CubeMX가 생성한 main.c 안에 통합됩니다.
 * 이 파일은 핵심 로직만 발췌하여 설명합니다.
 * ========================================================= */

/**
 * @brief  시스템 초기화 (CubeMX 자동 생성 — 수정 금지)
 *         HAL_Init(), SystemClock_Config(), MX_GPIO_Init() 순서로 호출됨
 */
/* [CubeMX 자동 생성 영역 — 생략] */

/**
 * @brief  메인 함수
 */
int main(void)
{
    /* --- 시스템 초기화 (CubeMX 자동 생성 코드) --- */
    HAL_Init();                 /* HAL 라이브러리 초기화, SysTick 1ms 설정 */
    SystemClock_Config();       /* 클럭 설정 (CubeMX 자동 생성) */
    MX_GPIO_Init();             /* GPIO 초기화 (CubeMX 자동 생성: PA5 출력 설정) */

    /* --- 메인 루프 --- */
    while (1)
    {
        /*
         * HAL_GPIO_TogglePin: 지정한 핀의 출력 상태를 반전시킨다.
         *   - 현재 HIGH → LOW로 변경
         *   - 현재 LOW  → HIGH로 변경
         * 매번 호출할 때마다 상태가 바뀌므로 별도 상태 변수가 필요 없다.
         */
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);  /* LD2 상태 반전 */

        /*
         * HAL_Delay: 밀리초(ms) 단위 지연 함수
         *   - SysTick 타이머 기반으로 동작 (1ms 해상도)
         *   - 500ms 대기 → LED 점멸 주기 = 500ms ON + 500ms OFF = 1초
         */
        HAL_Delay(500);                         /* 500ms 대기 */

        /* TODO: Ch02에서 LOG 시스템 추가 */
    }
    /* while(1)은 절대 탈출하지 않으므로 이 아래 코드는 실행되지 않는다 */
}

/* =========================================================
 * 동작 확인 방법:
 *   1. STM32CubeIDE에서 빌드 후 Run → NUCLEO 보드의 LD2 LED 관찰
 *   2. LED가 0.5초 간격으로 깜빡이면 정상 동작
 *   3. 점멸 속도를 바꾸려면 HAL_Delay(500) 값을 조정한다
 *      - HAL_Delay(100): 빠른 점멸 (0.1초 간격)
 *      - HAL_Delay(1000): 느린 점멸 (1초 간격)
 *
 * 심화 실험:
 *   - HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET)   → LED 강제 ON
 *   - HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET) → LED 강제 OFF
 *   - HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) → 현재 출력 상태 읽기
 * ========================================================= */
