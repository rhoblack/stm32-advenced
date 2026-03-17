/**
 * @file    ch14_i2c_dma_driver.h
 * @brief   I2C Driver — DMA 모드 인터페이스 (Ch14 v1.4)
 *
 * Ch11의 폴링 방식 i2c_driver.h를 DMA 비동기 방식으로 업그레이드합니다.
 * 상위 레이어(sht31_driver.c)는 이 인터페이스만 사용하면 됩니다.
 *
 * @note    STM32F411RE / HAL 라이브러리 기준
 * @version v1.4
 */

#ifndef CH14_I2C_DMA_DRIVER_H
#define CH14_I2C_DMA_DRIVER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ===== I2C DMA 드라이버 상태 (FSM) ===== */
typedef enum {
    I2C_DMA_STATE_IDLE      = 0,    /**< 대기 중 — 새 전송 가능 */
    I2C_DMA_STATE_TX_BUSY   = 1,    /**< DMA TX 진행 중 */
    I2C_DMA_STATE_RX_BUSY   = 2,    /**< DMA RX 진행 중 */
    I2C_DMA_STATE_ERROR     = 3,    /**< 오류 발생 — 재초기화 필요 */
} I2cDmaState;

/* ===== I2C DMA 완료 콜백 타입 ===== */
typedef void (*I2cDmaTxCpltCb)(void);  /**< TX 완료 콜백 */
typedef void (*I2cDmaRxCpltCb)(void);  /**< RX 완료 콜백 */
typedef void (*I2cDmaErrorCb)(void);   /**< 오류 콜백 */

/* ===== I2C DMA 드라이버 핸들 ===== */
typedef struct {
    I2C_HandleTypeDef *hi2c;        /**< HAL I2C 핸들 */
    I2cDmaState        state;       /**< 현재 드라이버 상태 */
    I2cDmaTxCpltCb     tx_cplt_cb; /**< TX 완료 사용자 콜백 */
    I2cDmaRxCpltCb     rx_cplt_cb; /**< RX 완료 사용자 콜백 */
    I2cDmaErrorCb      error_cb;   /**< 오류 사용자 콜백 */
    uint32_t           tx_count;   /**< 누적 TX 완료 횟수 (디버그용) */
    uint32_t           rx_count;   /**< 누적 RX 완료 횟수 (디버그용) */
    uint32_t           err_count;  /**< 누적 오류 횟수 */
} I2cDmaHandle;

/* ===== 공개 API ===== */

/**
 * @brief I2C DMA 드라이버 초기화
 * @param drv    드라이버 핸들 포인터
 * @param hi2c   HAL I2C 핸들 (CubeMX 생성)
 * @return HAL_OK 또는 HAL_ERROR
 */
HAL_StatusTypeDef I2cDma_Init(I2cDmaHandle *drv, I2C_HandleTypeDef *hi2c);

/**
 * @brief I2C DMA 비동기 송신 시작
 * @param drv       드라이버 핸들
 * @param dev_addr  I2C 슬레이브 주소 (7비트 << 1)
 * @param p_data    송신 데이터 포인터
 * @param size      송신 바이트 수
 * @param tx_cb     TX 완료 콜백 (NULL 허용)
 * @return HAL_OK, HAL_BUSY(이전 전송 중), HAL_ERROR
 *
 * @note 이 함수는 즉시 반환됩니다. 완료는 tx_cb로 통보됩니다.
 */
HAL_StatusTypeDef I2cDma_MasterTransmit(I2cDmaHandle     *drv,
                                         uint16_t          dev_addr,
                                         uint8_t          *p_data,
                                         uint16_t          size,
                                         I2cDmaTxCpltCb    tx_cb);

/**
 * @brief I2C DMA 비동기 수신 시작
 * @param drv       드라이버 핸들
 * @param dev_addr  I2C 슬레이브 주소 (7비트 << 1)
 * @param p_data    수신 버퍼 포인터
 * @param size      수신 바이트 수
 * @param rx_cb     RX 완료 콜백 (NULL 허용)
 * @return HAL_OK, HAL_BUSY(이전 수신 중), HAL_ERROR
 *
 * @note TX 완료 콜백에서 호출 시 최소 1ms 후 호출 권장 (HAL 상태 안정화)
 */
HAL_StatusTypeDef I2cDma_MasterReceive(I2cDmaHandle     *drv,
                                        uint16_t          dev_addr,
                                        uint8_t          *p_data,
                                        uint16_t          size,
                                        I2cDmaRxCpltCb    rx_cb);

/**
 * @brief 현재 드라이버 상태 반환
 * @param drv  드라이버 핸들
 * @return I2cDmaState 열거형 값
 */
I2cDmaState I2cDma_GetState(const I2cDmaHandle *drv);

/**
 * @brief I2C DMA 드라이버 오류 초기화 (에러 상태 복구)
 * @param drv  드라이버 핸들
 */
void I2cDma_Reset(I2cDmaHandle *drv);

/**
 * @brief HAL TX 완료 인터럽트 처리 — HAL_I2C_MasterTxCpltCallback에서 호출
 * @param drv  드라이버 핸들
 */
void I2cDma_TxCpltHandler(I2cDmaHandle *drv);

/**
 * @brief HAL RX 완료 인터럽트 처리 — HAL_I2C_MasterRxCpltCallback에서 호출
 * @param drv  드라이버 핸들
 */
void I2cDma_RxCpltHandler(I2cDmaHandle *drv);

/**
 * @brief HAL 오류 인터럽트 처리 — HAL_I2C_ErrorCallback에서 호출
 * @param drv  드라이버 핸들
 */
void I2cDma_ErrorHandler(I2cDmaHandle *drv);

#endif /* CH14_I2C_DMA_DRIVER_H */
