/**
 * @file    ch01_ex04_button_exti.c
 * @brief   EXTI 인터럽트 기반 버튼 — 콜백 패턴
 *
 * @board   NUCLEO-F411RE (STM32F411RE)
 * @핀연결
 *   - B1 (파란 버튼) = PC13, 액티브 로우 (누름 = GPIO_PIN_RESET)
 *   - LD2 (녹색 LED) = PA5, 액티브 하이
 *
 * @CubeMX_설정 (필수)
 *   1. PC13: GPIO_EXTI13, Falling Edge (하강 엣지에서 인터럽트 발생)
 *      - Falling Edge: 신호가 HIGH→LOW로 떨어지는 순간 = 버튼 누르는 순간
 *      - Pull-up 설정 (버튼 뗀 상태 = HIGH 유지)
 *   2. NVIC: EXTI15_10 IRQ → Enabled, Priority 설정
 *      - PC13은 EXTI 라인 13 → EXTI15_10 그룹에 속함
 *      - Preemption Priority: 0~15 (낮을수록 높은 우선순위)
 *
 * @인터럽트_동작_흐름
 *   버튼 누름 (PC13 HIGH→LOW)
 *       │
 *       ▼
 *   EXTI 라인 13 인터럽트 발생
 *       │
 *       ▼
 *   stm32f4xx_it.c: EXTI15_10_IRQHandler()  ← CubeMX가 자동 생성
 *       │  내부에서 HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13) 호출
 *       │  → 인터럽트 플래그 클리어
 *       │  → HAL_GPIO_EXTI_Callback(GPIO_PIN_13) 호출
 *       ▼
 *   HAL_GPIO_EXTI_Callback()  ← 사용자가 구현하는 함수 (이 파일)
 *       │  LED 토글
 *       ▼
 *   메인 루프로 복귀 (인터럽트 처리 완료)
 *
 * @__weak_설명
 *   HAL 라이브러리는 HAL_GPIO_EXTI_Callback()을 __weak 속성으로 빈 함수로 정의한다.
 *   __weak 함수는 "같은 이름의 강한(non-weak) 함수가 링커에 존재하면 그것을 사용"하는 규칙이다.
 *   이 파일에서 HAL_GPIO_EXTI_Callback()을 정의하면 __weak 버전이 자동으로 대체된다.
 *   별도의 헤더 선언이나 함수 포인터 등록이 필요 없다.
 *
 * TODO: Ch02에서 LOG 시스템 추가
 */

#include "main.h"

/* =========================================================
 * stm32f4xx_it.c 에 자동 생성되는 ISR (참고용 — 직접 구현 불필요)
 *
 * CubeMX는 활성화된 핀만 포함하는 코드를 생성한다.
 * PC13(GPIO_PIN_13) 하나만 사용할 경우 아래와 같이 생성된다:
 *
 * void EXTI15_10_IRQHandler(void)
 * {
 *     HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
 * }
 *
 * ⚠️ 주의: EXTI10~15를 모두 나열하는 것은 비표준 패턴이다.
 *    CubeMX 자동 생성 코드는 프로젝트에서 실제로 사용하는 핀만 포함한다.
 * ========================================================= */

/**
 * @brief  EXTI 인터럽트 콜백 함수 (HAL __weak 함수 오버라이드)
 *
 * @param  GPIO_Pin  인터럽트가 발생한 핀의 비트마스크
 *
 * @note   ⚠️ 중요: GPIO_Pin은 핀 번호(리터럴)가 아니라 비트마스크다!
 *           GPIO_PIN_13 = (uint16_t)0x2000 = 0b0010000000000000
 *           즉, 13번째 비트(0부터 셈)가 세팅된 16비트 값이다.
 *           따라서 (GPIO_Pin == 13)은 절대 참이 될 수 없다. → 버그!
 *           반드시 (GPIO_Pin == GPIO_PIN_13)으로 비교해야 한다.
 *
 * @note   이 함수는 인터럽트 컨텍스트에서 실행된다.
 *           - 실행 시간을 최소화해야 한다 (HAL_Delay 같은 블로킹 함수 금지)
 *           - 전역 변수 접근 시 volatile 키워드가 필요하다
 *           - printf, malloc 등 재진입 불가 함수는 사용하지 않는다
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /*
     * GPIO_Pin은 비트마스크이므로 == 비교로 특정 핀을 식별한다.
     *
     * 여러 EXTI 핀을 사용하는 경우 각 핀을 개별적으로 체크한다:
     *   if (GPIO_Pin == GPIO_PIN_13) { ... }
     *   if (GPIO_Pin == GPIO_PIN_0)  { ... }
     *
     * ⚠️ 주의: GPIO_PIN_13은 비트마스크 0x2000이다.
     *           리터럴 숫자 13과 혼동하지 말 것!
     *           (GPIO_Pin == 13) → 항상 거짓 → LED가 절대 토글되지 않는다.
     */
    if (GPIO_Pin == GPIO_PIN_13)
    {
        /*
         * 버튼 B1(PC13) 하강 엣지 인터럽트 처리
         * LED 토글: 인터럽트가 발생할 때마다 LED 상태가 바뀐다.
         *
         * 디바운싱 미적용 상태:
         *   채터링이 있으면 이 콜백이 여러 번 호출될 수 있다.
         *   → ch01_ex05에서 타임스탬프 디바운싱으로 해결
         */
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);  /* LD2 토글 */

        /* TODO: Ch02에서 LOG 시스템 추가 */
    }
}

/**
 * @brief  메인 함수
 */
int main(void)
{
    /* --- 시스템 초기화 (CubeMX 자동 생성 코드) --- */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();     /* PC13: EXTI Falling, PA5: 출력으로 설정됨 */

    /*
     * 인터럽트 방식의 장점:
     *   while(1) 루프가 비어 있어도 버튼 이벤트를 정확히 처리한다.
     *   CPU는 루프를 돌면서 다른 작업(센서 읽기, 통신 처리 등)을 수행할 수 있다.
     *   폴링 방식에서는 CPU가 항상 ReadPin을 반복 호출해야 했다.
     */

    /* --- 메인 루프 --- */
    while (1)
    {
        /*
         * 메인 루프는 의도적으로 비어 있다.
         * 버튼 처리는 HAL_GPIO_EXTI_Callback()에서 인터럽트로 처리된다.
         *
         * 실제 애플리케이션에서는 여기에 다른 작업을 추가할 수 있다:
         *   - 센서 데이터 읽기
         *   - 디스플레이 업데이트
         *   - 통신 프로토콜 처리
         *   - 저전력 모드 진입 (HAL_PWR_EnterSLEEPMode 등)
         *
         * CPU는 인터럽트가 발생할 때만 콜백으로 점프하고
         * 처리가 끝나면 즉시 이 루프로 돌아온다.
         */

        /* TODO: Ch02에서 다른 작업 추가 예정 */
    }
}

/* =========================================================
 * 동작 확인 방법:
 *   1. 빌드 후 플래시
 *   2. B1 버튼을 누를 때마다 LD2 LED가 토글되는지 확인
 *   3. 디버거에서 HAL_GPIO_EXTI_Callback에 브레이크포인트 설정 후
 *      버튼 누르면 실행이 멈추는지 확인 (인터럽트 동작 검증)
 *
 * EXTI 설정 검증:
 *   - STM32CubeIDE → Debug → Peripherals → EXTI → Line13 상태 확인
 *   - NVIC → EXTI15_10 Enabled 여부 확인
 * ========================================================= */
