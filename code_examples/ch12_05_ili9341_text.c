/**
 * ch12_05_ili9341_text.c
 * ILI9341 텍스트 출력 (비트맵 폰트)
 *
 * 5×7 ASCII 폰트 데이터를 사용하여 화면에 문자를 출력합니다.
 * 지원: ASCII 32~126 (영문, 숫자, 기호)
 * 미지원: 한글 (메모리 부족, Ch13 예정)
 */

#include "main.h"
#include "log_system.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

extern SPI_HandleTypeDef hspi1;

#define LCD_WIDTH   320
#define LCD_HEIGHT  240

#define FONT_WIDTH   5   /* 5픽셀 너비 */
#define FONT_HEIGHT  7   /* 7픽셀 높이 */

/* 외부 함수 (ch12_03_ili9341_pixel_write.c) */
extern void LCD_Draw_Pixel(uint16_t x, uint16_t y, uint16_t color);

/* ===== 5×7 ASCII 폰트 비트맵 데이터 ===== */

/**
 * @brief 5×7 ASCII 폰트 데이터
 *
 * 각 문자는 7바이트로 구성됩니다. (높이 7픽셀)
 * 각 바이트는 5픽셀의 가로 데이터를 가집니다.
 * 비트가 1이면 전경색, 0이면 배경색입니다.
 *
 * 예: '0' (ASCII 48)
 *   Byte 0: 0b00011100 (상단)
 *   Byte 1: 0b00100010
 *   ...
 *   Byte 6: 0b00011100 (하단)
 */
static const uint8_t font5x7[][FONT_HEIGHT] = {
    /* ASCII 32: 공백 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

    /* ASCII 33: '!' */
    {0x00, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04},

    /* ASCII 34: '"' */
    {0x00, 0x0A, 0x0A, 0x0A, 0x00, 0x00, 0x00},

    /* ASCII 35: '#' */
    {0x00, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x00},

    /* ASCII 36: '$' */
    {0x04, 0x0E, 0x14, 0x0E, 0x05, 0x0E, 0x04},

    /* ASCII 37: '%' */
    {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03},

    /* ASCII 38: '&' */
    {0x0C, 0x12, 0x12, 0x0C, 0x12, 0x12, 0x0D},

    /* ASCII 39: ''' */
    {0x00, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00},

    /* ASCII 40: '(' */
    {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02},

    /* ASCII 41: ')' */
    {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08},

    /* ASCII 42: '*' */
    {0x04, 0x15, 0x0E, 0x04, 0x0E, 0x15, 0x04},

    /* ASCII 43: '+' */
    {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00},

    /* ASCII 44: ',' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08},

    /* ASCII 45: '-' */
    {0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00},

    /* ASCII 46: '.' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04},

    /* ASCII 47: '/' */
    {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00},

    /* ASCII 48: '0' */
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},

    /* ASCII 49: '1' */
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},

    /* ASCII 50: '2' */
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},

    /* ASCII 51: '3' */
    {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E},

    /* ASCII 52: '4' */
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},

    /* ASCII 53: '5' */
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},

    /* ASCII 54: '6' */
    {0x0E, 0x11, 0x10, 0x1E, 0x11, 0x11, 0x0E},

    /* ASCII 55: '7' */
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10},

    /* ASCII 56: '8' */
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},

    /* ASCII 57: '9' */
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x11, 0x0E},

    /* ASCII 58: ':' */
    {0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00},

    /* ASCII 59: ';' */
    {0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x08},

    /* ASCII 60: '<' */
    {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02},

    /* ASCII 61: '=' */
    {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00},

    /* ASCII 62: '>' */
    {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08},

    /* ASCII 63: '?' */
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04},

    /* ASCII 64: '@' */
    {0x0E, 0x11, 0x17, 0x15, 0x17, 0x10, 0x0E},

    /* ASCII 65: 'A' */
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},

    /* ASCII 66: 'B' */
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},

    /* ASCII 67: 'C' */
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},

    /* ASCII 68: 'D' */
    {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E},

    /* ASCII 69: 'E' */
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},

    /* ASCII 70: 'F' */
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},

    /* ASCII 71: 'G' */
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F},

    /* ASCII 72: 'H' */
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},

    /* ASCII 73: 'I' */
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},

    /* ASCII 74: 'J' */
    {0x01, 0x01, 0x01, 0x01, 0x01, 0x11, 0x0E},

    /* ASCII 75: 'K' */
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},

    /* ASCII 76: 'L' */
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},

    /* ASCII 77: 'M' */
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},

    /* ASCII 78: 'N' */
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},

    /* ASCII 79: 'O' */
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},

    /* ASCII 80: 'P' */
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},

    /* ASCII 81: 'Q' */
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},

    /* ASCII 82: 'R' */
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},

    /* ASCII 83: 'S' */
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E},

    /* ASCII 84: 'T' */
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},

    /* ASCII 85: 'U' */
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},

    /* ASCII 86: 'V' */
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},

    /* ASCII 87: 'W' */
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11},

    /* ASCII 88: 'X' */
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},

    /* ASCII 89: 'Y' */
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},

    /* ASCII 90: 'Z' */
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},

    /* ASCII 91: '[' */
    {0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E},

    /* ASCII 92: '\' */
    {0x10, 0x10, 0x08, 0x04, 0x02, 0x01, 0x01},

    /* ASCII 93: ']' */
    {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E},

    /* ASCII 94: '^' */
    {0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00},

    /* ASCII 95: '_' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F},

    /* ASCII 96: '`' */
    {0x08, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00},

    /* ASCII 97: 'a' (소문자 - 여기서는 대문자 복사) */
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},

    /* ... 더 많은 문자들 (대문자와 동일하게 처리) ... */
    /* 실제 구현에서는 소문자 폰트 데이터를 추가하세요 */

    /* 나머지 ASCII 문자들... (이 예제에서는 생략) */
};

