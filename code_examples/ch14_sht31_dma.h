/**
 * @file    ch14_sht31_dma.h
 * @brief   SHT31 드라이버 — DMA 비동기 공개 인터페이스 (Ch14 v1.4)
 */

#ifndef CH14_SHT31_DMA_H
#define CH14_SHT31_DMA_H

#include "stm32f4xx_hal.h"
#include "ch14_i2c_dma_driver.h"
#include <stdbool.h>

/**
 * @brief SHT31 DMA 드라이버 초기화
 * @param i2c_drv    I2C DMA 드라이버 핸들
 * @param htim_wait  15ms 대기용 타이머 핸들 (TIM6 원샷 모드 권장)
 */
HAL_StatusTypeDef SHT31_DMA_Init(I2cDmaHandle *i2c_drv, TIM_HandleTypeDef *htim_wait);

/**
 * @brief 비동기 측정 시작 — 즉시 반환
 * @return HAL_OK: 시작됨, HAL_BUSY: 이전 측정 진행 중
 *
 * 측정 완료는 SHT31_GetData()의 반환값으로 확인합니다.
 */
HAL_StatusTypeDef SHT31_StartMeasurement(void);

/**
 * @brief 최근 측정 데이터 읽기
 * @param temp  온도 저장 포인터 (°C)
 * @param humi  습도 저장 포인터 (%)
 * @return true: 새 데이터 반환됨, false: 아직 없음
 */
bool SHT31_GetData(float *temp, float *humi);

/**
 * @brief 누적 측정 완료 횟수 반환 (디버그 용도)
 */
uint32_t SHT31_GetMeasCount(void);

/**
 * @brief 15ms 타이머 만료 시 호출 — HAL_TIM_PeriodElapsedCallback에서 분기
 *
 * 사용 예:
 * @code
 * void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
 *     if (htim->Instance == TIM6) {
 *         SHT31_DMA_OnTimerElapsed();
 *     }
 * }
 * @endcode
 */
void SHT31_DMA_OnTimerElapsed(void);

#endif /* CH14_SHT31_DMA_H */
