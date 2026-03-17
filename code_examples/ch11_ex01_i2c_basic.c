/**
 * Ch11_ex01_i2c_basic.c
 * I2C 폴링 기초: 마스터 송수신
 *
 * 목표: HAL_I2C_Master_Transmit/Receive로 기본 I2C 통신 구현
 * 하드웨어: NUCLEO-F411RE, I2C2 (PB10/PB3, 풀업 저항 4.7kΩ)
 */

#include "main.h"
#include "log.h"

#define I2C_TIMEOUT_MS  1000
#define SLAVE_ADDR      0x44  // SHT31 I2C 주소

/**
 * @brief I2C 마스터 송신 (폴링, 블로킹)
 *
 * 마스터가 슬레이브(센서)에게 명령 데이터를 보냅니다.
 * 예: SHT31 측정 명령 0x24, 0x00
 *
 * @param dev_addr 슬레이브 주소 (7비트, 예: 0x44)
 * @param data 송신 데이터 포인터
 * @param len 송신 바이트 수
 * @param timeout_ms 타임아웃 (밀리초)
 * @return 0 (OK), -1 (ERROR), -2 (TIMEOUT)
 */
int32_t i2c_master_transmit(uint16_t dev_addr, const uint8_t *data,
                             uint16_t len, uint32_t timeout_ms)
{
    HAL_StatusTypeDef status;

    LOG_D("i2c_master_transmit: addr=0x%02X, len=%d, timeout=%lu ms",
          dev_addr, len, timeout_ms);

    // HAL API 호출: 폴링 송신
    // - hi2c2: STM32CubeIDE가 생성한 I2C 핸들
    // - (dev_addr << 1): HAL은 내부적으로 비트 0에 R/W를 추가 (0=송신)
    // - (uint8_t *)data: 송신 데이터
    // - len: 송신 길이
    // - timeout_ms: 타임아웃
    status = HAL_I2C_Master_Transmit(&hi2c2, (dev_addr << 1),
                                      (uint8_t *)data, len, timeout_ms);

    if (status == HAL_OK) {
        LOG_I("i2c_master_transmit: SUCCESS");
        return 0;
    } else if (status == HAL_TIMEOUT) {
        LOG_W("i2c_master_transmit: TIMEOUT (클럭 스트레칭 또는 버스 장애)");
        return -2;
    } else {
        LOG_E("i2c_master_transmit: HAL_ERROR %d", status);
        return -1;
    }
}

/**
 * @brief I2C 마스터 수신 (폴링, 블로킹)
 *
 * 마스터가 슬레이브(센서)로부터 응답 데이터를 받습니다.
 * 예: SHT31 온습도 데이터 6바이트
 *
 * @param dev_addr 슬레이브 주소 (7비트)
 * @param data 수신 데이터 버퍼 (미리 할당되어야 함)
 * @param len 수신할 바이트 수
 * @param timeout_ms 타임아웃
 * @return 0 (OK), -1 (ERROR), -2 (TIMEOUT)
 */
int32_t i2c_master_receive(uint16_t dev_addr, uint8_t *data,
                            uint16_t len, uint32_t timeout_ms)
{
    HAL_StatusTypeDef status;

    LOG_D("i2c_master_receive: addr=0x%02X, len=%d, timeout=%lu ms",
          dev_addr, len, timeout_ms);

    // HAL API 호출: 폴링 수신
    // - (dev_addr << 1): 비트 0에 1이 자동 추가 (1=수신)
    // - data: 수신 버퍼
    // - len: 수신 길이
    // - timeout_ms: 타임아웃
    status = HAL_I2C_Master_Receive(&hi2c2, (dev_addr << 1),
                                     data, len, timeout_ms);

    if (status == HAL_OK) {
        LOG_I("i2c_master_receive: SUCCESS, data[0]=0x%02X, data[1]=0x%02X",
              data[0], data[1]);
        return 0;
    } else if (status == HAL_TIMEOUT) {
        LOG_W("i2c_master_receive: TIMEOUT");
        return -2;
    } else {
        LOG_E("i2c_master_receive: HAL_ERROR %d", status);
        return -1;
    }
}

