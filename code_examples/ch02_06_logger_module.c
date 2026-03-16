/**
 * 파일명 : ch02_06_logger_module.c
 * 목  적 : logger.c 최종 통합 모듈 — log_print() 완성 구현
 * MCU   : STM32F411RE (NUCLEO-F411RE)
 * 버  전 : v0.1
 *
 * 설명:
 *   ch02_04_log_macro.h에서 선언한 log_print()를 구현한다.
 *   snprintf로 헤더와 사용자 메시지를 단일 버퍼에 조립한 후
 *   ITM_SendChar()로 한 글자씩 SWO로 전송한다.
 *
 *   출력 형식:
 *     [T=  1234ms][I][main.c:42][my_func    ] 사용자 메시지\n
 *      ^^^^^^^^^^  ^  ^^^^^^^^^  ^^^^^^^^^^^
 *      타임스탬프   레벨  파일:줄    함수명(최대12자)
 */

#include "ch02_04_log_macro.h"   /* log_print 선언 + LOG_LEVEL 상수 */
#include "stm32f4xx_hal.h"
#include <stdarg.h>              /* va_list, va_start, va_end */
#include <string.h>              /* strrchr */
#include <stdio.h>               /* snprintf, vsnprintf */

/* ---------------------------------------------------------------
 * 내부 버퍼 크기
 *   헤더(~48자) + 사용자 메시지(~200자) + 개행 + 여유
 * --------------------------------------------------------------- */
#define LOG_BUF_SIZE    256

/* ---------------------------------------------------------------
 * basename_portable — 경로에서 파일명만 추출
 *   "/path/to/main.c" → "main.c"
 *   플랫폼 basename()은 동작이 달라 직접 구현한다.
 * --------------------------------------------------------------- */
static const char *basename_portable(const char *path)
{
    const char *slash_fwd = strrchr(path, '/');   /* Unix 경로 구분자 */
    const char *slash_bwd = strrchr(path, '\\');  /* Windows 경로 구분자 */

    /* 두 구분자 중 더 오른쪽(뒤쪽)에 있는 것을 찾는다 */
    const char *last = (slash_fwd > slash_bwd) ? slash_fwd : slash_bwd;

    return (last != NULL) ? (last + 1) : path;   /* 구분자 없으면 원본 반환 */
}

/* ---------------------------------------------------------------
 * ITM_SendChar — SWO 전송 (ch02_01과 동일, 여기서 재사용)
 * --------------------------------------------------------------- */
static void itm_send_char(char ch)
{
    if ((ITM->TCR & ITM_TCR_ITMENA_Msk) &&
        (ITM->TER & (1UL << 0)))
    {
        while (ITM->PORT[0].u32 == 0) {}
        ITM->PORT[0].u8 = (uint8_t)ch;
    }
}

/* ---------------------------------------------------------------
 * log_print — log_print() 완성 구현
 *
 *   헤더 포맷: [T=%6lums][레벨][파일:줄][함수(최대12자)]
 *   이후 vsnprintf로 사용자 메시지를 버퍼 끝에 이어 붙인다.
 *   오버플로 방지: offset이 버퍼 끝 - 4 바이트를 넘으면 클램핑.
 * --------------------------------------------------------------- */
void log_print(int level, const char *tag,
               const char *file, int line, const char *func,
               const char *fmt, ...)
{
    /* 현재 활성 LOG_LEVEL보다 높으면 즉시 반환 */
    if (level > LOG_LEVEL)
    {
        return;
    }

    char buf[LOG_BUF_SIZE];
    int  offset = 0;

    /* 레벨 태그 배열 (LOG_LEVEL_* 상수 = 1..4 인덱스) */
    static const char *level_tag[] = {
        "-",    /* 0: NONE  (사용 안 함) */
        "E",    /* 1: ERROR */
        "W",    /* 2: WARN  */
        "I",    /* 3: INFO  */
        "D",    /* 4: DEBUG */
    };
    const char *ltag = (level >= 1 && level <= 4) ? level_tag[level] : "?";

    /* ── 헤더 생성 ──────────────────────────────────────────── */
    offset += snprintf(buf + offset,
                       (size_t)(LOG_BUF_SIZE - offset),
                       "[T=%6lums][%s][%s:%d][%.12s] ",
                       HAL_GetTick(),        /* 타임스탬프 (ms) */
                       ltag,                 /* 레벨 태그 (E/W/I/D) */
                       basename_portable(file), /* 파일명만 추출 */
                       line,                 /* 줄 번호 */
                       func);                /* 함수명 최대 12자 절단 */

    /* ── 오버플로 방지 클램핑 ─────────────────────────────── */
    /* 버퍼 끝에서 최소 4바이트(메시지 1자 + "\r\n" + NUL)는 확보 */
    if (offset >= (int)(sizeof(buf) - 4))
    {
        offset = (int)(sizeof(buf) - 4);
    }

    /* ── 사용자 메시지 이어 붙이기 ──────────────────────────── */
    va_list args;
    va_start(args, fmt);
    offset += vsnprintf(buf + offset,
                        (size_t)(LOG_BUF_SIZE - offset),
                        fmt, args);
    va_end(args);

    /* ── 다시 클램핑 (vsnprintf 후) ─────────────────────────── */
    if (offset >= (int)(sizeof(buf) - 3))
    {
        offset = (int)(sizeof(buf) - 3);
    }

    /* ── 개행 추가 ──────────────────────────────────────────── */
    buf[offset++] = '\r';
    buf[offset++] = '\n';
    buf[offset]   = '\0';

    /* ── ITM_SendChar 루프로 전송 ────────────────────────────── */
    for (int i = 0; i < offset; i++)
    {
        itm_send_char(buf[i]);
    }
}
