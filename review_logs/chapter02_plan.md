# Ch02. 로그 시스템 구축 — 기획안 (v0.2)

**작성일:** 2026-03-16 | **버전:** v0.2 | **작성:** 기술 저자 + 교육 설계자 + 교육전문강사 통합

---

## 1. 절 구성 (6절)

| 절 | 제목 | 핵심 메시지 | 새 개념 | 예상 분량 |
|----|------|------------|---------|-----------|
| §2.1 | 디버깅의 눈을 열다 — SWO/ITM과 printf 리다이렉트 | SWO 핀 하나로 MCU 내부 상태를 IDE 콘솔에 실시간 출력한다 | SWO/ITM, `_write()` 오버라이드, SWV 설정 | 3,200자 |
| §2.2 | 느린 SWO의 대안 — SEGGER RTT 활용 | SEGGER RTT는 별도 핀 없이 메모리 버퍼만으로 초고속 로그 출력을 가능하게 한다 | RTT 동작 원리, RTT Viewer 연결, SWO vs RTT 비교 | 2,800자 |
| §2.3 | 로그에도 등급이 있다 — 로그 레벨 설계 | 4단계 로그 레벨로 심각도를 구분하고 불필요한 정보 홍수를 방지한다 | 로그 레벨 계층(ERROR/WARN/INFO/DEBUG), 포맷 설계, `HAL_GetTick()` 타임스탬프 | 2,400자 |
| §2.4 | LOG_E/W/I/D 매크로 설계 — C 전처리기의 실무 활용 | 가변 인자 매크로로 printf 수준의 편의성과 로그 레벨 필터링을 동시에 구현한다 | `##__VA_ARGS__`, `__FILE__`/`__LINE__`/`__func__`, 조건부 컴파일 구조 | 3,500자 |
| §2.5 | 빌드 타임에 로그를 끈다 — 컴파일 타임 레벨 필터링 | `#define LOG_LEVEL` 한 줄로 Debug/Release 빌드 로그 출력량 제어 및 플래시 절약 | `#if` 비교 치환 원리, CubeIDE 컴파일 옵션, .map 파일 크기 비교 | 2,600자 |
| §2.6 | 로그 시스템 통합 — v0.2 프로젝트 완성 | Ch01 GPIO/EXTI 코드에 표준 로그 인프라를 적용해 모든 이벤트를 추적 가능하게 만든다 | `logger.h/c` 모듈 설계, Ch01 before/after 비교 | 2,000자 |

**총 분량:** 약 16,500자 / 강의 4시간

---

## 2. 절별 비유/실생활 예시

| 절 | 비유 | 실생활 예시 |
|----|------|------------|
| §2.1 SWO/ITM | **공항 관제탑+블랙박스** — SWD(관제탑 교신)와 SWO(블랙박스 기록)가 동시에 동작 | 자동차 OBD-II 포트 — 외부 장비 연결 시에만 ECU 내부 상태 확인 가능 |
| §2.2 RTT | **공유 화이트보드** — MCU와 디버거가 같은 메모리 영역을 공유해 통신, 전용 선 불필요 | 공항 수하물 벨트 — 짐(로그)이 올라오면 반대편에서 빼가는 순환 구조 |
| §2.3 로그 레벨 | **병원 응급실 트리아지** — ERROR(즉시 수술)/WARN(빠른 처치)/INFO(일반 진료)/DEBUG(정기 검진) | 스마트폰 알림 설정 — 모든 알림 켜면 중요 알림 놓치듯, 로그도 필터링 필요 |
| §2.4 매크로 | **스탬프(도장)** — LOG_E 한 줄이 파일명·라인·함수명·메시지가 새겨진 도장 하나 | 카페 영수증 POS기 — 주문(메시지)하면 매장명·시각·번호(컨텍스트)를 자동으로 붙임 |
| §2.5 컴파일 필터링 | **항공기 블랙박스 기록 밀도 설정** — 개발 중 고밀도, 배포 후 이상 신호만 | 사진 RAW vs JPEG — 스튜디오(개발)는 RAW, SNS 공유(배포)는 JPEG |
| §2.6 통합 | **공장 생산라인 CCTV 설치** — Ch01이 라인 구축, Ch02는 모든 공정에 감시 카메라 설치 | — |

