/**
 * @file    ch01_ex02_pushpull_vs_opendrain.c
 * @brief   Push-Pull과 Open-Drain 출력 모드 비교 실험
 *
 * @board   NUCLEO-F411RE (STM32F411RE)
 * @핀연결  LD2 (녹색 LED) = PA5, 액티브 하이
 *
 * @실험목적
 *   GPIO 출력에는 두 가지 모드가 있다.
 *   이 코드는 런타임에 PA5를 Push-Pull → Open-Drain으로 전환하면서
 *   각 모드의 전기적 특성 차이를 직접 확인한다.
 *
 * @Push-Pull_vs_Open-Drain
 *   ┌─────────────────┬──────────────────────────────────────────┐
 *   │ 모드             │ 특성                                     │
 *   ├─────────────────┼──────────────────────────────────────────┤
 *   │ Push-Pull (PP)  │ HIGH: P-MOS ON → VDD에 연결 (강한 HIGH) │
 *   │                 │ LOW:  N-MOS ON → GND에 연결 (강한 LOW)  │
 *   ├─────────────────┼──────────────────────────────────────────┤
 *   │ Open-Drain (OD) │ HIGH: 두 MOS 모두 OFF → 핀이 플로팅    │
 *   │                 │       외부 풀업 저항 없으면 전압 불확정  │
 *   │                 │ LOW:  N-MOS ON → GND에 연결 (강한 LOW) │
 *   └─────────────────┴──────────────────────────────────────────┘
 *
 * @CubeMX_설정
 *   - PA5: 초기값 GPIO_OUTPUT Push-Pull, No Pull (런타임에 변경함)
 *
 * TODO: Ch02에서 LOG 시스템 추가
 */

#include "main.h"

/**
 * @brief  GPIO 모드를 런타임에 재설정하는 헬퍼 함수
 *
 * @note   HAL_GPIO_Init()은 이미 초기화된 핀도 재설정 가능하다.
 *         단, 재설정 직전에 핀 상태가 의도치 않은 값이 될 수 있으므로
 *         출력 데이터 레지스터(ODR)를 명시적으로 설정하는 것이 안전하다.
 *
 * @param  mode  GPIO_MODE_OUTPUT_PP 또는 GPIO_MODE_OUTPUT_OD
 */
static void gpio_pa5_reinit(uint32_t mode)
{
    GPIO_InitTypeDef gpio_init = {0};

    /*
     * GPIO_InitTypeDef 구조체 필드 설명:
     *
     *   .Pin   — 설정할 핀 번호 (비트마스크)
     *             GPIO_PIN_5 = 0x0020 = 6번째 비트
     *             여러 핀을 동시에 설정할 때는 OR 연산: GPIO_PIN_5 | GPIO_PIN_6
     */
    gpio_init.Pin = GPIO_PIN_5;

    /*
     *   .Mode  — GPIO 동작 모드
     *             GPIO_MODE_OUTPUT_PP  : Push-Pull 출력 (일반적인 출력)
     *             GPIO_MODE_OUTPUT_OD  : Open-Drain 출력 (I2C, 와이어드-AND 등)
     *             GPIO_MODE_INPUT      : 입력
     *             GPIO_MODE_AF_PP      : 대체 기능 Push-Pull (UART, SPI 등)
     *             GPIO_MODE_ANALOG     : 아날로그 (ADC/DAC)
     */
    gpio_init.Mode = mode;

    /*
     *   .Pull  — 내부 풀업/풀다운 저항 설정
     *             GPIO_NOPULL   : 풀업/풀다운 없음
     *             GPIO_PULLUP   : 내부 풀업 활성화 (~40kΩ)
     *             GPIO_PULLDOWN : 내부 풀다운 활성화 (~40kΩ)
     *
     *   Open-Drain + PULLUP 조합으로 외부 저항 없이 사용 가능하다.
     *   그러나 내부 풀업(40kΩ)은 I2C 스펙보다 높아 고속 I2C에는 부적합하다.
     */
    gpio_init.Pull = GPIO_NOPULL;

    /*
     *   .Speed — 출력 슬루레이트(slew rate) 설정
     *             GPIO_SPEED_FREQ_LOW       : 저속 (~2MHz)
     *             GPIO_SPEED_FREQ_MEDIUM    : 중속 (~25MHz)
     *             GPIO_SPEED_FREQ_HIGH      : 고속 (~50MHz)
     *             GPIO_SPEED_FREQ_VERY_HIGH : 초고속 (~100MHz)
     *
     *   속도가 높을수록 EMI(전자기 간섭)가 증가한다.
     *   LED 제어처럼 저주파 신호에는 LOW로 충분하다.
     */
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

    /* HAL_GPIO_Init: 위 구조체 설정을 하드웨어 레지스터에 적용 */
    HAL_GPIO_Init(GPIOA, &gpio_init);
}

