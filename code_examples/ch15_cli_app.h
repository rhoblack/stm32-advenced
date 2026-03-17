/**
 * @file    cli_app.h
 * @brief   CLI (Command Line Interface) 애플리케이션 — App 레이어
 * @details 링 버퍼 기반 비동기 UART 입력 처리 + 커맨드 테이블 디스패치
 *          Ch06 ring_buf_t 재사용, Ch04 UART Driver 위에 동작
 * @version v1.5
 */

#ifndef CLI_APP_H
#define CLI_APP_H

#include <stdint.h>
#include "ring_buf.h"           /* Ch06 링 버퍼 — 그대로 재사용 */
#include "stm32f4xx_hal.h"

/* ===== CLI 설정 상수 ===== */
#define CLI_LINE_MAX        128     /**< 최대 명령줄 길이 (바이트) */
#define CLI_ARGV_MAX        8       /**< 최대 인수 개수 */
#define CLI_PROMPT          "\033[1;36mSTM32>\033[0m "  /**< ANSI 프롬프트 */

/* ===== ANSI 이스케이프 코드 색상 정의 ===== */
#define ANSI_RED            "\033[31m"
#define ANSI_GREEN          "\033[32m"
#define ANSI_YELLOW         "\033[33m"
#define ANSI_CYAN           "\033[36m"
#define ANSI_BOLD           "\033[1m"
#define ANSI_RESET          "\033[0m"

/* ===== 공개 API ===== */

/**
 * @brief  CLI 초기화
 * @param  rx_buf  Ch06 Ring Buffer 포인터 (UART DMA RX 가 채워 넣음)
 * @param  huart   응답 전송에 사용할 UART 핸들
 */
void CLI_Init(ring_buf_t *rx_buf, UART_HandleTypeDef *huart);

/**
 * @brief  CLI 메인 폴링 함수 — 슈퍼루프(super-loop) 에서 주기적 호출
 * @note   라인 버퍼가 완성되면(개행 감지) 파서 + 디스패처 실행
 */
void CLI_Process(void);

/**
 * @brief  포맷된 문자열을 UART로 전송 (printf 스타일)
 * @param  fmt  printf 포맷 문자열
 */
void CLI_Printf(const char *fmt, ...);

#endif /* CLI_APP_H */
