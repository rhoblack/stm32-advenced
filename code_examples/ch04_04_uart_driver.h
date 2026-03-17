/* ch04_04_uart_driver.h — UART Driver 공개 인터페이스
 *
 * 레이어: Driver
 * 위치: 프로젝트 아키텍처 v0.4
 *
 * 이 인터페이스는 Ch06(DMA 전환)까지 변경 없이 유지됩니다.
 * 내부 구현만 폴링 → 인터럽트 → DMA로 교체됩니다.
 *
 * 상위 레이어(App)는 이 헤더만 include합니다.
 * HAL 헤더를 직접 include하지 않습니다.
 */

#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>

/* ===== 반환 코드 ===== */
typedef enum {
    UART_OK      = 0,   /* 성공 */
    UART_ERROR   = 1,   /* 일반 오류 */
    UART_BUSY    = 2,   /* 송신 중 (비동기 모드) */
    UART_TIMEOUT = 3    /* 타임아웃 */
} uart_status_t;

/* ===== 초기화 ===== */

/**
 * @brief UART 드라이버 초기화
 * @note  CubeMX에서 USART2 설정 후 호출
 *        내부적으로 인터럽트 수신 등록 포함
 */
void uart_driver_init(void);

/* ===== 송신 ===== */

/**
 * @brief 바이트 배열 송신
 * @param buf  송신할 데이터 포인터
 * @param len  송신 바이트 수
 * @return UART_OK 성공, UART_ERROR 실패
 */
uart_status_t uart_send(const uint8_t *buf, uint16_t len);

/**
 * @brief 문자열 송신 (null-terminated)
 * @param str  송신할 문자열
 * @return UART_OK 성공, UART_ERROR 실패
 */
uart_status_t uart_send_string(const char *str);

/* ===== 수신 ===== */

/**
 * @brief 수신 버퍼에 읽을 수 있는 바이트 수
 * @return 0이면 수신 데이터 없음
 */
uint16_t uart_available(void);

/**
 * @brief 수신 버퍼에서 1바이트 읽기 (비블로킹)
 * @return 읽은 바이트. 버퍼가 비어있으면 0
 */
uint8_t uart_read_byte(void);

#endif /* UART_DRIVER_H */