---

## 3. HAL 코드 예제 목록

| 파일명 | 목적 | 핵심 함수/API |
|--------|------|---------------|
| `code_examples/ch02_01_swo_printf.c` | SWO/ITM `printf` 리다이렉트 최소 구현 | `ITM_SendChar()`, `_write()`, `setvbuf()` |
| `code_examples/ch02_02_rtt_basic.c` | SEGGER RTT 기초 출력 | `SEGGER_RTT_Init()`, `SEGGER_RTT_printf()` |
| `code_examples/ch02_03_log_level_design.c` | 4단계 로그 레벨 개념 증명 (ANSI 색상 포함) | `HAL_GetTick()`, `printf()` |
| `code_examples/ch02_04_log_macro.h` | `LOG_E/W/I/D` 매크로 완성 구현 | `#define LOG_E(fmt, ...)`, `##__VA_ARGS__` |
| `code_examples/ch02_05_compile_time_filter.c` | `#define LOG_LEVEL` 필터링 데모 + .map 크기 측정 | `#if LOG_LEVEL >= X`, CubeIDE Preprocessor Defines |
| `code_examples/ch02_06_logger_module.c` | `logger.c/h` 최종 통합 모듈 | `Log_Init()`, `Log_SetBackend()`, 전체 LOG 매크로 |
| `code_examples/ch02_07_v02_integrated.c` | v0.2 완성본 — Ch01 코드 + Ch02 로그 통합 | `HAL_GPIO_EXTI_Callback()` 내 `LOG_I()`, `LOG_D()` |

---

## 4. SVG 다이어그램 목록 (파일명 확정)

> ⚠️ Phase 2 HTML 집필 시 아래 파일명을 **그대로** 사용할 것 (불일치 방지)

| 파일명 | 내용 | 핵심 개념 |
|--------|------|-----------|
| `figures/ch02_sec01_swo_signal_path.svg` | SWO 신호 경로 블록도: CPU Core → ITM → TPIU → SWO 핀 → ST-LINK → USB → PC CubeIDE SWV | ITM 트레이스의 물리적 경로 전체 흐름 |
| `figures/ch02_sec01_swv_cubemx_settings.svg` | CubeIDE Debug 설정 스크린샷 재현: SWO clock 설정 (Core Clock 100MHz 기준), SWV 활성화 순서 | 단계별 설정 가이드 |
| `figures/ch02_sec02_rtt_memory_layout.svg` | RTT 메모리 구조: MCU RAM 내 `_SEGGER_RTT` 구조체, Up/Down 링 버퍼, J-Link 폴링 방향 | 별도 핀 없이 동작하는 이유, SWO 방식과 아키텍처 차이 |
| `figures/ch02_sec02_swo_vs_rtt_compare.svg` | SWO vs RTT 비교 표+다이어그램: 핀 필요 여부, 대역폭, CPU 오버헤드, 설정 복잡도, 블로킹 여부 | 두 방식 트레이드오프, 선택 기준 |
| `figures/ch02_sec03_log_level_hierarchy.svg` | 로그 레벨 피라미드: ERROR(빨강)→WARN(주황)→INFO(초록)→DEBUG(회색), LOG_LEVEL=2 설정 시 차단 시각화 | 레벨별 중요도, 필터링 시 어떤 메시지가 사라지는지 |
| `figures/ch02_sec03_log_format_anatomy.svg` | 로그 출력 한 줄 해부: `[1234][ERROR][main.c:45][main] 메시지` → 각 필드 화살표 설명 | 각 정보의 출처(HAL_GetTick, __FILE__, __LINE__, __func__) |
| `figures/ch02_sec04_macro_expansion.svg` | C 전처리기 매크로 전개: `LOG_D("val=%d", x)` → 전처리기 → 컴파일러 → 바이너리 포함/제거 분기 | `##__VA_ARGS__` 동작, 빈 코드로 치환 vs 실제 코드 분기 |
| `figures/ch02_sec05_flash_size_comparison.svg` | 컴파일 타임 필터링 효과 막대 그래프: LOG_LEVEL 0/1/2/3/4별 .text 섹션 크기(바이트) 비교 | 로그 레벨이 플래시 크기에 미치는 실질적 영향 |
| `figures/ch02_sec06_logger_module_architecture.svg` | logger 모듈 레이어 다이어그램: App(LOG 매크로) → logger.c(백엔드 선택) → SWO/RTT/UART(예비) 백엔드 → 물리 출력 | 백엔드 추상화 설계 패턴, Ch06 UART DMA 백엔드 확장 예고 |

