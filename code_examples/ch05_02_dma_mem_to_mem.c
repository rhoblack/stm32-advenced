/* ch05_02_dma_mem_to_mem.c — DMA Mem-to-Mem 전송
 *
 * 레이어: App (테스트 코드)
 * 위치: 프로젝트 아키텍처 v0.5
 *
 * CubeMX에서 DMA2 Stream 0을 MEMTOMEM으로 설정한 후 사용.
 * HAL_DMA_Start_IT()로 전송하고, 콜백으로 완료를 감지한다.
 */

#include "main.h"
#include "log.h"
#include <string.h>

#define BUF_SIZE  1024   /* uint32_t 1024개 = 4KB */

static uint32_t src_buf[BUF_SIZE];
static uint32_t dst_buf[BUF_SIZE];

/* ISR과 메인 루프가 공유하는 플래그 — volatile 필수 */
static volatile uint8_t dma_complete = 0;
static volatile uint8_t dma_error = 0;

/* CubeMX가 생성한 DMA 핸들 (extern) */
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream0;

/* ===== DMA 전송 완료 콜백 ===== */
static void dma_xfer_cplt(DMA_HandleTypeDef *hdma)
{
    dma_complete = 1;
    LOG_D("DMA 전송 완료 콜백 호출됨");
}

/* ===== DMA 전송 에러 콜백 ===== */
static void dma_xfer_error(DMA_HandleTypeDef *hdma)
{
    dma_error = 1;
    LOG_E("DMA 전송 에러! ErrorCode=0x%08lX",
          hdma->ErrorCode);
}

/* ===== 콜백 등록 ===== */
static void dma_register_callbacks(void)
{
    hdma_memtomem_dma2_stream0.XferCpltCallback
        = dma_xfer_cplt;
    hdma_memtomem_dma2_stream0.XferErrorCallback
        = dma_xfer_error;
    LOG_D("DMA 콜백 등록 완료");
}

/* ===== 데이터 검증 ===== */
static int verify_copy(void)
{
    for (uint32_t i = 0; i < BUF_SIZE; i++) {
        if (src_buf[i] != dst_buf[i]) {
            LOG_E("검증 실패: idx=%lu, "
                  "src=0x%08lX, dst=0x%08lX",
                  i, src_buf[i], dst_buf[i]);
            return -1;
        }
    }
    return 0;
}

/* ===== 메인 데모 함수 ===== */
void dma_copy_demo(void)
{
    /* 테스트 패턴 준비 */
    for (uint32_t i = 0; i < BUF_SIZE; i++) {
        src_buf[i] = 0xDEAD0000 | i;
    }
    memset(dst_buf, 0, sizeof(dst_buf));

    /* 콜백 등록 */
    dma_register_callbacks();

    /* 플래그 초기화 */
    dma_complete = 0;
    dma_error = 0;

    LOG_I("DMA Mem-to-Mem 전송 시작...");
    LOG_I("  src: 0x%08lX, dst: 0x%08lX, 크기: %lu words",
          (uint32_t)src_buf, (uint32_t)dst_buf,
          (uint32_t)BUF_SIZE);

    /* DMA 전송 시작 (인터럽트 모드) */
    HAL_StatusTypeDef status;
    status = HAL_DMA_Start_IT(
        &hdma_memtomem_dma2_stream0,
        (uint32_t)src_buf,       /* 소스 주소 */
        (uint32_t)dst_buf,       /* 목적지 주소 */
        BUF_SIZE                 /* 전송 단위 수 (word) */
    );

    if (status != HAL_OK) {
        LOG_E("DMA 시작 실패: status=%d", status);
        return;
    }

    /* === CPU는 여기서 다른 작업 가능! === */
    LOG_D("DMA 전송 중... CPU는 자유!");

    /* 완료 대기 */
    uint32_t timeout = HAL_GetTick() + 1000;
    while (!dma_complete && !dma_error) {
        if (HAL_GetTick() > timeout) {
            LOG_E("DMA 전송 타임아웃!");
            return;
        }
        /* 실무에서는 여기서 다른 작업 수행 */
    }

    if (dma_error) {
        LOG_E("DMA 전송 실패");
        return;
    }

    /* 데이터 검증 */
    if (verify_copy() == 0) {
        LOG_I("DMA 복사 완료 + 검증 PASS");
    } else {
        LOG_E("DMA 복사 완료 — 검증 FAIL");
    }
}