/**
 * @brief 개별 문자 출력
 *
 * @param x, y: 문자의 좌상단 좌표
 * @param ch: 문자 (ASCII 32~126)
 * @param color: RGB565 전경색
 * @param bg_color: RGB565 배경색
 *
 * 예제:
 *   LCD_Draw_Char(100, 100, 'A', COLOR_WHITE, COLOR_BLACK);
 */
void LCD_Draw_Char(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color)
{
    /* ASCII 범위 확인 */
    if (ch < 32 || ch > 126) {
        LOG_W("지원하지 않는 문자: 0x%02X", ch);
        return;
    }

    /* 폰트 데이터 포인터 */
    const uint8_t *font_data = font5x7[ch - 32];

    /* 각 행(높이) 처리 */
    for (uint8_t row = 0; row < FONT_HEIGHT; row++) {
        uint8_t byte_val = font_data[row];

        /* 각 열(너비) 처리 */
        for (uint8_t col = 0; col < FONT_WIDTH; col++) {
            uint16_t px = x + col;
            uint16_t py = y + row;

            /* 범위 체크 */
            if (px >= LCD_WIDTH || py >= LCD_HEIGHT) continue;

            /* 비트를 확인하고 픽셀 색상 선택 */
            if (byte_val & (0x80 >> col)) {
                /* 비트가 1: 전경색 */
                LCD_Draw_Pixel(px, py, color);
            } else {
                /* 비트가 0: 배경색 */
                LCD_Draw_Pixel(px, py, bg_color);
            }
        }
    }
}

/**
 * @brief 문자열 출력
 *
 * @param x, y: 문자열의 시작 좌표
 * @param str: 출력할 문자열 (C 문자열)
 * @param color: RGB565 전경색
 * @param bg_color: RGB565 배경색
 *
 * 지원:
 * - 일반 ASCII 문자
 * - '\n': 다음 줄 (y += FONT_HEIGHT + 1)
 * - '\r': 줄의 처음으로 (x = 시작 x)
 *
 * 예제:
 *   LCD_Draw_String(10, 10, "Hello World!", COLOR_WHITE, COLOR_BLACK);
 */
void LCD_Draw_String(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color)
{
    if (str == NULL) {
        LOG_W("NULL 문자열!");
        return;
    }

    uint16_t px = x;  /* 현재 X 좌표 */
    uint16_t start_x = x;  /* 줄의 시작 X 좌표 */

    LOG_D("String at (%u, %u): %s", x, y, str);

    while (*str) {
        if (*str == '\n') {
            /* 다음 줄 */
            y += FONT_HEIGHT + 1;
            px = start_x;
        } else if (*str == '\r') {
            /* 줄의 처음 */
            px = start_x;
        } else {
            /* 일반 문자 출력 */
            LCD_Draw_Char(px, y, *str, color, bg_color);
            px += FONT_WIDTH + 1;  /* 다음 문자 위치 (너비 + 간격 1픽셀) */

            /* 화면 오른쪽 끝을 넘으면 다음 줄로 */
            if (px + FONT_WIDTH >= LCD_WIDTH) {
                y += FONT_HEIGHT + 1;
                px = start_x;
            }
        }
        str++;
    }

    LOG_D("String 출력 완료");
}

/**
 * @brief 정수를 문자열로 변환하여 출력
 *
 * @param x, y: 좌표
 * @param value: 정수값
 * @param color: RGB565 색상
 *
 * 예제:
 *   int temp = 25;
 *   LCD_Draw_Integer(10, 50, temp, COLOR_WHITE);  // "25" 출력
 */
void LCD_Draw_Integer(uint16_t x, uint16_t y, int32_t value, uint16_t color)
{
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%ld", value);
    LCD_Draw_String(x, y, buffer, color, 0x0000);  /* 배경 검정 */
}

/**
 * @brief 실수를 문자열로 변환하여 출력
 *
 * @param x, y: 좌표
 * @param value: 실수값
 * @param precision: 소수점 자리수
 * @param color: RGB565 색상
 *
 * 예제:
 *   float temp = 23.45;
 *   LCD_Draw_Float(10, 50, temp, 2, COLOR_WHITE);  // "23.45" 출력
 */
void LCD_Draw_Float(uint16_t x, uint16_t y, float value, uint8_t precision, uint16_t color)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
    LCD_Draw_String(x, y, buffer, color, 0x0000);
}

/**
 * 사용 예제:
 *
 * void Test_Text(void)
 * {
 *     // 전체 화면을 검정으로
 *     LCD_Fill_Color(0x0000);
 *
 *     // 문자열 출력
 *     LCD_Draw_String(10, 10, "STM32 LCD", 0xFFFF, 0x0000);
 *
 *     // 개별 문자
 *     LCD_Draw_Char(10, 30, 'A', 0xF800, 0x0000);  // 빨강 'A'
 *
 *     // 숫자 출력
 *     int temperature = 25;
 *     LCD_Draw_String(10, 50, "Temp: ", 0xFFFF, 0x0000);
 *     LCD_Draw_Integer(80, 50, temperature, 0x07E0);  // 초록
 *
 *     // 실수 출력
 *     float humidity = 65.5;
 *     LCD_Draw_String(10, 70, "Humi: ", 0xFFFF, 0x0000);
 *     LCD_Draw_Float(80, 70, humidity, 1, 0x07E0);  // 65.5
 * }
 */