---

## 5. 학습 목표 (블룸 분류체계)

| # | 학습 목표 | 블룸 수준 | 달성 절 |
|---|-----------|-----------|---------|
| LO-1 | SWO/ITM과 SEGGER RTT의 동작 원리를 **설명**하고, 두 방식의 기술적 차이를 **비교**할 수 있다. | 이해 | §2.1, §2.2 |
| LO-2 | STM32CubeIDE에서 SWO/ITM printf 리다이렉트를 **구성**하고 로그 출력을 **확인**할 수 있다. | 적용 | §2.1 |
| LO-3 | ERROR/WARN/INFO/DEBUG 4단계 로그 레벨 구조를 **설계**하고, LOG_E/W/I/D 매크로를 **구현**할 수 있다. | 적용 + 분석 | §2.3, §2.4 |
| LO-4 | 컴파일 타임 레벨 필터링 메커니즘을 **분석**하여, 빌드 목적(Debug/Release)에 따른 필터 레벨을 **선택**하고 **근거**와 함께 **설명**할 수 있다. | 분석 + 평가 | §2.5 |
| LO-5 | 구축한 로그 시스템을 실제 프로젝트에 **통합**하고, 로그 출력을 보고 버그 위치를 **추론**할 수 있다. | 평가 + 창조 | §2.6 |

---

## 6. 강의 흐름 (4시간 = 240분)

| 시간 | 내용 | 이론 | 실습 |
|------|------|------|------|
| 0:00~0:10 | 도입 — Ch01 복습 + "LED만 봐서는 부족하다" 문제 제기, 실무 사례 | 10분 | — |
| 0:10~0:55 | §2.1 SWO/ITM 원리 + CubeIDE SWV 설정 + `_write()` 실습 | 20분 | 25분 |
| 0:55~1:30 | §2.2 SEGGER RTT 개념 + SWO vs RTT 비교 (개념 중심) | 15분 | 20분 |
| 1:30~2:30 | §2.3 로그 레벨 설계 + §2.4 LOG 매크로 단계별 구현 | 25분 | 35분 |
| 2:30~3:15 | §2.5 컴파일 타임 필터링 + .map 크기 비교 실험 | 20분 | 25분 |
| 3:15~3:45 | §2.6 v0.2 통합 실습 — Ch01 EXTI 코드에 로그 적용 | — | 30분 |
| 3:45~4:00 | 면접 포인트 3개, 핵심 정리, Ch06 예고(링 버퍼 비동기 로그) | 10분 | 5분 |
| **합계** | | **100분(42%)** | **140분(58%)** |

---

## 7. 수강생 막힘 포인트 및 사전 해소

| # | 막힘 포인트 | 사전 해소 방법 |
|---|-------------|----------------|
| 1 | SWO가 "왜 별도 핀인가" — SWD와 SWO 역할 혼재 | 보드 핀맵에서 SWO 핀(PB3) 직접 찾기 실습 후 코드 진입 |
| 2 | `_write()` 오버라이드 — "어떻게 printf와 연결되나" | printf → vfprintf → fwrite → `_write()` 콜 체인 다이어그램 칠판 작도 |
| 3 | RTT vs SWO 개념 혼재 + ST-Link 호환성 의문 | 비교 표를 먼저 제시, "이 챕터는 SWO 주력, RTT는 개념만" 범위 선언 |
| 4 | `##__VA_ARGS__`, `__FILE__`/`__LINE__` — 전처리기 마법 | 3단계 분해: __FILE__ 단독 → 가변인자 매크로 → ##__VA_ARGS__ 순 |
| 5 | 컴파일 타임 제거 vs 런타임 if문 차이 인식 부재 | `if(0){}` 버전 vs `#if 0` 버전 빌드 후 .map 파일 크기 직접 비교 |

