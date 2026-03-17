/**
 * @file    gfx_service.h
 * @brief   GFX Service — LCD 그래픽 추상화 레이어 (Ch13, v1.3)
 * @author  STM32 고급 실무교육
 *
 * LCD Driver(ILI9341)를 직접 호출하지 않고,
 * 이 GFX Service를 통해 하드웨어 독립적인 그래픽 작업을 수행합니다.
 *
 * 레이어 위치: Service 레이어
 * 의존성: ili9341_driver.h (Driver 레이어)
 */

#ifndef GFX_SERVICE_H
#define GFX_SERVICE_H

#include <stdint.h>
#include <stddef.h>

/* ===== 화면 치수 ===== */
#define LCD_WIDTH   240U
#define LCD_HEIGHT  320U

/* ===== RGB565 색상 팔레트 ===== */
/* RGB565: R(5비트) | G(6비트) | B(5비트) = 총 16비트 */
#define GFX_COLOR_BLACK      0x0000U  /* 검정 */
#define GFX_COLOR_WHITE      0xFFFFU  /* 흰색 */
#define GFX_COLOR_RED        0xF800U  /* 빨강 */
#define GFX_COLOR_GREEN      0x07E0U  /* 초록 */
#define GFX_COLOR_BLUE       0x001FU  /* 파랑 */
#define GFX_COLOR_YELLOW     0xFFE0U  /* 노랑 */
#define GFX_COLOR_CYAN       0x07FFU  /* 청록 */
#define GFX_COLOR_MAGENTA    0xF81FU  /* 자홍 */
#define GFX_COLOR_ORANGE     0xFD20U  /* 주황 */
#define GFX_COLOR_DARKGRAY   0x7BEFU  /* 진회색 */
#define GFX_COLOR_LIGHTGRAY  0xC618U  /* 연회색 */
#define GFX_COLOR_NAVY       0x000FU  /* 남색 */
#define GFX_COLOR_DARKGREEN  0x03E0U  /* 짙은 초록 */
#define GFX_COLOR_SKYBLUE    0x867DU  /* 하늘색 */

/* RGB 값을 RGB565 포맷으로 변환하는 매크로 */
#define GFX_RGB(r, g, b)  ((uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)))

/* ===== 폰트 크기 (scale 배수) ===== */
#define GFX_FONT_SMALL   1U   /* 8×16 픽셀 */
#define GFX_FONT_MEDIUM  2U   /* 16×32 픽셀 */
#define GFX_FONT_LARGE   3U   /* 24×48 픽셀 */

/* ===== GFX 초기화 ===== */

/**
 * @brief GFX Service 초기화 (LCD Driver 초기화 포함)
 * @note  반드시 시스템 초기화 시 한 번 호출해야 합니다.
 */
void GFX_Init(void);

/* ===== 기본 도형 그리기 ===== */

/**
 * @brief 단일 픽셀 그리기
 * @param x:      X 좌표 (0 ~ LCD_WIDTH-1)
 * @param y:      Y 좌표 (0 ~ LCD_HEIGHT-1)
 * @param color:  RGB565 색상
 */
void GFX_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief 수평선 그리기 (최적화: 연속 DMA 전송)
 * @param x:      시작 X
 * @param y:      Y 좌표
 * @param w:      너비 (픽셀 수)
 * @param color:  색상
 */
void GFX_DrawHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color);

/**
 * @brief 수직선 그리기
 */
void GFX_DrawVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color);

/**
 * @brief 직선 그리기 (Bresenham 알고리즘)
 */
void GFX_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * @brief 빈 사각형 그리기
 */
void GFX_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief 채워진 사각형 그리기 (DMA 전송으로 최적화)
 * @param x, y:  좌상단 좌표
 * @param w, h:  너비, 높이
 * @param color: 채울 색상
 */
void GFX_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief 전체 화면 색상으로 채우기
 * @param color: 배경 색상
 */
void GFX_FillScreen(uint16_t color);

/**
 * @brief 원 그리기 (Midpoint Circle 알고리즘)
 */
void GFX_DrawCircle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color);

/**
 * @brief 채워진 원 그리기
 */
void GFX_FillCircle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color);

/* ===== 폰트 렌더링 ===== */

/**
 * @brief 단일 문자 출력
 * @param x:     시작 X 좌표
 * @param y:     시작 Y 좌표
 * @param c:     출력할 ASCII 문자 (0x20~0x7E)
 * @param fg:    전경색 (글자색)
 * @param bg:    배경색
 * @param scale: 배율 (1=8×16, 2=16×32, 3=24×48)
 */
void GFX_DrawChar(uint16_t x, uint16_t y, char c,
                  uint16_t fg, uint16_t bg, uint8_t scale);

/**
 * @brief 문자열 출력
 * @param x, y:  시작 좌표
 * @param str:   출력할 문자열 (NULL 종료)
 * @param fg:    글자색
 * @param bg:    배경색
 * @param scale: 배율
 */
void GFX_DrawString(uint16_t x, uint16_t y, const char *str,
                    uint16_t fg, uint16_t bg, uint8_t scale);

/**
 * @brief 정수형 숫자 출력
 * @param x, y:   시작 좌표
 * @param val:    출력할 정수
 * @param digits: 최소 자릿수 (0이면 자동)
 * @param fg, bg: 글자색, 배경색
 * @param scale:  배율
 */
void GFX_DrawNumber(uint16_t x, uint16_t y, int32_t val, uint8_t digits,
                    uint16_t fg, uint16_t bg, uint8_t scale);

/**
 * @brief 소수점 숫자 출력 (온습도 표시용)
 * @param x, y:       시작 좌표
 * @param integer:    정수부
 * @param decimal:    소수부 (1자리: 0~9)
 * @param fg, bg:     글자색, 배경색
 * @param scale:      배율
 */
void GFX_DrawFloat1(uint16_t x, uint16_t y, int32_t integer, int32_t decimal,
                    uint16_t fg, uint16_t bg, uint8_t scale);

#endif /* GFX_SERVICE_H */
