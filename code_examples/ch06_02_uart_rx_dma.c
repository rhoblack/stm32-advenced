/* ch06_02_uart_rx_dma.c — UART RX Circular DMA + IDLE Line 감지
 *
 * 레이어: Driver
 * 위치: 프로젝트 아키텍처 v0.6
 *
 * 동작 흐름:
 *   1) DMA Circular 모드로 USART2 RX 데이터를 dma_rx_buf에 자동 기록
 *   2) IDLE Line 감지 시 HAL_UARTEx_RxEventCallback 호출
 *   3) 콜백에서 DMA 버퍼 → 소프트웨어 링 버퍼로 복사
 *   4) 메인 루프에서 링 버퍼 읽기
 *
 * CubeMX 설정:
 *   - USART2: Asynchronous, 115200-8-N-1
 *   - DMA: USART2_RX → DMA1 Stream5 Ch4, Circular, Byte
 *   - NVIC: DMA1_Stream5, USART2 global interrupt → Enable
 */

#include "ch06_01_ring_buffer.h"
#include "main.h"
#include "log.h"

extern UART_HandleTypeDef huart2;

/* ===== DMA 하드웨어 버퍼 (Circular) ===== */
#define DMA_RX_BUF_SIZE  64
static uint8_t s_dma_rx_buf[DMA_RX_BUF_SIZE];

/* ===== 소프트웨어 링 버퍼 ===== */
#define APP_RX_BUF_SIZE  256
static uint8_t s_app_rx_mem[APP_RX_BUF_SIZE];
static ring_buffer_t s_rx_ring;

/* 이전 DMA 위치 추적 */
static uint16_t s_rx_old_pos = 0;

/* ===== 초기화 ===== */

void uart_rx_dma_init(void)
{
    ring_buf_init(&s_rx_ring, s_app_rx_mem, APP_RX_BUF_SIZE);
    s_rx_old_pos = 0;

    /* ★ Circular DMA + IDLE 감지 수신 시작 ★ */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2,
                                  s_dma_rx_buf,
                                  DMA_RX_BUF_SIZE);

    /* Half-Transfer 인터럽트 비활성화 (IDLE만 사용) */
    __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);

    LOG_I("UART RX DMA 초기화 완료 (Circular + IDLE)");
}

/* ===== IDLE 감지 / DMA 완료 콜백 ===== */

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
            /* DMA 순환 발생: old_pos ~ 끝 + 0 ~ new_pos */
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

    LOG_D("RX 이벤트: pos=%d, ring_available=%d",
          new_pos, ring_buf_available(&s_rx_ring));
}

/* ===== 수신 데이터 읽기 (메인 루프에서 호출) ===== */

uint16_t uart_rx_available(void)
{
    return ring_buf_available(&s_rx_ring);
}

bool uart_rx_read_byte(uint8_t *byte)
{
    return ring_buf_read(&s_rx_ring, byte);
}
