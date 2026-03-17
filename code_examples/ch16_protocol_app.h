/**
 * @file    protocol_app.h
 * @brief   PC 대시보드 통신 프로토콜 앱 레이어 — 공개 인터페이스
 * @details 시리얼 프레임(STX/TYPE/LEN/PAYLOAD/CRC16/ETX) 직렬화 및
 *          UART DMA 송신 관리. Ch15 uart_driver.c 위에서 동작.
 *
 * @note    프레임 구조:
 *          [STX 0xAA][TYPE 1B][LEN 1B][PAYLOAD 0~32B][CRC16 2B][ETX 0x55]
 *
 * @version v1.6 — Ch16 신규 (마일스톤 ②)
 */

#ifndef PROTOCOL_APP_H
#define PROTOCOL_APP_H

#include "main.h"
#include <stdint.h>

/* ────────────────────────────────────────────────
 * 프레임 상수 정의
 * ──────────────────────────────────────────────── */
#define PROTO_STX           0xAAU   /**< 프레임 시작 바이트 */
#define PROTO_ETX           0x55U   /**< 프레임 종료 바이트 */
#define PROTO_MAX_PAYLOAD   32U     /**< 최대 페이로드 크기 (바이트) */
#define PROTO_FRAME_OVERHEAD 6U     /**< STX+TYPE+LEN+CRC16(2)+ETX */
#define PROTO_MAX_FRAME     (PROTO_MAX_PAYLOAD + PROTO_FRAME_OVERHEAD)

/* ────────────────────────────────────────────────
 * 프레임 타입 정의
 * ──────────────────────────────────────────────── */
typedef enum {
    PROTO_TYPE_SENSOR_DATA    = 0x01U,  /**< 온도(4B float) + 습도(4B float) = 8B */
    PROTO_TYPE_TIME_DATA      = 0x02U,  /**< 시(1B) + 분(1B) + 초(1B) = 3B */
    PROTO_TYPE_MOTOR_STATUS   = 0x03U,  /**< 각도(2B uint16) + 상태(1B) = 3B */
    PROTO_TYPE_SYSTEM_STATUS  = 0x04U,  /**< 버전(2B) + 업타임(4B) = 6B */
    PROTO_TYPE_ACK            = 0x05U,  /**< 결과 코드(1B) */
} Proto_TypeDef;

/* ────────────────────────────────────────────────
 * 공개 API
 * ──────────────────────────────────────────────── */

/**
 * @brief 프로토콜 앱 레이어 초기화
 * @note  uart_driver 초기화 이후 호출
 * @return HAL_OK 또는 HAL_ERROR
 */
HAL_StatusTypeDef Protocol_Init(void);

/**
 * @brief 센서 데이터 프레임 전송
 * @param temp  온도 (°C, IEEE 754 float)
 * @param humi  상대 습도 (%, IEEE 754 float)
 * @return HAL_OK 또는 HAL_BUSY (이전 전송 진행 중)
 */
HAL_StatusTypeDef Protocol_SendSensorData(float temp, float humi);

/**
 * @brief 현재 시각 프레임 전송
 * @param hour  시 (0~23)
 * @param min   분 (0~59)
 * @param sec   초 (0~59)
 * @return HAL_OK 또는 HAL_BUSY
 */
HAL_StatusTypeDef Protocol_SendTimeData(uint8_t hour, uint8_t min, uint8_t sec);

/**
 * @brief 스텝 모터 상태 프레임 전송
 * @param angle  현재 각도 (0~4095 하프스텝 단위)
 * @param state  모터 FSM 상태 (0=IDLE, 1=MOVING, 2=HOMING, 3=ERROR)
 * @return HAL_OK 또는 HAL_BUSY
 */
HAL_StatusTypeDef Protocol_SendMotorStatus(uint16_t angle, uint8_t state);

/**
 * @brief 시스템 상태 프레임 전송 (주기: 10초)
 * @return HAL_OK 또는 HAL_BUSY
 */
HAL_StatusTypeDef Protocol_SendSystemStatus(void);

/**
 * @brief CRC-16/CCITT 계산 (다항식 0x1021, 초기값 0xFFFF)
 * @param data  입력 데이터 버퍼
 * @param len   버퍼 길이 (바이트)
 * @return CRC-16 결과값
 */
uint16_t Protocol_CalcCRC16(const uint8_t *data, uint16_t len);

/**
 * @brief 수신 프레임 처리 (PC → MCU 명령 파싱)
 * @note  메인 루프 또는 UART RX 콜백에서 호출
 */
void Protocol_ProcessRxFrame(void);

#endif /* PROTOCOL_APP_H */
