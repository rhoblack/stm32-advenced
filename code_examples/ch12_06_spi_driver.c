/**
 * ch12_06_spi_driver.c
 * SPI 드라이버 구현
 *
 * 역할:
 * - HAL과 응용 계층 사이의 중간 계층
 * - SPI1 DMA 전송의 상세 구현 은폐
 * - CS, D/C, RESET 핀 관리
 */

#include "main.h"
#include "log_system.h"
#include "ch12_06_spi_driver.h"

extern SPI_HandleTypeDef hspi1;

/**
 * @brief SPI1 드라이버 초기화
 */
void SPI_Driver_Init(void)
{
    LOG_I("SPI Driver 초기화 시작");

    /* CS 핀 초기화 (PA3) */
    SPI_CS_Set(GPIO_PIN_SET);  /* HIGH = LCD 비선택 */
    LOG_D("CS (PA3) = HIGH");

    /* D/C 핀 초기화 (PA4) */
    SPI_DC_Set(GPIO_PIN_RESET);  /* LOW = 명령어 모드 */
    LOG_D("D/C (PA4) = LOW");

    /* RESET 핀 초기화 (PA5) */
    SPI_RESET_Set(GPIO_PIN_SET);  /* HIGH */
    LOG_D("RESET (PA5) = HIGH");

    LOG_I("SPI Driver 초기화 완료");
}

/**
 * @brief SPI DMA 전송 (비동기)
 */
HAL_StatusTypeDef SPI_Transmit_DMA(uint8_t *pData, uint16_t Size)
{
    /* 유효성 검사 */
    if (pData == NULL || Size == 0) {
        LOG_E("SPI_Transmit_DMA: 잘못된 파라미터");
        return HAL_ERROR;
    }

    /* 이전 전송 중인지 확인 */
    if (hspi1.State != HAL_SPI_STATE_READY) {
        LOG_W("SPI가 이미 사용 중입니다");
        return HAL_BUSY;
    }

    LOG_D("SPI DMA TX 시작: %d bytes", Size);

    /* CS = LOW (LCD 선택) */
    SPI_CS_Set(GPIO_PIN_RESET);

    /* DMA 전송 시작 */
    HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA(&hspi1, pData, Size);

    if (status != HAL_OK) {
        LOG_E("SPI DMA 요청 실패! Status = %d", status);
        SPI_CS_Set(GPIO_PIN_SET);  /* 에러 시 CS = HIGH */
        return status;
    }

    return HAL_OK;
}

/**
 * @brief SPI 폴링 전송 (동기)
 */
HAL_StatusTypeDef SPI_Transmit_Polling(uint8_t *pData, uint16_t Size)
{
    if (pData == NULL || Size == 0) {
        LOG_E("SPI_Transmit_Polling: 잘못된 파라미터");
        return HAL_ERROR;
    }

    LOG_D("SPI Polling TX 시작: %d bytes", Size);

    /* CS = LOW */
    SPI_CS_Set(GPIO_PIN_RESET);

    /* 폴링 전송 */
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, pData, Size, 1000);

    if (status != HAL_OK) {
        LOG_E("SPI Polling 전송 실패! Status = %d", status);
        SPI_CS_Set(GPIO_PIN_SET);  /* 에러 시 CS = HIGH */
        return status;
    }

    /* 전송 완료 대기 */
    while (hspi1.State != HAL_SPI_STATE_READY) {
        __NOP();
    }

    /* CS = HIGH */
    SPI_CS_Set(GPIO_PIN_SET);

    LOG_D("SPI Polling TX 완료");

    return HAL_OK;
}

/**
 * @brief DMA 전송 완료 대기
 */
void SPI_Wait_Complete(void)
{
    uint32_t timeout = 5000000;  /* 타임아웃 */

    while (hspi1.State != HAL_SPI_STATE_READY && timeout-- > 0) {
        __NOP();
    }

    if (timeout == 0) {
        LOG_W("SPI DMA 전송 타임아웃!");
    }
}

/**
 * @brief SPI 상태 조회
 */
uint32_t SPI_Get_State(void)
{
    return hspi1.State;
}

/**
 * @brief SPI 에러 코드 조회
 */
uint32_t SPI_Get_Error(void)
{
    return hspi1.ErrorCode;
}

/**
 * ===== 핀 제어 함수 =====
 */

/**
 * @brief CS 핀 제어
 */
void SPI_CS_Set(GPIO_PinState state)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, state);
}

/**
 * @brief D/C 핀 제어
 */
void SPI_DC_Set(GPIO_PinState state)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, state);
}

/**
 * @brief RESET 핀 제어
 */
void SPI_RESET_Set(GPIO_PinState state)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, state);
}

/**
 * ===== 콜백 함수 =====
 */

/**
 * @brief DMA 전송 완료 콜백 (약한 심볼)
 *
 * 이 함수는 stm32f4xx_it.c의 DMA2_Stream2_IRQHandler()에서 호출됩니다.
 * 사용자는 이를 오버라이드하여 추가 작업을 수행할 수 있습니다.
 */
__weak void SPI_DMA_Callback(void)
{
    /* 기본 동작: CS = HIGH */
    SPI_CS_Set(GPIO_PIN_SET);
    LOG_D("SPI DMA 완료 콜백");
}

/**
 * @brief SPI 에러 콜백 (약한 심볼)
 */
__weak void SPI_Error_Callback(void)
{
    /* 안전한 상태로 복귀 */
    SPI_CS_Set(GPIO_PIN_SET);
    LOG_E("SPI 에러 콜백");
}

/**
 * @brief HAL SPI 전송 완료 콜백 (stm32f4xx_it.c에서 호출)
 *
 * 이 함수는 HAL에서 자동으로 호출됩니다.
 * 여기서 우리의 사용자 콜백을 호출합니다.
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        SPI_DMA_Callback();
    }
}

/**
 * @brief HAL SPI 에러 콜백
 */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        LOG_E("SPI 에러! ErrorCode = 0x%lX", hspi->ErrorCode);
        SPI_Error_Callback();
    }
}
