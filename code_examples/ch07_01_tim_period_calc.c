/* ch07_01_tim_period_calc.c — PSC/ARR 조합별 주기 계산 검증
 *
 * 레이어: App (학습용)
 * 목적: PSC와 ARR 값을 바꿔가며 실제 주기를 확인
 *
 * TIM2 클럭: APB1 = 84 MHz
 * 공식: T = (PSC + 1) × (ARR + 1) / f_clk
 */

#include "main.h"
#include "log.h"
#include <stdio.h>

/* 주기 계산 테이블 */
typedef struct {
    uint16_t psc;
    uint32_t arr;       /* TIM2는 32비트 */
    const char *desc;
} tim_calc_entry_t;

static const tim_calc_entry_t calc_table[] = {
    { 8399,   9999, "1초 (1 Hz)"       },
    { 8399,   4999, "500ms (2 Hz)"     },
    { 8399,    999, "100ms (10 Hz)"    },
    {   83,   9999, "10ms (100 Hz)"    },
    {   83,    999, "1ms (1 kHz)"      },
};

void tim_period_calc_demo(void)
{
    const uint32_t f_clk = 84000000;  /* 84 MHz */
    uint32_t count = sizeof(calc_table) / sizeof(calc_table[0]);

    LOG_I("=== TIM2 주기 계산 검증 (f_clk = %lu Hz) ===", f_clk);

    for (uint32_t i = 0; i < count; i++) {
        uint32_t psc = calc_table[i].psc;
        uint32_t arr = calc_table[i].arr;

        /* 주기 계산 (마이크로초 단위) */
        uint64_t period_us = (uint64_t)(psc + 1) * (arr + 1)
                             * 1000000 / f_clk;

        LOG_I("[%lu] PSC=%5lu, ARR=%5lu → T=%lu us (%s)",
              i, psc, arr, (uint32_t)period_us,
              calc_table[i].desc);
    }

    LOG_I("=== 계산 완료 ===");
}
