/**
 * @file    ch14_multi_dma.c
 * @brief   다중 DMA 동시 운용 — 우선순위 설정 예제 (Ch14 v1.4)
 *
 * UART DMA + SPI DMA + I2C DMA를 동시에 운용할 때의
 * DMA 스트림 우선순위 설정 및 NVIC 인터럽트 우선순위 설정 예제입니다.
 *
 * CubeMX에서 설정한 DMA 채널과 NVIC 설정을 코드로 검증합니다.
 *
 * @note    STM32F411RE 기준
 * @version v1.4
 */

#include "stm32f4xx_hal.h"
#include "log.h"

/* ===================================================================
 * DMA 스트림 우선순위 런타임 설정 (CubeMX 생성 후 검증용)
 *
 * CubeMX에서 설정하는 것이 권장되지만,
 * 코드로 이해하기 위해 명시적으로 작성합니다.
 * =================================================================== */

/**
 * @brief v1.4 프로젝트 DMA 우선순위 설정 확인 및 출력
 *
 * CubeMX가 생성한 DMA_HandleTypeDef의 Init.Priority 값을 검증합니다.
 * 실제 설정은 MX_DMA_Init()에서 이루어집니다.
 */
void MultiDma_PrintPriorityConfig(DMA_HandleTypeDef *hdma_spi_tx,
                                   DMA_HandleTypeDef *hdma_uart_tx,
                                   DMA_HandleTypeDef *hdma_i2c_tx,
                                   DMA_HandleTypeDef *hdma_i2c_rx)
{
    LOG_I("=== v1.4 DMA 우선순위 설정 현황 ===");

    /* SPI1 TX (ILI9341 LCD) — 최우선 */
    LOG_I("SPI1  TX: DMA2 S3 Ch3 Priority=%lu (기대값: DMA_PRIORITY_VERY_HIGH=%lu)",
          hdma_spi_tx->Init.Priority, (uint32_t)DMA_PRIORITY_VERY_HIGH);

    /* UART2 TX (PC 로그) — 중간 */
    LOG_I("UART2 TX: DMA1 S6 Ch4 Priority=%lu (기대값: DMA_PRIORITY_MEDIUM=%lu)",
          hdma_uart_tx->Init.Priority, (uint32_t)DMA_PRIORITY_MEDIUM);

    /* I2C1 TX (SHT31 CMD) — 낮음 */
    LOG_I("I2C1  TX: DMA1 S7 Ch1 Priority=%lu (기대값: DMA_PRIORITY_LOW=%lu)",
          hdma_i2c_tx->Init.Priority, (uint32_t)DMA_PRIORITY_LOW);

    /* I2C1 RX (SHT31 데이터) — 낮음 */
    LOG_I("I2C1  RX: DMA1 S0 Ch1 Priority=%lu (기대값: DMA_PRIORITY_LOW=%lu)",
          hdma_i2c_rx->Init.Priority, (uint32_t)DMA_PRIORITY_LOW);

    LOG_I("======================================");
}

/**
 * @brief NVIC 인터럽트 우선순위 설정 검증
 *
 * DMA 스트림 우선순위(하드웨어 아비터)와
 * NVIC 인터럽트 우선순위(CPU 서비스 순서)는 별개입니다.
 * 두 가지 모두 올바르게 설정해야 합니다.
 */
