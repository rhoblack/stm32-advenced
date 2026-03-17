/**
 * 파일명 : ch03_02_layer_interfaces.h
 * 목  적 : 4계층 아키텍처 인터페이스 설계 예시
 * MCU   : STM32F411RE (NUCLEO-F411RE)
 * 버  전 : v0.3 (아키텍처 설계 단계)
 *
 * 설명:
 *   각 레이어가 상위 레이어에 노출하는 API(인터페이스)를 정의한다.
 *   이 파일은 Ch03에서 "주문서" 역할을 이해하기 위한 설계 문서이다.
 *   실제 구현은 Ch04 이후에서 점진적으로 진행한다.
 *
 *   레스토랑 비유:
 *   - 이 헤더 파일 = "메뉴판" (상위 레이어가 주문할 수 있는 목록)
 *   - 함수 시그니처 = "메뉴 항목" (이름, 재료, 가격이 명시됨)
 *   - 구현 파일(.c) = "레시피" (주방 내부 비밀, 메뉴판에 안 나옴)
 */

#ifndef CH03_LAYER_INTERFACES_H
#define CH03_LAYER_INTERFACES_H

#include <stdint.h>

/* ================================================================
 * [1] HAL 레이어 — ST 제공, 우리가 수정하지 않음
 *     레스토랑 비유: 주방 기기 (가스레인지, 오븐, 냉장고)
 *     기기를 교체(가스→인덕션)해도 요리법은 동일
 * ================================================================
 *
 * HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
 * HAL_UART_Transmit(&huart2, buf, len, timeout);
 * HAL_I2C_Master_Transmit(&hi2c1, addr, data, size, timeout);
 *
 * ⇒ 이 함수들은 stm32f4xx_hal_*.h에 이미 정의되어 있다.
 *   우리는 이 위에 Driver를 쌓아올린다.
 */


/* ================================================================
 * [2] Driver 레이어 — 하드웨어 주변장치를 캡슐화
 *     레스토랑 비유: 요리사 (단위 조리 수행)
 *     "볶기/끓이기"만 할 줄 알면 됨. 손님(App)과 직접 대화 안 함
 * ================================================================ */

/* --- LED Driver (v0.3 설계, 현재는 main에서 HAL 직접 호출 중) --- */
typedef enum {
    LED_OFF = 0,
    LED_ON  = 1
} led_state_t;

/**
 * @brief  LED GPIO 초기화
 * @note   내부에서 HAL_GPIO_Init()을 호출한다
 */
void led_driver_init(void);

/**
 * @brief  LED 상태 설정
 * @param  state  LED_ON 또는 LED_OFF
 * @note   App 레이어는 GPIOA, PIN_5 같은 하드웨어 정보를 몰라도 됨
 */
void led_driver_set(led_state_t state);

/**
 * @brief  LED 토글
 */
void led_driver_toggle(void);

/* --- Button Driver (v0.3 설계) --- */
typedef void (*button_callback_t)(void);

/**
 * @brief  버튼 EXTI 초기화 + 디바운싱 설정
 * @param  on_press  유효한 버튼 누름 시 호출될 콜백
 * @note   디바운싱은 Driver 내부에서 처리 (App은 모름)
 */
void button_driver_init(button_callback_t on_press);

/**
 * @brief  누적 버튼 누름 횟수 반환
 * @return 유효한 누름 횟수 (디바운싱 통과한 것만)
 */
uint32_t button_driver_get_press_count(void);

/* --- Logger (v0.2에서 이미 구현됨 — 횡단 관심사) --- */
/*
 * LOG_E(fmt, ...)   에러: 즉시 조치 필요
 * LOG_W(fmt, ...)   경고: 주의 필요
 * LOG_I(fmt, ...)   정보: 주요 이벤트
 * LOG_D(fmt, ...)   디버그: 상세 추적
 *
 * 횡단 관심사(Cross-cutting Concern):
 *   로그는 모든 레이어에서 호출되지만,
 *   구현체(log_print)는 Driver 레이어에 위치한다.
 *   이것이 "횡단 관심사"이다.
 *   레스토랑 비유: 화재 경보기 — 주방/홀/사무실 어디서든 울림
 */


/* ================================================================
 * [3] Service 레이어 — 비즈니스 로직 (Ch10부터 등장)
 *     레스토랑 비유: 메뉴 관리자 (코스 편성)
 *     여러 요리사(Driver)를 조합하여 의미 있는 기능 완성
 * ================================================================ */

/* --- Clock Service (Ch10 예정) --- */
/*
 * void clock_service_init(void);
 * void clock_service_update(void);  // RTC 시각 → 모터 위치 변환
 *   내부: rtc_driver_get_time() + stepper_driver_set_position()
 */

/* --- Sensor Service (Ch11 예정) --- */
/*
 * void sensor_service_init(void);
 * void sensor_service_read(float *temp, float *humi);
 *   내부: sht31_driver_read() + CRC 검증 + 알람 임계값 판단
 */


/* ================================================================
 * [4] App 레이어 — 최상위 애플리케이션
 *     레스토랑 비유: 홀 매니저 (고객 응대, 최종 결정)
 *     Service에게 "주문서"를 전달할 뿐, 주방 내부를 모름
 * ================================================================ */

/*
 * main() {
 *     // 초기화 — 하위 계층부터 상위 순서
 *     led_driver_init();           // Driver
 *     button_driver_init(&on_btn); // Driver
 *     // clock_service_init();     // Service (Ch10)
 *
 *     while (1) {
 *         // Service API만 호출
 *         // clock_service_update();
 *         // sensor_service_read(&t, &h);
 *     }
 * }
 *
 * static void on_btn(void) {
 *     led_driver_toggle();  // Driver API (HAL 직접 호출 X!)
 *     LOG_I("버튼 → LED 토글");
 * }
 */

#endif /* CH03_LAYER_INTERFACES_H */
