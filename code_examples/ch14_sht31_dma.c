/**
 * @file    ch14_sht31_dma.c
 * @brief   SHT31 드라이버 — DMA 비동기 측정 루프 (Ch14 v1.4)
 *
 * Ch11의 SHT31_Read()(폴링, 블로킹)를 비동기 콜백 체인 방식으로 재설계합니다.
 *
 * 측정 시퀀스 (콜백 체인):
 *   SHT31_StartMeasurement()
 *     → I2C TX DMA (CMD 전송)
 *     → TxCpltCallback → 대기 타이머 시작 (15ms)
 *     → TIM6 인터럽트 → I2C RX DMA (데이터 수신)
 *     → RxCpltCallback → 파싱 + 데이터 갱신
 *     → 다음 주기 자동 대기 (TIM2 500ms)
 *
 * @note    STM32F411RE, HAL, Ch11 SHT31 CRC-8 로직 재사용
 * @version v1.4
 */

#include "ch14_sht31_dma.h"
#include "ch14_i2c_dma_driver.h"
#include "log.h"

/* ===== SHT31 I2C 주소 및 명령어 ===== */
#define SHT31_I2C_ADDR          (0x44U << 1)   /**< 7비트 주소 0x44, HAL용 8비트 */
#define SHT31_CMD_MEAS_HIGHREP  {0x24, 0x00}    /**< 고정밀 단발 측정 명령 */
#define SHT31_DATA_SIZE         6U              /**< 온도(2)+CRC(1)+습도(2)+CRC(1) */

/* ===== CRC-8 파라미터 (SHT31 규격) ===== */
#define SHT31_CRC_POLY          0x31U
#define SHT31_CRC_INIT          0xFFU

/* ===== 드라이버 내부 상태 ===== */
typedef enum {
    SHT31_DMA_IDLE       = 0,   /**< 대기 — 새 측정 요청 가능 */
    SHT31_DMA_CMD_TX     = 1,   /**< CMD I2C TX DMA 진행 중 */
    SHT31_DMA_WAIT_MEAS  = 2,   /**< SHT31 내부 측정 대기 (15ms) */
    SHT31_DMA_DATA_RX    = 3,   /**< 데이터 I2C RX DMA 진행 중 */
    SHT31_DMA_ERROR      = 4,   /**< 오류 상태 */
} Sht31DmaState;

/* ===== 모듈 전역 변수 ===== */
static I2cDmaHandle  *g_i2c_drv   = NULL;
static TIM_HandleTypeDef *g_htim  = NULL;   /**< 15ms 대기용 타이머 (TIM6) */

static Sht31DmaState  g_state     = SHT31_DMA_IDLE;
static uint8_t        g_tx_buf[2] = SHT31_CMD_MEAS_HIGHREP;
static uint8_t        g_rx_buf[SHT31_DATA_SIZE];

static float          g_temperature = 0.0f;   /**< 최근 온도 (°C) */
static float          g_humidity    = 0.0f;   /**< 최근 상대 습도 (%) */
static bool           g_data_ready  = false;  /**< 새 데이터 준비 플래그 */
static uint32_t       g_meas_count  = 0;      /**< 측정 완료 횟수 */

/* ===== 내부 함수 프로토타입 ===== */
static uint8_t sht31_calc_crc(const uint8_t *data, uint8_t len);
static void    sht31_parse_raw(const uint8_t *raw);
static void    on_tx_cplt(void);
static void    on_rx_cplt(void);

/* ===================================================================
 * 초기화
 * =================================================================== */
HAL_StatusTypeDef SHT31_DMA_Init(I2cDmaHandle *i2c_drv, TIM_HandleTypeDef *htim_wait)
{
    if (i2c_drv == NULL || htim_wait == NULL) {
        LOG_E("SHT31_DMA_Init: NULL 포인터");
        return HAL_ERROR;
    }

    g_i2c_drv = i2c_drv;
    g_htim    = htim_wait;
    g_state   = SHT31_DMA_IDLE;

    LOG_I("SHT31_DMA_Init: SHT31 DMA 드라이버 초기화 완료");
    return HAL_OK;
}

/* ===================================================================
 * 측정 시작 (비동기 — 즉시 반환)
 * =================================================================== */
HAL_StatusTypeDef SHT31_StartMeasurement(void)
{
    HAL_StatusTypeDef ret;

    if (g_state != SHT31_DMA_IDLE) {
        LOG_W("SHT31_StartMeasurement: 이전 측정 진행 중 (state=%d)", g_state);
        return HAL_BUSY;
    }

    g_data_ready = false;
    g_state      = SHT31_DMA_CMD_TX;

    LOG_D("SHT31_StartMeasurement: CMD 전송 시작");

    /* 명령 전송 — 완료 시 on_tx_cplt() 호출 */
    ret = I2cDma_MasterTransmit(g_i2c_drv,
                                SHT31_I2C_ADDR,
                                g_tx_buf,
                                2U,
                                on_tx_cplt);
    if (ret != HAL_OK) {
        g_state = SHT31_DMA_ERROR;
        LOG_E("SHT31_StartMeasurement: I2C TX 시작 실패 (ret=%d)", ret);
    }

    return ret;
}

/* ===================================================================
 * 데이터 읽기 (동기 — 마지막 완료 데이터 반환)
 * =================================================================== */
