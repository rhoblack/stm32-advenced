/**
 * @file    protocol_app.c
 * @brief   PC 대시보드 통신 프로토콜 구현 — App 레이어
 * @details 시리얼 프레임 직렬화(Serialization):
 *          [STX][TYPE][LEN][PAYLOAD...][CRC16_HI][CRC16_LO][ETX]
 *
 *          CRC-16/CCITT: 다항식 0x1021, 초기값 0xFFFF
 *          CRC 계산 범위: TYPE + LEN + PAYLOAD (STX, ETX 제외)
 *
 * @version v1.6 — Ch16 신규 (마일스톤 ②)
 */

#include "protocol_app.h"
#include "uart_driver.h"   /* Ch06: UART_Send(), UART_IsTxBusy() */
#include "log.h"           /* Ch02: LOG_D/I/W/E */
#include <string.h>
#include <stddef.h>

/* ────────────────────────────────────────────────
 * 내부 버퍼 (정적 할당 — 힙 사용 금지)
 * ──────────────────────────────────────────────── */
static uint8_t  s_tx_frame[PROTO_MAX_FRAME];  /**< 송신 프레임 버퍼 */
static uint32_t s_frame_count = 0;            /**< 누적 전송 프레임 수 */

/* ────────────────────────────────────────────────
 * 내부 함수 선언
 * ──────────────────────────────────────────────── */
static uint16_t build_frame(Proto_TypeDef type,
                             const uint8_t *payload, uint8_t payload_len,
                             uint8_t *out_frame);

/* ────────────────────────────────────────────────
 * 공개 API 구현
 * ──────────────────────────────────────────────── */

HAL_StatusTypeDef Protocol_Init(void)
{
    LOG_I("Protocol_Init: 프로토콜 앱 레이어 초기화");
    memset(s_tx_frame, 0, sizeof(s_tx_frame));
    s_frame_count = 0;
    LOG_D("Protocol_Init: 프레임 버퍼 크기 = %d bytes", PROTO_MAX_FRAME);
    return HAL_OK;
}

HAL_StatusTypeDef Protocol_SendSensorData(float temp, float humi)
{
    /* 페이로드: 온도(4B) + 습도(4B) = 8B */
    uint8_t payload[8];
    uint16_t frame_len;

    /* float → 바이트 직렬화 (리틀 엔디언) */
    memcpy(&payload[0], &temp, sizeof(float));
    memcpy(&payload[4], &humi, sizeof(float));

    LOG_D("Protocol_SendSensorData: temp=%.1f humi=%.1f", temp, humi);

    frame_len = build_frame(PROTO_TYPE_SENSOR_DATA, payload, 8, s_tx_frame);

    if (UART_IsTxBusy())
    {
        LOG_W("Protocol_SendSensorData: TX 버스 중 — 프레임 버림");
        return HAL_BUSY;
    }

    s_frame_count++;
    LOG_D("Protocol_SendSensorData: 프레임 #%lu 전송 (len=%d)", s_frame_count, frame_len);
    return UART_Send(s_tx_frame, frame_len);
}

HAL_StatusTypeDef Protocol_SendTimeData(uint8_t hour, uint8_t min, uint8_t sec)
{
    /* 페이로드: 시(1B) + 분(1B) + 초(1B) = 3B */
    uint8_t payload[3] = { hour, min, sec };
    uint16_t frame_len;

    LOG_D("Protocol_SendTimeData: %02d:%02d:%02d", hour, min, sec);

    frame_len = build_frame(PROTO_TYPE_TIME_DATA, payload, 3, s_tx_frame);

    if (UART_IsTxBusy())
    {
        LOG_W("Protocol_SendTimeData: TX 버스 중");
        return HAL_BUSY;
    }

    s_frame_count++;
    return UART_Send(s_tx_frame, frame_len);
}