/**
 * @brief  메인 함수
 */
int main(void)
{
    /* --- 시스템 초기화 (CubeMX 자동 생성 코드) --- */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();     /* PA5: 초기 Push-Pull 출력으로 설정됨 */

    /* --- 메인 루프 --- */
    while (1)
    {
        /* =====================================================
         * 실험 1: Push-Pull 모드
         * ===================================================== */

        /* Push-Pull 모드로 재설정 */
        gpio_pa5_reinit(GPIO_MODE_OUTPUT_PP);

        /* HIGH 출력: P-MOS ON → VDD(3.3V)에 직접 연결 → LED ON */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(1000);    /* 1초 대기 — LED 켜진 상태 관찰 */

        /* LOW 출력: N-MOS ON → GND에 직접 연결 → LED OFF */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        HAL_Delay(1000);    /* 1초 대기 — LED 꺼진 상태 관찰 */

        /* =====================================================
         * 실험 2: Open-Drain 모드
         * ===================================================== */

        /* Open-Drain 모드로 재설정 */
        gpio_pa5_reinit(GPIO_MODE_OUTPUT_OD);

        /*
         * HIGH 출력 시도: 두 MOS 모두 OFF → 핀이 HIGH-임피던스 상태
         *
         * ⚠️ 경고: 외부 풀업 저항이 없으면 핀 전압이 불확정(floating)이다!
         *          이 상태에서 LED는 켜지지 않거나 희미하게 켜질 수 있다.
         *          실제 I2C 회로에서는 보통 4.7kΩ 풀업을 VDD에 연결한다.
         *
         * NUCLEO-F411RE의 PA5는 보드 내부에서 LED와 저항으로 연결되어 있어
         * 미세한 전류가 흐를 수 있으나, 정상적인 HIGH 구동은 불가하다.
         */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(1000);    /* 1초 대기 — LED 꺼짐(또는 희미) 상태 관찰 */

        /* LOW 출력: N-MOS ON → GND에 연결 → LED ON (전류 방향: VDD→LED→핀→GND) */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        HAL_Delay(1000);    /* 1초 대기 — LED 켜진 상태 관찰 */

        /* TODO: Ch02에서 LOG 시스템 추가 */
    }
}

/* =========================================================
 * 실험 결과 요약:
 *
 *   Push-Pull HIGH  → LED 확실하게 켜짐
 *   Push-Pull LOW   → LED 꺼짐
 *   Open-Drain HIGH → LED 꺼짐 또는 희미 (풀업 없음)
 *   Open-Drain LOW  → LED 켜짐
 *
 * Open-Drain 활용 사례:
 *   - I2C (SDA, SCL): 여러 디바이스가 같은 버스를 공유하는 와이어드-AND
 *   - 레벨 시프트: 3.3V MCU와 5V 디바이스 인터페이스
 *   - 오픈 컬렉터 출력과의 인터페이스
 * ========================================================= */
