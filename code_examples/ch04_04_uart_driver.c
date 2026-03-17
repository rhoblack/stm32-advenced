/* ch04_04_uart_driver.c — UART Driver 구현 (인터럽트 수신 모드)
 *
 * 레이어: Driver
 * 의존: HAL (stm32f4xx_hal_uart.h) — 하위 레이어만 참조
 *
 * 이 파일은 HAL 상세를 캡슐화합니다.
 * 상위 레이어(App)는 uart_driver.h의 인터페이스만 사용합니다.
 *
 * CubeMX 설정:
 *   - USART2: Asynchronous, 115200-8-N-1
 *   - PA2 = USART2_TX, PA3 = USART2_RX
 *   - NVIC: USART2 global interrupt → Enable
 */

#include "uart_driver.h"
#include "main.h"       /* HAL, GPIO 정의 */
#include "log.h"        /* LOG_D/I/W/E */
#include <string.h>

/* ===== HAL 핸들 (CubeMX 생성) ===== */
extern UART_HandleTypeDef huart2;

/* ===== 내부 수신 버퍼 ===== */
#define RX_BUF_SIZE  128

static volatile uint8_t  s_rx_byte;              /* ISR 수신 버퍼 */
static volatile uint8_t  s_rx_buf[RX_BUF_SIZE];  /* 링 버퍼 (간이) */
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

/* ===== 초기화 ===== */

void uart_driver_init(void)
{
    /* CubeMX가 MX_USART2_UART_Init()에서 이미 초기화함.
     * 여기서는 인터럽트 수신만 등록 */
    s_rx_head = 0;
    s_rx_tail = 0;

    HAL_UART_Receive_IT(&huart2,
                        (uint8_t *)&s_rx_byte, 1);

    LOG_I("uart_driver 초기화 완료 (115200-8-N-1)");
}

/* ===== 송신 ===== */

uart_status_t uart_send(const uint8_t *buf, uint16_t len)
{
    if (buf == NULL || len == 0) {
        LOG_W("uart_send: 잘못된 파라미터");
        return UART_ERROR;
    }

    HAL_StatusTypeDef hal_status;
    hal_status = HAL_UART_Transmit(&huart2,
                                   (uint8_t *)buf, len,
                                   100);
    if (hal_status != HAL_OK) {
        LOG_W("uart_send 실패: hal=%d", hal_status);
        return UART_ERROR;
    }

    LOG_D("uart_send: %d바이트 전송 완료", len);
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

/* ===== 수신 ===== */

uint16_t uart_available(void)
{
    return (s_rx_head - s_rx_tail + RX_BUF_SIZE)
           % RX_BUF_SIZE;
}

uint8_t uart_read_byte(void)
{
    if (s_rx_head == s_rx_tail) {
        return 0;  /* 버퍼 비어있음 */
    }
    uint8_t byte = s_rx_buf[s_rx_tail];
    s_rx_tail = (s_rx_tail + 1) % RX_BUF_SIZE;
    return byte;
}

/* ===== HAL 콜백 (인터럽트 수신) ===== */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART2) {
        return;
    }

    /* 1) 수신 바이트를 링 버퍼에 저장 */
    uint16_t next = (s_rx_head + 1) % RX_BUF_SIZE;
    if (next != s_rx_tail) {
        s_rx_buf[s_rx_head] = s_rx_byte;
        s_rx_head = next;
    } else {
        /* 버퍼 가득 참 — Ch06 링 버퍼로 개선 */
        LOG_W("RX 버퍼 오버플로");
    }

    /* 2) ★ 다음 1바이트 수신 재등록 ★ */
    HAL_UART_Receive_IT(&huart2,
                        (uint8_t *)&s_rx_byte, 1);
}

/* ===== printf 리다이렉트 ===== */

int _write(int file, char *ptr, int len)
{
    (void)file;
    HAL_UART_Transmit(&huart2,
                      (uint8_t *)ptr, (uint16_t)len,
                      100);
    return len;
}
