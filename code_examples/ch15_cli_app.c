/**
 * @file    cli_app.c
 * @brief   CLI 애플리케이션 구현 — App 레이어
 * @details 링 버퍼에서 문자를 꺼내 라인 버퍼 조립 → 파서 → 디스패처
 * @version v1.5
 */

#include "cli_app.h"
#include "cmd_table.h"
#include "log.h"            /* Ch02 로그 시스템 */
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ===== 내부 전역 변수 ===== */
static ring_buf_t       *s_rx_buf;              /**< Ch06 Ring Buffer 참조 */
static UART_HandleTypeDef *s_huart;             /**< 응답 전송 UART */
static char             s_line_buf[CLI_LINE_MAX]; /**< 라인 조립 버퍼 */
static uint16_t         s_line_len;             /**< 현재 라인 버퍼 길이 */

/* ===== 내부 함수 선언 ===== */
static void cli_parse_and_dispatch(char *line);
static void cli_echo_char(char c);

/* ===== 공개 함수 구현 ===== */

void CLI_Init(ring_buf_t *rx_buf, UART_HandleTypeDef *huart)
{
    LOG_D("CLI_Init 진입");

    s_rx_buf   = rx_buf;
    s_huart    = huart;
    s_line_len = 0;
    memset(s_line_buf, 0, sizeof(s_line_buf));

    /* 시작 배너 출력 */
    CLI_Printf("\r\n");
    CLI_Printf(ANSI_BOLD ANSI_CYAN
               "=== STM32 Smart Clock CLI v1.5 ===\r\n" ANSI_RESET);
    CLI_Printf("'help' 입력 후 Enter 로 명령어 목록 확인\r\n\r\n");
    CLI_Printf(CLI_PROMPT);

    LOG_I("CLI 초기화 완료 — Ring Buffer 연결됨");
}

void CLI_Process(void)
{
    uint8_t byte;

    /* Ring Buffer 에서 모든 바이트를 처리 */
    while (ring_buf_pop(s_rx_buf, &byte) == RING_BUF_OK)
    {
        /* 백스페이스 처리 */
        if (byte == '\b' || byte == 0x7F)
        {
            if (s_line_len > 0)
            {
                s_line_len--;
                s_line_buf[s_line_len] = '\0';
                /* 터미널에서 문자 지우기: BS + Space + BS */
                CLI_Printf("\b \b");
            }
            continue;
        }

        /* 개행 감지 — 라인 완성 */
        if (byte == '\n' || byte == '\r')
        {
            if (s_line_len == 0)
            {
                /* 빈 줄 → 프롬프트만 재출력 */
                CLI_Printf("\r\n" CLI_PROMPT);
                continue;
            }

            s_line_buf[s_line_len] = '\0';
            CLI_Printf("\r\n");

            LOG_D("CLI 라인 수신: '%s' (len=%d)", s_line_buf, s_line_len);

            /* 파서 + 디스패처 실행 */
            cli_parse_and_dispatch(s_line_buf);

            /* 라인 버퍼 초기화 후 프롬프트 재출력 */
            s_line_len = 0;
            memset(s_line_buf, 0, sizeof(s_line_buf));
            CLI_Printf(CLI_PROMPT);
            continue;
        }

        /* 일반 문자: 에코 + 라인 버퍼에 추가 */
        if (s_line_len < CLI_LINE_MAX - 1)
        {
            s_line_buf[s_line_len++] = (char)byte;
            cli_echo_char((char)byte);
        }
        else
        {
            LOG_W("CLI 라인 버퍼 초과 — 입력 무시");
        }
    }
}

void CLI_Printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len > 0)
    {
        /* Ch04 UART Driver 사용: DMA 전송 */
        HAL_UART_Transmit(s_huart, (uint8_t *)buf, (uint16_t)len, 100);
    }
}

/* ===== 내부 함수 구현 ===== */

/**
 * @brief  라인 문자열을 파싱하여 커맨드 디스패처로 전달
 * @param  line  null-종료 라인 문자열 (수정됨 — strtok_r 가 '\0' 삽입)
 */
static void cli_parse_and_dispatch(char *line)
{
    char  *argv[CLI_ARGV_MAX];
    int    argc = 0;
    char  *saveptr;                    /* strtok_r 상태 포인터 (재진입 안전) */
    char  *token;

    /* strtok_r 로 공백 기준 토큰화 */
    token = strtok_r(line, " \t", &saveptr);
    while (token != NULL && argc < CLI_ARGV_MAX)
    {
        argv[argc++] = token;
        token = strtok_r(NULL, " \t", &saveptr);
    }

    if (argc == 0)
    {
        return;   /* 공백만 입력한 경우 */
    }

    LOG_D("CLI 파싱 완료: argc=%d, argv[0]='%s'", argc, argv[0]);

    /* 커맨드 테이블에 디스패치 */
    cmd_dispatch(argc, argv);
}

/**
 * @brief  단일 문자를 터미널에 에코
 */
static void cli_echo_char(char c)
{
    HAL_UART_Transmit(s_huart, (uint8_t *)&c, 1, 10);
}
