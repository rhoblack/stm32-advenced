# Ch02 기술 리뷰 — 기술 리뷰어

> 리뷰 날짜: 2026-03-16
> 대상 파일: `manuscripts/part0/chapter02.html`
> 리뷰어 역할: 기술 리뷰어 (STM32 HAL 정확성 / 실무 적합성)

---

## 종합 평가

Ch02는 SWO/ITM, SEGGER RTT, 로그 레벨 설계, `##__VA_ARGS__` 매크로, 컴파일 타임 필터링까지 임베디드 디버그 인프라의 핵심 개념을 체계적으로 다루고 있다. 전반적인 기술 내용은 NUCLEO-F411RE 실무 기준에서 정확하며, 완성 코드(`logger.h`, `logger.c`)는 실제 컴파일·동작이 가능한 수준이다.

다만 몇 가지 실무적으로 중요한 오류와 불완전한 설명이 발견되었다. Critical 이슈는 없으나 Major 이슈 3건, Minor 이슈 6건이 식별되었다.

---

## Critical 이슈

없음

---

## Major 이슈

### [Major-1] `log_print()` 버퍼 오버플로 위험 — `offset` 음수 미처리

**위치**: §2.6 `logger.c`, `log_print()` 함수 (약 817~841행)

**문제**:
```c
offset += snprintf(buf + offset, sizeof(buf) - offset, ...);
```
`snprintf`는 버퍼가 꽉 찼을 때 잘라낸 문자 수(즉, 실제 필요했던 길이)를 반환한다. `offset`이 이미 `sizeof(buf)`에 근접했을 때 두 번째 `vsnprintf` 호출에서 `sizeof(buf) - offset`이 0 이하가 될 수 있다. 이 경우 `size_t` 타입 언더플로가 발생하거나(`offset`이 `sizeof(buf)`를 초과했을 때) 개행 추가 조건 `if (offset < (int)(sizeof(buf) - 2))` 검사에서도 `int` 캐스트 비교 오류 위험이 있다.

구체적으로 `offset`이 `snprintf` 반환값 누적으로 254를 초과한 상태에서 개행 추가 분기가 실행되면 `buf[offset]`이 배열 범위를 벗어날 수 있다.

**권장 수정**: `offset`이 `(int)sizeof(buf) - 1`을 초과하지 않도록 클램핑(clamping) 처리 추가. 또는 `offset = offset < (int)sizeof(buf) ? offset : (int)sizeof(buf) - 1;` 패턴 사용.

---

### [Major-2] `ITM_SendChar()` 바쁜 대기(busy-wait) 설명 일부 부정확

**위치**: §2.1 aside.tip "ISR 내부에서 ITM_SendChar() 호출 주의" (189~194행)

**문제**:
본문 팁에서 "ITM_SendChar()는 ITM FIFO가 빌 때까지 바쁜 대기(busy-wait)를 합니다"라고 서술하였다. 그러나 STM32 CMSIS 헤더(`core_cm4.h`)의 `ITM_SendChar()` 구현을 보면:

```c
__STATIC_INLINE uint32_t ITM_SendChar(uint32_t ch)
{
  if (((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL) &&
      ((ITM->TER & 1UL               ) != 0UL)   )
  {
    while (ITM->PORT[0U].u32 == 0UL) { __NOP(); }
    ITM->PORT[0U].u8 = (uint8_t)ch;
  }
  return (ch);
}
```

이 함수는 ITM이 비활성화되어 있으면 즉시 반환한다. SWO 케이블이 없거나 디버거가 없어 ITM이 초기화되지 않은 상태(TCR/TER 비트가 0)에서는 busy-wait 없이 그냥 통과한다. 이는 배포 펌웨어에서 `_write()`를 그대로 두어도 런타임 행(hang)이 발생하지 않는다는 중요한 실무적 사실이다.

반면 디버거가 연결되어 ITM이 활성화된 상태에서 FIFO가 찬 경우에는 실제로 busy-wait가 발생한다. 즉 "항상 busy-wait"가 아니라 "ITM 활성화 + FIFO 가득 찬 경우에만 busy-wait"가 정확한 설명이다.