void MultiDma_PrintNvicConfig(void)
{
    uint32_t pri_spi_dma, pri_uart_dma, pri_i2c_dma_tx, pri_i2c_dma_rx;
    uint32_t sub_spi_dma, sub_uart_dma, sub_i2c_dma_tx, sub_i2c_dma_rx;

    /* NVIC 우선순위 읽기 */
    HAL_NVIC_GetPriority(DMA2_Stream3_IRQn, NVIC_PRIORITYGROUP_4,
                         &pri_spi_dma, &sub_spi_dma);
    HAL_NVIC_GetPriority(DMA1_Stream6_IRQn, NVIC_PRIORITYGROUP_4,
                         &pri_uart_dma, &sub_uart_dma);
    HAL_NVIC_GetPriority(DMA1_Stream7_IRQn, NVIC_PRIORITYGROUP_4,
                         &pri_i2c_dma_tx, &sub_i2c_dma_tx);
    HAL_NVIC_GetPriority(DMA1_Stream0_IRQn, NVIC_PRIORITYGROUP_4,
                         &pri_i2c_dma_rx, &sub_i2c_dma_rx);

    LOG_I("=== NVIC 인터럽트 우선순위 (낮은 숫자 = 높은 우선순위) ===");
    LOG_I("SPI1 TX DMA  (DMA2_S3): PreemptPriority=%lu", pri_spi_dma);
    LOG_I("UART2 TX DMA (DMA1_S6): PreemptPriority=%lu", pri_uart_dma);
    LOG_I("I2C1 TX DMA  (DMA1_S7): PreemptPriority=%lu", pri_i2c_dma_tx);
    LOG_I("I2C1 RX DMA  (DMA1_S0): PreemptPriority=%lu", pri_i2c_dma_rx);

    /* 권장값 검증 */
    /* SPI=5, UART=6, I2C_TX=7, I2C_RX=7 (숫자가 작을수록 우선) */
    if (pri_spi_dma < pri_uart_dma && pri_uart_dma <= pri_i2c_dma_tx) {
        LOG_I("NVIC 우선순위 설정 검증: OK (SPI > UART > I2C 순서 확인)");
    } else {
        LOG_W("NVIC 우선순위 설정 검증: 경고 — 권장 순서(SPI>UART>I2C) 미준수");
    }
}

/**
 * @brief 스트림 충돌 감지 (개발 시 검증용)
 *
 * DMA1 Stream6을 I2C1 TX와 UART2 TX가 동시에 사용하는 잘못된 설정을
 * 런타임에서 감지하는 헬퍼 함수입니다.
 *
 * @param hdma_i2c_tx  I2C1 TX DMA 핸들
 * @param hdma_uart_tx UART2 TX DMA 핸들
 * @return true: 충돌 감지됨 (동일 스트림)
 */
bool MultiDma_CheckStreamConflict(const DMA_HandleTypeDef *hdma_i2c_tx,
                                   const DMA_HandleTypeDef *hdma_uart_tx)
{
    /* 두 핸들이 동일한 DMA 스트림 인스턴스를 가리키는지 검사 */
    if (hdma_i2c_tx->Instance == hdma_uart_tx->Instance) {
        LOG_E("DMA 스트림 충돌 감지! I2C1 TX와 UART2 TX가 동일 스트림 사용!");
        LOG_E("  I2C1 TX: Instance=0x%08lX", (uint32_t)hdma_i2c_tx->Instance);
        LOG_E("  UART2 TX: Instance=0x%08lX", (uint32_t)hdma_uart_tx->Instance);
        LOG_E("해결: CubeMX에서 I2C1 TX를 DMA1 Stream7 Ch1으로 변경하세요");
        return true;
    }

    LOG_D("DMA 스트림 충돌 없음 확인");
    return false;
}

/**
 * @brief 다중 DMA 시스템 진단 — 초기화 후 1회 실행 권장
 */
void MultiDma_Diagnose(DMA_HandleTypeDef *hdma_spi_tx,
                        DMA_HandleTypeDef *hdma_uart_tx,
                        DMA_HandleTypeDef *hdma_i2c_tx,
                        DMA_HandleTypeDef *hdma_i2c_rx)
{
    LOG_I("MultiDma_Diagnose: v1.4 DMA 설정 진단 시작");

    MultiDma_PrintPriorityConfig(hdma_spi_tx, hdma_uart_tx,
                                  hdma_i2c_tx, hdma_i2c_rx);
    MultiDma_PrintNvicConfig();

    bool conflict = MultiDma_CheckStreamConflict(hdma_i2c_tx, hdma_uart_tx);
    if (conflict) {
        LOG_E("MultiDma_Diagnose: 치명적 오류 — CubeMX 설정을 수정하세요");
    } else {
        LOG_I("MultiDma_Diagnose: 진단 완료 — 설정 정상");
    }
}
