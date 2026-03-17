/**
 * @file    ch14_i2c_dma_driver.c
 * @brief   I2C Driver — DMA 모드 구현 (Ch14 v1.4)
 *
 * Ch11의 HAL_I2C_Master_Transmit/Receive(폴링)을
 * HAL_I2C_Master_Transmit_DMA / Receive_DMA 방식으로 업그레이드합니다.
 *
 * 핵심 변경 사항:
 *   - 블로킹 호출 → 비동기 호출 + 콜백 체인
 *   - 드라이버 상태 머신(FSM) 추가
 *   - 오류 복구 로직 추가
 *
 * @note    STM32F411RE, HAL 라이브러리, DMA1 Stream7(TX)/Stream0(RX)
 * @version v1.4
 */

#include "ch14_i2c_dma_driver.h"
#include "log.h"   /* Ch02 로그 매크로: LOG_D / LOG_I / LOG_W / LOG_E */

/* ===== 전역 드라이버 인스턴스 (단일 I2C1 사용 기준) ===== */
static I2cDmaHandle *g_i2c_drv = NULL;  /**< HAL 콜백에서 접근하기 위한 전역 포인터 */

/* ===================================================================
 * 초기화
 * =================================================================== */
HAL_StatusTypeDef I2cDma_Init(I2cDmaHandle *drv, I2C_HandleTypeDef *hi2c)
{
    if (drv == NULL || hi2c == NULL) {
        LOG_E("I2cDma_Init: NULL 포인터 인자");
        return HAL_ERROR;
    }

    drv->hi2c       = hi2c;
    drv->state      = I2C_DMA_STATE_IDLE;
    drv->tx_cplt_cb = NULL;
    drv->rx_cplt_cb = NULL;
    drv->error_cb   = NULL;
    drv->tx_count   = 0;
    drv->rx_count   = 0;
    drv->err_count  = 0;

    /* 전역 포인터 등록 — HAL 콜백에서 드라이버 핸들 조회에 사용 */
    g_i2c_drv = drv;

    LOG_I("I2cDma_Init: I2C DMA 드라이버 초기화 완료");
    return HAL_OK;
}

/* ===================================================================
 * 비동기 TX (명령 전송)
 * =================================================================== */
HAL_StatusTypeDef I2cDma_MasterTransmit(I2cDmaHandle     *drv,
                                         uint16_t          dev_addr,
                                         uint8_t          *p_data,
                                         uint16_t          size,
                                         I2cDmaTxCpltCb    tx_cb)
{
    HAL_StatusTypeDef ret;

    if (drv->state != I2C_DMA_STATE_IDLE) {
        LOG_W("I2cDma_MasterTransmit: 드라이버 BUSY (state=%d)", drv->state);
        return HAL_BUSY;
    }

    /* 콜백 등록 */
    drv->tx_cplt_cb = tx_cb;

    /* 상태 전이: IDLE → TX_BUSY */
    drv->state = I2C_DMA_STATE_TX_BUSY;

    LOG_D("I2cDma_MasterTransmit: addr=0x%02X, size=%d", dev_addr, size);

    /* HAL DMA 전송 시작 — 즉시 반환 */
    ret = HAL_I2C_Master_Transmit_DMA(drv->hi2c, dev_addr, p_data, size);
    if (ret != HAL_OK) {
        drv->state = I2C_DMA_STATE_ERROR;
        drv->err_count++;
        LOG_E("I2cDma_MasterTransmit: HAL 오류 (ret=%d, err_count=%lu)",
              ret, drv->err_count);
    }

    return ret;
}

/* ===================================================================
 * 비동기 RX (데이터 수신)
 * =================================================================== */
HAL_StatusTypeDef I2cDma_MasterReceive(I2cDmaHandle     *drv,
                                        uint16_t          dev_addr,
                                        uint8_t          *p_data,
                                        uint16_t          size,
                                        I2cDmaRxCpltCb    rx_cb)
{
    HAL_StatusTypeDef ret;

    if (drv->state != I2C_DMA_STATE_IDLE) {
        LOG_W("I2cDma_MasterReceive: 드라이버 BUSY (state=%d)", drv->state);
        return HAL_BUSY;
    }

    /* 콜백 등록 */
    drv->rx_cplt_cb = rx_cb;

    /* 상태 전이: IDLE → RX_BUSY */
    drv->state = I2C_DMA_STATE_RX_BUSY;

    LOG_D("I2cDma_MasterReceive: addr=0x%02X, size=%d", dev_addr, size);

    /* HAL DMA 수신 시작 — 즉시 반환 */
    ret = HAL_I2C_Master_Receive_DMA(drv->hi2c, dev_addr, p_data, size);
    if (ret != HAL_OK) {
        drv->state = I2C_DMA_STATE_ERROR;
        drv->err_count++;
        LOG_E("I2cDma_MasterReceive: HAL 오류 (ret=%d, err_count=%lu)",
              ret, drv->err_count);
    }

    return ret;
}

