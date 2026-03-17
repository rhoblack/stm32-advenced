/**
 * ch11_sensor_service.c
 * Sensor Service 구현 — FSM 기반 센서 관리
 */

#include "ch11_sensor_service.h"
#include "ch11_sht31_driver.h"
#include "main.h"
#include "log.h"

/* 전역 상태 머신 변수 */
static sensor_state_t g_state = SENSOR_IDLE;
static sensor_data_t g_data = {0.0f, 0.0f};
static float g_temp_high = 30.0f;     // 온도 상한
static float g_temp_low = 15.0f;      // 온도 하한
static uint8_t g_alarm_flag = 0;      // 알람 플래그
static uint32_t g_tick_start = 0;     // 측정 시작 시간

/**
 * @brief 초기화
 */
int32_t sensor_service_init(void)
{
    LOG_I("sensor_service_init: Sensor Service 초기화");

    // SHT31 드라이버 초기화
    if (sht31_init() != SHT31_OK) {
        LOG_E("sensor_service_init: SHT31 초기화 실패");
        g_state = SENSOR_ERROR;
        return -1;
    }

    g_state = SENSOR_IDLE;
    g_alarm_flag = 0;

    LOG_I("sensor_service_init: 초기화 완료");
    return 0;
}

/**
 * @brief 측정 시작 (IDLE → MEASURING)
 */
void sensor_service_start_measurement(void)
{
    if (g_state == SENSOR_IDLE) {
        LOG_D("sensor_service_start_measurement: IDLE → MEASURING");

        if (sht31_trigger_measurement() == SHT31_OK) {
            g_state = SENSOR_MEASURING;
            g_tick_start = HAL_GetTick();  // 시작 시간 기록
        } else {
            g_state = SENSOR_ERROR;
        }
    }
}

/**
 * @brief FSM 프로세스 (메인 루프에서 호출)
 *
 * 상태별 처리:
 *   - IDLE: 대기
 *   - MEASURING: 50ms 경과 확인 → 데이터 수신
 *   - DATA_READY / ALARM: 대기 (애플리케이션이 데이터 읽음)
 *   - ERROR: 에러 상태
 */
void sensor_service_process(void)
{
    sht31_status_t status;

    switch (g_state) {
        case SENSOR_IDLE:
            // 대기 상태, 다음 측정 기다림
            break;

        case SENSOR_MEASURING:
            // 측정 대기: 50ms 이상 경과했는지 확인
            if (HAL_GetTick() - g_tick_start >= 50) {
                LOG_D("sensor_service_process: 50ms 경과, 데이터 수신");

                // 데이터 수신 (비블로킹)
                status = sht31_read_measurement(&g_data.temp,
                                                 &g_data.humid);

                if (status == SHT31_OK) {
                    LOG_I("sensor_service_process: 데이터 수신 OK");
                    LOG_I("  온도: %.2f°C, 습도: %.2f%%",
                          g_data.temp, g_data.humid);

                    // 온도 임계값 검사
                    if (g_data.temp > g_temp_high) {
                        g_state = SENSOR_ALARM;
                        g_alarm_flag = 1;
                        LOG_W("sensor_service_process: 온도 알람!");
                        LOG_W("  온도 %.2f°C > 임계값 %.2f°C",
                              g_data.temp, g_temp_high);
                    } else {
                        g_state = SENSOR_DATA_READY;
                    }
                } else if (status == SHT31_CRC_ERR) {
                    LOG_E("sensor_service_process: CRC 에러");
                    g_state = SENSOR_ERROR;
                } else {
                    LOG_E("sensor_service_process: I2C 통신 에러");
                    g_state = SENSOR_ERROR;
                }
            }
            break;

        case SENSOR_DATA_READY:
            // 데이터 준비됨, 애플리케이션이 읽음을 기다림
            // get_data() 호출 후 IDLE로 복귀
            break;

        case SENSOR_ALARM:
            // 알람 발생, 애플리케이션이 확인을 기다림
            // check_alarm() 호출 후 IDLE로 복귀
            break;

        case SENSOR_ERROR:
            // 에러 상태
            LOG_W("sensor_service_process: 에러 상태");
            // 향후: 자동 복구 로직 추가 가능
            break;

        default:
            break;
    }
}

/**
 * @brief 현재 상태 조회
 */
sensor_state_t sensor_service_get_state(void)
{
    return g_state;
}

/**
 * @brief 데이터 획득 (상태 복귀)
 */
int32_t sensor_service_get_data(float *p_temp, float *p_humid)
{
    if (g_state != SENSOR_DATA_READY && g_state != SENSOR_ALARM) {
        LOG_W("sensor_service_get_data: 상태 불일치 (현재: %d)", g_state);
        return -1;
    }

    *p_temp = g_data.temp;
    *p_humid = g_data.humid;

    LOG_I("sensor_service_get_data: temp=%.2f, humid=%.2f",
          *p_temp, *p_humid);

    // 데이터 읽음 후 상태 복귀
    g_state = SENSOR_IDLE;

    return 0;
}

/**
 * @brief 온도 임계값 설정
 */
void sensor_service_set_temp_threshold(float temp_high, float temp_low)
{
    g_temp_high = temp_high;
    g_temp_low = temp_low;
    LOG_I("sensor_service_set_temp_threshold: high=%.1f°C, low=%.1f°C",
          temp_high, temp_low);
}

/**
 * @brief 온도 알람 확인 및 클리어
 */
int32_t sensor_service_check_alarm(uint8_t *p_alarm_flag)
{
    *p_alarm_flag = g_alarm_flag;

    if (g_alarm_flag) {
        LOG_W("sensor_service_check_alarm: 알람 감지, 클리어됨");
        g_alarm_flag = 0;
        g_state = SENSOR_IDLE;
    }

    return 0;
}