**권장 수정**: aside 문구를 "ITM이 활성화된 상태에서 FIFO가 찬 경우 busy-wait가 발생합니다. 디버거가 연결되지 않은 배포 환경에서는 ITM이 비활성화되어 있어 즉시 반환됩니다"로 수정.

---

### [Major-3] `SEGGER_RTT_printf()` 인자 타입 불일치

**위치**: §2.2 RTT 기초 사용 예제 (292~293행)

**문제**:
```c
SEGGER_RTT_printf(0, "[%6u] 루프 카운트: %lu\r\n",
                  (unsigned)HAL_GetTick(), count++);
```

`count`는 `uint32_t`로 선언되어 있다. `SEGGER_RTT_printf()`는 표준 `printf`가 아니라 SEGGER 자체 경량 포맷 구현이다. 이 구현에서 `%lu`는 `unsigned long`을 기대하는데, ARM Cortex-M 환경에서 `uint32_t`는 `unsigned int`(32비트)이고 `unsigned long`도 32비트이지만, 엄밀한 MISRA-C 또는 IAR 환경에서는 경고가 발생할 수 있다.

더 중요한 문제는 `(unsigned)HAL_GetTick()`과 `%6u` 조합이다. `HAL_GetTick()`은 `uint32_t`를 반환하며, `unsigned`로 캐스트하면 동일하지만 명시적으로 `(unsigned int)` 또는 `%lu`를 사용하는 것이 더 명확하다. 두 포맷 지정자가 혼용(`%u`와 `%lu`)되어 타입 일관성이 없다.

**권장 수정**: `(unsigned int)HAL_GetTick()`으로 명시하거나, 두 인자 모두 `%lu`로 통일. 또는 `count`를 `unsigned int`로 선언.

---

## Minor 이슈

### [Minor-1] Makefile 빌드 인자 문법 오류

**위치**: §2.5 make 명령줄 예시 (647~649행)

**문제**:
```
/* make -DLOG_LEVEL=3 all      → INFO 레벨 빌드 */
/* make -DLOG_LEVEL=0 release  → 로그 없는 배포 빌드 */
```
`make`에서 전처리기 심볼을 전달하는 올바른 문법은 `make CFLAGS+=-DLOG_LEVEL=3 all` 또는 `make all EXTRA_CFLAGS="-DLOG_LEVEL=3"`이다. `-DLOG_LEVEL=3`은 GCC 컴파일러 옵션이지 `make` 명령줄 옵션이 아니다. CubeMX 생성 Makefile에서는 `CFLAGS` 변수를 이용한다.

STM32CubeIDE는 Makefile 기반이 아닌 Eclipse CDT 기반 빌드를 주로 사용하므로, 이 예시는 순수 Makefile 프로젝트에만 해당됨을 명확히 해야 한다.

---

### [Minor-2] `_write()` 반환 타입 — POSIX 시그니처와 불일치

**위치**: §2.1 `_write()` 오버라이드 코드 (153행)

**문제**:
```c
int _write(int fd, char *ptr, int len)
```
Newlib의 `_write()` 시그니처는 `int _write(int fd, char *ptr, int len)`로 선언된다. 이 자체는 정확하다. 그러나 `ptr` 매개변수 타입이 `char *`가 아닌 `const char *`로 선언되어야 한다는 GCC 경고(`-Wmissing-prototype`, `-Wwrite-strings`)가 발생할 수 있다.

실제로 `syscalls.c` 내 Newlib 원본 약한 심볼(weak symbol) 선언은 `int _write(int file, char *ptr, int len)`이며 `const`가 없다. 그러나 내부에서 `ptr`을 수정하지 않으므로 `const char *`로 받는 것이 더 안전하다. 이는 엄격히 말해 Minor이지만, 교재 코드가 그대로 복사되어 사용되므로 기재할 필요가 있다.

---

