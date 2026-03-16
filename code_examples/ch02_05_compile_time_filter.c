/**
 * 파일명 : ch02_05_compile_time_filter.c
 * 목  적 : 컴파일 타임 레벨 필터링 동작 데모
 * MCU   : STM32F411RE (NUCLEO-F411RE)
 * 버  전 : v0.1
 *
 * 설명:
 *   LOG_LEVEL을 빌드 시스템에서 주입하면 전처리기가
 *   비활성 레벨의 로그 코드를 완전히 제거한다.
 *   런타임 분기(if)가 없으므로 성능·코드 크기 모두 최적이다.
 *
 * ┌─────────────────────────────────────────────────────────────┐
 * │  CubeIDE에서 Preprocessor Define 설정하는 방법              │
 * │                                                             │
 * │  Project → Properties                                       │
 * │    → C/C++ Build → Settings                                 │
 * │    → Tool Settings → MCU GCC Compiler                      │
 * │    → Preprocessor → Defined symbols (-D)                   │
 * │                                                             │
 * │  추가할 심볼:  LOG_LEVEL=4   (DEBUG 빌드)                   │
 * │               LOG_LEVEL=3   (INFO  빌드)                   │
 * │               LOG_LEVEL=1   (ERROR 빌드)                   │
 * │               LOG_LEVEL=0   (출력 없음)                     │
 * └─────────────────────────────────────────────────────────────┘
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>

/* LOG_LEVEL은 빌드 시스템에서 주입된다. 없으면 INFO(3)로 설정. */
#ifndef LOG_LEVEL
    #define LOG_LEVEL   3
#endif

/* ===============================================================
 * 전처리기가 LOG_LEVEL을 평가하여 아래 #if 블록 중
 * 하나만 컴파일 대상에 포함시킨다.
 * =============================================================== */

/* ---------------------------------------------------------------
 * [빌드 구성 1] LOG_LEVEL=0 : 로그 전혀 없음 (릴리스/최종 양산)
 * --------------------------------------------------------------- */
#if (LOG_LEVEL == 0)
    #define LOG_E(fmt, ...)     /* 아무것도 생성하지 않음 */
    #define LOG_W(fmt, ...)
    #define LOG_I(fmt, ...)
    #define LOG_D(fmt, ...)
    #pragma message("LOG: 모든 로그 비활성화 (NONE 빌드)")

/* ---------------------------------------------------------------
 * [빌드 구성 2] LOG_LEVEL=1 : ERROR만 출력 (현장 배포)
 * --------------------------------------------------------------- */
#elif (LOG_LEVEL == 1)
    #define LOG_E(fmt, ...)  printf("[E] " fmt "\r\n", ##__VA_ARGS__)
    #define LOG_W(fmt, ...)
    #define LOG_I(fmt, ...)
    #define LOG_D(fmt, ...)
    #pragma message("LOG: ERROR 레벨 빌드")

/* ---------------------------------------------------------------
 * [빌드 구성 3] LOG_LEVEL=3 : INFO 이상 출력 (일반 개발)
 * --------------------------------------------------------------- */
#elif (LOG_LEVEL == 3)
    #define LOG_E(fmt, ...)  printf("[E] " fmt "\r\n", ##__VA_ARGS__)
    #define LOG_W(fmt, ...)  printf("[W] " fmt "\r\n", ##__VA_ARGS__)
    #define LOG_I(fmt, ...)  printf("[I] " fmt "\r\n", ##__VA_ARGS__)
    #define LOG_D(fmt, ...)
    #pragma message("LOG: INFO 레벨 빌드")

/* ---------------------------------------------------------------
 * [빌드 구성 4] LOG_LEVEL=4 : 모든 로그 출력 (상세 디버깅)
 * --------------------------------------------------------------- */
#elif (LOG_LEVEL >= 4)
    #define LOG_E(fmt, ...)  printf("[E] " fmt "\r\n", ##__VA_ARGS__)
    #define LOG_W(fmt, ...)  printf("[W] " fmt "\r\n", ##__VA_ARGS__)
    #define LOG_I(fmt, ...)  printf("[I] " fmt "\r\n", ##__VA_ARGS__)
    #define LOG_D(fmt, ...)  printf("[D] " fmt "\r\n", ##__VA_ARGS__)
    #pragma message("LOG: DEBUG 레벨 빌드")

#endif  /* LOG_LEVEL 선택 완료 */

/* ---------------------------------------------------------------
 * 데모 함수 — 4개 레벨 로그를 모두 호출한다.
 * 빌드된 LOG_LEVEL에 따라 실제로 출력되는 줄 수가 달라진다.
 * --------------------------------------------------------------- */
void ch02_05_demo(void)
{
    /* 현재 LOG_LEVEL을 런타임에서도 확인할 수 있도록 출력 */
    printf("=== 컴파일 타임 필터링 데모 (LOG_LEVEL=%d) ===\r\n", LOG_LEVEL);

    LOG_E("전원 이상 감지 — 이 줄은 항상(LEVEL>=1) 출력된다");
    LOG_W("UART FIFO 경고 — LEVEL>=2 일 때만 출력된다");
    LOG_I("초기화 완료   — LEVEL>=3 일 때만 출력된다");
    LOG_D("루프 카운터=0 — LEVEL>=4 일 때만 출력된다");

    printf("=== 데모 종료 ===\r\n");
}
