/**
 * @file    ch18_hardfault_handler.c
 * @brief   HardFault 핸들러 — 스택 덤프 자동 출력 + CFSR 디코딩
 * @author  STM32 고급 실무교육 Ch18
 *
 * @note    startup_stm32f411retx.s의 기본 HardFault_Handler를 이 파일로 대체합니다.
 *          빌드 전 startup 파일에서 HardFault_Handler weak 심볼이
 *          이 파일의 함수로 override 되는지 확인하세요.
 */

#include "main.h"
#include "debug_logger.h"   /* LOG_E, LOG_W 매크로 — Ch02 표준 */

/* ======================================================================
 * 레지스터 주소 정의 (Cortex-M4 System Control Block)
 * ====================================================================== */
#define SCB_HFSR    (*((volatile uint32_t *)0xE000ED2C))  /* HardFault Status */
#define SCB_CFSR    (*((volatile uint32_t *)0xE000ED28))  /* Configurable Fault Status */
#define SCB_MMFAR   (*((volatile uint32_t *)0xE000ED34))  /* MemManage Fault Address */
#define SCB_BFAR    (*((volatile uint32_t *)0xE000ED38))  /* BusFault Address */

/* CFSR 비트 마스크 */
#define CFSR_MMFSR_MASK     0x000000FF  /* MemManage Fault */
#define CFSR_BFSR_MASK      0x0000FF00  /* BusFault */
#define CFSR_UFSR_MASK      0xFFFF0000  /* UsageFault */

/* BusFault Status Register 비트 */
#define BFSR_PRECISERR      (1U << 9)   /* 정확한 버스 오류 (PC 신뢰 가능) */
#define BFSR_IMPRECISERR    (1U << 10)  /* 비정확 버스 오류 (PC 신뢰 불가) */
#define BFSR_IBUSERR        (1U << 8)   /* 명령어 버스 오류 */
#define BFSR_BFARVALID      (1U << 15)  /* BFAR 주소 유효 */

/* UsageFault Status Register 비트 */
#define UFSR_UNDEFINSTR     (1U << 16)  /* 미정의 명령어 */
#define UFSR_INVSTATE       (1U << 17)  /* 잘못된 실행 상태 */
#define UFSR_DIVBYZERO      (1U << 25)  /* 0으로 나누기 */
#define UFSR_UNALIGNED      (1U << 24)  /* 비정렬 접근 */

/* MemManage Fault Status Register 비트 */
#define MMFSR_DACCVIOL      (1U << 1)   /* 데이터 접근 위반 */
#define MMFSR_IACCVIOL      (1U << 0)   /* 명령어 접근 위반 */
#define MMFSR_MMARVALID     (1U << 7)   /* MMFAR 주소 유효 */

/* EXC_RETURN 패턴 (LR 레지스터 값으로 스택 종류 판별) */
#define EXC_RETURN_MSP_FPU_OFF   0xFFFFFFF9  /* MSP, FPU 미사용 */
#define EXC_RETURN_PSP_FPU_OFF   0xFFFFFFFD  /* PSP, FPU 미사용 */
#define EXC_RETURN_MSP_FPU_ON    0xFFFFFFE9  /* MSP, FPU 사용 */
#define EXC_RETURN_PSP_FPU_ON    0xFFFFFFED  /* PSP, FPU 사용 */

/* ======================================================================
 * 내부 함수 선언
 * ====================================================================== */
static void debug_dump_stack(uint32_t *frame);
static void debug_decode_cfsr(uint32_t cfsr);

/* ======================================================================
 * ASM 스텁 — PSP/MSP 분기 후 C 함수로 전달
 * startup 파일의 weak HardFault_Handler를 override 합니다.
 *
 * 동작 원리:
 *   예외 발생 → CPU가 스택에 R0~xPSR 자동 저장
 *   → LR(EXC_RETURN) 비트4 확인 → PSP(1) or MSP(0) 선택
 *   → 스택 포인터를 R0에 담아 hard_fault_handler_c() 호출
 * ====================================================================== */
__attribute__((naked))
void HardFault_Handler(void)
{
    __asm volatile (
        "TST    LR, #4          \n" /* LR 비트4: 0=MSP, 1=PSP */
        "ITE    EQ              \n"
        "MRSEQ  R0, MSP         \n" /* MSP 선택 */
        "MRSNE  R0, PSP         \n" /* PSP 선택 */
        "B      hard_fault_handler_c \n" /* R0 = 스택 포인터 전달 */
    );
}

