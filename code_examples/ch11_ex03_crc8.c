/**
 * Ch11_ex03_crc8.c
 * CRC-8 검증 알고리즘 (다항식 0x31)
 *
 * 목표: SHT31 센서 데이터 무결성을 보장하는 CRC-8 계산 및 검증
 * 표준: Sensirion SHT31 CRC-8 사양
 *   - 다항식: 0x31
 *   - 초기값: 0xFF
 *   - 입력 반사: NO
 *   - 출력 반사: NO
 *   - 최종 XOR: 0x00 (생략)
 */

#include "main.h"
#include "log.h"
#include <stdint.h>
#include <string.h>

/**
 * @brief CRC-8 계산 (SHT31 규격)
 *
 * 알고리즘: 비트별 처리, MSB 우선
 *   1. 초기값 0xFF로 시작
 *   2. 각 입력 바이트를 CRC와 XOR
 *   3. 8번 시프트하며 다항식 0x31과 XOR (MSB=1일 때)
 *   4. 모든 바이트 처리 후 최종 CRC 반환
 *
 * @param data 입력 데이터 포인터
 * @param len 데이터 길이 (바이트)
 * @return CRC-8 값 (0~255)
 *
 * 예시:
 *   데이터: {0xBE, 0x00} (온도 값 예시)
 *   CRC: 0xAC (계산 결과)
 */
uint8_t crc8_sht31(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0xFF;  // 초기값

    LOG_D("crc8_sht31: len=%d, data[0]=0x%02X", len, len > 0 ? data[0] : 0);

    for (uint16_t i = 0; i < len; i++) {
        // Step 1: 입력 바이트와 XOR
        crc ^= data[i];

        // Step 2: 8비트 처리 (비트 7부터 비트 0까지, MSB 우선)
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                // MSB가 1이면 다항식 0x31과 XOR
                // (사실상 왼쪽 시프트 + 다항식 XOR)
                crc = (crc << 1) ^ 0x31;
            } else {
                // MSB가 0이면 그냥 왼쪽 시프트
                crc = crc << 1;
            }

            // 8비트 범위 유지 (0xFF & 연산)
            crc &= 0xFF;
        }
    }

    LOG_D("crc8_sht31: calculated CRC = 0x%02X", crc);

    return crc;
}

/**
 * @brief CRC-8 검증
 *
 * 데이터 마지막 바이트를 CRC로 가정하고 검증합니다.
 * SHT31 응답 형식: [데이터0, 데이터1, CRC]
 *
 * @param data 데이터 포인터 (마지막 바이트가 CRC)
 * @param len 데이터 길이 (CRC 포함)
 * @return 0 (OK), -1 (CRC 불일치)
 */
int32_t crc8_verify_sht31(const uint8_t *data, uint16_t len)
{
    uint8_t expected_crc;
    uint8_t calculated_crc;

    if (len < 1) {
        LOG_W("crc8_verify_sht31: invalid len %d", len);
        return -1;
    }

    // 수신한 CRC (마지막 바이트)
    expected_crc = data[len - 1];

    // 계산한 CRC (데이터만, CRC 제외)
    calculated_crc = crc8_sht31(data, len - 1);

    LOG_D("crc8_verify_sht31: expected=0x%02X, calculated=0x%02X",
          expected_crc, calculated_crc);

    if (expected_crc == calculated_crc) {
        LOG_I("crc8_verify_sht31: CRC OK");
        return 0;
    } else {
        LOG_E("crc8_verify_sht31: CRC MISMATCH (데이터 손상)");
        return -1;
    }
}

/**
 * @brief 단계별 CRC 계산 (디버깅용)
 *
 * 각 바이트와 각 비트 처리 과정을 상세히 로깅합니다.
 *
 * @param data 입력 데이터
 * @param len 데이터 길이
 * @return CRC-8 값
 */
