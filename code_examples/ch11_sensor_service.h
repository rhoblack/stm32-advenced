/**
 * ch11_sensor_service.h
 * Sensor Service 헤더 — FSM 기반 비즈니스 로직
 *
 * 역할: SHT31 Device Driver 위에서 FSM을 구현하여,
 *       온습도 측정을 상태별로 관리하고, 온도 임계값 알람을 생성합니다.
 *
 * FSM 상태:
 *   - SENSOR_IDLE: 대기 중
 *   - SENSOR_MEASURING: 측정 진행 중 (50ms 대기)
 *   - SENSOR_DATA_READY: 데이터 준비 완료
 *   - SENSOR_ALARM: 온도 > 임계값
 *   - SENSOR_ERROR: 통신/CRC 에러
 */

#ifndef CH11_SENSOR_SERVICE_H
#define CH11_SENSOR_SERVICE_H

#include <stdint.h>

/* FSM 상태 */
typedef enum {
    SENSOR_IDLE,          // 대기
    SENSOR_MEASURING,     // 측정 중
    SENSOR_DATA_READY,    // 데이터 준비 완료
    SENSOR_ALARM,         // 온도 임계값 초과
    SENSOR_ERROR          // 에러
} sensor_state_t;

/* 센서 데이터 구조체 */
typedef struct {
    float temp;   // 온도 (°C)
    float humid;  // 습도 (%)
} sensor_data_t;

/**
 * @brief Sensor Service 초기화
 *
 * SHT31 드라이버를 초기화하고 FSM 상태를 IDLE로 설정합니다.
 *
 * @return 0 (OK), -1 (초기화 실패)
 */
int32_t sensor_service_init(void);

/**
 * @brief 측정 시작 (FSM: IDLE → MEASURING)
 *
 * 현재 상태가 IDLE이면 측정을 시작하고 MEASURING으로 전환합니다.
 * 다른 상태에서는 무시됩니다.
 */
void sensor_service_start_measurement(void);

/**
 * @brief FSM 프로세스 (메인 루프에서 주기적 호출)
 *
 * 측정 시간 경과(50ms)를 확인하고, 데이터를 수신합니다.
 * 메인 루프에서 100ms 주기로 호출하는 것을 권장합니다.
 */
void sensor_service_process(void);

/**
 * @brief 현재 FSM 상태 조회
 */
sensor_state_t sensor_service_get_state(void);

/**
 * @brief 측정 데이터 획득 (SENSOR_DATA_READY 또는 SENSOR_ALARM 상태에서만 유효)
 *
 * 데이터를 읽은 후 상태는 자동으로 IDLE로 복귀합니다.
 *
 * @param p_temp 온도 출력 포인터
 * @param p_humid 습도 출력 포인터
 * @return 0 (OK), -1 (상태 불일치)
 */
int32_t sensor_service_get_data(float *p_temp, float *p_humid);

/**
 * @brief 온도 임계값 설정
 *
 * @param temp_high 온도 상한 (°C, 예: 30.0)
 * @param temp_low 온도 하한 (°C, 예: 15.0)
 */
void sensor_service_set_temp_threshold(float temp_high, float temp_low);

/**
 * @brief 온도 알람 확인 및 클리어
 *
 * 온도 알람 플래그를 확인하고, 클리어합니다.
 * 알람이 발생했으면 1을 반환합니다.
 *
 * @param p_alarm_flag 알람 플래그 출력 포인터 (1=알람 발생, 0=정상)
 * @return 0 (OK), -1 (에러)
 */
int32_t sensor_service_check_alarm(uint8_t *p_alarm_flag);

#endif  // CH11_SENSOR_SERVICE_H
