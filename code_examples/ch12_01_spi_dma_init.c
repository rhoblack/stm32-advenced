/**
 * ch12_01_spi_dma_init.c
 * SPI1 DMA 기본 설정 및 초기화
 *
 * 이 파일은 STM32CubeIDE의 CubeMX에서 자동 생성되는 코드를 기반으로,
 * SPI DMA 전송을 위한 필수 설정을 보여줍니다.
 *
 * CubeMX 설정:
 * - SPI1 Master, CPOL=0, CPHA=0 (Mode 0), Clock 10MHz
 * - DMA2 Stream 2, Channel 0, Memory to Peripheral, Normal Mode
 * - GPIO: PA3(CS), PA4(D/C), PA5(RESET) Output
 */

#include "main.h"
#include "log_system.h"

/* STM32CubeIDE에서 생성된 HAL 핸들 */
extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_tx;

/**
 * @brief SPI1 및 DMA 초기화
 *
 * 주의:
 * 1. 이 함수는 CubeMX의 MX_GPIO_Init(), MX_SPI1_Init(), MX_DMA_Init()
 *    실행 후에 호출되어야 합니다. (main.c에서 SystemClock_Config() 직후)
 * 2. DMA 콜백 함수는 stm32f4xx_it.c의 사용자 영역에 등록해야 합니다.
 */
void SPI_DMA_Init(void)
{
    LOG_I("========================================");
    LOG_I("SPI1 DMA 초기화 시작");
    LOG_I("========================================");

    /* SPI1 기본 설정 확인 */
    LOG_D("SPI1 설정:");
    LOG_D("  Instance: SPI1");
    LOG_D("  Mode: Master");
    LOG_D("  CPOL: 0, CPHA: 0 (Mode 0)");
    LOG_D("  Prescaler: 4 (약 10MHz @ APB2 84MHz)");

    /* DMA 설정 확인 */
    LOG_D("DMA2 설정:");
    LOG_D("  Stream: 2");
    LOG_D("  Channel: 0");
    LOG_D("  Direction: Memory -> Peripheral (SPI1 TX)");
    LOG_D("  Mode: Normal (NOT Circular)");
    LOG_D("  Interrupt: ENABLED");

    /* GPIO 설정 확인 */
    LOG_D("GPIO 설정:");
    LOG_D("  PA3: CS (Output)");
    LOG_D("  PA4: D/C (Output)");
    LOG_D("  PA5: RESET (Output)");
    LOG_D("  PB3: SPI1_SCK (AF)");
    LOG_D("  PB5: SPI1_MOSI (AF)");

    /* CS 핀을 HIGH로 초기화 (LCD 비선택) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
    LOG_D("CS (PA3) = HIGH (LCD 비선택)");

    /* D/C 핀을 LOW로 초기화 (명령어 모드 준비) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    LOG_D("D/C (PA4) = LOW (명령어 모드 준비)");

    /* RESET 핀을 HIGH로 초기화 (이후 Reset 시퀀스에서 제어) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    LOG_D("RESET (PA5) = HIGH");

    LOG_I("SPI1 DMA 초기화 완료 ✓");
}

/**
 * @brief SPI DMA 전송 (폴링)
 *
 * @param pData: 송신할 데이터 버퍼 (반드시 static 또는 heap에 할당되어야 함!)
 * @param Size: 전송 바이트 수
 *
 * @return HAL_OK 성공, 그 외 실패
 *
 * 중요: DMA 버퍼는 절대 스택 변수를 사용하지 마세요!
 * 스택은 함수 return 시 해제되므로 DMA가 유효하지 않은 메모리에 접근하게 됩니다.
 */
HAL_StatusTypeDef SPI_Transmit_DMA(uint8_t *pData, uint16_t Size)
{
    LOG_D("SPI DMA 전송 시작: %d bytes", Size);

    /* 유효성 검사 */
    if (pData == NULL || Size == 0) {
        LOG_E("SPI_Transmit_DMA: 잘못된 파라미터!");
        return HAL_ERROR;
    }

    /* CS 핀을 LOW로 (LCD 선택) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
    LOG_D("CS = LOW (LCD 선택)");

    /* SPI DMA 전송 시작 */
    HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA(&hspi1, pData, Size);

    if (status != HAL_OK) {
        LOG_E("SPI DMA 전송 실패! Status = %d", status);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);  /* CS = HIGH */
        return status;
    }

    LOG_D("SPI DMA 전송 요청 완료");
    /* CS 핀은 DMA 완료 콜백에서 HIGH로 올립니다 */

    return HAL_OK;
}

/**
 * @brief SPI DMA 전송 완료 대기
 *
 * 이 함수는 DMA 전송이 완료될 때까지 blocking으로 대기합니다.
 * 초기화 명령어들 사이의 타이밍을 정확하게 유지하기 위해 사용합니다.
 */
void SPI_Wait_Complete(void)
{
    uint32_t timeout = 1000000;  /* 타임아웃 카운터 */

    while (hspi1.State != HAL_SPI_STATE_READY && timeout-- > 0) {
        __NOP();  /* NOP: No Operation */
    }

    if (timeout == 0) {
        LOG_W("SPI DMA 전송 타임아웃!");
    }
}

/**
 * @brief DMA 전송 완료 콜백 함수
 *
 * 이 함수는 SPI DMA 전송이 완료되었을 때 호출됩니다.
 * stm32f4xx_it.c의 DMA2_Stream2_IRQHandler()에서 호출합니다.
 *
 * 구현 위치: 사용자는 이 함수를 stm32f4xx_it.c의 "USER CODE" 섹션에 작성합니다.
 * 또는 이 파일에 정의하고 약한 심볼(weak)로 오버라이드합니다.
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        LOG_D("SPI DMA TX 완료 콜백 호출");

        /* CS 핀을 HIGH로 (LCD 비선택) */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);

        LOG_D("CS = HIGH (LCD 비선택)");
    }
}

/**
 * @brief DMA 전송 에러 콜백
 *
 * DMA 중 에러가 발생하면 호출됩니다.
 */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        LOG_E("SPI DMA 에러! ErrorCode = 0x%lX", hspi->ErrorCode);

        /* CS 핀을 HIGH로 (안전한 상태) */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
    }
}

/**
 * @brief 간단한 딜레이 함수 (루프 기반)
 *
 * HAL_Delay()는 마이크로초 정밀도가 부족하므로,
 * LCD 초기화 명령어 간 짧은 딜레이는 루프로 구현합니다.
 *
 * @param ms: 밀리초 수 (대략적)
 *
 * 주의: 이는 근사값이며, 최적화 레벨에 따라 달라질 수 있습니다.
 * 정확한 타이밍이 필요하면 TIM(Timer)을 사용하세요.
 */
void Delay_Ms(uint32_t ms)
{
    volatile uint32_t i;
    for (i = 0; i < ms * 10000; i++) {
        __NOP();
    }
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
 *     SPI_DMA_Init();  // SPI DMA 초기화
 *
 *     // 이제 SPI DMA를 사용할 수 있습니다
 *     static uint8_t test_data[] = {0x11, 0x22, 0x33};
 *     SPI_Transmit_DMA(test_data, 3);
 *     SPI_Wait_Complete();
 *
 *     while (1) {
 *         // 메인 루프
 *     }
 * }
 */