/* ======================================================================
 * C 언어 HardFault 핸들러 본체
 * @param stack_frame  CPU가 저장한 스택 프레임의 포인터
 *                     [0]=R0, [1]=R1, [2]=R2, [3]=R3,
 *                     [4]=R12, [5]=LR, [6]=PC, [7]=xPSR
 * ====================================================================== */
void hard_fault_handler_c(uint32_t *stack_frame)
{
    uint32_t cfsr = SCB_CFSR;
    uint32_t hfsr = SCB_HFSR;

    /* 긴급 로그 출력 — SEGGER RTT 또는 UART 폴링 방식 사용 */
    LOG_E("=== HARD FAULT DETECTED ===");

    /* 스택 덤프 출력 */
    debug_dump_stack(stack_frame);

    /* CFSR 디코딩 출력 */
    LOG_E("HFSR = 0x%08lX", hfsr);
    LOG_E("CFSR = 0x%08lX", cfsr);
    debug_decode_cfsr(cfsr);

    /* BFAR / MMFAR 유효 시 출력 */
    if (cfsr & BFSR_BFARVALID) {
        LOG_E("BFAR (버스 오류 주소) = 0x%08lX", SCB_BFAR);
    }
    if (cfsr & MMFSR_MMARVALID) {
        LOG_E("MMFAR (메모리 접근 위반 주소) = 0x%08lX", SCB_MMFAR);
    }

    LOG_E("=== 디버거 연결 또는 리셋 필요 ===");

    /* 무한 루프 — 디버거 중단점 설정 권장 */
    while (1) {
        /* 디버거: 이 위치에서 Call Stack 확인 후 stack_frame[6](PC) 역추적 */
        __NOP();
    }
}

/* ======================================================================
 * 스택 프레임 덤프 출력
 * ====================================================================== */
static void debug_dump_stack(uint32_t *frame)
{
    LOG_E("--- Stack Frame Dump ---");
    LOG_E("R0  = 0x%08lX", frame[0]);
    LOG_E("R1  = 0x%08lX", frame[1]);
    LOG_E("R2  = 0x%08lX", frame[2]);
    LOG_E("R3  = 0x%08lX", frame[3]);
    LOG_E("R12 = 0x%08lX", frame[4]);
    LOG_E("LR  = 0x%08lX  (EXC_RETURN 또는 복귀 주소)", frame[5]);
    LOG_E("PC  = 0x%08lX  <- 이 주소에서 예외 발생!", frame[6]);
    LOG_E("xPSR= 0x%08lX", frame[7]);
    LOG_E("------------------------");
}

/* ======================================================================
 * CFSR(Configurable Fault Status Register) 비트 디코딩
 * 가장 흔한 오류 비트만 검사 (나머지는 RM0383 참조)
 * ====================================================================== */
static void debug_decode_cfsr(uint32_t cfsr)
{
    LOG_E("--- CFSR 분석 ---");

    /* --- BusFault 분석 --- */
    if (cfsr & CFSR_BFSR_MASK) {
        LOG_E("[BusFault]");
        if (cfsr & BFSR_IBUSERR)     LOG_E("  IBUSERR: 명령어 버스 오류");
        if (cfsr & BFSR_PRECISERR)   LOG_E("  PRECISERR: 정확한 데이터 버스 오류 (PC 신뢰 가능)");
        if (cfsr & BFSR_IMPRECISERR) LOG_W("  IMPRECISERR: 비정확 데이터 버스 오류 (PC 오프셋 주의)");
    }

    /* --- UsageFault 분석 --- */
    if (cfsr & CFSR_UFSR_MASK) {
        LOG_E("[UsageFault]");
        if (cfsr & UFSR_UNDEFINSTR)  LOG_E("  UNDEFINSTR: 미정의 명령어 실행 (함수 포인터 오염 의심)");
        if (cfsr & UFSR_INVSTATE)    LOG_E("  INVSTATE: 잘못된 실행 상태 (Thumb 비트 오류)");
        if (cfsr & UFSR_DIVBYZERO)   LOG_E("  DIVBYZERO: 0으로 나누기");
        if (cfsr & UFSR_UNALIGNED)   LOG_E("  UNALIGNED: 비정렬 메모리 접근");
    }

    /* --- MemManage Fault 분석 --- */
    if (cfsr & CFSR_MMFSR_MASK) {
        LOG_E("[MemManage Fault]");
        if (cfsr & MMFSR_IACCVIOL)   LOG_E("  IACCVIOL: 명령어 접근 위반 (MPU 또는 실행 불가 영역)");
        if (cfsr & MMFSR_DACCVIOL)   LOG_E("  DACCVIOL: 데이터 접근 위반 (널포인터 또는 MPU 위반)");
    }

    LOG_E("-----------------");
}
