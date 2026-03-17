# Ch05 초안 집필 완료 보고 — 기술 저자

**챕터**: Ch05. DMA 아키텍처와 동작 원리
**프로젝트 버전**: v0.5
**완료 시점**: Phase 2 초안 집필

---

## 1. 산출물 목록

### HTML 원고
- `manuscripts/part1/chapter05.html` (~18,500자, 7개 절 + 실습 + 연습문제)

### 코드 예제 (3개)
| 파일 | 내용 | 줄 수 |
|------|------|-------|
| `code_examples/ch05_01_memcpy_polling.c` | 폴링 메모리 복사 + DWT 측정 | ~80줄 |
| `code_examples/ch05_02_dma_mem_to_mem.c` | DMA Mem-to-Mem 전송 + 콜백 | ~100줄 |
| `code_examples/ch05_03_performance_compare.c` | 폴링 vs DMA 성능 비교 + LED 시각화 | ~130줄 |

### SVG 다이어그램 (8개)
| 파일 | 내용 |
|------|------|
| `figures/ch05_sec00_architecture.svg` | v0.5 아키텍처 — DMA 레이어 추가 |
| `figures/ch05_sec00_sequence.svg` | DMA Mem-to-Mem 시퀀스 다이어그램 |
| `figures/ch05_sec01_dma_controller.svg` | DMA1/DMA2 Stream/Channel 구조 |
| `figures/ch05_sec01_stream_channel_map.svg` | 교재 주변장치 DMA 매핑 테이블 |
| `figures/ch05_sec02_polling_vs_dma_timeline.svg` | CPU 타임라인 비교 (폴링 vs DMA) |
| `figures/ch05_sec03_mem_to_mem_flow.svg` | Mem-to-Mem 전송 흐름도 |
| `figures/ch05_sec04_normal_vs_circular.svg` | Normal vs Circular 모드 비교 |
| `figures/ch05_sec05_nvic_priority.svg` | v0.5 인터럽트 우선순위 배치 |
| `figures/ch05_sec05_architecture_v05.svg` | v0.5 아키텍처 최종 (핵심 정리용) |

---

## 2. 절 구조 및 분량

| 절 | 제목 | 예상 분량 | 주요 내용 |
|----|------|-----------|-----------|
| §5.0 | 도입 + 아키텍처 위치 | ~2,500자 | CPU 블로킹 문제 제기, 서빙 로봇 비유, v0.5 아키텍처, 시퀀스 |
| §5.1 | DMA 컨트롤러 해부 | ~3,000자 | DMA1/DMA2, Stream/Channel/Priority, 공항 비유 |
| §5.2 | 폴링 메모리 복사 | ~2,500자 | DWT Cycle Counter, for 루프 복사, 기준선 측정 |
| §5.3 | DMA 메모리 복사 | ~3,500자 | CubeMX 설정, HAL_DMA_Start_IT, 콜백, volatile |
| §5.4 | 성능 대결 | ~3,000자 | 100회 반복 측정, LED 시각화, 면접 비교 테이블 |
| §5.5 | NVIC 심화 | ~3,000자 | 우선순위 배치, Priority Inversion, Critical Section |
| §5.6 | 정리 + Ch06 예고 | ~1,500자 | 3가지 전송 모드, Normal/Circular, Ch06 복선 |
| **합계** | | **~18,500자** | |

---

## 3. aside 박스 배치

| 유형 | 개수 | 위치 |
|------|------|------|
| 💡 실무 팁 | 3개 | §5.2(DWT), §5.5(NVIC 전략), 실습(안 될 때) |
| ❓ 단골 질문 | 4개 | §5.1(Stream vs Channel), §5.3(버스 경합), §5.3(volatile), §5.5(인터럽트 끄면 DMA도?) |
| 🎯 면접 포인트 | 2개 | §5.1(Stream/Channel), §5.4(폴링/인터럽트/DMA 비교) |
| 📌 강사 꿀팁 | 3개 | §5.0(CPU 블로킹 시연), §5.2(LCD 크기), §5.4(예측 퀴즈), 실습(축소안) |
| 🔍 스스로 점검 | 2개 | §5.3(콜백 미호출), §5.6(전송 모드-챕터 매핑) |

---

## 4. 연습문제 (8개, 블룸 분류체계)

| 수준 | 문제 번호 | 내용 |
|------|----------|------|
| 기억 | 1, 2 | 전송 모드 나열, DMA2 Mem-to-Mem 제약 |
| 이해 | 3, 4 | CPU 점유율 차이, Normal vs Circular |
| 적용 | 5, 6 | 버퍼 크기 변경 측정, 검증 코드 추가 |
| 분석 | 7, 8 | Priority 변경 영향, 면접 대비 3종 비교 |

---

## 5. Ch04 연계 포인트

1. **Ch04 연습문제 8번 완성**: "DMA는 Ch06에서 학습 예정"이라고 보류한 답을 §5.4 면접 포인트에서 완전 제공
2. **uart_send_string() 재사용**: 성능 결과를 PC 터미널에 출력 (Ch04 Driver 레이어 활용)
3. **v0.4 → v0.5 아키텍처 연속성**: DMA Controller가 횡단 관심사로 추가
4. **Ch06 복선 강화**: "uart_driver.h 인터페이스 변경 없이 DMA 전환" 반복 강조

---

## 6. 비유 체계

| 비유 | 출처 | 적용 |
|------|------|------|
| 레스토랑 + 서빙 로봇 | Ch03 확장 | §5.0 DMA 도입 |
| 공항 컨베이어 시스템 | 신규 | §5.1 Stream/Channel/Priority |
| 응급실 대기 | 신규 | §5.5 Priority Inversion |
| 회전초밥 벨트 | 신규 | §5.6 Circular 모드 |

---

## 7. 기술 정확성 확인 사항 (리뷰어 검증 필요)

1. **DMA2 Mem-to-Mem 전용**: DMA1에서는 Mem-to-Mem 불가 — RM0383 참조 필요
2. **DWT CYCCNT 초기화**: CoreDebug DEMCR TRCENA 비트 설정 필요 — 디버거 미연결 시 동작 확인
3. **HAL_DMA_Start_IT() BUF_SIZE 인자**: Data Width Word 설정 시 word 개수 전달 — byte 아님
4. **DMA Stream/Channel 매핑**: STM32F411 RM0383 Table 28/29 기준 — 정확성 확인 필요
5. **NVIC Priority 배치**: DMA(1) > UART(2) > EXTI(3) 순서 — 실무 적합성 확인 필요

---

## 8. 리뷰 요청 사항

- **기술 리뷰어**: DMA 레지스터 접근, HAL API 사용법, DWT 초기화 정확성
- **초급자 독자**: §5.1 DMA 구조가 한 번에 이해되는지, 공항 비유 적절성
- **교육심리전문가**: §5.2→§5.3 난이도 점프 적절성, 성능 측정의 동기 유발 효과
- **교육전문강사**: 4시간 강의 흐름, 축소안 실효성, 면접 포인트 현장 적합성
