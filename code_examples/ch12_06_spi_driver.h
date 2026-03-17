/**
 * ch12_06_spi_driver.h
 * SPI 드라이버 공개 인터페이스
 *
 * 이 헤더는 SPI 통신의 저수준 함수들을 제공합니다.
 * HAL과 응용 계층 사이에 위치합니다.
 */

#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include "main.h"

/**
 * ===== 초기화 함수 =====
 */

/**
 * @brief SPI1 드라이버 초기화
 *
 * 이 함수는 CubeMX에서 생성된 MX_GPIO_Init(), MX_SPI1_Init(),
 * MX_DMA_Init() 실행 후에 호출되어야 합니다.
 *
 * 초기화 내용:
 * - CS 핀 (PA3) = HIGH (LCD 비선택)
 * - D/C 핀 (PA4) = LOW (명령어 모드)
 * - RESET 핀 (PA5) = HIGH
 * - DMA 콜백 등록
 */
void SPI_Driver_Init(void);

/**
 * ===== 통신 함수 =====
 */

/**
 * @brief SPI DMA 전송 (비동기)
 *
 * @param pData: 송신 버퍼 (반드시 static 또는 heap!)
 * @param Size: 전송 바이트 수
 *
 * @return HAL_OK 성공, HAL_BUSY 전송 중, HAL_ERROR 실패
 *
 * 동작:
 * 1. CS = LOW (LCD 선택)
 * 2. DMA로 SPI 전송 시작 (비동기, CPU 대기 안 함)
 * 3. 전송 완료 시 콜백 호출 (CS = HIGH)
 *
 * 주의:
 * - 동시에 2개 이상의 전송을 시작하면 안 됨
 * - 버퍼는 스택에 할당하면 안 됨!
 *
 * 예제:
 *   static uint8_t data[] = {0x11, 0x22, 0x33};
 *   SPI_Transmit_DMA(data, 3);
 *   SPI_Wait_Complete();
 */
HAL_StatusTypeDef SPI_Transmit_DMA(uint8_t *pData, uint16_t Size);

/**
 * @brief SPI 폴링 전송 (동기)
 *
 * @param pData: 송신 버퍼
 * @param Size: 전송 바이트 수
 *
 * @return HAL_OK 성공, 그 외 실패
 *
 * 동작:
 * 1. CS = LOW
 * 2. SPI로 전송 (폴링, CPU 대기)
 * 3. CS = HIGH
 *
 * 사용:
 * - LCD 초기화 명령어 전송 (작은 데이터)
 * - 응답 기다릴 필요 없는 단발성 전송
 *
 * 예제:
 *   uint8_t cmd = 0x11;
 *   SPI_Transmit_Polling(&cmd, 1);
 */
HAL_StatusTypeDef SPI_Transmit_Polling(uint8_t *pData, uint16_t Size);

/**
 * ===== 동기화 함수 =====
 */

/**
 * @brief DMA 전송 완료 대기
 *
 * 이 함수는 DMA 전송이 완료될 때까지 Busy-Wait합니다.
 * 초기화 명령어들 사이에 정확한 타이밍을 유지하기 위해 사용합니다.
 *
 * 예제:
 *   SPI_Transmit_DMA(cmd_buf, size);
 *   SPI_Wait_Complete();
 *   Delay_Ms(5);  // 안전한 대기
 *   // 다음 명령어 전송
 */
void SPI_Wait_Complete(void);

/**
 * ===== 유틸리티 함수 =====
 */

/**
 * @brief SPI 상태 조회
 *
 * @return HAL_SPI_STATE_RESET, READY, BUSY 등
 */
uint32_t SPI_Get_State(void);

/**
 * @brief SPI 에러 코드 조회
 *
 * @return 에러 코드 (0 = 에러 없음)
 */
uint32_t SPI_Get_Error(void);

/**
 * ===== 핀 제어 함수 =====
 */

/**
 * @brief CS 핀 제어 (수동)
 *
 * @param state: GPIO_PIN_SET (HIGH) 또는 GPIO_PIN_RESET (LOW)
 *
 * 일반적으로 DMA 콜백에서 자동으로 관리되지만,
 * 특수한 경우 수동으로 제어해야 할 수 있습니다.
 *
 * 예제:
 *   SPI_CS_Set(GPIO_PIN_RESET);  // CS = LOW
 */
void SPI_CS_Set(GPIO_PinState state);

/**
 * @brief D/C 핀 제어
 *
 * @param state: GPIO_PIN_SET (1=데이터) 또는 GPIO_PIN_RESET (0=명령어)
 *
 * 예제:
 *   SPI_DC_Set(GPIO_PIN_RESET);  // D/C = 0 (명령어)
 *   SPI_Transmit_Polling(&cmd, 1);
 *   SPI_DC_Set(GPIO_PIN_SET);    // D/C = 1 (데이터)
 *   SPI_Transmit_Polling(data, len);
 */
void SPI_DC_Set(GPIO_PinState state);

/**
 * @brief RESET 핀 제어
 *
 * @param state: GPIO_PIN_SET (HIGH) 또는 GPIO_PIN_RESET (LOW)
 *
 * 예제:
 *   SPI_RESET_Set(GPIO_PIN_RESET);  // RESET = LOW
 *   Delay_Ms(10);
 *   SPI_RESET_Set(GPIO_PIN_SET);    // RESET = HIGH
 */
void SPI_RESET_Set(GPIO_PinState state);

/**
 * ===== 콜백 함수 (약한 심볼, 사용자 정의 가능) =====
 */

/**
 * @brief DMA 전송 완료 콜백
 *
 * 이 함수는 SPI DMA 전송이 완료되었을 때 자동으로 호출됩니다.
 * 사용자는 이를 오버라이드하여 추가 작업을 수행할 수 있습니다.
 *
 * 기본 동작: CS = HIGH (LCD 비선택)
 *
 * 예제 (커스텀 구현):
 *   void SPI_DMA_Callback(void)
 *   {
 *       // 기본 동작
 *       SPI_CS_Set(GPIO_PIN_SET);
 *
 *       // 추가 작업
 *       my_dma_complete_flag = 1;
 *   }
 */
void SPI_DMA_Callback(void);

/**
 * @brief SPI 에러 콜백
 *
 * SPI 또는 DMA 에러 발생 시 호출됩니다.
 * 기본 동작: CS = HIGH (안전한 상태)
 *
 * 예제:
 *   void SPI_Error_Callback(void)
 *   {
 *       SPI_CS_Set(GPIO_PIN_SET);
 *       LOG_E("SPI 에러 발생!");
 *   }
 */
void SPI_Error_Callback(void);

#endif /* SPI_DRIVER_H */
