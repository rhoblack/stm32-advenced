/**
 * 파일명 : ch03_01_spaghetti_demo.c
 * 목  적 : "3분 공포 체험" — 아키텍처 없이 모든 기능을 main.c에 넣은 예시
 * MCU   : STM32F411RE (NUCLEO-F411RE)
 * 버  전 : (교육용 안티패턴 — 실제 빌드하지 않음)
 *
 * 설명:
 *   Ch10(v1.0)까지의 기능을 아키텍처 없이 단일 파일에 넣었을 때
 *   발생하는 문제를 체감하기 위한 스파게티 코드 예시.
 *   "이 코드에서 SHT31 센서 읽기가 안 될 때, 어디부터 보시겠습니까?"
 *
 * ⚠️ 이 코드는 교육 목적의 안티패턴입니다. 실제 프로젝트에서 따라하지 마세요.
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

/* ===== 전역 변수 — 전부 한 곳에 모여 있다 ===== */
static volatile uint8_t  g_led_state      = 0;
static volatile uint32_t g_last_press_ms  = 0;
static volatile uint32_t g_press_count    = 0;
static volatile uint8_t  g_uart_rx_buf[256];
static volatile uint16_t g_uart_rx_idx    = 0;
static volatile uint8_t  g_dma_complete   = 0;
static volatile uint32_t g_tim_counter    = 0;
static volatile uint8_t  g_alarm_fired    = 0;
static float             g_temperature    = 0.0f;
static float             g_humidity       = 0.0f;
static volatile int16_t  g_motor_position = 0;
static volatile int16_t  g_motor_target   = 0;
static uint8_t           g_motor_phase    = 0;
static uint8_t           g_lcd_framebuf[320 * 240 * 2];
/* ⚠️ 의도적 데모 코드: 153KB > F411RE SRAM 128KB → 실제로는 빌드 불가!
 *    실전에서는 DMA 스트리밍으로 부분 전송한다 (Ch12~13 참조) */
/* ... 변수가 20개 이상 계속 늘어남 ... */

/* ===== GPIO 초기화 (Ch01) ===== */
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* PA5: LED */
    gpio.Pin = GPIO_PIN_5; gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL; gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* PC13: 버튼 EXTI */
    gpio.Pin = GPIO_PIN_13; gpio.Mode = GPIO_MODE_IT_FALLING;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &gpio);

    /* PA0~PA3: 스텝모터 (ULN2003) */
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_OUTPUT_PP; gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* PB6: SPI CS (LCD) */
    gpio.Pin = GPIO_PIN_6; gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* PB4: LCD DC핀 */
    gpio.Pin = GPIO_PIN_4; gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* ... 핀 설정이 계속 늘어남 ... */
}

/* ===== UART 초기화 (Ch04) ===== */
UART_HandleTypeDef huart2;
void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    HAL_UART_Init(&huart2);
}

/* ===== I2C 초기화 (Ch11) ===== */
I2C_HandleTypeDef hi2c1;
void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    HAL_I2C_Init(&hi2c1);
}

/* ===== SPI 초기화 (Ch12) ===== */
SPI_HandleTypeDef hspi1;
void MX_SPI1_Init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    HAL_SPI_Init(&hspi1);
}

/* ===== TIM 초기화 (Ch07) ===== */
TIM_HandleTypeDef htim2;
void MX_TIM2_Init(void)
{
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 99;
    htim2.Init.Period = 999;
    HAL_TIM_Base_Init(&htim2);
}

/* ===== SHT31 센서 읽기 (Ch11) ===== */
void sht31_read(float *temp, float *humi)
{
    uint8_t cmd[2] = {0x24, 0x00};
    uint8_t data[6];

    HAL_I2C_Master_Transmit(&hi2c1, 0x44 << 1, cmd, 2, 100);
    HAL_Delay(20);  /* ← 블로킹! 스텝모터 타이밍 놓침 */
    HAL_I2C_Master_Receive(&hi2c1, 0x44 << 1, data, 6, 100);

    uint16_t raw_temp = (data[0] << 8) | data[1];
    uint16_t raw_humi = (data[3] << 8) | data[4];
    *temp = -45.0f + 175.0f * (float)raw_temp / 65535.0f;
    *humi = 100.0f * (float)raw_humi / 65535.0f;
}

