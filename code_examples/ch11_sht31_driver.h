/**
 * ch11_sht31_driver.h
 * SHT31 Device Driver 헤더 — 센서 제어 + CRC 검증
 *
 * 역할: I2C Driver 위에서 SHT31 센서 명령어와 CRC 검증을 구현합니다.
 *       상위 Service 레이어에는 "온습도 읽기"라는 고수준 인터페이스를 제공합니다.
 */

#ifndef CH11_SHT31_DRIVER_H
#define CH11_SHT31_DRIVER_H

#include <stdint.h>

/* SHT31 정의 */
#define SHT31_ADDR      0x44       // I2C 주소 (7비트)
#define SHT31_MEAS_CMD  {0x24, 0x00}  // 측정 명령
#define SHT31_RESP_LEN  6          // 응답 길이 (온도2 + CRC1 + 습도2 + CRC1)

/* SHT31 응답 프레임 인덱스 (6바이트: [온도H][온도L][온도CRC][습도H][습도L][습도CRC]) */
#define SHT31_TEMP_H_IDX    0      // 온도 상위 바이트 인덱스
#define SHT31_TEMP_L_IDX    1      // 온도 하위 바이트 인덱스
#define SHT31_TEMP_CRC_IDX  2      // 온도 CRC 인덱스
#define SHT31_HUM_H_IDX     3      // 습도 상위 바이트 인덱스
#define SHT31_HUM_L_IDX     4      // 습도 하위 바이트 인덱스
#define SHT31_HUM_CRC_IDX   5      // 습도 CRC 인덱스

/* SHT31 상태 코드 */
typedef int32_t sht31_status_t;
#define SHT31_OK        0
#define SHT31_ERROR     -1
#define SHT31_CRC_ERR   -2

/**
 * @brief SHT31 초기화 (I2C 통신 확인)
 *
 * 센서와의 I2C 통신이 정상인지 확인합니다.
 *
 * @return SHT31_OK (0), SHT31_ERROR (-1)
 */
sht31_status_t sht31_init(void);

/**
 * @brief SHT31 온습도 데이터 읽기 (블로킹)
 *
 * 단계:
 * 1. 측정 명령 송신 (0x24, 0x00)
 * 2. 센서 측정 대기 (약 50ms)
 * 3. 데이터 수신 (6바이트)
 * 4. CRC-8 검증
 * 5. 온습도 계산
 *
 * @param p_temp 온도 출력 포인터 (°C)
 * @param p_humid 습도 출력 포인터 (%)
 * @return SHT31_OK (0), SHT31_ERROR (-1), 또는 SHT31_CRC_ERR (-2)
 */
sht31_status_t sht31_read_temp_humid(float *p_temp, float *p_humid);

/**
 * @brief SHT31 측정 시작 (비블로킹)
 *
 * 측정 명령만 송신하고 즉시 반환합니다.
 * 이후 sht31_read_measurement()를 호출하기 전에 최소 50ms 경과 필요.
 *
 * @return SHT31_OK (0), SHT31_ERROR (-1)
 */
sht31_status_t sht31_trigger_measurement(void);

/**
 * @brief SHT31 측정 결과 수신 (비블로킹)
 *
 * trigger_measurement() 호출 후 최소 50ms 이상 경과한 후 호출해야 합니다.
 * (Service FSM에서 시간 관리)
 *
 * @param p_temp 온도 출력 포인터
 * @param p_humid 습도 출력 포인터
 * @return SHT31_OK (0), SHT31_ERROR (-1), 또는 SHT31_CRC_ERR (-2)
 */
sht31_status_t sht31_read_measurement(float *p_temp, float *p_humid);

#endif  // CH11_SHT31_DRIVER_H
