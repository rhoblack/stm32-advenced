/* ch06_04_uart_driver_v2.c — UART Driver 통합 DMA 구현 (v0.6)
 *
 * 레이어: Driver
 * 의존: HAL (stm32f4xx_hal_uart.h), ring_buffer — 하위 레이어만 참조
 *
 * ★ Ch04 uart_driver.c에서 내부 구현만 교체 ★
 *   - 송신: HAL_UART_Transmit (폴링) → HAL_UART_Transmit_DMA
 *   - 수신: HAL_UART_Receive_IT (1바이트) → HAL_UARTEx_ReceiveToIdle_DMA (Circular)
 *   - 외부 API (uart_driver.h): 변경 없음
 *
 * CubeMX 설정:
 *   - USART2: Asynchronous, 115200-8-N-1
 *   - PA2 = USART2_TX, PA3 = USART2_RX
 *   - DMA Settings:
 *     USART2_RX → DMA1 Stream5 Ch4, Circular, Byte
 *     USART2_TX → DMA1 Stream6 Ch4, Normal, Byte
 *   - NVIC: DMA1_Stream5, DMA1_Stream6, USART2 → Enable
 */

#include "ch06_04_uart_driver_v2.h"
#include "ch06_01_ring_buffer.h"
#include "main.h"
#include "log.h"
#include <string.h>

/* ===== HAL 핸들 (CubeMX 생성) ===== */
extern UART_HandleTypeDef huart2;

/* ===== RX: DMA 하드웨어 버퍼 + 소프트웨어 링 버퍼 ===== */
#define DMA_RX_BUF_SIZE   64
#define APP_RX_BUF_SIZE  256

static uint8_t s_dma_rx_buf[DMA_RX_BUF_SIZE];
static uint8_t s_app_rx_mem[APP_RX_BUF_SIZE];
static ring_buffer_t s_rx_ring;
static uint16_t s_rx_old_pos = 0;

/* ===== TX: FSM + 링 버퍼 + DMA 전송 버퍼 ===== */
typedef enum {
    TX_IDLE = 0,
    TX_BUSY = 1
} tx_state_t;

static volatile tx_state_t s_tx_state = TX_IDLE;

#define TX_BUF_SIZE      512
#define TX_DMA_CHUNK      64

static uint8_t s_tx_mem[TX_BUF_SIZE];
static ring_buffer_t s_tx_ring;
static uint8_t s_tx_dma_buf[TX_DMA_CHUNK];

/* ===== 내부: TX DMA 전송 시작 ===== */

static void tx_start_dma(void)
{
    uint16_t count = 0;
    uint8_t byte;

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

/* ===== 공개 API: 초기화 ===== */

void uart_driver_init(void)
{
    /* RX 링 버퍼 초기화 */
    ring_buf_init(&s_rx_ring, s_app_rx_mem, APP_RX_BUF_SIZE);
    s_rx_old_pos = 0;

    /* TX 링 버퍼 + FSM 초기화 */
    ring_buf_init(&s_tx_ring, s_tx_mem, TX_BUF_SIZE);
    s_tx_state = TX_IDLE;

    /* ★ RX: Circular DMA + IDLE 감지 시작 ★ */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2,
                                  s_dma_rx_buf,
                                  DMA_RX_BUF_SIZE);

    /* Half-Transfer 인터럽트 비활성화 (IDLE만 사용) */
    __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);

    LOG_I("uart_driver v0.6 (DMA) 초기화 완료 (115200-8-N-1)");
}

/* ===== 공개 API: 송신 ===== */

uart_status_t uart_send(const uint8_t *buf, uint16_t len)
{
    if (buf == NULL || len == 0) {
        LOG_W("uart_send: 잘못된 파라미터");
        return UART_ERROR;
    }

    /* 링 버퍼에 데이터 저장 */
    for (uint16_t i = 0; i < len; i++) {
        if (!ring_buf_write(&s_tx_ring, buf[i])) {
            LOG_W("TX 버퍼 오버플로: %d/%d 저장", i, len);
            return UART_ERROR;
        }
    }

    /* ★ Critical Section: 상태 확인 + DMA 시작 ★ */
    __disable_irq();
    if (s_tx_state == TX_IDLE) {
        tx_start_dma();
    }
    __enable_irq();

    LOG_D("uart_send: %d바이트 큐잉 완료", len);
    return UART_OK;
}

uart_status_t uart_send_string(const char *str)
{
    if (str == NULL) {
        return UART_ERROR;
    }
    return uart_send((const uint8_t *)str,
                     (uint16_t)strlen(str));
}

/* ===== 공개 API: 수신 ===== */

uint16_t uart_available(void)
{
    return ring_buf_available(&s_rx_ring);
}

uint8_t uart_read_byte(void)
{
    uint8_t byte;
    if (ring_buf_read(&s_rx_ring, &byte)) {
        return byte;
    }
    return 0;  /* 버퍼 비어있음 */
}

/* ===== HAL 콜백: RX IDLE / DMA 완료 ===== */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart,
                                uint16_t Size)
{
    if (huart->Instance != USART2) {
        return;
    }

    uint16_t new_pos = Size;

    if (new_pos != s_rx_old_pos) {
        if (new_pos > s_rx_old_pos) {
            /* 연속 구간: old_pos ~ new_pos */
            for (uint16_t i = s_rx_old_pos; i < new_pos; i++) {
                ring_buf_write(&s_rx_ring, s_dma_rx_buf[i]);
            }
        } else {
            /* DMA 순환 발생: old_pos ~ 끝, 0 ~ new_pos */
            for (uint16_t i = s_rx_old_pos; i < DMA_RX_BUF_SIZE; i++) {
                ring_buf_write(&s_rx_ring, s_dma_rx_buf[i]);
            }
            for (uint16_t i = 0; i < new_pos; i++) {
                ring_buf_write(&s_rx_ring, s_dma_rx_buf[i]);
            }
        }
        s_rx_old_pos = new_pos;

        if (s_rx_old_pos >= DMA_RX_BUF_SIZE) {
            s_rx_old_pos = 0;
        }
    }

    LOG_D("RX: pos=%d, available=%d",
          new_pos, ring_buf_available(&s_rx_ring));
}

/* ===== HAL 콜백: TX DMA 완료 ===== */

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
    uart_send((const uint8_t *)ptr, (uint16_t)len);
    return len;
}
