/**
 * ch12_02_ili9341_init.c
 * ILI9341 TFT LCD 초기화 코드
 *
 * 이 파일은 ILI9341 데이터시트의 표준 초기화 시퀀스를 구현합니다.
 * 공식 명령어와 파라미터를 그대로 사용하므로, 수정하지 마세요.
 *
 * LCD 해상도: 320×240 픽셀
 * 색상 포맷: RGB565 (16비트)
 * 통신: SPI1 (CPOL=0, CPHA=0, ~10MHz)
 */

#include "main.h"
#include "log_system.h"

extern SPI_HandleTypeDef hspi1;

/**
 * @brief 딜레이 함수 (루프 기반, 마이크로초)
 *
 * HAL_Delay()는 밀리초 단위이고 정밀도가 부족하므로,
 * 정확한 타이밍이 필요한 초기화에는 루프 기반 딜레이를 사용합니다.
 *
 * @param ms: 밀리초 수 (대략적)
 */
static void Delay_Ms(uint32_t ms)
{
    volatile uint32_t i;
    for (i = 0; i < ms * 10000; i++) {
        __NOP();
    }
}

/**
 * @brief LCD 명령어 전송 (D/C = 0)
 *
 * @param cmd: 명령어 바이트 (예: 0x11, 0x29, 0x2A 등)
 *
 * 순서:
 * 1. D/C = 0 (명령어 신호)
 * 2. CS = 0 (LCD 선택)
 * 3. SPI로 명령어 전송
 * 4. CS = 1 (LCD 비선택)
 *
 * 주의: D/C는 LOW로 유지되고, 다음 함수에서 HIGH로 변경됩니다.
 */
static void LCD_Send_Command(uint8_t cmd)
{
    LOG_D("LCD CMD: 0x%02X", cmd);

    /* D/C = 0 (명령어 신호) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

    /* CS = 0 (LCD 선택) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);

    /* SPI 전송 (폴링 모드, DMA 미사용) */
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);

    if (status != HAL_OK) {
        LOG_E("LCD 명령어 전송 실패: 0x%02X", cmd);
    }

    /* SPI 전송 완료 대기 */
    while (hspi1.State != HAL_SPI_STATE_READY) {
        __NOP();
    }

    /* CS = 1 (LCD 비선택) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
}

/**
 * @brief LCD 데이터 전송 (D/C = 1)
 *
 * @param pData: 데이터 버퍼
 * @param Size: 바이트 수 (보통 1~5)
 *
 * 순서:
 * 1. D/C = 1 (데이터 신호)
 * 2. CS = 0 (LCD 선택)
 * 3. SPI로 데이터 전송
 * 4. CS = 1 (LCD 비선택)
 */
static void LCD_Send_Data(uint8_t *pData, uint16_t Size)
{
    LOG_D("LCD DATA: %d bytes", Size);

    /* D/C = 1 (데이터 신호) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    /* CS = 0 (LCD 선택) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);

    /* SPI 전송 (폴링 모드) */
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, pData, Size, 1000);

    if (status != HAL_OK) {
        LOG_E("LCD 데이터 전송 실패!");
    }

    /* SPI 전송 완료 대기 */
    while (hspi1.State != HAL_SPI_STATE_READY) {
        __NOP();
    }

    /* CS = 1 (LCD 비선택) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
}

/**
 * @brief ILI9341 Reset 시퀀스
 *
 * LCD 칩의 모든 레지스터를 초기값으로 설정합니다.
 * 전원이 켜진 후 첫 번째로 실행해야 합니다.
 *
 * 순서:
 * 1. RESET = 0 (LOW) — 최소 10ms
 * 2. RESET = 1 (HIGH) — 최소 120ms 대기
 */