HAL_StatusTypeDef Protocol_SendMotorStatus(uint16_t angle, uint8_t state)
{
    /* 페이로드: 각도(2B, 리틀 엔디언) + 상태(1B) = 3B */
    uint8_t payload[3];
    uint16_t frame_len;

    payload[0] = (uint8_t)(angle & 0xFF);
    payload[1] = (uint8_t)((angle >> 8) & 0xFF);
    payload[2] = state;

    LOG_D("Protocol_SendMotorStatus: angle=%d state=%d", angle, state);

    frame_len = build_frame(PROTO_TYPE_MOTOR_STATUS, payload, 3, s_tx_frame);

    if (UART_IsTxBusy())
    {
        LOG_W("Protocol_SendMotorStatus: TX 버스 중");
        return HAL_BUSY;
    }

    s_frame_count++;
    return UART_Send(s_tx_frame, frame_len);
}

HAL_StatusTypeDef Protocol_SendSystemStatus(void)
{
    /* 페이로드: 버전(2B) + 업타임(4B) = 6B */
    uint8_t payload[6];
    uint16_t version  = 0x0106;  /* v1.6 */
    uint32_t uptime   = HAL_GetTick() / 1000U;  /* 초 단위 업타임 */
    uint16_t frame_len;

    payload[0] = (uint8_t)(version & 0xFF);
    payload[1] = (uint8_t)((version >> 8) & 0xFF);
    memcpy(&payload[2], &uptime, sizeof(uint32_t));

    LOG_I("Protocol_SendSystemStatus: v%d.%d 업타임=%lus",
          (version >> 8), (version & 0xFF), uptime);

    frame_len = build_frame(PROTO_TYPE_SYSTEM_STATUS, payload, 6, s_tx_frame);

    if (UART_IsTxBusy())
    {
        LOG_W("Protocol_SendSystemStatus: TX 버스 중");
        return HAL_BUSY;
    }

    s_frame_count++;
    return UART_Send(s_tx_frame, frame_len);
}

uint16_t Protocol_CalcCRC16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFU;  /* 초기값 */
    uint16_t i;
    uint8_t  j;

    for (i = 0; i < len; i++)
    {
        crc ^= ((uint16_t)data[i] << 8);
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x8000U)
            {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);  /* 다항식 0x1021 */
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void Protocol_ProcessRxFrame(void)
{
    /*
     * TODO (Ch17 확장): PC → MCU 명령 수신 처리
     * 현재는 ACK 전송만 구현 (수신 프레임 검증 생략)
     */
    LOG_D("Protocol_ProcessRxFrame: 수신 처리 (미구현 — Ch17 확장 예정)");
}

/* ────────────────────────────────────────────────
 * 내부 함수 구현
 * ──────────────────────────────────────────────── */

/**
 * @brief 프레임 조립
 * @details 구조: [STX][TYPE][LEN][PAYLOAD...][CRC16_HI][CRC16_LO][ETX]
 *          CRC 계산 범위: TYPE(1) + LEN(1) + PAYLOAD(n) 바이트
 * @param type        프레임 타입
 * @param payload     페이로드 데이터
 * @param payload_len 페이로드 길이 (0~32)
 * @param out_frame   출력 프레임 버퍼 (최소 PROTO_MAX_FRAME 크기)
 * @return 완성된 프레임 총 길이 (바이트)
 */
static uint16_t build_frame(Proto_TypeDef type,
                             const uint8_t *payload, uint8_t payload_len,
                             uint8_t *out_frame)
{
    uint16_t crc;
    uint16_t idx = 0;

    /* 1. STX */
    out_frame[idx++] = PROTO_STX;

    /* 2. TYPE */
    out_frame[idx++] = (uint8_t)type;

    /* 3. LEN */
    out_frame[idx++] = payload_len;

    /* 4. PAYLOAD */
    if (payload != NULL && payload_len > 0)
    {
        memcpy(&out_frame[idx], payload, payload_len);
        idx += payload_len;
    }

    /* 5. CRC16 — TYPE + LEN + PAYLOAD 범위 계산 */
    crc = Protocol_CalcCRC16(&out_frame[1], 2U + payload_len);
    out_frame[idx++] = (uint8_t)((crc >> 8) & 0xFFU);  /* CRC 상위 바이트 */
    out_frame[idx++] = (uint8_t)(crc & 0xFFU);          /* CRC 하위 바이트 */

    /* 6. ETX */
    out_frame[idx++] = PROTO_ETX;

    LOG_D("build_frame: type=0x%02X len=%d total=%d CRC=0x%04X",
          (uint8_t)type, payload_len, idx, crc);

    return idx;
}
