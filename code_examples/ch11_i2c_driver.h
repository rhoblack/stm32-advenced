/**
 * ch11_i2c_driver.h
 * I2C Driver 헤더 — HAL 추상화 레이어
 *
 * 역할: HAL_I2C_* 함수를 래핑하여 상위 레이어(Device Driver)에
 *       깔끔한 I2C 인터페이스를 제공합니다.
 */

#ifndef CH11_I2C_DRIVER_H
#define CH11_I2C_DRIVER_H

#include <stdint.h>

/* I2C 상태 코드 */
typedef int32_t i2c_status_t;
#define I2C_OK          0
#define I2C_ERROR      -1
#define I2C_TIMEOUT    -2

/**
 * @brief I2C 마스터 송신 (폴링, 블로킹)
 *
 * 마스터가 슬레이브에게 데이터를 보냅니다.
 *
 * @param dev_addr I2C 슬레이브 주소 (7비트, 예: 0x44)
 * @param data 송신 데이터 포인터
 * @param len 송신 길이 (바이트)
 * @param timeout_ms 타임아웃 (밀리초)
 * @return I2C_OK (0), I2C_ERROR (-1), 또는 I2C_TIMEOUT (-2)
 */
i2c_status_t i2c_master_transmit(uint16_t dev_addr, const uint8_t *data,
                                  uint16_t len, uint32_t timeout_ms);

/**
 * @brief I2C 마스터 수신 (폴링, 블로킹)
 *
 * 마스터가 슬레이브로부터 데이터를 받습니다.
 *
 * @param dev_addr I2C 슬레이브 주소 (7비트)
 * @param data 수신 데이터 버퍼 (미리 할당)
 * @param len 수신 길이 (바이트)
 * @param timeout_ms 타임아웃
 * @return I2C_OK (0), I2C_ERROR (-1), 또는 I2C_TIMEOUT (-2)
 */
i2c_status_t i2c_master_receive(uint16_t dev_addr, uint8_t *data,
                                 uint16_t len, uint32_t timeout_ms);

#endif  // CH11_I2C_DRIVER_H