/* ===== LCD 커맨드 전송 (Ch12) ===== */
void lcd_send_command(uint8_t cmd)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET); /* DC=0 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); /* CS=0 */
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);   /* CS=1 */
}

/* ===== 스텝모터 한 스텝 (Ch10) ===== */
static const uint8_t motor_seq[8] = {
    0x01, 0x03, 0x02, 0x06, 0x04, 0x0C, 0x08, 0x09
};
void stepper_one_step(int direction)
{
    if (direction > 0) g_motor_phase = (g_motor_phase + 1) & 0x07;
    else               g_motor_phase = (g_motor_phase - 1) & 0x07;

    uint8_t bits = motor_seq[g_motor_phase];
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, (bits & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, (bits & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, (bits & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, (bits & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    g_motor_position += direction;
}

/* ===== 인터럽트 콜백들 — 전부 여기에 모여 있다 ===== */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_13) {
        uint32_t now = HAL_GetTick();
        if ((now - g_last_press_ms) < 50) return;
        g_last_press_ms = now;
        g_press_count++;
        g_led_state ^= 1;
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,
                          g_led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        g_tim_counter++;
        /* 스텝모터 이동 — 그런데 여기서 LCD도 갱신해야 하나?
         * 센서 읽기도 여기서? UART 전송도? */
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    /* UART 수신 처리 — CLI 파싱도 여기서? */
    g_uart_rx_buf[g_uart_rx_idx++] = 0; /* 수신 데이터 저장 */
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    g_alarm_fired = 1;
    /* 알람 처리 — 부저? LCD 갱신? 모터 정지? 전부? */
}

/* ===== 메인 루프 — 모든 기능이 뒤섞여 있다 ===== */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_I2C1_Init();
    MX_SPI1_Init();
    MX_TIM2_Init();
    /* ... 초기화 10개 이상 ... */

    HAL_TIM_Base_Start_IT(&htim2);

    uint32_t last_sensor = 0;
    uint32_t last_lcd    = 0;
    char buf[128];

    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* 1초마다 센서 읽기 — 20ms 블로킹! */
        if (now - last_sensor >= 1000) {
            last_sensor = now;
            sht31_read(&g_temperature, &g_humidity);
            /* ↑ 여기서 20ms 블로킹되면 스텝모터 타이밍 놓침! */
        }

        /* 500ms마다 LCD 갱신 */
        if (now - last_lcd >= 500) {
            last_lcd = now;
            lcd_send_command(0x2C);
            /* ... LCD에 시각 + 온습도 + 스톱워치 전부 그리기 ... */
            /* 이 작업도 수십 ms 소요 */
        }

        /* 스텝모터 위치 제어 — 1ms 정밀 타이밍 필요 */
        if (g_motor_position != g_motor_target) {
            int dir = (g_motor_target > g_motor_position) ? 1 : -1;
            stepper_one_step(dir);
            HAL_Delay(1);
            /* ↑ HAL_Delay(1)인데, 위의 센서/LCD가 블로킹하면
             *   실제로는 21ms~61ms 간격이 될 수 있다! */
        }

        /* UART 수신 처리 + CLI 파싱 */
        if (g_uart_rx_idx > 0) {
            /* 여기서 문자열 파싱하고 명령 실행...
             * "time set 12:30"이면 RTC 설정,
             * "motor pos 100"이면 모터 이동,
             * "sensor read"이면 센서 읽기...
             * 전부 여기 if-else 체인으로? */
        }

        /* 알람 처리 */
        if (g_alarm_fired) {
            g_alarm_fired = 0;
            /* 부저 울리기? LCD 갱신? 모터 정지? 로그 출력?
             * 전부 여기서 처리? */
        }

        /* 질문: 이 코드에서 "센서 값이 틀려요"라는 버그 리포트가 왔다.
         *       어디부터 봐야 하는가?
         *       sht31_read()? I2C 초기화? DMA 설정? LCD가 SPI 버스 점유?
         *       타이머 인터럽트가 I2C 전송 도중 끼어든 건 아닌가?
         *       ... 답: 전부 봐야 한다. 3000줄 전체를 수색해야 한다. */
    }
}
