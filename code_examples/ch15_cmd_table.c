/**
 * @file    cmd_table.c
 * @brief   커맨드 테이블 구현 + 각 명령어 핸들러
 * @details 새 명령어 추가 시 핸들러 함수 + 테이블 엔트리 1개만 추가
 * @version v1.5
 */

#include "cmd_table.h"
#include "cli_app.h"
#include "log.h"

/* Service 레이어 헤더 — CLI 는 Service API 만 호출 */
#include "rtc_service.h"        /* Ch09 RTC 서비스 */
#include "clock_service.h"      /* Ch10 스텝모터 + 시계 서비스 */
#include "sensor_service.h"     /* Ch11 SHT31 센서 서비스 */
#include "ui_service.h"         /* Ch13 LCD UI 서비스 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ===== 커맨드 테이블 정의 ===== */
/* 새 명령어 추가: 이 배열에 엔트리 1개 추가, 핸들러 함수 구현 */
static const cmd_entry_t s_cmd_table[] =
{
    {
        "time",
        "time get | time set HH:MM:SS",
        "RTC 현재 시각 조회 또는 설정",
        cmd_time_handler
    },
    {
        "alarm",
        "alarm set HH:MM",
        "RTC 알람 시각 설정",
        cmd_alarm_handler
    },
    {
        "motor",
        "motor pos N",
        "스텝모터를 N 스텝만큼 이동 (양수=CW, 음수=CCW)",
        cmd_motor_handler
    },
    {
        "sensor",
        "sensor read",
        "SHT31 온습도 즉시 읽기",
        cmd_sensor_handler
    },
    {
        "lcd",
        "lcd clear",
        "TFT LCD 화면 전체 지우기",
        cmd_lcd_handler
    },
    {
        "help",
        "help",
        "사용 가능한 명령어 목록 출력",
        cmd_help_handler
    },
    {
        "ver",
        "ver",
        "펌웨어 버전 출력",
        cmd_ver_handler
    },
    /* 테이블 종료 마커 — 반드시 마지막에 위치 */
    { NULL, NULL, NULL, NULL }
};

/* ===== 디스패처 구현 ===== */

void cmd_dispatch(int argc, char *argv[])
{
    const cmd_entry_t *entry = s_cmd_table;

    LOG_D("cmd_dispatch: '%s' 탐색 시작", argv[0]);

    /* 테이블 선형 탐색 (NULL 종료 마커까지) */
    while (entry->name != NULL)
    {
        if (strcmp(entry->name, argv[0]) == 0)
        {
            LOG_D("커맨드 '%s' 발견 → 핸들러 호출", entry->name);
            int ret = entry->handler(argc, argv);
            if (ret != 0)
            {
                CLI_Printf(ANSI_RED "[ERR] 명령 실패 (ret=%d)\r\n"
                           "사용법: %s\r\n" ANSI_RESET,
                           ret, entry->usage);
                LOG_W("커맨드 '%s' 실패: ret=%d", entry->name, ret);
            }
            return;
        }
        entry++;
    }

    /* 미등록 명령어 */
    CLI_Printf(ANSI_RED "알 수 없는 명령어: '%s'\r\n" ANSI_RESET
               "도움말: 'help' 입력\r\n", argv[0]);
    LOG_W("미등록 명령어: '%s'", argv[0]);
}

/* ===== 개별 핸들러 구현 ===== */

int cmd_time_handler(int argc, char *argv[])
{
    if (argc < 2)
    {
        return -1;  /* 서브커맨드 없음 */
    }

    if (strcmp(argv[1], "get") == 0)
    {
        /* RTC 현재 시각 조회 */
        RTC_TimeTypeDef time;
        if (RTC_GetTime(&time) != HAL_OK)
        {
            LOG_E("RTC_GetTime 실패");
            return -2;
        }
        CLI_Printf(ANSI_GREEN "현재 시각: %02d:%02d:%02d\r\n" ANSI_RESET,
                   time.Hours, time.Minutes, time.Seconds);
        LOG_I("time get: %02d:%02d:%02d", time.Hours, time.Minutes, time.Seconds);
        return 0;
    }

    if (strcmp(argv[1], "set") == 0)
    {
        /* time set HH:MM:SS 파싱 */
        if (argc < 3)
        {
            return -1;
        }
        /* %d 사용: newlib-nano 환경에서 %hhu 지원이 불안정할 수 있음 */
        int h_i, m_i, s_i;
        if (sscanf(argv[2], "%d:%d:%d", &h_i, &m_i, &s_i) != 3)
        {
            CLI_Printf(ANSI_RED "형식 오류: HH:MM:SS 로 입력하세요\r\n" ANSI_RESET);
            return -3;
        }
        uint8_t h = (uint8_t)h_i;
        uint8_t m = (uint8_t)m_i;
        uint8_t s = (uint8_t)s_i;
        if (h > 23 || m > 59 || s > 59)
        {
            CLI_Printf(ANSI_RED "범위 오류: 시(0-23) 분(0-59) 초(0-59)\r\n" ANSI_RESET);
            return -4;
        }
        if (RTC_SetTime(h, m, s) != HAL_OK)
        {
            LOG_E("RTC_SetTime 실패");
            return -5;
        }
        CLI_Printf(ANSI_GREEN "시각 설정 완료: %02d:%02d:%02d\r\n" ANSI_RESET, h, m, s);
        LOG_I("time set: %02d:%02d:%02d", h, m, s);
        return 0;
    }

    return -1;  /* 알 수 없는 서브커맨드 */
}

