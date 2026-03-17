/**
 * @file    ch18_dma_debug.c
 * @brief   DMA 관련 디버깅 패턴 3종
 *          패턴 A: DMA 전송 완료 타임아웃 감시
 *          패턴 B: DMA Error 인터럽트 + LISR 레지스터 분석
 *          패턴 C: Half-transfer 콜백 기반 더블버퍼 동기화 검증
 * @author  STM32 고급 실무교육 Ch18
 */

#include "main.h"
#include "debug_logger.h"

/* ======================================================================
 * DMA 상태 레지스터 주소 (STM32F411, DMA1/DMA2)
 * ====================================================================== */
#define DMA1_BASE_ADDR   0x40026000UL
#define DMA2_BASE_ADDR   0x40026400UL

/* LISR: 스트림 0~3 인터럽트 상태 / HISR: 스트림 4~7 */
#define DMA1_LISR   (*((volatile uint32_t *)(DMA1_BASE_ADDR + 0x00)))
#define DMA1_HISR   (*((volatile uint32_t *)(DMA1_BASE_ADDR + 0x04)))
#define DMA2_LISR   (*((volatile uint32_t *)(DMA2_BASE_ADDR + 0x00)))
#define DMA2_HISR   (*((volatile uint32_t *)(DMA2_BASE_ADDR + 0x04)))

/* LISR/HISR 비트 오프셋 (스트림별 6비트 블록) */
/* 스트림 0: 비트 0~5, 스트림 1: 비트 6~11, ... */
#define DMA_ISR_FEIF_BIT    0   /* FIFO 오버런/언더런 에러 */
#define DMA_ISR_DMEIF_BIT   2   /* 직접 모드 에러 */
#define DMA_ISR_TEIF_BIT    3   /* 전송 에러 */
#define DMA_ISR_HTIF_BIT    4   /* Half-transfer 완료 */
#define DMA_ISR_TCIF_BIT    5   /* Transfer 완료 */

/* ======================================================================
 * 패턴 A: DMA 전송 완료 타임아웃 감시
 *
 * 문제 상황: DMA 전송이 예상 시간 내에 완료되지 않을 때
 * 진단 방법: HAL_DMA_PollForTransfer() + 타임아웃 → 에러 코드 분석
 * ====================================================================== */

/**
 * @brief  SPI DMA 전송 + 타임아웃 감시 래퍼
 * @param  hdma   DMA 핸들 포인터
 * @param  timeout_ms 대기 최대 시간 (밀리초)
 * @retval HAL_OK / HAL_TIMEOUT / HAL_ERROR
 */
HAL_StatusTypeDef dma_transfer_with_timeout(DMA_HandleTypeDef *hdma,
                                             uint32_t timeout_ms)
{
    HAL_StatusTypeDef status;
    uint32_t start_tick = HAL_GetTick();

    LOG_D("DMA 전송 시작 — 타임아웃 %lu ms 설정", timeout_ms);

    /* DMA 전송 완료 폴링 대기 */
    status = HAL_DMA_PollForTransfer(hdma, HAL_DMA_FULL_TRANSFER, timeout_ms);

    if (status == HAL_TIMEOUT) {
        uint32_t elapsed = HAL_GetTick() - start_tick;
        LOG_E("DMA 전송 타임아웃! 경과시간: %lu ms", elapsed);

        /* DMA 에러 코드 확인 */
        uint32_t err_code = HAL_DMA_GetError(hdma);
        LOG_E("DMA ErrorCode = 0x%08lX", err_code);

        if (err_code & HAL_DMA_ERROR_TE)   LOG_E("  >> Transfer Error (TEIF)");
        if (err_code & HAL_DMA_ERROR_FE)   LOG_E("  >> FIFO Error (FEIF)");
        if (err_code & HAL_DMA_ERROR_DME)  LOG_E("  >> Direct Mode Error (DMEIF)");

        /* DMA 강제 중단 */
        HAL_DMA_Abort(hdma);
        LOG_W("DMA 강제 중단 완료 — 재초기화 필요");

    } else if (status == HAL_ERROR) {
        LOG_E("DMA HAL_ERROR 발생 — DMA 핸들 상태 확인 필요");
        LOG_E("DMA State = %d", (int)hdma->State);

    } else {
        uint32_t elapsed = HAL_GetTick() - start_tick;
        LOG_D("DMA 전송 완료 — 소요시간: %lu ms", elapsed);
    }

    return status;
}

/* ======================================================================
 * 패턴 B: DMA Error 인터럽트 핸들러 + LISR 레지스터 직접 분석
 *
 * 문제 상황: DMA 에러 인터럽트가 발생했으나 원인 불명
 * 진단 방법: XferErrorCallback에서 LISR/HISR 직접 읽기
 * ====================================================================== */

/**
 * @brief  DMA 전송 에러 콜백 (HAL 내부에서 호출)
 *         사용자가 HAL_DMA_RegisterCallback() 또는 직접 override로 등록
 * @param  hdma  에러 발생한 DMA 핸들
 */
