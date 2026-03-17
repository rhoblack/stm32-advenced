/**
 * @file    ch19_refactoring_before.c
 * @brief   리팩토링 전 코드 — 레이어 위반, 버그 패턴 사례
 * @note    Ch19 교육용: 어떤 코드가 나쁜 코드인지 보여주는 예시
 *          실제 프로젝트에서 이런 패턴을 발견하면 리팩토링 대상
 */

#include "main.h"
#include "stm32f4xx_hal.h"

/* ============================================================
 * 문제 1: volatile 누락 — ISR과 main() 공유 플래그
 * ============================================================ */

/* BAD: volatile 없이 ISR 공유 변수 선언 */
uint8_t g_dma_transfer_done = 0;    /* 컴파일러 최적화로 읽기 생략 가능! */
uint8_t g_timer_tick = 0;            /* ISR에서 설정, main에서 읽음 */

void bad_wait_for_dma(void)
{
    /* BAD: 컴파일러가 이 루프를 최적화로 무한루프로 변환하거나 제거 가능 */
    while (g_dma_transfer_done == 0)
    {
        /* 아무것도 하지 않음 — 최적화 레벨 -O2 이상에서 문제 발생 */
    }
    g_dma_transfer_done = 0;
}

/* ============================================================
 * 문제 2: 레이어 위반 — App이 HAL을 직접 호출
 * ============================================================ */

/* BAD: app_main.c에서 HAL GPIO를 직접 조작 (Driver 레이어 우회) */
void bad_app_update_status(uint8_t error_count)
{
    if (error_count > 5)
    {
        /* BAD: App 레이어가 HAL을 직접 호출 — 레이어드 아키텍처 위반 */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

        /* BAD: 또 다른 HAL 직접 접근 */
        extern UART_HandleTypeDef huart2;
        uint8_t msg[] = "ERROR!\r\n";
        HAL_UART_Transmit(&huart2, msg, 8, 100);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    }
}

/* ============================================================
 * 문제 3: HAL 반환값 무시
 * ============================================================ */

extern I2C_HandleTypeDef hi2c1;

/* BAD: HAL 함수 반환값을 확인하지 않음 */
void bad_sensor_read(uint8_t *buf, uint16_t size)
{
    uint8_t cmd[2] = {0x2C, 0x06};  /* SHT31 측정 명령 */

    /* BAD: HAL_OK/HAL_ERROR를 확인하지 않아 실패를 무시 */
    HAL_I2C_Master_Transmit(&hi2c1, 0x44 << 1, cmd, 2, 100);

    HAL_Delay(15);  /* 측정 대기 */

    /* BAD: 오류가 있어도 버퍼 읽기 시도 */
    HAL_I2C_Master_Receive(&hi2c1, 0x44 << 1, buf, size, 100);

    /* BAD: 이후 buf 내용이 유효한지 검증 없이 사용 */
}

/* ============================================================
 * 문제 4: 정수 오버플로우 — 부호 없는 정수 연산
 * ============================================================ */

/* BAD: 32비트 정수 오버플로우 위험 */
uint32_t bad_calc_steps(uint32_t rpm, uint32_t duration_ms)
{
    /* BAD: rpm * duration_ms가 4294967295를 초과할 수 있음 */
    /* rpm=100, duration_ms=1000000이면 10^8 → uint32_t 최대 4.29x10^9 초과 */
    uint32_t total_steps = rpm * duration_ms * 4096U / 60000U;

    return total_steps;
}

/* ============================================================
 * 문제 5: 재진입(Re-entrant) 문제 — 공유 자원 보호 없음
 * ============================================================ */

/* BAD: 전역 버퍼를 인터럽트와 main이 보호 없이 공유 */
static uint8_t g_tx_buf[64];
static uint8_t g_tx_len = 0;

/* main()에서 호출 */
void bad_uart_send(const char *str)
{
    /* BAD: 인터럽트가 여기서 개입하면 버퍼 내용이 덮어써질 수 있음 */
    g_tx_len = (uint8_t)strlen(str);

    /* BAD: Critical Section 없음 — ISR이 중간에 g_tx_buf 사용 가능 */
    memcpy(g_tx_buf, str, g_tx_len);

    extern UART_HandleTypeDef huart2;
    HAL_UART_Transmit_DMA(&huart2, g_tx_buf, g_tx_len);
}

/* ============================================================
 * 문제 6: 매크로 인수 괄호 누락
 * ============================================================ */

/* BAD: 인수에 괄호가 없어 연산자 우선순위 오류 발생 */
#define BAD_SQUARE(x)    x * x
#define BAD_ABS(x)       x < 0 ? -x : x

void bad_macro_usage(void)
{
    int a = 3;

    /* BAD: BAD_SQUARE(a+1) → a+1*a+1 = a + a + 1 = 7, 정답은 16 */
    int result1 = BAD_SQUARE(a + 1);

    /* BAD: BAD_ABS(a-5) → a-5 < 0 ? -a-5 : a-5 — 의도와 다름 */
    int result2 = BAD_ABS(a - 5);

    /* 컴파일은 되지만 결과가 틀림 */
    (void)result1;
    (void)result2;
}

/* ============================================================
 * 문제 7: switch-case fall-through 미표시
 * ============================================================ */

typedef enum {
    STATE_IDLE,
    STATE_RUNNING,
    STATE_ERROR
} BadState_t;

/* BAD: 의도하지 않은 fall-through */
void bad_state_handler(BadState_t state)
{
    switch (state)
    {
        case STATE_IDLE:
            /* 무언가 처리 */
            /* BAD: break 없음 — 의도치 않게 STATE_RUNNING으로 흘러내림 */

        case STATE_RUNNING:
            /* 무언가 처리 */
            break;

        case STATE_ERROR:
            /* 오류 처리 */
            break;

        /* BAD: default 없음 — 미처리 상태 발생 가능 */
    }
}
