# Ch07 TIM 기초 — 초안 완성 보고서

## 작성일: 2026-03-17
## 작성자: 기술 저자

---

## 산출물 목록

### 1. HTML 원고 (1개)
- `manuscripts/part2/chapter07.html`
- 7개 절: §7.0~§7.6
- 예상 분량: ~18,000자

### 2. 코드 예제 (5개 파일)
| 파일 | 내용 | 줄수 |
|------|------|------|
| `code_examples/ch07_01_tim_period_calc.c` | PSC/ARR 조합별 주기 계산 검증 | ~50줄 |
| `code_examples/ch07_02_tim_interrupt_led.c` | TIM2 500ms UEV 인터럽트 LED 토글 | ~35줄 |
| `code_examples/ch07_03_pwm_led_brightness.c` | PWM 4단계 LED 밝기 제어 | ~60줄 |
| `code_examples/ch07_04_tim_driver.h` | tim_driver 공개 인터페이스 | ~85줄 |
| `code_examples/ch07_04_tim_driver.c` | tim_driver 내부 구현 | ~130줄 |

### 3. SVG 다이어그램 (7개)
| 파일 | 내용 |
|------|------|
| `figures/ch07_sec00_architecture.svg` | v0.7 아키텍처 위치도 (tim_driver 강조) |
| `figures/ch07_sec00_sequence.svg` | App → TIM Driver → HAL → TIM HW 시퀀스 |
| `figures/ch07_sec01_timer_hierarchy.svg` | F411 타이머 계층도 (GP/Advanced, Basic 없음) |
| `figures/ch07_sec02_psc_arr_gear.svg` | PSC·ARR 기어 비유 + 마스터 공식 |
| `figures/ch07_sec03_uev_timeline.svg` | UEV 발생 타임라인 (카운터 Up-count) |
| `figures/ch07_sec04_pwm_waveform.svg` | PWM 파형: CNT vs CCR 비교 |
| `figures/ch07_sec06_architecture_v07.svg` | v0.7 아키텍처 최종 현황도 |

---

## 검증 체크리스트

- [x] HTML 7절 모두 작성 (§7.0~§7.6)
- [x] 코드 4세트 작성 (HAL API 정확: HAL_TIM_Base_Start_IT, HAL_TIM_PWM_Start, __HAL_TIM_SET_COMPARE 등)
- [x] SVG 7개 색상 팔레트 일관 (#1A73E8 파랑/HAL, #34A853 초록/Driver, #FBBC04 노랑/Service, #EA4335 빨강/App)
- [x] 비유 3개가 기술과 1:1 매핑:
  - 메트로놈 = 타이머 (일정 간격 이벤트)
  - 기어비 = PSC/ARR (클럭 분주 + 주기 결정)
  - 수도꼭지 = PWM Duty Cycle (출력 비율)
- [x] 학습 목표 5개 모두 본문에서 명시적 달성:
  - L1 기억: §7.1 타이머 계층 표
  - L2 이해: §7.2 PSC/ARR/CCR 상호 관계
  - L3 적용: §7.2 계산 표 + §7.3~7.4 코드
  - L4 분석: §7.3 HAL_Delay vs TIM 비교 표
  - L5 평가: 연습문제 6번 시나리오별 선택
- [x] FAQ 반영: 총 7개 FAQ aside 배치
  - uart_driver와 tim_driver 같은 구조 이유
  - SysTick vs TIM2 차이
  - PSC/ARR 나누기 기준
  - HAL_TIM_Base_Start_IT vs _Start 차이
  - ISR 안에서 LOG_D 안전성
  - PWM 주파수 선택 기준
  - 콜백 함수 포인터 등록 이유
- [x] 면접 포인트 4개 배치
- [x] 강사 꿀팁 4개 배치
- [x] 스스로 점검 3세트 배치
- [x] 강사 막힘 포인트 대처:
  - PSC/ARR 계산 → §7.2 역방향 계산법 + 강사 꿀팁
  - HAL_Delay 왜 안 되나 → §7.0 도입 + §7.3 비교 표
  - 핀 충돌 → §7.4 핀 맵 표 + 실무 팁
  - ISR 안 로그 → §7.3 FAQ (링 버퍼 연결)
  - Driver 캡슐화 이유 → §7.5 패턴 비교 + FAQ

---

## 이전 챕터 연결

| 연결 대상 | 연결 위치 | 방식 |
|-----------|----------|------|
| Ch01 GPIO/EXTI | §7.3 LED 토글, §7.6 버튼 통합 | 코드 재사용 |
| Ch02 로그 | 전체 코드 LOG_D/I/W/E | 표준 적용 |
| Ch03 아키텍처 | §7.0 도입, §7.5 패턴, §7.6 v0.7 다이어그램 | 레이어 강조 |
| Ch04 UART | §7.5 uart_driver vs tim_driver 패턴 비교 표 | 패턴 답습 |
| Ch05 DMA | §7.1 APB 버스 참조 | 용어 연결 |
| Ch06 링 버퍼 | §7.3 ISR 안 로그 안전성 FAQ | 기능 연결 |

---

## Service 레이어 처리
- 본문에서 Service 레이어는 "예고"만 포함 (§7.6 다음 챕터 예고)
- Ch08에서 스톱워치 FSM과 함께 Service 레이어 첫 도입 예고

---

## Phase 3 리뷰 요청 사항
1. F411RE APB1 타이머 클럭 84MHz 정확성 검증 (CubeMX 기본 설정 기준)
2. TIM2_CH1 PWM 핀 PA0 할당 정확성
3. tim_set_period_ms() 내부 PSC/ARR 계산 로직 검증
4. EGR UG 비트 소프트웨어 UEV 부작용 검증
5. 연습문제 난이도 적절성
