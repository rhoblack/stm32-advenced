/**
 * ch12_03_ili9341_pixel_write.c
 * ILI9341 픽셀 및 색상 데이터 쓰기
 *
 * LCD 초기화 후, 이 함수들을 사용해 화면에 색상을 표시합니다.
 *
 * RGB565 색상 포맷:
 * - 총 16비트 (2바이트)
 * - R: 상위 5비트 (Bit 15~11)
 * - G: 중간 6비트 (Bit 10~5)
 * - B: 하위 5비트 (Bit 4~0)
 */

#include "main.h"
#include "log_system.h"

extern SPI_HandleTypeDef hspi1;

/* LCD 해상도 */
#define LCD_WIDTH   320
#define LCD_HEIGHT  240

/* ===== RGB565 색상 매크로 ===== */

/**
 * @brief RGB888 → RGB565 변환 매크로
 *
 * @param r: 빨강 (0~255)
 * @param g: 초록 (0~255)
 * @param b: 파랑 (0~255)
 *
 * 계산:
 * - R: 8비트 → 5비트 (r >> 3)
 * - G: 8비트 → 6비트 (g >> 2)
 * - B: 8비트 → 5비트 (b >> 3)
 */
#define RGB565(r, g, b) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))

/* 기본 색상 팔레트 */
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

/**
 * @brief LCD에 색상 데이터 전송
 *
 * D/C=1(데이터 모드)로 SPI 전송합니다.
 *
 * @param pData: 색상 데이터 버퍼
 * @param Size: 바이트 수
 */
static void LCD_Write_Data(uint8_t *pData, uint32_t Size)
{
    LOG_D("LCD DATA write: %lu bytes", Size);

    /* D/C = 1 (데이터 신호) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    /* CS = 0 (LCD 선택) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);

    /* SPI 전송 */
    HAL_SPI_Transmit(&hspi1, pData, Size, 5000);

    /* SPI 완료 대기 */
    while (hspi1.State != HAL_SPI_STATE_READY) __NOP();

    /* CS = 1 (LCD 비선택) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
}

/**
 * @brief 컬럼(X 좌표) 범위 설정 명령어
 *
 * 0x2A 명령어로 X 좌표 범위(Column)를 설정합니다.
 *
 * @param x0: 시작 X 좌표
 * @param x1: 끝 X 좌표
 */
static void LCD_Set_Column(uint16_t x0, uint16_t x1)
{
    uint8_t cmd = 0x2A;
    uint8_t data[4];

    LOG_D("Set Column: %u ~ %u", x0, x1);

    /* D/C = 0 (명령어 신호) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);  /* CS = 0 */

    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    while (hspi1.State != HAL_SPI_STATE_READY) __NOP();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);    /* CS = 1 */

    /* 데이터: [X_Start_HIGH, X_Start_LOW, X_End_HIGH, X_End_LOW] */
    data[0] = (x0 >> 8) & 0xFF;
    data[1] = x0 & 0xFF;
    data[2] = (x1 >> 8) & 0xFF;
    data[3] = x1 & 0xFF;

    LCD_Write_Data(data, 4);
}

/**
 * @brief 로우(Y 좌표) 범위 설정 명령어
 *
 * 0x2B 명령어로 Y 좌표 범위(Row)를 설정합니다.
 *
 * @param y0: 시작 Y 좌표
 * @param y1: 끝 Y 좌표
 */
static void LCD_Set_Row(uint16_t y0, uint16_t y1)
{
    uint8_t cmd = 0x2B;
    uint8_t data[4];

    LOG_D("Set Row: %u ~ %u", y0, y1);

    /* D/C = 0 (명령어 신호) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);  /* CS = 0 */

    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    while (hspi1.State != HAL_SPI_STATE_READY) __NOP();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);    /* CS = 1 */

    /* 데이터: [Y_Start_HIGH, Y_Start_LOW, Y_End_HIGH, Y_End_LOW] */
    data[0] = (y0 >> 8) & 0xFF;
    data[1] = y0 & 0xFF;
    data[2] = (y1 >> 8) & 0xFF;
    data[3] = y1 & 0xFF;

    LCD_Write_Data(data, 4);
}