static void LCD_Reset(void)
{
    LOG_I("LCD Reset 시작");

    /* RESET = 0 (LOW) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    LOG_D("RESET = LOW");
    Delay_Ms(10);

    /* RESET = 1 (HIGH) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    LOG_D("RESET = HIGH");
    Delay_Ms(120);

    LOG_I("LCD Reset 완료");
}

/**
 * @brief ILI9341 초기화 (공식 데이터시트 기반)
 *
 * ILI9341은 전원 켜짐 후 약 25개의 명령어를 순서대로 실행해야 합니다.
 * 이 코드는 공식 초기화 시퀀스를 그대로 구현했습니다.
 *
 * 주의:
 * - 명령어의 순서를 바꾸지 마세요
 * - 파라미터 값을 임의로 수정하지 마세요
 * - 각 명령어 사이의 딜레이(1~5ms)는 필수입니다
 *
 * 데이터시트 참조:
 * - ILI9341 DATASHEET, Section "Display Initialization"
 */
void ILI9341_Init(void)
{
    LOG_I("========================================");
    LOG_I("ILI9341 LCD 초기화 시작");
    LOG_I("========================================");

    /* Reset 신호 전송 */
    LCD_Reset();

    /* ===== 초기화 명령어 시퀀스 (공식 데이터시트) ===== */

    /* 1. Power Control A (0xCB) */
    uint8_t pwr_ctrl_a[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
    LCD_Send_Command(0xCB);
    LCD_Send_Data(pwr_ctrl_a, 5);
    Delay_Ms(5);

    /* 2. Power Control B (0xCF) */
    uint8_t pwr_ctrl_b[] = {0x00, 0xC1, 0x30};
    LCD_Send_Command(0xCF);
    LCD_Send_Data(pwr_ctrl_b, 3);
    Delay_Ms(5);

    /* 3. Power Control 1 (0xC0) */
    uint8_t pwr_ctrl_1[] = {0x85, 0x01, 0x79};
    LCD_Send_Command(0xC0);
    LCD_Send_Data(pwr_ctrl_1, 3);
    Delay_Ms(5);

    /* 4. Power Control 2 (0xC1) */
    uint8_t pwr_ctrl_2[] = {0x10};
    LCD_Send_Command(0xC1);
    LCD_Send_Data(pwr_ctrl_2, 1);
    Delay_Ms(5);

    /* 5. VCOMH / VCML (0xC5) */
    uint8_t vcom_control[] = {0x3E, 0x28};
    LCD_Send_Command(0xC5);
    LCD_Send_Data(vcom_control, 2);
    Delay_Ms(5);

    /* 6. VCOMH Regulator (0xC7) */
    uint8_t vcomh_reg[] = {0x86};
    LCD_Send_Command(0xC7);
    LCD_Send_Data(vcomh_reg, 1);
    Delay_Ms(5);

    /* 7. Memory Access Control (0x36) — 화면 방향 및 색상 순서 */
    /* 0x48 = BGR mode, MY=0, MX=0, MV=0 (세로 방향) */
    uint8_t mem_access[] = {0x48};
    LCD_Send_Command(0x36);
    LCD_Send_Data(mem_access, 1);
    Delay_Ms(5);

    /* 8. Pixel Format Set (0x3A) — 16비트 색상 선택 */
    /* 0x55 = 16 bits per pixel (RGB565) */
    uint8_t pixel_format[] = {0x55};
    LCD_Send_Command(0x3A);
    LCD_Send_Data(pixel_format, 1);
    Delay_Ms(5);

    /* 9. Frame Rate Control (0xB1) */
    uint8_t frame_rate[] = {0x00, 0x1B};
    LCD_Send_Command(0xB1);
    LCD_Send_Data(frame_rate, 2);
    Delay_Ms(5);

    /* 10. Display Function Control (0xB6) */
    uint8_t disp_func[] = {0x0A, 0xA2, 0x27, 0x04};
    LCD_Send_Command(0xB6);
    LCD_Send_Data(disp_func, 4);
    Delay_Ms(5);

    /* 11. Gamma Set (0x26) */
    uint8_t gamma_set[] = {0x01};
    LCD_Send_Command(0x26);
    LCD_Send_Data(gamma_set, 1);
    Delay_Ms(5);

    /* 12. Positive Gamma Correction (0xE0) */
    uint8_t gamma_pos[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08,
                           0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
    LCD_Send_Command(0xE0);
    LCD_Send_Data(gamma_pos, 15);
    Delay_Ms(5);

    /* 13. Negative Gamma Correction (0xE1) */
    uint8_t gamma_neg[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07,
                           0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
    LCD_Send_Command(0xE1);
    LCD_Send_Data(gamma_neg, 15);
    Delay_Ms(5);

    /* 14. Sleep Out (0x11) — LCD 웨이크업 */
    LCD_Send_Command(0x11);
    LOG_D("Sleep Out 명령어 전송");
    Delay_Ms(120);  /* 최소 120ms 대기 (LCD 안정화) */

    /* 15. Display On (0x29) */
    LCD_Send_Command(0x29);
    LOG_D("Display On 명령어 전송");
    Delay_Ms(5);

    LOG_I("========================================");
    LOG_I("ILI9341 초기화 완료 ✓");
    LOG_I("LCD는 이제 사용 가능합니다");
    LOG_I("========================================");
}

/**
 * 사용 예제:
 *
 * int main(void)
 * {
 *     HAL_Init();
 *     SystemClock_Config();
 *
 *     MX_GPIO_Init();
 *     MX_SPI1_Init();
 *     MX_DMA_Init();
 *     MX_USART2_UART_Init();  // 로그
 *
 *     LOG_I("시스템 시작");
 *
 *     // SPI DMA 초기화 (ch12_01_spi_dma_init.c)
 *     SPI_DMA_Init();
 *
 *     // ILI9341 초기화
 *     ILI9341_Init();
 *
 *     // 이제 LCD를 사용할 수 있습니다
 *     // ILI9341_Fill_Color(0xF800);  // 화면을 빨강으로 채우기
 *
 *     while (1) {
 *         // 메인 루프
 *     }
 * }
 *
 * 문제 해결:
 * - "LCD가 켜지지 않음": Reset 신호 확인, 명령어 순서 확인
 * - "색상이 이상함": Memory Access Control (0x36) 값 확인
 * - "화면이 왜곡됨": Gamma 설정 또는 Voltage 설정 확인
 */
