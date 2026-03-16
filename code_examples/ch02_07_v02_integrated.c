/**
 * 파일명 : ch02_07_v02_integrated.c
 * 목  적 : v0.2 완성본 — Ch01 GPIO/EXTI 코드 + Ch02 로그 통합
 * MCU   : STM32F411RE (NUCLEO-F411RE)
 * 버  전 : v0.2
 *
 * 설명:
 *   Ch01에서 작성한 GPIO 초기화 및 EXTI 인터럽트 콜백에
 *   Ch02에서 완성한 LOG_I/LOG_D 매크로를 통합한다.
 *   - 버튼 누름/해제 이벤트를 LOG_I로 기록한다.
 *   - 디바운싱 상태(무시된 이벤트)를 LOG_D로 기록한다.
 *   - 메인 루프에서 1초마다 LOG_D로 동작 상태를 출력한다.
 *
 * 핀 배정 (NUCLEO-F411RE 기본):
 *   PC13 — 유저 버튼 (EXTI13, 액티브 LOW, 내부 풀업)
 *   PA5  — 유저 LED  (출력 푸시풀)
 */

#include "stm32f4xx_hal.h"
#include "ch02_04_log_macro.h"   /* LOG_E / LOG_W / LOG_I / LOG_D */

/* ---------------------------------------------------------------
 * 전역 상태 변수
 * --------------------------------------------------------------- */
static volatile uint8_t  g_led_state     = 0;      /* 현재 LED 상태 */
static volatile uint32_t g_last_press_ms = 0;      /* 마지막 버튼 누름 시각 */
static volatile uint32_t g_press_count   = 0;      /* 유효 버튼 누름 횟수 */

/* 디바운싱 임계값 (ms) */
#define DEBOUNCE_MS     50U

/* ---------------------------------------------------------------
 * SystemClock_Config 스켈레톤
 *   CubeIDE가 자동 생성하는 클럭 설정 함수.
 *   HSI 16 MHz → PLL → SYSCLK 100 MHz (F411RE 최대)
 * --------------------------------------------------------------- */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    /* HSI 16 MHz 활성화, PLL 설정 (100 MHz) */
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState       = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM       = 16;
    osc.PLL.PLLN       = 200;
    osc.PLL.PLLP       = RCC_PLLP_DIV2;
    osc.PLL.PLLQ       = 4;

    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    {
        LOG_E("클럭 발진기 설정 실패");
        Error_Handler();
    }

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_3) != HAL_OK)
    {
        LOG_E("클럭 버스 설정 실패");
        Error_Handler();
    }

    LOG_I("SystemClock 설정 완료: SYSCLK=%lu Hz", HAL_RCC_GetSysClockFreq());
}

/* ---------------------------------------------------------------
 * MX_GPIO_Init 스켈레톤
 *   PA5  : LED 출력 (푸시풀, 풀업 없음)
 *   PC13 : 버튼 입력 (EXTI13, 하강 에지, 내부 풀업)
 * --------------------------------------------------------------- */
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* PA5 — 유저 LED */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin   = GPIO_PIN_5;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);  /* LED 초기 OFF */
    LOG_I("PA5 LED 핀 초기화 완료");

    /* PC13 — 유저 버튼 (EXTI) */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    gpio.Pin  = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_IT_FALLING;   /* 하강 에지 = 버튼 누름 */
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &gpio);

    /* EXTI13 인터럽트 활성화 */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    LOG_I("PC13 버튼 EXTI 초기화 완료");
}

/* ---------------------------------------------------------------
 * HAL_GPIO_EXTI_Callback — EXTI 인터럽트 핸들러에서 호출됨
 *   디바운싱 후 LED 토글 + 로그 출력
 * --------------------------------------------------------------- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin != GPIO_PIN_13)
    {
        return;   /* PC13 외 핀은 무시 */
    }

    uint32_t now = HAL_GetTick();

    /* 디바운싱: 마지막 누름으로부터 DEBOUNCE_MS 이내이면 무시 */
    if ((now - g_last_press_ms) < DEBOUNCE_MS)
    {
        LOG_D("버튼 이벤트 무시 (디바운싱 중, 경과=%lu ms)",
              now - g_last_press_ms);
        return;
    }

    /* 유효한 버튼 누름 처리 */
    g_last_press_ms = now;
    g_press_count++;

    /* LED 토글 */
    g_led_state ^= 1U;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,
                      g_led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);

    LOG_I("버튼 누름 #%lu — LED %s (tick=%lu ms)",
          g_press_count,
          g_led_state ? "ON" : "OFF",
          now);

    LOG_D("EXTI Callback 완료: g_led_state=%u, g_last_press_ms=%lu",
          g_led_state, g_last_press_ms);
}

/* ---------------------------------------------------------------
 * main — 초기화 후 1초 주기 상태 로그 출력
 * --------------------------------------------------------------- */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    LOG_I("=== v0.2 통합 데모 시작 ===");
    LOG_D("LOG_LEVEL=%d", LOG_LEVEL);

    uint32_t loop_count = 0;

    while (1)
    {
        /* 1초마다 현재 상태를 DEBUG 레벨로 출력 */
        LOG_D("루프 #%lu | LED=%u | 버튼 누름=%lu회 | Tick=%lu ms",
              loop_count++,
              g_led_state,
              g_press_count,
              HAL_GetTick());

        HAL_Delay(1000);
    }
}
