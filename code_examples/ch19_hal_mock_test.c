/**
 * @file    ch19_hal_mock_test.c
 * @brief   HAL Mock 기반 단위 테스트 구현 예시
 * @note    PC(Windows/Linux)에서 빌드 및 실행 가능 — 실제 하드웨어 불필요
 *          빌드: gcc ch19_hal_mock_test.c -o test_runner
 *          실행: ./test_runner
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

/* ============================================================
 * Mock HAL 정의 — 실제 stm32f4xx_hal.h 대신 사용
 * ============================================================ */

typedef enum {
    HAL_OK      = 0x00U,
    HAL_ERROR   = 0x01U,
    HAL_BUSY    = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

typedef struct {
    uint32_t Instance;
} I2C_HandleTypeDef;

/* Mock I2C 제어 변수 — 테스트에서 주입 */
static HAL_StatusTypeDef g_mock_i2c_tx_result = HAL_OK;
static HAL_StatusTypeDef g_mock_i2c_rx_result = HAL_OK;
static uint8_t g_mock_rx_data[6]  = {0};
static uint8_t g_mock_tx_cmd[2]   = {0};
static int     g_mock_tx_called   = 0;
static int     g_mock_rx_called   = 0;

/* Mock HAL_I2C_Master_Transmit */
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c,
                                           uint16_t DevAddress,
                                           uint8_t *pData,
                                           uint16_t Size,
                                           uint32_t Timeout)
{
    (void)hi2c;
    (void)DevAddress;
    (void)Timeout;

    g_mock_tx_called++;
    if (Size >= 2U) {
        g_mock_tx_cmd[0] = pData[0];
        g_mock_tx_cmd[1] = pData[1];
    }
    printf("[MOCK] HAL_I2C_Master_Transmit: cmd=%02X %02X → ret=%d\n",
           pData[0], pData[1], g_mock_i2c_tx_result);
    return g_mock_i2c_tx_result;
}

/* Mock HAL_I2C_Master_Receive */
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c,
                                          uint16_t DevAddress,
                                          uint8_t *pData,
                                          uint16_t Size,
                                          uint32_t Timeout)
{
    (void)hi2c;
    (void)DevAddress;
    (void)Timeout;

    g_mock_rx_called++;
    memcpy(pData, g_mock_rx_data, (size_t)Size);
    printf("[MOCK] HAL_I2C_Master_Receive: data=%02X %02X %02X → ret=%d\n",
           g_mock_rx_data[0], g_mock_rx_data[1], g_mock_rx_data[2],
           g_mock_i2c_rx_result);
    return g_mock_i2c_rx_result;
}

/* Mock HAL_Delay — PC에서 실시간 지연 없이 즉시 반환 */
void HAL_Delay(uint32_t Delay) {
    (void)Delay;
    /* 테스트에서는 지연 없이 통과 */
}

/* ============================================================
 * 테스트 대상 코드 (실제 sht31_driver.c와 동일)
 * ============================================================ */

I2C_HandleTypeDef hi2c1 = {0};

#define SHT31_ADDR_WRITE    (0x44U << 1U)
#define SHT31_ADDR_READ     (0x44U << 1U | 0x01U)
#define SHT31_CMD_MEAS_H    0x2CU
#define SHT31_CMD_MEAS_L    0x06U
#define SHT31_DATA_SIZE     6U

/**
 * @brief SHT31 온습도 원시 데이터 읽기
 */
HAL_StatusTypeDef SHT31_ReadRaw(uint8_t *raw_buf)
{
    HAL_StatusTypeDef ret;
    uint8_t cmd[2] = { SHT31_CMD_MEAS_H, SHT31_CMD_MEAS_L };

    ret = HAL_I2C_Master_Transmit(&hi2c1, SHT31_ADDR_WRITE, cmd, 2U, 100U);
    if (ret != HAL_OK) {
        return ret;
    }

    HAL_Delay(15U);

    ret = HAL_I2C_Master_Receive(&hi2c1, SHT31_ADDR_READ, raw_buf, SHT31_DATA_SIZE, 100U);
    return ret;
}

/**
 * @brief SHT31 원시 데이터를 온도(°C)로 변환
 */
float SHT31_ConvertTemp(const uint8_t *raw_buf)
{
    uint16_t raw_temp = (uint16_t)((raw_buf[0] << 8U) | raw_buf[1]);
    return (float)(-45.0f + 175.0f * (float)raw_temp / 65535.0f);
}

/**
 * @brief SHT31 원시 데이터를 습도(%)로 변환
 */
float SHT31_ConvertHumi(const uint8_t *raw_buf)
{
    uint16_t raw_humi = (uint16_t)((raw_buf[3] << 8U) | raw_buf[4]);
    return (float)(100.0f * (float)raw_humi / 65535.0f);
}

/* ============================================================
 * 테스트 케이스 — Given / When / Then 패턴
 * ============================================================ */

static int g_pass = 0;
static int g_fail = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            printf("  [PASS] %s\n", msg); \
            g_pass++; \
        } else { \
            printf("  [FAIL] %s\n", msg); \
            g_fail++; \
        } \
    } while (0)

/* 테스트 픽스처 초기화 */
static void test_setup(void)
{
    g_mock_i2c_tx_result = HAL_OK;
    g_mock_i2c_rx_result = HAL_OK;
    g_mock_tx_called     = 0;
    g_mock_rx_called     = 0;
    memset(g_mock_rx_data, 0, sizeof(g_mock_rx_data));
}