void dma_error_callback(DMA_HandleTypeDef *hdma)
{
    uint32_t lisr_val = DMA1_LISR;  /* 실제 사용 DMA 번호에 맞게 변경 */
    uint32_t hisr_val = DMA1_HISR;

    LOG_E("=== DMA ERROR CALLBACK 진입 ===");
    LOG_E("DMA1 LISR = 0x%08lX", lisr_val);
    LOG_E("DMA1 HISR = 0x%08lX", hisr_val);

    /* Stream 별 에러 비트 확인 (스트림 0 기준, 6비트 오프셋 적용 필요) */
    /* TEIF0: 비트3, DMEIF0: 비트2, FEIF0: 비트0 */
    if (lisr_val & (1U << 3)) LOG_E("Stream0 전송 에러 (TEIF0) 발생");
    if (lisr_val & (1U << 2)) LOG_E("Stream0 직접 모드 에러 (DMEIF0) 발생");
    if (lisr_val & (1U << 0)) LOG_E("Stream0 FIFO 에러 (FEIF0) 발생");

    /* DMA 에러 플래그 초기화 후 재시도 또는 에러 처리 */
    __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_TE_FLAG_INDEX(hdma));
    __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_DME_FLAG_INDEX(hdma));
    __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_FE_FLAG_INDEX(hdma));

    LOG_W("DMA 에러 플래그 초기화 완료 — 상위 레이어에 에러 통보 필요");
    /* TODO: 서비스 레이어에 DMA_ERROR 이벤트 전달 */
}

/* ======================================================================
 * 패턴 C: Half-transfer 콜백 기반 더블버퍼 동기화 검증
 *
 * 문제 상황: Circular DMA + 더블버퍼에서 데이터 깨짐 (tearing 현상)
 * 원인: Half-transfer 콜백에서 버퍼 교체 타이밍 불일치
 * 진단 방법: Half/Full 콜백에서 처리 시작/종료 타임스탬프 비교
 * ====================================================================== */

/* 더블 버퍼 상태 추적 */
typedef struct {
    uint8_t  buf[2][512];   /* 버퍼 A(인덱스 0) / 버퍼 B(인덱스 1) */
    uint8_t  active_idx;    /* DMA가 현재 채우는 버퍼 인덱스 */
    uint32_t half_tick;     /* Half-transfer 발생 시각 */
    uint32_t full_tick;     /* Full-transfer 발생 시각 */
    uint32_t process_tick;  /* 처리 시작 시각 */
    uint32_t overrun_cnt;   /* 버퍼 오버런 카운트 */
} dma_double_buf_t;

static dma_double_buf_t g_rx_buf = {0};

/**
 * @brief  DMA Half-transfer 콜백 (버퍼 A 처리 시작)
 *         HAL_UART_RxHalfCpltCallback() 등에서 호출
 */
void dma_half_transfer_callback(DMA_HandleTypeDef *hdma)
{
    (void)hdma;
    g_rx_buf.half_tick = HAL_GetTick();
    g_rx_buf.active_idx = 1;  /* DMA가 이제 버퍼 B를 채움 → 버퍼 A 처리 가능 */

    LOG_D("Half-transfer 완료 @ %lu ms — 버퍼 A 처리 시작", g_rx_buf.half_tick);

    /* 버퍼 A 처리 시작 시각 기록 */
    g_rx_buf.process_tick = HAL_GetTick();

    /* 버퍼 A 데이터 처리 (여기서 처리 시간이 Full-transfer까지 남은 시간보다 짧아야 함) */
    /* process_buf_a(g_rx_buf.buf[0], 256); */
}

/**
 * @brief  DMA Full-transfer 콜백 (버퍼 B 처리 시작)
 *         HAL_UART_RxCpltCallback() 등에서 호출
 */
void dma_full_transfer_callback(DMA_HandleTypeDef *hdma)
{
    (void)hdma;
    g_rx_buf.full_tick = HAL_GetTick();
    g_rx_buf.active_idx = 0;  /* DMA가 이제 버퍼 A를 채움 → 버퍼 B 처리 가능 */

    LOG_D("Full-transfer 완료 @ %lu ms — 버퍼 B 처리 시작", g_rx_buf.full_tick);

    /* 동기화 검증: 이전 Half에서 시작한 버퍼 A 처리가 완료되었는지 확인 */
    uint32_t process_duration = HAL_GetTick() - g_rx_buf.process_tick;
    uint32_t half_to_full = g_rx_buf.full_tick - g_rx_buf.half_tick;

    if (process_duration > half_to_full) {
        /* 처리 시간이 DMA 반주기보다 길면 오버런 위험 */
        g_rx_buf.overrun_cnt++;
        LOG_W("버퍼 오버런 위험! 처리시간(%lu ms) > 반주기(%lu ms), 누적: %lu",
              process_duration, half_to_full, g_rx_buf.overrun_cnt);
    }

    /* 버퍼 B 처리 */
    /* process_buf_b(g_rx_buf.buf[1], 256); */
}

/* ======================================================================
 * UART TX DMA 재진입 방지 패턴 (시나리오 B 해결책)
 *
 * 문제: CLI 응답 전송 중 두 번째 명령 처리 시 DMA 중복 시작
 * 해결: 전송 전 DMA 상태 확인
 * ====================================================================== */

extern UART_HandleTypeDef huart2;  /* CubeMX 생성 핸들 */

/**
 * @brief  UART TX DMA 안전 전송 (재진입 방지)
 * @param  data  전송 데이터 포인터
 * @param  len   전송 길이
 * @retval HAL_OK / HAL_BUSY
 */
HAL_StatusTypeDef uart_tx_dma_safe(const uint8_t *data, uint16_t len)
{
    /* DMA 전송 중이면 대기하지 않고 즉시 반환 (논블로킹) */
    if (huart2.hdmatx != NULL &&
        huart2.hdmatx->State == HAL_DMA_STATE_BUSY) {
        LOG_W("UART TX DMA 사용 중 — 전송 스킵 (len=%u)", len);
        return HAL_BUSY;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(&huart2,
                                                      (uint8_t *)data,
                                                      len);
    if (status != HAL_OK) {
        LOG_E("HAL_UART_Transmit_DMA 실패 — status=%d, UART state=%d",
              (int)status, (int)huart2.gState);
    } else {
        LOG_D("UART TX DMA 시작 — len=%u", len);
    }

    return status;
}