/**
 * @brief 테스트: SHT31 센서와 I2C 통신
 *
 * 1. 측정 명령 송신 (0x24, 0x00)
 * 2. 대기 50ms (센서 측정 시간)
 * 3. 응답 수신 (6바이트)
 * 4. 원본 데이터 로깅
 */
void test_sht31_communication(void)
{
    uint8_t cmd[] = {0x24, 0x00};  // SHT31 측정 명령
    uint8_t response[6];            // 응답: 온도(2) + CRC(1) + 습도(2) + CRC(1)
    int32_t ret;

    LOG_I("\n=== I2C Master Communication Test ===");
    LOG_I("Test: SHT31 센서와 I2C 통신");

    // Step 1: 측정 명령 송신
    LOG_I("[Step 1] 측정 명령 송신");
    ret = i2c_master_transmit(SLAVE_ADDR, cmd, sizeof(cmd), I2C_TIMEOUT_MS);
    if (ret != 0) {
        LOG_E("측정 명령 송신 실패!");
        return;
    }

    // Step 2: 센서 측정 대기 (SHT31은 약 15ms 소요, 여유있게 50ms)
    LOG_I("[Step 2] 센서 측정 대기 중 (50ms)...");
    HAL_Delay(50);

    // Step 3: 응답 수신
    LOG_I("[Step 3] 응답 데이터 수신");
    ret = i2c_master_receive(SLAVE_ADDR, response, sizeof(response),
                             I2C_TIMEOUT_MS);
    if (ret != 0) {
        LOG_E("응답 수신 실패!");
        return;
    }

    // Step 4: 원본 데이터 로깅
    LOG_I("[Step 4] 수신 데이터 분석");
    LOG_I("응답 (16진수): ");
    LOG_I("  [0] 0x%02X (온도 상위 바이트)", response[0]);
    LOG_I("  [1] 0x%02X (온도 하위 바이트)", response[1]);
    LOG_I("  [2] 0x%02X (온도 CRC)", response[2]);
    LOG_I("  [3] 0x%02X (습도 상위 바이트)", response[3]);
    LOG_I("  [4] 0x%02X (습도 하위 바이트)", response[4]);
    LOG_I("  [5] 0x%02X (습도 CRC)", response[5]);

    // big-endian 조합
    uint16_t temp_raw = ((uint16_t)response[0] << 8) | response[1];
    uint16_t humid_raw = ((uint16_t)response[3] << 8) | response[4];

    LOG_I("16비트 조합:");
    LOG_I("  온도 (raw): 0x%04X = %u", temp_raw, temp_raw);
    LOG_I("  습도 (raw): 0x%04X = %u", humid_raw, humid_raw);

    // 계산 (실제 CRC 검증 전)
    float temp_c = (175.0f / 65535.0f) * (float)temp_raw - 45.0f;
    float humid_pct = (100.0f / 65535.0f) * (float)humid_raw;

    LOG_I("계산된 값 (CRC 검증 전):");
    LOG_I("  온도: %.2f°C", temp_c);
    LOG_I("  습도: %.2f%%", humid_pct);

    LOG_I("=== Test Complete ===\n");
}

/**
 * @brief main() 에서 호출
 */
void i2c_test_example(void)
{
    // 시스템 초기화 (이전에 완료된 것 가정)
    // HAL_Init(), SystemClock_Config(), MX_GPIO_Init(), MX_I2C2_Init()

    // 로그 초기화 (Ch02)
    log_init();

    // 테스트 실행
    while (1) {
        test_sht31_communication();
        HAL_Delay(2000);  // 2초마다 반복
    }
}
