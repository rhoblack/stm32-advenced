# Ch03 최종 수정 완료 보고

**작성자**: 기술 저자 (Technical Author)
**작성일**: 2026-03-17
**Phase**: 4 (종합 회의 — 리뷰 피드백 반영 완료)

---

## 필수 수정 (CRITICAL) — 2/2 완료

### M1. ASCII Art → SVG 변환 ✅
- **문제**: §3.5 시퀀스 다이어그램이 `<pre><code>` ASCII art로 작성됨 (CLAUDE.md 위반)
- **수정**:
  - `figures/ch03_sec05_sequence.svg` 신규 생성 (버튼→LED 시퀀스 다이어그램)
  - `manuscripts/part0/chapter03.html` §3.5에서 ASCII art를 `<figure><img>` 태그로 교체
  - figcaption "그림 03-7" 추가
- **검증**: SVG 파일 존재 확인, HTML에서 ASCII `<pre>` 블록 제거 확인

### M2. SRAM 오버플로우 명시 ✅
- **문제**: `ch03_01_spaghetti_demo.c`의 `g_lcd_framebuf[320*240*2]` = 153KB > F411RE SRAM 128KB
- **수정**: 변수 선언 직후에 주석 추가
  ```c
  /* ⚠️ 의도적 데모 코드: 153KB > F411RE SRAM 128KB → 실제로는 빌드 불가!
   *    실전에서는 DMA 스트리밍으로 부분 전송한다 (Ch12~13 참조) */
  ```
- **검증**: `code_examples/ch03_01_spaghetti_demo.c` 33~35행 확인

---

## 권장 수정 — 3/3 완료

### R1. §3.1 도입부 이전 성취 확인 문장 추가 ✅
- **문제**: 교육심리전문가 권고 — 새 챕터 시작 전 이전 성취 인정 필요
- **수정**: §3.1 첫 문단 앞에 4줄 추가
  - "잠깐, 여기까지 온 여러분을 축하합니다."
  - "Ch01에서 버튼으로 LED를 토글하고, Ch02에서 LOG_I 매크로로 MCU 내부를 들여다보는 데 성공했습니다."
  - "GPIO와 인터럽트, 로그 시스템이라는 임베디드 개발의 기초 체력을 갖춘 것입니다."
  - "이제 그 체력 위에 설계도를 세울 차례입니다."
- **검증**: `chapter03.html` §3.1 도입부 확인

### R2. Driver/Service 경계 FAQ 추가 ✅
- **문제**: 교육강사 권고 — "Driver와 Service 경계" 질문이 매 강의에서 발생
- **수정**: §3.2의 횡단 관심사 FAQ 직후에 새 FAQ aside 추가
  - Q: "Driver와 Service의 경계가 헷갈립니다. 어떻게 구분하나요?"
  - A: 핵심 기준 "하드웨어를 아는가?" + sht31_driver vs sensor_service 예시 + 레스토랑 비유 연결
- **검증**: `chapter03.html` §3.2 내 faq aside 개수 확인 (2→3개)

### R3. 레스토랑 비유 동시성 한계 추가 ✅
- **문제**: 기술 리뷰어/교육강사 권고 — 동시성(인터럽트/DMA) 한계 미언급
- **수정**: 비유 한계 `<small>` 태그에 "(2) 동시성" 항목 1줄 추가
  - "레스토랑에서는 주문이 순서대로 처리되지만, 임베디드 시스템에서는 인터럽트/DMA가 요리 도중에 끼어드는 '동시성'이 존재합니다. Ch05/Ch06에서 다룹니다."
- **검증**: `chapter03.html` §3.2 비유 블록 내 small 태그 확인

---

## 수정된 파일 목록

| 파일 | 수정 내용 | 수정 유형 |
|------|----------|----------|
| `figures/ch03_sec05_sequence.svg` | 신규 생성 | M1 |
| `manuscripts/part0/chapter03.html` | ASCII art→SVG, 도입부 추가, FAQ 추가, 비유 한계 추가 | M1+R1+R2+R3 |
| `code_examples/ch03_01_spaghetti_demo.c` | SRAM 오버플로우 주석 추가 | M2 |

---

## 최종 산출물 총 목록 (SVG 7개로 증가)

- `manuscripts/part0/chapter03.html`
- `figures/ch03_sec01_spaghetti_vs_layered.svg`
- `figures/ch03_sec02_restaurant_layers.svg`
- `figures/ch03_sec02_four_layers.svg`
- `figures/ch03_sec03_v02_mapping.svg`
- `figures/ch03_sec04_button_fsm.svg`
- `figures/ch03_sec05_architecture_v03.svg`
- `figures/ch03_sec05_sequence.svg` ← 신규
- `code_examples/ch03_01_spaghetti_demo.c`
- `code_examples/ch03_02_layer_interfaces.h`
- `code_examples/ch03_03_v02_layer_analysis.c`

최종 승인을 요청합니다.