/* ─── TC01: 정상 측정 ─── */
void test_TC01_normal_read(void)
{
    printf("\n[TC01] SHT31 정상 읽기\n");
    test_setup();

    /* Given: 25.0°C, 50% RH에 해당하는 원시 데이터 설정 */
    /* temp = (25 + 45) / 175 * 65535 = 26214 = 0x6666 */
    g_mock_rx_data[0] = 0x66U;
    g_mock_rx_data[1] = 0x66U;
    g_mock_rx_data[2] = 0xFFU;  /* CRC (테스트용 더미) */
    /* humi = 50 / 100 * 65535 = 32768 = 0x8000 */
    g_mock_rx_data[3] = 0x80U;
    g_mock_rx_data[4] = 0x00U;
    g_mock_rx_data[5] = 0xFFU;  /* CRC (테스트용 더미) */

    /* When: 센서 읽기 실행 */
    uint8_t raw[6];
    HAL_StatusTypeDef ret = SHT31_ReadRaw(raw);

    /* Then: 반환값과 호출 횟수, 온도/습도 값 검증 */
    TEST_ASSERT(ret == HAL_OK,            "HAL_OK 반환");
    TEST_ASSERT(g_mock_tx_called == 1,    "Transmit 1회 호출");
    TEST_ASSERT(g_mock_rx_called == 1,    "Receive 1회 호출");
    TEST_ASSERT(g_mock_tx_cmd[0] == 0x2C, "측정 명령 바이트0 = 0x2C");
    TEST_ASSERT(g_mock_tx_cmd[1] == 0x06, "측정 명령 바이트1 = 0x06");

    float temp = SHT31_ConvertTemp(raw);
    float humi = SHT31_ConvertHumi(raw);
    TEST_ASSERT(temp > 24.5f && temp < 25.5f, "온도 ≈ 25°C");
    TEST_ASSERT(humi > 49.0f && humi < 51.0f, "습도 ≈ 50%");
    printf("  온도=%.2f°C, 습도=%.2f%%\n", temp, humi);
}

/* ─── TC02: I2C Transmit 오류 시 동작 ─── */
void test_TC02_i2c_tx_error(void)
{
    printf("\n[TC02] I2C Transmit 오류 처리\n");
    test_setup();

    /* Given: Mock이 HAL_ERROR를 반환하도록 설정 */
    g_mock_i2c_tx_result = HAL_ERROR;

    /* When: 센서 읽기 시도 */
    uint8_t raw[6];
    HAL_StatusTypeDef ret = SHT31_ReadRaw(raw);

    /* Then: HAL_ERROR 반환, Receive는 호출되지 않아야 함 */
    TEST_ASSERT(ret == HAL_ERROR,         "HAL_ERROR 반환");
    TEST_ASSERT(g_mock_tx_called == 1,    "Transmit 1회 호출");
    TEST_ASSERT(g_mock_rx_called == 0,    "Receive 미호출 (오류 후 조기 반환)");
}

/* ─── TC03: I2C Receive 오류 시 동작 ─── */
void test_TC03_i2c_rx_error(void)
{
    printf("\n[TC03] I2C Receive 오류 처리\n");
    test_setup();

    /* Given: Transmit 성공, Receive 실패 설정 */
    g_mock_i2c_tx_result = HAL_OK;
    g_mock_i2c_rx_result = HAL_TIMEOUT;

    /* When: 센서 읽기 시도 */
    uint8_t raw[6];
    HAL_StatusTypeDef ret = SHT31_ReadRaw(raw);

    /* Then: HAL_TIMEOUT 반환 */
    TEST_ASSERT(ret == HAL_TIMEOUT,       "HAL_TIMEOUT 반환");
    TEST_ASSERT(g_mock_tx_called == 1,    "Transmit 1회 호출");
    TEST_ASSERT(g_mock_rx_called == 1,    "Receive 1회 호출");
}

/* ─── TC04: 온도 변환 경계값 ─── */
void test_TC04_temp_boundary(void)
{
    printf("\n[TC04] 온도 변환 경계값 (-40°C, 85°C)\n");
    test_setup();

    /* Given: raw=0x0000 → -45°C + 0 = -45°C (최솟값) */
    uint8_t raw_min[6] = {0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF};
    float temp_min = SHT31_ConvertTemp(raw_min);
    TEST_ASSERT(temp_min < -44.0f,        "raw=0x0000 → 약 -45°C");

    /* Given: raw=0xFFFF → -45°C + 175 = 130°C (최댓값) */
    uint8_t raw_max[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    float temp_max = SHT31_ConvertTemp(raw_max);
    TEST_ASSERT(temp_max > 129.0f,        "raw=0xFFFF → 약 130°C");

    printf("  최솟값=%.2f°C, 최댓값=%.2f°C\n", temp_min, temp_max);
}

/* ============================================================
 * 테스트 실행 진입점
 * ============================================================ */

int main(void)
{
    printf("====================================================\n");
    printf("  Ch19 — HAL Mock 단위 테스트 실행\n");
    printf("  대상: SHT31 Driver (sht31_driver.c)\n");
    printf("====================================================\n");

    test_TC01_normal_read();
    test_TC02_i2c_tx_error();
    test_TC03_i2c_rx_error();
    test_TC04_temp_boundary();

    printf("\n====================================================\n");
    printf("  결과: PASS=%d / FAIL=%d / TOTAL=%d\n",
           g_pass, g_fail, g_pass + g_fail);
    if (g_fail == 0) {
        printf("  모든 테스트 통과! (v2.2 품질 인증)\n");
    } else {
        printf("  실패한 테스트가 있습니다. 코드를 수정하세요.\n");
    }
    printf("====================================================\n");

    return (g_fail == 0) ? 0 : 1;
}