/* ===================================================================
 * 상태 조회 / 리셋
 * =================================================================== */
I2cDmaState I2cDma_GetState(const I2cDmaHandle *drv)
{
    return drv->state;
}

void I2cDma_Reset(I2cDmaHandle *drv)
{
    LOG_W("I2cDma_Reset: 드라이버 강제 리셋 (이전 상태=%d, err=%lu)",
          drv->state, drv->err_count);

    /* HAL I2C 강제 초기화 — 버스 잠금(lock) 해제 */
    HAL_I2C_DeInit(drv->hi2c);
    HAL_I2C_Init(drv->hi2c);

    drv->state      = I2C_DMA_STATE_IDLE;
    drv->tx_cplt_cb = NULL;
    drv->rx_cplt_cb = NULL;

    LOG_I("I2cDma_Reset: 리셋 완료");
}

/* ===================================================================
 * HAL 인터럽트 핸들러 (DMA ISR에서 호출)
 * =================================================================== */

/**
 * TX 완료 처리:
 *   1. 상태 IDLE 복귀
 *   2. 등록된 사용자 콜백 호출
 */
void I2cDma_TxCpltHandler(I2cDmaHandle *drv)
{
    drv->tx_count++;
    drv->state = I2C_DMA_STATE_IDLE;

    LOG_D("I2cDma_TxCpltHandler: TX 완료 (누적=%lu)", drv->tx_count);

    if (drv->tx_cplt_cb != NULL) {
        drv->tx_cplt_cb();   /* 사용자 콜백 호출 (예: SHT31_OnTxCplt) */
    }
}

/**
 * RX 완료 처리:
 *   1. 상태 IDLE 복귀
 *   2. 등록된 사용자 콜백 호출
 */
void I2cDma_RxCpltHandler(I2cDmaHandle *drv)
{
    drv->rx_count++;
    drv->state = I2C_DMA_STATE_IDLE;

    LOG_D("I2cDma_RxCpltHandler: RX 완료 (누적=%lu)", drv->rx_count);

    if (drv->rx_cplt_cb != NULL) {
        drv->rx_cplt_cb();   /* 사용자 콜백 호출 (예: SHT31_OnRxCplt) */
    }
}

/**
 * 오류 처리:
 *   1. 상태 ERROR 전이
 *   2. 오류 콜백 호출
 */
void I2cDma_ErrorHandler(I2cDmaHandle *drv)
{
    drv->err_count++;
    drv->state = I2C_DMA_STATE_ERROR;

    LOG_E("I2cDma_ErrorHandler: I2C DMA 오류 (HAL ErrorCode=0x%08lX, 누적=%lu)",
          drv->hi2c->ErrorCode, drv->err_count);

    if (drv->error_cb != NULL) {
        drv->error_cb();
    }
}

/* ===================================================================
 * HAL 콜백 — STM32 HAL이 자동 호출 (ISR 컨텍스트)
 * =================================================================== */

/**
 * @brief I2C Master TX 완료 — HAL 자동 호출
 * @note  이 함수는 DMA 인터럽트 컨텍스트에서 실행됩니다.
 *        가능한 한 빠르게 처리하고, 긴 작업은 플래그로 메인 루프에 위임하세요.
 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1 && g_i2c_drv != NULL) {
        I2cDma_TxCpltHandler(g_i2c_drv);
    }
}

/**
 * @brief I2C Master RX 완료 — HAL 자동 호출
 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1 && g_i2c_drv != NULL) {
        I2cDma_RxCpltHandler(g_i2c_drv);
    }
}

/**
 * @brief I2C 오류 — HAL 자동 호출
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1 && g_i2c_drv != NULL) {
        I2cDma_ErrorHandler(g_i2c_drv);
    }
}