### [Minor-3] `HAL_GetTick()` 오버플로 주기 계산 오류

**위치**: §2.3 타임스탬프 설명 (391행)

**문제**:
"약 49.7일 후에 오버플로되어 0으로 초기화됩니다"라는 설명은 정확하다.
그러나 "0으로 초기화됩니다"라는 표현이 약간 오해를 줄 수 있다. `uint32_t` 오버플로는 "초기화"가 아니라 자연적인 산술 래핑(wrapping)이다. `uwTick` 변수가 0으로 리셋되는 것이 아니라 `0xFFFFFFFF`에서 `0x00000000`으로 자연 래핑된다. 실무에서는 이 래핑이 발생해도 차이 연산(`now - last`) 방식을 사용하면 올바르게 동작한다는 점을 추가 언급하면 유용하다. Ch02의 디바운싱 코드가 이미 차이 연산 패턴을 사용하므로 연결 설명이 적절하다.

---

### [Minor-4] `__BASE_FILE__` 매크로 설명 부정확

**위치**: §2.4 aside.faq `__FILE__` 길이 문제 (550행)

**문제**:
"GCC의 `__BASE_FILE__` 매크로를 사용하는 방법도 있지만, 이식성이 떨어집니다"라는 설명 중 `__BASE_FILE__`의 역할이 부정확하다. `__BASE_FILE__`은 주 소스 파일의 이름을 반환하는 GCC 확장인데, 인클루드된 헤더 파일에서 호출되면 `.c` 파일 이름을 반환한다. 즉 이것은 `strrchr`로 경로를 잘라내는 것과 다른 동작이다. `__BASE_FILE__`은 파일명에서 디렉토리 경로를 제거하지 않는다. 정확히는 GCC `-fmacro-prefix-map` 옵션이나 `__FILE_NAME__`(GCC 12+, Clang) 매크로가 경로 없는 파일명을 제공한다.

**권장 수정**: `__BASE_FILE__` 언급을 제거하거나 정확한 동작 설명으로 교체. 대신 `__FILE_NAME__`(GCC 12+ / Clang)을 언급하고 이식성 제약을 설명하는 것이 더 정확하다.

---

### [Minor-5] 예상 로그 출력에서 함수명 잘림 표시 부정확

**위치**: §2.6 실습 예상 로그 출력 (1022~1028행)

**문제**:
```
[  1523][INFO ][main.c:38][HAL_GPIO_EX ] 버튼 클릭 감지 @ 1523 ms
```
`[%-12s]` 포맷으로 함수명을 12자로 제한할 때 `HAL_GPIO_EXTI_Callback`은 22자이므로 잘리지 않고 12자를 초과하여 그대로 출력된다. `%-12s`는 최소 12자 좌정렬이지, 최대 12자 절단이 아니다. 실제 출력은 `[HAL_GPIO_EXTI_Callback]`으로 나온다.

예상 출력에서 `[HAL_GPIO_EX ]`로 보여주는 것은 독자에게 잘못된 동작을 기대하게 만든다. 정확한 출력 또는 "함수명이 12자를 초과하면 필드 너비를 초과하여 그대로 출력됩니다"라는 주석이 필요하다.

---

### [Minor-6] 연습문제 4번 힌트 — `do-while` 패턴 언급 누락 설명

**위치**: §연습문제 4번 힌트 (1120행)

**문제**:
힌트에서 `do-while` 패턴을 언급하였으나, 정작 §2.4의 컴파일 타임 필터링 매크로(`#if` 방식 `LOG_D`) 최종 구현에는 `do { ... } while(0)` 래퍼가 적용되어 있지 않다. `log_print()` 단일 함수 호출이므로 `do-while` 없어도 안전하지만, 1단계/2단계 빌드업 예시(§2.4 1단계~3단계)에서 직접 `printf`를 쓰는 매크로에는 `do-while`이 없다. 이 패턴이 왜 필요한지(`if-else` 매크로 오용 방지) 본문에 설명이 없는 상태에서 연습문제 힌트에만 등장해 일관성이 부족하다.

