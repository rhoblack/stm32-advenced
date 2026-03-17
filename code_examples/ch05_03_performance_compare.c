/* ch05_03_performance_compare.c — 폴링 vs DMA 성능 비교
 *
 * 레이어: App (테스트 코드)
 * 위치: 프로젝트 아키텍처 v0.5
 *
 * 폴링 복사와 DMA 복사를 100회 반복 측정하여 평균 비교.
 * LED 토글로 CPU 해방 효과를 시각적으로 확인.
 * Ch04의 uart_send_string()을 활용하여 PC에 결과 출력.
 */

#include "main.h"
#include "log.h"
#include "uart_driver.h"   /* Ch04 Driver 재사용 */
#include <stdio.h>
#include <string.h>

#define BUF_SIZE    1024   /* uint32_t 1024개 = 4KB */
#define REPEAT      100    /* 반복 횟수 */

static uint32_t src_buf[BUF_SIZE];
static uint32_t dst_buf[BUF_SIZE];
static volatile uint8_t dma_done = 0;

/* CubeMX 생성 DMA 핸들 */
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream0;

/* ===== DWT 초기화 ===== */
static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/* ===== DMA 완료 콜백 ===== */
static void on_dma_done(DMA_HandleTypeDef *hdma)
{
    dma_done = 1;
}

/* ===== 테스트 패턴 ===== */
static void fill_pattern(void)
{
    for (uint32_t i = 0; i < BUF_SIZE; i++) {
        src_buf[i] = 0xCAFE0000 | i;
    }
}

/* ===== 폴링 복사 100회 측정 ===== */
static uint32_t bench_polling(void)
{
    uint32_t total = 0;

    for (int r = 0; r < REPEAT; r++) {
        memset(dst_buf, 0, sizeof(dst_buf));

        uint32_t s = DWT->CYCCNT;
        for (uint32_t i = 0; i < BUF_SIZE; i++) {
            dst_buf[i] = src_buf[i];
        }
        uint32_t e = DWT->CYCCNT;

        total += (e - s);
    }

    return total / REPEAT;  /* 평균 사이클 */
}

/* ===== DMA 복사 100회 측정 ===== */
static uint32_t bench_dma(void)
{
    hdma_memtomem_dma2_stream0.XferCpltCallback
        = on_dma_done;

    uint32_t total = 0;

    for (int r = 0; r < REPEAT; r++) {
        memset(dst_buf, 0, sizeof(dst_buf));
        dma_done = 0;

        uint32_t s = DWT->CYCCNT;
        HAL_DMA_Start_IT(
            &hdma_memtomem_dma2_stream0,
            (uint32_t)src_buf,
            (uint32_t)dst_buf,
            BUF_SIZE);
        while (!dma_done) {
            /* 완료 대기 */
        }
        uint32_t e = DWT->CYCCNT;

        total += (e - s);
    }

    return total / REPEAT;  /* 평균 사이클 */
}

/* ===== 성능 비교 메인 함수 ===== */
void performance_compare(void)
{
    dwt_init();
    fill_pattern();

    LOG_I("=== 성능 비교 시작 ===");
    LOG_I("버퍼 크기: %lu bytes, 반복: %d회",
          (uint32_t)(BUF_SIZE * 4), REPEAT);

    /* 폴링 측정 */
    uint32_t poll_cycles = bench_polling();
    uint32_t poll_us = poll_cycles
                       / (SystemCoreClock / 1000000);

    /* DMA 측정 */
    uint32_t dma_cycles = bench_dma();
    uint32_t dma_us = dma_cycles
                      / (SystemCoreClock / 1000000);

    /* 결과 로그 출력 */
    LOG_I("=== 결과 ===");
    LOG_I("폴링: %5lu cycles (%lu us)",
          poll_cycles, poll_us);
    LOG_I("DMA : %5lu cycles (%lu us)",
          dma_cycles, dma_us);

    if (dma_cycles > 0) {
        uint32_t ratio_x10 = (poll_cycles * 10)
                              / dma_cycles;
        LOG_I("비율: 폴링/DMA = %lu.%lux",
              ratio_x10 / 10, ratio_x10 % 10);
    }

    /* UART로 PC 터미널에도 출력 */
    char msg[128];
    snprintf(msg, sizeof(msg),
        "\r\n[BENCH] Poll=%lu us, DMA=%lu us\r\n",
        poll_us, dma_us);
    uart_send_string(msg);
}

/* ===== CPU 해방 시각화 — LED 토글 비교 ===== */
void cpu_freedom_demo(void)
{
    LOG_I("=== CPU 해방 시각화 ===");

    hdma_memtomem_dma2_stream0.XferCpltCallback
        = on_dma_done;

    /* 1) 폴링 중 LED 토글 시도 */
    LOG_I("[폴링] 복사 시작 — LED 토글 불가");
    for (int r = 0; r < 1000; r++) {
        for (uint32_t i = 0; i < BUF_SIZE; i++) {
            dst_buf[i] = src_buf[i];
        }
        /* CPU가 복사에 묶여 토글 주기 불규칙 */
    }
    LOG_I("[폴링] 완료");

    /* 2) DMA 중 LED 토글 */
    LOG_I("[DMA] 복사 시작 — LED 자유롭게 토글");
    for (int r = 0; r < 1000; r++) {
        dma_done = 0;
        HAL_DMA_Start_IT(
            &hdma_memtomem_dma2_stream0,
            (uint32_t)src_buf,
            (uint32_t)dst_buf,
            BUF_SIZE);
        while (!dma_done) {
            /* CPU가 자유! LED를 토글할 수 있다 */
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        }
    }
    LOG_I("[DMA] 완료 — LED 정상 토글 확인");
}