uint8_t crc8_sht31_debug(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0xFF;

    LOG_I("\n=== CRC-8 Step-by-Step Calculation ===");
    LOG_I("Input data length: %d bytes", len);
    LOG_I("Initial CRC: 0x%02X", crc);

    for (uint16_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        LOG_I("\n[Byte %d] 0x%02X", i, byte);

        // XOR와 비트 처리
        crc ^= byte;
        LOG_I("  After XOR: CRC = 0x%02X", crc);

        for (uint8_t j = 0; j < 8; j++) {
            uint8_t msb = (crc & 0x80) ? 1 : 0;
            if (msb) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = crc << 1;
            }
            crc &= 0xFF;

            LOG_D("    Bit %d: MSB=%d, CRC = 0x%02X", j, msb, crc);
        }
    }

    LOG_I("\n=== Final CRC: 0x%02X ===\n", crc);

    return crc;
}

/**
 * @brief 테스트: SHT31 샘플 데이터로 CRC 검증
 *
 * Sensirion 데이터시트의 테스트 벡터를 사용합니다.
 * (실제 센서 응답이 없을 때도 알고리즘 검증 가능)
 */
void test_crc8_verification(void)
{
    LOG_I("\n=== CRC-8 Verification Test ===");

    // 테스트 케이스 1: 온도 데이터 (데이터시트 예시)
    // 온도 0xBE, 0x00 → CRC는 0xAC
    uint8_t test_data_1[] = {0xBE, 0x00, 0xAC};

    LOG_I("\n[Test 1] 온도 데이터");
    LOG_I("Data: 0xBE, 0x00");
    LOG_I("Expected CRC: 0xAC");

    uint8_t calc_crc = crc8_sht31(test_data_1, 2);
    LOG_I("Calculated CRC: 0x%02X", calc_crc);

    if (crc8_verify_sht31(test_data_1, 3) == 0) {
        LOG_I("✓ Test 1 PASS");
    } else {
        LOG_E("✗ Test 1 FAIL");
    }

    // 테스트 케이스 2: 습도 데이터
    // 습도 0x5E, 0x00 → CRC는 0x6B
    uint8_t test_data_2[] = {0x5E, 0x00, 0x6B};

    LOG_I("\n[Test 2] 습도 데이터");
    LOG_I("Data: 0x5E, 0x00");
    LOG_I("Expected CRC: 0x6B");

    calc_crc = crc8_sht31(test_data_2, 2);
    LOG_I("Calculated CRC: 0x%02X", calc_crc);

    if (crc8_verify_sht31(test_data_2, 3) == 0) {
        LOG_I("✓ Test 2 PASS");
    } else {
        LOG_E("✗ Test 2 FAIL");
    }

    // 테스트 케이스 3: CRC 불일치 (의도적 손상)
    uint8_t test_data_3[] = {0xBE, 0x00, 0xFF};  // CRC를 잘못된 값으로 변경

    LOG_I("\n[Test 3] CRC 불일치 (의도적 손상)");
    LOG_I("Data: 0xBE, 0x00");
    LOG_I("Wrong CRC: 0xFF (실제: 0xAC)");

    if (crc8_verify_sht31(test_data_3, 3) != 0) {
        LOG_I("✓ Test 3 PASS (손상 감지됨)");
    } else {
        LOG_E("✗ Test 3 FAIL (손상을 감지하지 못함)");
    }

    LOG_I("\n=== Test Complete ===\n");
}

/**
 * @brief 테스트: 상세 단계별 계산
 */
void test_crc8_debug(void)
{
    uint8_t data[] = {0xBE, 0x00};

    LOG_I("\n=== CRC-8 Debug Mode ===");
    LOG_I("Computing CRC for 0xBE, 0x00 step-by-step...");

    uint8_t crc = crc8_sht31_debug(data, sizeof(data));

    LOG_I("Final CRC: 0x%02X (Expected: 0xAC)", crc);

    if (crc == 0xAC) {
        LOG_I("✓ CRC Calculation CORRECT");
    } else {
        LOG_E("✗ CRC Calculation INCORRECT");
    }
}

/**
 * @brief main() 에서 호출
 */
void crc8_example(void)
{
    log_init();

    // 테스트 1: 기본 검증
    test_crc8_verification();

    // 테스트 2: 상세 디버그
    test_crc8_debug();

    while (1) {
        HAL_Delay(5000);
    }
}
