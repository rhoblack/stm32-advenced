/**
 * ch11_sht31_driver.c
 * SHT31 Device Driver 구현
 */

#include "ch11_sht31_driver.h"
#include "ch11_i2c_driver.h"
#include "ch11_ex03_crc8.c"  // CRC 함수 포함 (또는 별도 헤더)
#include "main.h"
#include "log.h"

/**
 * @brief SHT31 초기화
 */
sht31_status_t sht31_init(void)
{
    uint8_t cmd[] = SHT31_MEAS_CMD;

    LOG_I("sht31_init: SHT31 센서 초기화 시작");

    // 더미 송신으로 센서 응답 확인
    if (i2c_master_transmit(SHT31_ADDR, cmd, sizeof(cmd), 1000) == I2C_OK) {
        LOG_I("sht31_init: SHT31 센서 응답 OK");
        return SHT31_OK;
    } else {
        LOG_E("sht31_init: SHT31 센서 응답 실패");
        return SHT31_ERROR;
    }
}

/**
 * @brief SHT31 온습도 데이터 읽기 (블로킹)
 */
sht31_status_t sht31_read_temp_humid(float *p_temp, float *p_humid)
{
    uint8_t cmd[] = SHT31_MEAS_CMD;
    uint8_t response[SHT31_RESP_LEN];
    uint16_t temp_raw, humid_raw;
    i2c_status_t ret;

    LOG_D("sht31_read_temp_humid: 측정 시작");

    // Step 1: 측정 명령 송신
    ret = i2c_master_transmit(SHT31_ADDR, cmd, sizeof(cmd), 1000);
    if (ret != I2C_OK) {
        LOG_E("sht31_read_temp_humid: 명령 송신 실패");
        return SHT31_ERROR;
    }

    // Step 2: 센서 측정 대기 (약 50ms)
    LOG_D("sht31_read_temp_humid: 측정 대기 중 (50ms)");
    HAL_Delay(50);

    // Step 3: 데이터 수신 (6바이트)
    ret = i2c_master_receive(SHT31_ADDR, response, SHT31_RESP_LEN, 1000);
    if (ret != I2C_OK) {
        LOG_E("sht31_read_temp_humid: 데이터 수신 실패");
        return SHT31_ERROR;
    }

    // Step 4: CRC 검증 (온도)
    if (crc8_verify_sht31(&response[0], 3) != 0) {
        LOG_E("sht31_read_temp_humid: 온도 CRC 불일치");
        return SHT31_CRC_ERR;
    }

    // Step 4: CRC 검증 (습도)
    if (crc8_verify_sht31(&response[3], 3) != 0) {
        LOG_E("sht31_read_temp_humid: 습도 CRC 불일치");
        return SHT31_CRC_ERR;
    }

    // Step 5: 온습도 계산
    temp_raw = ((uint16_t)response[0] << 8) | response[1];
    humid_raw = ((uint16_t)response[3] << 8) | response[4];

    *p_temp = (175.0f / 65535.0f) * (float)temp_raw - 45.0f;
    *p_humid = (100.0f / 65535.0f) * (float)humid_raw;

    LOG_I("sht31_read_temp_humid: temp=%.2f°C, humid=%.2f%%",
          *p_temp, *p_humid);

    return SHT31_OK;
}

/**
 * @brief SHT31 측정 시작 (비블로킹)
 */
sht31_status_t sht31_trigger_measurement(void)
{
    uint8_t cmd[] = SHT31_MEAS_CMD;

    LOG_D("sht31_trigger_measurement: 측정 명령 송신");

    if (i2c_master_transmit(SHT31_ADDR, cmd, sizeof(cmd), 1000) == I2C_OK) {
        return SHT31_OK;
    } else {
        LOG_E("sht31_trigger_measurement: 송신 실패");
        return SHT31_ERROR;
    }
}

/**
 * @brief SHT31 측정 결과 수신 (비블로킹)
 */
sht31_status_t sht31_read_measurement(float *p_temp, float *p_humid)
{
    uint8_t response[SHT31_RESP_LEN];
    uint16_t temp_raw, humid_raw;
    i2c_status_t ret;

    LOG_D("sht31_read_measurement: 데이터 수신");

    // Step 1: 데이터 수신
    ret = i2c_master_receive(SHT31_ADDR, response, SHT31_RESP_LEN, 1000);
    if (ret != I2C_OK) {
        LOG_E("sht31_read_measurement: 수신 실패");
        return SHT31_ERROR;
    }

    // Step 2: CRC 검증 (온도)
    if (crc8_verify_sht31(&response[0], 3) != 0) {
        LOG_E("sht31_read_measurement: 온도 CRC 불일치");
        return SHT31_CRC_ERR;
    }

    // Step 3: CRC 검증 (습도)
    if (crc8_verify_sht31(&response[3], 3) != 0) {
        LOG_E("sht31_read_measurement: 습도 CRC 불일치");
        return SHT31_CRC_ERR;
    }

    // Step 4: 계산
    temp_raw = ((uint16_t)response[0] << 8) | response[1];
    humid_raw = ((uint16_t)response[3] << 8) | response[4];

    *p_temp = (175.0f / 65535.0f) * (float)temp_raw - 45.0f;
    *p_humid = (100.0f / 65535.0f) * (float)humid_raw;

    LOG_I("sht31_read_measurement: temp=%.2f°C, humid=%.2f%%",
          *p_temp, *p_humid);

    return SHT31_OK;
}
