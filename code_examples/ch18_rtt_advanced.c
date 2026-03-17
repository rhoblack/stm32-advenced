/**
 * @file    ch18_rtt_advanced.c
 * @brief   SEGGER RTT 고급 활용 — 다채널 + 타임스탬프 + ITM/SWO 비교
 * @author  STM32 고급 실무교육 Ch18
 *
 * @note    SEGGER RTT 사용을 위해 다음 파일이 프로젝트에 포함되어야 합니다:
 *          SEGGER_RTT.c, SEGGER_RTT.h (SEGGER 공식 배포 파일)
 *          STM32CubeIDE: Project > Properties > C/C++ Build > Settings >
 *          Include Paths에 RTT 폴더 추가
 */

#include "main.h"
#include "SEGGER_RTT.h"
#include "debug_logger.h"

/* ======================================================================
 * RTT 채널 번호 정의 (Ch02 단일 채널에서 다채널로 확장)
 * ====================================================================== */
#define RTT_CH_GENERAL   0  /* 채널 0: 일반 로그 (기본 채널, J-Link RTT Viewer) */
#define RTT_CH_DMA       1  /* 채널 1: DMA 진단 전용 (고속 출력) */
#define RTT_CH_HARDFAULT 2  /* 채널 2: HardFault 긴급 출력 (가장 높은 우선도) */

/* RTT 채널 버퍼 크기 */
#define RTT_BUF_SIZE_GENERAL    1024
#define RTT_BUF_SIZE_DMA        2048  /* DMA 채널은 더 큰 버퍼 */
#define RTT_BUF_SIZE_HARDFAULT   512

/* ======================================================================
 * RTT 다채널 초기화
 * main.c의 SystemInit() 직후 호출 권장
 * ====================================================================== */

/* RTT 채널 버퍼 (섹션 지정: .noinit — 리셋 후에도 유지) */
static char rtt_buf_dma[RTT_BUF_SIZE_DMA] __attribute__((section(".noinit")));
static char rtt_buf_hf[RTT_BUF_SIZE_HARDFAULT] __attribute__((section(".noinit")));

void rtt_advanced_init(void)
{
    /* 채널 0은 SEGGER_RTT_Init()에서 자동 초기화 */
    SEGGER_RTT_Init();

    /* 채널 1: DMA 진단 채널 설정 (Overwrite 모드 — 버퍼 가득 차면 오래된 것 덮어씀) */
    SEGGER_RTT_ConfigUpBuffer(RTT_CH_DMA, "DMA_Diag",
                               rtt_buf_dma, RTT_BUF_SIZE_DMA,
                               SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);

    /* 채널 2: HardFault 긴급 채널 (No Overwrite — 손실 없이 블로킹) */
    SEGGER_RTT_ConfigUpBuffer(RTT_CH_HARDFAULT, "HardFault",
                               rtt_buf_hf, RTT_BUF_SIZE_HARDFAULT,
                               SEGGER_RTT_MODE_NO_BLOCK_SKIP);

    LOG_I("RTT 다채널 초기화 완료: CH0=General, CH1=DMA, CH2=HardFault");
}

/* ======================================================================
 * 타임스탬프 포함 RTT 로그 출력
 * DWT(Data Watchpoint and Trace) 사이클 카운터로 마이크로초 단위 측정
 * ====================================================================== */

/* DWT 사이클 카운터 레지스터 */
#define DWT_CYCCNT  (*((volatile uint32_t *)0xE0001004))
#define DWT_CTRL    (*((volatile uint32_t *)0xE0001000))
#define DWT_LAR     (*((volatile uint32_t *)0xE0001FB0))  /* Lock Access Register */

/* 코어 클럭 (STM32F411 최대 100MHz) */
#define CORE_CLOCK_HZ   100000000UL

/**
 * @brief  DWT 사이클 카운터 활성화 (마이크로초 타임스탬프 용)
 */
void dwt_init(void)
{
    /* CoreSight 잠금 해제 */
    DWT_LAR = 0xC5ACCE55;

    /* DWT 사이클 카운터 활성화 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT_CYCCNT = 0;
    DWT_CTRL |= (1U << 0);  /* CYCCNTENA 비트 활성화 */

    LOG_D("DWT 사이클 카운터 초기화 완료 (코어 클럭: %lu Hz)", CORE_CLOCK_HZ);
}