bool SHT31_GetData(float *temp, float *humi)
{
    if (!g_data_ready) {
        return false;   /**< 아직 새 데이터 없음 */
    }

    *temp = g_temperature;
    *humi = g_humidity;
    g_data_ready = false;   /**< 소비 완료 플래그 클리어 */

    return true;
}

uint32_t SHT31_GetMeasCount(void)
{
    return g_meas_count;
}

/* ===================================================================
 * 콜백 체인 구현
 * =================================================================== */

/**
 * @brief TX 완료 콜백 — I2C CMD 전송 완료 후 호출 (ISR 컨텍스트)
 *
 * Repeated Start 미지원 우회:
 *   STOP이 발행된 후 이 콜백이 호출됩니다.
 *   SHT31 내부 측정 시간(최대 15ms) 대기 후 데이터를 수신합니다.
 *   TIM6를 15ms 원샷 타이머로 사용합니다.
 */
static void on_tx_cplt(void)
{
    g_state = SHT31_DMA_WAIT_MEAS;

    LOG_D("SHT31 on_tx_cplt: CMD 전송 완료 → 15ms 측정 대기 시작");

    /* TIM6 원샷 모드 15ms 타이머 시작 */
    /* TIM6 설정: PSC=839, ARR=1499
     * 계산: TIM6 클럭=84MHz, 타이머클럭=84M/840=100kHz, 주기=1500×(1/100kHz)=15ms */
    HAL_TIM_Base_Start_IT(g_htim);
}

/**
 * @brief TIM6 인터럽트 핸들러에서 호출 — 15ms 경과 후 데이터 수신 시작
 *        실제 코드에서는 HAL_TIM_PeriodElapsedCallback에서 분기
 */
void SHT31_DMA_OnTimerElapsed(void)
{
    HAL_StatusTypeDef ret;

    if (g_state != SHT31_DMA_WAIT_MEAS) {
        return;   /**< 이 모듈의 타이머 이벤트가 아님 */
    }

    HAL_TIM_Base_Stop_IT(g_htim);  /**< 원샷: 타이머 중지 */

    g_state = SHT31_DMA_DATA_RX;

    LOG_D("SHT31 OnTimerElapsed: 15ms 경과 → 데이터 수신 시작");

    /* 데이터 수신 — 완료 시 on_rx_cplt() 호출 */
    ret = I2cDma_MasterReceive(g_i2c_drv,
                               SHT31_I2C_ADDR,
                               g_rx_buf,
                               SHT31_DATA_SIZE,
                               on_rx_cplt);
    if (ret != HAL_OK) {
        g_state = SHT31_DMA_ERROR;
        LOG_E("SHT31 OnTimerElapsed: I2C RX 시작 실패 (ret=%d)", ret);
    }
}

/**
 * @brief RX 완료 콜백 — 6바이트 데이터 수신 완료 (ISR 컨텍스트)
 */
static void on_rx_cplt(void)
{
    LOG_D("SHT31 on_rx_cplt: 데이터 수신 완료 → 파싱 시작");

    sht31_parse_raw(g_rx_buf);

    g_meas_count++;
    g_state      = SHT31_DMA_IDLE;
    g_data_ready = true;

    LOG_I("SHT31 측정 완료 #%lu: 온도=%.1f°C, 습도=%.1f%%",
          g_meas_count, g_temperature, g_humidity);
}

/* ===================================================================
 * 내부 헬퍼 함수
 * =================================================================== */

/**
 * @brief CRC-8 검증 (다항식 0x31, 초기값 0xFF — SHT31 규격)
 */
static uint8_t sht31_calc_crc(const uint8_t *data, uint8_t len)
{
    uint8_t crc = SHT31_CRC_INIT;

    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80U) {
                crc = (crc << 1) ^ SHT31_CRC_POLY;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/**
 * @brief SHT31 원시 데이터 파싱
 * @param raw  6바이트 버퍼 [T_MSB, T_LSB, T_CRC, H_MSB, H_LSB, H_CRC]
 */
static void sht31_parse_raw(const uint8_t *raw)
{
    uint8_t crc_temp = sht31_calc_crc(raw, 2U);
    uint8_t crc_humi = sht31_calc_crc(raw + 3U, 2U);

    if (crc_temp != raw[2] || crc_humi != raw[5]) {
        LOG_W("SHT31: CRC 오류 (온도CRC: 계산=0x%02X 수신=0x%02X, 습도CRC: 계산=0x%02X 수신=0x%02X)",
              crc_temp, raw[2], crc_humi, raw[5]);
        /* 이전 데이터 유지 — g_temperature, g_humidity 갱신하지 않음 */
        return;
    }

    uint16_t raw_t = ((uint16_t)raw[0] << 8) | raw[1];
    uint16_t raw_h = ((uint16_t)raw[3] << 8) | raw[4];

    /* SHT31 데이터시트 변환 공식 */
    g_temperature = -45.0f + 175.0f * ((float)raw_t / 65535.0f);
    g_humidity    = 100.0f * ((float)raw_h / 65535.0f);

    LOG_D("SHT31 파싱: raw_t=0x%04X → %.2f°C, raw_h=0x%04X → %.2f%%",
          raw_t, g_temperature, raw_h, g_humidity);
}
