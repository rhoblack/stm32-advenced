/**
 * @file    ch19_refactoring_after.c
 * @brief   리팩토링 후 코드 — MISRA-C 준수, 레이어 분리, 방어적 프로그래밍
 * @note    ch19_refactoring_before.c의 모든 문제를 수정한 실무 수준 코드
 */

#include "main.h"
#include "log.h"
#include "led_driver.h"     /* Driver 레이어 인터페이스 — HAL 직접 접근 대신 */
#include "uart_driver.h"    /* Driver 레이어 인터페이스 */
#include "sht31_driver.h"   /* Driver 레이어 인터페이스 */

/* ============================================================
 * 수정 1: volatile 선언 추가 — ISR과 main() 공유 플래그
 * ============================================================ */

/* GOOD: volatile 선언으로 컴파일러 최적화 방지 */
volatile uint8_t g_dma_transfer_done = 0;   /* ISR에서 설정, main에서 읽음 */
volatile uint8_t g_timer_tick = 0;           /* ISR에서 설정, main에서 읽음 */

#define DMA_WAIT_TIMEOUT_MS   (100U)

/**
 * @brief DMA 전송 완료 대기 (타임아웃 포함)
 * @return 0: 성공, -1: 타임아웃
 */
int8_t good_wait_for_dma(void)
{
    uint32_t start_tick = HAL_GetTick();

    /* GOOD: volatile 변수는 매번 메모리에서 읽음 + 타임아웃으로 무한 루프 방지 */
    while (g_dma_transfer_done == 0U)
    {
        if ((HAL_GetTick() - start_tick) >= DMA_WAIT_TIMEOUT_MS)
        {
            LOG_E("DMA 대기 타임아웃: %lu ms 초과", DMA_WAIT_TIMEOUT_MS);
            return -1;
        }
    }

    g_dma_transfer_done = 0U;
    LOG_D("DMA 전송 완료 확인");
    return 0;
}

/* ============================================================
 * 수정 2: 레이어 준수 — App은 Driver 인터페이스만 호출
 * ============================================================ */

/* GOOD: App 레이어가 Driver 레이어의 공개 인터페이스만 사용 */
void good_app_update_status(uint8_t error_count)
{
    if (error_count > 5U)
    {
        /* GOOD: Driver 레이어 인터페이스를 통해 LED 제어 */
        LED_SetState(LED_GREEN, LED_ON);

        /* GOOD: UART Driver 인터페이스를 통해 메시지 전송 */
        UART_SendString("ERROR!\r\n");

        LOG_E("오류 횟수 초과: count=%u", error_count);
    }
    else
    {
        LED_SetState(LED_GREEN, LED_OFF);
    }
}

/* ============================================================
 * 수정 3: HAL 반환값 확인 — 방어적 프로그래밍
 * ============================================================ */

/**
 * @brief SHT31 센서 데이터 읽기 (오류 처리 포함)
 * @param[out] buf  수신 버퍼 (최소 6바이트)
 * @param[in]  size 버퍼 크기
 * @return HAL_OK: 성공, HAL_ERROR: 통신 오류
 */
HAL_StatusTypeDef good_sensor_read(uint8_t *buf, uint16_t size)
{
    extern I2C_HandleTypeDef hi2c1;
    HAL_StatusTypeDef ret;
    uint8_t cmd[2] = {0x2CU, 0x06U};  /* SHT31 High Repeatability 측정 명령 */

    /* GOOD: 반환값 확인 후 오류 처리 */
    ret = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(0x44U << 1U), cmd, 2U, 100U);
    if (ret != HAL_OK)
    {
        LOG_E("SHT31 명령 전송 실패: ret=%d", (int)ret);
        return ret;
    }

    HAL_Delay(15U);  /* 측정 완료 대기 (15ms) */

    /* GOOD: 수신 오류도 확인 */
    ret = HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(0x44U << 1U), buf, size, 100U);
    if (ret != HAL_OK)
    {
        LOG_E("SHT31 데이터 수신 실패: ret=%d", (int)ret);
        return ret;
    }

    LOG_D("SHT31 읽기 성공: buf[0]=%02X buf[1]=%02X", buf[0], buf[1]);
    return HAL_OK;
}

/* ============================================================
 * 수정 4: 정수 오버플로우 방지 — 연산 순서 조정 및 타입 명시
 * ============================================================ */

#define STEPS_PER_REV   (4096U)         /* 28BYJ-48 하프 스텝 1회전 */
#define MS_PER_MIN      (60000U)        /* 분당 밀리초 */

