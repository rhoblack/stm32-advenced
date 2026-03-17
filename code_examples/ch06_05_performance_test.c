/* ch06_05_performance_test.c — 폴링 vs 인터럽트 vs DMA 성능 비교
 *
 * 레이어: App
 * 위치: 프로젝트 아키텍처 v0.6
 *
 * DWT CYCCNT(사이클 카운터)로 CPU 점유 시간을 측정하여
 * 3가지 UART 전송 방식의 성능을 비교합니다.
 *
 * 사전 조건:
 *   - Ch05에서 DWT 사이클 카운터 활성화 코드 재사용
 *   - uart_driver v0.6 (DMA) 초기화 완료 상태
 *   - USART2: 115200-8-N-1
 */

#include "main.h"
#include "ch06_04_uart_driver_v2.h"
#include "log.h"
#include <stdio.h>

extern UART_HandleTypeDef huart2;

/* ===== DWT 사이클 카운터 초기화 (Ch05 동일) ===== */

static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    LOG_D("DWT 사이클 카운터 활성화");
}

/* ===== 테스트 데이터 ===== */
#define TEST_SIZE  100
static uint8_t test_buf[TEST_SIZE];

static void fill_test_data(void)
{
    for (uint16_t i = 0; i < TEST_SIZE; i++) {
        test_buf[i] = 'A' + (i % 26);
    }
}

/* ===== 성능 측정 함수 ===== */

void performance_test(void)
{
    uint32_t start, end;
    uint32_t polling_cycles;
    uint32_t dma_cycles;

    dwt_init();
    fill_test_data();

    LOG_I("=== 성능 비교 시작 (100바이트 @115200bps) ===");

    /* --- 1) 폴링 전송 --- */
    start = DWT->CYCCNT;
    HAL_UART_Transmit(&huart2, test_buf, TEST_SIZE, 100);
    end = DWT->CYCCNT;
    polling_cycles = end - start;

    LOG_I("[폴링]     CPU 사이클: %lu (%.2f ms)",
          polling_cycles,
          (float)polling_cycles / (SystemCoreClock / 1000));

    HAL_Delay(100);  /* 전송 완료 대기 */

    /* --- 2) DMA 전송 (v0.6 드라이버) --- */
    start = DWT->CYCCNT;
    uart_send(test_buf, TEST_SIZE);
    end = DWT->CYCCNT;
    dma_cycles = end - start;

    LOG_I("[DMA]      CPU 사이클: %lu (%.2f ms)",
          dma_cycles,
          (float)dma_cycles / (SystemCoreClock / 1000));

    HAL_Delay(100);  /* DMA 전송 완료 대기 */

    /* --- 결과 요약 --- */
    LOG_I("=== 성능 비교 결과 ===");
    LOG_I("폴링:  %lu cycles", polling_cycles);
    LOG_I("DMA:   %lu cycles", dma_cycles);

    if (dma_cycles > 0) {
        LOG_I("성능 비: 폴링/DMA = %lu배",
              polling_cycles / dma_cycles);
    }

    LOG_I("=== 테스트 완료 ===");
}

/* ===== 메인 루프에서 호출 예시 ===== */
/*
 * int main(void)
 * {
 *     HAL_Init();
 *     SystemClock_Config();
 *     MX_GPIO_Init();
 *     MX_DMA_Init();       // ★ DMA 먼저 초기화
 *     MX_USART2_UART_Init();
 *
 *     uart_driver_init();   // v0.6 DMA 드라이버
 *     performance_test();   // 성능 비교 실행
 *
 *     while (1) {
 *         // 에코 테스트: 수신 데이터를 그대로 송신
 *         if (uart_available() > 0) {
 *             uint8_t byte = uart_read_byte();
 *             uart_send(&byte, 1);
 *         }
 *     }
 * }
 */
