/* ch09_01_bcd_read_write.c — BCD 값 읽기 및 변환
 *
 * BCD(Binary-Coded Decimal) 이해를 위한 변환 예제
 * RTC 레지스터는 BCD 형식으로 시간을 저장합니다.
 *
 * 핵심: 0x23 = "23" (십진수 35가 아님!)
 *       상위 4비트 = 십의 자리, 하위 4비트 = 일의 자리
 */

#include "main.h"
#include "log.h"

/* ===== BCD ↔ Binary 변환 함수 ===== */

/**
 * @brief BCD 값을 Binary(이진수)로 변환
 * @param bcd BCD 인코딩된 값 (예: 0x23)
 * @return 이진수 값 (예: 23)
 *
 * 공식: (상위 니블 × 10) + 하위 니블
 */
static uint8_t bcd_to_binary(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/**
 * @brief Binary 값을 BCD로 변환
 * @param bin 이진수 값 (예: 23)
 * @return BCD 인코딩된 값 (예: 0x23)
 *
 * 공식: (십의 자리 << 4) | 일의 자리
 */
static uint8_t binary_to_bcd(uint8_t bin)
{
    return ((bin / 10) << 4) | (bin % 10);
}

/* ===== BCD 변환 연습 ===== */

void bcd_conversion_example(void)
{
    LOG_I("===== BCD 변환 연습 =====");

    /* 예시 1: 23시 */
    uint8_t hour_bcd = 0x23;
    uint8_t hour_bin = bcd_to_binary(hour_bcd);
    LOG_D("BCD 0x%02X → Binary %d (23시)", hour_bcd, hour_bin);

    /* 예시 2: 59분 */
    uint8_t min_bcd = 0x59;
    uint8_t min_bin = bcd_to_binary(min_bcd);
    LOG_D("BCD 0x%02X → Binary %d (59분)", min_bcd, min_bin);

    /* 예시 3: Binary → BCD 역변환 */
    uint8_t sec_bin = 8;
    uint8_t sec_bcd = binary_to_bcd(sec_bin);
    LOG_D("Binary %d → BCD 0x%02X (08초)", sec_bin, sec_bcd);

    /* 예시 4: HAL 제공 함수 (실무에서는 이것을 사용) */
    uint8_t value = 45;
    uint8_t bcd_val = RTC_ByteToBcd2(value);      /* HAL 매크로 */
    uint8_t bin_val = RTC_Bcd2ToByte(bcd_val);     /* HAL 매크로 */
    LOG_D("HAL 변환: %d → BCD 0x%02X → Binary %d", value, bcd_val, bin_val);

    LOG_I("BCD 변환 연습 완료");
}

/* ===== 주의: BCD에서 유효하지 않은 값 ===== */

void bcd_validation_example(void)
{
    LOG_I("===== BCD 유효성 검증 =====");

    /* BCD는 각 니블이 0~9만 유효 */
    uint8_t valid_bcd   = 0x59;   /* 유효: 5와 9 모두 0~9 */
    uint8_t invalid_bcd = 0x5A;   /* 무효: 하위 니블 A(=10) */

    uint8_t v = bcd_to_binary(valid_bcd);
    LOG_D("유효 BCD 0x%02X → %d", valid_bcd, v);

    /* 무효 BCD를 변환하면 잘못된 값 발생 */
    uint8_t iv = bcd_to_binary(invalid_bcd);
    LOG_W("무효 BCD 0x%02X → %d (잘못된 값!)", invalid_bcd, iv);

    LOG_I("디버거에서 RTC_TR 레지스터를 읽을 때 A~F가 보이면 오류입니다");
}
