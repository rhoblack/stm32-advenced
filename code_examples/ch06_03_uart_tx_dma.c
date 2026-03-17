/* ch06_03_uart_tx_dma.c — UART TX DMA + FSM + printf 리다이렉트
 *
 * 레이어: Driver
 * 위치: 프로젝트 아키텍처 v0.6
 *
 * 동작 흐름:
 *   1) uart_tx_send() → TX 링 버퍼에 데이터 저장
 *   2) TX_IDLE이면 링 버퍼에서 꺼내 DMA 전송 시작
 *   3) DMA 완료 콜백 → 링 버퍼에 남은 데이터 있으면 연쇄 전송
 *
 * CubeMX 설정:
 *   - DMA: USART2_TX → DMA1 Stream6 Ch4, Normal, Byte
 *   - NVIC: DMA1_Stream6 global interrupt → Enable
 */

#include "ch06_01_ring_buffer.h"
#include "main.h"
#include "log.h"
#include <string.h>

extern UART_HandleTypeDef huart2;

/* ===== TX FSM 상태 ===== */
typedef enum {
    TX_IDLE = 0,    /* DMA 유휴 — 즉시 전송 가능 */
    TX_BUSY = 1     /* DMA 전송 중 — 링 버퍼에 큐잉 */
} tx_state_t;

static volatile tx_state_t s_tx_state = TX_IDLE;

/* ===== TX 링 버퍼 ===== */
#define TX_BUF_SIZE  512
static uint8_t s_tx_mem[TX_BUF_SIZE];
static ring_buffer_t s_tx_ring;

/* DMA 전송용 임시 버퍼 */
#define TX_DMA_CHUNK  64
static uint8_t s_tx_dma_buf[TX_DMA_CHUNK];

/* ===== 내부: 링 버퍼에서 꺼내 DMA 전송 시작 ===== */

static void tx_start_dma(void)
{
    uint16_t count = 0;
    uint8_t byte;

    /* 링 버퍼에서 최대 TX_DMA_CHUNK바이트 꺼내기 */
    while (count < TX_DMA_CHUNK &&
           ring_buf_read(&s_tx_ring, &byte)) {
        s_tx_dma_buf[count++] = byte;
    }

    if (count > 0) {
        s_tx_state = TX_BUSY;
        HAL_UART_Transmit_DMA(&huart2, s_tx_dma_buf, count);
        LOG_D("TX DMA 시작: %d바이트", count);
    } else {
        s_tx_state = TX_IDLE;
    }
}

/* ===== TX 초기화 ===== */

void uart_tx_dma_init(void)
{
    ring_buf_init(&s_tx_ring, s_tx_mem, TX_BUF_SIZE);
    s_tx_state = TX_IDLE;
    LOG_I("UART TX DMA 초기화 완료 (FSM: IDLE)");
}

/* ===== 공개 API: 데이터 송신 ===== */

void uart_tx_dma_send(const uint8_t *buf, uint16_t len)
{
    /* 링 버퍼에 데이터 저장 */
    for (uint16_t i = 0; i < len; i++) {
        if (!ring_buf_write(&s_tx_ring, buf[i])) {
            LOG_W("TX 버퍼 오버플로: %d/%d 저장됨", i, len);
            break;
        }
    }

    /* ★ Critical Section: 상태 확인 + DMA 시작 ★ */
    __disable_irq();
    if (s_tx_state == TX_IDLE) {
        tx_start_dma();
    }
    __enable_irq();
}

/* ===== DMA 전송 완료 콜백 ===== */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART2) {
        return;
    }

    /* 링 버퍼에 남은 데이터 있으면 연쇄 전송 */
    if (ring_buf_available(&s_tx_ring) > 0) {
        tx_start_dma();
    } else {
        s_tx_state = TX_IDLE;
        LOG_D("TX 완료, IDLE 복귀");
    }
}

/* ===== printf 리다이렉트 (DMA 논블로킹) ===== */

int _write(int file, char *ptr, int len)
{
    (void)file;
    uart_tx_dma_send((const uint8_t *)ptr, (uint16_t)len);
    return len;
}