/**
 * @brief 메모리 쓰기 명령어 전송
 *
 * 0x2C 명령어로 메모리 쓰기 시작을 알립니다.
 * 이 함수 직후 색상 데이터를 전송합니다.
 */
static void LCD_Write_Memory_Start(void)
{
    uint8_t cmd = 0x2C;

    LOG_D("Write Memory Start");

    /* D/C = 0 (명령어 신호) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);  /* CS = 0 */

    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    while (hspi1.State != HAL_SPI_STATE_READY) __NOP();
    /* CS = 1은 나중에 데이터 전송 후 올림 */
}

/**
 * @brief 전체 화면을 특정 색상으로 채우기
 *
 * @param color: RGB565 색상값
 *
 * 주의:
 * - 전체 화면 크기: 320 × 240 = 76,800 픽셀
 * - 색상 데이터: 76,800 × 2 = 153,600 바이트
 * - 버퍼 크기 제약이 있을 수 있으므로 분할 전송 권장
 */
void LCD_Fill_Color(uint16_t color)
{
    LOG_I("LCD 화면 채우기: color = 0x%04X", color);

    /* Column 범위: 0 ~ 319 */
    LCD_Set_Column(0, LCD_WIDTH - 1);

    /* Row 범위: 0 ~ 239 */
    LCD_Set_Row(0, LCD_HEIGHT - 1);

    /* 메모리 쓰기 시작 */
    LCD_Write_Memory_Start();

    /* 색상 데이터 버퍼 (1000개 픽셀 = 2000 바이트) */
    #define COLOR_BUFFER_SIZE 2000
    static uint8_t color_buf[COLOR_BUFFER_SIZE];

    /* 버퍼에 색상값 저장 (Big Endian) */
    for (uint32_t i = 0; i < COLOR_BUFFER_SIZE; i += 2) {
        color_buf[i]     = (color >> 8) & 0xFF;  /* HIGH byte */
        color_buf[i + 1] = color & 0xFF;         /* LOW byte */
    }

    /* 분할 전송 (153,600 바이트÷ 2,000 바이트 = 77회) */
    uint32_t total_bytes = LCD_WIDTH * LCD_HEIGHT * 2;
    uint32_t transmitted = 0;

    while (transmitted < total_bytes) {
        uint32_t remaining = total_bytes - transmitted;
        uint32_t send_size = (remaining > COLOR_BUFFER_SIZE) ? COLOR_BUFFER_SIZE : remaining;

        LCD_Write_Data(color_buf, send_size);
        transmitted += send_size;

        LOG_D("전송 진행: %lu / %lu bytes", transmitted, total_bytes);
    }

    LOG_I("화면 채우기 완료");
}

/**
 * @brief 개별 픽셀 그리기
 *
 * @param x: X 좌표 (0 ~ 319)
 * @param y: Y 좌표 (0 ~ 239)
 * @param color: RGB565 색상값
 *
 * 주의: 개별 픽셀 그리기는 매우 느립니다!
 * 많은 픽셀을 그려야 할 때는 LCD_Fill_Rectangle() 사용 권장.
 */
void LCD_Draw_Pixel(uint16_t x, uint16_t y, uint16_t color)
{
    LOG_D("Pixel at (%u, %u) = 0x%04X", x, y, color);

    /* 범위 체크 */
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        LOG_W("픽셀 좌표 범위 초과: (%u, %u)", x, y);
        return;
    }

    /* Column 설정 (한 픽셀 너비) */
    LCD_Set_Column(x, x);

    /* Row 설정 (한 픽셀 높이) */
    LCD_Set_Row(y, y);

    /* 메모리 쓰기 시작 */
    LCD_Write_Memory_Start();

    /* 색상 데이터 (2바이트) */
    uint8_t color_data[2];
    color_data[0] = (color >> 8) & 0xFF;  /* HIGH byte */
    color_data[1] = color & 0xFF;         /* LOW byte */

    LCD_Write_Data(color_data, 2);
}

