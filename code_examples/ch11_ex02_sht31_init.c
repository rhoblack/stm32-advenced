/**
 * Ch11_ex02_sht31_init.c
 * SHT31 초기화 및 데이터 읽기
 *
 * 목표: SHT31 센서 초기화 확인 후 블로킹 방식으로 온습도 데이터 읽기
 * 하드웨어: NUCLEO-F411RE, I2C2, SHT31
 */

#include "main.h"
#include "log.h"
#include <string.h>

#define SHT31_ADDR      0x44
#define SHT31_MEAS_CMD  {0x24, 0x00}

/**
 * @brief SHT31 센서 초기화
 *
 * 센서와의 I2C 통신이 정상인지 확인합니다.
 * 더미 측정 명령을 보내서 응답을 확인합니다.
 *
 * @return 0 (OK), -1 (ERROR)
 */
int32_t sht31_init(void)
{
    uint8_t cmd[] = SHT31_MEAS_CMD;
    int32_t ret;

    LOG_I("sht31_init: SHT31 초기화 시작");

    // 더미 송신: 마스터가 센서에 도달 가능한지 확인
    // 센서가 ACK를 보내면 I2C_OK 반환
    ret = i2c_master_transmit(SHT31_ADDR, cmd, sizeof(cmd), 1000);

    if (ret == 0) {
        LOG_I("sht31_init: SHT31 센서 응답 OK");
        return 0;
    } else {
        LOG_E("sht31_init: SHT31 센서 응답 없음 (ACK 실패)");
        LOG_E("  → 풀업 저항, 케이블 연결, 센서 전원 확인");
        return -1;
    }
}

/**
 * @brief SHT31 온습도 데이터 읽기 (블로킹)
 *
 * 단계:
 * 1. 측정 명령 송신
 * 2. 센서 측정 대기 (50ms)
 * 3. 데이터 수신
 * 4. CRC 검증은 여기서 스킵 (다음 예제에서 구현)
 *
 * @param p_temp 온도 출력 포인터 (°C)
 * @param p_humid 습도 출력 포인터 (%)
 * @return 0 (OK), -1 (ERROR)
 */
int32_t sht31_read_simple(float *p_temp, float *p_humid)
{
    uint8_t cmd[] = SHT31_MEAS_CMD;
    uint8_t response[6];
    int32_t ret;
    uint16_t temp_raw, humid_raw;

    LOG_D("sht31_read_simple: 온습도 읽기 시작");

    // Step 1: 측정 명령 송신
    ret = i2c_master_transmit(SHT31_ADDR, cmd, sizeof(cmd), 1000);
    if (ret != 0) {
        LOG_E("sht31_read_simple: 측정 명령 송신 실패");
        return -1;
    }
    LOG_D("sht31_read_simple: 측정 명령 송신 완료");

    // Step 2: 센서 측정 대기 (SHT31 클록 스트레칭 없음 모드에서 약 15ms)
    LOG_D("sht31_read_simple: 센서 측정 중 (50ms 대기)...");
    HAL_Delay(50);

    // Step 3: 데이터 수신 (6바이트)
    ret = i2c_master_receive(SHT31_ADDR, response, sizeof(response), 1000);
    if (ret != 0) {
        LOG_E("sht31_read_simple: 데이터 수신 실패");
        return -1;
    }
    LOG_D("sht31_read_simple: 데이터 수신 완료");

    // Step 4: 데이터 조합 (big-endian, CRC 검증 생략)
    temp_raw = ((uint16_t)response[0] << 8) | response[1];
    humid_raw = ((uint16_t)response[3] << 8) | response[4];

    // Step 5: 값 계산 (데이터시트 공식)
    *p_temp = (175.0f / 65535.0f) * (float)temp_raw - 45.0f;
    *p_humid = (100.0f / 65535.0f) * (float)humid_raw;

    LOG_I("sht31_read_simple: 온도=%.2f°C, 습도=%.2f%%",
          *p_temp, *p_humid);

    return 0;
}

/**
 * @brief 상세 데이터 출력 (디버깅용)
 */
void sht31_print_raw_data(const uint8_t *response)
{
    LOG_I("=== SHT31 Raw Data (16진수) ===");
    LOG_I("[0] 0x%02X (온도 상위)", response[0]);
    LOG_I("[1] 0x%02X (온도 하위)", response[1]);
    LOG_I("[2] 0x%02X (온도 CRC)", response[2]);
    LOG_I("[3] 0x%02X (습도 상위)", response[3]);
    LOG_I("[4] 0x%02X (습도 하위)", response[4]);
    LOG_I("[5] 0x%02X (습도 CRC)", response[5]);
    LOG_I("============================");
}

/**
 * @brief 테스트: SHT31 초기화 및 주기 측정
 *
 * 1. 센서 초기화
 * 2. 1초마다 온습도 읽기
 * 3. 결과 출력
 */
void test_sht31_init_and_read(void)
{
    float temp, humid;
    int32_t ret;
    uint32_t count = 0;

    LOG_I("\n=== SHT31 Init and Read Test ===");

    // 초기화
    ret = sht31_init();
    if (ret != 0) {
        LOG_E("센서 초기화 실패! 프로그램 중단.");
        while (1);
    }

    LOG_I("센서 초기화 완료. 1초마다 측정을 시작합니다.\n");

    // 주기 측정
    uint32_t tick = HAL_GetTick();

    while (count < 10) {  // 10회 측정
        if (HAL_GetTick() - tick >= 1000) {
            tick = HAL_GetTick();

            LOG_I("\n[측정 %d] 온습도 읽기 중...", count + 1);

            ret = sht31_read_simple(&temp, &humid);

            if (ret == 0) {
                LOG_I("[측정 %d] 결과: 온도=%.2f°C, 습도=%.2f%%",
                      count + 1, temp, humid);

                // 상태 판단
                if (temp > 30.0f) {
                    LOG_W("  경고: 온도가 높습니다!");
                } else if (temp < 10.0f) {
                    LOG_W("  경고: 온도가 낮습니다!");
                }

                if (humid > 80.0f) {
                    LOG_W("  경고: 습도가 높습니다!");
                } else if (humid < 30.0f) {
                    LOG_W("  경고: 습도가 낮습니다!");
                }
            } else {
                LOG_E("[측정 %d] 읽기 실패", count + 1);
            }

            count++;
        }

        HAL_Delay(10);  // 10ms 주기로 체크
    }

    LOG_I("\n=== Test Complete ===\n");
}

/**
 * @brief main() 에서 호출
 */
void sht31_example(void)
{
    // 시스템 초기화
    // HAL_Init(), SystemClock_Config(), MX_GPIO_Init(), MX_I2C2_Init()

    log_init();

    // 테스트 실행
    while (1) {
        test_sht31_init_and_read();
        HAL_Delay(2000);  // 테스트 끝난 후 2초 대기, 반복
    }
}
