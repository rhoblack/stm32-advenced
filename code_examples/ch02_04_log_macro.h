/**
 * 파일명 : ch02_04_log_macro.h
 * 목  적 : LOG_E/W/I/D 매크로 완성 구현 헤더
 * MCU   : STM32F411RE (NUCLEO-F411RE)
 * 버  전 : v0.1
 *
 * 설명:
 *   컴파일 타임에 로그 레벨을 선택할 수 있는 매크로 기반 로거.
 *   불필요한 로그 코드는 전처리기 단계에서 완전히 제거되므로
 *   릴리스 빌드에서 CPU 사이클과 코드 크기를 절약한다.
 *
 * 사용 예:
 *   #define LOG_LEVEL LOG_LEVEL_DEBUG    // 헤더 포함 전에 정의
 *   #include "ch02_04_log_macro.h"
 *
 *   LOG_I("초기화 완료 tick=%lu", HAL_GetTick());
 *   LOG_E("오류 코드: %d", err);
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>   /* uint32_t */

/* ===============================================================
 * 로그 레벨 상수 — 숫자가 클수록 상세(덜 심각)
 * =============================================================== */
#define LOG_LEVEL_NONE      0   /* 모든 로그 비활성화 */
#define LOG_LEVEL_ERROR     1   /* 치명적 오류만 출력 */
#define LOG_LEVEL_WARN      2   /* 경고 이상 출력 */
#define LOG_LEVEL_INFO      3   /* 정보 이상 출력 */
#define LOG_LEVEL_DEBUG     4   /* 모든 로그 출력 */

/* ---------------------------------------------------------------
 * LOG_LEVEL 기본값 설정
 *   빌드 시스템(CubeIDE Preprocessor)에서 재정의하지 않은 경우
 *   INFO 레벨로 동작한다.
 * --------------------------------------------------------------- */
#ifndef LOG_LEVEL
    #define LOG_LEVEL   LOG_LEVEL_INFO
#endif

/* ===============================================================
 * log_print() 함수 선언
 *   logger.c(ch02_06)에서 구현한다.
 *   직접 호출하지 않고 아래 매크로를 통해 사용한다.
 *
 *   level    : LOG_LEVEL_* 상수
 *   tag      : 레벨 문자열 ("E"/"W"/"I"/"D")
 *   file     : 소스 파일 경로 (__FILE__)
 *   line     : 소스 줄 번호 (__LINE__)
 *   func     : 함수 이름 (__func__)
 *   fmt, ... : printf 형식 문자열과 인자
 * =============================================================== */
void log_print(int level, const char *tag,
               const char *file, int line, const char *func,
               const char *fmt, ...);

/* ===============================================================
 * 로그 매크로 — 조건부 컴파일로 비활성 레벨은 코드 크기 0
 *
 *   ##__VA_ARGS__ : GNU 확장 — 가변 인자가 없을 때 앞쪽 쉼표 제거
 *   __FILE__      : 전처리기가 현재 파일 경로로 치환
 *   __LINE__      : 전처리기가 현재 줄 번호(정수)로 치환
 *   __func__      : 컴파일러가 현재 함수 이름 문자열로 치환
 * =============================================================== */

/* ERROR 매크로 */
#if (LOG_LEVEL >= LOG_LEVEL_ERROR)
    #define LOG_E(fmt, ...) \
        log_print(LOG_LEVEL_ERROR, "E", __FILE__, __LINE__, __func__, \
                  fmt, ##__VA_ARGS__)
#else
    #define LOG_E(fmt, ...)     /* 컴파일 시 완전히 제거됨 */
#endif

/* WARN 매크로 */
#if (LOG_LEVEL >= LOG_LEVEL_WARN)
    #define LOG_W(fmt, ...) \
        log_print(LOG_LEVEL_WARN, "W", __FILE__, __LINE__, __func__, \
                  fmt, ##__VA_ARGS__)
#else
    #define LOG_W(fmt, ...)
#endif

/* INFO 매크로 */
#if (LOG_LEVEL >= LOG_LEVEL_INFO)
    #define LOG_I(fmt, ...) \
        log_print(LOG_LEVEL_INFO, "I", __FILE__, __LINE__, __func__, \
                  fmt, ##__VA_ARGS__)
#else
    #define LOG_I(fmt, ...)
#endif

/* DEBUG 매크로 */
#if (LOG_LEVEL >= LOG_LEVEL_DEBUG)
    #define LOG_D(fmt, ...) \
        log_print(LOG_LEVEL_DEBUG, "D", __FILE__, __LINE__, __func__, \
                  fmt, ##__VA_ARGS__)
#else
    #define LOG_D(fmt, ...)
#endif

#endif /* LOGGER_H */
