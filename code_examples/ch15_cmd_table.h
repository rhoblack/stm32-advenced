/**
 * @file    cmd_table.h
 * @brief   커맨드 테이블 패턴 — 함수 포인터 배열 기반 명령어 디스패처
 * @version v1.5
 */

#ifndef CMD_TABLE_H
#define CMD_TABLE_H

#include <stdint.h>

/* ===== 커맨드 핸들러 함수 포인터 타입 정의 ===== */
/**
 * @brief  커맨드 핸들러 함수 시그니처
 * @param  argc  인수 개수 (argv[0] = 명령어 이름 포함)
 * @param  argv  인수 배열
 * @return 0 = 성공, 음수 = 에러
 */
typedef int (*cmd_handler_t)(int argc, char *argv[]);

/* ===== 커맨드 엔트리 구조체 ===== */
typedef struct
{
    const char    *name;          /**< 명령어 이름 (예: "time") */
    const char    *usage;         /**< 사용법 (예: "time set HH:MM:SS | time get") */
    const char    *description;   /**< 한 줄 설명 */
    cmd_handler_t  handler;       /**< 핸들러 함수 포인터 */
} cmd_entry_t;

/* ===== 공개 API ===== */

/**
 * @brief  명령어 디스패치 — 커맨드 테이블에서 argv[0] 탐색 후 핸들러 호출
 * @param  argc  인수 개수
 * @param  argv  인수 배열
 */
void cmd_dispatch(int argc, char *argv[]);

/* ===== 개별 커맨드 핸들러 선언 ===== */
int cmd_time_handler(int argc, char *argv[]);
int cmd_alarm_handler(int argc, char *argv[]);
int cmd_motor_handler(int argc, char *argv[]);
int cmd_sensor_handler(int argc, char *argv[]);
int cmd_lcd_handler(int argc, char *argv[]);
int cmd_help_handler(int argc, char *argv[]);
int cmd_ver_handler(int argc, char *argv[]);

#endif /* CMD_TABLE_H */