int cmd_alarm_handler(int argc, char *argv[])
{
    if (argc < 3 || strcmp(argv[1], "set") != 0)
    {
        return -1;
    }

    int h_i, m_i;
    if (sscanf(argv[2], "%d:%d", &h_i, &m_i) != 2)
    {
        CLI_Printf(ANSI_RED "형식 오류: HH:MM 으로 입력하세요\r\n" ANSI_RESET);
        return -2;
    }
    uint8_t h = (uint8_t)h_i;
    uint8_t m = (uint8_t)m_i;
    if (h > 23 || m > 59)
    {
        CLI_Printf(ANSI_RED "범위 오류: 시(0-23) 분(0-59)\r\n" ANSI_RESET);
        return -3;
    }

    if (RTC_SetAlarm(h, m) != HAL_OK)
    {
        LOG_E("RTC_SetAlarm 실패");
        return -4;
    }

    CLI_Printf(ANSI_GREEN "알람 설정 완료: %02d:%02d\r\n" ANSI_RESET, h, m);
    LOG_I("alarm set: %02d:%02d", h, m);
    return 0;
}

int cmd_motor_handler(int argc, char *argv[])
{
    if (argc < 3 || strcmp(argv[1], "pos") != 0)
    {
        return -1;
    }

    int32_t steps = (int32_t)atoi(argv[2]);
    CLI_Printf(ANSI_YELLOW "스텝모터 이동: %ld 스텝 (%s)\r\n" ANSI_RESET,
               (long)steps, steps >= 0 ? "시계방향(CW)" : "반시계방향(CCW)");

    if (Motor_SetPosition(steps) != HAL_OK)
    {
        LOG_E("Motor_SetPosition 실패: steps=%ld", (long)steps);
        return -2;
    }

    LOG_I("motor pos: %ld 스텝 이동 완료", (long)steps);
    return 0;
}

int cmd_sensor_handler(int argc, char *argv[])
{
    if (argc < 2 || strcmp(argv[1], "read") != 0)
    {
        return -1;
    }

    float temp = 0.0f, humi = 0.0f;
    if (Sensor_ReadNow(&temp, &humi) != HAL_OK)
    {
        CLI_Printf(ANSI_RED "센서 읽기 실패 — SHT31 연결 확인\r\n" ANSI_RESET);
        LOG_E("Sensor_ReadNow 실패");
        return -2;
    }

    CLI_Printf(ANSI_GREEN "온도: %.1f°C  습도: %.1f%%\r\n" ANSI_RESET, temp, humi);
    LOG_I("sensor read: T=%.1f H=%.1f", temp, humi);
    return 0;
}

int cmd_lcd_handler(int argc, char *argv[])
{
    if (argc < 2 || strcmp(argv[1], "clear") != 0)
    {
        return -1;
    }

    LCD_ClearScreen();
    CLI_Printf(ANSI_GREEN "LCD 화면 초기화 완료\r\n" ANSI_RESET);
    LOG_I("lcd clear 실행");
    return 0;
}

int cmd_help_handler(int argc, char *argv[])
{
    (void)argc; (void)argv;  /* 미사용 파라미터 경고 억제 */

    CLI_Printf(ANSI_BOLD ANSI_CYAN
               "\r\n=== 사용 가능한 명령어 ===\r\n" ANSI_RESET);

    const cmd_entry_t *entry = s_cmd_table;
    while (entry->name != NULL)
    {
        CLI_Printf("  " ANSI_YELLOW "%-12s" ANSI_RESET
                   " %s\r\n    → %s\r\n\r\n",
                   entry->name,
                   entry->usage,
                   entry->description);
        entry++;
    }
    return 0;
}

int cmd_ver_handler(int argc, char *argv[])
{
    (void)argc; (void)argv;
    CLI_Printf(ANSI_GREEN "STM32 Smart Clock v1.5 (STM32F411RE)\r\n"
               "Build: " __DATE__ " " __TIME__ "\r\n" ANSI_RESET);
    LOG_I("ver 명령 실행");
    return 0;
}