/**
 * @brief 채워진 사각형 그리기
 *
 * @param x: 시작 X 좌표
 * @param y: 시작 Y 좌표
 * @param width: 너비 (픽셀)
 * @param height: 높이 (픽셀)
 * @param color: RGB565 색상값
 *
 * 예제: 중앙에 100×100 초록 사각형
 *   LCD_Fill_Rectangle(110, 70, 100, 100, COLOR_GREEN);
 */
void LCD_Fill_Rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    LOG_I("사각형 그리기: (%u, %u) %ux%u, color=0x%04X", x, y, width, height, color);

    /* 범위 체크 */
    if (x + width > LCD_WIDTH || y + height > LCD_HEIGHT) {
        LOG_W("사각형 범위 초과!");
        return;
    }

    /* Column 범위 설정 */
    LCD_Set_Column(x, x + width - 1);

    /* Row 범위 설정 */
    LCD_Set_Row(y, y + height - 1);

    /* 메모리 쓰기 시작 */
    LCD_Write_Memory_Start();

    /* 색상 데이터 버퍼 */
    #define RECT_BUFFER_SIZE 2000
    static uint8_t color_buf[RECT_BUFFER_SIZE];

    /* 버퍼에 색상값 저장 */
    for (uint32_t i = 0; i < RECT_BUFFER_SIZE; i += 2) {
        color_buf[i]     = (color >> 8) & 0xFF;
        color_buf[i + 1] = color & 0xFF;
    }

    /* 사각형 면적만큼 전송 */
    uint32_t total_pixels = width * height;
    uint32_t total_bytes = total_pixels * 2;
    uint32_t transmitted = 0;

    while (transmitted < total_bytes) {
        uint32_t remaining = total_bytes - transmitted;
        uint32_t send_size = (remaining > RECT_BUFFER_SIZE) ? RECT_BUFFER_SIZE : remaining;

        LCD_Write_Data(color_buf, send_size);
        transmitted += send_size;
    }

    LOG_I("사각형 그리기 완료");
}

/**
 * @brief RGB565 색상값 계산 (8비트 RGB에서)
 *
 * @param r: 빨강 (0~255)
 * @param g: 초록 (0~255)
 * @param b: 파랑 (0~255)
 *
 * @return RGB565 16비트 값
 *
 * 예제:
 *   uint16_t orange = RGB565(255, 165, 0);  // 주황색
 *   LCD_Fill_Color(orange);
 */
uint16_t Color_RGB565(uint8_t r, uint8_t g, uint8_t b)
{
    return RGB565(r, g, b);
}

/**
 * 사용 예제:
 *
 * int main(void)
 * {
 *     // 초기화 (ch12_01, ch12_02 참조)
 *     SystemInit();
 *     SPI_DMA_Init();
 *     ILI9341_Init();
 *
 *     // 예제 1: 전체 화면을 파랑으로 채우기
 *     LCD_Fill_Color(COLOR_BLUE);
 *     HAL_Delay(1000);
 *
 *     // 예제 2: 중앙에 초록 사각형
 *     LCD_Fill_Color(COLOR_BLACK);  // 배경 검정
 *     LCD_Fill_Rectangle(110, 70, 100, 100, COLOR_GREEN);
 *     HAL_Delay(1000);
 *
 *     // 예제 3: 여러 픽셀 그리기
 *     LCD_Fill_Color(COLOR_WHITE);
 *     for (int i = 0; i < 50; i++) {
 *         LCD_Draw_Pixel(160 + i, 120, COLOR_RED);
 *     }
 *     HAL_Delay(1000);
 *
 *     // 예제 4: 커스텀 색상
 *     uint16_t orange = Color_RGB565(255, 165, 0);
 *     LCD_Fill_Color(orange);
 *
 *     while (1);
 * }
 */
