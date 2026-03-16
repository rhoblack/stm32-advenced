/**
 * 파일명 : ch02_01_swo_printf.c
 * 목  적 : SWO/ITM을 통한 printf 리다이렉트 최소 구현
 * MCU   : STM32F411RE (NUCLEO-F411RE)
 * 버  전 : v0.1
 *
 * 설명:
 *   SWO(Serial Wire Output) 핀을 통해 ITM(Instrumentation Trace Macrocell)
 *   채널 0으로 printf 출력을 리다이렉트한다.
 *   CubeIDE에서 SWV(Serial Wire Viewer) 기능을 활성화하면
 *   디버그 세션 중 ITM Console에서 출력을 확인할 수 있다.
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------------
 * ITM 채널 0 초기화
 *   CoreDebug : 디버그 코어 레지스터 베이스
 *   ITM->TER  : Trace Enable Register — 채널별 활성화 비트맵
 * --------------------------------------------------------------- */
void ITM_Init(void)
{
    /* CoreDebug DEMCR: TRCENA 비트(24번)를 세트해야 ITM이 동작한다 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* ITM 잠금 해제 (쓰기 접근 허용) */
    ITM->LAR = 0xC5ACCE55;

    /* 채널 0 활성화 (비트 0 = 채널 0) */
    ITM->TER |= (1UL << 0);

    /* ITM 트레이스 제어 활성화 */
    ITM->TCR |= ITM_TCR_ITMENA_Msk;
}

/* ---------------------------------------------------------------
 * ITM_SendChar — 문자 1개를 ITM 채널 0으로 전송
 *   newlib의 _write()가 이 함수를 호출한다.
 * --------------------------------------------------------------- */
int ITM_SendChar(int ch)
{
    /* FIFO가 비어 있을 때까지 대기 후 전송 */
    if ((ITM->TCR & ITM_TCR_ITMENA_Msk) &&
        (ITM->TER & (1UL << 0)))
    {
        while (ITM->PORT[0].u32 == 0) {}   /* 포트 FIFO 준비 대기 */
        ITM->PORT[0].u8 = (uint8_t)ch;     /* 1바이트 전송 */
    }
    return ch;
}

/* ---------------------------------------------------------------
 * _write — newlib stdio의 write 시스템 콜 오버라이드
 *   printf → fwrite → _write → ITM_SendChar 경로로 호출된다.
 * --------------------------------------------------------------- */
int _write(int file, char *ptr, int len)
{
    (void)file;   /* 파일 디스크립터 미사용 (경고 억제) */
    for (int i = 0; i < len; i++)
    {
        ITM_SendChar((int)ptr[i]);
    }
    return len;
}

/* ---------------------------------------------------------------
 * 사용 예시 (main.c에서 호출)
 * --------------------------------------------------------------- */
void ch02_01_demo(void)
{
    ITM_Init();

    /* stdout 버퍼링 해제 — 즉시 출력을 보장한다 */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("[CH02-01] SWO printf 테스트\r\n");
    printf("  ITM 채널 0 활성화 완료\r\n");

    uint32_t tick = HAL_GetTick();
    printf("  현재 HAL Tick: %lu ms\r\n", tick);
}
