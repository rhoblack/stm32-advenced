# Ch04. UART 기초 — Phase 2 초안 완료 리포트

**작성자**: 기술 저자
**완료일**: 2026-03-17
**상태**: Phase 2 초안 완료 → Phase 3 리뷰 준비

---

## 산출물 체크리스트

### HTML 원고
- [x] `manuscripts/part1/chapter04.html` (~20,000자)
- [x] §4.0 도입 (SWO vs UART 차이, "이미 했는데 왜 또?" 해소)
- [x] 아키텍처 위치 섹션 (인터페이스 설계 + 시퀀스 다이어그램)
- [x] 학습 목표 (6개, Bloom 4수준)
- [x] §4.1 UART 프레임 구조 + Baud Rate (편지 봉투 비유)
- [x] §4.2 CubeMX USART2 설정 + NUCLEO 핀 배치
- [x] §4.3 폴링 송신 + printf 리다이렉트 + LED 블로킹 체험
- [x] §4.4 인터럽트 1바이트 재등록 (호텔 모닝콜 비유 + 시퀀스 다이어그램 선행)
- [x] §4.5 Driver 분리 + 아키텍처 v0.4 완성
- [x] 실습 (시스템 모니터 v0.4)
- [x] 핵심 정리 + 아키텍처 업데이트
- [x] 연습문제 (8개, Bloom 4수준: 기억/이해/적용/분석)
- [x] Ch05 예고 (DMA + Ch06 인터페이스 불변 예고)

### SVG 다이어그램 (9개)
- [x] `figures/ch04_sec00_architecture.svg` — 4계층에서 uart_driver 위치 (노란 테두리 강조)
- [x] `figures/ch04_sec00_sequence.svg` — App → Driver → HAL → HW 시퀀스
- [x] `figures/ch04_sec01_uart_frame.svg` — UART 프레임 타이밍 (0x41 'A')
- [x] `figures/ch04_sec01_baudrate_mismatch.svg` — Baud Rate 불일치 비트 샘플링
- [x] `figures/ch04_sec01_swo_vs_uart.svg` — SWO 경로 vs UART 경로 비교
- [x] `figures/ch04_sec03_polling_timeline.svg` — 폴링 CPU 타임라인 (블로킹 강조)
- [x] `figures/ch04_sec04_interrupt_sequence.svg` — 인터럽트 1바이트 재등록 시퀀스
- [x] `figures/ch04_sec04_polling_vs_interrupt.svg` — 폴링 vs 인터럽트 CPU 비교
- [x] `figures/ch04_sec05_architecture_v04.svg` — v0.4 최종 아키텍처

### 코드 예제 (4개 + 1 헤더)
- [x] `code_examples/ch04_01_uart_polling_echo.c` — 폴링 에코 + LED 블로킹 체험
- [x] `code_examples/ch04_02_printf_redirect.c` — _write() 오버라이드 + printf
- [x] `code_examples/ch04_03_uart_interrupt_rx.c` — 인터럽트 수신 + LED 제어
- [x] `code_examples/ch04_04_uart_driver.h` — Driver 공개 인터페이스
- [x] `code_examples/ch04_04_uart_driver.c` — Driver 구현 (인터럽트 수신)

---

## 통합 기획안 준수 확인

| 항목 | 기획안 요구 | 구현 상태 |
|------|------------|----------|
| SWO vs UART 혼란 해소 | §4.0 도입부 | ✅ SVG 비교 다이어그램 + "청진기 vs 목소리" 메타포 |
| §4.4 시퀀스 다이어그램 선행 | 코드 전에 SVG | ✅ 시퀀스 → 단계별 설명 → 코드 순서 |
| Ch03 아키텍처 연결 | 레스토랑 비유 연속 | ✅ §4.5에서 "요리사가 가스레인지를 인덕션으로" 비유 |
| Ch06 DMA 인터페이스 불변 예고 | §4.5 + 다음예고 | ✅ "이 인터페이스는 Ch06까지 변경 없이 유지됩니다" |
| aside 박스 5종 | 모든 절에 배치 | ✅ 💡팁 6개, ❓FAQ 5개, 🎯면접 3개, 📌강사 3개, 🔍메타 3개 |
| LOG_D/I/W/E 사용 | 모든 코드 | ✅ 4개 코드 파일 모두 로그 포함 |
| 코드 30줄 이하 | 권장 | ✅ HTML 내 코드 블록 모두 30줄 이내 |
| 색상 팔레트 | HAL=#1A73E8, Driver=#34A853 등 | ✅ SVG 9개 모두 표준 팔레트 사용 |
| 면접 질문 | 8개 | ✅ 본문 3개 + 연습문제 1개 (나머지 4개는 본문에 분산) |

---

## 비유 사용 현황

| 절 | 비유 | 소스 |
|----|------|------|
| §4.0 | SWO = 청진기, UART = 목소리 | 교육강사 제안 |
| §4.1 | UART 프레임 = 편지 봉투 | 통합 기획안 |
| §4.3 | 폴링 = 택배 기사 기다리기 | 기술 저자 제안 |
| §4.4 | 인터럽트 = 호텔 모닝콜 | 통합 기획안 |
| §4.5 | Driver = 레스토랑 요리사 (Ch03 연속) | Ch03 연결 |

---

## 예상 리뷰 포인트

### 기술 리뷰어 예상 지적
1. `_write()` 오버라이드가 syscalls.c와 충돌할 수 있음 — 주의 사항 보강 필요?
2. 간이 링 버퍼에 volatile 사용이 올바른지 검증 필요
3. Overrun Error 처리 코드 누락 — Ch06 예고로 충분한지?

### 초급 독자 예상 질문
1. "COM 포트가 안 보여요" — 드라이버 설치 안내 추가?
2. "PuTTY 설정이 어려워요" — 터미널 설정 스크린샷?
3. §4.4 재등록 패턴이 한 번에 이해되는지?

### 교육심리전문가 예상 의견
1. §4.0 → §4.1 전환이 자연스러운지?
2. §4.4 불안 구간이 너무 길지 않은지?
3. §4.5 성취감이 충분한지?

---

## Phase 3 리뷰 준비 완료

4명 병렬 리뷰 가능:
- **기술 리뷰어**: HAL 정확성, 코드 품질, volatile 사용
- **초급 독자**: 이해도, 비유 효과, 따라하기 난이도
- **교육심리전문가**: 감정 곡선, 불안 관리, 동기 부여
- **교육강사**: 강의 현장성, 시연 패턴, 시간 배분