§2.5의 런타임 if 필터링 예제에서는 `do { ... } while(0)`을 올바르게 사용하였다. 빌드업 단계 예시에서도 일관성 있게 사용하거나 이유를 설명하는 각주를 추가하는 것이 좋다.

---

## 긍정적 평가

1. **NUCLEO-F411RE 특이사항 정확 반영**: PB3 SWO 핀, SB12 솔더 브리지, SWV Core Clock 100MHz 일치 요건이 모두 정확하게 서술되어 있다.

2. **`ITM_SendChar()` 활성화 조건 확인 로직 포함**: `logger.c`의 ITM 채널 0 직접 호출이 CMSIS API를 그대로 활용하는 표준 방식으로 구현되어 있다.

3. **콜 체인 설명 정확성**: `printf → vfprintf → fwrite → _write → ITM_SendChar` 경로가 Newlib 아키텍처와 정확히 일치한다.

4. **`##__VA_ARGS__` 이식성 경고 명시**: GCC/Clang 전용임을 aside에서 명확히 경고하고, C23 `__VA_OPT__`와 STM32CubeIDE GCC 환경에서의 선택을 명확히 설명한 점이 우수하다.

5. **컴파일 타임 필터링 `((void)0)` 처리**: `LOG_D` 등 비활성 매크로를 `((void)0)`으로 정의하여 컴파일러 경고("사용되지 않은 인자") 없이 처리한 설계가 실무 수준에 부합한다.

6. **`logger.h`의 `#ifndef LOG_LEVEL` 가드**: CubeIDE Preprocessor Define에서 오버라이드 가능하도록 헤더 기본값에 가드를 씌운 설계가 올바르다.

7. **`basename()` 구현의 Windows/Linux 경로 구분자 처리**: `/`와 `\\` 모두 처리하는 점이 STM32CubeIDE(Windows 개발 환경) 기준에서 실용적이다.

8. **SEGGER RTT의 ST-LINK v2 지원 정확 언급**: NUCLEO 내장 ST-LINK v2가 RTT를 지원하며 RTT Viewer에서 연결 가능함을 정확히 설명하고 있다.

---

## 권장 수정사항 요약

우선순위 순으로 정렬:

| 우선순위 | 이슈 | 위치 | 내용 |
|---------|------|------|------|
| 1 | [Major-1] | `logger.c` `log_print()` | `snprintf` 반환값 누적 시 버퍼 오버플로 방지 클램핑 추가 |
| 2 | [Major-2] | §2.1 aside.tip | `ITM_SendChar()` busy-wait 발생 조건을 "ITM 활성화 + FIFO 가득 찬 경우"로 정확히 수정 |
| 3 | [Major-3] | §2.2 RTT 예제 | `SEGGER_RTT_printf()` 포맷 지정자와 인자 타입 일관성 수정 (`%u`/`%lu` 혼용 제거) |
| 4 | [Minor-5] | §2.6 실습 예상 로그 | `[%-12s]`가 절단이 아닌 최소 너비임을 명확히 하고 예상 출력 수정 |
| 5 | [Minor-1] | §2.5 make 명령줄 예시 | `make -DLOG_LEVEL=3` → `make CFLAGS+=-DLOG_LEVEL=3`으로 문법 수정 |
| 6 | [Minor-4] | §2.4 aside.faq | `__BASE_FILE__` 설명을 정확히 수정하거나 `__FILE_NAME__`(GCC12+)으로 교체 |
| 7 | [Minor-6] | §2.4 빌드업 예시 | 단계별 직접 `printf` 매크로에 `do-while` 래퍼 미사용 이유 설명 추가 또는 일관성 확보 |
| 8 | [Minor-2] | §2.1 `_write()` | `ptr` 매개변수 `const char *` 사용 여부 언급 (선택적) |
| 9 | [Minor-3] | §2.3 타임스탬프 | "0으로 초기화" → "자연 래핑(wrapping)" 표현 수정 및 차이 연산 안전성 언급 추가 |
