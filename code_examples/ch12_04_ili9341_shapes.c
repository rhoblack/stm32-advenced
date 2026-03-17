/**
 * ch12_04_ili9341_shapes.c
 * ILI9341 기본 도형 그리기 (직선, 사각형, 원)
 *
 * Bresenham 직선 알고리즘을 사용하여 효율적으로 도형을 렌더링합니다.
 *
 * 알고리즘:
 * - Bresenham Line: 정수 연산만으로 직선 그리기
 * - Rectangle: 4개의 직선 조합
 * - Midpoint Circle: 대칭성을 이용한 원 그리기
 */

#include "main.h"
#include "log_system.h"

extern SPI_HandleTypeDef hspi1;

#define LCD_WIDTH   320
#define LCD_HEIGHT  240

/* 외부 함수 (ch12_03_ili9341_pixel_write.c에서 정의) */
extern void LCD_Draw_Pixel(uint16_t x, uint16_t y, uint16_t color);

/* ===== 직선 그리기: Bresenham 알고리즘 ===== */

/**
 * @brief Bresenham 직선 알고리즘을 사용한 직선 그리기
 *
 * @param x0, y0: 시작점
 * @param x1, y1: 끝점
 * @param color: RGB565 색상값
 *
 * 원리:
 * 직선 그리기는 픽셀이 불연속적이므로, "어느 픽셀을 칠할 것인가"를 결정해야 합니다.
 * Bresenham은 오차(error)를 계산하여, 직선에 가장 가까운 픽셀을 선택합니다.
 *
 * 예제:
 *   LCD_Draw_Line(10, 10, 100, 100, COLOR_WHITE);  // 대각선
 *   LCD_Draw_Line(50, 50, 250, 50, COLOR_RED);     // 가로선
 */
void LCD_Draw_Line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    LOG_D("Line: (%u, %u) → (%u, %u)", x0, y0, x1, y1);

    /* 직선 방향 계산 */
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int16_t sx = (x0 < x1) ? 1 : -1;  /* X 방향 (-1 또는 1) */
    int16_t sy = (y0 < y1) ? 1 : -1;  /* Y 방향 (-1 또는 1) */
    int16_t err = dx - dy;            /* 오차값 초기화 */

    int16_t x = x0, y = y0;

    while (1) {
        /* 현재 픽셀 그리기 */
        LCD_Draw_Pixel(x, y, color);

        /* 끝점에 도달했는지 확인 */
        if (x == x1 && y == y1) break;

        /* 오차값 기반 다음 픽셀 선택 */
        int16_t e2 = 2 * err;

        /* X 방향 진행 여부 */
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }

        /* Y 방향 진행 여부 */
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }

    LOG_D("Line 그리기 완료");
}

/**
 * @brief 수평선 그리기 (빠른 버전)
 *
 * @param x0, x1: X 좌표 범위
 * @param y: Y 좌표 (상수)
 * @param color: RGB565 색상값
 *
 * Bresenham보다 빠릅니다. 픽셀을 일괄 채우는 것을 고려할 수 있습니다.
 */
void LCD_Draw_Horizontal_Line(uint16_t x0, uint16_t x1, uint16_t y, uint16_t color)
{
    if (x0 > x1) {
        uint16_t temp = x0;
        x0 = x1;
        x1 = temp;
    }

    for (uint16_t x = x0; x <= x1; x++) {
        LCD_Draw_Pixel(x, y, color);
    }
}

/**
 * @brief 수직선 그리기 (빠른 버전)
 *
 * @param x: X 좌표 (상수)
 * @param y0, y1: Y 좌표 범위
 * @param color: RGB565 색상값
 */
void LCD_Draw_Vertical_Line(uint16_t x, uint16_t y0, uint16_t y1, uint16_t color)
{
    if (y0 > y1) {
        uint16_t temp = y0;
        y0 = y1;
        y1 = temp;
    }

    for (uint16_t y = y0; y <= y1; y++) {
        LCD_Draw_Pixel(x, y, color);
    }
}

/* ===== 사각형 그리기 ===== */

/**
 * @brief 사각형 윤곽선 그리기
 *
 * @param x, y: 시작점 (좌상단)
 * @param width: 너비
 * @param height: 높이
 * @param color: RGB565 색상값
 *
 * 예제:
 *   LCD_Draw_Rectangle(50, 50, 100, 100, COLOR_BLUE);
 */
void LCD_Draw_Rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    LOG_D("Rectangle: (%u, %u) %ux%u", x, y, width, height);

    /* 범위 체크 */
    if (x + width > LCD_WIDTH || y + height > LCD_HEIGHT) {
        LOG_W("사각형 범위 초과!");
        return;
    }

    /* 위쪽 선 */
    LCD_Draw_Horizontal_Line(x, x + width - 1, y, color);

    /* 아래쪽 선 */
    LCD_Draw_Horizontal_Line(x, x + width - 1, y + height - 1, color);

    /* 왼쪽 선 */
    LCD_Draw_Vertical_Line(x, y, y + height - 1, color);

    /* 오른쪽 선 */
    LCD_Draw_Vertical_Line(x + width - 1, y, y + height - 1, color);

    LOG_D("Rectangle 그리기 완료");
}

/**
 * @brief 채워진 사각형 그리기
 *
 * @param x, y: 시작점 (좌상단)
 * @param width: 너비
 * @param height: 높이
 * @param color: RGB565 색상값
 *
 * 예제:
 *   LCD_Draw_Filled_Rectangle(50, 50, 100, 100, COLOR_GREEN);
 */