---

## 8. 면접 연결 포인트 (3개)

1. **로그 시스템 설계 경험** — 컴파일 타임 필터링, 메타 정보 자동 삽입, 백엔드 추상화 레이어 설계
2. **ARM Cortex-M 디버그 인터페이스** — SWD(제어)와 SWO(관찰) 역할 분리, ITM 채널 구조
3. **C 전처리기 최적화** — 가변인자 매크로(`__VA_ARGS__`), 컴파일 타임 코드 제거, `#if`/`#endif` 활용

---

## 9. 누적 성장 연결

### Ch01 → Ch02 연결
- **기반 코드 재사용**: Ch01 `HAL_GPIO_EXTI_Callback()` 코드에 `LOG_I()`/`LOG_D()` 추가가 §2.6 실습
- **HAL_GetTick() 재사용**: Ch01 디바운싱에서 쓴 타임스탬프를 로그 포맷에 재활용
- **감정 전환**: Ch01 마지막 성취감 → "이 코드가 제대로 동작하는지 어떻게 확인했나요?" 질문으로 새로운 호기심 유도

### Ch02 → 이후 챕터 영향
- **모든 챕터 공통 인프라**: `logger.h`를 Ch03~Ch19 모든 코드 예제에서 `#include` 사용
- **Ch04**: printf 리다이렉트 백엔드 SWO → UART 전환 실습 (`Log_SetBackend()` 활용)
- **Ch06**: RTT/SWO 백엔드 → UART DMA 링 버퍼 백엔드 추가 (Ch02에서 예고한 완성)
- **Ch17**: `logger.c`가 횡단 관심사(cross-cutting concern)의 대표 사례로 등장
- **Ch18**: SWO ITM + SEGGER RTT 고급 활용 심화 (Ch02 기초 위에 구축)

---

## 10. NUCLEO-F411RE 특이사항

| 이슈 | 내용 | 교재 처리 |
|------|------|-----------|
| SWO 핀 = PB3 | NUCLEO SB12 점퍼 기본 연결 확인 필요 — 미연결 시 SWV 무반응 | 실습 전 체크리스트 + aside 경고 박스 |
| SWO Core Clock 설정 | CubeIDE SWV → Core Clock을 실제 HSE 기반 클럭(100MHz)과 일치 — 불일치 시 글자 깨짐 | §2.1 설정 단계에서 "Core Clock 반드시 일치" 강조 박스 |
| SEGGER RTT + ST-LINK v2 | NUCLEO 내장 ST-LINK v2는 RTT 지원 (RTT Viewer에서 ST-LINK 모드 선택) | §2.2 설정 시 "NUCLEO는 ST-LINK 모드 선택" 명시 |
| ITM ISR 안전성 | `ITM_SendChar()`는 ISR 내 호출 가능하나 블로킹 — Busy-Wait 주의 | §2.1 주의사항 박스: "ISR 내 LOG_D 남용 금지" |

---

## 11. 연습문제 계획 (블룸 5수준)

| # | 수준 | 내용 |
|---|------|------|
| 문제 1 | 기억/이해 | SWO/ITM과 SEGGER RTT 특징 연결 (보기 매칭) |
| 문제 2 | 이해 | `LOG_LEVEL=LOG_WARN`일 때 출력되는 라인 고르기 + 컴파일 타임 필터링 원리 설명 |
| 문제 3 | 적용 | `LOG_I` 매크로 직접 작성 (형식, 컴파일 조건, 출력 함수, 가변인자 포함) |
| 문제 4 | 분석 | 팀원의 잘못된 매크로 구현 코드에서 문제점 2개 이상 찾고 개선 방안 제시 |
| 문제 5 | 평가/창조 | IoT 배터리 센서 노드의 Debug/Release 로그 전략 설계 (출력 채널 선택 + 근거 제시) |
