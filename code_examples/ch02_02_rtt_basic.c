/**
 * 파일명 : ch02_02_rtt_basic.c
 * 목  적 : SEGGER RTT(Real-Time Transfer) 기초 출력 예제
 * MCU   : STM32F411RE (NUCLEO-F411RE)
 * 버  전 : v0.1
 *
 * 설명:
 *   SEGGER RTT는 J-Link 디버거가 MCU SRAM의 링 버퍼를 직접 읽어
 *   실시간 로그를 출력하는 방식이다. SWO와 달리 추가 핀이 필요 없고,
 *   MCU 동작 속도에 거의 영향을 주지 않는다.
 *
 * 사전 조건:
 *   - SEGGER RTT 소스(SEGGER_RTT.c, SEGGER_RTT.h)를 프로젝트에 추가
 *   - J-Link OB(온보드) 또는 외부 J-Link 사용
 *   - J-Link RTT Viewer 또는 Ozone에서 채널 0 확인
 */

#include "stm32f4xx_hal.h"

/* SEGGER RTT 헤더 — SEGGER RTT 소스 파일과 함께 포함해야 한다 */
#include "SEGGER_RTT.h"

/* ---------------------------------------------------------------
 * RTT 채널 정의
 *   채널 0 : 기본 텍스트 출력/입력 채널
 * --------------------------------------------------------------- */
#define RTT_CHANNEL     0

/* ---------------------------------------------------------------
 * ch02_02_rtt_demo — RTT 기초 출력 데모
 * --------------------------------------------------------------- */
void ch02_02_rtt_demo(void)
{
    /* RTT 제어 블록 초기화 — 최초 1회만 호출하면 된다 */
    SEGGER_RTT_Init();

    /* 채널 0에 컬러 터미널 코드와 함께 문자열 출력 */
    SEGGER_RTT_WriteString(RTT_CHANNEL, "=== CH02-02 RTT 기초 출력 ===\r\n");

    /* printf 형식 출력 (내부적으로 snprintf + WriteString 사용) */
    SEGGER_RTT_printf(RTT_CHANNEL, "  HAL Tick : %lu ms\r\n", HAL_GetTick());
    SEGGER_RTT_printf(RTT_CHANNEL, "  MCU      : STM32F411RE\r\n");
    SEGGER_RTT_printf(RTT_CHANNEL, "  채널     : %d\r\n", RTT_CHANNEL);

    /* 입력 수신 예시 — 논블로킹으로 1바이트 확인 */
    int key = SEGGER_RTT_GetKey();
    if (key >= 0)
    {
        /* RTT Viewer에서 키를 입력한 경우 에코 */
        SEGGER_RTT_printf(RTT_CHANNEL, "  입력된 키: %c (0x%02X)\r\n",
                          (char)key, (unsigned)key);
    }
    else
    {
        SEGGER_RTT_WriteString(RTT_CHANNEL, "  (입력 없음)\r\n");
    }
}

/* ---------------------------------------------------------------
 * main 루프에서 주기적으로 RTT 로그를 출력하는 예시
 * --------------------------------------------------------------- */
void ch02_02_rtt_loop_demo(void)
{
    SEGGER_RTT_Init();

    uint32_t count = 0;

    while (1)
    {
        /* 1초마다 카운터와 Tick 출력 */
        SEGGER_RTT_printf(RTT_CHANNEL, "[%4lu] Tick=%lu ms\r\n",
                          count++, HAL_GetTick());
        HAL_Delay(1000);
    }
}
