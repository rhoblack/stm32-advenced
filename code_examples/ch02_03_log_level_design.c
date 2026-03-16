/**
 * 파일명 : ch02_03_log_level_design.c
 * 목  적 : 4단계 로그 레벨 개념 증명 (매크로 없이 단순 구현)
 * MCU   : STM32F411RE (NUCLEO-F411RE)
 * 버  전 : v0.1
 *
 * 설명:
 *   로그 레벨(ERROR / WARN / INFO / DEBUG)의 개념과 필터링 동작을
 *   매크로 없이 직접 코드로 표현한다.
 *   "왜 레벨이 필요한가"를 이해하는 데 집중한 개념 증명용 코드이다.
 *
 *   레벨 값이 낮을수록 심각도가 높다:
 *     0=ERROR  1=WARN  2=INFO  3=DEBUG
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>

/* ---------------------------------------------------------------
 * 로그 레벨 상수 정의
 * --------------------------------------------------------------- */
#define LEVEL_ERROR     0   /* 치명적 오류 — 반드시 확인해야 한다 */
#define LEVEL_WARN      1   /* 경고 — 즉시 문제는 아니지만 주의 필요 */
#define LEVEL_INFO      2   /* 정보 — 정상 동작 흐름 확인 */
#define LEVEL_DEBUG     3   /* 디버그 — 상세 내부 상태 */

/* ---------------------------------------------------------------
 * 현재 활성 레벨 — 이 값보다 높은 레벨 메시지는 출력하지 않는다
 * (나중에 ch02_05에서 컴파일 타임 상수로 전환한다)
 * --------------------------------------------------------------- */
static int g_log_level = LEVEL_DEBUG;   /* 기본: 모든 레벨 출력 */

/* ---------------------------------------------------------------
 * 레벨 태그 문자열 배열
 * --------------------------------------------------------------- */
static const char *level_name[] = {
    "ERROR",   /* 0 */
    "WARN ",   /* 1 */
    "INFO ",   /* 2 */
    "DEBUG",   /* 3 */
};

/* ---------------------------------------------------------------
 * log_raw — 단순 레벨 필터링 출력 함수
 *   level    : 메시지의 심각도 레벨
 *   message  : 출력할 문자열
 * --------------------------------------------------------------- */
static void log_raw(int level, const char *message)
{
    /* 현재 활성 레벨보다 높은(덜 중요한) 메시지는 버린다 */
    if (level > g_log_level)
    {
        return;
    }

    /* HAL_GetTick()으로 ms 단위 타임스탬프 삽입 */
    uint32_t ts = HAL_GetTick();
    printf("[%6lu ms][%s] %s\r\n", ts, level_name[level], message);
}

/* ---------------------------------------------------------------
 * 레벨 필터링 동작 데모
 * --------------------------------------------------------------- */
void ch02_03_demo(void)
{
    printf("--- g_log_level = DEBUG (모두 출력) ---\r\n");
    g_log_level = LEVEL_DEBUG;

    log_raw(LEVEL_ERROR, "전원 전압 이상 감지!");          /* 출력됨 */
    log_raw(LEVEL_WARN,  "버퍼 사용률 80% 초과");          /* 출력됨 */
    log_raw(LEVEL_INFO,  "GPIO 초기화 완료");               /* 출력됨 */
    log_raw(LEVEL_DEBUG, "루프 카운터 = 42");               /* 출력됨 */

    printf("\r\n--- g_log_level = INFO (DEBUG 제외) ---\r\n");
    g_log_level = LEVEL_INFO;

    log_raw(LEVEL_ERROR, "전원 전압 이상 감지!");          /* 출력됨 */
    log_raw(LEVEL_WARN,  "버퍼 사용률 80% 초과");          /* 출력됨 */
    log_raw(LEVEL_INFO,  "GPIO 초기화 완료");               /* 출력됨 */
    log_raw(LEVEL_DEBUG, "루프 카운터 = 42");               /* 필터됨! */

    printf("\r\n--- g_log_level = ERROR (ERROR만 출력) ---\r\n");
    g_log_level = LEVEL_ERROR;

    log_raw(LEVEL_ERROR, "전원 전압 이상 감지!");          /* 출력됨 */
    log_raw(LEVEL_WARN,  "버퍼 사용률 80% 초과");          /* 필터됨! */
    log_raw(LEVEL_INFO,  "GPIO 초기화 완료");               /* 필터됨! */
    log_raw(LEVEL_DEBUG, "루프 카운터 = 42");               /* 필터됨! */
}
