/* ch07_04_tim_driver.h — TIM Driver 공개 인터페이스
 *
 * 레이어: Driver
 * 위치: 프로젝트 아키텍처 v0.7
 *
 * 이 인터페이스는 Ch08(Advanced TIM)까지 변경 없이 유지됩니다.
 * Ch04 uart_driver.h와 동일한 설계 원칙을 따릅니다:
 *   - 상위 레이어(App)는 이 헤더만 include
 *   - HAL 헤더를 직접 include하지 않음
 *   - 반환 코드 enum으로 결과 전달
 */

#ifndef TIM_DRIVER_H
#define TIM_DRIVER_H

#include <stdint.h>

/* ===== 반환 코드 ===== */
typedef enum {
    TIM_OK       = 0,   /* 성공 */
    TIM_ERROR    = 1,   /* 일반 오류 */
    TIM_BUSY     = 2,   /* 동작 중 */
    TIM_PARAM    = 3    /* 파라미터 오류 */
} tim_status_t;

/* ===== 초기화 ===== */

/**
 * @brief TIM 드라이버 초기화
 * @note  CubeMX에서 TIM2 설정 후 호출
 *        내부적으로 Base 인터럽트 + PWM 채널 초기화
 */
void tim_driver_init(void);

/* ===== 주기 타이머 ===== */

/**
 * @brief 주기 타이머 시작 (인터럽트 모드)
 * @return TIM_OK 성공, TIM_ERROR 실패
 */
tim_status_t tim_start(void);

/**
 * @brief 주기 타이머 정지
 * @return TIM_OK 성공, TIM_ERROR 실패
 */
tim_status_t tim_stop(void);

/**
 * @brief 런타임 주기 변경 (밀리초 단위)
 * @param period_ms  원하는 주기 (1~60000 ms)
 * @return TIM_OK 성공, TIM_PARAM 범위 초과
 *
 * @note 내부적으로 PSC/ARR을 재계산합니다.
 *       타이머가 동작 중이어도 안전하게 변경됩니다.
 */
tim_status_t tim_set_period_ms(uint32_t period_ms);

/* ===== PWM ===== */

/**
 * @brief PWM 출력 시작 (CH1)
 * @return TIM_OK 성공, TIM_ERROR 실패
 */
tim_status_t pwm_start(void);

/**
 * @brief PWM 출력 정지 (CH1)
 * @return TIM_OK 성공, TIM_ERROR 실패
 */
tim_status_t pwm_stop(void);

/**
 * @brief PWM 듀티 변경
 * @param percent  듀티 비율 (0~100%)
 * @return TIM_OK 성공, TIM_PARAM 범위 초과
 */
tim_status_t pwm_set_duty(uint8_t percent);

/* ===== 콜백 등록 ===== */

/**
 * @brief UEV 콜백 함수 타입
 * @note  ISR 컨텍스트에서 호출됩니다.
 *        짧은 처리만 수행하세요.
 */
typedef void (*tim_callback_t)(void);

/**
 * @brief UEV 콜백 등록
 * @param cb  주기마다 호출될 함수 (NULL이면 해제)
 */
void tim_register_callback(tim_callback_t cb);

#endif /* TIM_DRIVER_H */