/**
 * @brief RPM과 시간으로 스텝 수 계산 (오버플로우 안전)
 * @param[in] rpm         분당 회전수 (최대 100)
 * @param[in] duration_ms 지속 시간 밀리초 (최대 60000)
 * @return 총 스텝 수
 */
uint32_t good_calc_steps(uint32_t rpm, uint32_t duration_ms)
{
    /* GOOD: 나눗셈을 먼저 수행하여 중간 값 범위 제한
     * 최대: 100 * 4096 / 60000 * 60000 = 409600 (uint32_t 범위 내) */
    uint32_t steps_per_ms = (rpm * STEPS_PER_REV) / MS_PER_MIN;
    uint32_t total_steps  = steps_per_ms * duration_ms;

    LOG_D("스텝 계산: rpm=%lu, dur=%lu ms, steps=%lu", rpm, duration_ms, total_steps);
    return total_steps;
}

/* ============================================================
 * 수정 5: Critical Section으로 재진입 문제 해결
 * ============================================================ */

static uint8_t  g_tx_buf[64];
static uint8_t  g_tx_len = 0U;
static volatile uint8_t g_tx_busy = 0U;  /* DMA 전송 중 플래그 */

/**
 * @brief UART DMA 전송 (Critical Section 보호)
 * @param[in] str 전송할 문자열
 * @return 0: 성공, -1: 버스 중, -2: 파라미터 오류
 */
int8_t good_uart_send(const char *str)
{
    extern UART_HandleTypeDef huart2;
    uint32_t primask;
    size_t   len;

    if (str == NULL)
    {
        LOG_E("UART 전송: NULL 포인터");
        return -2;
    }

    len = strlen(str);
    if (len == 0U || len >= sizeof(g_tx_buf))
    {
        LOG_W("UART 전송: 잘못된 길이 %u", (unsigned)len);
        return -2;
    }

    /* GOOD: Critical Section 진입 — 인터럽트 차단 */
    primask = __get_PRIMASK();
    __disable_irq();

    if (g_tx_busy != 0U)
    {
        __set_PRIMASK(primask);  /* 인터럽트 복원 */
        LOG_W("UART 전송 버스 중 — 요청 무시");
        return -1;
    }

    g_tx_len  = (uint8_t)len;
    g_tx_busy = 1U;
    memcpy(g_tx_buf, str, g_tx_len);

    __set_PRIMASK(primask);  /* GOOD: 인터럽트 복원 */

    HAL_StatusTypeDef ret = HAL_UART_Transmit_DMA(&huart2, g_tx_buf, g_tx_len);
    if (ret != HAL_OK)
    {
        g_tx_busy = 0U;
        LOG_E("UART DMA 전송 실패: ret=%d", (int)ret);
        return -1;
    }

    LOG_D("UART DMA 전송 시작: len=%u", g_tx_len);
    return 0;
}

/* DMA 전송 완료 콜백에서 호출 */
void UART_TxDMA_CompleteCallback(void)
{
    g_tx_busy = 0U;
    LOG_D("UART DMA 전송 완료");
}

/* ============================================================
 * 수정 6: 매크로 인수 완전 괄호 처리
 * ============================================================ */

/* GOOD: 모든 인수와 결과를 괄호로 감쌈 */
#define GOOD_SQUARE(x)   ((x) * (x))
#define GOOD_ABS(x)      (((x) < 0) ? (-(x)) : (x))

void good_macro_usage(void)
{
    int a = 3;

    /* GOOD: GOOD_SQUARE(a+1) → ((a+1)*(a+1)) = (4*4) = 16 — 올바름 */
    int result1 = GOOD_SQUARE(a + 1);

    /* GOOD: GOOD_ABS(a-5) → (((a-5) < 0) ? (-(a-5)) : (a-5)) = 2 */
    int result2 = GOOD_ABS(a - 5);

    LOG_D("SQUARE(%d+1)=%d, ABS(%d-5)=%d", a, result1, a, result2);
}

/* ============================================================
 * 수정 7: switch-case 명시적 break + default
 * ============================================================ */

typedef enum {
    STATE_IDLE    = 0U,
    STATE_RUNNING = 1U,
    STATE_ERROR   = 2U
} GoodState_t;

/* GOOD: 모든 case에 break, default 추가 */
void good_state_handler(GoodState_t state)
{
    switch (state)
    {
        case STATE_IDLE:
            LOG_D("상태: IDLE");
            break;  /* GOOD: 명시적 break */

        case STATE_RUNNING:
            LOG_D("상태: RUNNING");
            break;

        case STATE_ERROR:
            LOG_W("상태: ERROR");
            break;

        default:
            /* GOOD: 알 수 없는 상태 처리 */
            LOG_E("알 수 없는 상태: %d", (int)state);
            break;
    }
}
