/* ch05_01_memcpy_polling.c — 폴링 메모리 복사 + DWT 시간 측정
 *
 * 레이어: App (테스트 코드)
 * 위치: 프로젝트 아키텍처 v0.5
 *
 * DMA 성능 비교의 기준선(Baseline) 측정용.
 * for 루프로 uint32_t 배열을 복사하고 DWT Cycle Counter로 시간을 측정한다.
 */

#include "main.h"
#include "log.h"
#include <string.h>

#define BUF_SIZE  1024   /* uint32_t 1024개 = 4KB */

static uint32_t src_buf[BUF_SIZE];
static uint32_t dst_buf[BUF_SIZE];

/* ===== DWT Cycle Counter 초기화 ===== */
static void dwt_init(void)
{
    /* 트레이스 유닛 활성화 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    /* 사이클 카운터 리셋 */
    DWT->CYCCNT = 0;
    /* 사이클 카운터 시작 */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    LOG_D("DWT Cycle Counter 초기화 완료");
}

/* ===== 소스 버퍼에 테스트 패턴 채우기 ===== */
static void fill_test_pattern(void)
{
    for (uint32_t i = 0; i < BUF_SIZE; i++) {
        src_buf[i] = 0xDEAD0000 | i;
    }
    LOG_D("테스트 패턴 채우기 완료: 0xDEAD0000~0xDEAD%04lX",
          (uint32_t)(BUF_SIZE - 1));
}

/* ===== 폴링 복사 (for 루프) ===== */
static uint32_t copy_polling(void)
{
    uint32_t start = DWT->CYCCNT;

    for (uint32_t i = 0; i < BUF_SIZE; i++) {
        dst_buf[i] = src_buf[i];
    }

    uint32_t end = DWT->CYCCNT;
    return end - start;
}

/* ===== 데이터 검증 ===== */
static int verify_copy(void)
{
    for (uint32_t i = 0; i < BUF_SIZE; i++) {
        if (src_buf[i] != dst_buf[i]) {
            LOG_E("검증 실패: idx=%lu, src=0x%08lX, "
                  "dst=0x%08lX", i, src_buf[i], dst_buf[i]);
            return -1;
        }
    }
    return 0;
}

/* ===== 메인 데모 함수 ===== */
void polling_copy_demo(void)
{
    dwt_init();
    fill_test_pattern();
    memset(dst_buf, 0, sizeof(dst_buf));

    uint32_t cycles = copy_polling();
    uint32_t us = cycles / (SystemCoreClock / 1000000);

    LOG_I("=== 폴링 복사 결과 ===");
    LOG_I("  크기: %lu bytes (%lu words)",
          (uint32_t)(BUF_SIZE * 4), (uint32_t)BUF_SIZE);
    LOG_I("  소요: %lu cycles (%lu us)", cycles, us);

    if (verify_copy() == 0) {
        LOG_I("  검증: PASS");
    } else {
        LOG_E("  검증: FAIL");
    }
}
