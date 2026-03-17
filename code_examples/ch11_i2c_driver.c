/**
 * ch11_i2c_driver.c
 * I2C Driver 구현 — HAL 추상화 레이어
 */

#include "ch11_i2c_driver.h"
#include "main.h"
#include "log.h"

/**
 * @brief I2C 마스터 송신 (폴링)
 */
i2c_status_t i2c_master_transmit(uint16_t dev_addr, const uint8_t *data,
                                  uint16_t len, uint32_t timeout_ms)
{
    HAL_StatusTypeDef status;

    LOG_D("i2c_master_transmit: addr=0x%02X, len=%d", dev_addr, len);

    // HAL_I2C_Master_Transmit: 마스터 송신 (폴링)
    // - &hi2c2: STM32CubeIDE가 생성한 I2C2 핸들
    // - (dev_addr << 1): 7비트 주소를 8비트로 변환 (비트 0에 0 추가 = 송신)
    // - (uint8_t *)data: 송신 데이터
    // - len: 송신 바이트 수
    // - timeout_ms: 타임아웃
    status = HAL_I2C_Master_Transmit(&hi2c2, (dev_addr << 1),
                                      (uint8_t *)data, len, timeout_ms);

    if (status == HAL_OK) {
        LOG_I("i2c_master_transmit: SUCCESS");
        return I2C_OK;
    } else if (status == HAL_TIMEOUT) {
        LOG_W("i2c_master_transmit: TIMEOUT");
        return I2C_TIMEOUT;
    } else {
        LOG_E("i2c_master_transmit: ERROR (status=%d)", status);
        return I2C_ERROR;
    }
}

/**
 * @brief I2C 마스터 수신 (폴링)
 */
i2c_status_t i2c_master_receive(uint16_t dev_addr, uint8_t *data,
                                 uint16_t len, uint32_t timeout_ms)
{
    HAL_StatusTypeDef status;

    LOG_D("i2c_master_receive: addr=0x%02X, len=%d", dev_addr, len);

    // HAL_I2C_Master_Receive: 마스터 수신 (폴링)
    // - (dev_addr << 1): 비트 0에 1이 자동 추가되어 읽기 주소가 됨
    status = HAL_I2C_Master_Receive(&hi2c2, (dev_addr << 1),
                                     data, len, timeout_ms);

    if (status == HAL_OK) {
        LOG_I("i2c_master_receive: SUCCESS");
        return I2C_OK;
    } else if (status == HAL_TIMEOUT) {
        LOG_W("i2c_master_receive: TIMEOUT");
        return I2C_TIMEOUT;
    } else {
        LOG_E("i2c_master_receive: ERROR (status=%d)", status);
        return I2C_ERROR;
    }
}