/**
 * @brief  현재 시간을 마이크로초 단위로 반환
 * @retval 경과 마이크로초 (오버플로우: 약 42초 주기 @ 100MHz)
 */
static uint32_t get_timestamp_us(void)
{
    return DWT_CYCCNT / (CORE_CLOCK_HZ / 1000000UL);
}

/**
 * @brief  타임스탬프 포함 DMA 진단 로그 출력 (채널 1)
 * @param  fmt  printf 형식 포맷 문자열
 */
void rtt_dma_log(const char *fmt, ...)
{
    char buf[128];
    uint32_t ts_us = get_timestamp_us();
    int prefix_len = snprintf(buf, sizeof(buf), "[DMA][%lu us] ", ts_us);

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf + prefix_len, sizeof(buf) - prefix_len, fmt, args);
    va_end(args);

    SEGGER_RTT_WriteString(RTT_CH_DMA, buf);
}

/**
 * @brief  HardFault 긴급 RTT 출력 (채널 2, 인터럽트 컨텍스트 안전)
 * @note   printf 계열 함수 사용 금지 — SEGGER_RTT_Write 직접 사용
 * @param  msg  출력할 문자열 (NULL 종료)
 */
void rtt_hardfault_log(const char *msg)
{
    SEGGER_RTT_WriteString(RTT_CH_HARDFAULT, msg);
}

/* ======================================================================
 * ITM/SWO vs RTT 비교 출력 (Ch02 심화)
 *
 * ITM 포트 채널:
 *   포트 0: printf 리다이렉트 (기본)
 *   포트 1: DMA 진단
 *   포트 2: HardFault 긴급
 *
 * 제약: SWD 핀 외에 SWO 핀(PB3) 추가 필요, 고속 전송 시 손실 가능
 * RTT 장점: 양방향, 손실 없음, SWD만으로 동작
 * ====================================================================== */

/**
 * @brief  ITM 포트를 통한 4바이트 출력 (폴링, 블로킹)
 * @param  port   ITM 포트 번호 (0~31)
 * @param  value  출력할 32비트 값
 */
void itm_send_u32(uint32_t port, uint32_t value)
{
    /* ITM 활성화 및 포트 사용 가능 여부 확인 */
    if ((ITM->TCR & ITM_TCR_ITMENA_Msk) &&
        (ITM->TER & (1UL << port))) {

        /* 포트 FIFO 여유 대기 (타임아웃 없음 — 주의) */
        while (ITM->PORT[port].u32 == 0) { __NOP(); }
        ITM->PORT[port].u32 = value;
    }
}

/* ======================================================================
 * 링 버퍼 오버플로우 진단 (시나리오 C 해결책)
 * Ch06에서 구현한 ring_buffer_t 구조체 활용
 * ====================================================================== */

/**
 * @brief  링 버퍼 상태 진단 로그 출력
 *         오버플로우 카운터가 0보다 크면 경고 발령
 * @param  name        버퍼 식별 이름
 * @param  used_bytes  현재 사용 중인 바이트 수
 * @param  capacity    전체 버퍼 용량 (바이트)
 * @param  overflow_cnt 오버플로우 발생 횟수
 */
void debug_ring_buffer_status(const char *name,
                               uint32_t used_bytes,
                               uint32_t capacity,
                               uint32_t overflow_cnt)
{
    uint32_t usage_pct = (used_bytes * 100U) / capacity;

    if (overflow_cnt > 0U) {
        LOG_E("링버퍼[%s] 오버플로우 %lu회 발생! 사용률=%lu%%(%lu/%lu)",
              name, overflow_cnt, usage_pct, used_bytes, capacity);
        LOG_W("  >> 해결책 1: LOG_LEVEL을 LOG_WARN으로 상향 (#define LOG_LEVEL 2)");
        LOG_W("  >> 해결책 2: 버퍼 크기 증가 (현재 %lu 바이트)", capacity);

    } else if (usage_pct > 80U) {
        LOG_W("링버퍼[%s] 사용률 높음: %lu%% (%lu/%lu 바이트)",
              name, usage_pct, used_bytes, capacity);

    } else {
        LOG_D("링버퍼[%s] 정상: %lu%% (%lu/%lu 바이트)",
              name, usage_pct, used_bytes, capacity);
    }
}