void LCD_Draw_Filled_Rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    LOG_D("Filled Rectangle: (%u, %u) %ux%u", x, y, width, height);

    if (x + width > LCD_WIDTH || y + height > LCD_HEIGHT) {
        LOG_W("사각형 범위 초과!");
        return;
    }

    /* 각 행을 수평선으로 채우기 */
    for (uint16_t row = y; row < y + height; row++) {
        LCD_Draw_Horizontal_Line(x, x + width - 1, row, color);
    }

    LOG_D("Filled Rectangle 그리기 완료");
}

/* ===== 원 그리기: Midpoint Circle Algorithm ===== */

/**
 * @brief 원 그리기 (Midpoint Circle Algorithm 사용)
 *
 * @param cx, cy: 원의 중심
 * @param radius: 반지름
 * @param color: RGB565 색상값
 *
 * 원리:
 * 원은 대칭성이 높으므로, 1/8만 계산하고 8배 대칭 복사합니다.
 * 이로써 삼각함수 없이 정수 연산만으로 원을 그릴 수 있습니다.
 *
 * 예제:
 *   LCD_Draw_Circle(160, 120, 50, COLOR_YELLOW);
 */
void LCD_Draw_Circle(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t color)
{
    LOG_D("Circle: center (%u, %u), radius %u", cx, cy, radius);

    int16_t x = 0, y = radius;
    int16_t d = 3 - 2 * radius;  /* Decision parameter 초기화 */

    while (x <= y) {
        /* 8개 대칭 포인트 그리기 */

        /* 0도, 90도, 180도, 270도 (축) */
        LCD_Draw_Pixel(cx + x, cy + y, color);  /* 0도 */
        LCD_Draw_Pixel(cx - x, cy + y, color);  /* 180도 */
        LCD_Draw_Pixel(cx + x, cy - y, color);  /* 0도 (음) */
        LCD_Draw_Pixel(cx - x, cy - y, color);  /* 180도 (음) */

        /* 45도, 135도, 225도, 315도 (대각선) */
        LCD_Draw_Pixel(cx + y, cy + x, color);  /* 45도 */
        LCD_Draw_Pixel(cx - y, cy + x, color);  /* 135도 */
        LCD_Draw_Pixel(cx + y, cy - x, color);  /* 45도 (음) */
        LCD_Draw_Pixel(cx - y, cy - x, color);  /* 135도 (음) */

        /* Decision parameter 업데이트 */
        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }

        x++;
    }

    LOG_D("Circle 그리기 완료");
}

/**
 * @brief 채워진 원 그리기
 *
 * @param cx, cy: 원의 중심
 * @param radius: 반지름
 * @param color: RGB565 색상값
 *
 * 예제:
 *   LCD_Draw_Filled_Circle(160, 120, 50, COLOR_CYAN);
 */
void LCD_Draw_Filled_Circle(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t color)
{
    LOG_D("Filled Circle: center (%u, %u), radius %u", cx, cy, radius);

    int16_t x = 0, y = radius;
    int16_t d = 3 - 2 * radius;

    while (x <= y) {
        /* 대칭 수평선 그리기 */
        LCD_Draw_Horizontal_Line(cx - x, cx + x, cy + y, color);
        LCD_Draw_Horizontal_Line(cx - x, cx + x, cy - y, color);
        LCD_Draw_Horizontal_Line(cx - y, cx + y, cy + x, color);
        LCD_Draw_Horizontal_Line(cx - y, cx + y, cy - x, color);

        /* Decision parameter 업데이트 */
        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }

        x++;
    }

    LOG_D("Filled Circle 그리기 완료");
}

/**
 * @brief 삼각형 그리기
 *
 * @param x0, y0: 첫 번째 점
 * @param x1, y1: 두 번째 점
 * @param x2, y2: 세 번째 점
 * @param color: RGB565 색상값
 *
 * 예제:
 *   LCD_Draw_Triangle(100, 50, 150, 150, 50, 150, COLOR_MAGENTA);
 */
void LCD_Draw_Triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    /* 3개의 직선으로 삼각형 구성 */
    LCD_Draw_Line(x0, y0, x1, y1, color);
    LCD_Draw_Line(x1, y1, x2, y2, color);
    LCD_Draw_Line(x2, y2, x0, y0, color);

    LOG_D("Triangle 그리기 완료");
}

/**
 * 사용 예제:
 *
 * void Test_Shapes(void)
 * {
 *     // 화면 초기화
 *     LCD_Fill_Color(0x0000);  // 검정
 *
 *     // 직선 그리기
 *     LCD_Draw_Line(10, 10, 310, 10, 0xF800);        // 빨강 가로선
 *     LCD_Draw_Line(10, 10, 10, 230, 0x07E0);        // 초록 수직선
 *     LCD_Draw_Line(10, 10, 310, 230, 0x001F);       // 파랑 대각선
 *
 *     // 사각형 그리기
 *     LCD_Draw_Rectangle(50, 50, 100, 100, 0xFFFF);  // 흰색 윤곽선
 *     LCD_Draw_Filled_Rectangle(200, 50, 100, 100, 0xFFE0);  // 노랑 채움
 *
 *     // 원 그리기
 *     LCD_Draw_Circle(160, 150, 50, 0x07FF);         // 시안 원
 *     LCD_Draw_Filled_Circle(100, 150, 30, 0xF81F);  // 매젠타 채워진 원
 *
 *     // 삼각형 그리기
 *     LCD_Draw_Triangle(160, 50, 200, 150, 120, 150, 0xF800);
 * }
 */
