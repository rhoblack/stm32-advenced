/**
 * 파일명 : ch03_03_v02_layer_analysis.c
 * 목  적 : v0.2 통합 코드를 4계층으로 분류한 분석본
 * MCU   : STM32F411RE (NUCLEO-F411RE)
 * 버  전 : v0.2 → v0.3 분석
 *
 * 설명:
 *   ch02_07_v02_integrated.c의 각 코드 요소에
 *   [HAL], [Driver], [App] 태그를 붙여 분류한다.
 *   수강생이 §3.3 실습에서 직접 분류한 후 답을 확인하는 용도.
 *
 * 분류 기준:
 *   [HAL]    — ST 제공 함수, 레지스터 접근 추상화
 *   [Driver] — 하드웨어 캡슐화, HAL 위의 래퍼
 *   [App]    — 사용자 로직, 이벤트 핸들링, 메인 루프
 *   [횡단]   — 모든 계층에서 사용 (로그 시스템)
 *   ⚠️      — 계층 위반 (App에서 HAL 직접 호출)
 */

#include "stm32f4xx_hal.h"     /* [HAL] ST 제공 헤더 */
#include "ch02_04_log_macro.h" /* [횡단] 모든 계층에서 사용 */

/* -----------------------------------------------------------
 * [App] 전역 상태 변수 — 현재 App 레이어에서 관리
 *   → 향후 Driver 또는 Service에서 캡슐화 필요
 * ----------------------------------------------------------- */
static volatile uint8_t  g_led_state     = 0;
static volatile uint32_t g_last_press_ms = 0;
static volatile uint32_t g_press_count   = 0;

#define DEBOUNCE_MS     50U

/* -----------------------------------------------------------
 * [HAL 설정] SystemClock_Config
 *   CubeMX 자동 생성. HAL API만 사용하여 클럭 트리 구성.
 *   아키텍처 관점: 시스템 초기화, 특정 레이어에 속하지 않음
 * ----------------------------------------------------------- */
void SystemClock_Config(void)
{
    /* [HAL] HAL_RCC_OscConfig, HAL_RCC_ClockConfig 호출 */
    LOG_I("SystemClock 설정 완료");  /* [횡단] 로그 */
}

/* -----------------------------------------------------------
 * [Driver 후보] MX_GPIO_Init
 *   현재 App 레벨에서 호출하지만, 내용은 Driver 초기화.
 *   → 향후 led_driver_init(), button_driver_init()으로 분리
 * ----------------------------------------------------------- */
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* [HAL] 클럭 활성화 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* [Driver 후보] LED 핀 초기화 → led_driver_init()으로 이동 예정 */
    gpio.Pin   = GPIO_PIN_5;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);         /* [HAL] 함수 호출 */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    LOG_I("PA5 LED 핀 초기화 완료");      /* [횡단] */

    /* [Driver 후보] 버튼 핀 초기화 → button_driver_init()으로 이동 예정 */
    gpio.Pin  = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_IT_FALLING;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &gpio);         /* [HAL] 함수 호출 */

    /* [HAL] NVIC 설정 */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    LOG_I("PC13 버튼 EXTI 초기화 완료");  /* [횡단] */
}

/* -----------------------------------------------------------
 * [App] HAL_GPIO_EXTI_Callback — 이벤트 핸들러
 *   아키텍처 관점:
 *   - HAL이 __weak로 선언한 콜백을 App에서 오버라이드
 *   - 현재 HAL_GPIO_WritePin() 직접 호출 ⚠️ 계층 위반!
 *   - 이상적: led_driver_toggle() 호출 (Driver API)
 * ----------------------------------------------------------- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin != GPIO_PIN_13) return;

    uint32_t now = HAL_GetTick();         /* [HAL] 시스템 틱 */

    /* [App] 디바운싱 로직 — 향후 button_driver 내부로 이동 */
    if ((now - g_last_press_ms) < DEBOUNCE_MS)
    {
        LOG_D("디바운싱 무시");           /* [횡단] */
        return;
    }

    g_last_press_ms = now;
    g_press_count++;

    /* ⚠️ [계층 위반] App에서 HAL 직접 호출!
     *    App → HAL (Driver를 건너뜀)
     *    이상적: led_driver_toggle(); */
    g_led_state ^= 1U;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,                  /* ⚠️ */
                      g_led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);

    LOG_I("버튼 #%lu — LED %s",          /* [횡단] */
          g_press_count,
          g_led_state ? "ON" : "OFF");
}

/* -----------------------------------------------------------
 * [App] main — 시스템 진입점
 *   초기화 오케스트레이션 + 메인 루프
 * ----------------------------------------------------------- */
int main(void)
{
    HAL_Init();              /* [HAL] HAL 초기화 */
    SystemClock_Config();    /* [HAL 설정] 클럭 설정 */
    MX_GPIO_Init();          /* [Driver 후보] GPIO 초기화 */

    /* [App] 시작 메시지 */
    LOG_I("=== v0.2 통합 데모 시작 ===");

    uint32_t loop_count = 0;

    /* [App] 메인 루프 */
    while (1)
    {
        LOG_D("루프 #%lu | LED=%u | 버튼=%lu회",
              loop_count++, g_led_state, g_press_count);

        HAL_Delay(1000);     /* [HAL] 폴링 대기 */
    }
}

/* ===== 분석 요약 =====
 *
 * 현재 v0.2 레이어 분류:
 * ┌──────────┬──────────────────────────┬────────────────────┐
 * │ 레이어   │ 코드 요소                │ 상태               │
 * ├──────────┼──────────────────────────┼────────────────────┤
 * │ App      │ main(), EXTI_Callback    │ ✅ 올바른 위치     │
 * │ Service  │ (없음)                   │ ⬜ Ch10부터 추가   │
 * │ Driver   │ log_print(), LOG 매크로  │ ✅ 올바른 위치     │
 * │ Driver   │ GPIO 초기화              │ ⚠️ 미분리 (App에 혼재) │
 * │ HAL      │ HAL_GPIO_*, HAL_NVIC_*   │ ✅ ST 제공         │
 * │ 위반     │ Callback → HAL 직접 호출 │ ⚠️ Ch04에서 수정   │
 * └──────────┴──────────────────────────┴────────────────────┘
 */
